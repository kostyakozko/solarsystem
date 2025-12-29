/**
 * @file gtest_compat.hpp
 * @brief Google Test compatibility layer for Solar System Suite
 *
 * This header provides a compatibility layer that maps the custom test framework
 * macros to Google Test equivalents. This allows gradual migration of existing
 * tests to Google Test without requiring immediate rewrite of all test files.
 *
 * Usage:
 *   1. Replace #include "../utils/test_framework.h" with:
 *      #include "../utils/gtest_compat.hpp"
 *   2. Replace main() function with Google Test main or use gtest_main
 *   3. Replace TEST_SUITE/run_test pattern with TEST/TEST_F macros
 *
 * Migration Path:
 *   - Phase 1: Use this compatibility layer with existing test structure
 *   - Phase 2: Convert to native Google Test TEST/TEST_F macros
 *   - Phase 3: Remove compatibility layer usage
 */

#ifndef GTEST_COMPAT_HPP
#define GTEST_COMPAT_HPP

#include <gtest/gtest.h>
#include <sys/stat.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// Include test utilities that don't conflict with Google Test
#include "test_diagnostics.hpp"
#include "test_port_manager.hpp"

// ============================================================================
// Assertion Macro Mappings
// ============================================================================
// These macros map the custom framework assertions to Google Test equivalents.
// Google Test provides both EXPECT_* (non-fatal) and ASSERT_* (fatal) variants.
// We map to ASSERT_* to match the original behavior (test stops on failure).

// Note: Google Test already provides ASSERT_TRUE, ASSERT_FALSE, ASSERT_EQ, etc.
// The custom framework macros are compatible with Google Test's ASSERT_* macros.
// We only need to provide compatibility for macros with different signatures.

// ASSERT_NEAR compatibility - Google Test uses (val1, val2, abs_error)
// Our custom framework uses the same signature, so this is compatible.
// No mapping needed for ASSERT_NEAR.

// ASSERT_NOT_NULL and ASSERT_NULL - map to Google Test pointer assertions
#ifndef ASSERT_NOT_NULL
#define ASSERT_NOT_NULL(ptr) ASSERT_NE(nullptr, ptr)
#endif

#ifndef ASSERT_NULL
#define ASSERT_NULL(ptr) ASSERT_EQ(nullptr, ptr)
#endif

// ============================================================================
// EXPECT_* Macro Aliases (for migration convenience)
// ============================================================================
// These provide explicit mappings from custom ASSERT_* to Google Test EXPECT_*
// Use EXPECT_* when you want the test to continue after a failure

#ifndef EXPECT_NOT_NULL
#define EXPECT_NOT_NULL(ptr) EXPECT_NE(nullptr, ptr)
#endif

#ifndef EXPECT_NULL
#define EXPECT_NULL(ptr) EXPECT_EQ(nullptr, ptr)
#endif

// Mapping table for reference (all these work identically in both frameworks):
// Custom Framework    -> Google Test (fatal)  -> Google Test (non-fatal)
// ASSERT_TRUE(x)      -> ASSERT_TRUE(x)       -> EXPECT_TRUE(x)
// ASSERT_FALSE(x)     -> ASSERT_FALSE(x)      -> EXPECT_FALSE(x)
// ASSERT_EQ(a, b)     -> ASSERT_EQ(a, b)      -> EXPECT_EQ(a, b)
// ASSERT_NE(a, b)     -> ASSERT_NE(a, b)      -> EXPECT_NE(a, b)
// ASSERT_LT(a, b)     -> ASSERT_LT(a, b)      -> EXPECT_LT(a, b)
// ASSERT_LE(a, b)     -> ASSERT_LE(a, b)      -> EXPECT_LE(a, b)
// ASSERT_GT(a, b)     -> ASSERT_GT(a, b)      -> EXPECT_GT(a, b)
// ASSERT_GE(a, b)     -> ASSERT_GE(a, b)      -> EXPECT_GE(a, b)
// ASSERT_NEAR(a,b,t)  -> ASSERT_NEAR(a,b,t)   -> EXPECT_NEAR(a,b,t)
// ASSERT_NOT_NULL(p)  -> ASSERT_NE(nullptr,p) -> EXPECT_NE(nullptr,p)
// ASSERT_NULL(p)      -> ASSERT_EQ(nullptr,p) -> EXPECT_EQ(nullptr,p)

