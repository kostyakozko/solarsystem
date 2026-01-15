/**
 * @file performance_monitor.cpp
 * @brief Implementation of performance monitoring system
 */

#include "solar_core/performance/performance_monitor.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>

namespace SolarSystem::Performance {

// Histogram implementation
Histogram::Histogram(std::string name, std::vector<double> buckets)
    : name_(std::move(name)), buckets_(std::move(buckets)) {
  if (buckets_.empty()) {
    // Default buckets: 1ms, 10ms, 100ms, 1s, 10s
    buckets_ = {0.001, 0.01, 0.1, 1.0, 10.0};
  }
  std::sort(buckets_.begin(), buckets_.end());
  for (double bucket : buckets_) {
    bucket_counts_[bucket] = 0;
  }
}

void Histogram::observe(double value) {
  std::lock_guard<std::mutex> lock(mutex_);
  values_.push_back(value);

  // Update bucket counts
  for (double bucket : buckets_) {
    if (value <= bucket) {
      bucket_counts_[bucket]++;
      break;
    }
  }
}

Histogram::Statistics Histogram::get_statistics() const {
  std::lock_guard<std::mutex> lock(mutex_);

  Statistics stats;
  stats.count = values_.size();

  if (values_.empty()) {
    stats.min = 0;
    stats.max = 0;
    stats.mean = 0;
    stats.median = 0;
    stats.p95 = 0;
    stats.p99 = 0;
    stats.sum = 0;
    return stats;
  }

  // Calculate basic statistics
  stats.min = *std::min_element(values_.begin(), values_.end());
  stats.max = *std::max_element(values_.begin(), values_.end());
  stats.sum = std::accumulate(values_.begin(), values_.end(), 0.0);
  stats.mean = stats.sum / static_cast<double>(values_.size());

  // Calculate percentiles
  std::vector<double> sorted_values = values_;
  std::sort(sorted_values.begin(), sorted_values.end());

  size_t median_idx = sorted_values.size() / 2;
  stats.median = sorted_values[median_idx];

  size_t p95_idx = static_cast<size_t>(static_cast<double>(sorted_values.size()) * 0.95);
  stats.p95 = sorted_values[std::min(p95_idx, sorted_values.size() - 1)];

  size_t p99_idx = static_cast<size_t>(static_cast<double>(sorted_values.size()) * 0.99);
  stats.p99 = sorted_values[std::min(p99_idx, sorted_values.size() - 1)];

  return stats;
}

// PerformanceMonitor implementation
PerformanceMonitor& PerformanceMonitor::instance() {
  static PerformanceMonitor instance;
  return instance;
}

std::shared_ptr<Counter> PerformanceMonitor::register_counter(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = counters_.find(name);
  if (it != counters_.end()) {
    return it->second;
  }

  auto counter = std::make_shared<Counter>(name);
  counters_[name] = counter;
  return counter;
}

std::shared_ptr<Gauge> PerformanceMonitor::register_gauge(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = gauges_.find(name);
  if (it != gauges_.end()) {
    return it->second;
  }

  auto gauge = std::make_shared<Gauge>(name);
  gauges_[name] = gauge;
  return gauge;
}

std::shared_ptr<Histogram> PerformanceMonitor::register_histogram(
    const std::string& name, const std::vector<double>& buckets) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = histograms_.find(name);
  if (it != histograms_.end()) {
    return it->second;
  }

  auto histogram = std::make_shared<Histogram>(name, buckets);
  histograms_[name] = histogram;
  return histogram;
}

std::shared_ptr<Timer> PerformanceMonitor::register_timer(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = timers_.find(name);
  if (it != timers_.end()) {
    return it->second;
  }

  auto timer = std::make_shared<Timer>(name);
  timers_[name] = timer;
  return timer;
}

std::shared_ptr<Counter> PerformanceMonitor::get_counter(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = counters_.find(name);
  return (it != counters_.end()) ? it->second : nullptr;
}

std::shared_ptr<Gauge> PerformanceMonitor::get_gauge(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = gauges_.find(name);
  return (it != gauges_.end()) ? it->second : nullptr;
}

std::shared_ptr<Histogram> PerformanceMonitor::get_histogram(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = histograms_.find(name);
  return (it != histograms_.end()) ? it->second : nullptr;
}

std::shared_ptr<Timer> PerformanceMonitor::get_timer(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = timers_.find(name);
  return (it != timers_.end()) ? it->second : nullptr;
}

void PerformanceMonitor::set_threshold(const PerformanceThreshold& threshold) {
  std::lock_guard<std::mutex> lock(mutex_);
  thresholds_[threshold.metric_name] = threshold;
}

void PerformanceMonitor::remove_threshold(const std::string& metric_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  thresholds_.erase(metric_name);
}

std::vector<PerformanceThreshold> PerformanceMonitor::get_thresholds() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<PerformanceThreshold> result;
  result.reserve(thresholds_.size());
  for (const auto& [name, threshold] : thresholds_) {
    result.push_back(threshold);
  }
  return result;
}

std::vector<PerformanceAlert> PerformanceMonitor::get_alerts() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return alerts_;
}

void PerformanceMonitor::clear_alerts() {
  std::lock_guard<std::mutex> lock(mutex_);
  alerts_.clear();
}

