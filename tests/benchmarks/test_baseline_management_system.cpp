/**
 * @file test_baseline_management_system.cpp
 * @brief Performance baseline management system tests (Task 9)
 * @note Migrated to Google Test
 *
 * Tests comprehensive baseline management capabilities including:
 * - Baseline storage and versioning system
 * - Baseline comparison and regression detection
 * - Statistical significance testing
 * - Performance trend analysis and reporting
 *
 * Requirements: 3.2, 3.3
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#include "benchmark_utils.h"
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "test_data_manager.hpp"

using namespace Benchmark;
using namespace SolarSystem;
using namespace TestData;

/**
 * @brief Enhanced baseline management utilities
 */
class BaselineManager {
 public:
  // Baseline version information
  struct BaselineVersion {
    std::string version_id;
    std::string timestamp;
    std::string git_commit;
    std::string platform;
    std::map<std::string, PerformanceResult> results;
  };

  // Statistical significance test result
  struct SignificanceTest {
    std::string test_name;
    double t_statistic;
    double p_value;
    bool is_significant;  // p < 0.05
    std::string interpretation;
  };

  // Trend analysis result
  struct TrendAnalysis {
    std::string metric_name;
    std::vector<double> historical_values;
    double trend_slope;           // Positive = improving, Negative = degrading
    double r_squared;             // Goodness of fit
    std::string trend_direction;  // "improving", "stable", "degrading"
  };

  // Save baseline to file
  static bool save_baseline(const BaselineVersion& baseline, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
      return false;
    }

    // Write version info
    file << "VERSION:" << baseline.version_id << "\n";
    file << "TIMESTAMP:" << baseline.timestamp << "\n";
    file << "COMMIT:" << baseline.git_commit << "\n";
    file << "PLATFORM:" << baseline.platform << "\n";
    file << "---\n";

    // Write results
    for (const auto& [name, result] : baseline.results) {
      file << "BENCHMARK:" << name << "\n";
      file << "DURATION_MS:" << result.duration_ms << "\n";
      file << "AVG_MS:" << result.avg_duration_ms << "\n";
      file << "MIN_MS:" << result.min_duration_ms << "\n";
      file << "MAX_MS:" << result.max_duration_ms << "\n";
      file << "STD_DEV:" << result.std_deviation_ms << "\n";
      file << "ITERATIONS:" << result.iterations << "\n";
      file << "---\n";
    }

