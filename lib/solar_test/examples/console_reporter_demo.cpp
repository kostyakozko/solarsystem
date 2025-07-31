/**
 * @file console_reporter_demo.cpp
 * @brief Demonstration of the console reporter functionality
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;

class SamplePassingTest : public TestCase {
 public:
  SamplePassingTest()
      : TestCase({"SamplePassingTest", "A simple passing test", {"unit", "demo"}}) {}

  void run() override {
    assert_true(true, "This should always pass");
    assert_equals(42, 42, "Basic equality check");

    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
};

class SampleFailingTest : public TestCase {
 public:
  SampleFailingTest()
      : TestCase(
            {"SampleFailingTest", "A test that demonstrates failure reporting", {"unit", "demo"}}) {
  }

  void run() override {
    assert_true(false, "This assertion should fail");
    assert_equals(1, 2, "This equality check should fail");
  }
};

class SampleSlowTest : public TestCase {
 public:
  SampleSlowTest()
      : TestCase(
            {"SampleSlowTest", "A test that takes some time", {"integration", "slow", "demo"}}) {}

  void run() override {
    // Simulate longer work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert_true(true, "Slow test completed");
  }
};

class SampleSkippedTest : public TestCase {
 public:
  SampleSkippedTest()
      : TestCase({"SampleSkippedTest", "A test that gets skipped", {"unit", "demo"}}) {}

  void run() override {
    // This test will be skipped in the demo
    assert_true(true, "This won't run");
  }
};

int main() {
  std::cout << "=== Console Reporter Demo ===\n\n";

  // Create test runner with console reporter
  TestRunner::Configuration config;
  config.verbose = true;
  config.parallel_execution = false;  // Sequential for clearer demo output

  TestRunner runner(config);

  // Register test cases
  runner.register_test(std::make_unique<SamplePassingTest>());
  runner.register_test(std::make_unique<SampleFailingTest>());
  runner.register_test(std::make_unique<SampleSlowTest>());

  // Run all tests
  std::cout << "Running tests with verbose console output:\n\n";
  TestSuiteResult result = runner.run_all_tests();

  std::cout << "\n=== Demo with Quiet Mode ===\n\n";

  // Create another runner with quiet mode
  TestRunner::Configuration quiet_config;
  quiet_config.quiet = true;
  quiet_config.parallel_execution = false;

  TestRunner quiet_runner(quiet_config);
  quiet_runner.register_test(std::make_unique<SamplePassingTest>());
  quiet_runner.register_test(std::make_unique<SampleFailingTest>());

  std::cout << "Running tests with quiet mode (only failures shown):\n\n";
  TestSuiteResult quiet_result = quiet_runner.run_all_tests();

  std::cout << "\n=== Demo with Non-Colorized Output ===\n\n";

  // Create runner with non-colorized console reporter
  TestRunner::Configuration no_color_config;
  no_color_config.verbose = true;
  no_color_config.parallel_execution = false;

  TestRunner no_color_runner(no_color_config);
  no_color_runner.clear_reporters();  // Remove default reporter

  ConsoleReporter::Configuration console_config;
  console_config.colorized = false;
  console_config.show_progress = false;
  no_color_runner.add_reporter(std::make_unique<ConsoleReporter>(console_config));

  no_color_runner.register_test(std::make_unique<SamplePassingTest>());
  no_color_runner.register_test(std::make_unique<SampleFailingTest>());

  std::cout << "Running tests with non-colorized output:\n\n";
  TestSuiteResult no_color_result = no_color_runner.run_all_tests();

  return result.all_passed() ? 0 : 1;
}
