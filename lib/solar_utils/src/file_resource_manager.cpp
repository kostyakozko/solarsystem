/**
 * @file file_resource_manager.cpp
 * @brief Implementation of file resource management system
 */

#include "solar_utils/file_resource_manager.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <random>
#include <sstream>

namespace SolarSystem::Utils {

// ManagedFileHandle implementation
ManagedFileHandle::ManagedFileHandle(const std::string& file_path, FileAccessMode mode, FileLockType lock_type) {
  open(file_path, mode, lock_type);
}

ManagedFileHandle::~ManagedFileHandle() {
  close();
}

ManagedFileHandle::ManagedFileHandle(ManagedFileHandle&& other) noexcept
    : file_path_(std::move(other.file_path_)),
      access_mode_(other.access_mode_),
      lock_type_(other.lock_type_),
      resource_id_(std::move(other.resource_id_)),
      input_stream_(std::move(other.input_stream_)),
      output_stream_(std::move(other.output_stream_)),
      bidirectional_stream_(std::move(other.bidirectional_stream_)),
      bytes_read_(other.bytes_read_),
      bytes_written_(other.bytes_written_) {
  other.access_mode_ = FileAccessMode::Read;
  other.lock_type_ = FileLockType::None;
  other.bytes_read_ = 0;
  other.bytes_written_ = 0;
}

ManagedFileHandle& ManagedFileHandle::operator=(ManagedFileHandle&& other) noexcept {
  if (this != &other) {
    close();

    file_path_ = std::move(other.file_path_);
    access_mode_ = other.access_mode_;
    lock_type_ = other.lock_type_;
    resource_id_ = std::move(other.resource_id_);
    input_stream_ = std::move(other.input_stream_);
    output_stream_ = std::move(other.output_stream_);
    bidirectional_stream_ = std::move(other.bidirectional_stream_);
    bytes_read_ = other.bytes_read_;
    bytes_written_ = other.bytes_written_;

    other.access_mode_ = FileAccessMode::Read;
    other.lock_type_ = FileLockType::None;
    other.bytes_read_ = 0;
    other.bytes_written_ = 0;
  }
  return *this;
}

bool ManagedFileHandle::open(const std::string& file_path, FileAccessMode mode, FileLockType lock_type) {
  close();  // Close any existing handle

  file_path_ = file_path;
  access_mode_ = mode;
  lock_type_ = lock_type;

  // Check if we can acquire the requested lock
  auto& file_manager = FileResourceManager::instance();
  if (!file_manager.can_acquire_lock(file_path, lock_type)) {
    return false;
  }

  try {
    std::ios_base::openmode open_mode = std::ios_base::in;

    switch (mode) {
      case FileAccessMode::Read:
        open_mode = std::ios_base::in;
        if (mode == FileAccessMode::Binary) {
          open_mode |= std::ios_base::binary;
        }
        input_stream_ = std::make_unique<std::ifstream>(file_path, open_mode);
  break;

      case FileAccessMode::Write:
        open_mode = std::ios_base::out | std::ios_base::trunc;
        if (mode == FileAccessMode::Binary) {
          open_mode |= std::ios_base::binary;
        }
        output_stream_ = std::make_unique<std::ofstream>(file_path, open_mode);
        break;

      case FileAccessMode::Append:
        open_mode = std::ios_base::out | std::ios_base::app;
        output_stream_ = std::make_unique<std::ofstream>(file_path, open_mode);
        break;

      case FileAccessMode::ReadWrite:
        open_mode = std::ios_base::in | std::ios_base::out;
        bidirectional_stream_ = std::make_unique<std::fstream>(file_path, open_mode);
        break;

      case FileAccessMode::Binary:
        open_mode = std::ios_base::in | std::ios_base::binary;
        input_stream_ = std::make_unique<std::ifstream>(file_path, open_mode);
        break;
    }

    if (is_open()) {
      register_with_resource_manager();
      return true;
    }
  } catch (const std::exception&) {
    // File opening failed
  }

  return false;
}

void ManagedFileHandle::close() {
  if (is_open()) {
    unregister_from_resource_manager();

    if (input_stream_) {
      input_stream_->close();
      input_stream_.reset();
    }
    if (output_stream_) {
      output_stream_->close();
      output_stream_.reset();
    }
    if (bidirectional_stream_) {
      bidirectional_stream_->close();
      bidirectional_stream_.reset();
    }
  }
}

bool ManagedFileHandle::is_open() const {
  return (input_stream_ && input_stream_->is_open()) ||
         (output_stream_ && output_stream_->is_open()) ||
         (bidirectional_stream_ && bidirectional_stream_->is_open());
}

std::string ManagedFileHandle::read_all() {
  if (!input_stream_ && !bidirectional_stream_) {
    return "";
  }

  std::istream* stream = input_stream_ ? static_cast<std::istream*>(input_stream_.get()) :
                                        static_cast<std::istream*>(bidirectional_stream_.get());
  std::ostringstream content;
  content << stream->rdbuf();

  std::string result = content.str();
  bytes_read_ += result.size();
  update_access_time();

  return result;
}

std::string ManagedFileHandle::read_line() {
  if (!input_stream_ && !bidirectional_stream_) {
    return "";
  }

  std::istream* stream = input_stream_ ? static_cast<std::istream*>(input_stream_.get()) :
                                        static_cast<std::istream*>(bidirectional_stream_.get());
  std::string line;
  std::getline(*stream, line);

  bytes_read_ += line.size() + 1;  // +1 for newline
  update_access_time();

  return line;
}

std::vector<char> ManagedFileHandle::read_binary(size_t bytes) {
  if (!input_stream_ && !bidirectional_stream_) {
    return {};
  }

  std::istream* stream = input_stream_ ? static_cast<std::istream*>(input_stream_.get()) :
                                        static_cast<std::istream*>(bidirectional_stream_.get());
  std::vector<char> buffer(bytes);
  stream->read(buffer.data(), bytes);

  size_t actual_bytes = stream->gcount();
  buffer.resize(actual_bytes);

  bytes_read_ += actual_bytes;
  update_access_time();

  return buffer;
}

size_t ManagedFileHandle::read(char* buffer, size_t size) {
  if (!input_stream_ && !bidirectional_stream_) {
    return 0;
  }

  std::istream* stream = input_stream_ ? static_cast<std::istream*>(input_stream_.get()) :
                                        static_cast<std::istream*>(bidirectional_stream_.get());
  stream->read(buffer, size);

  size_t actual_bytes = stream->gcount();
  bytes_read_ += actual_bytes;
  update_access_time();

  return actual_bytes;
}

bool ManagedFileHandle::write(const std::string& data) {
  if (!output_stream_ && !bidirectional_stream_) {
    return false;
  }

  std::ostream* stream = output_stream_ ? static_cast<std::ostream*>(output_stream_.get()) :
                                         static_cast<std::ostream*>(bidirectional_stream_.get());
  *stream << data;

  if (stream->good()) {
    bytes_written_ += data.size();
    update_access_time();
    return true;
  }

  return false;
}

bool ManagedFileHandle::write_line(const std::string& line) {
  return write(line + "\n");
}

bool ManagedFileHandle::write_binary(const std::vector<char>& data) {
  if (!output_stream_ && !bidirectional_stream_) {
    return false;
  }

  std::ostream* stream = output_stream_ ? static_cast<std::ostream*>(output_stream_.get()) :
                                         static_cast<std::ostream*>(bidirectional_stream_.get());
  stream->write(data.data(), data.size());

  if (stream->good()) {
    bytes_written_ += data.size();
    update_access_time();
    return true;
  }

  return false;
}

bool ManagedFileHandle::write(const char* data, size_t size) {
  if (!output_stream_ && !bidirectional_stream_) {
    return false;
  }

  std::ostream* stream = output_stream_ ? static_cast<std::ostream*>(output_stream_.get()) :
                                         static_cast<std::ostream*>(bidirectional_stream_.get());
  stream->write(data, size);

  if (stream->good()) {
    bytes_written_ += size;
    update_access_time();
    return true;
  }

  return false;
}

size_t ManagedFileHandle::size() const {
  if (!exists()) {
    return 0;
  }

  try {
    return std::filesystem::file_size(file_path_);
  } catch (const std::exception&) {
    return 0;
  }
}

bool ManagedFileHandle::exists() const {
  return std::filesystem::exists(file_path_);
}

void ManagedFileHandle::register_with_resource_manager() {
  ResourceInfo info("", ResourceType::FileHandle, size(), "", "File: " + file_path_);
  info.resource_ptr = this;
  info.cleanup_function = [this]() {
    close();
  };

  resource_id_ = ResourceManager::instance().register_resource(info);

  // Also register with file resource manager
  FileHandleInfo file_info;
  file_info.file_path = file_path_;
  file_info.access_mode = access_mode_;
  file_info.lock_type = lock_type_;
  file_info.opened_at = std::chrono::system_clock::now();
  file_info.last_accessed = file_info.opened_at;
  file_info.owner_id = resource_id_;

  FileResourceManager::instance().register_file_handle(file_path_, file_info);
}

void ManagedFileHandle::unregister_from_resource_manager() {
  if (!resource_id_.empty()) {
    ResourceManager::instance().unregister_resource(resource_id_);
    FileResourceManager::instance().unregister_file_handle(file_path_);
    resource_id_.clear();
  }
}

void ManagedFileHandle::update_access_time() {
  // Update access time in file resource manager
  // This would be implemented with a callback to FileResourceManager
}

// FileResourceManager implementation
FileResourceManager& FileResourceManager::instance() {
  static FileResourceManager instance;
  return instance;
}

FileResourceManager::~FileResourceManager() {
  force_close_all_files();
  cleanup_temp_files();
  cleanup_temp_directories();
}

std::unique_ptr<ManagedFileHandle> FileResourceManager::open_file(const std::string& file_path,
                                                                 FileAccessMode mode,
                                                                 FileLockType lock_type) {
  std::lock_guard<std::mutex> lock(files_mutex_);

  if (!check_file_limits()) {
    return nullptr;
  }

  if (!can_acquire_lock(file_path, lock_type)) {
    return nullptr;
  }

  auto handle = std::make_unique<ManagedFileHandle>();
  if (handle->open(file_path, mode, lock_type)) {
    acquire_file_lock(file_path, lock_type);
    return handle;
  }

  return nullptr;
}

bool FileResourceManager::is_file_locked(const std::string& file_path) const {
  std::lock_guard<std::mutex> lock(files_mutex_);
  return file_locks_.find(file_path) != file_locks_.end();
}

bool FileResourceManager::can_acquire_lock(const std::string& file_path, FileLockType requested_lock) const {
  std::lock_guard<std::mutex> lock(files_mutex_);

  auto it = file_locks_.find(file_path);
  if (it == file_locks_.end()) {
    return true;  // No existing lock
  }

  FileLockType existing_lock = it->second;

  // Check lock compatibility
  if (existing_lock == FileLockType::Exclusive || requested_lock == FileLockType::Exclusive) {
    return false;  // Exclusive locks are incompatible with any other lock
  }

  // Shared locks are compatible with other shared locks
  return existing_lock == FileLockType::Shared && requested_lock == FileLockType::Shared;
}

std::vector<std::string> FileResourceManager::get_conflicting_files(const std::string& file_path,
                                                                   FileLockType requested_lock) const {
  std::vector<std::string> conflicts;

  if (!can_acquire_lock(file_path, requested_lock)) {
    conflicts.push_back(file_path);
  }

  return conflicts;
}

std::unique_ptr<ManagedFileHandle> FileResourceManager::create_temp_file(const std::string& prefix,
                                                                        const std::string& suffix,
                                                                        bool auto_delete) {
  std::string temp_path = generate_temp_path(prefix, suffix);

  auto handle = open_file(temp_path, FileAccessMode::ReadWrite);
  if (handle && auto_delete) {
    std::lock_guard<std::mutex> lock(files_mutex_);
    temp_files_.insert(temp_path);
  }

  return handle;
}

std::string FileResourceManager::create_temp_directory(const std::string& prefix) {
  std::string temp_path = generate_temp_path(prefix, "");

  try {
    std::filesystem::create_directories(temp_path);
    std::lock_guard<std::mutex> lock(files_mutex_);
    temp_directories_.insert(temp_path);
    return temp_path;
  } catch (const std::exception&) {
    return "";
  }
}

void FileResourceManager::cleanup_temp_files() {
  std::lock_guard<std::mutex> lock(files_mutex_);

  auto it = temp_files_.begin();
  while (it != temp_files_.end()) {
    try {
      if (std::filesystem::exists(*it)) {
        std::filesystem::remove(*it);
      }
      it = temp_files_.erase(it);
    } catch (const std::exception&) {
      ++it;
    }
  }
}

void FileResourceManager::cleanup_temp_directories() {
  std::lock_guard<std::mutex> lock(files_mutex_);

  auto it = temp_directories_.begin();
  while (it != temp_directories_.end()) {
    try {
      if (std::filesystem::exists(*it)) {
        std::filesystem::remove_all(*it);
      }
      it = temp_directories_.erase(it);
    } catch (const std::exception&) {
      ++it;
    }
  }
}

bool FileResourceManager::copy_file_managed(const std::string& source, const std::string& destination, bool overwrite) {
  try {
    std::filesystem::copy_options options = std::filesystem::copy_options::none;
    if (overwrite) {
      options = std::filesystem::copy_options::overwrite_existing;
    }

    return std::filesystem::copy_file(source, destination, options);
  } catch (const std::exception&) {
    return false;
  }
}

bool FileResourceManager::move_file_managed(const std::string& source, const std::string& destination) {
  try {
    std::filesystem::rename(source, destination);
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool FileResourceManager::delete_file_managed(const std::string& file_path) {
  try {
    return std::filesystem::remove(file_path);
  } catch (const std::exception&) {
    return false;
  }
}

bool FileResourceManager::atomic_write(const std::string& file_path, const std::string& content) {
  std::string temp_path = file_path + ".tmp";

  try {
    // Write to temporary file first
    auto temp_handle = open_file(temp_path, FileAccessMode::Write);
    if (!temp_handle || !temp_handle->write(content)) {
      return false;
    }
    temp_handle->close();

    // Atomically move temporary file to final location
    return move_file_managed(temp_path, file_path);
  } catch (const std::exception&) {
    // Clean up temporary file on failure
    delete_file_managed(temp_path);
    return false;
  }
}

bool FileResourceManager::atomic_write_binary(const std::string& file_path, const std::vector<char>& data) {
  std::string temp_path = file_path + ".tmp";

  try {
    // Write to temporary file first
    auto temp_handle = open_file(temp_path, FileAccessMode::Write);
    if (!temp_handle || !temp_handle->write_binary(data)) {
      return false;
    }
    temp_handle->close();

    // Atomically move temporary file to final location
    return move_file_managed(temp_path, file_path);
  } catch (const std::exception&) {
    // Clean up temporary file on failure
    delete_file_managed(temp_path);
    return false;
  }
}

std::string FileResourceManager::atomic_read(const std::string& file_path) {
  auto handle = open_file(file_path, FileAccessMode::Read, FileLockType::Shared);
  if (!handle) {
    return "";
  }

  return handle->read_all();
}

bool FileResourceManager::create_backup(const std::string& file_path, const std::string& backup_suffix) {
  if (!std::filesystem::exists(file_path)) {
    return false;
  }

  std::string backup_path = file_path + backup_suffix;
  return copy_file_managed(file_path, backup_path, true);
}

bool FileResourceManager::restore_from_backup(const std::string& file_path, const std::string& backup_suffix) {
  std::string backup_path = file_path + backup_suffix;
  if (!std::filesystem::exists(backup_path)) {
    return false;
  }

  return copy_file_managed(backup_path, file_path, true);
}

void FileResourceManager::cleanup_backups(const std::string& directory, std::chrono::hours max_age) {
  try {
    auto cutoff_time = std::filesystem::file_time_type::clock::now() - max_age;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        if (filename.find(".bak") != std::string::npos) {
          auto file_time = entry.last_write_time();
          if (file_time < cutoff_time) {
            std::filesystem::remove(entry.path());
          }
        }
      }
    }
  } catch (const std::exception&) {
    // Ignore errors during cleanup
  }
}

FileOperationStats FileResourceManager::get_statistics() const {
  std::lock_guard<std::mutex> lock(files_mutex_);
  return stats_;
}

std::vector<FileHandleInfo> FileResourceManager::get_open_files() const {
  std::lock_guard<std::mutex> lock(files_mutex_);

  std::vector<FileHandleInfo> files;
  files.reserve(open_files_.size());

  for (const auto& [path, info] : open_files_) {
    files.push_back(info);
  }

  return files;
}

void FileResourceManager::generate_file_usage_report(std::ostream& output) const {
  std::lock_guard<std::mutex> lock(files_mutex_);

  output << "=== File Resource Usage Report ===\n";
  output << "Total files opened: " << stats_.total_files_opened << "\n";
  output << "Current files open: " << stats_.current_files_open << "\n";
  output << "Peak files open: " << stats_.peak_files_open << "\n";
  output << "Total bytes read: " << stats_.total_bytes_read << "\n";
  output << "Total bytes written: " << stats_.total_bytes_written << "\n";
  output << "Failed operations: " << stats_.failed_operations << "\n";
  output << "Lock conflicts: " << stats_.lock_conflicts << "\n";
  output << "Total I/O time: " << stats_.total_io_time.count() << "ms\n\n";

  output << "Open Files:\n";
  for (const auto& [path, info] : open_files_) {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now() - info.opened_at);

    output << "  " << path << " (" << static_cast<int>(info.access_mode) << "): "
           << "age " << age.count() << "s, "
           << "read " << info.bytes_read << " bytes, "
           << "written " << info.bytes_written << " bytes\n";
  }

