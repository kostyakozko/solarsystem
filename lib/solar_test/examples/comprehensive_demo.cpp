#include <chrono>
#include <iostream>
#include <thread>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_runner.hpp"

using namespace SolarSystem::Testing;

// Normal tests that should pass
class BasicMathTest : public TestCase {
 public:
  BasicMathTest()
      : TestCase({"BasicMathTest",
                  "Test basic math operations",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  false,
                  ""}) {}

  void run() override {
    assert_true(2 + 2 == 4, "Basic addition should work");
    assert_true(10 - 5 == 5, "Basic subtraction should work");
  }
};

class StringTest : public TestCase {
 public:
  StringTest()
      : TestCase({"StringTest",
                  "Test string operations",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  false,
                  ""}) {}

  void run() override {
    std::string test = "Hello World";
    assert_true(test.length() == 11, "String length should be correct");
    assert_true(test.find("World") != std::string::npos, "Should find substring");
  }
};

// Expected failure tests (for testing error handling, edge cases, etc.)
class ErrorHandlingTest : public TestCase {
 public:
  ErrorHandlingTest()
      : TestCase({"ErrorHandlingTest",
                  "Test error handling",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  true,
                  "Testing assertion failure handling"}) {}

  void run() override {
    // This test is designed to fail to verify error handling works
    assert_true(false, "This assertion should fail to test error handling");
  }
};

class TimeoutHandlingTest : public TestCase {
 public:
  TimeoutHandlingTest()
      : TestCase({"TimeoutHandlingTest",
                  "Test timeout handling",
                  {"slow"},
                  std::chrono::milliseconds(50),
                  false,
                  true,
                  "Testing timeout mechanism"}) {}

  void run() override {
    // This test is designed to timeout to verify timeout handling works
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert_true(true, "Should not reach here due to timeout");
  }
};

class ExceptionHandlingTest : public TestCase {
 public:
  ExceptionHandlingTest()
      : TestCase({"ExceptionHandlingTest",
                  "Test exception handling",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  true,
                  "Testing exception handling mechanism"}) {}

  void run() override {
    // This test is designed to throw to verify exception handling works
    throw std::runtime_error("Intentional exception for testing");
  }
};

// Performance test
class PerformanceTest : public TestCase {
 public:
  PerformanceTest()
      : TestCase({"PerformanceTest",
                  "Test performance requirements",
                  {"performance"},
                  std::chrono::seconds(5),
                  false,
                  false,
                  ""}) {}

  void run() override {
    assert_execution_time_less_than(
        []() {
          // Simple operation that should complete quickly
          int sum = 0;
          for (int i = 0; i < 1000; ++i) {
            sum += i;
          }
        },
        std::chrono::milliseconds(5));

    assert_true(true, "Performance test completed");
  }
};

int main() {
  std::cout << "=== Comprehensive Testing Framework Demo ===" << std::endl;
  std::cout
      << "This demo shows how to properly use expected failures to achieve high success rates.\n"
      << std::endl;

  TestRunner::Configuration config;
  config.parallel_execution = false;
  config.verbose = true;
  config.quiet = false;

  TestRunner runner(config);

  // Register all test cases
  runner.register_test(std::make_unique<BasicMathTest>());
  runner.register_test(std::make_unique<StringTest>());
  runner.register_test(std::make_unique<ErrorHandlingTest>());
  runner.register_test(std::make_unique<TimeoutHandlingTest>());
  runner.register_test(std::make_unique<ExceptionHandlingTest>());
  runner.register_test(std::make_unique<PerformanceTest>());

  // Set up callbacks
  runner.set_progress_callback([](const std::string& message, double percentage) {
    std::cout << "[PROGRESS] " << message << " (" << percentage << "%)" << std::endl;
  });

  runner.set_test_started_callback(
      [](const std::string& test_name) { std::cout << "[STARTED] " << test_name << std::endl; });

  runner.set_test_completed_callback([](const TestResult& result) {
    std::cout << "[COMPLETED] " << result.to_string() << std::endl;
  });

  // Run all tests
  std::cout << "=== Running All Tests ===" << std::endl;
  TestSuiteResult suite_result = runner.run_all_tests();

  // Print detailed summary
  std::cout << "\n=== Detailed Test Summary ===" << std::endl;
  std::cout << suite_result.summary() << std::endl;

  std::cout << "\n=== Test Behavior Analysis ===" << std::endl;
  for (const auto& result : suite_result.test_results) {
    std::cout << "• " << result.test_name << ": " << result.status_string();
    if (result.was_expected_to_fail) {
      std::cout << " (Expected to fail: " << result.expected_failure_reason << ")";
    }
    std::cout << std::endl;
  }

  std::cout << "\n=== Framework Validation ===" << std::endl;
  if (suite_result.success_rate() >= 100.0) {
    std::cout << "🎉 Perfect! All tests behaved exactly as expected." << std::endl;
    std::cout << "✅ Normal tests passed successfully" << std::endl;
    std::cout << "✅ Expected failure tests failed as intended (counting as successes)"
              << std::endl;
    std::cout << "✅ The testing framework is working correctly!" << std::endl;
  } else {
    std::cout << "⚠️  Some tests didn't behave as expected." << std::endl;
    std::cout << "Success rate: " << suite_result.success_rate() << "%" << std::endl;
  }

  // Test filtering capabilities
  std::cout << "\n=== Testing Filter Capabilities ===" << std::endl;

  std::cout << "\nRunning only unit tests:" << std::endl;
  TestSuiteResult unit_tests = runner.run_tests_with_tag("unit");
  std::cout << "Unit tests: " << unit_tests.passed_count << " passed, " << unit_tests.failed_count
            << " failed (" << unit_tests.success_rate() << "% success)" << std::endl;

  std::cout << "\nRunning only performance tests:" << std::endl;
  TestSuiteResult perf_tests = runner.run_tests_with_tag("performance");
  std::cout << "Performance tests: " << perf_tests.passed_count << " passed, "
            << perf_tests.failed_count << " failed (" << perf_tests.success_rate() << "% success)"
            << std::endl;

  return suite_result.all_passed() ? 0 : 1;
}
