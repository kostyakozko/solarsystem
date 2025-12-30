#include "solar_test/reporters/json_reporter.hpp"

#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

namespace SolarSystem::Testing {

JsonReporter::JsonReporter(Configuration config) : config_(std::move(config)) {
  output_file_.open(config_.output_file);
  if (!output_file_.is_open()) {
    throw std::runtime_error("Failed to open JSON output file: " + config_.output_file);
  }
}

JsonReporter::JsonReporter(const std::string& output_file)
    : JsonReporter(Configuration{output_file}) {}

void JsonReporter::on_suite_started(const std::string& suite_name, size_t total_tests) {
  current_suite_name_ = suite_name;
  current_suite_results_.clear();
  current_suite_results_.reserve(total_tests);
  suite_start_time_ = std::chrono::steady_clock::now();
  progress_messages_.clear();
  error_messages_.clear();
  if (current_suite_results_.empty()) {
    write_json_header();
  }
}

void JsonReporter::on_suite_finished(const TestSuiteResult& result) {
  write_test_suite(result);
  write_json_footer();
  output_file_.flush();
}

void JsonReporter::on_test_started(const std::string& test_name) {
  // JSON reporter doesn't need to do anything when test starts
  (void)test_name;  // Suppress unused parameter warning
}

void JsonReporter::on_test_finished(const TestResult& result) {
  current_suite_results_.push_back(result);
}

void JsonReporter::on_progress(const std::string& message, double percentage) {
  if (!quiet_) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << percentage << "%: " << message;
    progress_messages_.push_back(oss.str());
  }
}

void JsonReporter::on_error(const std::string& error_message) {
  error_messages_.push_back(error_message);
}

void JsonReporter::write_json_header() {
  // Header will be written as part of the complete JSON object
  // We'll build the JSON incrementally and write at the end
}

void JsonReporter::write_json_footer() {
  // Footer is handled by write_test_suite which writes the complete JSON
}

void JsonReporter::write_test_suite(const TestSuiteResult& result) {
  auto suite_end_time = std::chrono::steady_clock::now();
  auto suite_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(suite_end_time - suite_start_time_);

  nlohmann::json j;
  j["format"] = "solar_test_json";
  j["version"] = "1.0";

  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestamp;
  timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
  j["timestamp"] = timestamp.str();

  // Build test suite JSON
  nlohmann::json suite_json;
  suite_json["name"] = result.suite_name;
  suite_json["total_tests"] = result.test_results.size();
  suite_json["passed"] = result.passed_count;
  suite_json["failed"] = result.failed_count;
  suite_json["skipped"] = result.skipped_count;

  if (config_.include_timing) {
    suite_json["duration_ms"] = suite_duration.count();
    suite_json["success_rate"] = result.success_rate();
  }

  if (result.code_coverage_percentage > 0.0) {
    suite_json["code_coverage"] = result.code_coverage_percentage;
  }

  // Build test results array
  nlohmann::json tests_array = nlohmann::json::array();
  for (const auto& test_result : result.test_results) {
    nlohmann::json test_json;
    test_json["name"] = test_result.test_name;
    test_json["status"] = status_to_string(test_result.status);

    if (!test_result.error_message.empty()) {
      test_json["error_message"] = test_result.error_message;
    }

    if (config_.include_timing) {
      test_json["execution_time_ms"] = test_result.execution_time.count();
    }

    if (config_.include_memory_usage && test_result.memory_usage_bytes > 0) {
      test_json["memory_usage_bytes"] = test_result.memory_usage_bytes;
    }

    if (!test_result.assertion_failures.empty()) {
      test_json["assertion_failures"] = test_result.assertion_failures;
    }

    if (test_result.was_expected_to_fail) {
      test_json["expected_to_fail"] = true;
      if (!test_result.expected_failure_reason.empty()) {
        test_json["expected_failure_reason"] = test_result.expected_failure_reason;
      }
    }

    if (config_.include_metadata && !test_result.metadata.empty()) {
      test_json["metadata"] = test_result.metadata;
    }

    tests_array.push_back(test_json);
  }
  suite_json["tests"] = tests_array;

  j["test_suites"] = nlohmann::json::array({suite_json});

  // Add progress messages if any
  if (!progress_messages_.empty()) {
    j["progress"] = progress_messages_;
  }

  // Add error messages if any
  if (!error_messages_.empty()) {
    j["errors"] = error_messages_;
  }

  // Write the complete JSON
  if (config_.pretty_print) {
    output_file_ << j.dump(2);
  } else {
    output_file_ << j.dump();
  }
}

void JsonReporter::write_test_result(const TestResult& result, bool is_last) {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
  (void)result;
  (void)is_last;
}

void JsonReporter::write_metadata_object(const std::map<std::string, std::string>& metadata) {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
  (void)metadata;
}

void JsonReporter::write_progress_array() {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
}

void JsonReporter::write_errors_array() {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
}

std::string JsonReporter::json_escape(const std::string& text) const {
  // nlohmann/json handles escaping automatically
  // This method is kept for interface compatibility
  return text;
}

std::string JsonReporter::status_to_string(TestResult::Status status) const {
  switch (status) {
    case TestResult::Status::Passed:
      return "passed";
    case TestResult::Status::Failed:
      return "failed";
    case TestResult::Status::Skipped:
      return "skipped";
    case TestResult::Status::Timeout:
      return "timeout";
    case TestResult::Status::Error:
      return "error";
    case TestResult::Status::ExpectedFailure:
      return "expected_failure";
    default:
      return "unknown";
  }
}

void JsonReporter::write_indented(const std::string& content, size_t indent_level) {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
  (void)content;
  (void)indent_level;
}

void JsonReporter::write_key_value(const std::string& key, const std::string& value, bool is_last,
                                   size_t indent_level) {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
  (void)key;
  (void)value;
  (void)is_last;
  (void)indent_level;
}

void JsonReporter::write_key_value(const std::string& key, int64_t value, bool is_last,
                                   size_t indent_level) {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
  (void)key;
  (void)value;
  (void)is_last;
  (void)indent_level;
}

void JsonReporter::write_key_value(const std::string& key, double value, bool is_last,
                                   size_t indent_level) {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
  (void)key;
  (void)value;
  (void)is_last;
  (void)indent_level;
}

void JsonReporter::write_key_value(const std::string& key, bool value, bool is_last,
                                   size_t indent_level) {
  // This method is no longer needed with nlohmann/json
  // Kept for interface compatibility
  (void)key;
  (void)value;
  (void)is_last;
  (void)indent_level;
}

}  // namespace SolarSystem::Testing
