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
  quality.overall_score = total_quality / static_cast<double>(snapshot.data_points.size());

  // Calculate individual metric averages
  double total_freshness = 0.0;
  double total_accuracy = 0.0;
  double total_completeness = 0.0;
  double total_consistency = 0.0;

  for (const auto& point_quality : quality.body_qualities) {
    total_freshness += point_quality.data_freshness;
    total_accuracy += point_quality.data_accuracy;
    total_completeness += point_quality.data_completeness;
    total_consistency += point_quality.data_consistency;
  }

  size_t count = quality.body_qualities.size();
  quality.avg_data_freshness = count > 0 ? total_freshness / static_cast<double>(count) : 0.0;
  quality.avg_data_accuracy = count > 0 ? total_accuracy / static_cast<double>(count) : 0.0;
  quality.avg_data_completeness = count > 0 ? total_completeness / static_cast<double>(count) : 0.0;
  quality.avg_data_consistency = count > 0 ? total_consistency / static_cast<double>(count) : 0.0;

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

  std::copy_if(
      quality_history_.begin(), quality_history_.end(), std::back_inserter(recent_history),
      [cutoff_time](const SnapshotQuality& quality) { return quality.timestamp >= cutoff_time; });

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

  trends.avg_quality_score = std::accumulate(quality_scores.begin(), quality_scores.end(), 0.0) /
                             static_cast<double>(quality_scores.size());
  trends.min_quality_score = *std::min_element(quality_scores.begin(), quality_scores.end());
  trends.max_quality_score = *std::max_element(quality_scores.begin(), quality_scores.end());

  // Calculate trend slope using linear regression
  if (quality_scores.size() > 1) {
    trends.quality_trend_slope = calculate_trend_slope(quality_scores, timestamps);

    // Also calculate latency trend
    std::vector<double> latency_values;
    for (const auto& quality : recent_history) {
      latency_values.push_back(static_cast<double>(quality.avg_latency.count()));
    }
    trends.latency_trend_slope = calculate_trend_slope(latency_values, timestamps);
  }

  // Calculate error and warning rates
  size_t total_errors = 0;
  size_t total_warnings = 0;

  for (const auto& quality : recent_history) {
    total_errors += quality.bodies_with_errors;
    total_warnings += quality.bodies_with_warnings;
  }

  trends.error_rate =
      static_cast<double>(total_errors) / static_cast<double>(recent_history.size());
  trends.warning_rate =
      static_cast<double>(total_warnings) / static_cast<double>(recent_history.size());

  // Calculate reliability score
  trends.reliability_score = std::max(0.0, 1.0 - trends.error_rate - (trends.warning_rate * 0.5));

  return trends;
}

std::vector<SnapshotQuality> QualityMonitor::get_quality_history(
    std::chrono::milliseconds window) const {
  auto now = std::chrono::system_clock::now();
  return get_quality_history(now - window, now);
}

std::vector<SnapshotQuality> QualityMonitor::get_quality_history(
    std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end) const {
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
  oss << "  Avg Quality: " << std::fixed << trends.avg_quality_score << "\n";
  oss << "  Quality Range: " << trends.min_quality_score << " - " << trends.max_quality_score
      << "\n";
  oss << "  Reliability: " << trends.reliability_score << "\n";
  oss << "  Error Rate: " << trends.error_rate << "\n";
  oss << "  Warning Rate: " << trends.warning_rate << "\n";
  return oss.str();
}