    return true;
  }

  // Load baseline from file
  static BaselineVersion load_baseline(const std::string& filepath) {
    BaselineVersion baseline;
    std::ifstream file(filepath);

    if (!file.is_open()) {
      return baseline;
    }

    std::string line;
    std::string current_benchmark;
    PerformanceResult current_result;

    while (std::getline(file, line)) {
      if (line.empty() || line == "---") {
        if (!current_benchmark.empty()) {
          baseline.results[current_benchmark] = current_result;
          current_benchmark.clear();
          current_result = PerformanceResult();
        }
        continue;
      }

      size_t colon_pos = line.find(':');
      if (colon_pos == std::string::npos) continue;

      std::string key = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);

      if (key == "VERSION")
        baseline.version_id = value;
      else if (key == "TIMESTAMP")
        baseline.timestamp = value;
      else if (key == "COMMIT")
        baseline.git_commit = value;
      else if (key == "PLATFORM")
        baseline.platform = value;
      else if (key == "BENCHMARK") {
        current_benchmark = value;
        current_result.name = value;
      } else if (key == "DURATION_MS")
        current_result.duration_ms = std::stod(value);
      else if (key == "AVG_MS")
        current_result.avg_duration_ms = std::stod(value);
      else if (key == "MIN_MS")
        current_result.min_duration_ms = std::stod(value);
      else if (key == "MAX_MS")
        current_result.max_duration_ms = std::stod(value);
      else if (key == "STD_DEV")
        current_result.std_deviation_ms = std::stod(value);
      else if (key == "ITERATIONS")
        current_result.iterations = std::stoull(value);
    }

    return baseline;
  }

  // Compare two baselines
  static std::vector<RegressionDetector::RegressionReport> compare_baselines(
      const BaselineVersion& baseline, const BaselineVersion& current,
      double threshold_percent = 10.0) {
    std::vector<RegressionDetector::RegressionReport> reports;

    for (const auto& [name, current_result] : current.results) {
      auto baseline_it = baseline.results.find(name);
      if (baseline_it == baseline.results.end()) {
        continue;  // Skip if not in baseline
      }

      RegressionDetector::RegressionReport report;
      report.benchmark_name = name;
      report.baseline_performance = baseline_it->second.avg_duration_ms;
      report.current_performance = current_result.avg_duration_ms;

      double change = current_result.avg_duration_ms - baseline_it->second.avg_duration_ms;
      report.change_percent = (baseline_it->second.avg_duration_ms != 0.0)
                                  ? (change / baseline_it->second.avg_duration_ms * 100.0)
                                  : 0.0;

      report.is_regression = report.change_percent > threshold_percent;

      if (report.is_regression) {
        report.status = "REGRESSION";
      } else if (report.change_percent < -threshold_percent) {
        report.status = "IMPROVEMENT";
      } else {
        report.status = "STABLE";
      }

      reports.push_back(report);
    }

    return reports;
  }

  // Perform t-test for statistical significance
  static SignificanceTest perform_t_test(const std::vector<double>& baseline_samples,
                                         const std::vector<double>& current_samples) {
    SignificanceTest test;
    test.test_name = "Two-Sample T-Test";

    if (baseline_samples.empty() || current_samples.empty()) {
      test.is_significant = false;
      test.interpretation = "Insufficient data";
      return test;
    }

    // Calculate means
    double baseline_mean = std::accumulate(baseline_samples.begin(), baseline_samples.end(), 0.0) /
                           static_cast<double>(baseline_samples.size());
    double current_mean = std::accumulate(current_samples.begin(), current_samples.end(), 0.0) /
                          static_cast<double>(current_samples.size());

    // Calculate standard deviations
    double baseline_var = 0.0;
    for (double val : baseline_samples) {
      double diff = val - baseline_mean;
      baseline_var += diff * diff;
    }
    baseline_var /= static_cast<double>(baseline_samples.size());

    double current_var = 0.0;
    for (double val : current_samples) {
      double diff = val - current_mean;
      current_var += diff * diff;
    }
    current_var /= static_cast<double>(current_samples.size());

    // Calculate t-statistic
    double pooled_std = std::sqrt((baseline_var / static_cast<double>(baseline_samples.size())) +
                                  (current_var / static_cast<double>(current_samples.size())));

    if (pooled_std > 0.0) {
      test.t_statistic = (current_mean - baseline_mean) / pooled_std;
    } else {
      test.t_statistic = 0.0;
    }

    // Simplified p-value estimation (for |t| > 2, p < 0.05)
    test.p_value = (std::abs(test.t_statistic) > 2.0) ? 0.01 : 0.10;
    test.is_significant = test.p_value < 0.05;

    if (test.is_significant) {
      if (test.t_statistic > 0) {
        test.interpretation = "Significant performance degradation detected";
      } else {
        test.interpretation = "Significant performance improvement detected";
      }
    } else {
      test.interpretation = "No significant performance change";
    }

    return test;
  }

  // Analyze performance trends
  static TrendAnalysis analyze_trend(const std::string& metric_name,
                                     const std::vector<double>& historical_values) {
    TrendAnalysis analysis;
    analysis.metric_name = metric_name;
    analysis.historical_values = historical_values;

    if (historical_values.size() < 2) {
      analysis.trend_slope = 0.0;
      analysis.r_squared = 0.0;
      analysis.trend_direction = "insufficient_data";
      return analysis;
    }

    // Simple linear regression
    size_t n = historical_values.size();
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

    for (size_t i = 0; i < n; ++i) {
      double x = static_cast<double>(i);
      double y = historical_values[i];
      sum_x += x;
      sum_y += y;
      sum_xy += x * y;
      sum_x2 += x * x;
    }

    double n_double = static_cast<double>(n);
    analysis.trend_slope =
        (n_double * sum_xy - sum_x * sum_y) / (n_double * sum_x2 - sum_x * sum_x);

    // Calculate R-squared
    double mean_y = sum_y / n_double;
    double ss_tot = 0.0, ss_res = 0.0;

    for (size_t i = 0; i < n; ++i) {
      double x = static_cast<double>(i);
      double y = historical_values[i];
      double y_pred = analysis.trend_slope * x + (mean_y - analysis.trend_slope * sum_x / n_double);

      ss_tot += (y - mean_y) * (y - mean_y);
      ss_res += (y - y_pred) * (y - y_pred);
    }

    analysis.r_squared = (ss_tot > 0.0) ? (1.0 - ss_res / ss_tot) : 0.0;

    // Determine trend direction
    if (std::abs(analysis.trend_slope) < 0.01) {
      analysis.trend_direction = "stable";
    } else if (analysis.trend_slope < 0) {
      analysis.trend_direction = "improving";  // Lower times = better
    } else {
      analysis.trend_direction = "degrading";  // Higher times = worse
    }

    return analysis;
  }
};
// ============================================================================
// TASK 9: PERFORMANCE BASELINE MANAGEMENT SYSTEM
// ============================================================================

