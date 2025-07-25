#include <iostream>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;

// Example basic test case
SOLAR_TEST_CASE(BasicMathTest, "Test basic mathematical operations") {
  // Test basic assertions
  assert_true(2 + 2 == 4, "Basic addition should work");
  assert_false(2 + 2 == 5, "Basic addition should not equal 5");

  // Test equality assertions
  ASSERT_EQ(10, 5 + 5);
  ASSERT_NE(10, 5 + 4);

  // Test numeric comparisons
  ASSERT_GT(10, 5);
  ASSERT_LT(5, 10);

  // Test floating point near equality
  ASSERT_NEAR(3.14159, 22.0 / 7.0, 0.01);

  add_metadata("category", "math");
  add_metadata("complexity", "simple");
}

// Example test with string operations
SOLAR_TEST_CASE(StringOperationsTest, "Test string manipulation functions") {
  std::string test_string = "Solar System Suite";

  ASSERT_CONTAINS(test_string, "Solar");
  ASSERT_CONTAINS(test_string, "System");

  // Test string starts/ends with
  assert_true(test_string.find("Solar") == 0, "Should start with Solar");
  assert_true(test_string.find("Suite") != std::string::npos, "Should contain Suite");

  add_metadata("category", "string");
}

// Example test that demonstrates exception handling
SOLAR_TEST_CASE(ExceptionHandlingTest, "Test exception assertion capabilities") {
  // Test that an exception is thrown
  ASSERT_THROWS(std::runtime_error, []() { throw std::runtime_error("Test exception"); });

  // Test that no exception is thrown
  ASSERT_NO_THROW([]() {
    int x = 5 + 5;
    (void)x;  // Suppress unused variable warning
  });

  add_metadata("category", "exceptions");
}

// Example performance test
SOLAR_BENCHMARK_CASE(PerformanceTest, "Test performance measurement capabilities") {
  // Test execution time assertion
  assert_execution_time_less_than(
      []() {
        // Simulate some work
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) {
          sum += i;
        }
      },
      std::chrono::milliseconds(10));

  add_metadata("category", "performance");
  add_metadata("benchmark", "true");
}

// Example test that can be skipped conditionally
SOLAR_TEST_CASE(ConditionalTest, "Test that demonstrates conditional skipping") {
  // Skip test under certain conditions
  if (std::getenv("SKIP_CONDITIONAL_TESTS")) {
    skip_test("Skipped due to SKIP_CONDITIONAL_TESTS environment variable");
    return;
  }

  assert_true(true, "This test should pass if not skipped");
  add_metadata("category", "conditional");
}

// Register all tests with the test registry
REGISTER_TEST(BasicMathTest);
REGISTER_TEST(StringOperationsTest);
REGISTER_TEST(ExceptionHandlingTest);
REGISTER_TEST(PerformanceTest);
REGISTER_TEST(ConditionalTest);

// Example of how to create a simple test runner
int main(int argc, char* argv[]) {
  std::cout << "Solar System Testing Framework - Example Tests\n";
  std::cout << "==============================================\n\n";

  return SolarSystem::Testing::run_tests(argc, argv);
}
