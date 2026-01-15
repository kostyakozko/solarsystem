#include "solar_test/reporters/xml_reporter.hpp"

#include <iomanip>
#include <sstream>

namespace SolarSystem::Testing {

XmlReporter::XmlReporter(Configuration config) : config_(std::move(config)) {
  output_file_.open(config_.output_file);
  if (!output_file_.is_open()) {
    throw std::runtime_error("Failed to open XML output file: " + config_.output_file);
  }
}

XmlReporter::XmlReporter(const std::string& output_file)
    : XmlReporter(Configuration{output_file}) {}

void XmlReporter::on_suite_started(const std::string& suite_name, size_t total_tests) {
  if (quiet_) return;

  current_suite_name_ = suite_name;
  current_suite_results_.clear();
  current_suite_results_.reserve(total_tests);
  suite_start_time_ = std::chrono::steady_clock::now();

  if (current_suite_results_.empty()) {
    write_xml_header();
  }
}

void XmlReporter::on_suite_finished(const TestSuiteResult& result) {
  write_testsuite_element(result);

  // If this is the last suite, write footer
  // For now, we'll write the footer immediately since we don't track multiple suites
  write_xml_footer();
  output_file_.flush();
}

void XmlReporter::on_test_started(const std::string& test_name) {
  // XML reporter doesn't need to do anything when test starts
  (void)test_name;  // Suppress unused parameter warning
}

void XmlReporter::on_test_finished(const TestResult& result) {
  current_suite_results_.push_back(result);
}

void XmlReporter::on_progress(const std::string& message, double percentage) {
  // XML reporter doesn't output progress information
  (void)message;     // Suppress unused parameter warning
  (void)percentage;  // Suppress unused parameter warning
}

void XmlReporter::on_error(const std::string& error_message) {
  // Add error as a comment in XML
  output_file_ << "<!-- ERROR: " << xml_escape(error_message) << " -->\n";
}

void XmlReporter::write_xml_header() {
  output_file_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  output_file_ << "<testsuites>\n";
}

void XmlReporter::write_xml_footer() { output_file_ << "</testsuites>\n"; }

void XmlReporter::write_testsuite_element(const TestSuiteResult& result) {
  auto suite_end_time = std::chrono::steady_clock::now();
  auto suite_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(suite_end_time - suite_start_time_);

  write_indented("<testsuite", 1);
  output_file_ << " name=\"" << xml_escape(result.suite_name) << "\""
               << " tests=\"" << result.test_results.size() << "\""
               << " failures=\"" << result.failed_count << "\""
               << " errors=\"0\""  // We don't distinguish errors from failures in our model
               << " skipped=\"" << result.skipped_count << "\""
               << " time=\"" << format_duration_seconds(suite_duration) << "\""
               << ">\n";

  // Write individual test cases
  for (const auto& test_result : result.test_results) {
    write_testcase_element(test_result);
  }

  write_indented("</testsuite>\n", 1);
}

void XmlReporter::write_testcase_element(const TestResult& result) {
  write_indented("<testcase", 2);
  output_file_ << " name=\"" << xml_escape(result.test_name) << "\""
               << " classname=\"" << xml_escape(current_suite_name_) << "\""
               << " time=\"" << format_duration_seconds(result.execution_time) << "\"";

  // Check if test has failure, error, or is skipped
  bool has_content = false;

  if (result.status == TestResult::Status::Failed || result.status == TestResult::Status::Error ||
      result.status == TestResult::Status::Timeout) {
    output_file_ << ">\n";
    has_content = true;
    write_failure_element(result);
  } else if (result.status == TestResult::Status::Skipped) {
    output_file_ << ">\n";
    has_content = true;
    write_skipped_element(result);
  }

  // Add system output if available and configured
  if (config_.include_system_out && result.has_metadata("captured_output")) {
    if (!has_content) {
      output_file_ << ">\n";
      has_content = true;
    }
    write_system_out_element(result);
  }

  if (has_content) {
    write_indented("</testcase>\n", 2);
  } else {
    output_file_ << " />\n";
  }
}

void XmlReporter::write_failure_element(const TestResult& result) {
  std::string failure_type;
  switch (result.status) {
    case TestResult::Status::Failed:
      failure_type = "AssertionFailure";
      break;
    case TestResult::Status::Error:
      failure_type = "TestError";
      break;
    case TestResult::Status::Timeout:
      failure_type = "TestTimeout";
      break;
    default:
      failure_type = "UnknownFailure";
      break;
  }

  write_indented("<failure", 3);
  output_file_ << " type=\"" << failure_type << "\""
               << " message=\"" << xml_escape(result.error_message) << "\"";

  if (!result.assertion_failures.empty()) {
    output_file_ << ">\n";

    // Write assertion failures as CDATA
    write_indented("<![CDATA[\n", 4);
    for (const auto& failure : result.assertion_failures) {
      output_file_ << failure << "\n";
    }
    write_indented("]]>\n", 4);

    write_indented("</failure>\n", 3);
  } else {
    output_file_ << " />\n";
  }
}

void XmlReporter::write_error_element(const TestResult& result) {
  write_indented("<error", 3);
  output_file_ << " type=\"TestError\""
               << " message=\"" << xml_escape(result.error_message) << "\" />\n";
}

void XmlReporter::write_skipped_element(const TestResult& result) {
  write_indented("<skipped", 3);
  if (!result.error_message.empty()) {
    output_file_ << " message=\"" << xml_escape(result.error_message) << "\"";
  }
  output_file_ << " />\n";
}

void XmlReporter::write_system_out_element(const TestResult& result) {
  std::string output = result.get_metadata("captured_output");
  if (!output.empty()) {
    write_indented("<system-out>\n", 3);
    write_indented("<![CDATA[\n", 4);
    output_file_ << output;
    if (!output.empty() && output.back() != '\n') {
      output_file_ << "\n";
    }
    write_indented("]]>\n", 4);
    write_indented("</system-out>\n", 3);
  }
}

void XmlReporter::write_system_err_element(const TestResult& result) {
  // For now, we don't separate stdout and stderr in our test results
  // This could be enhanced in the future
  (void)result;  // Suppress unused parameter warning
}

std::string XmlReporter::xml_escape(const std::string& text) const {
  std::string escaped;
  escaped.reserve(static_cast<size_t>(static_cast<double>(text.length()) * 1.2));  // Reserve some extra space

  for (char c : text) {
    switch (c) {
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '&':
        escaped += "&amp;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&apos;";
        break;
      default:
        escaped += c;
        break;
    }
  }

  return escaped;
}

std::string XmlReporter::format_duration_seconds(std::chrono::milliseconds duration) const {
  double seconds = static_cast<double>(duration.count()) / 1000.0;
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3) << seconds;
  return oss.str();
}

void XmlReporter::write_indented(const std::string& content, size_t indent_level) {
  if (config_.pretty_print) {
    output_file_ << std::string(indent_level * 2, ' ');
  }
  output_file_ << content;
}

}  // namespace SolarSystem::Testing
