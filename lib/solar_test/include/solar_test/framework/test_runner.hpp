#pragma once

#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "test_case.hpp"
#include "test_discovery.hpp"
#include "test_result.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Main test runner for executing test suites
 */
class TestRunner {
 public:
  struct Configuration {
    std::vector<std::string> test_patterns;
    std::vector<std::string> tags;
    bool parallel_execution = true;
    size_t max_threads = std::thread::hardware_concurrency();
    std::chrono::milliseconds timeout = std::chrono::minutes(5);
    bool generate_coverage = false;
    std::string output_format = "console";
    std::string output_file;
    bool verbose = false;
    bool quiet = false;
  };

  explicit TestRunner(Configuration config);

  // Test registration
  void register_test(std::unique_ptr<TestCase> test_case);
  void register_test_suite(const std::string& suite_name,
                           std::vector<std::unique_ptr<TestCase>> test_cases);

  // Test discovery and execution
  [[nodiscard]] TestSuiteResult run_all_tests();
  [[nodiscard]] TestSuiteResult run_tests_with_tag(const std::string& tag);
  [[nodiscard]] TestSuiteResult run_specific_test(const std::string& test_name);
  [[nodiscard]] TestSuiteResult run_tests_matching_pattern(const std::string& pattern);

  // Configuration
  void set_configuration(const Configuration& config) { config_ = config; }
  [[nodiscard]] const Configuration& configuration() const { return config_; }

  // Progress callbacks
  void set_progress_callback(std::function<void(const std::string&, double)> callback);
  void set_test_started_callback(std::function<void(const std::string&)> callback);
  void set_test_completed_callback(std::function<void(const TestResult&)> callback);

  // Statistics
  [[nodiscard]] size_t total_test_count() const;
  [[nodiscard]] std::vector<std::string> available_tags() const;
  [[nodiscard]] std::vector<std::string> available_test_names() const;

 private:
  Configuration config_;
  std::vector<std::unique_ptr<TestCase>> registered_tests_;
  std::map<std::string, std::vector<std::unique_ptr<TestCase>>> test_suites_;

  // Callbacks
  std::function<void(const std::string&, double)> progress_callback_;
  std::function<void(const std::string&)> test_started_callback_;
  std::function<void(const TestResult&)> test_completed_callback_;

  // Internal execution methods
  [[nodiscard]] std::vector<TestCase*> filter_tests(const std::vector<std::string>& patterns,
                                                    const std::vector<std::string>& tags) const;
  [[nodiscard]] TestSuiteResult execute_tests(const std::vector<TestCase*>& tests);
  [[nodiscard]] TestSuiteResult execute_tests_sequential(const std::vector<TestCase*>& tests);
  [[nodiscard]] TestSuiteResult execute_tests_parallel(const std::vector<TestCase*>& tests);

  // Utility methods
  [[nodiscard]] bool matches_pattern(const std::string& test_name,
                                     const std::string& pattern) const;
  [[nodiscard]] bool has_tag(const TestCase& test_case, const std::string& tag) const;
  void notify_progress(const std::string& message, double percentage);
  void notify_test_started(const std::string& test_name);
  void notify_test_completed(const TestResult& result);
};

/**
 * @brief Test registry for automatic test registration
 */
class TestRegistry {
 public:
  static TestRegistry& instance();

  void register_test(std::unique_ptr<TestCase> test_case);
  void register_test_factory(const std::string& name,
                             std::function<std::unique_ptr<TestCase>()> factory);

  [[nodiscard]] std::vector<std::unique_ptr<TestCase>> create_all_tests() const;
  [[nodiscard]] std::unique_ptr<TestCase> create_test(const std::string& name) const;

 private:
  TestRegistry() = default;
  std::map<std::string, std::function<std::unique_ptr<TestCase>()>> test_factories_;
};

// Macro for automatic test registration (using TestDiscovery)
#define REGISTER_TEST(TestCaseType)                                                       \
  static SolarSystem::Testing::TestRegistrar<TestCaseType> test_registrar_##TestCaseType( \
      #TestCaseType)

}  // namespace SolarSystem::Testing
