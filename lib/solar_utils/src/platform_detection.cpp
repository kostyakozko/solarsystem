/**
 * @file platform_detection.cpp
 * @brief Implementation of platform detection utilities
 */

#include "solar_utils/platform_detection.hpp"

#include <algorithm>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/utsname.h>
#else
#include <sys/utsname.h>
#endif

namespace SolarSystem::Utils::Platform {

// PlatformDetector implementation

PlatformInfo PlatformDetector::get_platform_info() {
  PlatformInfo info;

  info.os = detect_os();
  info.os_name = get_os_name();
  info.os_version = get_os_version();
  info.arch = detect_architecture();
  info.arch_name = get_architecture_name();
  info.compiler = detect_compiler();
  info.compiler_version = get_compiler_version();
  info.pointer_size = get_pointer_size();
  info.is_64bit = is_64bit();
  info.is_little_endian = is_little_endian();
  info.capabilities = detect_capabilities();

  return info;
}

OperatingSystem PlatformDetector::detect_os() {
#ifdef _WIN32
  return OperatingSystem::Windows;
#elif defined(__APPLE__)
  return OperatingSystem::MacOS;
#elif defined(__linux__)
  return OperatingSystem::Linux;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
  return OperatingSystem::BSD;
#elif defined(__unix__)
  return OperatingSystem::Unix;
#else
  return OperatingSystem::Unknown;
#endif
}

std::string PlatformDetector::get_os_name() {
  switch (detect_os()) {
    case OperatingSystem::Windows:
      return "Windows";
    case OperatingSystem::MacOS:
      return "macOS";
    case OperatingSystem::Linux:
      return "Linux";
    case OperatingSystem::BSD:
      return "BSD";
    case OperatingSystem::Unix:
      return "Unix";
    default:
      return "Unknown";
  }
}

std::string PlatformDetector::get_os_version() {
#ifdef _WIN32
  OSVERSIONINFOEXW osvi = {};
  osvi.dwOSVersionInfoSize = sizeof(osvi);

#pragma warning(push)
#pragma warning(disable : 4996)
  if (GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi))) {
    std::ostringstream oss;
    oss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "." << osvi.dwBuildNumber;
    return oss.str();
  }
#pragma warning(pop)

  return "Unknown";
#else
  struct utsname uts;
  if (uname(&uts) == 0) {
    return std::string(uts.release);
  }
  return "Unknown";
#endif
}

Architecture PlatformDetector::detect_architecture() {
#if defined(__x86_64__) || defined(_M_X64)
  return Architecture::x64;
#elif defined(__i386__) || defined(_M_IX86)
  return Architecture::x86;
#elif defined(__aarch64__) || defined(_M_ARM64)
  return Architecture::ARM64;
#elif defined(__arm__) || defined(_M_ARM)
  return Architecture::ARM;
#elif defined(__powerpc__) || defined(__ppc__)
  return Architecture::PowerPC;
#elif defined(__mips__)
  return Architecture::MIPS;
#else
  return Architecture::Unknown;
#endif
}

std::string PlatformDetector::get_architecture_name() {
  switch (detect_architecture()) {
    case Architecture::x86:
      return "x86";
    case Architecture::x64:
      return "x64";
    case Architecture::ARM:
      return "ARM";
    case Architecture::ARM64:
      return "ARM64";
    case Architecture::PowerPC:
      return "PowerPC";
    case Architecture::MIPS:
      return "MIPS";
    default:
      return "Unknown";
  }
}

Compiler PlatformDetector::detect_compiler() {
#if defined(_MSC_VER)
  return Compiler::MSVC;
#elif defined(__clang__)
  return Compiler::Clang;
#elif defined(__GNUC__)
  return Compiler::GCC;
#elif defined(__INTEL_COMPILER)
  return Compiler::Intel;
#else
  return Compiler::Unknown;
#endif
}

std::string PlatformDetector::get_compiler_version() {
#if defined(_MSC_VER)
  std::ostringstream oss;
  oss << _MSC_VER;
  return oss.str();
#elif defined(__clang__)
  return __clang_version__;
#elif defined(__GNUC__)
  std::ostringstream oss;
  oss << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
  return oss.str();
#elif defined(__INTEL_COMPILER)
  std::ostringstream oss;
  oss << __INTEL_COMPILER;
  return oss.str();
#else
  return "Unknown";
#endif
}

bool PlatformDetector::is_64bit() { return sizeof(void*) == 8; }

bool PlatformDetector::is_little_endian() {
  int num = 1;
  return *reinterpret_cast<char*>(&num) == 1;
}

int PlatformDetector::get_pointer_size() { return static_cast<int>(sizeof(void*)); }

