#pragma once

#include <chrono>
#include <iostream>
#include <string>

#include "test_reporter.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Console-based test reporter with colorized output and progress indicators
 */
class ConsoleReporter : public TestReporter {
 public:
  struct Configuration {
    bool colorized = true;
    bool show_progress = true;
    bool show_stack_traces = true;
    bool show_timing = true;
    bool show_memory_usage = false;
    std::ostream* output_stream = &std::cout;
    std::ostream* error_stream = &std::cerr;
  };

  explicit ConsoleReporter(Configuration config);
  ConsoleReporter();

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
  size_t current_test_index_ = 0;
  size_t total_tests_ = 0;
  std::chrono::steady_clock::time_point suite_start_time_;
  std::chrono::steady_clock::time_point test_start_time_;

  // Color codes for terminal output
  struct Colors {
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* DIM = "\033[2m";
  };

  // Output formatting methods
  void print_header(const std::string& title);
  void print_separator(char character = '=', size_t length = 80);
  void print_progress_bar(double percentage, size_t width = 50);
  void print_test_status(const TestResult& result);
  void print_failure_details(const TestResult& result);
  void print_summary_statistics(const TestSuiteResult& result);
  void print_execution_time(std::chrono::milliseconds duration);
  void print_memory_usage(size_t bytes);

  // Color formatting
  [[nodiscard]] std::string colorize(const std::string& text, const char* color) const;
  [[nodiscard]] const char* status_color(TestResult::Status status) const;
  [[nodiscard]] std::string format_duration(std::chrono::milliseconds duration) const;
  [[nodiscard]] std::string format_memory(size_t bytes) const;

  // Progress indicators
  void show_spinner();
  void clear_line();
  void move_cursor_up(size_t lines = 1);

  // Stack trace formatting
  void print_stack_trace(const std::vector<std::string>& assertion_failures);
  [[nodiscard]] std::string format_assertion_failure(const std::string& failure) const;
};

}  // namespace SolarSystem::Testing
