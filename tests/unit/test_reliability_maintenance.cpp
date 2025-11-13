/**
 * @file test_reliability_maintenance.cpp
 * @brief Test reliability and maintenance system (Task 31)
 *
 * Tests reliability and maintenance capabilities:
 * - Flaky test detection and resolution
 * - Test execution monitoring and analysis
 * - Test performance optimization and tuning
 * - Test suite maintenance and cleanup
 *
 * Requirements: 10.5
 */

#include <algorithm>
#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "test_framework.h"

/**
 * @brief Flaky test detector
 */
class FlakyTestDetector {
 public:
  struct TestExecution {
    std::string test_name;
    bool passed;
    std::chrono::milliseconds duration;
    std::chrono::system_clock::time_point timestamp;
  };

  void record_execution(const TestExecution& execution) {
    test_history_[execution.test_name].push_back(execution);
  }

  bool is_flaky(const std::string& test_name, int min_runs = 5) {
    if (test_history_.find(test_name) == test_history_.end()) return false;

    const auto& history = test_history_[test_name];
    if (history.size() < static_cast<size_t>(min_runs)) return false;

    // Check if test has both passes and failures
    bool has_pass = false;
    bool has_fail = false;

    for (const auto& exec : history) {
      if (exec.passed)
        has_pass = true;
      else
        has_fail = true;
    }

    return has_pass && has_fail;
  }

  double get_pass_rate(const std::string& test_name) {
    if (test_history_.find(test_name) == test_history_.end()) return 0.0;

    const auto& history = test_history_[test_name];
    if (history.empty()) return 0.0;

    int passed = 0;
    for (const auto& exec : history) {
      if (exec.passed) passed++;
    }

    return static_cast<double>(passed) / history.size() * 100.0;
  }

  std::vector<std::string> get_flaky_tests() {
    std::vector<std::string> flaky;
    for (const auto& [test_name, history] : test_history_) {
      if (is_flaky(test_name)) {
        flaky.push_back(test_name);
      }
    }
    return flaky;
  }

 private:
  std::map<std::string, std::vector<TestExecution>> test_history_;
};

/**
 * @brief Test execution monitor
 */
class TestExecutionMonitor {
 public:
  struct ExecutionMetrics {
    int total_runs = 0;
    int successful_runs = 0;
    int failed_runs = 0;
    std::chrono::milliseconds total_duration{0};
    std::chrono::milliseconds avg_duration{0};
    std::chrono::milliseconds min_duration{999999};
    std::chrono::milliseconds max_duration{0};
  };

  void record_run(const std::string& test_name, bool success,
                 std::chrono::milliseconds duration) {
    auto& metrics = test_metrics_[test_name];
    metrics.total_runs++;

    if (success)
      metrics.successful_runs++;
    else
      metrics.failed_runs++;

    metrics.total_duration += duration;
    metrics.avg_duration =
        std::chrono::milliseconds(metrics.total_duration.count() / metrics.total_runs);

    if (duration < metrics.min_duration) metrics.min_duration = duration;
    if (duration > metrics.max_duration) metrics.max_duration = duration;
  }

  ExecutionMetrics get_metrics(const std::string& test_name) {
    if (test_metrics_.find(test_name) == test_metrics_.end()) {
      return ExecutionMetrics();
    }
    return test_metrics_[test_name];
  }

  std::vector<std::string> get_slow_tests(std::chrono::milliseconds threshold) {
    std::vector<std::string> slow_tests;
    for (const auto& [test_name, metrics] : test_metrics_) {
      if (metrics.avg_duration > threshold) {
        slow_tests.push_back(test_name);
      }
    }
    return slow_tests;
  }

 private:
  std::map<std::string, ExecutionMetrics> test_metrics_;
};

/**
 * @brief Test performance optimizer
 */
class TestPerformanceOptimizer {
 public:
  struct OptimizationSuggestion {
    std::string test_name;
    std::string suggestion;
    int priority;  // 1-5, 5 being highest
  };

  std::vector<OptimizationSuggestion> analyze_test(const std::string& test_name,
                                                   std::chrono::milliseconds avg_duration) {
    std::vector<OptimizationSuggestion> suggestions;

    // Suggest optimization if test is slow
    if (avg_duration > std::chrono::milliseconds(1000)) {
      OptimizationSuggestion suggestion;
      suggestion.test_name = test_name;
      suggestion.suggestion = "Consider parallelizing or reducing test scope";
      suggestion.priority = 5;
      suggestions.push_back(suggestion);
    } else if (avg_duration > std::chrono::milliseconds(100)) {
      OptimizationSuggestion suggestion;
      suggestion.test_name = test_name;
      suggestion.suggestion = "Review for unnecessary operations";
      suggestion.priority = 3;
      suggestions.push_back(suggestion);
    }

    return suggestions;
  }