// ============================================================================
// Test Suite Compatibility Class
// ============================================================================
// This class provides a compatibility layer for the TestSuite pattern used
// in the custom framework. It wraps Google Test functionality.

class GTestCompatSuite {
 public:
  explicit GTestCompatSuite(const std::string& name) : suite_name_(name), test_count_(0) {
    std::cout << "\n=== Running Test Suite: " << suite_name_ << " ===" << std::endl;
  }

  ~GTestCompatSuite() { print_summary(); }

  // Run a test using the old-style lambda pattern
  // This is for compatibility - new tests should use TEST/TEST_F macros
  void run_test(const std::string& test_name, std::function<void()> test_func) {
    test_count_++;
    auto start = std::chrono::high_resolution_clock::now();

    try {
      test_func();
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      double ms = static_cast<double>(duration.count()) / 1000.0;

      std::cout << "✓ " << test_name << " (" << std::fixed << std::setprecision(2) << ms << " ms)"
                << std::endl;
      results_.push_back({test_name, true, "PASSED", ms});
      passed_count_++;
    } catch (const std::exception& e) {
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      double ms = static_cast<double>(duration.count()) / 1000.0;

      std::cout << "✗ " << test_name << " (" << std::fixed << std::setprecision(2) << ms << " ms)"
                << std::endl;
      std::cout << "  Error: " << e.what() << std::endl;
      results_.push_back({test_name, false, e.what(), ms});

      // Record failure in Google Test
      ADD_FAILURE() << "Test '" << test_name << "' failed: " << e.what();
    }
  }

  bool all_passed() const { return passed_count_ == test_count_; }
  int get_failed_count() const { return test_count_ - passed_count_; }

  // Port allocation for tests that need network ports
  TestUtils::ScopedPortAllocation allocate_port() {
    return TestUtils::ScopedPortAllocation(suite_name_);
  }

 private:
  struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    double duration_ms;
  };

  std::string suite_name_;
  std::vector<TestResult> results_;
  int test_count_ = 0;
  int passed_count_ = 0;

  void print_summary() {
    std::cout << "\n=== Test Suite Summary: " << suite_name_ << " ===" << std::endl;
    std::cout << "Total tests: " << test_count_ << std::endl;
    std::cout << "Passed: " << passed_count_ << std::endl;
    std::cout << "Failed: " << (test_count_ - passed_count_) << std::endl;

    if (test_count_ > 0) {
      double success_rate = static_cast<double>(passed_count_) / test_count_ * 100.0;
      std::cout << "Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%"
                << std::endl;
    }

    double total_duration = 0.0;
    for (const auto& result : results_) {
      total_duration += result.duration_ms;
    }
    std::cout << "Total duration: " << std::fixed << std::setprecision(2) << total_duration << " ms"
              << std::endl;

    if (test_count_ - passed_count_ > 0) {
      std::cout << "\nFailed tests:" << std::endl;
      for (const auto& result : results_) {
        if (!result.passed) {
          std::cout << "  - " << result.name << ": " << result.message << std::endl;
        }
      }
    }
    std::cout << std::string(50, '=') << std::endl;
  }
};

// ============================================================================
// Legacy Macro Compatibility
// ============================================================================
// These macros provide backward compatibility with the old test framework.
// New tests should use native Google Test macros instead.

// TEST_SUITE macro - creates a GTestCompatSuite for legacy tests
#define TEST_SUITE_COMPAT(name) GTestCompatSuite gtest_compat_suite(name)

// For tests that use the old pattern with current_suite
// This is a transitional helper - prefer TEST/TEST_F for new tests
#define LEGACY_TEST_SUITE(name)    \
  GTestCompatSuite* current_suite; \
  GTestCompatSuite suite(name);    \
  current_suite = &suite

