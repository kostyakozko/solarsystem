/**
 * @file platform_apis.hpp
 * @brief Platform-specific API integrations
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Utils::Platform {

/**
 * @brief Process information
 */
struct ProcessInfo {
  int pid;
  std::string name;
  std::string command_line;
  size_t memory_usage_kb;
  double cpu_usage_percent;
  std::string user;
};

/**
 * @brief System resource information
 */
struct SystemResources {
  size_t total_memory_mb;
  size_t available_memory_mb;
  size_t used_memory_mb;
  double memory_usage_percent;
  double cpu_usage_percent;
  size_t disk_total_gb;
  size_t disk_available_gb;
  double disk_usage_percent;
};

/**
 * @brief Cross-platform system APIs
 */
class SystemAPIs {
 public:
  /**
   * @brief Get current process ID
   */
  static int get_current_pid();

  /**
   * @brief Get parent process ID
   */
  static int get_parent_pid();

  /**
   * @brief Get process information
   */
  static std::optional<ProcessInfo> get_process_info(int pid);

  /**
   * @brief List all running processes
   */
  static std::vector<ProcessInfo> list_processes();

  /**
   * @brief Kill a process
   */
  static bool kill_process(int pid);

  /**
   * @brief Get system resource usage
   */
  static SystemResources get_system_resources();

  /**
   * @brief Get system uptime in seconds
   */
  static uint64_t get_system_uptime();

  /**
   * @brief Get current username
   */
  static std::string get_username();

  /**
   * @brief Get hostname
   */
  static std::string get_hostname();

  /**
   * @brief Get environment variable
   */
  static std::optional<std::string> get_env(const std::string& name);

  /**
   * @brief Set environment variable
   */
  static bool set_env(const std::string& name, const std::string& value);
};

#ifdef __APPLE__

/**
 * @brief macOS-specific APIs
 */
class MacOSAPIs {
 public:
  /**
   * @brief Get macOS version
   */
  static std::string get_macos_version();

  /**
   * @brief Get system information using sysctl
   */
  static std::optional<std::string> get_sysctl_string(const std::string& name);
  static std::optional<int> get_sysctl_int(const std::string& name);

  /**
   * @brief Get hardware model
   */
  static std::string get_hardware_model();

  /**
   * @brief Get CPU brand
   */
  static std::string get_cpu_brand();

  /**
   * @brief Check if running on Apple Silicon
   */
  static bool is_apple_silicon();

  /**
   * @brief Get bundle identifier for current app
   */
  static std::optional<std::string> get_bundle_identifier();

  /**
   * @brief Get application support directory
   */
  static std::filesystem::path get_application_support_dir();

  /**
   * @brief Get user library directory
   */
  static std::filesystem::path get_user_library_dir();
};

#endif  // __APPLE__

#ifdef __linux__

/**
 * @brief Linux-specific APIs
 */
class LinuxAPIs {
 public:
  /**
   * @brief Get Linux distribution name
   */
  static std::string get_distribution_name();

  /**
   * @brief Get Linux distribution version
   */
  static std::string get_distribution_version();

  /**
   * @brief Read from /proc filesystem
   */
  static std::optional<std::string> read_proc_file(const std::string& path);

  /**
   * @brief Get CPU information from /proc/cpuinfo
   */
  static std::string get_cpu_info();

  /**
   * @brief Get memory information from /proc/meminfo
   */
  static std::string get_memory_info();

  /**
   * @brief Check if running in container (Docker, LXC, etc)
   */
  static bool is_running_in_container();

  /**
   * @brief Get cgroup information
   */
  static std::optional<std::string> get_cgroup_info();

  /**
   * @brief Get systemd version (if available)
   */
  static std::optional<std::string> get_systemd_version();
};

#endif  // __linux__

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

/**
 * @brief BSD-specific APIs
 */
class BSDAPIs {
 public:
  /**
   * @brief Get BSD variant name
   */
  static std::string get_bsd_variant();

  /**
   * @brief Get system information using sysctl
   */
  static std::optional<std::string> get_sysctl_string(const std::string& name);
  static std::optional<int> get_sysctl_int(const std::string& name);

  /**
   * @brief Get kernel version
   */
  static std::string get_kernel_version();

  /**
   * @brief Get hardware platform
   */
  static std::string get_hardware_platform();
};

#endif  // BSD

}  // namespace SolarSystem::Utils::Platform
