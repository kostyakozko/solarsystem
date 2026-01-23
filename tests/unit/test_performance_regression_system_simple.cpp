/**
 * @file test_performance_regression_system_simple.cpp
 * @brief Unit tests for simplified performance regression testing system (Task 22)
 * @note Migrated to Google Test
 *
 * Tests the basic performance regression detection, baseline management,
 * and optimization recommendation system.
 */

#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

// Performance regression system
#include "../utils/performance_regression_system_simple.hpp"

// Test framework
#include <gtest/gtest.h>

#include "../utils/test_diagnostics.hpp"
#include "../utils/test_port_manager.hpp"

using namespace SolarSystem::Testing::Regression;

/**
 * @brief Mock performance test functions for testing
 */
class MockSimplePerformanceTests {
 public:
  static SimplePerformanceMetrics fast_test() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return create_simple_metrics("fast_test", std::chrono::milliseconds(10), 1024, 15.0);
  }

  static SimplePerformanceMetrics slow_test() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return create_simple_metrics("slow_test", std::chrono::milliseconds(100), 8192, 85.0);
  }

  static SimplePerformanceMetrics memory_intensive_test() {
    // Allocate some memory to simulate memory-intensive operation
    std::vector<int> data(100000, 42);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return create_simple_metrics("memory_intensive_test", std::chrono::milliseconds(50), 16384,
                                 60.0);
  }

  static SimplePerformanceMetrics regression_test() {
    // Simulate a performance regression
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return create_simple_metrics("regression_test", std::chrono::milliseconds(200), 32768, 95.0);
  }
};
TEST(SimplePerformanceRegressionSystemTests, Simple_Baseline_Manager) {
  SimpleBaselineManager manager;

  // Test setting baseline
  auto metrics = MockSimplePerformanceTests::fast_test();
  manager.set_baseline("TestComponent", "fast_test", metrics);

  // Test getting baseline
  auto* baseline = manager.get_baseline("TestComponent", "fast_test");
  ASSERT_NE(nullptr, baseline);

  ASSERT_EQ(baseline->component_name, "TestComponent");
  ASSERT_EQ(baseline->test_name, "fast_test");
  ASSERT_EQ(baseline->sample_count, static_cast<size_t>(1));

  // Test updating baseline
  auto new_metrics = MockSimplePerformanceTests::fast_test();
  new_metrics.execution_time = std::chrono::milliseconds(12);
  manager.update_baseline("TestComponent", "fast_test", new_metrics);

  auto* updated_baseline = manager.get_baseline("TestComponent", "fast_test");
  ASSERT_NE(nullptr, updated_baseline);
  ASSERT_EQ(updated_baseline->sample_count, static_cast<size_t>(2));

  // Test baseline count
  ASSERT_EQ(manager.get_baseline_count(), static_cast<size_t>(1));

  // Test save/load baselines
  manager.save_baselines("test_simple_baselines.txt");

  SimpleBaselineManager new_manager;
  new_manager.load_baselines("test_simple_baselines.txt");
  ASSERT_EQ(new_manager.get_baseline_count(), static_cast<size_t>(1));

  auto* loaded_baseline = new_manager.get_baseline("TestComponent", "fast_test");
  ASSERT_NE(nullptr, loaded_baseline);

  // Cleanup
  std::remove("test_simple_baselines.txt");
}
TEST(SimplePerformanceRegressionSystemTests, Simple_Regression_Detector) {
  auto baseline_manager = std::make_shared<SimpleBaselineManager>();
  SimpleRegressionDetector detector(1.2);  // 20% threshold
  detector.set_baseline_manager(baseline_manager);

  // Set up baseline
  auto baseline_metrics = MockSimplePerformanceTests::fast_test();
  baseline_manager->set_baseline("TestComponent", "performance_test", baseline_metrics);

  // Test no regression (similar performance)
  auto good_metrics = MockSimplePerformanceTests::fast_test();
  auto alerts = detector.detect_regressions("TestComponent", "performance_test", good_metrics);
  ASSERT_EQ(alerts.size(), static_cast<size_t>(0));

  // Test regression detection (much slower performance)
  auto bad_metrics = MockSimplePerformanceTests::regression_test();
  bad_metrics.test_name = "performance_test";
  alerts = detector.detect_regressions("TestComponent", "performance_test", bad_metrics);
  ASSERT_GT(alerts.size(), static_cast<size_t>(0));

  // Verify alert details
  const auto& alert = alerts[0];
  ASSERT_EQ(alert.component_name, "TestComponent");
  ASSERT_EQ(alert.test_name, "performance_test");
  ASSERT_GT(alert.regression_factor, 1.0);
  ASSERT_FALSE(alert.description.empty());
  ASSERT_FALSE(alert.recommendation.empty());
}
TEST(SimplePerformanceRegressionSystemTests, Simple_Optimization_Analyzer) {
  SimpleOptimizationAnalyzer analyzer;

  // Test analysis of slow performance
  auto slow_metrics = MockSimplePerformanceTests::slow_test();
  auto recommendations = analyzer.analyze_performance("TestComponent", "slow_test", slow_metrics);

  ASSERT_GT(recommendations.size(), static_cast<size_t>(0));

  // Check for CPU optimization recommendation
  bool found_cpu_optimization = false;
  for (const auto& rec : recommendations) {
    if (rec.issue_description.find("CPU usage") != std::string::npos) {
      found_cpu_optimization = true;
      ASSERT_GT(rec.priority_score, 0);
      ASSERT_GT(rec.potential_improvement_percent, 0.0);
      ASSERT_FALSE(rec.recommendation.empty());
    }
  }
  ASSERT_TRUE(found_cpu_optimization);

  // Test analysis of memory-intensive performance
  auto memory_metrics = MockSimplePerformanceTests::memory_intensive_test();
  recommendations = analyzer.analyze_performance("TestComponent", "memory_test", memory_metrics);

  // Check for memory optimization recommendation
  bool found_memory_optimization = false;
  for (const auto& rec : recommendations) {
    if (rec.issue_description.find("memory usage") != std::string::npos) {
      found_memory_optimization = true;
    }
  }
  ASSERT_TRUE(found_memory_optimization);
}
TEST(SimplePerformanceRegressionSystemTests, Simple_Performance_Regression_Testing_System) {
  SimplePerformanceRegressionSystem system;
  system.initialize("test_simple_system_baselines.txt");

  // Register component tests
  std::map<std::string, std::function<SimplePerformanceMetrics()>> tests = {
      {"fast_test", MockSimplePerformanceTests::fast_test},
      {"memory_test", MockSimplePerformanceTests::memory_intensive_test}};

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

  // Test that the system can handle additional components
  std::map<std::string, std::function<SimplePerformanceMetrics()>> additional_tests = {
      {"additional_test", MockSimplePerformanceTests::slow_test}};
  system.register_component("AdditionalComponent", additional_tests);

  // Run all tests again
  alerts = system.detect_regressions();

  // Get optimization recommendations (should have some due to slow test)
  auto recommendations = system.get_optimization_recommendations();
  // Note: May or may not have recommendations depending on baseline comparison

  // Generate reports
  system.generate_report("test_simple_performance_report.md");

  // Verify report file was created
  std::ifstream report_file("test_simple_performance_report.md");
  ASSERT_TRUE(report_file.is_open());
  report_file.close();

  // Cleanup
  std::remove("test_simple_system_baselines.txt");
  std::remove("test_simple_performance_report.md");
}
TEST(SimplePerformanceRegressionSystemTests, Baseline_Persistence_and_Loading) {
  const std::string baseline_file = "test_simple_persistence_baselines.txt";

  // Create system and baselines
  {
    SimplePerformanceRegressionSystem system;
    system.initialize(baseline_file);

    std::map<std::string, std::function<SimplePerformanceMetrics()>> tests = {
        {"test1", MockSimplePerformanceTests::fast_test},
        {"test2", MockSimplePerformanceTests::memory_intensive_test}};

    system.register_component("PersistenceComponent", tests);
    system.create_baselines();

    ASSERT_EQ(system.get_baseline_count(), static_cast<size_t>(2));
  }

  // Load system from saved baselines
  {
    SimplePerformanceRegressionSystem new_system;
    new_system.initialize(baseline_file);

    ASSERT_EQ(new_system.get_baseline_count(), static_cast<size_t>(2));
  }

  // Cleanup
  std::remove(baseline_file.c_str());
}
TEST(SimplePerformanceRegressionSystemTests, Utility_Functions) {
  // Test global system initialization
  initialize_simple_performance_regression_testing("test_global_baselines.txt");

  ASSERT_NE(nullptr, g_simple_performance_system.get());

  // Test component registration
  std::map<std::string, std::function<SimplePerformanceMetrics()>> tests = {
      {"global_test", MockSimplePerformanceTests::fast_test}};

  register_simple_component_performance_tests("GlobalComponent", tests);

  ASSERT_EQ(g_simple_performance_system->get_registered_component_count(), static_cast<size_t>(1));

  // Test regression detection
  auto alerts = run_simple_performance_regression_tests();
  // Should be empty since no baselines exist yet

  // Test optimization recommendations
  auto recommendations = get_simple_performance_optimization_recommendations();

  // Test report generation
  generate_simple_performance_reports();

  // Verify report file was created
  std::ifstream report_file("simple_performance_regression_report.md");
  ASSERT_TRUE(report_file.is_open());
  report_file.close();

  // Cleanup
  std::remove("test_global_baselines.txt");
  std::remove("simple_performance_regression_report.md");
}
TEST(SimplePerformanceRegressionSystemTests, Create_Simple_Metrics_Utility) {
  auto metrics = create_simple_metrics("test_metrics", std::chrono::milliseconds(100), 2048, 50.0);

  ASSERT_EQ(metrics.test_name, "test_metrics");
  ASSERT_EQ(metrics.execution_time.count(), 100);
  ASSERT_EQ(metrics.memory_usage_kb, static_cast<size_t>(2048));
  ASSERT_EQ(metrics.cpu_usage_percent, 50.0);
  ASSERT_EQ(metrics.execution_time_ms(), 100.0);
}
