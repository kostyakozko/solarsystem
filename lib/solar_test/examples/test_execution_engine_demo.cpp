#include <chrono>
#include <iostream>
#include <thread>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_runner.hpp"

using namespace SolarSystem::Testing;

// Test case that passes quickly
class QuickPassingTest : public TestCase {
 public:
  QuickPassingTest()
      : TestCase({"QuickPassingTest",
                  "A test that passes quickly",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    assert_true(true, "This should always pass");
    std::cout << "Quick test output" << std::endl;
  }
};

// Test case that fails with assertion
class FailingTest : public TestCase {
 public:
  FailingTest()
      : TestCase({"FailingTest", "A test that fails", {"unit"}, std::chrono::seconds(5), false}) {}

  void run() override { assert_true(false, "This test is designed to fail"); }
};

// Test case that times out
class TimeoutTest : public TestCase {
 public:
  TimeoutTest()
      : TestCase({"TimeoutTest",
                  "A test that times out",
                  {"slow"},
                  std::chrono::milliseconds(100),
                  false}) {}

  void run() override {
    // Sleep longer than the timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    assert_true(true, "This should not be reached due to timeout");
  }
};

// Test case that throws an exception
class ExceptionTest : public TestCase {
 public:
  ExceptionTest()
      : TestCase({"ExceptionTest",
                  "A test that throws an exception",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override { throw std::runtime_error("Test exception"); }
};

// Test case that produces output
class OutputTest : public TestCase {
 public:
  OutputTest()
      : TestCase({"OutputTest",
                  "A test that produces output",
                  {"unit"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    std::cout << "This is stdout output" << std::endl;
    std::cerr << "This is stderr output" << std::endl;
    assert_true(true, "Test passes after producing output");
  }
};

int main() {
  std::cout << "=== Test Execution Engine Demo ===" << std::endl;

  // Create test runner with configuration
  TestRunner::Configuration config;
  config.parallel_execution = false;  // Use sequential for clearer output
  config.verbose = true;
  config.quiet = false;
  config.timeout = std::chrono::seconds(10);

  TestRunner runner(config);

  // Register test cases
  runner.register_test(std::make_unique<QuickPassingTest>());
  runner.register_test(std::make_unique<FailingTest>());
  runner.register_test(std::make_unique<TimeoutTest>());
  runner.register_test(std::make_unique<ExceptionTest>());
  runner.register_test(std::make_unique<OutputTest>());

  // Set up callbacks to see the execution progress
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
  std::cout << "\n=== Running All Tests ===" << std::endl;
  TestSuiteResult suite_result = runner.run_all_tests();

  // Print summary
  std::cout << "\n=== Test Summary ===" << std::endl;
  std::cout << suite_result.summary() << std::endl;

  // Test specific functionality
  std::cout << "\n=== Testing Timeout Handling ===" << std::endl;
  TestSuiteResult timeout_result = runner.run_specific_test("TimeoutTest");
  std::cout << "Timeout test result: " << timeout_result.test_results[0].to_string() << std::endl;

  std::cout << "\n=== Testing Tag Filtering ===" << std::endl;
  TestSuiteResult unit_tests = runner.run_tests_with_tag("unit");
  std::cout << "Unit tests summary: " << unit_tests.summary() << std::endl;

  return suite_result.all_passed() ? 0 : 1;
}