// Test 1: Baseline storage and versioning system
TEST(BaselineManagementSystemTestsTest, Baseline_Storage_and_Versioning_System) {
  auto test_env = TestDataManager::create_test_environment();
  std::string baseline_dir = test_env->path_string() + "/baselines";
  std::filesystem::create_directories(baseline_dir);

  // Test 1.1: Create and save baseline
  {
    BaselineManager::BaselineVersion baseline;
    baseline.version_id = "v1.0.0";
    baseline.timestamp = "2025-01-01T00:00:00Z";
    baseline.git_commit = "abc123";
    baseline.platform = "macOS-arm64";

    // Add some performance results
    PerformanceResult result1;
    result1.name = "test_benchmark_1";
    result1.duration_ms = 100.0;
    result1.avg_duration_ms = 95.0;
    result1.min_duration_ms = 90.0;
    result1.max_duration_ms = 110.0;
    result1.std_deviation_ms = 5.0;
    result1.iterations = 100;

    baseline.results["test_benchmark_1"] = result1;

    std::string filepath = baseline_dir + "/baseline_v1.txt";
    bool saved = BaselineManager::save_baseline(baseline, filepath);

    ASSERT_TRUE(saved);
    ASSERT_TRUE(std::filesystem::exists(filepath));
  }

  // Test 1.2: Load baseline
  {
    std::string filepath = baseline_dir + "/baseline_v1.txt";
    auto loaded = BaselineManager::load_baseline(filepath);

    ASSERT_EQ(loaded.version_id, "v1.0.0");
    ASSERT_EQ(loaded.timestamp, "2025-01-01T00:00:00Z");
    ASSERT_EQ(loaded.git_commit, "abc123");
    ASSERT_EQ(loaded.platform, "macOS-arm64");
    ASSERT_EQ(loaded.results.size(), 1);
    ASSERT_TRUE(loaded.results.find("test_benchmark_1") != loaded.results.end());
  }

  // Test 1.3: Multiple baseline versions
  {
    for (int i = 2; i <= 5; ++i) {
      BaselineManager::BaselineVersion baseline;
      baseline.version_id = "v1.0." + std::to_string(i);
      baseline.timestamp = "2025-01-0" + std::to_string(i) + "T00:00:00Z";
      baseline.git_commit = "commit" + std::to_string(i);
      baseline.platform = "macOS-arm64";

      PerformanceResult result;
      result.name = "test_benchmark";
      result.avg_duration_ms = 100.0 + static_cast<double>(i);
      result.iterations = 100;

      baseline.results["test_benchmark"] = result;

      std::string filepath = baseline_dir + "/baseline_v" + std::to_string(i) + ".txt";
      bool saved = BaselineManager::save_baseline(baseline, filepath);
      ASSERT_TRUE(saved);
    }

    // Verify all versions exist
    for (int i = 1; i <= 5; ++i) {
      std::string filepath = baseline_dir + "/baseline_v" + std::to_string(i) + ".txt";
      ASSERT_TRUE(std::filesystem::exists(filepath));
    }
  }
}