// Private helper methods
double QualityMonitor::calculate_data_freshness(const DataPoint& data_point) const {
  // Comprehensive age/staleness checking

  // 1. Check data age (time since data was generated)
  auto now = std::chrono::system_clock::now();
  auto data_age = std::chrono::duration_cast<std::chrono::milliseconds>(now - data_point.timestamp);

  // 2. Check latency (time to receive data)
  auto latency = data_point.latency;

  // 3. Calculate freshness score based on both age and latency
  double age_score = 1.0;
  double latency_score = 1.0;

  // Age scoring with exponential decay
  auto max_acceptable_age = config_.warning_latency * 2;  // Allow 2x warning latency for age
  if (data_age > max_acceptable_age) {
    double age_ratio =
        static_cast<double>(data_age.count()) / static_cast<double>(max_acceptable_age.count());
    age_score = std::exp(-age_ratio + 1.0);  // Exponential decay
  }

  // Latency scoring with exponential decay
  if (latency > config_.warning_latency) {
    double latency_ratio =
        static_cast<double>(latency.count()) / static_cast<double>(config_.warning_latency.count());
    latency_score = std::exp(-latency_ratio + 1.0);  // Exponential decay
  }

  // 4. Check for stale data (data that hasn't been updated recently)
  double staleness_penalty = 1.0;
  {
    std::lock_guard<std::mutex> lock(body_history_mutex_);
    auto it = body_quality_history_.find(data_point.body_name);
    if (it != body_quality_history_.end() && !it->second.empty()) {
      const auto& last_quality = it->second.back();
      auto time_since_last_update = std::chrono::duration_cast<std::chrono::milliseconds>(
          data_point.timestamp - last_quality.timestamp);

      // Penalize if updates are too infrequent
      auto expected_update_interval = config_.warning_latency;
      if (time_since_last_update > expected_update_interval * 3) {
        staleness_penalty = 0.7;  // 30% penalty for stale data
      } else if (time_since_last_update > expected_update_interval * 2) {
        staleness_penalty = 0.85;  // 15% penalty
      }
    }
  }

  // Combined freshness score (weighted average)
  return (age_score * 0.4 + latency_score * 0.4 + staleness_penalty * 0.2);
}

double QualityMonitor::calculate_data_accuracy(const DataPoint& data_point) const {
  // Comprehensive accuracy assessment with range validation and historical comparison
  double accuracy = 1.0;

  // 1. Check for NaN or infinite values (critical errors)
  if (std::isnan(data_point.position.x()) || std::isnan(data_point.position.y()) ||
      std::isnan(data_point.position.z()) || std::isinf(data_point.position.x()) ||
      std::isinf(data_point.position.y()) || std::isinf(data_point.position.z())) {
    return 0.0;  // Invalid data
  }

  if (std::isnan(data_point.velocity.x()) || std::isnan(data_point.velocity.y()) ||
      std::isnan(data_point.velocity.z()) || std::isinf(data_point.velocity.x()) ||
      std::isinf(data_point.velocity.y()) || std::isinf(data_point.velocity.z())) {
    return 0.0;  // Invalid data
  }

  if (data_point.mass <= 0.0 || std::isnan(data_point.mass) || std::isinf(data_point.mass)) {
    accuracy *= 0.3;  // Severe penalty for invalid mass
  }

  // 2. Range validation - check if values are within physically reasonable bounds
  // Position should be within solar system bounds (roughly ±100 AU)
  const double MAX_POSITION = 100.0 * 1.496e11;  // 100 AU in meters
  double position_magnitude =
      static_cast<double>(std::sqrt(data_point.position.x() * data_point.position.x() +
                                    data_point.position.y() * data_point.position.y() +
                                    data_point.position.z() * data_point.position.z()));

  if (position_magnitude > MAX_POSITION) {
    accuracy *= 0.5;  // Position seems unreasonable
  }

  // Velocity should be within reasonable bounds (< 100 km/s for solar system objects)
  const double MAX_VELOCITY = 100000.0;  // 100 km/s in m/s
  double velocity_magnitude =
      static_cast<double>(std::sqrt(data_point.velocity.x() * data_point.velocity.x() +
                                    data_point.velocity.y() * data_point.velocity.y() +
                                    data_point.velocity.z() * data_point.velocity.z()));

  if (velocity_magnitude > MAX_VELOCITY) {
    accuracy *= 0.7;  // Velocity seems high but possible
  }

  // Mass should be within reasonable bounds (1 kg to 2e30 kg - Sun's mass)
  const double MIN_MASS = 1.0;
  const double MAX_MASS = 2.0e30;
  if (data_point.mass < MIN_MASS || data_point.mass > MAX_MASS) {
    accuracy *= 0.6;
  }

  // 3. Historical comparison - check consistency with previous values
  {
    std::lock_guard<std::mutex> lock(body_history_mutex_);
    auto it = body_quality_history_.find(data_point.body_name);
    if (it != body_quality_history_.end() && it->second.size() >= 3) {
      // Get recent history
      const auto& history = it->second;

      // Check if quality scores show sudden degradation
      double recent_avg = 0.0;
      for (size_t i = history.size() - 3; i < history.size(); ++i) {
        recent_avg += history[i].overall_score;
      }
      recent_avg /= 3.0;

      // If historical quality was good but current is poor, penalize
      if (recent_avg > 0.8 && accuracy < 0.5) {
        accuracy *= 0.8;  // Sudden quality drop is suspicious
      }
    }
  }

  // 4. Cross-validation with expected orbital mechanics
  // Check if velocity magnitude is reasonable for the position
  // Objects farther from sun should generally move slower (Kepler's third law approximation)
  if (position_magnitude > 1e10) {  // Beyond 0.1 AU
    double expected_max_velocity = 50000.0 * std::sqrt(1.5e11 / position_magnitude);
    if (velocity_magnitude > expected_max_velocity * 2.0) {
      accuracy *= 0.85;  // Velocity inconsistent with position
    }
  }

  return std::max(0.0, std::min(1.0, accuracy));
}