PlatformCapabilities PlatformDetector::detect_capabilities() {
  PlatformCapabilities caps;

  caps.has_threads = FeatureDetector::has_threading();
  caps.has_filesystem = FeatureDetector::has_filesystem();
  caps.has_networking = FeatureDetector::has_networking();
  caps.has_simd_sse = FeatureDetector::has_sse();
  caps.has_simd_avx = FeatureDetector::has_avx();
  caps.has_simd_avx2 = FeatureDetector::has_avx2();
  caps.has_simd_neon = FeatureDetector::has_neon();
  caps.has_atomic_operations = FeatureDetector::has_atomics();
  caps.has_64bit_pointers = is_64bit();
  caps.has_hardware_aes = FeatureDetector::has_aes();
  caps.cache_line_size = get_cache_line_size();

  return caps;
}

bool PlatformDetector::has_cpu_feature(const std::string& feature) {
  std::string lower_feature = feature;
  std::transform(lower_feature.begin(), lower_feature.end(), lower_feature.begin(), ::tolower);

  if (lower_feature == "sse") return FeatureDetector::has_sse();
  if (lower_feature == "avx") return FeatureDetector::has_avx();
  if (lower_feature == "avx2") return FeatureDetector::has_avx2();
  if (lower_feature == "neon") return FeatureDetector::has_neon();
  if (lower_feature == "aes") return FeatureDetector::has_aes();

  return false;
}

std::string PlatformDetector::get_cpu_brand() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#ifdef _WIN32
  int cpuInfo[4];
  char brand[49] = {0};

  __cpuid(cpuInfo, 0x80000000);
  unsigned int nExIds = cpuInfo[0];

  if (nExIds >= 0x80000004) {
    __cpuid(reinterpret_cast<int*>(brand), 0x80000002);
    __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
    __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
    return std::string(brand);
  }
#endif
#endif
  return "Unknown CPU";
}

int PlatformDetector::get_cpu_core_count() {
  return static_cast<int>(std::thread::hardware_concurrency());
}

int PlatformDetector::get_cache_line_size() {
#ifdef __cpp_lib_hardware_interference_size
  return std::hardware_destructive_interference_size;
#else
  // Common cache line size for most modern CPUs
  return 64;
#endif
}

// FeatureDetector implementation

bool FeatureDetector::has_threading() {
#ifdef __cpp_lib_thread
  return true;
#else
  return false;
#endif
}

bool FeatureDetector::has_filesystem() {
#ifdef __cpp_lib_filesystem
  return true;
#else
  return false;
#endif
}

bool FeatureDetector::has_networking() {
  // Check if we can compile with networking support
  // This is a compile-time check
  return true;  // Assume networking is available
}

bool FeatureDetector::has_sse() {
#if defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
  return true;
#else
  return false;
#endif
}

bool FeatureDetector::has_avx() {
#if defined(__AVX__)
  return true;
#else
  return false;
#endif
}

bool FeatureDetector::has_avx2() {
#if defined(__AVX2__)
  return true;
#else
  return false;
#endif
}

bool FeatureDetector::has_neon() {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  return true;
#else
  return false;
#endif
}

bool FeatureDetector::has_aes() {
#if defined(__AES__)
  return true;
#else
  return false;
#endif
}

bool FeatureDetector::has_atomics() {
#ifdef __cpp_lib_atomic
  return true;
#else
  return false;
#endif
}

std::vector<std::string> FeatureDetector::get_available_features() {
  std::vector<std::string> features;

  if (has_threading()) features.push_back("Threading");
  if (has_filesystem()) features.push_back("Filesystem");
  if (has_networking()) features.push_back("Networking");
  if (has_sse()) features.push_back("SSE");
  if (has_avx()) features.push_back("AVX");
  if (has_avx2()) features.push_back("AVX2");
  if (has_neon()) features.push_back("NEON");
  if (has_aes()) features.push_back("AES");
  if (has_atomics()) features.push_back("Atomics");

  return features;
}

// VersionComparator implementation

bool VersionComparator::is_at_least(int major, int minor, int patch) {
  std::string current_version = PlatformDetector::get_os_version();
  auto parsed = parse_version(current_version);

  if (!parsed) {
    return false;
  }

  auto [cur_major, cur_minor, cur_patch] = *parsed;

  if (cur_major > major) return true;
  if (cur_major < major) return false;

  if (cur_minor > minor) return true;
  if (cur_minor < minor) return false;

  return cur_patch >= patch;
}

std::optional<std::tuple<int, int, int>> VersionComparator::parse_version(
    const std::string& version) {
  std::istringstream iss(version);
  int major = 0, minor = 0, patch = 0;
  char dot;

  if (!(iss >> major)) {
    return std::nullopt;
  }

  if (iss >> dot >> minor) {
    iss >> dot >> patch;
  }

  return std::make_tuple(major, minor, patch);
}

int VersionComparator::compare_versions(const std::string& v1, const std::string& v2) {
  auto parsed1 = parse_version(v1);
  auto parsed2 = parse_version(v2);

  if (!parsed1 || !parsed2) {
    return 0;  // Can't compare
  }

  auto [major1, minor1, patch1] = *parsed1;
  auto [major2, minor2, patch2] = *parsed2;

  if (major1 != major2) return major1 - major2;
  if (minor1 != minor2) return minor1 - minor2;
  return patch1 - patch2;
}

}  // namespace SolarSystem::Utils::Platform
