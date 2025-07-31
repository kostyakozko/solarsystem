#pragma once

/**
 * @file regression_detector.hpp
 * @brief Performance regression detection system
 *
 * This header provides comprehensive performance regression detection:
 * - Baseline performance storage and comparison
 * - Performance alert system for degradation detection
 * - Performance trend analysis and reporting
 * - CI integration for automated performance monitoring
 */

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "benchmark.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Performance baseline data
 */
struct PerformanceBaseline {
  std::string benchmark_name;                       ///< Name of the benchmark
  std::chrono::nanoseconds mean_time{0};            ///< Baseline mean execution time
  std::chrono::nanoseconds std_dev{0};              ///< Baseline standard deviation
  size_t memory_usage_bytes = 0;                    ///< Baseline memory usage
  double operations_per_second = 0.0;               ///< Baseline operations per second
  std::chrono::system_clock::time_point timestamp;  ///< When baseline was created
  std::string version;                              ///< Software version for baseline
  std::map<std::string, std::string> metadata;      ///< Additional baseline metadata

  /**
   * @brief Convert to JSON string for storage
   */
  [[nodiscard]] std::string to_json() const;

  /**
   * @brief Create from JSON string
   */
  static PerformanceBaseline from_json(const std::string& json);

  /**
   * @brief Convert to CSV row for reporting
   */
  [[nodiscard]] std::string to_csv_row() const;

  /**
   * @brief Get CSV header for baseline data
   */
  [[nodiscard]] static std::string csv_header();
};

/**
 * @brief Performance regression analysis result
 */
struct RegressionAnalysis {
  std::string benchmark_name;                 ///< Name of the benchmark
  bool has_regression = false;                ///< Whether regression was detected
  double time_regression_percentage = 0.0;    ///< Time regression as percentage
  double memory_regression_percentage = 0.0;  ///< Memory regression as percentage
  double ops_regression_percentage = 0.0;     ///< Operations/sec regression as percentage
  std::string severity;                       ///< Regression severity (minor, major, critical)
  std::vector<std::string> alerts;            ///< List of alert messages
  PerformanceBaseline baseline;               ///< Baseline used for comparison
  BenchmarkResult current_result;             ///< Current benchmark result
  std::chrono::system_clock::time_point analysis_time;  ///< When analysis was performed

  /**
   * @brief Check if regression exceeds threshold
   */
  [[nodiscard]] bool exceeds_threshold(double threshold_percentage) const;

  /**
   * @brief Generate human-readable report
   */
  [[nodiscard]] std::string generate_report() const;

  /**
   * @brief Convert to JSON for CI integration
   */
  [[nodiscard]] std::string to_json() const;
};

/**
 * @brief Performance trend data point
 */
struct TrendDataPoint {
  std::chrono::system_clock::time_point timestamp;
  std::chrono::nanoseconds execution_time{0};
  size_t memory_usage_bytes = 0;
  double operations_per_second = 0.0;
  std::string version;
  std::map<std::string, std::string> metadata;
};

/**
 * @brief Performance trend analysis
 */
struct TrendAnalysis {
  std::string benchmark_name;
  std::vector<TrendDataPoint> data_points;
  double time_trend_slope = 0.0;    ///< Trend slope for execution time
  double memory_trend_slope = 0.0;  ///< Trend slope for memory usage
  double ops_trend_slope = 0.0;     ///< Trend slope for operations/sec
  std::string trend_direction;      ///< "improving", "stable", "degrading"
  double confidence_level = 0.0;    ///< Statistical confidence in trend

  /**
   * @brief Generate trend report
   */
  [[nodiscard]] std::string generate_report() const;
};

/**
 * @brief Performance regression detector configuration
 */
struct RegressionDetectorConfig {
  double time_regression_threshold = 10.0;    ///< Time regression threshold (%)
  double memory_regression_threshold = 15.0;  ///< Memory regression threshold (%)
  double ops_regression_threshold = 10.0;     ///< Ops/sec regression threshold (%)
  size_t min_samples_for_trend = 5;           ///< Minimum samples for trend analysis
  std::string baseline_storage_path = "baseline_performance.json";
  std::string trend_storage_path = "performance_trends.json";
  bool enable_ci_integration = true;  ///< Enable CI-specific features
  bool auto_update_baseline = false;  ///< Auto-update baseline on improvement
};

/**
 * @brief Performance regression detector
 */
class RegressionDetector {
 public:
  /**
   * @brief Create regression detector with configuration
   */
  explicit RegressionDetector(RegressionDetectorConfig config = {});

  /**
   * @brief Load baseline data from storage
   */
  void load_baselines();

