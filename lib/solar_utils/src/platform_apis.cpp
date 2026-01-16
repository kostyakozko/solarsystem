/**
 * @file platform_apis.cpp
 * @brief Implementation of platform-specific APIs
 */

#include "solar_utils/platform_apis.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <psapi.h>
#include <tlhelp32.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <pwd.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <pwd.h>
#include <signal.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace SolarSystem::Utils::Platform {

// SystemAPIs implementation

int SystemAPIs::get_current_pid() {
#ifdef _WIN32
  return static_cast<int>(GetCurrentProcessId());
#else
  return static_cast<int>(getpid());
#endif
}

int SystemAPIs::get_parent_pid() {
#ifdef _WIN32
  // Windows implementation would require process snapshot
  return 0;
#else
  return static_cast<int>(getppid());
#endif
}

std::optional<ProcessInfo> SystemAPIs::get_process_info(int pid) {
  ProcessInfo info;
  info.pid = pid;

#ifdef _WIN32
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (!hProcess) {
    return std::nullopt;
  }

  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
    info.memory_usage_kb = pmc.WorkingSetSize / 1024;
  }

  CloseHandle(hProcess);
#else
  // Unix-like implementation using /proc filesystem
  std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");
  if (stat_file.is_open()) {
    std::string line;
    std::getline(stat_file, line);

    // Parse /proc/pid/stat format: pid (name) state ppid ...
    size_t start = line.find('(');
    size_t end = line.rfind(')');
    if (start != std::string::npos && end != std::string::npos && end > start) {
      info.name = line.substr(start + 1, end - start - 1);
    } else {
      info.name = "process_" + std::to_string(pid);
    }

    // Parse memory usage from /proc/pid/status
    std::ifstream status_file("/proc/" + std::to_string(pid) + "/status");
    if (status_file.is_open()) {
      std::string status_line;
      while (std::getline(status_file, status_line)) {
        if (status_line.find("VmRSS:") == 0) {
          std::istringstream iss(status_line.substr(6));
          size_t mem_kb;
          if (iss >> mem_kb) {
            info.memory_usage_kb = mem_kb;
          }
          break;
        }
      }
    }

    // Get command line
    std::ifstream cmdline_file("/proc/" + std::to_string(pid) + "/cmdline");
    if (cmdline_file.is_open()) {
      std::getline(cmdline_file, info.command_line, '\0');
    }

    // Get user
    struct stat st;
    std::string proc_path = "/proc/" + std::to_string(pid);
    if (stat(proc_path.c_str(), &st) == 0) {
      struct passwd* pw = getpwuid(st.st_uid);
      if (pw) {
        info.user = std::string(pw->pw_name);
      }
    }
  } else {
    return std::nullopt;
  }
#endif

  return info;
}

std::vector<ProcessInfo> SystemAPIs::list_processes() {
  std::vector<ProcessInfo> processes;

#ifdef _WIN32
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE) {
    return processes;
  }

  PROCESSENTRY32 pe32;
  pe32.dwSize = sizeof(PROCESSENTRY32);

  if (Process32First(hSnapshot, &pe32)) {
    do {
      ProcessInfo info;
      info.pid = static_cast<int>(pe32.th32ProcessID);
      info.name = std::string(pe32.szExeFile);
      processes.push_back(info);
    } while (Process32Next(hSnapshot, &pe32));
  }

  CloseHandle(hSnapshot);
#else
  // Unix-like: read /proc directory
  try {
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
      if (entry.is_directory()) {
        std::string dirname = entry.path().filename().string();
        // Check if directory name is a number (PID)
        if (!dirname.empty() && std::all_of(dirname.begin(), dirname.end(), ::isdigit)) {
          int pid = std::stoi(dirname);
          auto proc_info = get_process_info(pid);
          if (proc_info) {
            processes.push_back(*proc_info);
          }
        }
      }
    }
  } catch (...) {
    // Failed to read /proc
  }
#endif

  return processes;
}

bool SystemAPIs::kill_process(int pid) {
#ifdef _WIN32
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if (!hProcess) {
    return false;
  }

  bool result = TerminateProcess(hProcess, 0) != 0;
  CloseHandle(hProcess);
  return result;
#else
  return kill(pid, SIGTERM) == 0;
#endif
}

