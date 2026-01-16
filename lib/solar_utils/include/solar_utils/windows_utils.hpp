/**
 * @file windows_utils.hpp
 * @brief Windows-specific utility functions and system integration
 */

#pragma once

#ifdef _WIN32

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Utils::Windows {

/**
 * @brief Windows system information
 */
struct WindowsSystemInfo {
  std::string os_version;
  std::string build_number;
  std::string architecture;
  bool is_64bit;
  size_t total_memory_mb;
  size_t available_memory_mb;
  int processor_count;
};

/**
 * @brief Windows registry value types
 */
enum class RegistryValueType { String, DWord, QWord, Binary, MultiString };

/**
 * @brief Windows service status
 */
enum class ServiceStatus { Running, Stopped, Paused, StartPending, StopPending, Unknown };

/**
 * @brief Windows utilities class
 */
class WindowsUtils {
 public:
  /**
   * @brief Get Windows system information
   */
  static WindowsSystemInfo get_system_info();

  /**
   * @brief Get Windows version string
   */
  static std::string get_windows_version();

  /**
   * @brief Check if running on Windows 10 or later
   */
  static bool is_windows_10_or_later();

  /**
   * @brief Get system architecture (x86, x64, ARM64)
   */
  static std::string get_architecture();

  /**
   * @brief Get number of processors
   */
  static int get_processor_count();

  /**
   * @brief Get total physical memory in MB
   */
  static size_t get_total_memory_mb();

  /**
   * @brief Get available physical memory in MB
   */
  static size_t get_available_memory_mb();
};

/**
 * @brief Windows Registry utilities
 */
class RegistryUtils {
 public:
  /**
   * @brief Read string value from registry
   */
  static std::optional<std::string> read_string(const std::string& key_path,
                                                const std::string& value_name);

  /**
   * @brief Read DWORD value from registry
   */
  static std::optional<uint32_t> read_dword(const std::string& key_path,
                                            const std::string& value_name);

  /**
   * @brief Write string value to registry
   */
  static bool write_string(const std::string& key_path, const std::string& value_name,
                           const std::string& value);

  /**
   * @brief Write DWORD value to registry
   */
  static bool write_dword(const std::string& key_path, const std::string& value_name,
                          uint32_t value);

  /**
   * @brief Check if registry key exists
   */
  static bool key_exists(const std::string& key_path);

  /**
   * @brief Delete registry key
   */
  static bool delete_key(const std::string& key_path);

  /**
   * @brief List all value names in a key
   */
  static std::vector<std::string> list_values(const std::string& key_path);
};

/**
 * @brief Windows Service utilities
 */
class ServiceUtils {
 public:
  /**
   * @brief Check if service exists
   */
  static bool service_exists(const std::string& service_name);

  /**
   * @brief Get service status
   */
  static ServiceStatus get_service_status(const std::string& service_name);

  /**
   * @brief Start a service
   */
  static bool start_service(const std::string& service_name);

  /**
   * @brief Stop a service
   */
  static bool stop_service(const std::string& service_name);

  /**
   * @brief Install a service
   */
  static bool install_service(const std::string& service_name, const std::string& display_name,
                              const std::filesystem::path& executable_path);

  /**
   * @brief Uninstall a service
   */
  static bool uninstall_service(const std::string& service_name);
};

/**
 * @brief Windows file system utilities
 */
class WindowsFileSystem {
 public:
  /**
   * @brief Get file attributes
   */
  static uint32_t get_file_attributes(const std::filesystem::path& path);

  /**
   * @brief Set file attributes
   */
  static bool set_file_attributes(const std::filesystem::path& path, uint32_t attributes);

  /**
   * @brief Check if file is hidden
   */
  static bool is_hidden(const std::filesystem::path& path);

  /**
   * @brief Set file hidden attribute
   */
  static bool set_hidden(const std::filesystem::path& path, bool hidden);

  /**
   * @brief Check if file is read-only
   */
  static bool is_readonly(const std::filesystem::path& path);

  /**
   * @brief Set file read-only attribute
   */
  static bool set_readonly(const std::filesystem::path& path, bool readonly);

  /**
   * @brief Get short path name (8.3 format)
   */
  static std::optional<std::string> get_short_path(const std::filesystem::path& path);

  /**
   * @brief Get long path name
   */
  static std::optional<std::string> get_long_path(const std::filesystem::path& path);
};

}  // namespace SolarSystem::Utils::Windows

#endif  // _WIN32