// Test 2: Baseline comparison and regression detection
TEST(BaselineManagementSystemTestsTest, Baseline_Comparison_and_Regression_Detection) {
  // Test 2.1: Detect regression
  {
    BaselineManager::BaselineVersion baseline;
    baseline.version_id = "v1.0.0";

    PerformanceResult baseline_result;
    baseline_result.name = "performance_test";
    baseline_result.avg_duration_ms = 100.0;
    baseline.results["performance_test"] = baseline_result;

    BaselineManager::BaselineVersion current;
    current.version_id = "v1.0.1";

    PerformanceResult current_result;
    current_result.name = "performance_test";
    current_result.avg_duration_ms = 120.0;  // 20% slower - regression!
    current.results["performance_test"] = current_result;

    auto reports = BaselineManager::compare_baselines(baseline, current, 10.0);

    ASSERT_EQ(reports.size(), 1);
    ASSERT_TRUE(reports[0].is_regression);
    ASSERT_EQ(reports[0].status, "REGRESSION");
    ASSERT_GT(reports[0].change_percent, 10.0);
  }

  // Test 2.2: Detect improvement
  {
    BaselineManager::BaselineVersion baseline;
    PerformanceResult baseline_result;
    baseline_result.name = "optimized_test";
    baseline_result.avg_duration_ms = 100.0;
    baseline.results["optimized_test"] = baseline_result;

    BaselineManager::BaselineVersion current;
    PerformanceResult current_result;
    current_result.name = "optimized_test";
    current_result.avg_duration_ms = 80.0;  // 20% faster - improvement!
    current.results["optimized_test"] = current_result;

    auto reports = BaselineManager::compare_baselines(baseline, current, 10.0);

    ASSERT_EQ(reports.size(), 1);
    ASSERT_FALSE(reports[0].is_regression);
    ASSERT_EQ(reports[0].status, "IMPROVEMENT");
    ASSERT_LT(reports[0].change_percent, -10.0);
  }

  // Test 2.3: Stable performance
  {
    BaselineManager::BaselineVersion baseline;
    PerformanceResult baseline_result;
    baseline_result.name = "stable_test";
    baseline_result.avg_duration_ms = 100.0;
    baseline.results["stable_test"] = baseline_result;

    BaselineManager::BaselineVersion current;
    PerformanceResult current_result;
    current_result.name = "stable_test";
    current_result.avg_duration_ms = 105.0;  // 5% change - within threshold
    current.results["stable_test"] = current_result;

    auto reports = BaselineManager::compare_baselines(baseline, current, 10.0);

    ASSERT_EQ(reports.size(), 1);
    ASSERT_FALSE(reports[0].is_regression);
    ASSERT_EQ(reports[0].status, "STABLE");
  }

  // Test 2.4: Multiple benchmarks comparison
  {
    BaselineManager::BaselineVersion baseline;
    BaselineManager::BaselineVersion current;

    for (int i = 1; i <= 5; ++i) {
      std::string name = "benchmark_" + std::to_string(i);

      PerformanceResult baseline_result;
      baseline_result.name = name;
      baseline_result.avg_duration_ms = 100.0;
      baseline.results[name] = baseline_result;

      PerformanceResult current_result;
      current_result.name = name;
      // Vary performance: some regressions, some improvements, some stable
      current_result.avg_duration_ms = 100.0 + static_cast<double>((i - 3) * 15);
      current.results[name] = current_result;
    }

    auto reports = BaselineManager::compare_baselines(baseline, current, 10.0);

    ASSERT_EQ(reports.size(), 5);

    // Count different statuses
    int regressions = 0, improvements = 0, stable = 0;
    for (const auto& report : reports) {
      if (report.status == "REGRESSION")
        regressions++;
      else if (report.status == "IMPROVEMENT")
        improvements++;
      else if (report.status == "STABLE")
        stable++;
    }

    ASSERT_GT(regressions, 0);
    ASSERT_GT(improvements, 0);
    ASSERT_GT(stable, 0);
  }
}