  std::vector<OptimizationSuggestion> get_all_suggestions(
      const std::map<std::string, std::chrono::milliseconds>& test_durations) {
    std::vector<OptimizationSuggestion> all_suggestions;

    for (const auto& [test_name, duration] : test_durations) {
      auto suggestions = analyze_test(test_name, duration);
      all_suggestions.insert(all_suggestions.end(), suggestions.begin(), suggestions.end());
    }

    // Sort by priority
    std::sort(all_suggestions.begin(), all_suggestions.end(),
             [](const OptimizationSuggestion& a, const OptimizationSuggestion& b) {
               return a.priority > b.priority;
             });

    return all_suggestions;
  }
};

/**
 * @brief Test suite maintenance manager
 */
class TestSuiteMaintenanceManager {
 public:
  struct MaintenanceReport {
    int total_tests = 0;
    int deprecated_tests = 0;
    int duplicate_tests = 0;
    int outdated_tests = 0;
    std::vector<std::string> tests_to_remove;
    std::vector<std::string> tests_to_update;
  };

  void mark_deprecated(const std::string& test_name) { deprecated_tests_.insert(test_name); }

  void mark_duplicate(const std::string& test_name) { duplicate_tests_.insert(test_name); }

  void mark_outdated(const std::string& test_name) { outdated_tests_.insert(test_name); }

  MaintenanceReport generate_report(const std::vector<std::string>& all_tests) {
    MaintenanceReport report;
    report.total_tests = static_cast<int>(all_tests.size());
    report.deprecated_tests = static_cast<int>(deprecated_tests_.size());
    report.duplicate_tests = static_cast<int>(duplicate_tests_.size());
    report.outdated_tests = static_cast<int>(outdated_tests_.size());

    // Tests to remove (deprecated or duplicate)
    for (const auto& test : deprecated_tests_) {
      report.tests_to_remove.push_back(test);
    }
    for (const auto& test : duplicate_tests_) {
      report.tests_to_remove.push_back(test);
    }

    // Tests to update (outdated)
    for (const auto& test : outdated_tests_) {
      report.tests_to_update.push_back(test);
    }

    return report;
  }

  void cleanup_test(const std::string& test_name) {
    deprecated_tests_.erase(test_name);
    duplicate_tests_.erase(test_name);
    outdated_tests_.erase(test_name);
  }

 private:
  std::set<std::string> deprecated_tests_;
  std::set<std::string> duplicate_tests_;
  std::set<std::string> outdated_tests_;
};

