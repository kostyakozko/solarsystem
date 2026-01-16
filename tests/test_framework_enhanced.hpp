#pragma once

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace SolarSystem::Testing {

/**
 * @brief Test result with basic metrics
 */
struct TestResult {
  std::string test_name;
  bool passed = false;
  std::string error_message;
  std::chrono::nanoseconds execution_time{0};
  std::chrono::system_clock::time_point timestamp;

  TestResult(const std::string& name)
      : test_name(name), timestamp(std::chrono::system_clock::now()) {}

  [[nodiscard]] double execution_time_ms() const {
    return std::chrono::duration<double, std::milli>(execution_time).count();
  }
};

/**
 * @brief Enhanced test framework with performance monitoring
 */
class EnhancedTestFramework {
 public:
  static EnhancedTestFramework& instance() {
    static EnhancedTestFramework framework;
    return framework;
  }

  void run_test(const std::string& test_name, std::function<void()> test_function) {
    TestResult result(test_name);

    std::cout << "🧪 Running: " << test_name << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
      // Run the test
      test_function();
      result.passed = true;
      std::cout << "✅ " << test_name << " PASSED" << std::endl;
    } catch (const std::exception& e) {
      result.passed = false;
      result.error_message = e.what();
      std::cout << "❌ " << test_name << " FAILED: " << e.what() << std::endl;
    } catch (...) {
      result.passed = false;
      result.error_message = "Unknown exception";
      std::cout << "❌ " << test_name << " FAILED: Unknown exception" << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.execution_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

    // Store result
    test_results_.push_back(result);

    // Print metrics
    std::cout << "  ⏱️  Time: " << std::fixed << std::setprecision(2) << result.execution_time_ms()
              << " ms" << std::endl;
    std::cout << std::endl;
  }

  void run_test_suite(const std::string& suite_name,
                      const std::vector<std::pair<std::string, std::function<void()>>>& tests) {
    std::cout << std::endl << "🚀 Running Test Suite: " << suite_name << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    auto suite_start = std::chrono::high_resolution_clock::now();

    for (const auto& [test_name, test_function] : tests) {
      run_test(suite_name + "::" + test_name, test_function);
    }

    auto suite_end = std::chrono::high_resolution_clock::now();
    auto suite_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(suite_end - suite_start);

    print_suite_summary(suite_name, suite_duration);
  }

  void generate_report(const std::string& filename = "test_report.html") const {
    std::ofstream report(filename);
    if (!report.is_open()) {
      std::cerr << "Failed to create report file: " << filename << std::endl;
      return;
    }

    generate_html_report(report);
    std::cout << "📊 Test report generated: " << filename << std::endl;
  }

  void load_performance_baselines(const std::string& filename) {
    // Placeholder for baseline loading
    std::cout << "📈 Loading baselines from: " << filename << std::endl;
  }

  void save_performance_baselines(const std::string& filename) const {
    // Placeholder for baseline saving
    std::cout << "💾 Saving baselines to: " << filename << std::endl;
  }

  [[nodiscard]] size_t get_total_tests() const { return test_results_.size(); }
  [[nodiscard]] size_t get_passed_tests() const {
    return static_cast<size_t>(std::count_if(test_results_.begin(), test_results_.end(),
                                             [](const TestResult& r) { return r.passed; }));
  }
  [[nodiscard]] size_t get_failed_tests() const { return get_total_tests() - get_passed_tests(); }

 private:
  std::vector<TestResult> test_results_;

  void print_suite_summary(const std::string& suite_name,
                           std::chrono::milliseconds duration) const {
    size_t suite_tests = 0;
    size_t suite_passed = 0;

    for (const auto& result : test_results_) {
      if (result.test_name.find(suite_name + "::") == 0) {
        suite_tests++;
        if (result.passed) suite_passed++;
      }
    }

    std::cout << std::endl << "📊 Suite Summary: " << suite_name << std::endl;
    std::cout << "Total tests: " << suite_tests << std::endl;
    std::cout << "Passed: " << suite_passed << std::endl;
    std::cout << "Failed: " << (suite_tests - suite_passed) << std::endl;
    std::cout << "Duration: " << duration.count() << " ms" << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1)
              << (suite_tests > 0 ? (suite_passed * 100.0 / suite_tests) : 0.0) << "%" << std::endl;
    std::cout << std::string(50, '=') << std::endl << std::endl;
  }

  void generate_html_report(std::ofstream& report) const {
    report << R"(<!DOCTYPE html>
<html>
<head>
    <title>Solar System Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background: #2c3e50; color: white; padding: 20px; border-radius: 5px; }
        .summary { background: #ecf0f1; padding: 15px; margin: 20px 0; border-radius: 5px; }
        .test-result { margin: 10px 0; padding: 10px; border-radius: 5px; }
        .passed { background: #d5f4e6; border-left: 4px solid #27ae60; }
        .failed { background: #fadbd8; border-left: 4px solid #e74c3c; }
        .metrics { font-size: 0.9em; color: #666; }
    </style>
</head>
<body>
)";

    // Header
    report << "<div class='header'>" << std::endl;
    report << "<h1>🚀 Solar System Test Report</h1>" << std::endl;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    report << "<p>Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
           << "</p>" << std::endl;
    report << "</div>" << std::endl;

    // Summary
    report << "<div class='summary'>" << std::endl;
    report << "<h2>📊 Summary</h2>" << std::endl;
    report << "<p><strong>Total Tests:</strong> " << get_total_tests() << "</p>" << std::endl;
    report << "<p><strong>Passed:</strong> " << get_passed_tests() << "</p>" << std::endl;
    report << "<p><strong>Failed:</strong> " << get_failed_tests() << "</p>" << std::endl;
    report << "<p><strong>Success Rate:</strong> " << std::fixed << std::setprecision(1)
           << (get_total_tests() > 0 ? (get_passed_tests() * 100.0 / get_total_tests()) : 0.0)
           << "%</p>" << std::endl;
    report << "</div>" << std::endl;

    // Test Results
    report << "<h2>🧪 Test Results</h2>" << std::endl;
    for (const auto& result : test_results_) {
      std::string css_class = result.passed ? "passed" : "failed";

      report << "<div class='test-result " << css_class << "'>" << std::endl;
      report << "<h3>" << (result.passed ? "✅" : "❌") << " " << result.test_name << "</h3>"
             << std::endl;

      if (!result.error_message.empty()) {
        report << "<p><strong>Error:</strong> " << result.error_message << "</p>" << std::endl;
      }

      report << "<div class='metrics'>" << std::endl;
      report << "<p><strong>⏱️ Execution Time:</strong> " << std::fixed << std::setprecision(2)
             << result.execution_time_ms() << " ms</p>" << std::endl;
      report << "</div>" << std::endl;
      report << "</div>" << std::endl;
    }

    report << "</body>" << std::endl << "</html>" << std::endl;
  }
};

// Convenience macros for testing
#define ENHANCED_TEST(name, code) EnhancedTestFramework::instance().run_test(name, [&]() { code; })

#define ENHANCED_TEST_SUITE(suite_name, tests) \
  EnhancedTestFramework::instance().run_test_suite(suite_name, tests)

#define ASSERT_TRUE(condition)                                 \
  if (!(condition)) {                                          \
    throw std::runtime_error("Assertion failed: " #condition); \
  }

#define ASSERT_FALSE(condition)                                 \
  if (condition) {                                              \
    throw std::runtime_error("Assertion failed: !" #condition); \
  }

#define ASSERT_EQ(expected, actual)                                          \
  if ((expected) != (actual)) {                                              \
    throw std::runtime_error("Assertion failed: " #expected " == " #actual); \
  }

#define ASSERT_NE(expected, actual)                                          \
  if ((expected) == (actual)) {                                              \
    throw std::runtime_error("Assertion failed: " #expected " != " #actual); \
  }

}  // namespace SolarSystem::Testing
