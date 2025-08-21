/**
 * @file file_resource_manager.hpp
 * @brief File resource management system (Task 18)
 *
 * Implements requirements 8.2:
 * - Proper file handle management
 * - Temporary resource cleanup mechanisms
 * - Resource conflict detection and resolution
 */

#pragma once

#include "resource_manager.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SolarSystem::Utils {

/**
 * @brief File access modes
 */
enum class FileAccessMode {
  Read,
  Write,
  Append,
  ReadWrite,
  Binary
};

/**
 * @brief File lock types
 */
enum class FileLockType {
  None,
  Shared,     // Multiple readers
  Exclusive   // Single writer
};

/**
 * @brief File handle information
 */
struct FileHandleInfo {
  std::string file_path;
  FileAccessMode access_mode;
  FileLockType lock_type;
  std::chrono::system_clock::time_point opened_at;
  std::chrono::system_clock::time_point last_accessed;
  size_t bytes_read = 0;
  size_t bytes_written = 0;
  bool is_temporary = false;
  std::string owner_id;
};

/**
 * @brief File operation statistics
 */
struct FileOperationStats {
  size_t total_files_opened = 0;
  size_t current_files_open = 0;
  size_t peak_files_open = 0;
  size_t total_bytes_read = 0;
  size_t total_bytes_written = 0;
  size_t failed_operations = 0;
  size_t lock_conflicts = 0;
  std::chrono::milliseconds total_io_time{0};
};

/**
 * @brief RAII file handle wrapper with resource management
 */
class ManagedFileHandle {
public:
  ManagedFileHandle() = default;
  explicit ManagedFileHandle(const std::string& file_path,
                           FileAccessMode mode = FileAccessMode::Read,
                           FileLockType lock_type = FileLockType::None);
  ~ManagedFileHandle();

  // Disable copy, enable move
  ManagedFileHandle(const ManagedFileHandle&) = delete;
  ManagedFileHandle& operator=(const ManagedFileHandle&) = delete;
  ManagedFileHandle(ManagedFileHandle&& other) noexcept;
  ManagedFileHandle& operator=(ManagedFileHandle&& other) noexcept;

  // File operations
  bool is_open() const;
  bool open(const std::string& file_path, FileAccessMode mode, FileLockType lock_type = FileLockType::None);
  void close();

  // Read operations
  std::string read_all();
  std::string read_line();
  std::vector<char> read_binary(size_t bytes);
  size_t read(char* buffer, size_t size);

  // Write operations
  bool write(const std::string& data);
  bool write_line(const std::string& line);
  bool write_binary(const std::vector<char>& data);
  bool write(const char* data, size_t size);

  // File information
  const std::string& path() const { return file_path_; }
  FileAccessMode mode() const { return access_mode_; }
  FileLockType lock_type() const { return lock_type_; }
  size_t size() const;
  bool exists() const;

  // Statistics
  size_t bytes_read() const { return bytes_read_; }
  size_t bytes_written() const { return bytes_written_; }

  // Stream access (for compatibility)
  std::ifstream* input_stream() { return input_stream_.get(); }
  std::ofstream* output_stream() { return output_stream_.get(); }
  std::fstream* bidirectional_stream() { return bidirectional_stream_.get(); }

private:
  std::string file_path_;
  FileAccessMode access_mode_ = FileAccessMode::Read;
  FileLockType lock_type_ = FileLockType::None;
  std::string resource_id_;

  // Stream objects
  std::unique_ptr<std::ifstream> input_stream_;
  std::unique_ptr<std::ofstream> output_stream_;
  std::unique_ptr<std::fstream> bidirectional_stream_;

  // Statistics
  size_t bytes_read_ = 0;
  size_t bytes_written_ = 0;

  void register_with_resource_manager();
  void unregister_from_resource_manager();
  void update_access_time();
};

/**
 * @brief File resource manager for handling file operations and conflicts
 */
class FileResourceManager {
public:
  static FileResourceManager& instance();

  // File handle management
  std::unique_ptr<ManagedFileHandle> open_file(const std::string& file_path,
                                              FileAccessMode mode = FileAccessMode::Read,
                                              FileLockType lock_type = FileLockType::None);

  bool is_file_locked(const std::string& file_path) const;
  bool can_acquire_lock(const std::string& file_path, FileLockType requested_lock) const;
  std::vector<std::string> get_conflicting_files(const std::string& file_path, FileLockType requested_lock) const;

  // Temporary file management
  std::unique_ptr<ManagedFileHandle> create_temp_file(const std::string& prefix = "solar_temp",
                                                     const std::string& suffix = ".tmp",
                                                     bool auto_delete = true);

  std::string create_temp_directory(const std::string& prefix = "solar_temp_dir");
  void cleanup_temp_files();
  void cleanup_temp_directories();