int main() {
  TEST_SUITE("Test Reliability and Maintenance Tests");

  // Test 1: Flaky test detection
  TEST_CASE("Flaky Test Detection") {
    FlakyTestDetector detector;

    // Test 1.1: Record stable test executions
    for (int i = 0; i < 10; ++i) {
      FlakyTestDetector::TestExecution exec;
      exec.test_name = "stable_test";
      exec.passed = true;
      exec.duration = std::chrono::milliseconds(10);
      detector.record_execution(exec);
    }

    ASSERT_FALSE(detector.is_flaky("stable_test"));
    ASSERT_EQ(detector.get_pass_rate("stable_test"), 100.0);

    // Test 1.2: Record flaky test executions
    for (int i = 0; i < 10; ++i) {
      FlakyTestDetector::TestExecution exec;
      exec.test_name = "flaky_test";
      exec.passed = (i % 2 == 0);  // Alternates between pass and fail
      exec.duration = std::chrono::milliseconds(10);
      detector.record_execution(exec);
    }

    ASSERT_TRUE(detector.is_flaky("flaky_test"));
    ASSERT_EQ(detector.get_pass_rate("flaky_test"), 50.0);

    // Test 1.3: Get all flaky tests
    auto flaky_tests = detector.get_flaky_tests();
    ASSERT_EQ(flaky_tests.size(), 1);
    ASSERT_EQ(flaky_tests[0], "flaky_test");
  });

  // Test 2: Test execution monitoring
  TEST_CASE("Test Execution Monitoring") {
    TestExecutionMonitor monitor;

    // Test 2.1: Record test runs
    monitor.record_run("test1", true, std::chrono::milliseconds(10));
    monitor.record_run("test1", true, std::chrono::milliseconds(20));
    monitor.record_run("test1", false, std::chrono::milliseconds(15));

    auto metrics = monitor.get_metrics("test1");
    ASSERT_EQ(metrics.total_runs, 3);
    ASSERT_EQ(metrics.successful_runs, 2);
    ASSERT_EQ(metrics.failed_runs, 1);
    ASSERT_EQ(metrics.avg_duration.count(), 15);  // (10+20+15)/3
    ASSERT_EQ(metrics.min_duration.count(), 10);
    ASSERT_EQ(metrics.max_duration.count(), 20);

    // Test 2.2: Identify slow tests
    monitor.record_run("slow_test", true, std::chrono::milliseconds(500));
    auto slow_tests = monitor.get_slow_tests(std::chrono::milliseconds(100));
    ASSERT_EQ(slow_tests.size(), 1);
    ASSERT_EQ(slow_tests[0], "slow_test");
  });

  // Test 3: Performance optimization
  TEST_CASE("Performance Optimization") {
    TestPerformanceOptimizer optimizer;

    // Test 3.1: Analyze slow test
    auto suggestions1 = optimizer.analyze_test("very_slow_test", std::chrono::milliseconds(2000));
    ASSERT_FALSE(suggestions1.empty());
    ASSERT_EQ(suggestions1[0].priority, 5);

    // Test 3.2: Analyze moderately slow test
    auto suggestions2 = optimizer.analyze_test("moderate_test", std::chrono::milliseconds(200));
    ASSERT_FALSE(suggestions2.empty());
    ASSERT_EQ(suggestions2[0].priority, 3);

    // Test 3.3: Analyze fast test
    auto suggestions3 = optimizer.analyze_test("fast_test", std::chrono::milliseconds(10));
    ASSERT_TRUE(suggestions3.empty());

    // Test 3.4: Get all suggestions
    std::map<std::string, std::chrono::milliseconds> test_durations = {
        {"test1", std::chrono::milliseconds(1500)},
        {"test2", std::chrono::milliseconds(150)},
        {"test3", std::chrono::milliseconds(50)}};

    auto all_suggestions = optimizer.get_all_suggestions(test_durations);
    ASSERT_EQ(all_suggestions.size(), 2);  // test1 and test2
    ASSERT_EQ(all_suggestions[0].priority, 5);  // test1 (highest priority)
  });

  // Test 4: Test suite maintenance
  TEST_CASE("Test Suite Maintenance") {
    TestSuiteMaintenanceManager manager;

    // Test 4.1: Mark tests for maintenance
    manager.mark_deprecated("old_test");
    manager.mark_duplicate("duplicate_test");
    manager.mark_outdated("outdated_test");

    // Test 4.2: Generate maintenance report
    std::vector<std::string> all_tests = {"test1", "test2", "old_test", "duplicate_test",
                                         "outdated_test"};
    auto report = manager.generate_report(all_tests);

    ASSERT_EQ(report.total_tests, 5);
    ASSERT_EQ(report.deprecated_tests, 1);
    ASSERT_EQ(report.duplicate_tests, 1);
    ASSERT_EQ(report.outdated_tests, 1);
    ASSERT_EQ(report.tests_to_remove.size(), 2);  // deprecated + duplicate
    ASSERT_EQ(report.tests_to_update.size(), 1);  // outdated

    // Test 4.3: Cleanup test
    manager.cleanup_test("old_test");
    report = manager.generate_report(all_tests);
    ASSERT_EQ(report.deprecated_tests, 0);
  });

  // Test 5: Comprehensive reliability workflow
  TEST_CASE("Comprehensive Reliability Workflow") {
    FlakyTestDetector flaky_detector;
    TestExecutionMonitor monitor;
    TestPerformanceOptimizer optimizer;
    TestSuiteMaintenanceManager maintenance;

    // Test 5.1: Simulate test suite execution
    std::vector<std::string> test_suite = {"test1", "test2", "test3", "flaky_test"};

    for (int run = 0; run < 10; ++run) {
      for (const auto& test_name : test_suite) {
        bool passed = true;
        std::chrono::milliseconds duration(50);

        // Simulate flaky test
        if (test_name == "flaky_test") {
          passed = (run % 3 != 0);  // Fails every 3rd run
        }

        // Simulate slow test
        if (test_name == "test3") {
          duration = std::chrono::milliseconds(500);
        }

        // Record execution
        FlakyTestDetector::TestExecution exec;
        exec.test_name = test_name;
        exec.passed = passed;
        exec.duration = duration;
        flaky_detector.record_execution(exec);

        monitor.record_run(test_name, passed, duration);
      }
    }

    // Test 5.2: Identify flaky tests
    auto flaky_tests = flaky_detector.get_flaky_tests();
    ASSERT_EQ(flaky_tests.size(), 1);
    ASSERT_EQ(flaky_tests[0], "flaky_test");

    // Test 5.3: Identify slow tests
    auto slow_tests = monitor.get_slow_tests(std::chrono::milliseconds(100));
    ASSERT_EQ(slow_tests.size(), 1);
    ASSERT_EQ(slow_tests[0], "test3");

    // Test 5.4: Get optimization suggestions
    std::map<std::string, std::chrono::milliseconds> durations;
    for (const auto& test_name : test_suite) {
      auto metrics = monitor.get_metrics(test_name);
      durations[test_name] = metrics.avg_duration;
    }

    auto suggestions = optimizer.get_all_suggestions(durations);
    ASSERT_FALSE(suggestions.empty());

    // Test 5.5: Mark problematic tests for maintenance
    for (const auto& test_name : flaky_tests) {
      maintenance.mark_outdated(test_name);
    }

    auto report = maintenance.generate_report(test_suite);
    ASSERT_EQ(report.tests_to_update.size(), 1);
  });

  return current_suite->all_passed() ? 0 : 1;
}
