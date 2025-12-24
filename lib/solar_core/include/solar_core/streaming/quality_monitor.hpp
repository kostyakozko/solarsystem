#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "solar_core/export.hpp"
#include "solar_core/streaming/data_stream.hpp"

namespace SolarSystem::Streaming {

/**
 * @brief Quality metrics for individual data points
 */
struct DataPointQuality {
  std::string body_name;
  std::chrono::system_clock::time_point timestamp;

  // Core quality metrics
  double overall_score = 1.0;        // 0.0 to 1.0, composite quality score
  double data_freshness = 1.0;       // How recent the data is
  double data_accuracy = 1.0;        // Estimated accuracy of the data
  double data_completeness = 1.0;    // Completeness of required fields
  double data_consistency = 1.0;     // Consistency with previous data

  // Performance metrics
  std::chrono::milliseconds latency{0};
  std::chrono::milliseconds processing_time{0};

  // Source information
  std::string data_source;
  std::string quality_assessment_reason;

  // Flags
  bool has_anomalies = false;
  bool requires_attention = false;
  bool is_predicted = false;
  bool is_interpolated = false;

  std::vector<std::string> warnings;
  std::vector<std::string> errors;

  [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Quality metrics for entire snapshots
 */
struct SnapshotQuality {
  std::chrono::system_clock::time_point timestamp;
  size_t total_bodies = 0;
  size_t valid_bodies = 0;
  size_t bodies_with_warnings = 0;
  size_t bodies_with_errors = 0;

  // Aggregate quality scores
  double overall_score = 1.0;
  double avg_data_freshness = 1.0;
  double avg_data_accuracy = 1.0;
  double avg_data_completeness = 1.0;
  double avg_data_consistency = 1.0;

  // Performance metrics
  std::chrono::milliseconds total_latency{0};
  std::chrono::milliseconds avg_latency{0};
  std::chrono::milliseconds max_latency{0};
  std::chrono::milliseconds processing_time{0};

  // Quality distribution
  size_t excellent_quality_count = 0;  // score >= 0.9
  size_t good_quality_count = 0;       // score >= 0.7
  size_t acceptable_quality_count = 0; // score >= 0.5
  size_t poor_quality_count = 0;       // score < 0.5

  std::vector<DataPointQuality> body_qualities;
  std::vector<std::string> system_warnings;
  std::vector<std::string> system_errors;

  [[nodiscard]] std::string to_string() const;
  [[nodiscard]] bool meets_quality_threshold(double threshold) const;
};

/**
 * @brief Historical quality trends and statistics
 */
struct QualityTrends {
  std::chrono::system_clock::time_point analysis_time;
  std::chrono::milliseconds analysis_window;
  size_t total_snapshots_analyzed = 0;

  // Trend analysis
  double quality_trend_slope = 0.0;        // Positive = improving, negative = degrading
  double latency_trend_slope = 0.0;        // Positive = increasing latency
  double reliability_score = 1.0;          // Overall system reliability

  // Statistical summaries
  double avg_quality_score = 1.0;
  double min_quality_score = 1.0;
  double max_quality_score = 1.0;
  double quality_std_deviation = 0.0;

  std::chrono::milliseconds avg_latency{0};
  std::chrono::milliseconds min_latency{0};
  std::chrono::milliseconds max_latency{0};

  // Problem frequency
  double error_rate = 0.0;              // Errors per snapshot
  double warning_rate = 0.0;            // Warnings per snapshot
  double anomaly_rate = 0.0;            // Anomalies per snapshot

  // Per-body analysis
  std::unordered_map<std::string, double> body_avg_quality;
  std::unordered_map<std::string, size_t> body_error_counts;
  std::unordered_map<std::string, size_t> body_warning_counts;

  [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Configuration for quality monitoring
 */
struct QualityMonitorConfig {
  // Quality thresholds
  double excellent_threshold = 0.9;
  double good_threshold = 0.7;
  double acceptable_threshold = 0.5;
  double critical_threshold = 0.3;

  // Latency thresholds
  std::chrono::milliseconds warning_latency{1000};
  std::chrono::milliseconds critical_latency{5000};

  // Monitoring behavior
  bool enable_trend_analysis = true;
  bool enable_anomaly_detection = true;
  bool enable_predictive_alerts = true;

  // Analysis windows
  std::chrono::minutes short_term_window{5};
  std::chrono::hours long_term_window{1};
  std::chrono::hours trend_analysis_window{24};

  // Alert thresholds
  double quality_degradation_threshold = 0.1;  // Trigger alert if quality drops by this much
  size_t consecutive_failures_threshold = 3;   // Alert after N consecutive failures
  double error_rate_threshold = 0.05;          // Alert if error rate exceeds 5%

  // Performance settings
  size_t max_history_size = 10000;             // Maximum snapshots to keep in history
  std::chrono::seconds analysis_interval{30};  // How often to run trend analysis
};

/**
 * @brief Callback function types for quality events
 */
using QualityAlertCallback = std::function<void(const std::string& alert_message,
                                               const SnapshotQuality& snapshot)>;
using TrendAnalysisCallback = std::function<void(const QualityTrends& trends)>;
using AnomalyDetectionCallback = std::function<void(const std::string& body_name,
                                                   const DataPointQuality& anomaly)>;

/**
 * @brief Comprehensive quality monitoring system for data streams
 */
class SOLAR_CORE_API QualityMonitor {
public:
  explicit QualityMonitor(QualityMonitorConfig config = {});
  ~QualityMonitor();

  // Non-copyable, non-movable (due to atomic members)
  QualityMonitor(const QualityMonitor&) = delete;
  QualityMonitor& operator=(const QualityMonitor&) = delete;
  QualityMonitor(QualityMonitor&&) = delete;
  QualityMonitor& operator=(QualityMonitor&&) = delete;

  // Configuration
  void set_config(const QualityMonitorConfig& config) { config_ = config; }
  [[nodiscard]] const QualityMonitorConfig& get_config() const noexcept { return config_; }

  // Monitoring control
  [[nodiscard]] bool start();
  void stop();
  [[nodiscard]] bool is_running() const noexcept { return running_.load(); }

  // Data processing
  void process_snapshot(const DataSnapshot& snapshot);
  void process_data_point(const DataPoint& data_point);

  // Quality assessment
  [[nodiscard]] SnapshotQuality assess_snapshot_quality(const DataSnapshot& snapshot) const;
  [[nodiscard]] DataPointQuality assess_data_point_quality(const DataPoint& data_point) const;

  // Statistics and trends
  [[nodiscard]] QualityTrends get_current_trends() const;
  [[nodiscard]] QualityTrends analyze_trends(std::chrono::milliseconds window) const;

  // Historical data
  [[nodiscard]] std::vector<SnapshotQuality> get_quality_history(
      std::chrono::milliseconds window = std::chrono::hours{1}) const;
  [[nodiscard]] std::vector<SnapshotQuality> get_quality_history(
      std::chrono::system_clock::time_point start,
      std::chrono::system_clock::time_point end) const;

  // Real-time status
  [[nodiscard]] SnapshotQuality get_latest_quality() const;
  [[nodiscard]] double get_current_quality_score() const;
  [[nodiscard]] std::chrono::milliseconds get_current_latency() const;

  // Callbacks
  void set_quality_alert_callback(QualityAlertCallback callback) {
    quality_alert_callback_ = std::move(callback);
  }
  void set_trend_analysis_callback(TrendAnalysisCallback callback) {
    trend_analysis_callback_ = std::move(callback);
  }
  void set_anomaly_detection_callback(AnomalyDetectionCallback callback) {
    anomaly_detection_callback_ = std::move(callback);
  }

  // Utility methods
  void reset_history();
  [[nodiscard]] std::string get_status_summary() const;
  [[nodiscard]] std::string get_quality_report() const;

private:
  QualityMonitorConfig config_;
  std::atomic<bool> running_{false};

  // Quality history
  std::vector<SnapshotQuality> quality_history_;
  mutable std::mutex history_mutex_;

  // Statistics tracking
  std::atomic<size_t> total_snapshots_processed_{0};
  std::atomic<size_t> total_errors_{0};
  std::atomic<size_t> total_warnings_{0};
  std::atomic<size_t> consecutive_failures_{0};

  // Per-body tracking
  std::unordered_map<std::string, std::vector<DataPointQuality>> body_quality_history_;
  mutable std::mutex body_history_mutex_;

  // Callbacks
  QualityAlertCallback quality_alert_callback_;
  TrendAnalysisCallback trend_analysis_callback_;
  AnomalyDetectionCallback anomaly_detection_callback_;

  // Background analysis
  std::unique_ptr<std::thread> analysis_thread_;
  std::atomic<bool> stop_analysis_{false};
  std::mutex analysis_mutex_;
  std::condition_variable analysis_cv_;

  // Quality assessment methods
  [[nodiscard]] double calculate_data_freshness(const DataPoint& data_point) const;
  [[nodiscard]] double calculate_data_accuracy(const DataPoint& data_point) const;
  [[nodiscard]] double calculate_data_completeness(const DataPoint& data_point) const;
  [[nodiscard]] double calculate_data_consistency(const DataPoint& data_point) const;
  [[nodiscard]] double calculate_overall_score(const DataPointQuality& quality) const;

  // Anomaly detection
  [[nodiscard]] bool detect_anomaly(const DataPoint& data_point) const;
  [[nodiscard]] bool is_outlier(const DataPoint& data_point, const std::string& body_name) const;

  // Trend analysis
  [[nodiscard]] double calculate_trend_slope(const std::vector<double>& values,
                                            const std::vector<std::chrono::system_clock::time_point>& times) const;

  // Alert management
  void check_quality_alerts(const SnapshotQuality& quality);
  void trigger_quality_alert(const std::string& message, const SnapshotQuality& quality);

  // Background analysis thread
  void analysis_loop();
  void perform_trend_analysis();
  void cleanup_old_history();

  // Utility methods
  void maintain_history_size();
  [[nodiscard]] std::vector<SnapshotQuality> filter_history_by_time(
      std::chrono::system_clock::time_point start,
      std::chrono::system_clock::time_point end) const;
};

/**
 * @brief Factory for creating quality monitors with common configurations
 */
class SOLAR_CORE_API QualityMonitorFactory {
public:
  [[nodiscard]] static std::unique_ptr<QualityMonitor> create_basic_monitor();
  [[nodiscard]] static std::unique_ptr<QualityMonitor> create_strict_monitor();
  [[nodiscard]] static std::unique_ptr<QualityMonitor> create_performance_monitor();
  [[nodiscard]] static std::unique_ptr<QualityMonitor> create_scientific_monitor();
  [[nodiscard]] static std::unique_ptr<QualityMonitor> create_realtime_monitor();
};

}  // namespace SolarSystem::Streaming
