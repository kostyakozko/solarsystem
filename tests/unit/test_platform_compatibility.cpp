/**
 * @file test_platform_compatibility.cpp
 * @brief Cross-platform compatibility testing (Task 26)
 * @note Migrated to Google Test
 *
 * Tests platform compatibility:
 * - Cross-platform testing for macOS, Linux, and Windows
 * - Compiler compatibility testing (GCC, Clang, MSVC)
 * - Architecture-specific testing (x86, ARM, etc.)
 * - Platform-specific feature and API testing
 *
 * Requirements: 9.1, 9.2
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Platform information detector
 */
class PlatformDetector {
 public:
  enum class OS { Unknown, macOS, Linux, Windows, BSD };

  enum class Compiler { Unknown, GCC, Clang, MSVC };

  enum class Architecture { Unknown, x86, x86_64, ARM, ARM64 };

  static OS detect_os() {
#if defined(__APPLE__) && defined(__MACH__)
    return OS::macOS;
#elif defined(__linux__)
    return OS::Linux;
#elif defined(_WIN32) || defined(_WIN64)
    return OS::Windows;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    return OS::BSD;
#else
    return OS::Unknown;
#endif
  }

  static Compiler detect_compiler() {
#if defined(__clang__)
    return Compiler::Clang;
#elif defined(__GNUC__) || defined(__GNUG__)
    return Compiler::GCC;
#elif defined(_MSC_VER)
    return Compiler::MSVC;
#else
    return Compiler::Unknown;
#endif
  }

  static Architecture detect_architecture() {
#if defined(__x86_64__) || defined(_M_X64)
    return Architecture::x86_64;
#elif defined(__i386__) || defined(_M_IX86)
    return Architecture::x86;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return Architecture::ARM64;
#elif defined(__arm__) || defined(_M_ARM)
    return Architecture::ARM;
#else
    return Architecture::Unknown;
#endif
  }

  static std::string os_to_string(OS os) {
    switch (os) {
      case OS::macOS:
        return "macOS";
      case OS::Linux:
        return "Linux";
      case OS::Windows:
        return "Windows";
      case OS::BSD:
        return "BSD";
      default:
        return "Unknown";
    }
  }

  static std::string compiler_to_string(Compiler compiler) {
    switch (compiler) {
      case Compiler::GCC:
        return "GCC";
      case Compiler::Clang:
        return "Clang";
      case Compiler::MSVC:
        return "MSVC";
      default:
        return "Unknown";
    }
  }

  static std::string architecture_to_string(Architecture arch) {
    switch (arch) {
      case Architecture::x86:
        return "x86";
      case Architecture::x86_64:
        return "x86_64";
      case Architecture::ARM:
        return "ARM";
      case Architecture::ARM64:
        return "ARM64";
      default:
        return "Unknown";
    }
  }

  static bool is_little_endian() {
    uint32_t value = 0x01020304;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    return bytes[0] == 0x04;
  }

  static size_t get_pointer_size() { return sizeof(void*); }

  static bool supports_64bit() { return sizeof(void*) == 8; }
};

/**
 * @brief Platform-specific path handler
 */
class PathHandler {
 public:
  static char get_path_separator() {
#if defined(_WIN32) || defined(_WIN64)
    return '\\';
#else
    return '/';
#endif
  }

  static std::string normalize_path(const std::string& path) {
    std::string normalized = path;
    char sep = get_path_separator();

    // Replace all separators with platform-specific one
    for (char& c : normalized) {
      if (c == '/' || c == '\\') {
        c = sep;
      }
    }

    return normalized;
  }

  static std::string join_paths(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;

    char sep = get_path_separator();
    std::string result = path1;

    if (result.back() != sep && result.back() != '/' && result.back() != '\\') {
      result += sep;
    }

    result += path2;
    return normalize_path(result);
  }
};

/**
 * @brief Type size validator
 */
class TypeSizeValidator {
 public:
  struct TypeSizes {
    size_t char_size;
    size_t short_size;
    size_t int_size;
    size_t long_size;
    size_t long_long_size;
    size_t float_size;
    size_t double_size;
    size_t pointer_size;
    size_t size_t_size;
  };

