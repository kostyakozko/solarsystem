/**
 * @file file_security.cpp
 * @brief Implementation of file system security
 */

#include "solar_core/security/file_security.hpp"

#include <algorithm>
#include <fstream>

namespace SolarSystem::Security {

// WhitelistPolicy implementation
void WhitelistPolicy::add_allowed_directory(const std::filesystem::path& dir) {
  allowed_directories_.push_back(dir);
}

void WhitelistPolicy::add_allowed_file(const std::filesystem::path& file) {
  allowed_files_.push_back(file);
}

void WhitelistPolicy::remove_allowed_directory(const std::filesystem::path& dir) {
  allowed_directories_.erase(
      std::remove(allowed_directories_.begin(), allowed_directories_.end(), dir),
      allowed_directories_.end());
}

FileSecurityResult WhitelistPolicy::check_access(
    const std::filesystem::path& path,
    FileAccessMode /* mode */) const {

  // Check if file is explicitly allowed
  for (const auto& allowed_file : allowed_files_) {
    if (path == allowed_file) {
      return FileSecurityResult(true, "File is in whitelist");
    }
  }

  // Check if file is in an allowed directory
  for (const auto& allowed_dir : allowed_directories_) {
    auto rel_path = std::filesystem::relative(path, allowed_dir);
    if (!rel_path.empty() && rel_path.native()[0] != '.') {
      return FileSecurityResult(true, "File is in allowed directory");
    }
  }

  return FileSecurityResult(false, "File not in whitelist");
}

// FileSystemSecurity implementation
FileSystemSecurity& FileSystemSecurity::instance() {
  static FileSystemSecurity instance;
  return instance;
}

void FileSystemSecurity::set_policy(std::shared_ptr<FileSecurityPolicy> policy) {
  policy_ = std::move(policy);
}

std::shared_ptr<FileSecurityPolicy> FileSystemSecurity::get_policy() const {
  return policy_;
}

FileSecurityResult FileSystemSecurity::check_file_access(
    const std::filesystem::path& path,
    FileAccessMode mode) const {

  if (!policy_) {
    return FileSecurityResult(true, "No policy set - allowing access");
  }

  auto result = policy_->check_access(path, mode);
  log_access(path, mode, result.allowed);
  return result;
}

bool FileSystemSecurity::can_read(const std::filesystem::path& path) const {
  return check_file_access(path, FileAccessMode::READ).allowed;
}

bool FileSystemSecurity::can_write(const std::filesystem::path& path) const {
  return check_file_access(path, FileAccessMode::WRITE).allowed;
}

bool FileSystemSecurity::can_execute(const std::filesystem::path& path) const {
  return check_file_access(path, FileAccessMode::EXECUTE).allowed;
}

bool FileSystemSecurity::can_delete(const std::filesystem::path& path) const {
  return check_file_access(path, FileAccessMode::DELETE).allowed;
}

bool FileSystemSecurity::check_permissions(const std::filesystem::path& path) const {
  if (!std::filesystem::exists(path)) {
    return false;
  }

  // Platform-specific permission checking
#ifdef _WIN32
  return check_permissions_windows(path);
#else
  return check_permissions_posix(path);
#endif
}

#ifdef _WIN32
#include <windows.h>
#include <aclapi.h>

bool FileSystemSecurity::check_permissions_windows(const std::filesystem::path& path) const {
  // Get file security descriptor
  PSECURITY_DESCRIPTOR pSD = nullptr;
  PACL pDacl = nullptr;

  DWORD result = GetNamedSecurityInfoW(
      path.wstring().c_str(),
      SE_FILE_OBJECT,
      DACL_SECURITY_INFORMATION,
      nullptr,
      nullptr,
      &pDacl,
      nullptr,
      &pSD
  );

  if (result != ERROR_SUCCESS) {
    return false;
  }

  // Check if we have access
  HANDLE hToken = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
    if (pSD) LocalFree(pSD);
    return false;
  }

  // Check read access
  DWORD accessMask = GENERIC_READ;
  PRIVILEGE_SET privilegeSet;
  DWORD privilegeSetLength = sizeof(PRIVILEGE_SET);
  DWORD grantedAccess = 0;
  BOOL accessStatus = FALSE;

