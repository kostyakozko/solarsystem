/**
 * @file test_framework.h
 * @brief Lightweight testing framework for Solar System Suite
 *
 * Provides comprehensive testing utilities including:
 * - Unit test macros and assertions
 * - Test suite organization
 * - Performance measurement
 * - Test data validation
 * - Mock data generation
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Test result tracking
struct TestResult {
  std::string name;
  bool passed;
  std::string message;
  double duration_ms;
};

class TestSuite {
 private:
  std::string suite_name;
  std::vector<TestResult> results;
  int total_tests;
  int passed_tests;

 public:
  TestSuite(const std::string& name);
  ~TestSuite();

  void run_test(const std::string& test_name, std::function<void()> test_func);
  void print_summary();
  bool all_passed() const;
  int get_failed_count() const;
};

// Global test suite instance
extern TestSuite* current_suite;

// Test macros
#define TEST_SUITE(name) \
  TestSuite suite(name); \
  current_suite = &suite;

#define TEST_CASE(name) \
    current_suite->run_test(name, []()

#define ASSERT_TRUE(condition)                                                            \
  do {                                                                                    \
    if (!(condition)) {                                                                   \
      std::ostringstream oss;                                                             \
      oss << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__; \
      throw std::runtime_error(oss.str());                                                \
    }                                                                                     \
  } while (0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual)                                                           \
  do {                                                                                        \
    if ((expected) != (actual)) {                                                             \
      std::ostringstream oss;                                                                 \
      oss << "Assertion failed: expected " << (expected) << " but got " << (actual) << " at " \
          << __FILE__ << ":" << __LINE__;                                                     \
      throw std::runtime_error(oss.str());                                                    \
    }                                                                                         \
  } while (0)

#define ASSERT_NE(expected, actual)                                                      \
  do {                                                                                   \
    if ((expected) == (actual)) {                                                        \
      std::ostringstream oss;                                                            \
      oss << "Assertion failed: expected " << (expected) << " != " << (actual) << " at " \
          << __FILE__ << ":" << __LINE__;                                                \
      throw std::runtime_error(oss.str());                                               \
    }                                                                                    \
  } while (0)

#define ASSERT_NEAR(expected, actual, tolerance)                                                \
  do {                                                                                          \
    double diff = std::abs((expected) - (actual));                                              \
    if (diff > (tolerance)) {                                                                   \
      std::ostringstream oss;                                                                   \
      oss << "Assertion failed: expected " << (expected) << " ± " << (tolerance) << " but got " \
          << (actual) << " (diff: " << diff << ")"                                              \
          << " at " << __FILE__ << ":" << __LINE__;                                             \
      throw std::runtime_error(oss.str());                                                      \
    }                                                                                           \
  } while (0)

#define ASSERT_GT(value, threshold)                                                     \
  do {                                                                                  \
    if (!((value) > (threshold))) {                                                     \
      std::ostringstream oss;                                                           \
      oss << "Assertion failed: expected " << (value) << " > " << (threshold) << " at " \
          << __FILE__ << ":" << __LINE__;                                               \
      throw std::runtime_error(oss.str());                                              \
    }                                                                                   \
  } while (0)

#define ASSERT_LT(value, threshold)                                                     \
  do {                                                                                  \
    if (!((value) < (threshold))) {                                                     \
      std::ostringstream oss;                                                           \
      oss << "Assertion failed: expected " << (value) << " < " << (threshold) << " at " \
          << __FILE__ << ":" << __LINE__;                                               \
      throw std::runtime_error(oss.str());                                              \
    }                                                                                   \
  } while (0)

#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != nullptr)

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == nullptr)

// Performance testing macros
#define BENCHMARK(name, iterations) \
    current_suite->run_test(name, [&]() { \
        auto start = std::chrono::high_resolution_clock::now(); \
        for (int i = 0; i < iterations; ++i) {
#define END_BENCHMARK()                                                               \
  }                                                                                   \
  auto end = std::chrono::high_resolution_clock::now();                               \
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start); \
  std::cout << "  Benchmark completed in " << duration.count() << " μs" << std::endl; \
  });

// Test data validation utilities
namespace TestUtils {
// Floating point comparison with relative tolerance
bool nearly_equal(double a, double b, double rel_tolerance = 1e-9);

// Vector comparison utilities
bool vectors_equal(const std::vector<double>& a, const std::vector<double>& b,
                   double tolerance = 1e-9);

// String utilities for test output
std::string format_double(double value, int precision = 6);
std::string format_vector(const std::vector<double>& vec);

// Test data generation
std::vector<double> generate_test_positions(int count, double range = 1e12);
std::vector<double> generate_test_velocities(int count, double range = 1e5);

// File system utilities for tests
bool file_exists(const std::string& filename);
bool create_test_directory(const std::string& dirname);
void cleanup_test_files(const std::vector<std::string>& filenames);
}

#endif  // TEST_FRAMEWORK_H