  output << "\nTemporary Files: " << temp_files_.size() << "\n";
  output << "Temporary Directories: " << temp_directories_.size() << "\n";
}

void FileResourceManager::force_close_all_files() {
  std::lock_guard<std::mutex> lock(files_mutex_);

  // Note: In a real implementation, we would need to track ManagedFileHandle instances
  // and call close() on them. For now, we just clear the tracking data.
  open_files_.clear();
  file_locks_.clear();
  stats_.current_files_open = 0;
}

void FileResourceManager::cleanup_expired_handles(std::chrono::seconds max_age) {
  std::lock_guard<std::mutex> lock(files_mutex_);

  auto now = std::chrono::system_clock::now();
  auto it = open_files_.begin();

  while (it != open_files_.end()) {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_accessed);
    if (age > max_age) {
      // In a real implementation, we would close the actual file handle
      file_locks_.erase(it->first);
      it = open_files_.erase(it);
      stats_.current_files_open--;
    } else {
      ++it;
    }
  }
}

bool FileResourceManager::check_file_limits() const {
  return stats_.current_files_open < max_open_files_;
}

void FileResourceManager::update_statistics(const FileHandleInfo& info, bool is_open) {
  if (is_open) {
    stats_.total_files_opened++;
    stats_.current_files_open++;
    if (stats_.current_files_open > stats_.peak_files_open) {
      stats_.peak_files_open = stats_.current_files_open;
    }
  } else {
    if (stats_.current_files_open > 0) {
      stats_.current_files_open--;
    }
    stats_.total_bytes_read += info.bytes_read;
    stats_.total_bytes_written += info.bytes_written;
  }
}