// ============================================================================
// TEST_CASE Macro Mapping
// ============================================================================
// Maps the custom TEST_CASE macro to Google Test TEST macro
// The old pattern: TEST_CASE("name") { ... });
// The new pattern: TEST(SuiteName, TestName) { ... }

// For use with GTestCompatSuite - runs test through the compatibility layer
#define TEST_CASE_COMPAT(suite_ptr, name) (suite_ptr)->run_test(name, []()

// Direct mapping to Google Test TEST macro for new code
// Usage: GTEST_CASE(SuiteName, TestName) { ... }
#define GTEST_CASE(suite_name, test_name) TEST(suite_name, test_name)

// Macro to help convert old TEST_CASE pattern to Google Test
// This creates a test that can be discovered by gtest_discover_tests
// Usage: MIGRATED_TEST_CASE(OldSuiteName, "Old Test Name") { ... }
// Note: Test names with spaces need to be converted to CamelCase or underscores
#define MIGRATED_TEST_CASE(suite_name, test_name) TEST(suite_name, test_name)

// ============================================================================
// Test Utilities Namespace
// ============================================================================
// These utilities are compatible with both frameworks

namespace TestUtils {

// Floating point comparison with relative tolerance
inline bool nearly_equal(double a, double b, double rel_tolerance = 1e-9) {
  if (std::abs(a - b) < 1e-15) return true;  // Handle exact zeros

  double abs_a = std::abs(a);
  double abs_b = std::abs(b);
  double largest = (abs_a > abs_b) ? abs_a : abs_b;

  return std::abs(a - b) <= rel_tolerance * largest;
}

// Vector comparison utilities
inline bool vectors_equal(const std::vector<double>& a, const std::vector<double>& b,
                          double tolerance = 1e-9) {
  if (a.size() != b.size()) return false;

  for (size_t i = 0; i < a.size(); ++i) {
    if (std::abs(a[i] - b[i]) > tolerance) {
      return false;
    }
  }
  return true;
}

// String utilities for test output
inline std::string format_double(double value, int precision = 6) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << value;
  return oss.str();
}

inline std::string format_vector(const std::vector<double>& vec) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << format_double(vec[i], 3);
  }
  oss << "]";
  return oss.str();
}

// Test data generation
inline std::vector<double> generate_test_positions(int count, double range = 1e12) {
  std::vector<double> positions;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(-range, range);

  for (int i = 0; i < count; ++i) {
    positions.push_back(dis(gen));
  }
  return positions;
}

inline std::vector<double> generate_test_velocities(int count, double range = 1e5) {
  std::vector<double> velocities;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(-range, range);

  for (int i = 0; i < count; ++i) {
    velocities.push_back(dis(gen));
  }
  return velocities;
}

// File system utilities for tests
inline bool file_exists(const std::string& filename) {
  std::ifstream file(filename);
  return file.good();
}

inline bool create_test_directory(const std::string& dirname) {
  return mkdir(dirname.c_str(), 0755) == 0 || errno == EEXIST;
}

inline void cleanup_test_files(const std::vector<std::string>& filenames) {
  for (const auto& filename : filenames) {
    std::remove(filename.c_str());
  }
}

}  // namespace TestUtils

// ============================================================================
// Google Test Custom Matchers (Optional)
// ============================================================================
// These provide more expressive assertions for common patterns

// Custom matcher for near-equality with relative tolerance
MATCHER_P2(NearWithRelTolerance, expected, rel_tolerance,
           std::string(negation ? "isn't" : "is") + " approximately " +
               ::testing::PrintToString(expected) + " (rel_tol=" +
               ::testing::PrintToString(rel_tolerance) + ")") {
  return TestUtils::nearly_equal(arg, expected, rel_tolerance);
}

// Custom matcher for vector equality
MATCHER_P2(VectorNear, expected, tolerance,
           std::string(negation ? "isn't" : "is") + " approximately equal to expected vector") {
  return TestUtils::vectors_equal(arg, expected, tolerance);
}