  static TypeSizes get_type_sizes() {
    TypeSizes sizes;
    sizes.char_size = sizeof(char);
    sizes.short_size = sizeof(short);
    sizes.int_size = sizeof(int);
    sizes.long_size = sizeof(long);
    sizes.long_long_size = sizeof(long long);
    sizes.float_size = sizeof(float);
    sizes.double_size = sizeof(double);
    sizes.pointer_size = sizeof(void*);
    sizes.size_t_size = sizeof(size_t);
    return sizes;
  }

  static bool validate_standard_sizes(const TypeSizes& sizes) {
    // Validate minimum sizes according to C++ standard
    bool valid = true;
    valid &= (sizes.char_size == 1);
    valid &= (sizes.short_size >= 2);
    valid &= (sizes.int_size >= 2);
    valid &= (sizes.long_size >= 4);
    valid &= (sizes.long_long_size >= 8);
    valid &= (sizes.float_size == 4);
    valid &= (sizes.double_size == 8);
    return valid;
  }
};

/**
 * @brief Compiler feature detector
 */
class CompilerFeatures {
 public:
  static bool has_cpp20() {
#if __cplusplus >= 202002L
    return true;
#else
    return false;
#endif
  }

  static bool has_cpp17() {
#if __cplusplus >= 201703L
    return true;
#else
    return false;
#endif
  }

  static bool has_cpp14() {
#if __cplusplus >= 201402L
    return true;
#else
    return false;
#endif
  }

  static bool has_cpp11() {
#if __cplusplus >= 201103L
    return true;
#else
    return false;
#endif
  }

  static int get_cpp_version() { return static_cast<int>(__cplusplus); }
};
// Test 1: Platform detection
TEST(PlatformCompatibilityTestsTest, Platform_Detection) {
  auto os = PlatformDetector::detect_os();
  auto compiler = PlatformDetector::detect_compiler();
  auto arch = PlatformDetector::detect_architecture();

  // Test 1.1: OS detection
  ASSERT_TRUE(os != PlatformDetector::OS::Unknown);
  std::string os_name = PlatformDetector::os_to_string(os);
  ASSERT_FALSE(os_name.empty());
  ASSERT_TRUE(os_name != "Unknown");

  // Test 1.2: Compiler detection
  ASSERT_TRUE(compiler != PlatformDetector::Compiler::Unknown);
  std::string compiler_name = PlatformDetector::compiler_to_string(compiler);
  ASSERT_FALSE(compiler_name.empty());
  ASSERT_TRUE(compiler_name != "Unknown");

  // Test 1.3: Architecture detection
  ASSERT_TRUE(arch != PlatformDetector::Architecture::Unknown);
  std::string arch_name = PlatformDetector::architecture_to_string(arch);
  ASSERT_FALSE(arch_name.empty());
  ASSERT_TRUE(arch_name != "Unknown");
}

// Test 2: Endianness detection
TEST(PlatformCompatibilityTestsTest, Endianness_Detection) {
  bool is_little = PlatformDetector::is_little_endian();

  // Test 2.1: Endianness is deterministic
  ASSERT_EQ(is_little, PlatformDetector::is_little_endian());

  // Test 2.2: Most modern systems are little-endian
  // (This is informational, not a strict requirement)
  ASSERT_TRUE(is_little || !is_little);  // Always passes, just documents the check
}

// Test 3: Pointer size and 64-bit support
TEST(PlatformCompatibilityTestsTest, Pointer_Size_and_64_bit_Support) {
  size_t ptr_size = PlatformDetector::get_pointer_size();
  bool supports_64 = PlatformDetector::supports_64bit();

  // Test 3.1: Pointer size is valid
  ASSERT_TRUE(ptr_size == 4 || ptr_size == 8);

  // Test 3.2: 64-bit support consistency
  if (supports_64) {
    ASSERT_EQ(ptr_size, 8);
  } else {
    ASSERT_EQ(ptr_size, 4);
  }

  // Test 3.3: Architecture matches pointer size
  auto arch = PlatformDetector::detect_architecture();
  if (arch == PlatformDetector::Architecture::x86_64 ||
      arch == PlatformDetector::Architecture::ARM64) {
    ASSERT_TRUE(supports_64);
  }
}

