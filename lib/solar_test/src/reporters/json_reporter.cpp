#include "solar_test/reporters/json_reporter.hpp"

#include <iomanip>
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
  write_indented("{\n");
  write_key_value("format", "solar_test_json", false, 1);
  write_key_value("version", "1.0", false, 1);

  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestamp;
  timestamp << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
  write_key_value("timestamp", timestamp.str(), false, 1);

  write_indented("\"test_suites\": [\n", 1);
}

void JsonReporter::write_json_footer() {
  write_indented("\n", 1);
  write_indented("],\n", 1);

  // Write progress messages if any
  if (!progress_messages_.empty()) {
    write_progress_array();
    write_indented(",\n");
  }

  // Write error messages if any
  if (!error_messages_.empty()) {
    write_errors_array();
    write_indented(",\n");
  }

  // Remove trailing comma if present
  output_file_.seekp(-2, std::ios_base::cur);
  write_indented("\n");

  write_indented("}\n");
}

void JsonReporter::write_test_suite(const TestSuiteResult& result) {
  auto suite_end_time = std::chrono::steady_clock::now();
  auto suite_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(suite_end_time - suite_start_time_);

  write_indented("{\n", 2);
  write_key_value("name", result.suite_name, false, 3);
  write_key_value("total_tests", static_cast<int64_t>(result.test_results.size()), false, 3);
  write_key_value("passed", static_cast<int64_t>(result.passed_count), false, 3);
  write_key_value("failed", static_cast<int64_t>(result.failed_count), false, 3);
  write_key_value("skipped", static_cast<int64_t>(result.skipped_count), false, 3);

  if (config_.include_timing) {
    write_key_value("duration_ms", static_cast<int64_t>(suite_duration.count()), false, 3);
    write_key_value("success_rate", result.success_rate(), false, 3);
  }

  if (result.code_coverage_percentage > 0.0) {
    write_key_value("code_coverage", result.code_coverage_percentage, false, 3);
  }

  // Write individual test results
  write_indented("\"tests\": [\n", 3);
  for (size_t i = 0; i < result.test_results.size(); ++i) {
    write_test_result(result.test_results[i], i == result.test_results.size() - 1);
  }
  write_indented("]\n", 3);

  write_indented("}", 2);
}

void JsonReporter::write_test_result(const TestResult& result, bool is_last) {
  write_indented("{\n", 4);
  write_key_value("name", result.test_name, false, 5);
  write_key_value("status", status_to_string(result.status), false, 5);

  if (!result.error_message.empty()) {
    write_key_value("error_message", result.error_message, false, 5);
  }

  if (config_.include_timing) {
    write_key_value("execution_time_ms", static_cast<int64_t>(result.execution_time.count()), false,
                    5);
  }

  if (config_.include_memory_usage && result.memory_usage_bytes > 0) {
    write_key_value("memory_usage_bytes", static_cast<int64_t>(result.memory_usage_bytes), false,
                    5);
  }

  if (!result.assertion_failures.empty()) {
    write_indented("\"assertion_failures\": [\n", 5);
    for (size_t i = 0; i < result.assertion_failures.size(); ++i) {
      write_indented("\"" + json_escape(result.assertion_failures[i]) + "\"", 6);
      if (i < result.assertion_failures.size() - 1) {
        output_file_ << ",";
      }
      output_file_ << "\n";
    }
    write_indented("],\n", 5);
  }

  if (result.was_expected_to_fail) {
    write_key_value("expected_to_fail", true, false, 5);
    if (!result.expected_failure_reason.empty()) {
      write_key_value("expected_failure_reason", result.expected_failure_reason, false, 5);
    }
  }

  if (config_.include_metadata && !result.metadata.empty()) {
    write_indented("\"metadata\": ", 5);
    write_metadata_object(result.metadata);
    write_indented(",\n");
  }

  // Remove trailing comma
  output_file_.seekp(-2, std::ios_base::cur);
  write_indented("\n");

  write_indented("}", 4);
  if (!is_last) {
    output_file_ << ",";
  }
  output_file_ << "\n";
}

void JsonReporter::write_metadata_object(const std::map<std::string, std::string>& metadata) {
  output_file_ << "{\n";
  size_t count = 0;
  for (const auto& [key, value] : metadata) {
    write_key_value(key, value, count == metadata.size() - 1, 6);
    count++;
  }
  write_indented("}", 5);
}

void JsonReporter::write_progress_array() {
  write_indented("\"progress\": [\n", 1);
  for (size_t i = 0; i < progress_messages_.size(); ++i) {
    write_indented("\"" + json_escape(progress_messages_[i]) + "\"", 2);
    if (i < progress_messages_.size() - 1) {
      output_file_ << ",";
    }
    output_file_ << "\n";
  }
  write_indented("]", 1);
}

void JsonReporter::write_errors_array() {
  write_indented("\"errors\": [\n", 1);
  for (size_t i = 0; i < error_messages_.size(); ++i) {
    write_indented("\"" + json_escape(error_messages_[i]) + "\"", 2);
    if (i < error_messages_.size() - 1) {
      output_file_ << ",";
    }
    output_file_ << "\n";
  }
  write_indented("]", 1);
}

std::string JsonReporter::json_escape(const std::string& text) const {
  std::string escaped;
  escaped.reserve(text.length() * 1.2);  // Reserve some extra space

  for (char c : text) {
    switch (c) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          // Control characters
          std::ostringstream oss;
          oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<unsigned int>(static_cast<unsigned char>(c));
          escaped += oss.str();
        } else {
          escaped += c;
        }
        break;
    }
  }

  return escaped;
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
  if (config_.pretty_print && indent_level > 0) {
    output_file_ << std::string(indent_level * 2, ' ');
  }
  output_file_ << content;
}

void JsonReporter::write_key_value(const std::string& key, const std::string& value, bool is_last,
                                   size_t indent_level) {
  write_indented("\"" + key + "\": \"" + json_escape(value) + "\"", indent_level);
  if (!is_last) {
    output_file_ << ",";
  }
  output_file_ << "\n";
}

void JsonReporter::write_key_value(const std::string& key, int64_t value, bool is_last,
                                   size_t indent_level) {
  write_indented("\"" + key + "\": " + std::to_string(value), indent_level);
  if (!is_last) {
    output_file_ << ",";
  }
  output_file_ << "\n";
}

void JsonReporter::write_key_value(const std::string& key, double value, bool is_last,
                                   size_t indent_level) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << value;
  write_indented("\"" + key + "\": " + oss.str(), indent_level);
  if (!is_last) {
    output_file_ << ",";
  }
  output_file_ << "\n";
}

void JsonReporter::write_key_value(const std::string& key, bool value, bool is_last,
                                   size_t indent_level) {
  write_indented("\"" + key + "\": " + (value ? "true" : "false"), indent_level);
  if (!is_last) {
    output_file_ << ",";
  }
  output_file_ << "\n";
}

}  // namespace SolarSystem::Testing