// ============================================================================
// Migration Helper Macros
// ============================================================================
// These help identify code that needs migration

// Mark tests that still use legacy patterns
#define LEGACY_TEST_CASE(name) \
  std::cerr << "[MIGRATION] Legacy test pattern used: " << name << std::endl;

// ============================================================================
// Documentation: Migration Guide
// ============================================================================
/*
 * MIGRATION FROM CUSTOM FRAMEWORK TO GOOGLE TEST
 * ==============================================
 *
 * Step 1: Change includes
 * -----------------------
 * OLD: #include "../utils/test_framework.h"
 * NEW: #include <gtest/gtest.h>
 *      // Or for compatibility: #include "../utils/gtest_compat.hpp"
 *
 * Step 2: Convert test structure
 * ------------------------------
 * OLD:
 *   int main() {
 *     TestSuite suite("My Tests");
 *     suite.run_test("Test Name", []() {
 *       ASSERT_EQ(expected, actual);
 *     });
 *     return suite.all_passed() ? 0 : 1;
 *   }
 *
 * NEW:
 *   TEST(MyTests, TestName) {
 *     EXPECT_EQ(expected, actual);
 *   }
 *   // No main() needed - use gtest_main
 *
 * Step 3: Convert assertions
 * --------------------------
 * Most assertions have the same name but different behavior:
 * - ASSERT_* = fatal (stops test on failure)
 * - EXPECT_* = non-fatal (continues test on failure)
 *
 * Mapping:
 *   ASSERT_TRUE(x)      -> EXPECT_TRUE(x) or ASSERT_TRUE(x)
 *   ASSERT_FALSE(x)     -> EXPECT_FALSE(x) or ASSERT_FALSE(x)
 *   ASSERT_EQ(a, b)     -> EXPECT_EQ(a, b) or ASSERT_EQ(a, b)
 *   ASSERT_NE(a, b)     -> EXPECT_NE(a, b) or ASSERT_NE(a, b)
 *   ASSERT_LT(a, b)     -> EXPECT_LT(a, b) or ASSERT_LT(a, b)
 *   ASSERT_LE(a, b)     -> EXPECT_LE(a, b) or ASSERT_LE(a, b)
 *   ASSERT_GT(a, b)     -> EXPECT_GT(a, b) or ASSERT_GT(a, b)
 *   ASSERT_GE(a, b)     -> EXPECT_GE(a, b) or ASSERT_GE(a, b)
 *   ASSERT_NEAR(a,b,t)  -> EXPECT_NEAR(a, b, t) or ASSERT_NEAR(a, b, t)
 *   ASSERT_NOT_NULL(p)  -> EXPECT_NE(nullptr, p) or ASSERT_NE(nullptr, p)
 *   ASSERT_NULL(p)      -> EXPECT_EQ(nullptr, p) or ASSERT_EQ(nullptr, p)
 *
 * Step 4: Update CMakeLists.txt
 * -----------------------------
 * OLD:
 *   add_executable(test_foo test_foo.cpp)
 *   target_link_libraries(test_foo test_utils solar_core)
 *   add_test(NAME Test_Foo COMMAND test_foo)
 *
 * NEW:
 *   add_executable(test_foo test_foo.cpp)
 *   target_link_libraries(test_foo GTest::gtest_main solar_core)
 *   gtest_discover_tests(test_foo)
 *
 * Step 5: Use fixtures for shared setup
 * -------------------------------------
 * OLD:
 *   // Setup code repeated in each test
 *
 * NEW:
 *   class MyTestFixture : public ::testing::Test {
 *   protected:
 *     void SetUp() override { // setup code }
 *     void TearDown() override { // cleanup code }
 *     // shared data members
 *   };
 *   TEST_F(MyTestFixture, TestName) { ... }
 */

// ============================================================================
// Include Guards for Conflicting Macros
// ============================================================================
// Prevent inclusion of the old test_framework.h when using this header

#define TEST_FRAMEWORK_H  // Prevent old framework from being included

#endif  // GTEST_COMPAT_HPP
