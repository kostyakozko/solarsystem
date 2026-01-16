#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_runner.hpp"

using namespace SolarSystem::Testing;

// Realistic test cases that should all pass
class StringUtilsTest : public TestCase {
 public:
  StringUtilsTest()
      : TestCase({"StringUtilsTest",
                  "Test string utility functions",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    std::string test_str = "Hello, World!";
    assert_true(test_str.length() == 13, "String length should be 13");
    assert_true(test_str.find("World") != std::string::npos, "Should contain 'World'");
    assert_false(test_str.empty(), "String should not be empty");
  }
};

class MathOperationsTest : public TestCase {
 public:
  MathOperationsTest()
      : TestCase({"MathOperationsTest",
                  "Test basic math operations",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    int a = 10, b = 5;
    assert_true(a + b == 15, "Addition should work correctly");
    assert_true(a - b == 5, "Subtraction should work correctly");
    assert_true(a * b == 50, "Multiplication should work correctly");
    assert_true(a / b == 2, "Division should work correctly");
  }
};

class VectorOperationsTest : public TestCase {
 public:
  VectorOperationsTest()
      : TestCase({"VectorOperationsTest",
                  "Test vector operations",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    assert_true(vec.size() == 5, "Vector should have 5 elements");
    assert_true(vec[0] == 1, "First element should be 1");
    assert_true(vec.back() == 5, "Last element should be 5");

    vec.push_back(6);
    assert_true(vec.size() == 6, "Vector should have 6 elements after push_back");
  }
};

class PerformanceTest : public TestCase {
 public:
  PerformanceTest()
      : TestCase({"PerformanceTest",
                  "Test performance requirements",
                  {"performance"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    // Test that a simple operation completes quickly
    assert_execution_time_less_than(
        []() {
          std::vector<int> vec;
          for (int i = 0; i < 1000; ++i) {
            vec.push_back(i);
          }
        },
        std::chrono::milliseconds(10));

    assert_true(true, "Performance test completed");
  }
};

class EdgeCaseTest : public TestCase {
 public:
  EdgeCaseTest()
      : TestCase({"EdgeCaseTest",
                  "Test edge cases and boundary conditions",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    // Test empty containers
    std::vector<int> empty_vec;
    assert_true(empty_vec.empty(), "Empty vector should be empty");
    assert_true(empty_vec.size() == 0, "Empty vector size should be 0");

    // Test string edge cases
    std::string empty_str = "";
    assert_true(empty_str.empty(), "Empty string should be empty");
    assert_true(empty_str.length() == 0, "Empty string length should be 0");

    // Test boundary values
    int max_int = std::numeric_limits<int>::max();
    int min_int = std::numeric_limits<int>::min();
    assert_true(max_int > min_int, "Max int should be greater than min int");
  }
};

class ExceptionHandlingTest : public TestCase {
 public:
  ExceptionHandlingTest()
      : TestCase({"ExceptionHandlingTest",
                  "Test proper exception handling",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    // Test that expected exceptions are thrown
    assert_throws(
        []() {
          std::vector<int> vec;
          // cppcheck-suppress containerOutOfBounds
          vec.at(10);  // Should throw std::out_of_range
        },
        "Vector at() should throw for invalid index");

    // Test that no exception is thrown for valid operations
    assert_no_throw(
        []() {
          std::vector<int> vec = {1, 2, 3};
          int value = vec.at(1);  // Should not throw
          (void)value;            // Suppress unused variable warning
        },
        "Vector at() should not throw for valid index");
  }
};

int main() {
  std::cout << "=== Realistic Test Suite Demo ===" << std::endl;

  // Create test runner with configuration
  TestRunner::Configuration config;
  config.parallel_execution = false;  // Use sequential for clearer output
  config.verbose = true;
  config.quiet = false;
  config.timeout = std::chrono::seconds(10);

  TestRunner runner(config);

  // Register realistic test cases that should all pass
  runner.register_test(std::make_unique<StringUtilsTest>());
  runner.register_test(std::make_unique<MathOperationsTest>());
  runner.register_test(std::make_unique<VectorOperationsTest>());
  runner.register_test(std::make_unique<PerformanceTest>());
  runner.register_test(std::make_unique<EdgeCaseTest>());
  runner.register_test(std::make_unique<ExceptionHandlingTest>());

  // Set up callbacks
  runner.set_progress_callback([](const std::string& message, double percentage) {
    std::cout << "[PROGRESS] " << message << " (" << percentage << "%)" << std::endl;
  });

  runner.set_test_started_callback(
      [](const std::string& test_name) { std::cout << "[STARTED] " << test_name << std::endl; });

  runner.set_test_completed_callback([](const TestResult& result) {
    std::cout << "[COMPLETED] " << result.to_string() << std::endl;
    if (result.has_metadata("captured_output")) {
      std::cout << "[OUTPUT] " << result.get_metadata("captured_output") << std::endl;
    }
  });

  // Run all tests
  std::cout << "\n=== Running Realistic Test Suite ===" << std::endl;
  TestSuiteResult suite_result = runner.run_all_tests();

  // Print summary
  std::cout << "\n=== Test Summary ===" << std::endl;
  std::cout << suite_result.summary() << std::endl;

  // Test filtering by tags
  std::cout << "\n=== Running Unit Tests Only ===" << std::endl;
  TestSuiteResult unit_tests = runner.run_tests_with_tag("unit");
  std::cout << "Unit tests summary: " << unit_tests.summary() << std::endl;

  std::cout << "\n=== Running Performance Tests Only ===" << std::endl;
  TestSuiteResult perf_tests = runner.run_tests_with_tag("performance");
  std::cout << "Performance tests summary: " << perf_tests.summary() << std::endl;

  // Check if all tests passed
  if (suite_result.all_passed()) {
    std::cout << "\n🎉 All tests passed! The testing framework is working correctly." << std::endl;
    return 0;
  } else {
    std::cout << "\n❌ Some tests failed. Check the output above for details." << std::endl;
    return 1;
  }
}
