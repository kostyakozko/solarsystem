/**
 * @file platform_security.cpp
 * @brief Implementation of cross-platform security utilities
 */

#include "solar_utils/platform_security.hpp"

#include <cstring>

#ifdef _WIN32
#include <lmcons.h>
#include <sddl.h>
#include <windows.h>
#else
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace SolarSystem::Utils::Platform {

/**
 * @brief Get current security context
 */
SecurityInfo PlatformSecurity::get_security_info() {
#ifdef _WIN32
  return get_security_info_windows();
#else
  return get_security_info_posix();
#endif
}

/**
 * @brief Check if running with elevated privileges
 */
bool PlatformSecurity::is_elevated() {
#ifdef _WIN32
  BOOL isElevated = FALSE;
  HANDLE hToken = nullptr;

  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
    TOKEN_ELEVATION elevation;
    DWORD size = sizeof(TOKEN_ELEVATION);

    if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
      isElevated = elevation.TokenIsElevated;
    }
    CloseHandle(hToken);
  }

  return isElevated;
#else
  return getuid() == 0;
#endif
}

/**
 * @brief Get current user name
 */
std::string PlatformSecurity::get_current_user() {
#ifdef _WIN32
  char username[UNLEN + 1];
  DWORD username_len = UNLEN + 1;

  if (GetUserNameA(username, &username_len)) {
    return std::string(username);
  }
  return "unknown";
#else
  uid_t uid = getuid();
  struct passwd* pw = getpwuid(uid);

  if (pw) {
    return std::string(pw->pw_name);
  }
  return "unknown";
#endif
}

/**
 * @brief Get user groups
 */
std::vector<std::string> PlatformSecurity::get_user_groups() {
  std::vector<std::string> groups;

#ifdef _WIN32
  // Windows group enumeration is complex, simplified for now
  groups.push_back("Users");
#else
  gid_t gid = getgid();
  struct group* gr = getgrgid(gid);

  if (gr) {
    groups.push_back(std::string(gr->gr_name));
  }

  // Get supplementary groups
  int ngroups = 0;
  getgroups(0, nullptr);

  if (ngroups > 0) {
    std::vector<gid_t> gids(ngroups);
    if (getgroups(ngroups, gids.data()) != -1) {
      for (gid_t g : gids) {
        struct group* sg = getgrgid(g);
        if (sg) {
          groups.push_back(std::string(sg->gr_name));
        }
      }
    }
  }
#endif

  return groups;
}

/**
 * @brief Get file permissions
 */
std::optional<FilePermissions> PlatformSecurity::get_file_permissions(
    const std::filesystem::path& path) {
#ifdef _WIN32
  return get_file_permissions_windows(path);
#else
  return get_file_permissions_posix(path);
#endif
}

/**
 * @brief Set file permissions
 */
bool PlatformSecurity::set_file_permissions(const std::filesystem::path& path, int mode) {
#ifdef _WIN32
  return set_file_permissions_windows(path, mode);
#else
  return set_file_permissions_posix(path, mode);
#endif
}

/**
 * @brief Check if path is secure
 */
bool PlatformSecurity::is_secure_path(const std::filesystem::path& path) {
  // Check for symlink attacks
  std::error_code ec;
  auto status = std::filesystem::symlink_status(path, ec);

  if (ec) {
    return false;
  }

  // Reject symlinks for security
  if (std::filesystem::is_symlink(status)) {
    return false;
  }

  // Check parent directories
  auto parent = path.parent_path();
  while (!parent.empty() && parent != parent.parent_path()) {
    auto parent_status = std::filesystem::symlink_status(parent, ec);
    if (ec || std::filesystem::is_symlink(parent_status)) {
      return false;
    }
    parent = parent.parent_path();
  }

  return true;
}

/**
 * @brief Get platform name
 */
std::string PlatformSecurity::get_platform_name() {
#ifdef _WIN32
  return "Windows";
#elif defined(__APPLE__)
  return "macOS";
#elif defined(__linux__)
  return "Linux";
#else
  return "Unknown";
#endif
}

// Platform-specific implementations

#ifdef _WIN32

SecurityInfo PlatformSecurity::get_security_info_windows() {
  SecurityInfo info;
  info.is_elevated = is_elevated();
  info.current_user = get_current_user();
  info.groups = get_user_groups();
  info.has_admin_rights = is_elevated();
  return info;
}

std::optional<FilePermissions> PlatformSecurity::get_file_permissions_windows(
    const std::filesystem::path& path) {
  FilePermissions perms;
  perms.can_read = false;
  perms.can_write = false;
  perms.can_execute = false;
  perms.is_owner = false;
  perms.owner = "unknown";
  perms.group = "unknown";
  perms.mode = 0;

  // Check file attributes
  DWORD attrs = GetFileAttributesW(path.wstring().c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return std::nullopt;
  }

  // Basic permission checks
  perms.can_read = true;  // If we can get attributes, we can read
  perms.can_write = !(attrs & FILE_ATTRIBUTE_READONLY);
  perms.can_execute = (path.extension() == ".exe" || path.extension() == ".bat");

  return perms;
}

bool PlatformSecurity::set_file_permissions_windows(const std::filesystem::path& path, int mode) {
  // Windows doesn't use Unix-style permissions
  // Set read-only attribute based on write permission
  DWORD attrs = GetFileAttributesW(path.wstring().c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return false;
  }

  if (mode & 0200) {  // Owner write permission
    attrs &= ~FILE_ATTRIBUTE_READONLY;
  } else {
    attrs |= FILE_ATTRIBUTE_READONLY;
  }

  return SetFileAttributesW(path.wstring().c_str(), attrs) != 0;
}

#else

SecurityInfo PlatformSecurity::get_security_info_posix() {
  SecurityInfo info;
  info.is_elevated = is_elevated();
  info.current_user = get_current_user();
  info.groups = get_user_groups();
  info.has_admin_rights = is_elevated();
  return info;
}

std::optional<FilePermissions> PlatformSecurity::get_file_permissions_posix(
    const std::filesystem::path& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return std::nullopt;
  }

  FilePermissions perms;
  perms.mode = st.st_mode & 0777;

  uid_t uid = getuid();
  gid_t gid = getgid();

  perms.is_owner = (st.st_uid == uid);

  // Get owner name
  struct passwd* pw = getpwuid(st.st_uid);
  perms.owner = pw ? std::string(pw->pw_name) : "unknown";

  // Get group name
  struct group* gr = getgrgid(st.st_gid);
  perms.group = gr ? std::string(gr->gr_name) : "unknown";

  // Check permissions
  if (perms.is_owner) {
    perms.can_read = (st.st_mode & S_IRUSR) != 0;
    perms.can_write = (st.st_mode & S_IWUSR) != 0;
    perms.can_execute = (st.st_mode & S_IXUSR) != 0;
  } else if (st.st_gid == gid) {
    perms.can_read = (st.st_mode & S_IRGRP) != 0;
    perms.can_write = (st.st_mode & S_IWGRP) != 0;
    perms.can_execute = (st.st_mode & S_IXGRP) != 0;
  } else {
    perms.can_read = (st.st_mode & S_IROTH) != 0;
    perms.can_write = (st.st_mode & S_IWOTH) != 0;
    perms.can_execute = (st.st_mode & S_IXOTH) != 0;
  }

  return perms;
}

bool PlatformSecurity::set_file_permissions_posix(const std::filesystem::path& path, int mode) {
  return chmod(path.c_str(), mode) == 0;
}

#endif

}  // namespace SolarSystem::Utils::Platform
