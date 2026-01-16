/**
 * @file test_optimization_guidance_system.cpp
 * @brief Performance optimization guidance system tests (Task 10)
 * @note Migrated to Google Test
 *
 * Tests optimization guidance capabilities including:
 * - Performance analysis and recommendation engine
 * - Bottleneck identification and optimization suggestions
 * - Performance impact analysis for code changes
 * - Performance monitoring and alerting
 *
 * Requirements: 3.5
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "benchmark_utils.h"
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

using namespace Benchmark;
using namespace SolarSystem;

/**
 * @brief Optimization guidance system
 */
class OptimizationGuidance {
 public:
  // Optimization recommendation
  struct Recommendation {
    std::string category;  // "memory", "cpu", "io", "algorithm"
    std::string issue;
    std::string suggestion;
    std::string priority;  // "high", "medium", "low"
    double potential_improvement_percent;
  };

  // Performance analysis result
  struct AnalysisResult {
    std::string component_name;
    double execution_time_ms;
    size_t memory_usage_bytes;
    double cpu_utilization_percent;
    std::vector<Recommendation> recommendations;
    std::string overall_assessment;
  };

  // Analyze performance and generate recommendations
  static AnalysisResult analyze_performance(const std::string& component_name,
                                            double execution_time_ms, size_t memory_usage_bytes,
                                            double cpu_utilization_percent) {
    AnalysisResult result;
    result.component_name = component_name;
    result.execution_time_ms = execution_time_ms;
    result.memory_usage_bytes = memory_usage_bytes;
    result.cpu_utilization_percent = cpu_utilization_percent;

    // Analyze execution time
    if (execution_time_ms > 1000.0) {
      Recommendation rec;
      rec.category = "cpu";
      rec.issue = "High execution time detected";
      rec.suggestion = "Consider algorithm optimization or caching";
      rec.priority = "high";
      rec.potential_improvement_percent = 50.0;
      result.recommendations.push_back(rec);
    }

    // Analyze memory usage
    if (memory_usage_bytes > 100 * 1024 * 1024) {  // > 100MB
      Recommendation rec;
      rec.category = "memory";
      rec.issue = "High memory usage detected";
      rec.suggestion = "Review data structures and consider memory pooling";
      rec.priority = "medium";
      rec.potential_improvement_percent = 30.0;
      result.recommendations.push_back(rec);
    }

    // Analyze CPU utilization
    if (cpu_utilization_percent > 80.0) {
      Recommendation rec;
      rec.category = "cpu";
      rec.issue = "High CPU utilization";
      rec.suggestion = "Consider parallel processing or workload distribution";
      rec.priority = "medium";
      rec.potential_improvement_percent = 40.0;
      result.recommendations.push_back(rec);
    }

    // Overall assessment
    if (result.recommendations.empty()) {
      result.overall_assessment = "Performance is optimal";
    } else if (result.recommendations.size() == 1) {
      result.overall_assessment = "Minor optimization opportunity";
    } else {
      result.overall_assessment = "Multiple optimization opportunities";
    }

    return result;
  }

  // Identify bottlenecks with detailed analysis
  struct BottleneckAnalysis {
    std::string operation_name;
    double time_ms;
    double percentage_of_total;
    bool is_critical_bottleneck;  // > 30% of total time
    std::vector<std::string> optimization_suggestions;
  };

  static std::vector<BottleneckAnalysis> identify_bottlenecks_detailed(
      const std::map<std::string, double>& operation_times) {
    std::vector<BottleneckAnalysis> bottlenecks;
    double total_time = 0.0;

    for (const auto& [name, time] : operation_times) {
      total_time += time;
    }

    for (const auto& [name, time] : operation_times) {
      BottleneckAnalysis analysis;
      analysis.operation_name = name;
      analysis.time_ms = time;
      analysis.percentage_of_total = (total_time > 0.0) ? (time / total_time * 100.0) : 0.0;
      analysis.is_critical_bottleneck = analysis.percentage_of_total > 30.0;

      // Generate suggestions based on operation type
      if (analysis.is_critical_bottleneck) {
        if (name.find("io") != std::string::npos) {
          analysis.optimization_suggestions.push_back("Use async I/O");
          analysis.optimization_suggestions.push_back("Implement buffering");
        } else if (name.find("compute") != std::string::npos) {
          analysis.optimization_suggestions.push_back("Optimize algorithm");
          analysis.optimization_suggestions.push_back("Use SIMD instructions");
        } else if (name.find("memory") != std::string::npos) {
          analysis.optimization_suggestions.push_back("Reduce allocations");
          analysis.optimization_suggestions.push_back("Use memory pooling");
        } else {
          analysis.optimization_suggestions.push_back("Profile and optimize");
        }
      }

      bottlenecks.push_back(analysis);
    }

    // Sort by time descending
    std::sort(bottlenecks.begin(), bottlenecks.end(),
              [](const BottleneckAnalysis& a, const BottleneckAnalysis& b) {
                return a.time_ms > b.time_ms;
              });

    return bottlenecks;
  }

