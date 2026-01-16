/**
 * @file platform_detection.hpp
 * @brief Comprehensive platform detection and capability reporting
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Utils::Platform {

/**
 * @brief Operating system types
 */
enum class OperatingSystem { Windows, MacOS, Linux, BSD, Unix, Unknown };

/**
 * @brief CPU architecture types
 */
enum class Architecture { x86, x64, ARM, ARM64, PowerPC, MIPS, Unknown };

/**
 * @brief Compiler types
 */
enum class Compiler { MSVC, GCC, Clang, Intel, Unknown };

/**
 * @brief Platform capabilities
 */
struct PlatformCapabilities {
  bool has_threads;
  bool has_filesystem;
  bool has_networking;
  bool has_simd_sse;
  bool has_simd_avx;
  bool has_simd_avx2;
  bool has_simd_neon;
  bool has_atomic_operations;
  bool has_64bit_pointers;
  bool has_hardware_aes;
  int cache_line_size;
};

/**
 * @brief Platform information
 */
struct PlatformInfo {
  OperatingSystem os;
  std::string os_name;
  std::string os_version;
  Architecture arch;
  std::string arch_name;
  Compiler compiler;
  std::string compiler_version;
  int pointer_size;
  bool is_64bit;
  bool is_little_endian;
  PlatformCapabilities capabilities;
};

/**
 * @brief Platform detection utilities
 */
class PlatformDetector {
 public:
  /**
   * @brief Get complete platform information
   */
  static PlatformInfo get_platform_info();

  /**
   * @brief Detect operating system
   */
  static OperatingSystem detect_os();

  /**
   * @brief Get OS name string
   */
  static std::string get_os_name();

  /**
   * @brief Get OS version
   */
  static std::string get_os_version();

  /**
   * @brief Detect CPU architecture
   */
  static Architecture detect_architecture();

  /**
   * @brief Get architecture name
   */
  static std::string get_architecture_name();

  /**
   * @brief Detect compiler
   */
  static Compiler detect_compiler();

  /**
   * @brief Get compiler version
   */
  static std::string get_compiler_version();

  /**
   * @brief Check if system is 64-bit
   */
  static bool is_64bit();

  /**
   * @brief Check endianness
   */
  static bool is_little_endian();

  /**
   * @brief Get pointer size in bytes
   */
  static int get_pointer_size();

  /**
   * @brief Detect platform capabilities
   */
  static PlatformCapabilities detect_capabilities();

  /**
   * @brief Check for specific CPU feature
   */
  static bool has_cpu_feature(const std::string& feature);

  /**
   * @brief Get CPU brand string
   */
  static std::string get_cpu_brand();

  /**
   * @brief Get number of CPU cores
   */
  static int get_cpu_core_count();

  /**
   * @brief Get cache line size
   */
  static int get_cache_line_size();
};

/**
 * @brief Feature detection utilities
 */
class FeatureDetector {
 public:
  /**
   * @brief Check if threading is available
   */
  static bool has_threading();

  /**
   * @brief Check if filesystem is available
   */
  static bool has_filesystem();

  /**
   * @brief Check if networking is available
   */
  static bool has_networking();

  /**
   * @brief Check for SSE support
   */
  static bool has_sse();

  /**
   * @brief Check for AVX support
   */
  static bool has_avx();

  /**
   * @brief Check for AVX2 support
   */
  static bool has_avx2();

  /**
   * @brief Check for NEON support (ARM)
   */
  static bool has_neon();

  /**
   * @brief Check for hardware AES
   */
  static bool has_aes();

  /**
   * @brief Check for atomic operations
   */
  static bool has_atomics();

  /**
   * @brief Get list of all available features
   */
  static std::vector<std::string> get_available_features();
};

/**
 * @brief OS version comparison utilities
 */
class VersionComparator {
 public:
  /**
   * @brief Check if OS version is at least the specified version
   */
  static bool is_at_least(int major, int minor = 0, int patch = 0);

  /**
   * @brief Parse version string
   */
  static std::optional<std::tuple<int, int, int>> parse_version(const std::string& version);

  /**
   * @brief Compare two version strings
   */
  static int compare_versions(const std::string& v1, const std::string& v2);
};

}  // namespace SolarSystem::Utils::Platform