SystemResources SystemAPIs::get_system_resources() {
  SystemResources resources = {};

#ifdef _WIN32
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  if (GlobalMemoryStatusEx(&memInfo)) {
    resources.total_memory_mb = memInfo.ullTotalPhys / (1024 * 1024);
    resources.available_memory_mb = memInfo.ullAvailPhys / (1024 * 1024);
    resources.used_memory_mb = resources.total_memory_mb - resources.available_memory_mb;
    resources.memory_usage_percent =
        (static_cast<double>(resources.used_memory_mb) / resources.total_memory_mb) * 100.0;
  }
#elif defined(__APPLE__)
  int mib[2];
  int64_t physical_memory;
  size_t length

      mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  length = sizeof(int64_t);
  sysctl(mib, 2, &physical_memory, &length, nullptr, 0);

  resources.total_memory_mb = physical_memory / (1024 * 1024);

  vm_size_t page_size;
  mach_port_t mach_port = mach_host_self();
  mach_msg_type_number_t count = sizeof(vm_statistics_data_t) / sizeof(integer_t);
  vm_statistics_data_t vm_stats;

  if (host_page_size(mach_port, &page_size) == KERN_SUCCESS &&
      host_statistics(mach_port, HOST_VM_INFO, reinterpret_cast<host_info_t>(&vm_stats), &count) ==
          KERN_SUCCESS) {
    int64_t free_memory = static_cast<int64_t>(vm_stats.free_count) * page_size;
    resources.available_memory_mb = free_memory / (1024 * 1024);
    resources.used_memory_mb = resources.total_memory_mb - resources.available_memory_mb;
    resources.memory_usage_percent =
        (static_cast<double>(resources.used_memory_mb) / resources.total_memory_mb) * 100.0;
  }
#else
  struct sysinfo si;
  if (sysinfo(&si) == 0) {
    resources.total_memory_mb = si.totalram / (1024 * 1024);
    resources.available_memory_mb = si.freeram / (1024 * 1024);
    resources.used_memory_mb = resources.total_memory_mb - resources.available_memory_mb;
    resources.memory_usage_percent =
        (static_cast<double>(resources.used_memory_mb) / resources.total_memory_mb) * 100.0;
  }
#endif

  return resources;
}

uint64_t SystemAPIs::get_system_uptime() {
#ifdef _WIN32
  return GetTickCount64() / 1000;
#elif defined(__APPLE__)
  struct timeval boottime;
  size_t len = sizeof(boottime);
  int mib[2] = {CTL_KERN, KERN_BOOTTIME};

  if (sysctl(mib, 2, &boottime, &len, nullptr, 0) == 0) {
    time_t now = time(nullptr);
    return static_cast<uint64_t>(now - boottime.tv_sec);
  }
  return 0;
#else
  struct sysinfo si;
  if (sysinfo(&si) == 0) {
    return static_cast<uint64_t>(si.uptime);
  }
  return 0;
#endif
}

std::string SystemAPIs::get_username() {
#ifdef _WIN32
  char username[256];
  DWORD username_len = sizeof(username);
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

std::string SystemAPIs::get_hostname() {
  char hostname[256];
#ifdef _WIN32
  DWORD size = sizeof(hostname);
  if (GetComputerNameA(hostname, &size)) {
    return std::string(hostname);
  }
#else
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    return std::string(hostname);
  }
#endif
  return "unknown";
}

std::optional<std::string> SystemAPIs::get_env(const std::string& name) {
  const char* value = std::getenv(name.c_str());
  if (value) {
    return std::string(value);
  }
  return std::nullopt;
}

