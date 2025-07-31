#include "solar_test/reporters/tap_reporter.hpp"

#include <iostream>
#include <sstream>

namespace SolarSystem::Testing {

TapReporter::TapReporter(Configuration config) : config_(std::move(config)) {
  if (config_.output_file.empty()) {
    output_stream_ = &std::cout;
  } else {
    output_file_.open(config_.output_file);
    if (!output_file_.is_open()) {
      throw std::runtime_error("Failed to open TAP output file: " + config_.output_file);
    }
    output_stream_ = &output_file_;
  }
}

TapReporter::TapReporter(const std::string& output_file)
    : TapReporter(Configuration{output_file}) {}

void TapReporter::on_suite_started(const std::string& suite_name, size_t total_tests) {
  test_count_ = total_tests;
  current_test_number_ = 0;
  plan_written_ = false;

  write_version();

  if (config_.include_diagnostics) {
    write_diagnostic("Starting test suite: " + suite_name);
  }

  write_plan(total_tests);
}

void TapReporter::on_suite_finished(const TestSuiteResult& result) {
  if (config_.include_diagnostics) {
    write_diagnostic("Test suite completed");
    write_diagnostic("Passed: " + std::to_string(result.passed_count));
    write_diagnostic("Failed: " + std::to_string(result.failed_count));
    write_diagnostic("Skipped: " + std::to_string(result.skipped_count));

    if (result.code_coverage_percentage > 0.0) {
      std::ostringstream oss;
      oss << "Coverage: " << std::fixed << std::setprecision(1) << result.code_coverage_percentage
          << "%";
      write_diagnostic(oss.str());
    }
  }

  if (output_stream_ != &std::cout) {
    output_file_.flush();
  }
}

void TapReporter::on_test_started(const std::string& test_name) {
  // TAP doesn't require notification when test starts
  (void)test_name;  // Suppress unused parameter warning
}

void TapReporter::on_test_finished(const TestResult& result) {
  current_test_number_++;
  write_test_line(result);

  if (config_.include_yaml_diagnostics && !result.passed()) {
    write_yaml_diagnostic(result);
  }
}

void TapReporter::on_progress(const std::string& message, double percentage) {
  if (config_.include_diagnostics && !quiet_) {
    std::ostringstream oss;
    oss << "Progress: " << std::fixed << std::setprecision(1) << percentage << "% - " << message;
    write_diagnostic(oss.str());
  }
}

void TapReporter::on_error(const std::string& error_message) {
  write_diagnostic("ERROR: " + error_message);
}

void TapReporter::write_version() { *output_stream_ << "TAP version 13\n"; }

void TapReporter::write_plan(size_t total_tests) {
  *output_stream_ << "1.." << total_tests << "\n";
  plan_written_ = true;
}

void TapReporter::write_test_line(const TestResult& result) {
  std::string status;
  std::string directive;

  switch (result.status) {
    case TestResult::Status::Passed:
      status = "ok";
      break;
    case TestResult::Status::Failed:
    case TestResult::Status::Error:
    case TestResult::Status::Timeout:
      status = "not ok";
      break;
    case TestResult::Status::Skipped:
      status = "ok";
      directive = " # SKIP";
      if (!result.error_message.empty()) {
        directive += " " + result.error_message;
      }
      break;
    case TestResult::Status::ExpectedFailure:
      status = "not ok";
      directive = " # TODO";
      if (!result.expected_failure_reason.empty()) {
        directive += " " + result.expected_failure_reason;
      }
      break;
  }

  *output_stream_ << status << " " << current_test_number_ << " "
                  << format_test_description(result.test_name) << directive << "\n";

  // Add diagnostic information for failures
  if (!result.passed() && config_.include_diagnostics) {
    if (!result.error_message.empty()) {
      write_diagnostic("Error: " + result.error_message);
    }

    for (const auto& failure : result.assertion_failures) {
      write_diagnostic("Assertion failure: " + failure);
    }

    if (config_.include_timing && result.execution_time.count() > 0) {
      write_diagnostic("Execution time: " + std::to_string(result.execution_time.count()) + "ms");
    }
  }
}

void TapReporter::write_diagnostic(const std::string& message) {
  // TAP diagnostic lines start with #
  std::istringstream iss(message);
  std::string line;
  while (std::getline(iss, line)) {
    *output_stream_ << "# " << line << "\n";
  }
}

void TapReporter::write_yaml_diagnostic(const TestResult& result) {
  *output_stream_ << "  ---\n";
  *output_stream_ << "  message: \"" << escape_tap_string(result.error_message) << "\"\n";
  *output_stream_ << "  severity: "
                  << (result.status == TestResult::Status::Failed ? "fail" : "error") << "\n";

  if (!result.assertion_failures.empty()) {
    *output_stream_ << "  data:\n";
    *output_stream_ << "    failures:\n";
    for (const auto& failure : result.assertion_failures) {
      *output_stream_ << "      - \"" << escape_tap_string(failure) << "\"\n";
    }
  }

  if (config_.include_timing && result.execution_time.count() > 0) {
    *output_stream_ << "    execution_time_ms: " << result.execution_time.count() << "\n";
  }

  if (result.memory_usage_bytes > 0) {
    *output_stream_ << "    memory_usage_bytes: " << result.memory_usage_bytes << "\n";
  }

  if (!result.metadata.empty()) {
    *output_stream_ << "    metadata:\n";
    for (const auto& [key, value] : result.metadata) {
      *output_stream_ << "      " << key << ": \"" << escape_tap_string(value) << "\"\n";
    }
  }

  *output_stream_ << "  ...\n";
}

void TapReporter::write_bail_out(const std::string& reason) {
  *output_stream_ << "Bail out! " << reason << "\n";
}

std::string TapReporter::escape_tap_string(const std::string& text) const {
  std::string escaped;
  escaped.reserve(text.length() * 1.1);

  for (char c : text) {
    switch (c) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
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
        escaped += c;
        break;
    }
  }

  return escaped;
}

std::string TapReporter::format_test_description(const std::string& test_name) const {
  // TAP test descriptions should not contain certain characters
  std::string formatted = test_name;

  // Replace problematic characters
  for (char& c : formatted) {
    if (c == '\n' || c == '\r') {
      c = ' ';
    }
  }

  return formatted;
}

}  // namespace SolarSystem::Testing
