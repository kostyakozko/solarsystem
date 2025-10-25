/**
 * @file robust_file_handler.hpp
 * @brief Robust file handling utilities for test reporters (Task 15)
 *
 * Implements requirements 7.1, 7.3, and 7.5:
 * - Handle file system errors gracefully with retry mechanisms
 * - Provide fallback mechanisms or clear error reporting
 * - Attempt alternative locations or provide clear error messages
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Testing::Utils {

/**
 * @brief Configuration for robust file operations
 */
struct FileOperationConfig {
  // Retry configuration
  int max_retries = 3;
  std::chrono::milliseconds retry_delay = std::chrono::milliseconds(100);
  double retry_backoff_multiplier = 2.0;

  // Fallback configuration
  std::vector<std::string> fallback_directories = {"./test_output", "/tmp/solar_system_tests", "."};

  // File operation configuration
  bool create_directories = true;
  bool use_atomic_writes = true;
  bool backup_existing_files = false;

  // Permissions (Unix-style)
  std::filesystem::perms file_permissions = std::filesystem::perms::owner_read |
                                            std::filesystem::perms::owner_write |
                                            std::filesystem::perms::group_read;
};

/**
 * @brief Result of a file operation
 */
struct FileOperationResult {
  bool success = false;
  std::string final_path;
  std::string error_message;
  std::vector<std::string> attempted_paths;
  int retry_count = 0;
  std::chrono::milliseconds total_time{0};

  FileOperationResult() = default;
  FileOperationResult(bool success, const std::string& path) : success(success), final_path(path) {}

  explicit operator bool() const { return success; }
};

/**
 * @brief RAII wrapper for robust file streams
 */
class RobustFileStream {
 public:
  enum class Mode { Read, Write, Append };

  RobustFileStream(const std::string& filename, Mode mode,
                   const FileOperationConfig& config = FileOperationConfig{});

  ~RobustFileStream();

  // Non-copyable, movable
  RobustFileStream(const RobustFileStream&) = delete;
  RobustFileStream& operator=(const RobustFileStream&) = delete;
  RobustFileStream(RobustFileStream&&) = default;
  RobustFileStream& operator=(RobustFileStream&&) = default;

  // Stream access
  std::ofstream& output_stream();
  std::ifstream& input_stream();

  // Status checking
  bool is_open() const;
  bool good() const;
  const FileOperationResult& operation_result() const { return result_; }

  // Manual operations
  bool flush();
  bool sync();
  void close();

  // Stream operators
  template <typename T>
  RobustFileStream& operator<<(const T& value) {
    if (mode_ == Mode::Write || mode_ == Mode::Append) {
      (*output_stream_) << value;
    }
    return *this;
  }

  template <typename T>
  RobustFileStream& operator>>(T& value) {
    if (mode_ == Mode::Read) {
      input_stream_ >> value;
    }
    return *this;
  }

 private:
  Mode mode_;
  FileOperationConfig config_;
  FileOperationResult result_;
  std::unique_ptr<std::ofstream> output_stream_;
  std::unique_ptr<std::ifstream> input_stream_;
  std::string temp_file_path_;  // For atomic writes

  bool attempt_open(const std::string& path);
  std::string generate_fallback_path(const std::string& original_filename,
                                     const std::string& fallback_dir) const;
  bool ensure_directory_exists(const std::filesystem::path& dir_path);
  void setup_atomic_write(const std::string& final_path);
  void finalize_atomic_write();
  void cleanup_temp_file();
};

/**
 * @brief Robust file operations utility class
 */
class RobustFileHandler {
 public:
  explicit RobustFileHandler(const FileOperationConfig& config = FileOperationConfig{});

  // File writing operations
  FileOperationResult write_file(const std::string& filename, const std::string& content);
  FileOperationResult append_file(const std::string& filename, const std::string& content);

  // File reading operations
  FileOperationResult read_file(const std::string& filename, std::string& content);