double QualityMonitor::calculate_data_completeness(const DataPoint& data_point) const {
  // Full field validation for data completeness
  int total_fields = 0;
  int complete_fields = 0;

  // 1. Required fields
  total_fields++;
  if (!data_point.body_name.empty()) {
    complete_fields++;
  }

  total_fields++;
  if (!data_point.data_source.empty()) {
    complete_fields++;
  }

  // 2. Position data (3 components)
  total_fields += 3;
  if (!std::isnan(data_point.position.x()) && !std::isinf(data_point.position.x())) {
    complete_fields++;
  }
  if (!std::isnan(data_point.position.y()) && !std::isinf(data_point.position.y())) {
    complete_fields++;
  }
  if (!std::isnan(data_point.position.z()) && !std::isinf(data_point.position.z())) {
    complete_fields++;
  }

  // 3. Velocity data (3 components)
  total_fields += 3;
  if (!std::isnan(data_point.velocity.x()) && !std::isinf(data_point.velocity.x())) {
    complete_fields++;
  }
  if (!std::isnan(data_point.velocity.y()) && !std::isinf(data_point.velocity.y())) {
    complete_fields++;
  }
  if (!std::isnan(data_point.velocity.z()) && !std::isinf(data_point.velocity.z())) {
    complete_fields++;
  }

  // 4. Mass data
  total_fields++;
  if (data_point.mass > 0.0 && !std::isnan(data_point.mass) && !std::isinf(data_point.mass)) {
    complete_fields++;
  }

  // 5. Timestamp
  total_fields++;
  if (data_point.timestamp != std::chrono::system_clock::time_point{}) {
    complete_fields++;
  }

  // 6. Optional but valuable fields
  // These don't penalize as much if missing
  int optional_fields = 0;
  int complete_optional = 0;

  optional_fields++;
  if (data_point.latency.count() >= 0) {
    complete_optional++;
  }

  // Calculate completeness score
  double required_completeness = static_cast<double>(complete_fields) / total_fields;
  double optional_completeness =
      optional_fields > 0 ? static_cast<double>(complete_optional) / optional_fields : 1.0;

  // Weighted combination (required fields are more important)
  return required_completeness * 0.9 + optional_completeness * 0.1;
}