  // Performance impact analysis
  struct ImpactAnalysis {
    std::string change_description;
    double baseline_time_ms;
    double new_time_ms;
    double improvement_percent;
    bool is_improvement;
    bool is_significant;  // > 10% change
    std::string verdict;
  };

  static ImpactAnalysis analyze_impact(const std::string& change_description,
                                       double baseline_time_ms, double new_time_ms) {
    ImpactAnalysis impact;
    impact.change_description = change_description;
    impact.baseline_time_ms = baseline_time_ms;
    impact.new_time_ms = new_time_ms;

    double change = baseline_time_ms - new_time_ms;
    impact.improvement_percent =
        (baseline_time_ms > 0.0) ? (change / baseline_time_ms * 100.0) : 0.0;

    impact.is_improvement = impact.improvement_percent > 0.0;
    impact.is_significant = std::abs(impact.improvement_percent) > 10.0;

    if (impact.is_improvement && impact.is_significant) {
      impact.verdict = "Significant improvement - recommend merging";
    } else if (impact.is_improvement) {
      impact.verdict = "Minor improvement - acceptable";
    } else if (!impact.is_improvement && impact.is_significant) {
      impact.verdict = "Significant regression - requires attention";
    } else {
      impact.verdict = "No significant change";
    }

    return impact;
  }

  // Performance alert
  struct PerformanceAlert {
    std::string alert_type;  // "warning", "critical"
    std::string metric_name;
    double current_value;
    double threshold_value;
    std::string message;
    std::vector<std::string> action_items;
  };

  static std::vector<PerformanceAlert> check_thresholds(
      const std::map<std::string, double>& metrics,
      const std::map<std::string, double>& thresholds) {
    std::vector<PerformanceAlert> alerts;

    for (const auto& [metric, value] : metrics) {
      auto threshold_it = thresholds.find(metric);
      if (threshold_it == thresholds.end()) {
        continue;
      }

      if (value > threshold_it->second) {
        PerformanceAlert alert;
        alert.metric_name = metric;
        alert.current_value = value;
        alert.threshold_value = threshold_it->second;

        double excess_percent = ((value - threshold_it->second) / threshold_it->second) * 100.0;

        if (excess_percent > 50.0) {
          alert.alert_type = "critical";
          alert.message = "Critical: " + metric + " exceeded threshold by " +
                          std::to_string(static_cast<int>(excess_percent)) + "%";
          alert.action_items.push_back("Immediate investigation required");
          alert.action_items.push_back("Consider rollback if recent change");
        } else {
          alert.alert_type = "warning";
          alert.message = "Warning: " + metric + " exceeded threshold by " +
                          std::to_string(static_cast<int>(excess_percent)) + "%";
          alert.action_items.push_back("Monitor closely");
          alert.action_items.push_back("Plan optimization");
        }

        alerts.push_back(alert);
      }
    }

    return alerts;
  }
};
// ============================================================================
// TASK 10: PERFORMANCE OPTIMIZATION GUIDANCE SYSTEM
// ============================================================================

// Test 1: Performance analysis and recommendation engine
TEST(OptimizationGuidanceSystemTestsTest, Performance_Analysis_and_Recommendation_Engine) {
  // Test 1.1: Optimal performance
  {
    auto result =
        OptimizationGuidance::analyze_performance("fast_component", 50.0, 1024 * 1024, 30.0);

    ASSERT_EQ(result.component_name, "fast_component");
    ASSERT_EQ(result.execution_time_ms, 50.0);
    ASSERT_TRUE(result.recommendations.empty());
    ASSERT_EQ(result.overall_assessment, "Performance is optimal");
  }

  // Test 1.2: High execution time
  {
    auto result =
        OptimizationGuidance::analyze_performance("slow_component", 2000.0, 1024 * 1024, 30.0);

    ASSERT_FALSE(result.recommendations.empty());

    bool found_cpu_rec = false;
    for (const auto& rec : result.recommendations) {
      if (rec.category == "cpu") {
        found_cpu_rec = true;
        ASSERT_EQ(rec.priority, "high");
        ASSERT_GT(rec.potential_improvement_percent, 0.0);
      }
    }
    ASSERT_TRUE(found_cpu_rec);
  }

  // Test 1.3: High memory usage
  {
    auto result =
        OptimizationGuidance::analyze_performance("memory_heavy", 100.0, 200 * 1024 * 1024, 30.0);

    bool found_memory_rec = false;
    for (const auto& rec : result.recommendations) {
      if (rec.category == "memory") {
        found_memory_rec = true;
        ASSERT_FALSE(rec.suggestion.empty());
      }
    }
    ASSERT_TRUE(found_memory_rec);
  }

  // Test 1.4: Multiple issues
  {
    auto result =
        OptimizationGuidance::analyze_performance("problematic", 2000.0, 200 * 1024 * 1024, 85.0);

    ASSERT_GE(result.recommendations.size(), 2);
    ASSERT_EQ(result.overall_assessment, "Multiple optimization opportunities");
  }
}

