#pragma once

#include <fstream>
#include <string>

#include "test_reporter.hpp"

namespace SolarSystem::Testing {

/**
 * @brief TAP (Test Anything Protocol) reporter
 *
 * Implements TAP version 13 specification for compatibility with TAP consumers
 * like prove, Jenkins TAP plugin, etc.
 */
class TapReporter : public TestReporter {
 public:
  struct Configuration {
    std::string output_file = "";  // Empty means stdout
    bool include_diagnostics = true;
    bool include_timing = false;
    bool include_yaml_diagnostics = false;
  };

  explicit TapReporter(Configuration config);
  explicit TapReporter(const std::string& output_file = "");

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
  std::ostream* output_stream_;
  size_t test_count_ = 0;
  size_t current_test_number_ = 0;
  bool plan_written_ = false;

  // TAP output methods
  void write_version();
  void write_plan(size_t total_tests);
  void write_test_line(const TestResult& result);
  void write_diagnostic(const std::string& message);
  void write_yaml_diagnostic(const TestResult& result);
  void write_bail_out(const std::string& reason);

  // TAP utility methods
  [[nodiscard]] std::string escape_tap_string(const std::string& text) const;
  [[nodiscard]] std::string format_test_description(const std::string& test_name) const;
};

}  // namespace SolarSystem::Testing