double QualityMonitor::calculate_data_consistency(const DataPoint& data_point) const {
  // Cross-source validation and temporal consistency checking
  double consistency = 1.0;

  std::lock_guard<std::mutex> lock(body_history_mutex_);
  auto it = body_quality_history_.find(data_point.body_name);

  if (it == body_quality_history_.end() || it->second.empty()) {
    // No history to compare against - assume consistent
    return 1.0;
  }

  const auto& history = it->second;

  // 1. Temporal consistency - check if data follows expected patterns
  if (history.size() >= 2) {
    const auto& prev = history.back();

    // Check timestamp ordering
    if (data_point.timestamp < prev.timestamp) {
      consistency *= 0.5;  // Out-of-order data is suspicious
    }

    // Check for reasonable time gaps
    auto time_gap = std::chrono::duration_cast<std::chrono::milliseconds>(data_point.timestamp -
                                                                          prev.timestamp);

    if (time_gap > config_.warning_latency * 10) {
      consistency *= 0.8;  // Large time gap suggests missing data
    } else if (time_gap < std::chrono::milliseconds(1)) {
      consistency *= 0.9;  // Duplicate or too-frequent updates
    }
  }

  // 2. Value consistency - check if values change smoothly
  if (history.size() >= 3) {
    // Calculate variance in recent quality scores
    size_t recent_count = std::min(size_t(5), history.size());
    std::vector<double> recent_scores;

    for (size_t i = history.size() - recent_count; i < history.size(); ++i) {
      recent_scores.push_back(history[i].overall_score);
    }

    // Calculate mean and standard deviation
    double mean = std::accumulate(recent_scores.begin(), recent_scores.end(), 0.0) /
                  static_cast<double>(recent_scores.size());
    double sq_sum = 0.0;
    for (double score : recent_scores) {
      sq_sum += (score - mean) * (score - mean);
    }
    double std_dev = std::sqrt(sq_sum / static_cast<double>(recent_scores.size()));

    // High variance suggests inconsistent data
    if (std_dev > 0.3) {
      consistency *= 0.7;  // High variance penalty
    } else if (std_dev > 0.2) {
      consistency *= 0.85;  // Moderate variance penalty
    }
  }

  // 3. Source consistency - check if data source is consistent
  if (history.size() >= 1) {
    const auto& prev = history.back();
    if (data_point.data_source != prev.data_source) {
      consistency *= 0.95;  // Small penalty for source changes
    }
  }

  // 4. Pattern consistency - check for expected patterns
  // Orbital data should follow smooth curves - check for sudden jumps
  if (history.size() >= 5) {
    // Calculate smoothness by checking variance in quality changes
    std::vector<double> quality_changes;
    for (size_t i = 1; i < std::min(size_t(10), history.size()); ++i) {
      double change = std::abs(history[history.size() - i].overall_score -
                               history[history.size() - i - 1].overall_score);
      quality_changes.push_back(change);
    }

    // Calculate variance of changes
    double mean_change = std::accumulate(quality_changes.begin(), quality_changes.end(), 0.0) /
                         static_cast<double>(quality_changes.size());
    double variance = 0.0;
    for (double change : quality_changes) {
      variance += (change - mean_change) * (change - mean_change);
    }
    variance /= static_cast<double>(quality_changes.size());

    // High variance in changes suggests erratic/inconsistent data
    if (variance > 0.1) {
      consistency *= 0.8;  // Erratic pattern penalty
    }
  }

  return std::max(0.0, std::min(1.0, consistency));
}

double QualityMonitor::calculate_overall_score(const DataPointQuality& quality) const {
  // Weighted average of quality metrics
  return (quality.data_freshness * 0.3 + quality.data_accuracy * 0.4 +
          quality.data_completeness * 0.2 + quality.data_consistency * 0.1);
}