std::string FileResourceManager::generate_temp_path(const std::string& prefix, const std::string& suffix) const {
  std::filesystem::path temp_dir = temp_directory_.empty() ?
                                  std::filesystem::temp_directory_path() :
                                  std::filesystem::path(temp_directory_);

  // Generate unique filename
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream filename;
  filename << prefix << "_";
  for (int i = 0; i < 8; ++i) {
    filename << std::hex << dis(gen);
  }
  filename << suffix;

  return (temp_dir / filename.str()).string();
}

void FileResourceManager::register_file_handle(const std::string& file_path, const FileHandleInfo& info) {
  std::lock_guard<std::mutex> lock(files_mutex_);
  open_files_[file_path] = info;
  update_statistics(info, true);
}

void FileResourceManager::unregister_file_handle(const std::string& file_path) {
  std::lock_guard<std::mutex> lock(files_mutex_);

  auto it = open_files_.find(file_path);
  if (it != open_files_.end()) {
    update_statistics(it->second, false);
    open_files_.erase(it);
  }

  release_file_lock(file_path);
}

bool FileResourceManager::acquire_file_lock(const std::string& file_path, FileLockType lock_type) {
  if (lock_type == FileLockType::None) {
    return true;
  }

  if (can_acquire_lock(file_path, lock_type)) {
    file_locks_[file_path] = lock_type;
    return true;
  }

  stats_.lock_conflicts++;
  return false;
}

