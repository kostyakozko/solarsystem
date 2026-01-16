/**
 * @file windows_utils.cpp
 * @brief Implementation of Windows-specific utilities
 */

#ifdef _WIN32

#include "solar_utils/windows_utils.hpp"

#include <psapi.h>
#include <windows.h>
#include <winsvc.h>

#include <sstream>

namespace SolarSystem::Utils::Windows {

// WindowsUtils implementation

WindowsSystemInfo WindowsUtils::get_system_info() {
  WindowsSystemInfo info;

  info.os_version = get_windows_version();
  info.architecture = get_architecture();
  info.is_64bit = (sizeof(void*) == 8);
  info.processor_count = get_processor_count();
  info.total_memory_mb = get_total_memory_mb();
  info.available_memory_mb = get_available_memory_mb();

  // Get build number
  OSVERSIONINFOEXW osvi = {};
  osvi.dwOSVersionInfoSize = sizeof(osvi);

#pragma warning(push)
#pragma warning(disable : 4996)  // Suppress deprecation warning
  if (GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi))) {
    info.build_number = std::to_string(osvi.dwBuildNumber);
  }
#pragma warning(pop)

  return info;
}

std::string WindowsUtils::get_windows_version() {
  OSVERSIONINFOEXW osvi = {};
  osvi.dwOSVersionInfoSize = sizeof(osvi);

#pragma warning(push)
#pragma warning(disable : 4996)
  if (GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi))) {
    std::ostringstream oss;
    oss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion;
    return oss.str();
  }
#pragma warning(pop)

  return "Unknown";
}

bool WindowsUtils::is_windows_10_or_later() {
  OSVERSIONINFOEXW osvi = {};
  osvi.dwOSVersionInfoSize = sizeof(osvi);

#pragma warning(push)
#pragma warning(disable : 4996)
  if (GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi))) {
    return osvi.dwMajorVersion >= 10;
  }
#pragma warning(pop)

  return false;
}

std::string WindowsUtils::get_architecture() {
  SYSTEM_INFO si;
  GetNativeSystemInfo(&si);

  switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      return "x64";
    case PROCESSOR_ARCHITECTURE_ARM:
      return "ARM";
    case PROCESSOR_ARCHITECTURE_ARM64:
      return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:
      return "x86";
    default:
      return "Unknown";
  }
}

int WindowsUtils::get_processor_count() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return static_cast<int>(si.dwNumberOfProcessors);
}

size_t WindowsUtils::get_total_memory_mb() {
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);

  if (GlobalMemoryStatusEx(&memInfo)) {
    return static_cast<size_t>(memInfo.ullTotalPhys / (1024 * 1024));
  }

  return 0;
}

size_t WindowsUtils::get_available_memory_mb() {
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);

  if (GlobalMemoryStatusEx(&memInfo)) {
    return static_cast<size_t>(memInfo.ullAvailPhys / (1024 * 1024));
  }

  return 0;
}

// RegistryUtils implementation

std::optional<std::string> RegistryUtils::read_string(const std::string& key_path,
                                                      const std::string& value_name) {
  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key_path.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    return std::nullopt;
  }

  char buffer[1024];
  DWORD bufferSize = sizeof(buffer);
  DWORD type;

  LONG result = RegQueryValueExA(hKey, value_name.c_str(), nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer), &bufferSize);

  RegCloseKey(hKey);

  if (result == ERROR_SUCCESS && type == REG_SZ) {
    return std::string(buffer);
  }

  return std::nullopt;
}

std::optional<uint32_t> RegistryUtils::read_dword(const std::string& key_path,
                                                  const std::string& value_name) {
  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key_path.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    return std::nullopt;
  }

  DWORD value;
  DWORD bufferSize = sizeof(value);
  DWORD type;

  LONG result = RegQueryValueExA(hKey, value_name.c_str(), nullptr, &type,
                                 reinterpret_cast<LPBYTE>(&value), &bufferSize);

  RegCloseKey(hKey);

  if (result == ERROR_SUCCESS && type == REG_DWORD) {
    return value;
  }

  return std::nullopt;
}