  GENERIC_MAPPING mapping = {
      FILE_GENERIC_READ,
      FILE_GENERIC_WRITE,
      FILE_GENERIC_EXECUTE,
      FILE_ALL_ACCESS
  };

  MapGenericMask(&accessMask, &mapping);

  BOOL result_check = AccessCheck(
      pSD,
      hToken,
      accessMask,
      &mapping,
      &privilegeSet,
      &privilegeSetLength,
      &grantedAccess,
      &accessStatus
  );

  CloseHandle(hToken);
  if (pSD) LocalFree(pSD);

  return result_check && accessStatus;
}
#else
#include <sys/stat.h>
#include <unistd.h>

bool FileSystemSecurity::check_permissions_posix(const std::filesystem::path& path) const {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }

  // Get current user and group
  uid_t uid = getuid();
  gid_t gid = getgid();

  // Check owner permissions
  if (st.st_uid == uid) {
    return (st.st_mode & S_IRUSR) != 0;
  }

  // Check group permissions
  if (st.st_gid == gid) {
    return (st.st_mode & S_IRGRP) != 0;
  }

  // Check other permissions
  return (st.st_mode & S_IROTH) != 0;
}
#endif

bool FileSystemSecurity::is_safe_path(const std::filesystem::path& path) const {
  std::string path_str = path.string();

  // Check for path traversal attempts
  if (path_str.find("..") != std::string::npos) {
    return false;
  }

  // Check for absolute paths that might be dangerous
  if (path.is_absolute()) {
    // Allow only certain absolute paths
    std::string abs_str = path.string();
    if (abs_str.find("/etc") == 0 || abs_str.find("/sys") == 0) {
      return false;
    }
  }

  return true;
}

bool FileSystemSecurity::validate_path(const std::filesystem::path& path) const {
  return is_safe_path(path);
}

std::optional<std::filesystem::path> FileSystemSecurity::sanitize_path(
    const std::filesystem::path& path) const {

  if (!is_safe_path(path)) {
    return std::nullopt;
  }

  // Return canonical path if possible
  std::error_code ec;
  auto canonical = std::filesystem::canonical(path, ec);
  if (!ec) {
    return canonical;
  }

  return path;
}

void FileSystemSecurity::log_access(const std::filesystem::path& path,
                                   FileAccessMode mode,
                                   bool allowed) const {
  std::string mode_str;
  switch (mode) {
    case FileAccessMode::READ: mode_str = "READ"; break;
    case FileAccessMode::WRITE: mode_str = "WRITE"; break;
    case FileAccessMode::EXECUTE: mode_str = "EXECUTE"; break;
    case FileAccessMode::DELETE: mode_str = "DELETE"; break;
  }

  std::string log_entry = (allowed ? "ALLOWED: " : "DENIED: ") +
                         mode_str + " " + path.string();
  audit_log_.push_back(log_entry);
}

// SecureFileOperations implementation
std::optional<std::string> SecureFileOperations::read_file(
    const std::filesystem::path& path) {

  auto& security = FileSystemSecurity::instance();
  if (!security.can_read(path)) {
    return std::nullopt;
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
  return content;
}

bool SecureFileOperations::write_file(
    const std::filesystem::path& path,
    const std::string& content) {

  auto& security = FileSystemSecurity::instance();
  if (!security.can_write(path)) {
    return false;
  }

  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }

  file << content;
  return file.good();
}

bool SecureFileOperations::delete_file(
    const std::filesystem::path& path) {

  auto& security = FileSystemSecurity::instance();
  if (!security.can_delete(path)) {
    return false;
  }

  std::error_code ec;
  return std::filesystem::remove(path, ec);
}

bool SecureFileOperations::create_directory(
    const std::filesystem::path& path) {

  auto& security = FileSystemSecurity::instance();
  if (!security.can_write(path)) {
    return false;
  }

  std::error_code ec;
  return std::filesystem::create_directories(path, ec);
}

std::vector<std::filesystem::path> SecureFileOperations::list_directory(
    const std::filesystem::path& path) {

  auto& security = FileSystemSecurity::instance();
  if (!security.can_read(path)) {
    return {};
  }

  std::vector<std::filesystem::path> entries;
  std::error_code ec;

  for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
    if (!ec) {
      entries.push_back(entry.path());
    }
  }

  return entries;
}

}  // namespace SolarSystem::Security
