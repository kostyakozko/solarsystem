/**
 * @file platform_security.hpp
 * @brief Cross-platform security utilities
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Utils::Platform {

/**
 * @brief Platform-specific security information
 */
struct SecurityInfo {
  bool is_elevated;           // Running with elevated privileges
  std::string current_user;   // Current user name
  std::vector<std::string> groups;  // User groups
  bool has_admin_rights;      // Has administrator rights
};

/**
 * @brief File permission information
 */
struct FilePermissions {
  bool can_read;
  bool can_write;
  bool can_execute;
  bool is_owner;
  std::string owner;
  std::string group;
  int mode;  // Unix-style permission bits
};

/**
 * @brief Cross-platform security utilities
 */
class PlatformSecurity {
 public:
  /**
   * @brief Get current security context
   */
  static SecurityInfo get_security_info();

  /**
   * @brief Check if running with elevated privileges
   */
  static bool is_elevated();

  /**
   * @brief Get current user name
   */
  static std::string get_current_user();

  /**
   * @brief Get user groups
   */
  static std::vector<std::string> get_user_groups();

  /**
   * @brief Check file permissions
   */
  static std::optional<FilePermissions> get_file_permissions(
      const std::filesystem::path& path);

  /**
   * @brief Set file permissions (Unix-style mode)
   */
  static bool set_file_permissions(
      const std::filesystem::path& path, int mode);

  /**
   * @brief Check if path is secure (no symlink attacks, etc.)
   */
  static bool is_secure_path(const std::filesystem::path& path);

  /**
   * @brief Get platform name
   */
  static std::string get_platform_name();

 private:
#ifdef _WIN32
  static SecurityInfo get_security_info_windows();
  static std::optional<FilePermissions> get_file_permissions_windows(
      const std::filesystem::path& path);
  static bool set_file_permissions_windows(
      const std::filesystem::path& path, int mode);
#else
  static SecurityInfo get_security_info_posix();
  static std::optional<FilePermissions> get_file_permissions_posix(
      const std::filesystem::path& path);
  static bool set_file_permissions_posix(
      const std::filesystem::path& path, int mode);
#endif
};

}  // namespace SolarSystem::Utils::Platform