// Test 2: Bottleneck identification and optimization suggestions
TEST(OptimizationGuidanceSystemTestsTest, Bottleneck_Identification_and_Optimization_Suggestions) {
  // Test 2.1: Single critical bottleneck
  {
    std::map<std::string, double> times = {
        {"io_operation", 500.0}, {"compute_task", 100.0}, {"memory_access", 50.0}};

    auto bottlenecks = OptimizationGuidance::identify_bottlenecks_detailed(times);

    ASSERT_EQ(bottlenecks.size(), 3);
    ASSERT_EQ(bottlenecks[0].operation_name, "io_operation");
    ASSERT_TRUE(bottlenecks[0].is_critical_bottleneck);
    ASSERT_FALSE(bottlenecks[0].optimization_suggestions.empty());
  }

  // Test 2.2: Multiple bottlenecks
  {
    std::map<std::string, double> times = {
        {"compute_heavy", 400.0}, {"io_bound", 350.0}, {"fast_op", 50.0}};

    auto bottlenecks = OptimizationGuidance::identify_bottlenecks_detailed(times);

    int critical_count = 0;
    for (const auto& b : bottlenecks) {
      if (b.is_critical_bottleneck) {
        critical_count++;
        ASSERT_GT(b.percentage_of_total, 30.0);
      }
    }
    ASSERT_GT(critical_count, 0);
  }

  // Test 2.3: Optimization suggestions
  {
    std::map<std::string, double> times = {{"io_read", 600.0}, {"compute_loop", 200.0}};

    auto bottlenecks = OptimizationGuidance::identify_bottlenecks_detailed(times);

    for (const auto& b : bottlenecks) {
      if (b.is_critical_bottleneck) {
        ASSERT_FALSE(b.optimization_suggestions.empty());
        for (const auto& suggestion : b.optimization_suggestions) {
          ASSERT_FALSE(suggestion.empty());
        }
      }
    }
  }
}

// Test 3: Performance impact analysis for code changes
TEST(OptimizationGuidanceSystemTestsTest, Performance_Impact_Analysis_for_Code_Changes) {
  // Test 3.1: Significant improvement
  {
    auto impact = OptimizationGuidance::analyze_impact("Algorithm optimization", 1000.0, 700.0);

    ASSERT_TRUE(impact.is_improvement);
    ASSERT_TRUE(impact.is_significant);
    ASSERT_GT(impact.improvement_percent, 10.0);
    EXPECT_NE(std::string::npos, impact.verdict.find("improvement"));
  }

  // Test 3.2: Minor improvement
  {
    auto impact = OptimizationGuidance::analyze_impact("Small tweak", 1000.0, 950.0);

    ASSERT_TRUE(impact.is_improvement);
    ASSERT_FALSE(impact.is_significant);
    ASSERT_LT(impact.improvement_percent, 10.0);
  }

  // Test 3.3: Regression
  {
    auto impact = OptimizationGuidance::analyze_impact("New feature", 1000.0, 1300.0);

    ASSERT_FALSE(impact.is_improvement);
    ASSERT_TRUE(impact.is_significant);
    ASSERT_LT(impact.improvement_percent, 0.0);
    EXPECT_NE(std::string::npos, impact.verdict.find("regression"));
  }

  // Test 3.4: No significant change
  {
    auto impact = OptimizationGuidance::analyze_impact("Refactoring", 1000.0, 1050.0);

    ASSERT_FALSE(impact.is_improvement);
    ASSERT_FALSE(impact.is_significant);
    EXPECT_NE(std::string::npos, impact.verdict.find("No significant"));
  }
}