bool QualityMonitor::detect_anomaly(const DataPoint& data_point) const {
  // Statistical anomaly detection using Z-score and IQR methods

  // Quick check for obvious anomalies
  if (calculate_data_accuracy(data_point) < 0.3) {
    return true;  // Clearly anomalous data
  }

  std::lock_guard<std::mutex> lock(body_history_mutex_);
  auto it = body_quality_history_.find(data_point.body_name);

  if (it == body_quality_history_.end() || it->second.size() < 10) {
    // Not enough history for statistical analysis
    return false;
  }

  const auto& history = it->second;

  // Collect recent quality scores for statistical analysis
  std::vector<double> scores;
  size_t window_size = std::min(size_t(30), history.size());

  for (size_t i = history.size() - window_size; i < history.size(); ++i) {
    scores.push_back(history[i].overall_score);
  }

  // Method 1: Z-score anomaly detection
  // Calculate mean and standard deviation
  double mean =
      std::accumulate(scores.begin(), scores.end(), 0.0) / static_cast<double>(scores.size());
  double sq_sum = 0.0;
  for (double score : scores) {
    sq_sum += (score - mean) * (score - mean);
  }
  double std_dev = std::sqrt(sq_sum / static_cast<double>(scores.size()));

  // Calculate current quality score
  double current_score = calculate_data_accuracy(data_point);

  // Z-score threshold (typically 3.0 for outliers, 2.5 for anomalies)
  const double Z_SCORE_THRESHOLD = 2.5;

  if (std_dev > 1e-6) {  // Avoid division by zero
    double z_score = std::abs((current_score - mean) / std_dev);
    if (z_score > Z_SCORE_THRESHOLD) {
      return true;  // Anomaly detected by Z-score
    }
  }

  // Method 2: IQR (Interquartile Range) method
  // Sort scores to find quartiles
  std::vector<double> sorted_scores = scores;
  std::sort(sorted_scores.begin(), sorted_scores.end());

  size_t n = sorted_scores.size();
  double q1 = sorted_scores[n / 4];
  double q3 = sorted_scores[3 * n / 4];
  double iqr = q3 - q1;

  // IQR outlier detection (1.5 * IQR is standard, 3.0 * IQR for extreme outliers)
  const double IQR_MULTIPLIER = 1.5;
  double lower_bound = q1 - IQR_MULTIPLIER * iqr;
  double upper_bound = q3 + IQR_MULTIPLIER * iqr;

  if (current_score < lower_bound || current_score > upper_bound) {
    return true;  // Anomaly detected by IQR method
  }

  // Method 3: Sudden change detection
  // Check if there's a sudden drop or spike compared to recent average
  if (history.size() >= 3) {
    double recent_avg =
        (history[history.size() - 1].overall_score + history[history.size() - 2].overall_score +
         history[history.size() - 3].overall_score) /
        3.0;

    double change_ratio = std::abs(current_score - recent_avg) / (recent_avg + 1e-6);

    if (change_ratio > 0.5) {  // 50% change is suspicious
      return true;             // Sudden change detected
    }
  }

  return false;  // No anomaly detected
}

