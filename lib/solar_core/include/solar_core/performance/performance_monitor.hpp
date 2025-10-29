/**
 * @file performance_monitor.hpp
 * @brief Comprehensive performance monitoring system
 */

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SolarSystem::Performance {

/**
 * @brief Performance metric types
 */
enum class MetricType {
  COUNTER,      // Monotonically increasing value
  GAUGE,        // Value that can go up or down
  HISTOGRAM,    // Distribution of values
  TIMER         // Duration measurements
};

/**
 * @brief Performance metric data
 */
struct MetricData {
  std::string name;
  MetricType type;
  double value;
  std::chrono::system_clock::time_point timestamp;
  std::map<std::string, std::string> labels;

  MetricData() : type(MetricType::GAUGE), value(0.0),
                 timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Performance counter - monotonically increasing value
 */
class Counter {
public:
  explicit Counter(std::string name) : name_(std::move(name)), value_(0) {}

  void increment(double amount = 1.0) {
    value_.fetch_add(amount, std::memory_order_relaxed);
  }

  double get() const {
    return value_.load(std::memory_order_relaxed);
  }

  void reset() {
    value_.store(0, std::memory_order_relaxed);
  }

  const std::string& name() const { return name_; }

private:
  std::string name_;
  std::atomic<double> value_;
};

/**
 * @brief Performance gauge - value that can go up or down
 */
class Gauge {
public:
  explicit Gauge(std::string name) : name_(std::move(name)), value_(0) {}

  void set(double value) {
    value_.store(value, std::memory_order_relaxed);
  }

  void increment(double amount = 1.0) {
    double current = value_.load(std::memory_order_relaxed);
    while (!value_.compare_exchange_weak(current, current + amount,
                                         std::memory_order_relaxed)) {}
  }

  void decrement(double amount = 1.0) {
    increment(-amount);
  }

  double get() const {
    return value_.load(std::memory_order_relaxed);
  }

  const std::string& name() const { return name_; }

private:
  std::string name_;
  std::atomic<double> value_;
};

/**
 * @brief Performance histogram - distribution of values
 */
class Histogram {
public:
  explicit Histogram(std::string name, std::vector<double> buckets = {});

  void observe(double value);

  struct Statistics {
    double min;
    double max;
    double mean;
    double median;
    double p95;
    double p99;
    size_t count;
    double sum;
  };

  Statistics get_statistics() const;
  const std::string& name() const { return name_; }

private:
  std::string name_;
  mutable std::mutex mutex_;
  std::vector<double> values_;
  std::vector<double> buckets_;
  std::map<double, size_t> bucket_counts_;
};

/**
 * @brief Performance timer - duration measurements
 */
class Timer {
public:
  explicit Timer(std::string name) : name_(std::move(name)), histogram_(name) {}

  class ScopedTimer {
  public:
    explicit ScopedTimer(Timer& timer)
        : timer_(timer), start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
      auto end = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
          end - start_).count();
      timer_.record(duration / 1000000.0);  // Convert to seconds
    }

  private:
    Timer& timer_;
    std::chrono::steady_clock::time_point start_;
  };

  void record(double seconds) {
    histogram_.observe(seconds);
  }

  ScopedTimer time() {
    return ScopedTimer(*this);
  }

  Histogram::Statistics get_statistics() const {
    return histogram_.get_statistics();
  }

  const std::string& name() const { return name_; }

private:
  std::string name_;
  Histogram histogram_;
};

/**
 * @brief Performance threshold for alerting
 */
struct PerformanceThreshold {
  std::string metric_name;
  double warning_threshold;
  double critical_threshold;
  bool above_threshold;  // true = alert when above, false = alert when below

  PerformanceThreshold()
      : warning_threshold(0), critical_threshold(0), above_threshold(true) {}
};

/**
 * @brief Performance alert
 */
struct PerformanceAlert {
  enum class Severity { INFO, WARNING, CRITICAL };