bool RegistryUtils::write_string(const std::string& key_path, const std::string& value_name,
                                 const std::string& value) {
  HKEY hKey;
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, key_path.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                      KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
    return false;
  }

  LONG result = RegSetValueExA(hKey, value_name.c_str(), 0, REG_SZ,
                               reinterpret_cast<const BYTE*>(value.c_str()),
                               static_cast<DWORD>(value.length() + 1));

  RegCloseKey(hKey);

  return result == ERROR_SUCCESS;
}

bool RegistryUtils::write_dword(const std::string& key_path, const std::string& value_name,
                                uint32_t value) {
  HKEY hKey;
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, key_path.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                      KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
    return false;
  }

  LONG result = RegSetValueExA(hKey, value_name.c_str(), 0, REG_DWORD,
                               reinterpret_cast<const BYTE*>(&value), sizeof(value));

  RegCloseKey(hKey);

  return result == ERROR_SUCCESS;
}

bool RegistryUtils::key_exists(const std::string& key_path) {
  HKEY hKey;
  LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, key_path.c_str(), 0, KEY_READ, &hKey);

  if (result == ERROR_SUCCESS) {
    RegCloseKey(hKey);
    return true;
  }

  return false;
}

bool RegistryUtils::delete_key(const std::string& key_path) {
  return RegDeleteKeyA(HKEY_LOCAL_MACHINE, key_path.c_str()) == ERROR_SUCCESS;
}

std::vector<std::string> RegistryUtils::list_values(const std::string& key_path) {
  std::vector<std::string> values;

  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key_path.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    return values;
  }

  DWORD index = 0;
  char valueName[256];
  DWORD valueNameSize;

  while (true) {
    valueNameSize = sizeof(valueName);
    LONG result =
        RegEnumValueA(hKey, index++, valueName, &valueNameSize, nullptr, nullptr, nullptr, nullptr);

    if (result == ERROR_SUCCESS) {
      values.push_back(std::string(valueName));
    } else {
      break;
    }
  }

  RegCloseKey(hKey);
  return values;
}

// ServiceUtils implementation

bool ServiceUtils::service_exists(const std::string& service_name) {
  SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm) {
    return false;
  }

  SC_HANDLE service = OpenServiceA(scm, service_name.c_str(), SERVICE_QUERY_STATUS);
  bool exists = (service != nullptr);

  if (service) CloseServiceHandle(service);
  CloseServiceHandle(scm);

  return exists;
}

ServiceStatus ServiceUtils::get_service_status(const std::string& service_name) {
  SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm) {
    return ServiceStatus::Unknown;
  }

  SC_HANDLE service = OpenServiceA(scm, service_name.c_str(), SERVICE_QUERY_STATUS);
  if (!service) {
    CloseServiceHandle(scm);
    return ServiceStatus::Unknown;
  }

  SERVICE_STATUS status;
  if (!QueryServiceStatus(service, &status)) {
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ServiceStatus::Unknown;
  }

  CloseServiceHandle(service);
  CloseServiceHandle(scm);

  switch (status.dwCurrentState) {
    case SERVICE_RUNNING:
      return ServiceStatus::Running;
    case SERVICE_STOPPED:
      return ServiceStatus::Stopped;
    case SERVICE_PAUSED:
      return ServiceStatus::Paused;
    case SERVICE_START_PENDING:
      return ServiceStatus::StartPending;
    case SERVICE_STOP_PENDING:
      return ServiceStatus::StopPending;
    default:
      return ServiceStatus::Unknown;
  }
}

bool ServiceUtils::start_service(const std::string& service_name) {
  SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm) {
    return false;
  }

  SC_HANDLE service = OpenServiceA(scm, service_name.c_str(), SERVICE_START);
  if (!service) {
    CloseServiceHandle(scm);
    return false;
  }

  bool result = StartServiceA(service, 0, nullptr) != 0;

  CloseServiceHandle(service);
  CloseServiceHandle(scm);

  return result;
}

