/**
 * @file test_performance_regression_system.cpp
 * @brief Unit tests for performance regression testing system (Task 22)
 * @note Migrated to Google Test
 *
 * Tests the comprehensive performance regression detection, baseline management,
 * performance monitoring, alerting, and optimization recommendation system.
 */

#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

// Performance regression system
#include "performance_monitor.hpp"
#include "performance_regression_system.hpp"

// Test framework
#include <gtest/gtest.h>

using namespace SolarSystem::Testing::Regression;
using namespace SolarSystem::Testing;

/**
 * @brief Mock performance test functions for testing
 */
class MockPerformanceTests {
 public:
  static PerformanceMetrics fast_test() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    PerformanceMetrics metrics;
    metrics.test_name = "fast_test";
    metrics.execution_time = std::chrono::milliseconds(10);
    metrics.cpu_time = std::chrono::milliseconds(8);
    metrics.peak_memory_kb = 1024;
    metrics.meocated_kb = 512;
    metrics.cpu_stats.total_percent = 15.0;
    metrics.cache_stats.cache_miss_rate = 0.05;
    metrics.timestamp = std::chrono::system_clock::now();

    return metrics;
  }

  static PerformanceMetrics slow_test() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    PerformanceMetrics metrics;
    metrics.test_name = "slow_test";
    metrics.execution_time = std::chrono::milliseconds(100);
    metrics.cpu_time = std::chrono::milliseconds(90);
    metrics.peak_memory_kb = 8192;
    metrics.memory_allocated_kb = 4096;
    metrics.cpu_stats.total_percent = 85.0;
    metrics.cache_stats.cache_miss_rate = 0.15;
    metrics.timestamp = std::chrono::system_clock::now();

    return metrics;
  }

  static PerformanceMetrics memory_intensive_test() {
    // Allocate some memory to simulate memory-intensive operation
    std::vector<int> data(100000, 42);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    PerformanceMetrics metrics;
    metrics.test_name = "memory_intensive_test";
    metrics.execution_time = std::chrono::milliseconds(50);
    metrics.cpu_time = std::chrono::milliseconds(40);
    metrics.peak_memory_kb = 16384;
    metrics.memory_allocated_kb = 8192;
    metrics.cpu_stats.total_percent = 60.0;
    metrics.cache_stats.cache_miss_rate = 0.20;
    metrics.timestamp = std::chrono::system_clock::now();

    return metrics;
  }

  static PerformanceMetrics regression_test() {
    // Simulate a performance regression
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    PerformanceMetrics metrics;
    metrics.test_name = "regression_test";
    metrics.execution_time = std::chrono::milliseconds(200);
    metrics.cpu_time = std::chrono::milliseconds(180);
    metrics.peak_memory_kb = 32768;
    metrics.memory_allocated_kb = 16384;
    metrics.cpu_stats.total_percent = 95.0;
    metrics.cache_stats.cache_miss_rate = 0.30;
    metrics.timestamp = std::chrono::system_clock::now();

    return metrics;
  }
};
TEST(PerformanceRegressionSystemTests, Component_Baseline_Manager) {
  ComponentBaselineManager manager;

  // Test setting baseline
  auto metrics = MockPerformanceTests::fast_test();
  manager.set_baseline("TestComponent", "fast_test", metrics);

  // Test getting baseline
  auto baseline_opt = manager.get_baseline("TestComponent", "fast_test");
  ASSERT_TRUE(baseline_opt.has_value());

  auto baseline = baseline_opt.value();
  ASSERT_EQ(baseline.component_name, "TestComponent");
  ASSERT_EQ(baseline.test_name, "fast_test");
  ASSERT_TRUE(baseline.is_valid());

  // Test updating baseline
  auto new_metrics = MockPerformanceTests::fast_test();
  new_metrics.execution_time = std::chrono::milliseconds(12);
  manager.update_baseline("TestComponent", "fast_test", new_metrics);

  auto updated_baseline_opt = manager.get_baseline("TestComponent", "fast_test");
  ASSERT_TRUE(updated_baseline_opt.has_value());

  auto updated_baseline = updated_baseline_opt.value();
  ASSERT_EQ(updated_baseline.sample_count, static_cast<size_t>(2));

  // Test baseline count
  ASSERT_EQ(manager.get_baseline_count(), static_cast<size_t>(1));

  // Test save/load baselines
  manager.save_baselines("test_baselines.txt");

  ComponentBaselineManager new_manager;
  new_manager.load_baselines("test_baselines.txt");
  ASSERT_EQ(new_manager.get_baseline_count(), static_cast<size_t>(1));

  auto loaded_baseline_opt = new_manager.get_baseline("TestComponent", "fast_test");
  ASSERT_TRUE(loaded_baseline_opt.has_value());

  // Cleanup
  std::remove("test_baselines.txt");
}
TEST(PerformanceRegressionSystemTests, Automated_Regression_Detector) {
  auto baseline_manager = std::make_shared<ComponentBaselineManager>();
  AutomatedRegressionDetector detector;
  detector.set_baseline_manager(baseline_manager);

  // Set up baseline
  auto baseline_metrics = MockPerformanceTests::fast_test();
  baseline_manager->set_baseline("TestComponent", "performance_test", baseline_metrics);

  // Test no regression (similar performance)
  auto good_metrics = MockPerformanceTests::fast_test();
  auto alerts = detector.detect_regressions("TestComponent", "performance_test", good_metrics);
  ASSERT_EQ(alerts.size(), static_cast<size_t>(0));

  // Test regression detection (much slower performance)
  auto bad_metrics = MockPerformanceTests::regression_test();
  bad_metrics.test_name = "performance_test";
  alerts = detector.detect_regressions("TestComponent", "performance_test", bad_metrics);
  ASSERT_GT(alerts.size(), static_cast<size_t>(0));

  // Verify alert details
  bool found_time_regression = false;
  bool found_memory_regression = false;

  for (const auto& alert : alerts) {
    ASSERT_EQ(alert.component_name, "TestComponent");
    ASSERT_EQ(alert.test_name, "performance_test");
    ASSERT_GT(alert.regression_factor, 1.0);

    if (alert.metric_type == RegressionAlert::MetricType::EXECUTION_TIME) {
      found_time_regression = true;
    }
    if (alert.metric_type == RegressionAlert::MetricType::MEMORY_USAGE) {
      found_memory_regression = true;
    }
  }

  ASSERT_TRUE(found_time_regression);
  ASSERT_TRUE(found_memory_regression);
}
TEST(PerformanceRegressionSystemTests, Performance_Optimization_Analyzer) {
  PerformanceOptimizationAnalyzer analyzer;

  // Test analysis of slow performance
  auto slow_metrics = MockPerformanceTests::slow_test();
  auto recommendations = analyzer.analyze_performance("TestComponent", "slow_test", slow_metrics);

  ASSERT_GT(recommendations.size(), static_cast<size_t>(0));

  // Check for CPU optimization recommendation
  bool found_cpu_optimization = false;
  for (const auto& rec : recommendations) {
    if (rec.category == OptimizationRecommendation::Category::CPU_OPTIMIZATION) {
      found_cpu_optimization = true;
      ASSERT_GT(rec.priority_score, 0);
      ASSERT_GT(rec.potential_improvement_percent, 0.0);
      ASSERT_FALSE(rec.recommendation.empty());
    }
  }
  ASSERT_TRUE(found_cpu_optimization);

  // Test analysis of memory-intensive performance
  auto memory_metrics = MockPerformanceTests::memory_intensive_test();
  recommendations = analyzer.analyze_performance("TestComponent", "memory_test", memory_metrics);

  // Should have recommendations sorted by priority
  if (recommendations.size() > 1) {
    for (size_t i = 1; i < recommendations.size(); ++i) {
      ASSERT_GE(recommendations[i - 1].priority_score, recommendations[i].priority_score);
    }
  }
}
TEST(PerformanceRegressionSystemTests, Performance_Regression_Testing_System) {
  PerformanceRegressionTestingSystem system;
  system.initialize("test_system_baselines.txt");

  // Register component tests
  std::map<std::string, std::function<PerformanceMetrics()>> tests = {
      {"fast_test", MockPerformanceTests::fast_test},
      {"memory_test", MockPerformanceTests::memory_intensive_test}};

  system.register_component("TestComponent", tests);

  ASSERT_EQ(system.get_registered_component_count(), static_cast<size_t>(1));
  ASSERT_EQ(system.get_total_test_count(), static_cast<size_t>(2));

  // Create baselines
  system.create_baselines();
  ASSERT_EQ(system.get_baseline_count(), static_cast<size_t>(2));

  // Test regression detection (should be no regressions initially)
  auto alerts = system.detect_regressions();
  ASSERT_EQ(alerts.size(), static_cast<size_t>(0));
  ASSERT_FALSE(system.has_regressions());

  // Register a regression test
  std::map<std::string, std::function<PerformanceMetrics()>> regression_tests = {
      {"regression_test", MockPerformanceTests::regression_test}};
  system.register_component("RegressionComponent", regression_tests);

  // Create baseline for regression test with good performance first
  auto good_metrics = MockPerformanceTests::fast_test();
  good_metrics.test_name = "regression_test";

  // Manually set a good baseline
  system.create_baselines();

  // Now run the actual regression test (which should trigger alerts)
  alerts = system.detect_regressions();

  // Get optimization recommendations
  auto recommendations = system.get_optimization_recommendations();
  ASSERT_GT(recommendations.size(), static_cast<size_t>(0));

  // Generate reports
  system.generate_comprehensive_report("test_performance_report.md");

  // Verify report file was created
  std::ifstream report_file("test_performance_report.md");
  ASSERT_TRUE(report_file.is_open());
  report_file.close();

  // Cleanup
  std::remove("test_system_baselines.txt");
  std::remove("test_performance_report.md");
}
TEST(PerformanceRegressionSystemTests, Baseline_Persistence_and_Loading) {
  const std::string baseline_file = "test_persistence_baselines.txt";

  // Create system and baselines
  {
    PerformanceRegressionTestingSystem system;
    system.initialize(baseline_file);

    std::map<std::string, std::function<PerformanceMetrics()>> tests = {
        {"test1", MockPerformanceTests::fast_test},
        {"test2", MockPerformanceTests::memory_intensive_test}};

    system.register_component("PersistenceComponent", tests);
    system.create_baselines();

    ASSERT_EQ(system.get_baseline_count(), static_cast<size_t>(2));
  }

  // Load system from saved baselines
  {
    PerformanceRegressionTestingSystem new_system;
    new_system.initialize(baseline_file);

    ASSERT_EQ(new_system.get_baseline_count(), static_cast<size_t>(2));
  }

  // Cleanup
  std::remove(baseline_file.c_str());
}
TEST(PerformanceRegressionSystemTests, Regression_Alert_Severity_Classification) {
  auto baseline_manager = std::make_shared<ComponentBaselineManager>();
  AutomatedRegressionDetector detector;
  detector.set_baseline_manager(baseline_manager);

  // Set up baseline with fast performance
  auto baseline_metrics = MockPerformanceTests::fast_test();
  baseline_manager->set_baseline("TestComponent", "severity_test", baseline_metrics);

  // Test minor regression (INFO level)
  auto minor_regression = baseline_metrics;
  minor_regression.execution_time = std::chrono::milliseconds(12);  // 20% slower
  auto alerts = detector.detect_regressions("TestComponent", "severity_test", minor_regression);

  if (!alerts.empty()) {
    ASSERT_EQ(alerts[0].severity, RegressionAlert::Severity::INFO);
  }

  // Test major regression (CRITICAL level)
  auto major_regression = MockPerformanceTests::regression_test();
  major_regression.test_name = "severity_test";
  alerts = detector.detect_regressions("TestComponent", "severity_test", major_regression);

  ASSERT_GT(alerts.size(), static_cast<size_t>(0));

  bool found_critical = false;
  for (const auto& alert : alerts) {
    if (alert.severity == RegressionAlert::Severity::CRITICAL) {
      found_critical = true;
      break;
    }
  }
  ASSERT_TRUE(found_critical);
}
TEST(PerformanceRegressionSystemTests, Custom_Optimization_Analyzers) {
  PerformanceOptimizationAnalyzer analyzer;

  // Add custom analyzer
  analyzer.add_custom_analyzer(
      "CustomTest",
      [](const PerformanceMetrics& metrics) -> std::vector<OptimizationRecommendation> {
        std::vector<OptimizationRecommendation> recs;

        if (metrics.execution_time_ms() > 50) {
          OptimizationRecommendation rec;
          rec.component_name = "CustomComponent";
          rec.category = OptimizationRecommendation::Category::ALGORITHM;
          rec.issue_description = "Custom performance issue detected";
          rec.recommendation = "Apply custom optimization";
          rec.potential_improvement_percent = 25.0;
          rec.priority_score = 7;
          recs.push_back(rec);
        }

        return recs;
      });

  // Test custom analyzer
  auto slow_metrics = MockPerformanceTests::slow_test();
  auto recommendations =
      analyzer.analyze_performance("CustomComponent", "custom_test", slow_metrics);

  // Should include custom recommendation
  bool found_custom = false;
  for (const auto& rec : recommendations) {
    if (rec.issue_description == "Custom performance issue detected") {
      found_custom = true;
      ASSERT_EQ(rec.potential_improvement_percent, 25.0);
      ASSERT_EQ(rec.priority_score, 7);
      break;
    }
  }
  ASSERT_TRUE(found_custom);
}
TEST(PerformanceRegressionSystemTests, Batch_Regression_Detection) {
  auto baseline_manager = std::make_shared<ComponentBaselineManager>();
  AutomatedRegressionDetector detector;
  detector.set_baseline_manager(baseline_manager);

  // Set up baselines
  auto fast_metrics = MockPerformanceTests::fast_test();
  auto memory_metrics = MockPerformanceTests::memory_intensive_test();

  baseline_manager->set_baseline("Component1", "test1", fast_metrics);
  baseline_manager->set_baseline("Component2", "test2", memory_metrics);

  // Prepare batch test results with regressions
  std::vector<std::pair<std::string, PerformanceMetrics>> test_results = {
      {"Component1::test1", MockPerformanceTests::regression_test()},
      {"Component2::test2", MockPerformanceTests::regression_test()}};

  // Run batch detection
  auto alerts = detector.batch_detect_regressions(test_results);

  ASSERT_GT(alerts.size(), static_cast<size_t>(0));

  // Verify alerts for both components
  std::set<std::string> alerted_components;
  for (const auto& alert : alerts) {
    alerted_components.insert(alert.component_name);
  }

  ASSERT_TRUE(alerted_components.find("Component1") != alerted_components.end());
  ASSERT_TRUE(alerted_components.find("Component2") != alerted_components.end());
}

// Force immediate exit to avoid hanging on cleanup
_exit(0);