bool QualityMonitor::is_outlier(const DataPoint& data_point, const std::string& metric_name) const {
  // Robust outlier detection using Tukey's fences method

  std::lock_guard<std::mutex> lock(body_history_mutex_);
  auto it = body_quality_history_.find(data_point.body_name);

  if (it == body_quality_history_.end() || it->second.size() < 10) {
    // Not enough history for outlier detection
    return false;
  }

  const auto& history = it->second;

  // Collect metric values based on metric_name
  std::vector<double> values;
  size_t window_size = std::min(size_t(50), history.size());

  for (size_t i = history.size() - window_size; i < history.size(); ++i) {
    const auto& quality = history[i];

    if (metric_name == "freshness") {
      values.push_back(quality.data_freshness);
    } else if (metric_name == "accuracy") {
      values.push_back(quality.data_accuracy);
    } else if (metric_name == "completeness") {
      values.push_back(quality.data_completeness);
    } else if (metric_name == "consistency") {
      values.push_back(quality.data_consistency);
    } else {
      values.push_back(quality.overall_score);
    }
  }

  if (values.empty()) {
    return false;
  }

  // Tukey's fences method
  // 1. Sort the values
  std::sort(values.begin(), values.end());

  // 2. Calculate quartiles
  size_t n = values.size();
  double q1, q3;

  size_t q1_idx = n / 4;
  size_t q3_idx = 3 * n / 4;

  q1 = values[q1_idx];
  q3 = values[q3_idx];

  // 3. Calculate IQR
  double iqr = q3 - q1;

  // 4. Calculate fences
  // Standard Tukey's fences use 1.5 * IQR for outliers
  // and 3.0 * IQR for extreme outliers
  const double OUTLIER_MULTIPLIER = 1.5;
  const double EXTREME_OUTLIER_MULTIPLIER = 3.0;

  double lower_fence = q1 - OUTLIER_MULTIPLIER * iqr;
  double upper_fence = q3 + OUTLIER_MULTIPLIER * iqr;
  double lower_extreme_fence = q1 - EXTREME_OUTLIER_MULTIPLIER * iqr;
  double upper_extreme_fence = q3 + EXTREME_OUTLIER_MULTIPLIER * iqr;

  // 5. Get current value
  double current_value = 0.0;
  if (metric_name == "freshness") {
    current_value = calculate_data_freshness(data_point);
  } else if (metric_name == "accuracy") {
    current_value = calculate_data_accuracy(data_point);
  } else if (metric_name == "completeness") {
    current_value = calculate_data_completeness(data_point);
  } else if (metric_name == "consistency") {
    current_value = calculate_data_consistency(data_point);
  } else {
    // Calculate overall score
    DataPointQuality quality = assess_data_point_quality(data_point);
    current_value = quality.overall_score;
  }

  // 6. Check if current value is an outlier
  bool is_outlier = (current_value < lower_fence || current_value > upper_fence);
  bool is_extreme_outlier =
      (current_value < lower_extreme_fence || current_value > upper_extreme_fence);

  // Log extreme outliers
  if (is_extreme_outlier) {
    LOG_WARN("QualityMonitor", "Extreme outlier detected for " + data_point.body_name + " (" +
                                   metric_name + "): " + std::to_string(current_value) +
                                   " (Q1=" + std::to_string(q1) + ", Q3=" + std::to_string(q3) +
                                   ", IQR=" + std::to_string(iqr) + ")");
  }

  return is_outlier;
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
    double x = static_cast<double>(
        std::chrono::duration_cast<std::chrono::seconds>(times[i] - start_time).count());
    double y = values[i];

    sum_x += x;
    sum_y += y;
    sum_xy += x * y;
    sum_x2 += x * x;
  }

  double denominator = static_cast<double>(n) * sum_x2 - sum_x * sum_x;
  if (std::abs(denominator) < 1e-10) {
    return 0.0;
  }

  return (static_cast<double>(n) * sum_xy - sum_x * sum_y) / denominator;
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

void QualityMonitor::trigger_quality_alert(const std::string& message,
                                           const SnapshotQuality& quality) {
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
      if (analysis_cv_.wait_for(lock, config_.analysis_interval,
                                [this] { return stop_analysis_.load(); })) {
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

void QualityMonitor::cleanup_old_history() { maintain_history_size(); }

void QualityMonitor::maintain_history_size() {
  // Remove old entries if history is too large
  while (quality_history_.size() > config_.max_history_size) {
    quality_history_.erase(quality_history_.begin());
  }
}

std::vector<SnapshotQuality> QualityMonitor::filter_history_by_time(
    std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end) const {
  std::vector<SnapshotQuality> filtered;

  std::copy_if(quality_history_.begin(), quality_history_.end(), std::back_inserter(filtered),
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