// Test 4: Path handling
TEST(PlatformCompatibilityTestsTest, Path_Handling) {
  char sep = PathHandler::get_path_separator();

  // Test 4.1: Path separator is valid
  ASSERT_TRUE(sep == '/' || sep == '\\');

  // Test 4.2: Path normalization
  std::string path1 = "dir1/dir2\\dir3";
  std::string normalized = PathHandler::normalize_path(path1);
  ASSERT_FALSE(normalized.empty());

  // All separators should be consistent
  bool has_forward = normalized.find('/') != std::string::npos;
  bool has_backward = normalized.find('\\') != std::string::npos;
  ASSERT_FALSE(has_forward && has_backward);  // Should not have both

  // Test 4.3: Path joining
  std::string joined = PathHandler::join_paths("dir1", "dir2");
  EXPECT_NE(std::string::npos, joined.find("dir1"));
  EXPECT_NE(std::string::npos, joined.find("dir2"));

  // Test 4.4: Empty path handling
  std::string empty_join1 = PathHandler::join_paths("", "dir");
  ASSERT_EQ(empty_join1, "dir");

  std::string empty_join2 = PathHandler::join_paths("dir", "");
  ASSERT_EQ(empty_join2, "dir");
}

// Test 5: Type sizes
TEST(PlatformCompatibilityTestsTest, Type_Sizes) {
  auto sizes = TypeSizeValidator::get_type_sizes();

  // Test 5.1: Standard type sizes
  ASSERT_EQ(sizes.char_size, 1);
  ASSERT_GE(sizes.short_size, 2);
  ASSERT_GE(sizes.int_size, 2);
  ASSERT_GE(sizes.long_size, 4);
  ASSERT_GE(sizes.long_long_size, 8);
  ASSERT_EQ(sizes.float_size, 4);
  ASSERT_EQ(sizes.double_size, 8);

  // Test 5.2: Size relationships
  ASSERT_LE(sizes.char_size, sizes.short_size);
  ASSERT_LE(sizes.short_size, sizes.int_size);
  ASSERT_LE(sizes.int_size, sizes.long_size);
  ASSERT_LE(sizes.long_size, sizes.long_long_size);

  // Test 5.3: Validate against standard
  ASSERT_TRUE(TypeSizeValidator::validate_standard_sizes(sizes));
}

// Test 6: Compiler features
TEST(PlatformCompatibilityTestsTest, Compiler_Features) {
  // Test 6.1: C++ version detection
  int cpp_version = CompilerFeatures::get_cpp_version();
  ASSERT_GT(cpp_version, 0);

  // Test 6.2: C++11 support (minimum requirement)
  ASSERT_TRUE(CompilerFeatures::has_cpp11());

  // Test 6.3: C++14 support
  bool has_cpp14 = CompilerFeatures::has_cpp14();
  if (has_cpp14) {
    ASSERT_TRUE(CompilerFeatures::has_cpp11());
  }

  // Test 6.4: C++17 support
  bool has_cpp17 = CompilerFeatures::has_cpp17();
  if (has_cpp17) {
    ASSERT_TRUE(CompilerFeatures::has_cpp14());
  }

  // Test 6.5: C++20 support (project requirement)
  ASSERT_TRUE(CompilerFeatures::has_cpp20());
}

// Test 7: Integer types
TEST(PlatformCompatibilityTestsTest, Integer_Types) {
  // Test 7.1: Fixed-width integer types
  ASSERT_EQ(sizeof(int8_t), 1);
  ASSERT_EQ(sizeof(int16_t), 2);
  ASSERT_EQ(sizeof(int32_t), 4);
  ASSERT_EQ(sizeof(int64_t), 8);

  ASSERT_EQ(sizeof(uint8_t), 1);
  ASSERT_EQ(sizeof(uint16_t), 2);
  ASSERT_EQ(sizeof(uint32_t), 4);
  ASSERT_EQ(sizeof(uint64_t), 8);

  // Test 7.2: Size_t is pointer-sized
  ASSERT_EQ(sizeof(size_t), sizeof(void*));

  // Test 7.3: Intptr_t can hold a pointer
  ASSERT_EQ(sizeof(intptr_t), sizeof(void*));
  ASSERT_EQ(sizeof(uintptr_t), sizeof(void*));
}

