/**
 * @file test_discovery_example.cpp
 * @brief Example demonstrating automatic test discovery and registration
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>

#include "solar_test/framework/assertions.hpp"
#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_discovery.hpp"

using namespace SolarSystem::Testing;

// Example 1: Simple test with automatic registration
SOLAR_TEST_CASE_AUTO(BasicArithmeticTest, "Test basic arithmetic operations", "unit", "fast") {
  int result = 2 + 2;
  Assertions::assert_equals(4, result, "2 + 2 should equal 4");

  double division = 10.0 / 3.0;
  Assertions::assert_true(division > 3.0, "10/3 should be greater than 3");
}

// Example 2: Integration test with slow tag
SOLAR_TEST_CASE_AUTO(DatabaseConnectionTest, "Test database connectivity", "integration", "slow") {
  // Simulate a slow database connection test
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Assertions::assert_true(true, "Database connection successful");
}

// Example 3: Benchmark test
SOLAR_BENCHMARK_CASE_AUTO(SortingPerformance, "Benchmark sorting algorithms", "benchmark",
                          "performance") {
  std::vector<int> data(1000);
  std::iota(data.begin(), data.end(), 0);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(data.begin(), data.end(), g);

  auto start = std::chrono::high_resolution_clock::now();
  std::sort(data.begin(), data.end());
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  Assertions::assert_true(duration.count() < 10000, "Sorting should complete within 10ms");
}

// Example 4: Custom test class with manual registration
class CustomMathTest : public TestCase {
 public:
  CustomMathTest()
      : TestCase({"CustomMathTest",
                  "Custom mathematical operations test",
                  {"unit", "math", "custom"},
                  std::chrono::seconds(30),
                  false,
                  false,
                  ""}) {}

  void run() override {
    // Test trigonometric functions
    double sin_result = std::sin(M_PI / 2);
    Assertions::assert_near(1.0, sin_result, 1e-10, "sin(π/2) should equal 1");

    double cos_result = std::cos(0);
    Assertions::assert_near(1.0, cos_result, 1e-10, "cos(0) should equal 1");

    // Test logarithmic functions
    double log_result = std::log(std::exp(1));
    Assertions::assert_near(1.0, log_result, 1e-10, "log(e) should equal 1");
  }
};

// Register the custom test with tags
SOLAR_REGISTER_TEST_WITH_TAGS(CustomMathTest, "unit", "math", "custom");

// Example 5: Test with specific timeout
class TimeoutTest : public TestCase {
 public:
  TimeoutTest()
      : TestCase({"TimeoutTest",
                  "Test with custom timeout",
                  {"unit", "timeout"},
                  std::chrono::milliseconds(500),  // Short timeout
                  false,
                  false,
                  ""}) {}

  void run() override {
    // This test should complete quickly
    Assertions::assert_true(true, "Quick test");
  }
};

SOLAR_REGISTER_TEST_WITH_TAGS(TimeoutTest, "unit", "timeout");

// Example 6: Test that demonstrates tag-based filtering
class NetworkTest : public TestCase {
 public:
  NetworkTest()
      : TestCase({"NetworkTest",
                  "Network connectivity test",
                  {"integration", "network", "external"},
                  std::chrono::minutes(1),
                  false,
                  false,
                  ""}) {}

  void run() override {
    // Simulate network test
    Assertions::assert_true(true, "Network test passed");
  }
};

SOLAR_REGISTER_TEST_WITH_TAGS(NetworkTest, "integration", "network", "external");

// Example main function showing how to use discovery
int main() {
  std::cout << "=== Test Discovery Example ===" << std::endl;

  // Get all available tests
  auto all_tests = TestDiscovery::instance().discover_all_tests();
  std::cout << "Total tests discovered: " << all_tests.size() << std::endl;

  // Get available test names
  auto test_names = TestDiscovery::instance().get_available_test_names();
  std::cout << "\nAvailable tests:" << std::endl;
  for (const auto& name : test_names) {
    std::cout << "  - " << name << std::endl;
  }

  // Get available tags
  auto tags = TestDiscovery::instance().get_available_tags();
  std::cout << "\nAvailable tags:" << std::endl;
  for (const auto& tag : tags) {
    std::cout << "  - " << tag << std::endl;
  }

  // Demonstrate filtering by tag
  std::cout << "\n=== Unit Tests ===" << std::endl;
  auto unit_tests = TestDiscovery::instance().discover_tests_by_tag("unit");
  for (const auto& test : unit_tests) {
    std::cout << "  - " << test->info().name << ": " << test->info().description << std::endl;
  }

  std::cout << "\n=== Integration Tests ===" << std::endl;
  auto integration_tests = TestDiscovery::instance().discover_tests_by_tag("integration");
  for (const auto& test : integration_tests) {
    std::cout << "  - " << test->info().name << ": " << test->info().description << std::endl;
  }

  std::cout << "\n=== Benchmark Tests ===" << std::endl;
  auto benchmark_tests = TestDiscovery::instance().discover_tests_by_tag("benchmark");
  for (const auto& test : benchmark_tests) {
    std::cout << "  - " << test->info().name << ": " << test->info().description << std::endl;
  }

  // Demonstrate pattern matching
  std::cout << "\n=== Tests matching '*Math*' ===" << std::endl;
  auto math_tests = TestDiscovery::instance().discover_tests_by_pattern(".*Math.*");
  for (const auto& test : math_tests) {
    std::cout << "  - " << test->info().name << ": " << test->info().description << std::endl;
  }

  // Demonstrate advanced filtering
  std::cout << "\n=== Fast Unit Tests (excluding slow) ===" << std::endl;
  TestDiscovery::DiscoveryOptions options;
  options.required_tags = {"unit"};
  options.excluded_tags = {"slow"};
  auto fast_unit_tests = TestDiscovery::instance().discover_tests(options);
  for (const auto& test : fast_unit_tests) {
    std::cout << "  - " << test->info().name << ": " << test->info().description << std::endl;
  }

  // Show test count by category
  std::cout << "\n=== Test Count by Category ===" << std::endl;
  auto counts = TestDiscovery::instance().get_test_count_by_category();
  for (const auto& [category, count] : counts) {
    if (count > 0) {
      std::cout << "  " << category << ": " << count << std::endl;
    }
  }

  return 0;
}
