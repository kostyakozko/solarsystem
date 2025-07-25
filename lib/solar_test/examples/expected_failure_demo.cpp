#include <chrono>
#include <iostream>
#include <thread>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_runner.hpp"

using namespace SolarSystem::Testing;

// Test case that passes normally
class QuickPassingTest : public TestCase {
 public:
  QuickPassingTest()
      : TestCase({"QuickPassingTest",
                  "A test that passes quickly",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  false,
                  ""}) {}

  void run() override {
    assert_true(true, "This should always pass");
    std::cout << "Quick test output" << std::endl;
  }
};

// Test case that is EXPECTED to fail (testing error handling)
class ExpectedFailingTest : public TestCase {
 public:
  ExpectedFailingTest()
      : TestCase({"ExpectedFailingTest",
                  "A test that is expected to fail",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  true,
                  "Testing assertion failure handling"}) {}

  void run() override {
    assert_true(false, "This test is designed to fail to test error handling");
  }
};

// Test case that is EXPECTED to timeout (testing timeout handling)
class ExpectedTimeoutTest : public TestCase {
 public:
  ExpectedTimeoutTest()
      : TestCase({"ExpectedTimeoutTest",
                  "A test that is expected to timeout",
                  {"slow"},
                  std::chrono::milliseconds(100),
                  false,
                  true,
                  "Testing timeout handling"}) {}

  void run() override {
    // Sleep longer than the timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    assert_true(true, "This should not be reached due to timeout");
  }
};

// Test case that is EXPECTED to throw an exception (testing exception handling)
class ExpectedExceptionTest : public TestCase {
 public:
  ExpectedExceptionTest()
      : TestCase({"ExpectedExceptionTest",
                  "A test that is expected to throw",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  true,
                  "Testing exception handling"}) {}

  void run() override { throw std::runtime_error("Test exception for error handling validation"); }
};

// Test case that produces output and passes
class OutputTest : public TestCase {
 public:
  OutputTest()
      : TestCase({"OutputTest",
                  "A test that produces output",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  false,
                  ""}) {}

  void run() override {
    std::cout << "This is stdout output" << std::endl;
    std::cerr << "This is stderr output" << std::endl;
    assert_true(true, "Test passes after producing output");
  }
};

// Test case that was expected to fail but actually passes (this should be marked as failed)
class UnexpectedPassTest : public TestCase {
 public:
  UnexpectedPassTest()
      : TestCase({"UnexpectedPassTest",
                  "A test expected to fail but passes",
                  {"unit"},
                  std::chrono::seconds(5),
                  false,
                  true,
                  "This was supposed to fail but doesn't"}) {}

  void run() override {
    // This test is marked as expected to fail, but it actually passes
    // This should result in a failure because it didn't behave as expected
    assert_true(true, "This passes but was expected to fail");
  }
};

int main() {
  std::cout << "=== Expected Failure Test Demo ===" << std::endl;

  // Create test runner with configuration
  TestRunner::Configuration config;
  config.parallel_execution = false;  // Use sequential for clearer output
  config.verbose = true;
  config.quiet = false;
  config.timeout = std::chrono::seconds(10);

  TestRunner runner(config);

  // Register test cases
  runner.register_test(std::make_unique<QuickPassingTest>());
  runner.register_test(std::make_unique<ExpectedFailingTest>());
  runner.register_test(std::make_unique<ExpectedTimeoutTest>());
  runner.register_test(std::make_unique<ExpectedExceptionTest>());
  runner.register_test(std::make_unique<OutputTest>());
  runner.register_test(std::make_unique<UnexpectedPassTest>());

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

  // Explain the results
  std::cout << "\n=== Results Explanation ===" << std::endl;
  std::cout << "• QuickPassingTest: PASSED (normal success)" << std::endl;
  std::cout << "• ExpectedFailingTest: EXPECTED_FAILURE (counts as success - failed as expected)"
            << std::endl;
  std::cout << "• ExpectedTimeoutTest: EXPECTED_FAILURE (counts as success - timed out as expected)"
            << std::endl;
  std::cout << "• ExpectedExceptionTest: EXPECTED_FAILURE (counts as success - threw exception as "
               "expected)"
            << std::endl;
  std::cout << "• OutputTest: PASSED (normal success)" << std::endl;
  std::cout
      << "• UnexpectedPassTest: FAILED (unexpected behavior - was supposed to fail but passed)"
      << std::endl;

  std::cout << "\nExpected success rate: 83.3% (5 out of 6 tests behaved as expected)" << std::endl;
  std::cout << "Actual success rate: " << suite_result.success_rate() << "%" << std::endl;

  if (suite_result.success_rate() > 80.0) {
    std::cout << "\n✅ Success rate is high - most tests behaved as expected!" << std::endl;
  } else {
    std::cout << "\n❌ Success rate is low - tests are not behaving as expected." << std::endl;
  }

  return suite_result.all_passed() ? 0 : 1;
}
