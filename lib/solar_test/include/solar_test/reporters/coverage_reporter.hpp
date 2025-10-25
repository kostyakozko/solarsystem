#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "test_reporter.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Coverage reporter for generating code coverage reports
 *
 * This is a basic coverage reporter that can be extended to integrate
 * with coverage tools like gcov, lcov, or llvm-cov.
 */
class CoverageReporter : public TestReporter {
 public:
  struct Configuration {
    std::string output_file = "coverage_report.txt";
    std::string format = "text";  // "text", "html", "lcov"
    bool include_line_coverage = true;
    bool include_function_coverage = true;
    bool include_branch_coverage = false;
    double minimum_coverage_threshold = 0.0;  // 0-100%
  };

  explicit CoverageReporter(Configuration config);
  explicit CoverageReporter(const std::string& output_file);

  // TestReporter interface implementation
  void on_suite_started(const std::string& suite_name, size_t total_tests) override;
  void on_suite_finished(const TestSuiteResult& result) override;
  void on_test_started(const std::string& test_name) override;
  void on_test_finished(const TestResult& result) override;
  void on_progress(const std::string& message, double percentage) override;
  void on_error(const std::string& error_message) override;

  // Coverage-specific methods
  void set_coverage_data(const std::map<std::string, double>& file_coverage);
  void add_file_coverage(const std::string& file_path, double coverage_percentage);
  [[nodiscard]] double overall_coverage() const;
  [[nodiscard]] bool meets_threshold() const;

  // Configuration
  void set_configuration(const Configuration& config) { config_ = config; }
  [[nodiscard]] const Configuration& configuration() const { return config_; }

 private:
  Configuration config_;
  std::ofstream output_file_;
  std::map<std::string, double> file_coverage_data_;
  double overall_coverage_percentage_ = 0.0;

  // Coverage report generation
  void generate_text_report();
  void generate_html_report();
  void generate_lcov_report();
  void write_coverage_summary();
  void write_file_coverage_details();

  // Utility methods
  [[nodiscard]] std::string format_percentage(double percentage) const;
  [[nodiscard]] std::string get_coverage_status(double percentage) const;
};

}  // namespace SolarSystem::Testing
