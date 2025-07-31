#pragma once

#include <fstream>
#include <string>

#include "test_reporter.hpp"

namespace SolarSystem::Testing {

/**
 * @brief XML reporter for CI integration using JUnit format
 */
class XmlReporter : public TestReporter {
 public:
  struct Configuration {
    std::string output_file = "test_results.xml";
    bool pretty_print = true;
    bool include_system_out = true;
    bool include_system_err = true;
  };

  explicit XmlReporter(Configuration config);
  explicit XmlReporter(const std::string& output_file);

  // TestReporter interface implementation
  void on_suite_started(const std::string& suite_name, size_t total_tests) override;
  void on_suite_finished(const TestSuiteResult& result) override;
  void on_test_started(const std::string& test_name) override;
  void on_test_finished(const TestResult& result) override;
  void on_progress(const std::string& message, double percentage) override;
  void on_error(const std::string& error_message) override;

  // Configuration
  void set_configuration(const Configuration& config) { config_ = config; }
  [[nodiscard]] const Configuration& configuration() const { return config_; }

 private:
  Configuration config_;
  std::ofstream output_file_;
  std::string current_suite_name_;
  std::vector<TestResult> current_suite_results_;
  std::chrono::steady_clock::time_point suite_start_time_;

  // XML generation methods
  void write_xml_header();
  void write_xml_footer();
  void write_testsuite_element(const TestSuiteResult& result);
  void write_testcase_element(const TestResult& result);
  void write_failure_element(const TestResult& result);
  void write_error_element(const TestResult& result);
  void write_skipped_element(const TestResult& result);
  void write_system_out_element(const TestResult& result);
  void write_system_err_element(const TestResult& result);

  // XML utility methods
  [[nodiscard]] std::string xml_escape(const std::string& text) const;
  [[nodiscard]] std::string format_duration_seconds(std::chrono::milliseconds duration) const;
  void write_indented(const std::string& content, size_t indent_level = 0);
};

}  // namespace SolarSystem::Testing