void FileResourceManager::release_file_lock(const std::string& file_path) {
  file_locks_.erase(file_path);
}

// FileUtils implementation
namespace FileUtils {

FileResult<std::string> safe_read_file(const std::string& file_path) {
  try {
    auto handle = FileResourceManager::instance().open_file(file_path, FileAccessMode::Read, FileLockType::Shared);
    if (!handle) {
      return FileResult<std::string>("Failed to open file: " + file_path);
    }

    return FileResult<std::string>(handle->read_all());
  } catch (const std::exception& e) {
    return FileResult<std::string>("Error reading file: " + std::string(e.what()));
  }
}

FileResult<bool> safe_write_file(const std::string& file_path, const std::string& content) {
  try {
    bool success = FileResourceManager::instance().atomic_write(file_path, content);
    if (success) {
      return FileResult<bool>(true);
    } else {
      return FileResult<bool>("Failed to write file: " + file_path);
    }
  } catch (const std::exception& e) {
    return FileResult<bool>("Error writing file: " + std::string(e.what()));
  }
}

FileResult<bool> safe_append_file(const std::string& file_path, const std::string& content) {
  try {
    auto handle = FileResourceManager::instance().open_file(file_path, FileAccessMode::Append);
    if (!handle) {
      return FileResult<bool>("Failed to open file for append: " + file_path);
    }

    bool success = handle->write(content);
    return FileResult<bool>(success);
  } catch (const std::exception& e) {
    return FileResult<bool>("Error appending to file: " + std::string(e.what()));
  }
}

bool ensure_directory_exists(const std::string& directory_path) {
  try {
    return std::filesystem::create_directories(directory_path);
  } catch (const std::exception&) {
    return false;
  }
}

std::vector<std::string> list_files_in_directory(const std::string& directory_path, const std::string& pattern) {
  std::vector<std::string> files;

  try {
    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        // Simple pattern matching (could be enhanced with regex)
        if (pattern == "*" || filename.find(pattern) != std::string::npos) {
          files.push_back(entry.path().string());
        }
      }
    }
  } catch (const std::exception&) {
    // Return empty vector on error
  }

  return files;
}

