#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace SolarSystem::Testing {

/**
 * @brief Result of a single test execution
 */
struct TestResult {
  enum class Status { Passed, Failed, Skipped, Timeout, Error, ExpectedFailure };

  Status status = Status::Failed;
  std::string test_name;
  std::string error_message;
  std::chrono::milliseconds execution_time{0};
  size_t memory_usage_bytes = 0;
  std::vector<std::string> assertion_failures;
  std::map<std::string, std::string> metadata;
  bool was_expected_to_fail = false;
  std::string expected_failure_reason;

  [[nodiscard]] bool passed() const { return status == Status::Passed; }
  [[nodiscard]] bool failed() const { return status == Status::Failed; }
  [[nodiscard]] bool succeeded() const {
    return status == Status::Passed || status == Status::ExpectedFailure;
  }
  [[nodiscard]] std::string status_string() const;
  [[nodiscard]] std::string to_string() const;

  // Metadata management
  void add_metadata(const std::string& key, const std::string& value);
  [[nodiscard]] bool has_metadata(const std::string& key) const;
  [[nodiscard]] std::string get_metadata(const std::string& key) const;
};

/**
 * @brief Result of a complete test suite execution
 */
struct TestSuiteResult {
  std::string suite_name;
  std::vector<TestResult> test_results;
  std::chrono::milliseconds total_execution_time{0};
  size_t passed_count = 0;
  size_t failed_count = 0;
  size_t skipped_count = 0;
  double code_coverage_percentage = 0.0;

  [[nodiscard]] bool all_passed() const { return failed_count == 0; }
  [[nodiscard]] double success_rate() const;
  [[nodiscard]] std::string summary() const;

  void add_result(const TestResult& result);
  void calculate_statistics();
};

}  // namespace SolarSystem::Testing