  std::string metric_name;
  Severity severity;
  double current_value;
  double threshold_value;
  std::chrono::system_clock::time_point timestamp;
  std::string message;

  PerformanceAlert()
      : severity(Severity::INFO), current_value(0), threshold_value(0),
        timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Performance monitoring system
 */
class PerformanceMonitor {
public:
  static PerformanceMonitor& instance();

  // Metric registration
  std::shared_ptr<Counter> register_counter(const std::string& name);
  std::shared_ptr<Gauge> register_gauge(const std::string& name);
  std::shared_ptr<Histogram> register_histogram(const std::string& name,
                                                const std::vector<double>& buckets = {});
  std::shared_ptr<Timer> register_timer(const std::string& name);

  // Metric retrieval
  std::shared_ptr<Counter> get_counter(const std::string& name);
  std::shared_ptr<Gauge> get_gauge(const std::string& name);
  std::shared_ptr<Histogram> get_histogram(const std::string& name);
  std::shared_ptr<Timer> get_timer(const std::string& name);

  // Threshold management
  void set_threshold(const PerformanceThreshold& threshold);
  void remove_threshold(const std::string& metric_name);
  std::vector<PerformanceThreshold> get_thresholds() const;

  // Alert management
  std::vector<PerformanceAlert> get_alerts() const;
  void clear_alerts();

  // Reporting
  std::string generate_report() const;
  std::vector<MetricData> get_all_metrics() const;

  // Control
  void reset_all();
  void enable_monitoring(bool enabled);
  bool is_monitoring_enabled() const;

private:
  PerformanceMonitor() = default;

  void check_thresholds();
  void add_alert(const PerformanceAlert& alert);

  mutable std::mutex mutex_;
  std::map<std::string, std::shared_ptr<Counter>> counters_;
  std::map<std::string, std::shared_ptr<Gauge>> gauges_;
  std::map<std::string, std::shared_ptr<Histogram>> histograms_;
  std::map<std::string, std::shared_ptr<Timer>> timers_;
  std::map<std::string, PerformanceThreshold> thresholds_;
  std::vector<PerformanceAlert> alerts_;
  std::atomic<bool> monitoring_enabled_{true};
  static constexpr size_t MAX_ALERTS = 1000;
};

/**
 * @brief RAII helper for timing operations
 */
class ScopedPerformanceTimer {
public:
  explicit ScopedPerformanceTimer(const std::string& timer_name)
      : timer_(PerformanceMonitor::instance().get_timer(timer_name)) {
    if (!timer_) {
      timer_ = PerformanceMonitor::instance().register_timer(timer_name);
    }
    start_ = std::chrono::steady_clock::now();
  }

  ~ScopedPerformanceTimer() {
    if (timer_) {
      auto end = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
          end - start_).count();
      timer_->record(duration / 1000000.0);
    }
  }

private:
  std::shared_ptr<Timer> timer_;
  std::chrono::steady_clock::time_point start_;
};

/**
 * @brief Convenience macros for performance monitoring
 */
#define PERF_COUNTER_INC(name) \
  do { \
    auto counter = SolarSystem::Performance::PerformanceMonitor::instance().get_counter(name); \
    if (!counter) counter = SolarSystem::Performance::PerformanceMonitor::instance().register_counter(name); \
    counter->increment(); \
  } while(0)

#define PERF_GAUGE_SET(name, value) \
  do { \
    auto gauge = SolarSystem::Performance::PerformanceMonitor::instance().get_gauge(name); \
    if (!gauge) gauge = SolarSystem::Performance::PerformanceMonitor::instance().register_gauge(name); \
    gauge->set(value); \
  } while(0)

#define PERF_TIMER_SCOPE(name) \
  SolarSystem::Performance::ScopedPerformanceTimer __perf_timer_##__LINE__(name)

}  // namespace SolarSystem::Performance