  // Directory operations
  FileOperationResult create_directory(const std::string& dir_path);
  FileOperationResult ensure_directory_exists(const std::string& file_path);

  // File management operations
  FileOperationResult backup_file(const std::string& filename);
  FileOperationResult move_file(const std::string& source, const std::string& destination);
  FileOperationResult copy_file(const std::string& source, const std::string& destination);
  FileOperationResult delete_file(const std::string& filename);

  // Batch operations
  FileOperationResult write_files(const std::vector<std::pair<std::string, std::string>>& files);

  // Stream creation
  std::unique_ptr<RobustFileStream> create_output_stream(const std::string& filename);
  std::unique_ptr<RobustFileStream> create_input_stream(const std::string& filename);
  std::unique_ptr<RobustFileStream> create_append_stream(const std::string& filename);

  // Configuration
  void set_config(const FileOperationConfig& config) { config_ = config; }
  const FileOperationConfig& config() const { return config_; }

  // Utility methods
  static std::vector<std::string> get_default_fallback_directories();
  static bool is_path_writable(const std::string& path);
  static bool is_path_readable(const std::string& path);
  static std::string get_safe_filename(const std::string& filename);
  static std::string get_unique_filename(const std::string& base_filename);

 private:
  FileOperationConfig config_;

  // Internal retry mechanism
  template <typename Operation>
  FileOperationResult retry_operation(const std::string& description, Operation&& op);

  // Path resolution
  std::vector<std::string> resolve_fallback_paths(const std::string& filename) const;
  std::string resolve_absolute_path(const std::string& filename) const;

  // Error handling
  std::string get_system_error_message() const;
  void log_operation_attempt(const std::string& operation, const std::string& path, int attempt,
                             const std::string& error = "") const;
};

/**
 * @brief RAII temporary file manager
 */
class TemporaryFile {
 public:
  explicit TemporaryFile(const std::string& prefix = "solar_test_",
                         const std::string& suffix = ".tmp");
  ~TemporaryFile();

  // Non-copyable, movable
  TemporaryFile(const TemporaryFile&) = delete;
  TemporaryFile& operator=(const TemporaryFile&) = delete;
  TemporaryFile(TemporaryFile&&) = default;
  TemporaryFile& operator=(TemporaryFile&&) = default;

  const std::string& path() const { return file_path_; }
  bool exists() const;

  // Stream access
  std::unique_ptr<RobustFileStream> create_stream(RobustFileStream::Mode mode);

  // Manual cleanup
  void cleanup();

 private:
  std::string file_path_;
  bool cleanup_on_destroy_ = true;
};

/**
 * @brief File operation statistics and monitoring
 */
struct FileOperationStats {
  size_t total_operations = 0;
  size_t successful_operations = 0;
  size_t failed_operations = 0;
  size_t retry_operations = 0;
  size_t fallback_operations = 0;
  std::chrono::milliseconds total_time{0};
  std::chrono::milliseconds average_time{0};

  double success_rate() const {
    return total_operations > 0 ? static_cast<double>(successful_operations) / total_operations
                                : 0.0;
  }

  void update_average_time() {
    average_time = total_operations > 0
                       ? std::chrono::milliseconds(static_cast<uint64_t>(total_time.count()) / total_operations)
                       : std::chrono::milliseconds{0};
  }
};

/**
 * @brief Global file operation monitor
 */
class FileOperationMonitor {
 public:
  static FileOperationMonitor& instance();

  void record_operation(const FileOperationResult& result);
  const FileOperationStats& stats() const { return stats_; }
  void reset_stats() { stats_ = FileOperationStats{}; }

  // Callbacks for monitoring
  using OperationCallback = std::function<void(const FileOperationResult&)>;
  void set_operation_callback(OperationCallback callback) { callback_ = std::move(callback); }

 private:
  FileOperationStats stats_;
  OperationCallback callback_;

  FileOperationMonitor() = default;
};

}  // namespace SolarSystem::Testing::Utils