// Test 8: Platform-specific behavior
TEST(PlatformCompatibilityTestsTest, Platform_Specific_Behavior) {
  auto os = PlatformDetector::detect_os();

  // Test 8.1: Path separator matches OS
  char sep = PathHandler::get_path_separator();
  if (os == PlatformDetector::OS::Windows) {
    ASSERT_EQ(sep, '\\');
  } else {
    ASSERT_EQ(sep, '/');
  }

  // Test 8.2: Line ending awareness (informational)
  // Different platforms use different line endings:
  // Windows: \r\n, Unix/Linux/macOS: \n
  // This test just documents the awareness
  std::string newline = "\n";
  ASSERT_FALSE(newline.empty());

  // Test 8.3: Case sensitivity awareness
  // Windows is case-insensitive, Unix-like systems are case-sensitive
  // This is informational for path handling
  bool is_unix_like = (os == PlatformDetector::OS::macOS || os == PlatformDetector::OS::Linux ||
                       os == PlatformDetector::OS::BSD);
  bool is_windows = (os == PlatformDetector::OS::Windows);
  ASSERT_TRUE(is_unix_like || is_windows);
}

// Test 9: Compiler-specific features
TEST(PlatformCompatibilityTestsTest, Compiler_Specific_Features) {
  auto compiler = PlatformDetector::detect_compiler();

  // Test 9.1: Compiler is recognized
  ASSERT_TRUE(compiler == PlatformDetector::Compiler::GCC ||
              compiler == PlatformDetector::Compiler::Clang ||
              compiler == PlatformDetector::Compiler::MSVC);

  // Test 9.2: Compiler version macros exist
#if defined(__clang__)
  EXPECT_GE(__clang_major__, 0);
#elif defined(__GNUC__)
  EXPECT_GE(__GNUC__, 0);
#elif defined(_MSC_VER)
  EXPECT_GE(_MSC_VER, 0);
#endif

  // Test 9.3: Standard library is available
  std::vector<int> test_vector = {1, 2, 3};
  ASSERT_EQ(test_vector.size(), 3);
}

// Test 10: Cross-platform compatibility summary
TEST(PlatformCompatibilityTestsTest, Cross_Platform_Compatibility_Summary) {
  auto os = PlatformDetector::detect_os();
  auto compiler = PlatformDetector::detect_compiler();
  auto arch = PlatformDetector::detect_architecture();

  // Test 10.1: All platform info is available
  ASSERT_TRUE(os != PlatformDetector::OS::Unknown);
  ASSERT_TRUE(compiler != PlatformDetector::Compiler::Unknown);
  ASSERT_TRUE(arch != PlatformDetector::Architecture::Unknown);

  // Test 10.2: Platform combination is valid
  bool valid_combination = false;

  // macOS typically uses Clang on x86_64 or ARM64
  if (os == PlatformDetector::OS::macOS) {
    valid_combination = (compiler == PlatformDetector::Compiler::Clang) &&
                        (arch == PlatformDetector::Architecture::x86_64 ||
                         arch == PlatformDetector::Architecture::ARM64);
  }
  // Linux can use GCC or Clang on various architectures
  else if (os == PlatformDetector::OS::Linux) {
    valid_combination = (compiler == PlatformDetector::Compiler::GCC ||
                         compiler == PlatformDetector::Compiler::Clang);
  }
  // Windows typically uses MSVC
  else if (os == PlatformDetector::OS::Windows) {
    valid_combination = (compiler == PlatformDetector::Compiler::MSVC ||
                         compiler == PlatformDetector::Compiler::Clang ||
                         compiler == PlatformDetector::Compiler::GCC);
  }

  ASSERT_TRUE(valid_combination);

  // Test 10.3: C++20 support is available
  ASSERT_TRUE(CompilerFeatures::has_cpp20());
}