// Test 4: Performance monitoring and alerting
TEST(OptimizationGuidanceSystemTestsTest, Performance_Monitoring_and_Alerting) {
  // Test 4.1: No alerts (within thresholds)
  {
    std::map<std::string, double> metrics = {{"response_time_ms", 100.0}, {"memory_mb", 50.0}};

    std::map<std::string, double> thresholds = {{"response_time_ms", 200.0}, {"memory_mb", 100.0}};

    auto alerts = OptimizationGuidance::check_thresholds(metrics, thresholds);

    ASSERT_TRUE(alerts.empty());
  }

  // Test 4.2: Warning alert
  {
    std::map<std::string, double> metrics = {{"response_time_ms", 250.0}};

    std::map<std::string, double> thresholds = {{"response_time_ms", 200.0}};

    auto alerts = OptimizationGuidance::check_thresholds(metrics, thresholds);

    ASSERT_EQ(alerts.size(), 1);
    ASSERT_EQ(alerts[0].alert_type, "warning");
    ASSERT_FALSE(alerts[0].action_items.empty());
  }

  // Test 4.3: Critical alert
  {
    std::map<std::string, double> metrics = {{"response_time_ms", 500.0}};

    std::map<std::string, double> thresholds = {{"response_time_ms", 200.0}};

    auto alerts = OptimizationGuidance::check_thresholds(metrics, thresholds);

    ASSERT_EQ(alerts.size(), 1);
    ASSERT_EQ(alerts[0].alert_type, "critical");
    ASSERT_GT(alerts[0].current_value, alerts[0].threshold_value);
    ASSERT_FALSE(alerts[0].message.empty());
  }

  // Test 4.4: Multiple alerts
  {
    std::map<std::string, double> metrics = {
        {"response_time_ms", 500.0}, {"memory_mb", 200.0}, {"cpu_percent", 95.0}};

    std::map<std::string, double> thresholds = {
        {"response_time_ms", 200.0}, {"memory_mb", 100.0}, {"cpu_percent", 80.0}};

    auto alerts = OptimizationGuidance::check_thresholds(metrics, thresholds);

    ASSERT_EQ(alerts.size(), 3);

    for (const auto& alert : alerts) {
      ASSERT_FALSE(alert.metric_name.empty());
      ASSERT_GT(alert.current_value, alert.threshold_value);
      ASSERT_FALSE(alert.action_items.empty());
    }
  }
}

// Test 5: Integrated optimization workflow
TEST(OptimizationGuidanceSystemTestsTest, Integrated_Optimization_Workflow) {
  // Test 5.1: Complete analysis workflow
  {
    // Step 1: Run benchmark
    Timer timer;
    timer.start();

    Bodies::BodyFactory factory;
    auto result = factory.create_body("Earth");
    (void)result;

    timer.stop();
    double execution_time = timer.elapsed_ms();

    // Step 2: Analyze performance
    auto analysis = OptimizationGuidance::analyze_performance("body_creation", execution_time,
                                                              1024 * 1024, 50.0);

    ASSERT_FALSE(analysis.component_name.empty());
    ASSERT_GT(analysis.execution_time_ms, 0.0);

    // Step 3: Check for bottlenecks
    std::map<std::string, double> operation_times = {{"body_creation", execution_time},
                                                     {"other_op", 10.0}};

    auto bottlenecks = OptimizationGuidance::identify_bottlenecks_detailed(operation_times);

    ASSERT_FALSE(bottlenecks.empty());

    // Step 4: Monitor thresholds
    std::map<std::string, double> metrics = {{"body_creation_ms", execution_time}};

    std::map<std::string, double> thresholds = {{"body_creation_ms", 1000.0}};

    auto alerts = OptimizationGuidance::check_thresholds(metrics, thresholds);

    // Should not alert for fast operations
    ASSERT_TRUE(alerts.empty() || alerts[0].alert_type == "warning");
  }

  // Test 5.2: Impact analysis workflow
  {
    // Baseline measurement
    Timer baseline_timer;
    baseline_timer.start();
    Bodies::BodyFactory factory1;
    auto result1 = factory1.create_body("Mars");
    (void)result1;
    baseline_timer.stop();
    double baseline_time = baseline_timer.elapsed_ms();

    // New measurement
    Timer new_timer;
    new_timer.start();
    Bodies::BodyFactory factory2;
    auto result2 = factory2.create_body("Mars");
    (void)result2;
    new_timer.stop();
    double new_time = new_timer.elapsed_ms();

    // Analyze impact
    auto impact = OptimizationGuidance::analyze_impact("Test change", baseline_time, new_time);

    ASSERT_FALSE(impact.change_description.empty());
    ASSERT_FALSE(impact.verdict.empty());
  }
}