bool SystemAPIs::set_env(const std::string& name, const std::string& value) {
#ifdef _WIN32
  return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
  return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

#ifdef __APPLE__

// MacOSAPIs implementation

std::string MacOSAPIs::get_macos_version() {
  auto version = get_sysctl_string("kern.osproductversion");
  return version.value_or("Unknown");
}

std::optional<std::string> MacOSAPIs::get_sysctl_string(const std::string& name) {
  char buffer[256];
  size_t size = sizeof(buffer);

  if (sysctlbyname(name.c_str(), buffer, &size, nullptr, 0) == 0) {
    return std::string(buffer);
  }

  return std::nullopt;
}

std::optional<int> MacOSAPIs::get_sysctl_int(const std::string& name) {
  int value;
  size_t size = sizeof(value);

  if (sysctlbyname(name.c_str(), &value, &size, nullptr, 0) == 0) {
    return value;
  }

  return std::nullopt;
}

std::string MacOSAPIs::get_hardware_model() {
  auto model = get_sysctl_string("hw.model");
  return model.value_or("Unknown");
}

std::string MacOSAPIs::get_cpu_brand() {
  auto brand = get_sysctl_string("machdep.cpu.brand_string");
  return brand.value_or("Unknown CPU");
}

bool MacOSAPIs::is_apple_silicon() {
  auto arch = get_sysctl_string("hw.machine");
  return arch && arch->find("arm") != std::string::npos;
}

std::optional<std::string> MacOSAPIs::get_bundle_identifier() {
  // Get bundle identifier from Info.plist
  // First try to find the main bundle's Info.plist
  const char* paths[] = {"../Resources/Info.plist", "../../Resources/Info.plist",
                         "../../../Resources/Info.plist"};

  for (const char* path : paths) {
    std::ifstream plist(path);
    if (plist.is_open()) {
      std::string line;
      bool found_key = false;
      while (std::getline(plist, line)) {
        if (line.find("CFBundleIdentifier") != std::string::npos) {
          found_key = true;
        } else if (found_key && line.find("<string>") != std::string::npos) {
          size_t start = line.find("<string>") + 8;
          size_t end = line.find("</string>");
          if (end != std::string::npos && end > start) {
            return line.substr(start, end - start);
          }
        }
      }
    }
  }

  return std::nullopt;
}

std::filesystem::path MacOSAPIs::get_application_support_dir() {
  const char* home = std::getenv("HOME");
  if (home) {
    return std::filesystem::path(home) / "Library" / "Application Support";
  }
  return std::filesystem::path();
}

std::filesystem::path MacOSAPIs::get_user_library_dir() {
  const char* home = std::getenv("HOME");
  if (home) {
    return std::filesystem::path(home) / "Library";
  }
  return std::filesystem::path();
}

#endif  // __APPLE__

#ifdef __linux__

// LinuxAPIs implementation

std::string LinuxAPIs::get_distribution_name() {
  std::ifstream os_release("/etc/os-release");
  if (os_release.is_open()) {
    std::string line;
    while (std::getline(os_release, line)) {
      if (line.find("NAME=") == 0) {
        std::string name = line.substr(5);
        // Remove quotes
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        return name;
      }
    }
  }
  return "Unknown Linux";
}

std::string LinuxAPIs::get_distribution_version() {
  std::ifstream os_release("/etc/os-release");
  if (os_release.is_open()) {
    std::string line;
    while (std::getline(os_release, line)) {
      if (line.find("VERSION_ID=") == 0) {
        std::string version = line.substr(11);
        version.erase(std::remove(version.begin(), version.end(), '"'), version.end());
        return version;
      }
    }
  }
  return "Unknown";
}

std::optional<std::string> LinuxAPIs::read_proc_file(const std::string& path) {
  std::ifstream file(path);
  if (file.is_open()) {
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
  }
  return std::nullopt;
}

std::string LinuxAPIs::get_cpu_info() {
  auto info = read_proc_file("/proc/cpuinfo");
  return info.value_or("Unknown");
}

std::string LinuxAPIs::get_memory_info() {
  auto info = read_proc_file("/proc/meminfo");
  return info.value_or("Unknown");
}

bool LinuxAPIs::is_running_in_container() {
  // Check for Docker
  if (std::filesystem::exists("/.dockerenv")) {
    return true;
  }

  // Check cgroup
  auto cgroup = read_proc_file("/proc/1/cgroup");
  if (cgroup &&
      (cgroup->find("docker") != std::string::npos || cgroup->find("lxc") != std::string::npos)) {
    return true;
  }

  return false;
}

std::optional<std::string> LinuxAPIs::get_cgroup_info() {
  return read_proc_file("/proc/self/cgroup");
}

std::optional<std::string> LinuxAPIs::get_systemd_version() {
  // Try to get systemd version
  FILE* pipe = popen("systemctl --version 2>/dev/null | head -n1", "r");
  if (!pipe) {
    return std::nullopt;
  }

  char buffer[128];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }
  pclose(pipe);

  if (!result.empty()) {
    return result;
  }

  return std::nullopt;
}

#endif  // __linux__

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

// BSDAPIs implementation

std::string BSDAPIs::get_bsd_variant() {
#ifdef __FreeBSD__
  return "FreeBSD";
#elif defined(__NetBSD__)
  return "NetBSD";
#elif defined(__OpenBSD__)
  return "OpenBSD";
#else
  return "BSD";
#endif
}

std::optional<std::string> BSDAPIs::get_sysctl_string(const std::string& name) {
  char buffer[256];
  size_t size = sizeof(buffer);

  if (sysctlbyname(name.c_str(), buffer, &size, nullptr, 0) == 0) {
    return std::string(buffer);
  }

  return std::nullopt;
}

std::optional<int> BSDAPIs::get_sysctl_int(const std::string& name) {
  int value;
  size_t size = sizeof(value);

  if (sysctlbyname(name.c_str(), &value, &size, nullptr, 0) == 0) {
    return value;
  }

  return std::nullopt;
}

std::string BSDAPIs::get_kernel_version() {
  auto version = get_sysctl_string("kern.version");
  return version.value_or("Unknown");
}

std::string BSDAPIs::get_hardware_platform() {
  auto platform = get_sysctl_string("hw.machine");
  return platform.value_or("Unknown");
}

#endif  // BSD

}  // namespace SolarSystem::Utils::Platform