bool ServiceUtils::stop_service(const std::string& service_name) {
  SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm) {
    return false;
  }

  SC_HANDLE service = OpenServiceA(scm, service_name.c_str(), SERVICE_STOP);
  if (!service) {
    CloseServiceHandle(scm);
    return false;
  }

  SERVICE_STATUS status;
  bool result = ControlService(service, SERVICE_CONTROL_STOP, &status) != 0;

  CloseServiceHandle(service);
  CloseServiceHandle(scm);

  return result;
}

bool ServiceUtils::install_service(const std::string& service_name, const std::string& display_name,
                                   const std::filesystem::path& executable_path) {
  SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
  if (!scm) {
    return false;
  }

  SC_HANDLE service =
      CreateServiceA(scm, service_name.c_str(), display_name.c_str(), SERVICE_ALL_ACCESS,
                     SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                     executable_path.string().c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);

  bool result = (service != nullptr);

  if (service) CloseServiceHandle(service);
  CloseServiceHandle(scm);

  return result;
}

bool ServiceUtils::uninstall_service(const std::string& service_name) {
  SC_HANDLE scm = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm) {
    return false;
  }

  SC_HANDLE service = OpenServiceA(scm, service_name.c_str(), DELETE);
  if (!service) {
    CloseServiceHandle(scm);
    return false;
  }

  bool result = DeleteService(service) != 0;

  CloseServiceHandle(service);
  CloseServiceHandle(scm);

  return result;
}

// WindowsFileSystem implementation

uint32_t WindowsFileSystem::get_file_attributes(const std::filesystem::path& path) {
  return GetFileAttributesW(path.wstring().c_str());
}

bool WindowsFileSystem::set_file_attributes(const std::filesystem::path& path,
                                            uint32_t attributes) {
  return SetFileAttributesW(path.wstring().c_str(), attributes) != 0;
}

bool WindowsFileSystem::is_hidden(const std::filesystem::path& path) {
  DWORD attrs = get_file_attributes(path);
  return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_HIDDEN);
}

bool WindowsFileSystem::set_hidden(const std::filesystem::path& path, bool hidden) {
  DWORD attrs = get_file_attributes(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return false;
  }

  if (hidden) {
    attrs |= FILE_ATTRIBUTE_HIDDEN;
  } else {
    attrs &= ~FILE_ATTRIBUTE_HIDDEN;
  }

  return set_file_attributes(path, attrs);
}

bool WindowsFileSystem::is_readonly(const std::filesystem::path& path) {
  DWORD attrs = get_file_attributes(path);
  return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_READONLY);
}

bool WindowsFileSystem::set_readonly(const std::filesystem::path& path, bool readonly) {
  DWORD attrs = get_file_attributes(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return false;
  }

  if (readonly) {
    attrs |= FILE_ATTRIBUTE_READONLY;
  } else {
    attrs &= ~FILE_ATTRIBUTE_READONLY;
  }

  return set_file_attributes(path, attrs);
}

std::optional<std::string> WindowsFileSystem::get_short_path(const std::filesystem::path& path) {
  wchar_t shortPath[MAX_PATH];
  DWORD result = GetShortPathNameW(path.wstring().c_str(), shortPath, MAX_PATH);

  if (result > 0 && result < MAX_PATH) {
    std::wstring ws(shortPath);
    return std::string(ws.begin(), ws.end());
  }

  return std::nullopt;
}

std::optional<std::string> WindowsFileSystem::get_long_path(const std::filesystem::path& path) {
  wchar_t longPath[MAX_PATH];
  DWORD result = GetLongPathNameW(path.wstring().c_str(), longPath, MAX_PATH);

  if (result > 0 && result < MAX_PATH) {
    std::wstring ws(longPath);
    return std::string(ws.begin(), ws.end());
  }

  return std::nullopt;
}

}  // namespace SolarSystem::Utils::Windows

#endif  // _WIN32
