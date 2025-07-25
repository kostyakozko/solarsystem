#include "solar_test/framework/test_result.hpp"

#include <iomanip>
#include <sstream>

namespace SolarSystem::Testing {

std::string TestResult::status_string() const {
  switch (status) {
    case Status::Passed:
      return "PASSED";
    case Status::Failed:
      return "FAILED";
    case Status::Skipped:
      return "SKIPPED";
    case Status::Timeout:
      return "TIMEOUT";
    case Status::Error:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

std::string TestResult::to_string() const {
  std::ostringstream oss;
  oss << "[" << status_string() << "] " << test_name;

  if (execution_time.count() > 0) {
    oss << " (" << execution_time.count() << "ms)";
  }

  if (!error_message.empty()) {
    oss << " - " << error_message;
  }

  if (!assertion_failures.empty()) {
    oss << "\n  Assertion failures:";
    for (const auto& failure : assertion_failures) {
      oss << "\n    - " << failure;
    }
  }

  return oss.str();
}

double TestSuiteResult::success_rate() const {
  size_t total = passed_count + failed_count + skipped_count;
  if (total == 0) return 0.0;
  return static_cast<double>(passed_count) / static_cast<double>(total) * 100.0;
}

std::string TestSuiteResult::summary() const {
  std::ostringstream oss;
  oss << "Test Suite: " << suite_name << "\n";
  oss << "Results: " << passed_count << " passed, " << failed_count << " failed, " << skipped_count
      << " skipped\n";
  oss << "Success Rate: " << std::fixed << std::setprecision(1) << success_rate() << "%\n";
  oss << "Total Time: " << total_execution_time.count() << "ms";

  if (code_coverage_percentage > 0.0) {
    oss << "\nCode Coverage: " << std::fixed << std::setprecision(1) << code_coverage_percentage
        << "%";
  }

  return oss.str();
}

void TestSuiteResult::add_result(const TestResult& result) {
  test_results.push_back(result);
  total_execution_time += result.execution_time;

  switch (result.status) {
    case TestResult::Status::Passed:
      ++passed_count;
      break;
    case TestResult::Status::Failed:
    case TestResult::Status::Timeout:
    case TestResult::Status::Error:
      ++failed_count;
      break;
    case TestResult::Status::Skipped:
      ++skipped_count;
      break;
  }
}

void TestSuiteResult::calculate_statistics() {
  passed_count = 0;
  failed_count = 0;
  skipped_count = 0;
  total_execution_time = std::chrono::milliseconds(0);

  for (const auto& result : test_results) {
    total_execution_time += result.execution_time;

    switch (result.status) {
      case TestResult::Status::Passed:
        ++passed_count;
        break;
      case TestResult::Status::Failed:
      case TestResult::Status::Timeout:
      case TestResult::Status::Error:
        ++failed_count;
        break;
      case TestResult::Status::Skipped:
        ++skipped_count;
        break;
    }
  }
}

}  // namespace SolarSystem::Testing
