#include "solar_core/streaming/quality_monitor.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

#include "solar_utils/logging.hpp"

namespace SolarSystem::Streaming {

using namespace SolarSystem::Utils;

// DataPointQuality implementation
std::string DataPointQuality::to_string() const {
  std::ostringstream oss;
  oss << "DataPointQuality {\n";
  oss << "  Body: " << body_name << "\n";
  oss << "  Overall Score: " << overall_score << "\n";
  oss << "  Data Freshness: " << data_freshness << "\n";
  oss << "  Data Accuracy: " << data_accuracy << "\n";
  oss << "  Data Completeness: " << data_completeness << "\n";
  oss << "  Data Consistency: " << data_consistency << "\n";
  oss << "  Latency: " << latency.count() << "ms\n";
  oss << "  Source: " << data_source << "\n";
  if (has_anomalies) oss << "  Has Anomalies: true\n";
  if (requires_attention) oss << "  Requires Attention: true\n";
  oss << "}";
  return oss.str();
}

// SnapshotQuality implementation
std::string SnapshotQuality::to_string() const {
  std::ostringstream oss;
  oss << "SnapshotQuality {\n";
  oss << "  Total Bodies: " << total_bodies << "\n";
  oss << "  Valid Bodies: " << valid_bodies << "\n";
  oss << "  Overall Score: " << overall_score << "\n";
  oss << "  Avg Quality: " << overall_score << "\n";
  oss << "  Avg Latency: " << avg_latency.count() << "ms\n";
  oss << "  Processing Time: " << processing_time.count() << "ms\n";
  oss << "  Excellent: " << excellent_quality_count << "\n";
  oss << "  Good: " << good_quality_count << "\n";
  oss << "  Acceptable: " << acceptable_quality_count << "\n";
  oss << "  Poor: " << poor_quality_count << "\n";
  oss << "}";
  return oss.str();
}

bool SnapshotQuality::meets_quality_threshold(double threshold) const {
  return overall_score >= threshold;
}

// QualityTrends implementation
std::string QualityTrends::to_string() const {
  std::ostringstream oss;
  oss << "QualityTrends {\n";
  oss << "  Analysis Window: " << analysis_window.count() << "ms\n";
  oss << "  Total Snapshots: " << total_snapshots_analyzed << "\n";
  oss << "  Quality Trend: " << quality_trend_slope << "\n";
  oss << "  Latency Trend: " << latency_trend_slope << "\n";
  oss << "  Reliability Score: " << reliability_score << "\n";
  oss << "  Avg Quality: " << avg_quality_score << "\n";
  oss << "  Error Rate: " << error_rate << "\n";
  oss << "  Warning Rate: " << warning_rate << "\n";
  oss << "}";
  return oss.str();
}

// QualityMonitor implementation
QualityMonitor::QualityMonitor(QualityMonitorConfig config) : config_(std::move(config)) {}

QualityMonitor::~QualityMonitor() {
  if (running_.load()) {
    stop();
  }
}

bool QualityMonitor::start() {
  if (running_.load()) {
    return true;
  }

  running_.store(true);
  stop_analysis_.store(false);

  // Start background analysis thread
  if (config_.enable_trend_analysis) {
    analysis_thread_ = std::make_unique<std::thread>(&QualityMonitor::analysis_loop, this);
  }

  LOG_INFO("QualityMonitor", "Quality monitor started");
  return true;
}

void QualityMonitor::stop() {
  if (!running_.load()) {
    return;
  }

  running_.store(false);
  stop_analysis_.store(true);

  // Wake up analysis thread
  analysis_cv_.notify_all();

  // Wait for analysis thread to finish
  if (analysis_thread_ && analysis_thread_->joinable()) {
    analysis_thread_->join();
    analysis_thread_.reset();
  }

  LOG_INFO("QualityMonitor", "Quality monitor stopped");
}

void QualityMonitor::process_snapshot(const DataSnapshot& snapshot) {
  if (!running_.load()) {
    return;
  }

  total_snapshots_processed_.fetch_add(1);

  // Assess snapshot quality
  auto quality = assess_snapshot_quality(snapshot);

  // Store in history
  {
    std::lock_guard<std::mutex> lock(history_mutex_);
    quality_history_.push_back(quality);
    maintain_history_size();
  }

  // Process individual data points
  for (const auto& point : snapshot.data_points) {
    process_data_point(point);
  }

  // Check for quality alerts
  check_quality_alerts(quality);
}

void QualityMonitor::process_data_point(const DataPoint& data_point) {
  if (!running_.load()) {
    return;
  }

  // Assess data point quality
  auto quality = assess_data_point_quality(data_point);

  // Store in per-body history
  {
    std::lock_guard<std::mutex> lock(body_history_mutex_);
    auto& body_history = body_quality_history_[data_point.body_name];
    body_history.push_back(quality);

    // Limit per-body history size
    if (body_history.size() > config_.max_history_size / 10) {  // 10% of total per body
      body_history.erase(body_history.begin());
    }
  }

  // Check for anomalies
  if (config_.enable_anomaly_detection && detect_anomaly(data_point)) {
    if (anomaly_detection_callback_) {
      anomaly_detection_callback_(data_point.body_name, quality);
    }
  }
}

SnapshotQuality QualityMonitor::assess_snapshot_quality(const DataSnapshot& snapshot) const {
  SnapshotQuality quality;
  quality.timestamp = snapshot.timestamp;
  quality.total_bodies = snapshot.data_points.size();
  quality.processing_time = snapshot.processing_time;

  if (snapshot.data_points.empty()) {
    return quality;
  }

  // Assess individual data points
  double total_quality = 0.0;
  std::chrono::milliseconds total_latency{0};
  std::chrono::milliseconds max_latency{0};

  for (const auto& point : snapshot.data_points) {
    auto point_quality = assess_data_point_quality(point);
    quality.body_qualities.push_back(point_quality);

    total_quality += point_quality.overall_score;
    total_latency += point_quality.latency;

    if (point_quality.latency > max_latency) {
      max_latency = point_quality.latency;
    }

    // Count quality distribution
    if (point_quality.overall_score >= config_.excellent_threshold) {
      quality.excellent_quality_count++;
    } else if (point_quality.overall_score >= config_.good_threshold) {
      quality.good_quality_count++;
    } else if (point_quality.overall_score >= config_.acceptable_threshold) {
      quality.acceptable_quality_count++;
    } else {
      quality.poor_quality_count++;
    }

    if (point_quality.overall_score >= config_.acceptable_threshold) {
      quality.valid_bodies++;
    }

    if (!point_quality.warnings.empty()) {
      quality.bodies_with_warnings++;
    }

    if (!point_quality.errors.empty()) {
      quality.bodies_with_errors++;
    }
  }

  // Calculate aggregate metrics
  quality.overall_score = total_quality / snapshot.data_points.size();
  quality.avg_data_freshness = quality.overall_score;  // Simplified
  quality.avg_data_accuracy = quality.overall_score;   // Simplified
  quality.avg_data_completeness = quality.overall_score;  // Simplified
  quality.avg_data_consistency = quality.overall_score;   // Simplified

  quality.total_latency = total_latency;
  quality.avg_latency = total_latency / snapshot.data_points.size();
  quality.max_latency = max_latency;

  return quality;
}

DataPointQuality QualityMonitor::assess_data_point_quality(const DataPoint& data_point) const {
  DataPointQuality quality;
  quality.body_name = data_point.body_name;
  quality.timestamp = data_point.timestamp;
  quality.data_source = data_point.data_source;
  quality.latency = data_point.latency;

  // Calculate individual quality metrics
  quality.data_freshness = calculate_data_freshness(data_point);
  quality.data_accuracy = calculate_data_accuracy(data_point);
  quality.data_completeness = calculate_data_completeness(data_point);
  quality.data_consistency = calculate_data_consistency(data_point);

  // Calculate overall score
  quality.overall_score = calculate_overall_score(quality);

  // Check for issues
  if (quality.overall_score < config_.critical_threshold) {
    quality.requires_attention = true;
    quality.errors.push_back("Quality below critical threshold");
  } else if (quality.overall_score < config_.acceptable_threshold) {
    quality.warnings.push_back("Quality below acceptable threshold");
  }

  if (data_point.latency > config_.critical_latency) {
    quality.requires_attention = true;
    quality.errors.push_back("Latency exceeds critical threshold");
  } else if (data_point.latency > config_.warning_latency) {
    quality.warnings.push_back("Latency exceeds warning threshold");
  }

  // Check for anomalies
  quality.has_anomalies = detect_anomaly(data_point);

  return quality;
}

QualityTrends QualityMonitor::get_current_trends() const {
  return analyze_trends(config_.trend_analysis_window);
}

QualityTrends QualityMonitor::analyze_trends(std::chrono::milliseconds window) const {
  std::lock_guard<std::mutex> lock(history_mutex_);

  QualityTrends trends;
  trends.analysis_time = std::chrono::system_clock::now();
  trends.analysis_window = window;

  // Filter history by time window
  auto cutoff_time = trends.analysis_time - window;
  std::vector<SnapshotQuality> recent_history;

  std::copy_if(quality_history_.begin(), quality_history_.end(),
               std::back_inserter(recent_history),
               [cutoff_time](const SnapshotQuality& quality) {
                 return quality.timestamp >= cutoff_time;
               });

  trends.total_snapshots_analyzed = recent_history.size();

  if (recent_history.empty()) {
    return trends;
  }

  // Calculate basic statistics
  std::vector<double> quality_scores;
  std::vector<std::chrono::system_clock::time_point> timestamps;

  for (const auto& quality : recent_history) {
    quality_scores.push_back(quality.overall_score);
    timestamps.push_back(quality.timestamp);
  }

  trends.avg_quality_score = std::accumulate(quality_scores.begin(), quality_scores.end(), 0.0) / quality_scores.size();
  trends.min_quality_score = *std::min_element(quality_scores.begin(), quality_scores.end());
  trends.max_quality_score = *std::max_element(quality_scores.begin(), quality_scores.end());

  // Calculate trend slope (simplified linear regression)
  if (quality_scores.size() > 1) {
    trends.quality_trend_slope = calculate_trend_slope(quality_scores, timestamps);
  }

  // Calculate error and warning rates
  size_t total_errors = 0;
  size_t total_warnings = 0;

  for (const auto& quality : recent_history) {
    total_errors += quality.bodies_with_errors;
    total_warnings += quality.bodies_with_warnings;
  }

  trends.error_rate = static_cast<double>(total_errors) / recent_history.size();
  trends.warning_rate = static_cast<double>(total_warnings) / recent_history.size();

  // Calculate reliability score
  trends.reliability_score = std::max(0.0, 1.0 - trends.error_rate - (trends.warning_rate * 0.5));

  return trends;
}

std::vector<SnapshotQuality> QualityMonitor::get_quality_history(std::chrono::milliseconds window) const {
  auto now = std::chrono::system_clock::now();
  return get_quality_history(now - window, now);
}

std::vector<SnapshotQuality> QualityMonitor::get_quality_history(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
  std::lock_guard<std::mutex> lock(history_mutex_);
  return filter_history_by_time(start, end);
}

SnapshotQuality QualityMonitor::get_latest_quality() const {
  std::lock_guard<std::mutex> lock(history_mutex_);

  if (quality_history_.empty()) {
    return SnapshotQuality{};
  }

  return quality_history_.back();
}

double QualityMonitor::get_current_quality_score() const {
  auto latest = get_latest_quality();
  return latest.overall_score;
}

std::chrono::milliseconds QualityMonitor::get_current_latency() const {
  auto latest = get_latest_quality();
  return latest.avg_latency;
}

void QualityMonitor::reset_history() {
  std::lock_guard<std::mutex> history_lock(history_mutex_);
  std::lock_guard<std::mutex> body_lock(body_history_mutex_);

  quality_history_.clear();
  body_quality_history_.clear();

  total_snapshots_processed_.store(0);
  total_errors_.store(0);
  total_warnings_.store(0);
  consecutive_failures_.store(0);
}

std::string QualityMonitor::get_status_summary() const {
  std::ostringstream oss;
  oss << "QualityMonitor Status:\n";
  oss << "  Running: " << (running_.load() ? "Yes" : "No") << "\n";
  oss << "  Total Snapshots: " << total_snapshots_processed_.load() << "\n";
  oss << "  Total Errors: " << total_errors_.load() << "\n";
  oss << "  Total Warnings: " << total_warnings_.load() << "\n";
  oss << "  Current Quality: " << get_current_quality_score() << "\n";
  oss << "  Current Latency: " << get_current_latency().count() << "ms\n";
  return oss.str();
}

std::string QualityMonitor::get_quality_report() const {
  auto trends = get_current_trends();
  std::ostringstream oss;
  oss << "Quality Report:\n";
  oss << "  Avg Quality: " << std::fixed << std::setprecision(3) << trends.avg_quality_score << "\n";
  oss << "  Quality Range: " << trends.min_quality_score << " - " << trends.max_quality_score << "\n";
  oss << "  Reliability: " << trends.reliability_score << "\n";
  oss << "  Error Rate: " << trends.error_rate << "\n";
  oss << "  Warning Rate: " << trends.warning_rate << "\n";
  return oss.str();
}

// Private helper methods
double QualityMonitor::calculate_data_freshness(const DataPoint& data_point) const {
  // Simplified: based on latency
  auto max_acceptable_latency = config_.warning_latency;
  if (data_point.latency <= max_acceptable_latency) {
    return 1.0;
  } else {
    // Exponential decay based on latency
    double ratio = static_cast<double>(data_point.latency.count()) / max_acceptable_latency.count();
    return std::exp(-ratio + 1.0);
  }
}

double QualityMonitor::calculate_data_accuracy(const DataPoint& data_point) const {
  // Simplified: check for invalid values
  double accuracy = 1.0;

  // Check for NaN or infinite values
  if (std::isnan(data_point.position.x()) || std::isnan(data_point.position.y()) ||
      std::isnan(data_point.position.z()) || std::isinf(data_point.position.x()) ||
      std::isinf(data_point.position.y()) || std::isinf(data_point.position.z())) {
    accuracy *= 0.1;
  }

  if (std::isnan(data_point.velocity.x()) || std::isnan(data_point.velocity.y()) ||
      std::isnan(data_point.velocity.z()) || std::isinf(data_point.velocity.x()) ||
      std::isinf(data_point.velocity.y()) || std::isinf(data_point.velocity.z())) {
    accuracy *= 0.1;
  }

  if (data_point.mass <= 0.0 || std::isnan(data_point.mass) || std::isinf(data_point.mass)) {
    accuracy *= 0.5;
  }

  return accuracy;
}

double QualityMonitor::calculate_data_completeness(const DataPoint& data_point) const {
  // Simplified: check if all required fields are present
  double completeness = 1.0;

  if (data_point.body_name.empty()) {
    completeness *= 0.5;
  }

  if (data_point.data_source.empty()) {
    completeness *= 0.9;
  }

  return completeness;
}

double QualityMonitor::calculate_data_consistency(const DataPoint& data_point) const {
  // Simplified: assume consistent for now
  // In a real implementation, this would compare with historical data
  return 1.0;
}

double QualityMonitor::calculate_overall_score(const DataPointQuality& quality) const {
  // Weighted average of quality metrics
  return (quality.data_freshness * 0.3 +
          quality.data_accuracy * 0.4 +
          quality.data_completeness * 0.2 +
          quality.data_consistency * 0.1);
}

bool QualityMonitor::detect_anomaly(const DataPoint& data_point) const {
  // Simplified anomaly detection
  return calculate_data_accuracy(data_point) < 0.5;
}

bool QualityMonitor::is_outlier(const DataPoint& data_point, const std::string& body_name) const {
  // Simplified outlier detection
  return false;
}

double QualityMonitor::calculate_trend_slope(
    const std::vector<double>& values,
    const std::vector<std::chrono::system_clock::time_point>& times) const {

  if (values.size() != times.size() || values.size() < 2) {
    return 0.0;
  }

  // Simple linear regression slope calculation
  size_t n = values.size();
  double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

  auto start_time = times[0];

  for (size_t i = 0; i < n; ++i) {
    double x = std::chrono::duration_cast<std::chrono::seconds>(times[i] - start_time).count();
    double y = values[i];

    sum_x += x;
    sum_y += y;
    sum_xy += x * y;
    sum_x2 += x * x;
  }

  double denominator = n * sum_x2 - sum_x * sum_x;
  if (std::abs(denominator) < 1e-10) {
    return 0.0;
  }

  return (n * sum_xy - sum_x * sum_y) / denominator;
}

void QualityMonitor::check_quality_alerts(const SnapshotQuality& quality) {
  // Check for quality degradation
  if (quality.overall_score < config_.critical_threshold) {
    total_errors_.fetch_add(1);
    consecutive_failures_.fetch_add(1);

    trigger_quality_alert("Quality below critical threshold", quality);
  } else {
    consecutive_failures_.store(0);
  }

  // Check for consecutive failures
  if (consecutive_failures_.load() >= config_.consecutive_failures_threshold) {
    trigger_quality_alert("Consecutive quality failures detected", quality);
  }

  // Check error rate
  auto trends = get_current_trends();
  if (trends.error_rate > config_.error_rate_threshold) {
    trigger_quality_alert("Error rate exceeds threshold", quality);
  }
}

void QualityMonitor::trigger_quality_alert(const std::string& message, const SnapshotQuality& quality) {
  if (quality_alert_callback_) {
    quality_alert_callback_(message, quality);
  }

  LOG_WARN("QualityMonitor", "Quality alert: " + message);
}

void QualityMonitor::analysis_loop() {
  LOG_INFO("QualityMonitor", "Analysis thread started");

  while (!stop_analysis_.load()) {
    try {
      std::unique_lock<std::mutex> lock(analysis_mutex_);

      // Wait for analysis interval or stop signal
      if (analysis_cv_.wait_for(lock, config_.analysis_interval, [this] {
            return stop_analysis_.load();
          })) {
        break;  // Stop requested
      }

      lock.unlock();

      // Perform trend analysis
      perform_trend_analysis();

      // Cleanup old history
      cleanup_old_history();

    } catch (const std::exception& e) {
      LOG_ERROR("QualityMonitor", "Analysis thread error: " + std::string(e.what()));
    }
  }

  LOG_INFO("QualityMonitor", "Analysis thread stopped");
}

void QualityMonitor::perform_trend_analysis() {
  if (!config_.enable_trend_analysis) {
    return;
  }

  auto trends = get_current_trends();

  if (trend_analysis_callback_) {
    trend_analysis_callback_(trends);
  }

  // Log significant trends
  if (std::abs(trends.quality_trend_slope) > 0.01) {  // Significant trend
    std::string direction = trends.quality_trend_slope > 0 ? "improving" : "degrading";
    LOG_INFO("QualityMonitor", "Quality trend detected: " + direction +
             " (slope: " + std::to_string(trends.quality_trend_slope) + ")");
  }
}

void QualityMonitor::cleanup_old_history() {
  maintain_history_size();
}

void QualityMonitor::maintain_history_size() {
  // Remove old entries if history is too large
  while (quality_history_.size() > config_.max_history_size) {
    quality_history_.erase(quality_history_.begin());
  }
}

std::vector<SnapshotQuality> QualityMonitor::filter_history_by_time(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {

  std::vector<SnapshotQuality> filtered;

  std::copy_if(quality_history_.begin(), quality_history_.end(),
               std::back_inserter(filtered),
               [start, end](const SnapshotQuality& quality) {
                 return quality.timestamp >= start && quality.timestamp <= end;
               });

  return filtered;
}

// QualityMonitorFactory implementation
std::unique_ptr<QualityMonitor> QualityMonitorFactory::create_basic_monitor() {
  QualityMonitorConfig config;
  config.acceptable_threshold = 0.5;
  config.enable_trend_analysis = false;
  config.enable_anomaly_detection = false;
  return std::make_unique<QualityMonitor>(config);
}

std::unique_ptr<QualityMonitor> QualityMonitorFactory::create_strict_monitor() {
  QualityMonitorConfig config;
  config.excellent_threshold = 0.95;
  config.good_threshold = 0.85;
  config.acceptable_threshold = 0.75;
  config.critical_threshold = 0.5;
  config.warning_latency = std::chrono::milliseconds{500};
  config.critical_latency = std::chrono::milliseconds{2000};
  return std::make_unique<QualityMonitor>(config);
}

std::unique_ptr<QualityMonitor> QualityMonitorFactory::create_performance_monitor() {
  QualityMonitorConfig config;
  config.warning_latency = std::chrono::milliseconds{100};
  config.critical_latency = std::chrono::milliseconds{500};
  config.analysis_interval = std::chrono::seconds{10};
  return std::make_unique<QualityMonitor>(config);
}

std::unique_ptr<QualityMonitor> QualityMonitorFactory::create_scientific_monitor() {
  QualityMonitorConfig config;
  config.excellent_threshold = 0.98;
  config.good_threshold = 0.95;
  config.acceptable_threshold = 0.9;
  config.enable_trend_analysis = true;
  config.enable_anomaly_detection = true;
  config.enable_predictive_alerts = true;
  return std::make_unique<QualityMonitor>(config);
}

std::unique_ptr<QualityMonitor> QualityMonitorFactory::create_realtime_monitor() {
  QualityMonitorConfig config;
  config.acceptable_threshold = 0.6;
  config.warning_latency = std::chrono::milliseconds{1000};
  config.critical_latency = std::chrono::milliseconds{3000};
  config.analysis_interval = std::chrono::seconds{5};
  config.enable_trend_analysis = true;
  return std::make_unique<QualityMonitor>(config);
}

}  // namespace SolarSystem::Streaming
