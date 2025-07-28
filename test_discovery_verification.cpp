#include <iostream>

#include "lib/solar_test/include/solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;

// Test case to verify test discovery mechanism
SOLAR_TEST_CASE_AUTO(TestDiscoveryBasic, "Basic test discovery functionality", "unit",
                     "discovery") {
  // Test that we can create a test discovery instance
  auto& discovery = TestDiscovery::instance();

  // Verify basic functionality
  assert_true(true, "Basic assertion works");

  // Test that we can get available test names
  auto test_names = discovery.get_available_test_names();
  assert_true(!test_names.empty(), "Should have at least one test registered");

  // Test that we can get available tags
  auto tags = discovery.get_available_tags();
  assert_true(!tags.empty(), "Should have at least one tag available");
}

// Test case to verify test execution engine
SOLAR_TEST_CASE_AUTO(TestExecutionBasic, "Basic test execution functionality", "unit",
                     "execution") {
  // Test basic assertions
  assert_true(true, "True assertion should pass");
  assert_false(false, "False assertion should pass");
  assert_equals(42, 42, "Equality assertion should pass");
  assert_not_equals(42, 43, "Inequality assertion should pass");

  // Test string assertions
  assert_contains("hello world", "world", "String contains assertion should pass");

  // Test exception handling
  assert_throws([]() { throw std::runtime_error("test"); }, "Exception assertion should pass");
  assert_no_throw([]() { /* do nothing */ }, "No exception assertion should pass");
}

// Benchmark test case to verify benchmark functionality
SOLAR_BENCHMARK_CASE_AUTO(BenchmarkBasic, "Basic benchmark functionality", "benchmark") {
  // Simple benchmark test
  volatile int sum = 0;
  for (int i = 0; i < 1000; ++i) {
    sum += i;
  }

  assert_true(sum > 0, "Benchmark calculation should produce result");
}

SOLAR_TEST_MAIN()