  // File operation utilities
  bool copy_file_managed(const std::string& source, const std::string& destination,
                        bool overwrite = false);
  bool move_file_managed(const std::string& source, const std::string& destination);
  bool delete_file_managed(const std::string& file_path);

  // Atomic operations
  bool atomic_write(const std::string& file_path, const std::string& content);
  bool atomic_write_binary(const std::string& file_path, const std::vector<char>& data);
  std::string atomic_read(const std::string& file_path);

  // Backup operations
  bool create_backup(const std::string& file_path, const std::string& backup_suffix = ".bak");
  bool restore_from_backup(const std::string& file_path, const std::string& backup_suffix = ".bak");
  void cleanup_backups(const std::string& directory, std::chrono::hours max_age = std::chrono::hours(24));

  // Statistics and monitoring
  FileOperationStats get_statistics() const;
  std::vector<FileHandleInfo> get_open_files() const;
  void generate_file_usage_report(std::ostream& output) const;

  // Configuration
  void set_max_open_files(size_t max_files) { max_open_files_ = max_files; }
  void set_temp_directory(const std::string& temp_dir) { temp_directory_ = temp_dir; }
  void set_auto_cleanup_interval(std::chrono::seconds interval) { auto_cleanup_interval_ = interval; }

  // Cleanup operations
  void force_close_all_files();
  void cleanup_expired_handles(std::chrono::seconds max_age = std::chrono::hours(1));

  // Allow ManagedFileHandle to access private methods
  friend class ManagedFileHandle;

private:
  FileResourceManager() = default;
  ~FileResourceManager();

  // Disable copy and move
  FileResourceManager(const FileResourceManager&) = delete;
  FileResourceManager& operator=(const FileResourceManager&) = delete;

  mutable std::mutex files_mutex_;
  std::unordered_map<std::string, FileHandleInfo> open_files_;
  std::unordered_map<std::string, FileLockType> file_locks_;
  std::unordered_set<std::string> temp_files_;
  std::unordered_set<std::string> temp_directories_;

  FileOperationStats stats_;
  size_t max_open_files_ = 1000;
  std::string temp_directory_;
  std::chrono::seconds auto_cleanup_interval_{300}; // 5 minutes

  // Internal methods
  bool check_file_limits() const;
  void update_statistics(const FileHandleInfo& info, bool is_open);
  std::string generate_temp_path(const std::string& prefix, const std::string& suffix) const;
  void register_file_handle(const std::string& file_path, const FileHandleInfo& info);
  void unregister_file_handle(const std::string& file_path);
  bool acquire_file_lock(const std::string& file_path, FileLockType lock_type);
  void release_file_lock(const std::string& file_path);
};

/**
 * @brief File operation result with error information
 */
template<typename T>
class FileResult {
public:
  template<typename U = T, typename = std::enable_if_t<!std::is_same_v<U, std::string>>>
  FileResult(T value) : value_(std::move(value)), success_(true) {}

  FileResult(const char* error) : error_(error), success_(false) {}
  FileResult(std::string error) : error_(std::move(error)), success_(false) {}

  bool is_success() const { return success_; }
  const T& value() const { return value_; }
  const std::string& error() const { return error_; }

  explicit operator bool() const { return success_; }

private:
  T value_{};
  std::string error_;
  bool success_ = false;
};

/**
 * @brief Utility functions for file operations
 */
namespace FileUtils {
  // Safe file operations
  FileResult<std::string> safe_read_file(const std::string& file_path);
  FileResult<bool> safe_write_file(const std::string& file_path, const std::string& content);
  FileResult<bool> safe_append_file(const std::string& file_path, const std::string& content);

  // File system utilities
  bool ensure_directory_exists(const std::string& directory_path);
  std::vector<std::string> list_files_in_directory(const std::string& directory_path,
                                                  const std::string& pattern = "*");
  size_t get_directory_size(const std::string& directory_path);

  // File validation
  bool is_file_readable(const std::string& file_path);
  bool is_file_writable(const std::string& file_path);
  bool is_file_locked_by_system(const std::string& file_path);

  // Path utilities
  std::string get_file_extension(const std::string& file_path);
  std::string get_filename_without_extension(const std::string& file_path);
  std::string get_unique_filename(const std::string& base_path);
}

/**
 * @brief Utility macros for file resource management
 */
#define SOLAR_MANAGED_FILE(path, mode) \
  SolarSystem::Utils::FileResourceManager::instance().open_file(path, mode)

#define SOLAR_TEMP_FILE(prefix, suffix) \
  SolarSystem::Utils::FileResourceManager::instance().create_temp_file(prefix, suffix)

#define SOLAR_ATOMIC_WRITE(path, content) \
  SolarSystem::Utils::FileResourceManager::instance().atomic_write(path, content)

#define SOLAR_SAFE_READ(path) \
  SolarSystem::Utils::FileUtils::safe_read_file(path)

} // namespace SolarSystem::Utils
