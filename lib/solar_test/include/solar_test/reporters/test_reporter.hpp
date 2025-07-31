#pragma once

#include <memory>
#include <string>

#include "../framework/test_result.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Base interface for test result reporting
 */
class TestReporter {
 public:
  virtual ~TestReporter() = default;

  // Test suite lifecycle
  virtual void on_suite_started(const std::string& suite_name, size_t total_tests) = 0;
  virtual void on_suite_finished(const TestSuiteResult& result) = 0;

  // Individual test lifecycle
  virtual void on_test_started(const std::string& test_name) = 0;
  virtual void on_test_finished(const TestResult& result) = 0;

  // Progress reporting
  virtual void on_progress(const std::string& message, double percentage) = 0;

  // Error reporting
  virtual void on_error(const std::string& error_message) = 0;

  // Configuration
  virtual void set_verbose(bool verbose) { verbose_ = verbose; }
  virtual void set_quiet(bool quiet) { quiet_ = quiet; }
  [[nodiscard]] virtual bool is_verbose() const { return verbose_; }
  [[nodiscard]] virtual bool is_quiet() const { return quiet_; }

 protected:
  bool verbose_ = false;
  bool quiet_ = false;
};

/**
 * @brief Factory for creating test reporters
 */
class TestReporterFactory {
 public:
  static std::unique_ptr<TestReporter> create_console_reporter(bool colorized = true);
  static std::unique_ptr<TestReporter> create_xml_reporter(const std::string& output_file);
  static std::unique_ptr<TestReporter> create_json_reporter(const std::string& output_file);
  static std::unique_ptr<TestReporter> create_tap_reporter(const std::string& output_file = "");
  static std::unique_ptr<TestReporter> create_coverage_reporter(const std::string& output_file);
};

}  // namespace SolarSystem::Testing
