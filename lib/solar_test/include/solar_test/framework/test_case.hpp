#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "test_result.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Base class for all test cases
 */
class TestCase {
 public:
  struct TestInfo {
    std::string name;
    std::string description;
    std::vector<std::string> tags;
    std::chrono::milliseconds timeout = std::chrono::seconds(30);
    bool is_benchmark = false;
  };

  explicit TestCase(TestInfo info);
  virtual ~TestCase() = default;

  // Test lifecycle
  virtual void setup() {}
  virtual void run() = 0;
  virtual void teardown() {}

  // Test information
  [[nodiscard]] const TestInfo& info() const { return info_; }
  [[nodiscard]] const TestResult& result() const { return result_; }

  // Execute the complete test lifecycle
  TestResult execute();

 protected:
  // Assertion helpers
  void assert_true(bool condition, const std::string& message = "");
  void assert_false(bool condition, const std::string& message = "");

  template <typename T>
  void assert_equals(const T& expected, const T& actual, const std::string& message = "");

  template <typename T>
  void assert_not_equals(const T& expected, const T& actual, const std::string& message = "");

  void assert_throws(const std::function<void()>& func, const std::string& message = "");
  void assert_no_throw(const std::function<void()>& func, const std::string& message = "");

  // Performance assertions
  void assert_execution_time_less_than(const std::function<void()>& func,
                                       std::chrono::milliseconds max_time);
  void assert_memory_usage_less_than(const std::function<void()>& func, size_t max_bytes);

  // Test utilities
  void skip_test(const std::string& reason);
  void add_metadata(const std::string& key, const std::string& value);

 private:
  TestInfo info_;
  TestResult result_;
  bool test_skipped_ = false;

  void record_assertion_failure(const std::string& message);
  void measure_memory_usage();
};

/**
 * @brief Macro for creating test cases
 */
#define SOLAR_TEST_CASE(name, description)                                          \
  class name : public SolarSystem::Testing::TestCase {                              \
   public:                                                                          \
    name() : TestCase({#name, description, {}, std::chrono::seconds(30), false}) {} \
    void run() override;                                                            \
  };                                                                                \
  void name::run()

/**
 * @brief Macro for creating benchmark test cases
 */
#define SOLAR_BENCHMARK_CASE(name, description)                                              \
  class name : public SolarSystem::Testing::TestCase {                                       \
   public:                                                                                   \
    name() : TestCase({#name, description, {"benchmark"}, std::chrono::minutes(5), true}) {} \
    void run() override;                                                                     \
  };                                                                                         \
  void name::run()

}  // namespace SolarSystem::Testing