size_t get_directory_size(const std::string& directory_path) {
  size_t total_size = 0;

  try {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
      if (entry.is_regular_file()) {
        total_size += entry.file_size();
      }
    }
  } catch (const std::exception&) {
    // Return 0 on error
  }

  return total_size;
}

bool is_file_readable(const std::string& file_path) {
  try {
    std::ifstream file(file_path);
    return file.good();
  } catch (const std::exception&) {
    return false;
  }
}

bool is_file_writable(const std::string& file_path) {
  try {
    std::ofstream file(file_path, std::ios::app);
    return file.good();
  } catch (const std::exception&) {
    return false;
  }
}

bool is_file_locked_by_system(const std::string& file_path) {
  // Check if file is locked by attempting to open it exclusively
  try {
    std::ofstream test_file(file_path, std::ios::app);
    if (!test_file.is_open()) {
      return true;  // Could not open, likely locked
    }

    // Try to get an exclusive lock (platform-specific)
#ifdef _WIN32
    // Windows implementation would use LockFile API
    return false;  // Simplified for now
#else
    // Unix/Linux implementation would use flock or fcntl
    // For now, assume file is not locked if we can open it
    return false;
#endif
  } catch (const std::exception&) {
    return true;  // Error opening file, assume locked
  }
}

std::string get_file_extension(const std::string& file_path) {
  std::filesystem::path path(file_path);
  return path.extension().string();
}

std::string get_filename_without_extension(const std::string& file_path) {
  std::filesystem::path path(file_path);
  return path.stem().string();
}

std::string get_unique_filename(const std::string& base_path) {
  std::filesystem::path path(base_path);
  std::string stem = path.stem().string();
  std::string extension = path.extension().string();
  std::string directory = path.parent_path().string();

  int counter = 1;
  std::string unique_path = base_path;

  while (std::filesystem::exists(unique_path)) {
    std::ostringstream new_name;
    new_name << stem << "_" << counter << extension;
    unique_path = (std::filesystem::path(directory) / new_name.str()).string();
    counter++;
  }

  return unique_path;
}

} // namespace FileUtils

} // namespace SolarSystem::Utils
