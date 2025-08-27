/**
 * @file robust_file_handler.cpp
 * @brief Implementation of robust file handling utilities
 */

#include "solar_test/utils/robust_file_handler.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <string.h>
#include <unistd.h>
#endif

namespace SolarSystem::Testing::Utils {

// RobustFileStream implementation
RobustFileStream::RobustFileStream(const std::string& filename, Mode mode,
                                   const FileOperationConfig& config)
    : mode_(mode), config_(config) {
  auto start_time = std::chrono::steady_clock::now();

  // Try to open the file with retry mechanism
  bool opened = false;
  int attempt = 0;
  auto delay = config_.retry_delay;

  while (attempt <= config_.max_retries && !opened) {
    result_.attempted_paths.clear();

    // Try original path first
    if (attempt_open(filename)) {
      opened = true;
      result_.final_path = filename;
      break;
    }
    result_.attempted_paths.push_back(filename);

    // Try fallback directories
    for (const auto& fallback_dir : config_.fallback_directories) {
      std::string fallback_path = generate_fallback_path(filename, fallback_dir);
      if (attempt_open(fallback_path)) {
        opened = true;
        result_.final_path = fallback_path;
        break;
      }
      result_.attempted_paths.push_back(fallback_path);
    }

    if (!opened && attempt < config_.max_retries) {
      std::this_thread::sleep_for(delay);
      delay = std::chrono::milliseconds(
          static_cast<long>(delay.count() * config_.retry_backoff_multiplier));
      attempt++;
      result_.retry_count++;
    } else {
      break;
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  result_.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  result_.success = opened;
  if (!opened) {
    result_.error_message =
        "Failed to open file after " + std::to_string(attempt + 1) + " attempts";
  }

  // Record operation for monitoring
  FileOperationMonitor::instance().record_operation(result_);
}

RobustFileStream::~RobustFileStream() {
  try {
    close();
    cleanup_temp_file();
  } catch (...) {
    // Suppress exceptions in destructor
  }
}

std::ofstream& RobustFileStream::output_stream() {
  if (!output_stream_) {
    throw std::runtime_error("Output stream not available");
  }
  return *output_stream_;
}

std::ifstream& RobustFileStream::input_stream() {
  if (!input_stream_) {
    throw std::runtime_error("Input stream not available");
  }
  return *input_stream_;
}

bool RobustFileStream::is_open() const {
  if (mode_ == Mode::Read) {
    return input_stream_ && input_stream_->is_open();
  } else {
    return output_stream_ && output_stream_->is_open();
  }
}

bool RobustFileStream::good() const {
  if (mode_ == Mode::Read) {
    return input_stream_ && input_stream_->good();
  } else {
    return output_stream_ && output_stream_->good();
  }
}

bool RobustFileStream::flush() {
  if (output_stream_) {
    output_stream_->flush();
    return output_stream_->good();
  }
  return false;
}

bool RobustFileStream::sync() {
  if (output_stream_) {
    output_stream_->flush();  // Use flush instead of sync for ofstream
    return output_stream_->good();
  }
  return false;
}

void RobustFileStream::close() {
  if (output_stream_ && output_stream_->is_open()) {
    output_stream_->close();
    if (config_.use_atomic_writes && !temp_file_path_.empty()) {
      finalize_atomic_write();
    }
  }
  if (input_stream_ && input_stream_->is_open()) {
    input_stream_->close();
  }
}

bool RobustFileStream::attempt_open(const std::string& path) {
  try {
    // Ensure directory exists if configured
    if (config_.create_directories) {
      std::filesystem::path file_path(path);
      if (file_path.has_parent_path()) {
        if (!ensure_directory_exists(file_path.parent_path())) {
          return false;
        }
      }
    }

    // Setup atomic write if needed
    if ((mode_ == Mode::Write || mode_ == Mode::Append) && config_.use_atomic_writes) {
      setup_atomic_write(path);
    }

    // Open the stream
    if (mode_ == Mode::Read) {
      input_stream_ = std::make_unique<std::ifstream>(path, std::ios::binary);
      return input_stream_->is_open() && input_stream_->good();
    } else {
      std::ios::openmode open_mode = std::ios::binary;
      if (mode_ == Mode::Append) {
        open_mode |= std::ios::app;
      }

      std::string target_path =
          config_.use_atomic_writes && !temp_file_path_.empty() ? temp_file_path_ : path;

      output_stream_ = std::make_unique<std::ofstream>(target_path, open_mode);
      bool success = output_stream_->is_open() && output_stream_->good();

// Set file permissions if on Unix-like system
#ifndef _WIN32
      if (success) {
        try {
          std::filesystem::permissions(target_path, config_.file_permissions);
        } catch (...) {
          // Ignore permission errors
        }
      }
#endif

      return success;
    }
  } catch (const std::exception&) {
    return false;
  }
}

std::string RobustFileStream::generate_fallback_path(const std::string& original_filename,
                                                     const std::string& fallback_dir) const {
  std::filesystem::path original_path(original_filename);
  std::filesystem::path fallback_path(fallback_dir);

  // Use just the filename, not the full path
  fallback_path /= original_path.filename();

  return fallback_path.string();
}

bool RobustFileStream::ensure_directory_exists(const std::filesystem::path& dir_path) {
  try {
    if (!std::filesystem::exists(dir_path)) {
      return std::filesystem::create_directories(dir_path);
    }
    return std::filesystem::is_directory(dir_path);
  } catch (...) {
    return false;
  }
}

void RobustFileStream::setup_atomic_write(const std::string& final_path) {
  // Generate temporary file name
  std::filesystem::path final_file_path(final_path);
  std::string temp_name =
      final_file_path.filename().string() + ".tmp." + std::to_string(std::random_device{}());

  if (final_file_path.has_parent_path()) {
    temp_file_path_ = (final_file_path.parent_path() / temp_name).string();
  } else {
    temp_file_path_ = temp_name;
  }
}

void RobustFileStream::finalize_atomic_write() {
  if (temp_file_path_.empty() || result_.final_path.empty()) {
    return;
  }

  try {
    // Backup existing file if configured
    if (config_.backup_existing_files && std::filesystem::exists(result_.final_path)) {
      std::string backup_path = result_.final_path + ".backup";
      std::filesystem::copy_file(result_.final_path, backup_path,
                                 std::filesystem::copy_options::overwrite_existing);
    }

    // Move temp file to final location
    std::filesystem::rename(temp_file_path_, result_.final_path);
    temp_file_path_.clear();
  } catch (const std::exception& e) {
    result_.error_message += " Atomic write finalization failed: " + std::string(e.what());
    cleanup_temp_file();
  }
}

void RobustFileStream::cleanup_temp_file() {
  if (!temp_file_path_.empty()) {
    try {
      if (std::filesystem::exists(temp_file_path_)) {
        std::filesystem::remove(temp_file_path_);
      }
    } catch (...) {
      // Ignore cleanup errors
    }
    temp_file_path_.clear();
  }
}

// Helper function for system error messages
std::string get_system_error_message() {
#ifdef _WIN32
  DWORD error = GetLastError();
  if (error == 0) return "No error";

  LPSTR messageBuffer = nullptr;
  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

  std::string message(messageBuffer, size);
  LocalFree(messageBuffer);
  return message;
#else
  return std::string(strerror(errno));
#endif
}

// RobustFileHandler implementation
RobustFileHandler::RobustFileHandler(const FileOperationConfig& config) : config_(config) {}

FileOperationResult RobustFileHandler::write_file(const std::string& filename,
                                                  const std::string& content) {
  return retry_operation("write_file", [&]() -> FileOperationResult {
    auto stream = create_output_stream(filename);
    if (!stream || !stream->is_open()) {
      return FileOperationResult(false, "");
    }

    stream->output_stream() << content;
    stream->flush();

    if (!stream->good()) {
      return FileOperationResult(false, "");
    }

    return stream->operation_result();
  });
}

FileOperationResult RobustFileHandler::append_file(const std::string& filename,
                                                   const std::string& content) {
  return retry_operation("append_file", [&]() -> FileOperationResult {
    auto stream = create_append_stream(filename);
    if (!stream || !stream->is_open()) {
      return FileOperationResult(false, "");
    }

    stream->output_stream() << content;
    stream->flush();

    if (!stream->good()) {
      return FileOperationResult(false, "");
    }

    return stream->operation_result();
  });
}

FileOperationResult RobustFileHandler::read_file(const std::string& filename,
                                                 std::string& content) {
  return retry_operation("read_file", [&]() -> FileOperationResult {
    auto stream = create_input_stream(filename);
    if (!stream || !stream->is_open()) {
      return FileOperationResult(false, "");
    }

    std::ostringstream buffer;
    buffer << stream->input_stream().rdbuf();
    content = buffer.str();

    if (!stream->good() && !stream->input_stream().eof()) {
      return FileOperationResult(false, "");
    }

    return stream->operation_result();
  });
}

std::unique_ptr<RobustFileStream> RobustFileHandler::create_output_stream(
    const std::string& filename) {
  return std::make_unique<RobustFileStream>(filename, RobustFileStream::Mode::Write, config_);
}

std::unique_ptr<RobustFileStream> RobustFileHandler::create_input_stream(
    const std::string& filename) {
  return std::make_unique<RobustFileStream>(filename, RobustFileStream::Mode::Read, config_);
}

std::unique_ptr<RobustFileStream> RobustFileHandler::create_append_stream(
    const std::string& filename) {
  return std::make_unique<RobustFileStream>(filename, RobustFileStream::Mode::Append, config_);
}

template <typename Operation>
FileOperationResult RobustFileHandler::retry_operation(const std::string& description,
                                                       Operation&& op) {
  auto start_time = std::chrono::steady_clock::now();

  FileOperationResult result;
  int attempt = 0;
  auto delay = config_.retry_delay;

  while (attempt <= config_.max_retries) {
    log_operation_attempt(description, "various", attempt);

    try {
      result = op();
      if (result.success) {
        break;
      }
    } catch (const std::exception& e) {
      result.error_message = e.what();
    }

    if (attempt < config_.max_retries) {
      std::this_thread::sleep_for(delay);
      delay = std::chrono::milliseconds(
          static_cast<long>(delay.count() * config_.retry_backoff_multiplier));
      attempt++;
      result.retry_count++;
    } else {
      break;
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return result;
}

void RobustFileHandler::log_operation_attempt(const std::string& operation, const std::string& path,
                                              int attempt, const std::string& error) const {
  // Only log if verbose or if there's an error
  if (attempt > 0 || !error.empty()) {
    std::cerr << "[RobustFileHandler] " << operation << " attempt " << (attempt + 1) << " on "
              << path;
    if (!error.empty()) {
      std::cerr << " failed: " << error;
    }
    std::cerr << std::endl;
  }
}

// Static utility methods
std::vector<std::string> RobustFileHandler::get_default_fallback_directories() {
  std::vector<std::string> dirs;

  // Try current directory
  dirs.push_back(".");

  // Try test output directory
  dirs.push_back("./test_output");

// Try temporary directory
#ifdef _WIN32
  if (const char* temp = std::getenv("TEMP")) {
    dirs.push_back(std::string(temp) + "\\solar_system_tests");
  }
#else
  dirs.push_back("/tmp/solar_system_tests");
  if (const char* temp = std::getenv("TMPDIR")) {
    dirs.push_back(std::string(temp) + "/solar_system_tests");
  }
#endif

// Try user home directory
#ifdef _WIN32
  if (const char* home = std::getenv("USERPROFILE")) {
    dirs.push_back(std::string(home) + "\\solar_system_tests");
  }
#else
  if (const char* home = std::getenv("HOME")) {
    dirs.push_back(std::string(home) + "/solar_system_tests");
  }
#endif

  return dirs;
}

bool RobustFileHandler::is_path_writable(const std::string& path) {
  try {
    std::filesystem::path fs_path(path);

    if (std::filesystem::exists(fs_path)) {
// Check if we can write to existing path
#ifdef _WIN32
      return true;  // Simplified for Windows
#else
      return access(path.c_str(), W_OK) == 0;
#endif
    } else {
      // Check if we can write to parent directory
      auto parent = fs_path.parent_path();
      if (std::filesystem::exists(parent)) {
#ifdef _WIN32
        return true;  // Simplified for Windows
#else
        return access(parent.c_str(), W_OK) == 0;
#endif
      }
    }
  } catch (...) {
    return false;
  }

  return false;
}

std::string RobustFileHandler::get_safe_filename(const std::string& filename) {
  std::string safe_name = filename;

  // Replace unsafe characters
  const std::string unsafe_chars = "<>:\"/\\|?*";
  for (char& c : safe_name) {
    if (unsafe_chars.find(c) != std::string::npos) {
      c = '_';
    }
  }

  // Ensure it's not too long
  if (safe_name.length() > 255) {
    safe_name = safe_name.substr(0, 255);
  }

  return safe_name;
}

// TemporaryFile implementation
TemporaryFile::TemporaryFile(const std::string& prefix, const std::string& suffix) {
  // Generate unique temporary file name
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1000, 9999);

  std::string temp_dir;
#ifdef _WIN32
  if (const char* temp = std::getenv("TEMP")) {
    temp_dir = temp;
  } else {
    temp_dir = ".";
  }
#else
  temp_dir = "/tmp";
  if (const char* temp = std::getenv("TMPDIR")) {
    temp_dir = temp;
  }
#endif

  file_path_ = temp_dir + "/" + prefix + std::to_string(dis(gen)) + suffix;
}

TemporaryFile::~TemporaryFile() {
  if (cleanup_on_destroy_) {
    cleanup();
  }
}

bool TemporaryFile::exists() const { return std::filesystem::exists(file_path_); }

std::unique_ptr<RobustFileStream> TemporaryFile::create_stream(RobustFileStream::Mode mode) {
  FileOperationConfig config;
  config.create_directories = true;
  config.use_atomic_writes = false;  // Not needed for temp files

  return std::make_unique<RobustFileStream>(file_path_, mode, config);
}

void TemporaryFile::cleanup() {
  try {
    if (std::filesystem::exists(file_path_)) {
      std::filesystem::remove(file_path_);
    }
  } catch (...) {
    // Ignore cleanup errors
  }
}

// FileOperationMonitor implementation
FileOperationMonitor& FileOperationMonitor::instance() {
  static FileOperationMonitor instance;
  return instance;
}

void FileOperationMonitor::record_operation(const FileOperationResult& result) {
  stats_.total_operations++;
  if (result.success) {
    stats_.successful_operations++;
  } else {
    stats_.failed_operations++;
  }

  if (result.retry_count > 0) {
    stats_.retry_operations++;
  }

  if (result.attempted_paths.size() > 1) {
    stats_.fallback_operations++;
  }

  stats_.total_time += result.total_time;
  stats_.update_average_time();

  if (callback_) {
    callback_(result);
  }
}

}  // namespace SolarSystem::Testing::Utils