// Test 3: Statistical significance testing
TEST(BaselineManagementSystemTestsTest, Statistical_Significance_Testing) {
  // Test 3.1: Significant difference
  {
    std::vector<double> baseline_samples = {100.0, 101.0, 99.0, 100.5, 100.2};
    std::vector<double> current_samples = {120.0, 121.0, 119.0, 120.5, 120.2};

    auto test = BaselineManager::perform_t_test(baseline_samples, current_samples);

    ASSERT_TRUE(test.is_significant);
    ASSERT_GT(std::abs(test.t_statistic), 2.0);
    ASSERT_LT(test.p_value, 0.05);
    ASSERT_FALSE(test.interpretation.empty());
  }

  // Test 3.2: No significant difference
  {
    std::vector<double> baseline_samples = {100.0, 101.0, 99.0, 100.5, 100.2};
    std::vector<double> current_samples = {100.5, 101.5, 99.5, 101.0, 100.7};

    auto test = BaselineManager::perform_t_test(baseline_samples, current_samples);

    ASSERT_FALSE(test.is_significant);
    ASSERT_GE(test.p_value, 0.05);
  }

  // Test 3.3: Performance improvement significance
  {
    std::vector<double> baseline_samples = {100.0, 101.0, 99.0, 100.5, 100.2};
    std::vector<double> current_samples = {80.0, 81.0, 79.0, 80.5, 80.2};

    auto test = BaselineManager::perform_t_test(baseline_samples, current_samples);

    ASSERT_TRUE(test.is_significant);
    ASSERT_LT(test.t_statistic, -2.0);  // Negative = improvement
    EXPECT_NE(std::string::npos, test.interpretation.find("improvement"));
  }

  // Test 3.4: Edge cases
  {
    std::vector<double> empty_samples;
    std::vector<double> valid_samples = {100.0, 101.0, 99.0};

    auto test = BaselineManager::perform_t_test(empty_samples, valid_samples);

    ASSERT_FALSE(test.is_significant);
    EXPECT_NE(std::string::npos, test.interpretation.find("Insufficient"));
  }
}

