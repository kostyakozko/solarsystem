#pragma once

#include <fstream>
#include <string>

#include "test_reporter.hpp"

namespace SolarSystem::Testing {

/**
 * @brief JSON reporter for programmatic consumption
 */
class JsonReporter : public TestReporter {
 public:
  struct Configuration {
    std::string output_file = "test_results.json";
    bool pretty_print = true;
    bool include_metadata = true;
    bool include_timing = true;
    bool include_memory_usage = false;
  };

  explicit JsonReporter(Configuration config);
  explicit JsonReporter(const std::string& output_file);

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
  std::vector<std::string> progress_messages_;
  std::vector<std::string> error_messages_;

  // JSON generation methods
  void write_json_header();
  void write_json_footer();
  void write_test_suite(const TestSuiteResult& result);
  void write_test_result(const TestResult& result, bool is_last = false);
  void write_metadata_object(const std::map<std::string, std::string>& metadata);
  void write_progress_array();
  void write_errors_array();

  // JSON utility methods
  [[nodiscard]] std::string json_escape(const std::string& text) const;
  [[nodiscard]] std::string status_to_string(TestResult::Status status) const;
  void write_indented(const std::string& content, size_t indent_level = 0);
  void write_key_value(const std::string& key, const std::string& value, bool is_last = false,
                       size_t indent_level = 0);
  void write_key_value(const std::string& key, int64_t value, bool is_last = false,
                       size_t indent_level = 0);
  void write_key_value(const std::string& key, double value, bool is_last = false,
                       size_t indent_level = 0);
  void write_key_value(const std::string& key, bool value, bool is_last = false,
                       size_t indent_level = 0);
};

}  // namespace SolarSystem::Testing
