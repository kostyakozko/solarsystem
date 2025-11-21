/**
 * @file file_security.hpp
 * @brief File system security and access control
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace SolarSystem::Security {

/**
 * @brief File access mode
 */
enum class FileAccessMode {
  READ,
  WRITE,
  EXECUTE,
  DELETE
};

/**
 * @brief File security result
 */
struct FileSecurityResult {
  bool allowed;
  std::string reason;

  FileSecurityResult(bool allow = false, std::string msg = "")
      : allowed(allow), reason(std::move(msg)) {}
};

/**
 * @brief File security policy
 */
class FileSecurityPolicy {
public:
  virtual ~FileSecurityPolicy() = default;

  virtual FileSecurityResult check_access(
      const std::filesystem::path& path,
      FileAccessMode mode) const = 0;
};

/**
 * @brief Whitelist-based file security policy
 */
class WhitelistPolicy : public FileSecurityPolicy {
public:
  void add_allowed_directory(const std::filesystem::path& dir);
  void add_allowed_file(const std::filesystem::path& file);
  void remove_allowed_directory(const std::filesystem::path& dir);

  FileSecurityResult check_access(
      const std::filesystem::path& path,
      FileAccessMode mode) const override;

private:
  std::vector<std::filesystem::path> allowed_directories_;
  std::vector<std::filesystem::path> allowed_files_;
};

/**
 * @brief File system security manager
 */
class FileSystemSecurity {
public:
  static FileSystemSecurity& instance();

  // Policy management
  void set_policy(std::shared_ptr<FileSecurityPolicy> policy);
  std::shared_ptr<FileSecurityPolicy> get_policy() const;

  // Access control
  FileSecurityResult check_file_access(
      const std::filesystem::path& path,
      FileAccessMode mode) const;

  bool can_read(const std::filesystem::path& path) const;
  bool can_write(const std::filesystem::path& path) const;
  bool can_execute(const std::filesystem::path& path) const;
  bool can_delete(const std::filesystem::path& path) const;

  // Permission checking
  bool check_permissions(const std::filesystem::path& path) const;
  bool is_safe_path(const std::filesystem::path& path) const;

  // Path validation
  bool validate_path(const std::filesystem::path& path) const;
  std::optional<std::filesystem::path> sanitize_path(
      const std::filesystem::path& path) const;

  // Audit logging
  void log_access(const std::filesystem::path& path,
                 FileAccessMode mode,
                 bool allowed) const;

private:
  FileSystemSecurity() = default;

  std::shared_ptr<FileSecurityPolicy> policy_;
  mutable std::vector<std::string> audit_log_;

  // Platform-specific implementations
#ifdef _WIN32
  bool check_permissions_windows(const std::filesystem::path& path) const;
#else
  bool check_permissions_posix(const std::filesystem::path& path) const;
#endif
};

/**
 * @brief Secure file operations wrapper
 */
class SecureFileOperations {
public:
  static std::optional<std::string> read_file(
      const std::filesystem::path& path);

  static bool write_file(
      const std::filesystem::path& path,
      const std::string& content);

  static bool delete_file(
      const std::filesystem::path& path);

  static bool create_directory(
      const std::filesystem::path& path);

  static std::vector<std::filesystem::path> list_directory(
      const std::filesystem::path& path);
};

}  // namespace SolarSystem::Security