void PerformanceMonitor::check_thresholds() {
  if (!monitoring_enabled_) return;

  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto& [name, threshold] : thresholds_) {
    double current_value = 0;
    bool found = false;

    // Check counters
    auto counter_it = counters_.find(name);
    if (counter_it != counters_.end()) {
      current_value = counter_it->second->get();
      found = true;
    }

    // Check gauges
    auto gauge_it = gauges_.find(name);
    if (gauge_it != gauges_.end()) {
      current_value = gauge_it->second->get();
      found = true;
    }

    if (!found) continue;

    // Check thresholds
    bool exceeds_critical = threshold.above_threshold
        ? (current_value > threshold.critical_threshold)
        : (current_value < threshold.critical_threshold);

    bool exceeds_warning = threshold.above_threshold
        ? (current_value > threshold.warning_threshold)
        : (current_value < threshold.warning_threshold);

    if (exceeds_critical) {
      PerformanceAlert alert;
      alert.metric_name = name;
      alert.severity = PerformanceAlert::Severity::CRITICAL;
      alert.current_value = current_value;
      alert.threshold_value = threshold.critical_threshold;
      alert.timestamp = std::chrono::system_clock::now();
      alert.message = "Critical threshold exceeded for " + name;
      add_alert(alert);
    } else if (exceeds_warning) {
      PerformanceAlert alert;
      alert.metric_name = name;
      alert.severity = PerformanceAlert::Severity::WARNING;
      alert.current_value = current_value;
      alert.threshold_value = threshold.warning_threshold;
      alert.timestamp = std::chrono::system_clock::now();
      alert.message = "Warning threshold exceeded for " + name;
      add_alert(alert);
    }
  }
}

void PerformanceMonitor::add_alert(const PerformanceAlert& alert) {
  // Mutex already held by caller
  if (alerts_.size() >= MAX_ALERTS) {
    alerts_.erase(alerts_.begin());
  }
  alerts_.push_back(alert);
}

std::string PerformanceMonitor::generate_report() const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::ostringstream oss;
  oss << "=== Performance Monitoring Report ===\n\n";

  // Counters
  if (!counters_.empty()) {
    oss << "Counters:\n";
    for (const auto& [name, counter] : counters_) {
      oss << "  " << name << ": " << counter->get() << "\n";
    }
    oss << "\n";
  }

  // Gauges
  if (!gauges_.empty()) {
    oss << "Gauges:\n";
    for (const auto& [name, gauge] : gauges_) {
      oss << "  " << name << ": " << gauge->get() << "\n";
    }
    oss << "\n";
  }

  // Histograms
  if (!histograms_.empty()) {
    oss << "Histograms:\n";
    for (const auto& [name, histogram] : histograms_) {
      auto stats = histogram->get_statistics();
      oss << "  " << name << ":\n"
          << "    Count: " << stats.count << "\n"
          << "    Min: " << stats.min << "\n"
          << "    Max: " << stats.max << "\n"
          << "    Mean: " << stats.mean << "\n"
          << "    Median: " << stats.median << "\n"
          << "    P95: " << stats.p95 << "\n"
          << "    P99: " << stats.p99 << "\n";
    }
    oss << "\n";
  }

  // Timers
  if (!timers_.empty()) {
    oss << "Timers:\n";
    for (const auto& [name, timer] : timers_) {
      auto stats = timer->get_statistics();
      oss << "  " << name << ":\n"
          << "    Count: " << stats.count << "\n"
          << "    Min: " << stats.min << "s\n"
          << "    Max: " << stats.max << "s\n"
          << "    Mean: " << stats.mean << "s\n"
          << "    Median: " << stats.median << "s\n"
          << "    P95: " << stats.p95 << "s\n"
          << "    P99: " << stats.p99 << "s\n";
    }
    oss << "\n";
  }

  // Alerts
  if (!alerts_.empty()) {
    oss << "Recent Alerts (" << alerts_.size() << "):\n";
    size_t count = 0;
    for (auto it = alerts_.rbegin(); it != alerts_.rend() && count < 10; ++it, ++count) {
      const auto& alert = *it;
      oss << "  [";
      switch (alert.severity) {
        case PerformanceAlert::Severity::INFO: oss << "INFO"; break;
        case PerformanceAlert::Severity::WARNING: oss << "WARN"; break;
        case PerformanceAlert::Severity::CRITICAL: oss << "CRIT"; break;
      }
      oss << "] " << alert.message << " (value: " << alert.current_value << ")\n";
    }
  }

  return oss.str();
}

std::vector<MetricData> PerformanceMonitor::get_all_metrics() const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<MetricData> metrics;

  // Add counters
  for (const auto& [name, counter] : counters_) {
    MetricData data;
    data.name = name;
    data.type = MetricType::COUNTER;
    data.value = counter->get();
    data.timestamp = std::chrono::system_clock::now();
    metrics.push_back(data);
  }

  // Add gauges
  for (const auto& [name, gauge] : gauges_) {
    MetricData data;
    data.name = name;
    data.type = MetricType::GAUGE;
    data.value = gauge->get();
    data.timestamp = std::chrono::system_clock::now();
    metrics.push_back(data);
  }

  return metrics;
}

void PerformanceMonitor::reset_all() {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto& [name, counter] : counters_) {
    counter->reset();
  }

  alerts_.clear();
}

void PerformanceMonitor::enable_monitoring(bool enabled) {
  monitoring_enabled_ = enabled;
}

bool PerformanceMonitor::is_monitoring_enabled() const {
  return monitoring_enabled_;
}

}  // namespace SolarSystem::Performance
