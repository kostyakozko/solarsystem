/**
 * @file structured_reporters_demo.cpp
 * @brief Demonstration of structured output reporters (XML, JSON, TAP, Coverage)
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

class SampleSkippedTest : public TestCase {
 public:
  SampleSkippedTest()
      : TestCase({"SampleSkippedTest", "A test that gets skipped", {"unit", "demo"}}) {}

  void run() override {
    // Mark this test as skipped by throwing a skip exception
    // For now, we'll just pass it
    assert_true(true, "This test runs but could be skipped");
  }
};

int main() {
  std::cout << "=== Structured Output Reporters Demo ===\n\n";

  // Create test runner without default console reporter
  TestRunner::Configuration config;
  config.verbose = false;
  config.quiet = true;  // Suppress console output
  config.parallel_execution = false;

  TestRunner runner(config);
  runner.clear_reporters();  // Remove default console reporter

  // Add structured output reporters
  runner.add_reporter(TestReporterFactory::create_xml_reporter("demo_results.xml"));
  runner.add_reporter(TestReporterFactory::create_json_reporter("demo_results.json"));
  runner.add_reporter(TestReporterFactory::create_tap_reporter("demo_results.tap"));
  runner.add_reporter(TestReporterFactory::create_coverage_reporter("demo_coverage.txt"));

  // Register test cases
  runner.register_test(std::make_unique<SamplePassingTest>());
  runner.register_test(std::make_unique<SampleFailingTest>());
  runner.register_test(std::make_unique<SampleSkippedTest>());

  std::cout << "Running tests with structured output reporters...\n";
  std::cout << "Output files will be generated:\n";
  std::cout << "  - demo_results.xml (JUnit XML format)\n";
  std::cout << "  - demo_results.json (JSON format)\n";
  std::cout << "  - demo_results.tap (TAP format)\n";
  std::cout << "  - demo_coverage.txt (Coverage report)\n\n";

  // Run all tests
  TestSuiteResult result = runner.run_all_tests();

  std::cout << "Tests completed. Check the generated files for structured output.\n";
  std::cout << "Summary: " << result.passed_count << " passed, " << result.failed_count
            << " failed, " << result.skipped_count << " skipped\n";

  return result.all_passed() ? 0 : 1;
}