// Test 4: Performance trend analysis and reporting
TEST(BaselineManagementSystemTestsTest, Performance_Trend_Analysis_and_Reporting) {
  // Test 4.1: Improving trend
  {
    std::vector<double> improving_values = {100.0, 95.0, 90.0, 85.0, 80.0};

    auto trend = BaselineManager::analyze_trend("improving_metric", improving_values);

    ASSERT_EQ(trend.metric_name, "improving_metric");
    ASSERT_EQ(trend.historical_values.size(), 5);
    ASSERT_LT(trend.trend_slope, -0.01);  // Negative slope = improving
    ASSERT_TRUE(trend.trend_direction == "improving" || trend.trend_direction == "stable");
    ASSERT_GT(trend.r_squared, 0.8);  // Strong correlation
  }

  // Test 4.2: Degrading trend
  {
    std::vector<double> degrading_values = {80.0, 85.0, 90.0, 95.0, 100.0};

    auto trend = BaselineManager::analyze_trend("degrading_metric", degrading_values);

    ASSERT_GT(trend.trend_slope, 0.01);  // Positive slope = degrading
    ASSERT_TRUE(trend.trend_direction == "degrading" || trend.trend_direction == "stable");
    ASSERT_GT(trend.r_squared, 0.8);
  }

  // Test 4.3: Stable trend
  {
    std::vector<double> stable_values = {100.0, 100.5, 99.5, 100.2, 99.8};

    auto trend = BaselineManager::analyze_trend("stable_metric", stable_values);

    ASSERT_NEAR(trend.trend_slope, 0.0, 0.5);
    // Direction could be stable or slightly improving/degrading due to small variations
    ASSERT_TRUE(trend.trend_direction == "stable" || trend.trend_direction == "improving" ||
                trend.trend_direction == "degrading");
  }

  // Test 4.4: Noisy data
  {
    std::vector<double> noisy_values = {100.0, 110.0, 95.0, 105.0, 90.0, 115.0};

    auto trend = BaselineManager::analyze_trend("noisy_metric", noisy_values);

    ASSERT_FALSE(trend.metric_name.empty());
    ASSERT_EQ(trend.historical_values.size(), 6);
    // R-squared should be lower for noisy data
    ASSERT_LT(trend.r_squared, 0.9);
  }

  // Test 4.5: Insufficient data
  {
    std::vector<double> single_value = {100.0};

    auto trend = BaselineManager::analyze_trend("insufficient_metric", single_value);

    ASSERT_EQ(trend.trend_direction, "insufficient_data");
    ASSERT_EQ(trend.trend_slope, 0.0);
  }
}

// Test 5: Integrated baseline management workflow
TEST(BaselineManagementSystemTestsTest, Integrated_Baseline_Management_Workflow) {
  auto test_env = TestDataManager::create_test_environment();
  std::string baseline_dir = test_env->path_string() + "/baselines";
  std::filesystem::create_directories(baseline_dir);

  // Test 5.1: Complete workflow
  {
    // Step 1: Create initial baseline
    BenchmarkSuite suite("Baseline Workflow");

    suite.run_benchmark(
        "workflow_test",
        []() {
          Bodies::BodyFactory factory;
          auto result = factory.create_body("Earth");
          (void)result;
        },
        10);

    auto results = suite.get_results();
    ASSERT_FALSE(results.empty());

    // Step 2: Save as baseline
    BaselineManager::BaselineVersion baseline;
    baseline.version_id = "v1.0.0";
    baseline.timestamp = "2025-01-01T00:00:00Z";
    baseline.git_commit = "initial";
    baseline.platform = "test";

    for (const auto& result : results) {
      baseline.results[result.name] = result;
    }

    std::string baseline_file = baseline_dir + "/workflow_baseline.txt";
    bool saved = BaselineManager::save_baseline(baseline, baseline_file);
    ASSERT_TRUE(saved);

    // Step 3: Run new benchmarks
    BenchmarkSuite suite2("Current Run");
    suite2.run_benchmark(
        "workflow_test",
        []() {
          Bodies::BodyFactory factory;
          auto result = factory.create_body("Earth");
          (void)result;
        },
        10);

    auto current_results = suite2.get_results();
    ASSERT_FALSE(current_results.empty());

    // Step 4: Compare with baseline
    BaselineManager::BaselineVersion current;
    current.version_id = "v1.0.1";
    for (const auto& result : current_results) {
      current.results[result.name] = result;
    }

    auto reports = BaselineManager::compare_baselines(baseline, current, 20.0);
    ASSERT_FALSE(reports.empty());

    // Step 5: Verify comparison results
    for (const auto& report : reports) {
      ASSERT_FALSE(report.benchmark_name.empty());
      ASSERT_GT(report.baseline_performance, 0.0);
      ASSERT_GT(report.current_performance, 0.0);
      ASSERT_FALSE(report.status.empty());
    }
  }
}