  /**
   * @brief Save baseline data to storage
   */
  void save_baselines();

  /**
   * @brief Set baseline for a benchmark
   */
  void set_baseline(const std::string& benchmark_name, const BenchmarkResult& result,
                    const std::string& version = "");

  /**
   * @brief Get baseline for a benchmark
   */
  [[nodiscard]] std::optional<PerformanceBaseline> get_baseline(
      const std::string& benchmark_name) const;

  /**
   * @brief Analyze benchmark result for regressions
   */
  [[nodiscard]] RegressionAnalysis analyze_regression(const BenchmarkResult& result);

  /**
   * @brief Add data point for trend analysis
   */
  void add_trend_data_point(const std::string& benchmark_name, const BenchmarkResult& result,
                            const std::string& version = "");

  /**
   * @brief Analyze performance trends
   */
  [[nodiscard]] TrendAnalysis analyze_trends(const std::string& benchmark_name);

  /**
   * @brief Generate comprehensive regression report
   */
  [[nodiscard]] std::string generate_regression_report(
      const std::vector<RegressionAnalysis>& analyses);

  /**
   * @brief Generate CI-compatible output
   */
  void generate_ci_output(const std::vector<RegressionAnalysis>& analyses,
                          const std::string& output_path);

  /**
   * @brief Check if any regressions exceed threshold
   */
  [[nodiscard]] bool has_critical_regressions(
      const std::vector<RegressionAnalysis>& analyses) const;

  /**
   * @brief Get configuration
   */
  [[nodiscard]] const RegressionDetectorConfig& config() const { return config_; }

 private:
  RegressionDetectorConfig config_;
  std::map<std::string, PerformanceBaseline> baselines_;
  std::map<std::string, std::vector<TrendDataPoint>> trend_data_;

  /**
   * @brief Calculate regression percentage
   */
  [[nodiscard]] double calculate_regression_percentage(double baseline, double current) const;

  /**
   * @brief Determine regression severity
   */
  [[nodiscard]] std::string determine_severity(double time_regression, double memory_regression,
                                               double ops_regression) const;

  /**
   * @brief Calculate linear trend slope
   */
  [[nodiscard]] double calculate_trend_slope(const std::vector<double>& values) const;

  /**
   * @brief Load trend data from storage
   */
  void load_trend_data();

  /**
   * @brief Save trend data to storage
   */
  void save_trend_data();
};

/**
 * @brief Performance alert system
 */
class PerformanceAlertSystem {
 public:
  /**
   * @brief Alert configuration
   */
  struct AlertConfig {
    std::vector<std::string> email_recipients;
    std::string slack_webhook_url;
    std::string github_issue_token;
    bool enable_email_alerts = false;
    bool enable_slack_alerts = false;
    bool enable_github_issues = false;
    double critical_threshold = 20.0;  ///< Critical regression threshold (%)
  };

  /**
   * @brief Create alert system with configuration
   */
  explicit PerformanceAlertSystem(AlertConfig config);

  /**
   * @brief Send regression alert
   */
  void send_regression_alert(const RegressionAnalysis& analysis);

  /**
   * @brief Send trend alert
   */
  void send_trend_alert(const TrendAnalysis& analysis);

  /**
   * @brief Send batch alert for multiple regressions
   */
  void send_batch_alert(const std::vector<RegressionAnalysis>& analyses);

 private:
  AlertConfig config_;

  /**
   * @brief Send email alert
   */
  void send_email_alert(const std::string& subject, const std::string& body);

  /**
   * @brief Send Slack alert
   */
  void send_slack_alert(const std::string& message);

  /**
   * @brief Create GitHub issue
   */
  void create_github_issue(const std::string& title, const std::string& body);
};

/**
 * @brief CI integration utilities
 */
class CIIntegration {
 public:
  /**
   * @brief Generate GitHub Actions output
   */
  static void generate_github_actions_output(const std::vector<RegressionAnalysis>& analyses,
                                             const std::string& output_path);

  /**
   * @brief Generate Jenkins output
   */
  static void generate_jenkins_output(const std::vector<RegressionAnalysis>& analyses,
                                      const std::string& output_path);

  /**
   * @brief Set GitHub Actions step output
   */
  static void set_github_step_output(const std::string& name, const std::string& value);

  /**
   * @brief Exit with appropriate code for CI
   */
  static void exit_with_ci_code(const std::vector<RegressionAnalysis>& analyses,
                                double failure_threshold = 10.0);

  /**
   * @brief Generate performance badge data
   */
  static std::string generate_performance_badge(const std::vector<RegressionAnalysis>& analyses);
};

}  // namespace SolarSystem::Testing
