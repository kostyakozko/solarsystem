#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#include sctl.h>
#elif __linux__
#include <sys/resource.h>
#include <unistd.h>

#include <fstream>
#include <sstream>
#endif

namespace SolarSystem::Testing {

/**
 * @brief CPU usage statistics
 */
struct CPUStats {
  double user_percent = 0.0;
  double system_percent = 0.0;
  double total_percent = 0.0;
  uint64_t context_switches = 0;
  uint64_t voluntary_switches = 0;
  uint64_t involuntary_switches = 0;
};

/**
 * @brief Cache performance statistics
 */
struct CacheStats {
  uint64_t cache_references = 0;
  uint64_t cache_misses = 0;
  double cache_miss_rate = 0.0;
  uint64_t l1_misses = 0;
  uint64_t l2_misses = 0;
  uint64_t l3_misses = 0;
};

/**
 * @brief I/O performance statistics
 */
struct IOStats {
  uint64_t bytes_read = 0;
  uint64_t bytes_written = 0;
  uint64_t read_operations = 0;
  uint64_t write_operations = 0;
  double read_bandwidth_mbps = 0.0;
  double write_bandwidth_mbps = 0.0;
};

/**
 * @brief Comprehensive performance metrics
 */
struct PerformanceMetrics {
  std::string test_name;
  std::chrono::nanoseconds execution_time{0};
  std::chrono::nanoseconds cpu_time{0};
  CPUStats cpu_stats;
  CacheStats cache_stats;
  IOStats io_stats;
  size_t peak_memory_kb = 0;
  size_t memory_allocated_kb = 0;
  double cpu_efficiency = 0.0;  // cpu_time / execution_time
  std::chrono::system_clock::time_point timestamp;

  [[nodiscard]] double execution_time_ms() const {
    return std::chrono::duration<double, std::milli>(execution_time).count();
  }

  [[nodiscard]] double cpu_time_ms() const {
    return std::chrono::duration<double, std::milli>(cpu_time).count();
  }
};

/**
 * @brief Performance baseline for regression detection
 */
struct PerformanceBaseline {
  std::string test_name;
  std::chrono::nanoseconds baseline_time{0};
  double baseline_cpu_percent = 0.0;
  size_t baseline_memory_kb = 0;
  double baseline_cache_miss_rate = 0.0;
  std::chrono::system_clock::time_point created_at;
  size_t sample_count = 0;

  [[nodiscard]] bool is_regression(const PerformanceMetrics& metrics,
                                   double threshold = 1.5) const {
    if (baseline_time.count() == 0) return false;

    double time_ratio = static_cast<double>(metrics.execution_time.count()) / baseline_time.count();
    double memory_ratio = baseline_memory_kb > 0
                              ? static_cast<double>(metrics.peak_memory_kb) / baseline_memory_kb
                              : 1.0;

    return time_ratio > threshold || memory_ratio > threshold;
  }

  [[nodiscard]] double get_regression_factor(const PerformanceMetrics& metrics) const {
    if (baseline_time.count() == 0) return 1.0;
    return static_cast<double>(metrics.execution_time.count()) / baseline_time.count();
  }
};

/**
 * @brief Performance regression alert
 */
struct RegressionAlert {
  std::string test_name;
  double regression_factor;
  std::string metric_type;  // "time", "memory", "cpu", "cache"
  std::string description;
  PerformanceMetrics current_metrics;
  PerformanceBaseline baseline;
  std::chrono::system_clock::time_point detected_at;
};

/**
 * @brief System resource monitor
 */
class SystemResourceMonitor {
 public:
  SystemResourceMonitor() = default;
  ~SystemResourceMonitor() { stop_monitoring(); }

  void start_monitoring() {
    if (monitoring_active_.load()) return;

    monitoring_active_ = true;
    monitor_thread_ = std::thread(&SystemResourceMonitor::monitor_loop, this);
  }

  void stop_monitoring() {
    if (!monitoring_active_.load()) return;

    monitoring_active_ = false;
    monitor_cv_.notify_all();

    if (monitor_thread_.joinable()) {
      monitor_thread_.join();
    }
  }

  [[nodiscard]] CPUStats get_cpu_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return current_cpu_stats_;
  }

  [[nodiscard]] IOStats get_io_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return current_io_stats_;
  }

 private:
  std::atomic<bool> monitoring_active_{false};
  std::thread monitor_thread_;
  std::condition_variable monitor_cv_;
  mutable std::mutex stats_mutex_;

  CPUStats current_cpu_stats_;
  IOStats current_io_stats_;

  void monitor_loop() {
    auto last_update = std::chrono::steady_clock::now();

    while (monitoring_active_.load()) {
      update_cpu_stats();
      update_io_stats();

      std::unique_lock<std::mutex> lock(stats_mutex_);
      monitor_cv_.wait_for(lock, std::chrono::milliseconds(100));
    }
  }

  void update_cpu_stats() {
#ifdef __APPLE__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
      std::lock_guard<std::mutex> lock(stats_mutex_);

      auto total_time = usage.ru_utime.tv_sec + usage.ru_stime.tv_sec +
                        (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000000.0;

      current_cpu_stats_.user_percent =
          (usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0) / total_time * 100.0;
      current_cpu_stats_.system_percent =
          (usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0) / total_time * 100.0;
      current_cpu_stats_.total_percent =
          current_cpu_stats_.user_percent + current_cpu_stats_.system_percent;
      current_cpu_stats_.context_switches = usage.ru_nvcsw + usage.ru_nivcsw;
      current_cpu_stats_.voluntary_switches = usage.ru_nvcsw;
      current_cpu_stats_.involuntary_switches = usage.ru_nivcsw;
    }
#elif __linux__
    std::ifstream stat_file("/proc/self/stat");
    if (stat_file.is_open()) {
      std::string line;
      std::getline(stat_file, line);
      std::istringstream iss(line);

      // Skip to the relevant fields (utime, stime, etc.)
      std::string token;
      for (int i = 0; i < 13; ++i) iss >> token;

      long utime, stime;
      iss >> utime >> stime;

      std::lock_guard<std::mutex> lock(stats_mutex_);
      long total_time = utime + stime;
      if (total_time > 0) {
        current_cpu_stats_.user_percent = (double)utime / total_time * 100.0;
        current_cpu_stats_.system_percent = (double)stime / total_time * 100.0;
        current_cpu_stats_.total_percent = 100.0;
      }
    }
#endif
  }

  void update_io_stats() {
#ifdef __linux__
    std::ifstream io_file("/proc/self/io");
    if (io_file.is_open()) {
      std::string line;
      std::lock_guard<std::mutex> lock(stats_mutex_);

      while (std::getline(io_file, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;

        if (iss >> key >> value) {
          if (key == "read_bytes:") {
            current_io_stats_.bytes_read = value;
          } else if (key == "write_bytes:") {
            current_io_stats_.bytes_written = value;
          } else if (key == "syscr:") {
            current_io_stats_.read_operations = value;
          } else if (key == "syscw:") {
            current_io_stats_.write_operations = value;
          }
        }
      }
    }
#endif
  }
};

/**
 * @brief Comprehensive performance monitor
 */
class ComprehensivePerformanceMonitor {
 public:
  static ComprehensivePerformanceMonitor& instance() {
    static ComprehensivePerformanceMonitor monitor;
    return monitor;
  }

  void start_test_monitoring(const std::string& test_name) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    if (active_tests_.find(test_name) != active_tests_.end()) {
      throw std::runtime_error("Test " + test_name + " is already being monitored");
    }

    TestMonitoringState state;
    state.start_time = std::chrono::high_resolution_clock::now();
    state.start_cpu_time = get_cpu_time();
    state.start_memory = get_memory_usage();

    // Start system monitoring if not already active
    if (active_tests_.empty()) {
      system_monitor_.start_monitoring();
    }

    active_tests_[test_name] = state;
  }

  [[nodiscard]] PerformanceMetrics stop_test_monitoring(const std::string& test_name) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    auto it = active_tests_.find(test_name);
    if (it == active_tests_.end()) {
      throw std::runtime_error("Test " + test_name + " is not being monitored");
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto end_cpu_time = get_cpu_time();
    auto end_memory = get_memory_usage();

    const auto& state = it->second;

    PerformanceMetrics metrics;
    metrics.test_name = test_name;
    metrics.execution_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - state.start_time);
    metrics.cpu_time = std::chrono::nanoseconds(end_cpu_time - state.start_cpu_time);
    metrics.peak_memory_kb = std::max(state.peak_memory, end_memory);
    metrics.memory_allocated_kb =
        end_memory > state.start_memory ? end_memory - state.start_memory : 0;
    metrics.cpu_stats = system_monitor_.get_cpu_stats();
    metrics.io_stats = system_monitor_.get_io_stats();
    metrics.timestamp = std::chrono::system_clock::now();

    // Calculate CPU efficiency
    if (metrics.execution_time.count() > 0) {
      metrics.cpu_efficiency =
          static_cast<double>(metrics.cpu_time.count()) / metrics.execution_time.count();
    }

    // Estimate cache stats (platform-specific implementation would be more accurate)
    estimate_cache_stats(metrics);

    active_tests_.erase(it);

    // Stop system monitoring if no active tests
    if (active_tests_.empty()) {
      system_monitor_.stop_monitoring();
    }

    // Store metrics for baseline comparison
    test_metrics_[test_name].push_back(metrics);

    // Check for regressions
    check_for_regressions(metrics);

    return metrics;
  }

  void set_baseline(const std::string& test_name, const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(baseline_mutex_);

    PerformanceBaseline baseline;
    baseline.test_name = test_name;
    baseline.baseline_time = metrics.execution_time;
    baseline.baseline_cpu_percent = metrics.cpu_stats.total_percent;
    baseline.baseline_memory_kb = metrics.peak_memory_kb;
    baseline.baseline_cache_miss_rate = metrics.cache_stats.cache_miss_rate;
    baseline.created_at = std::chrono::system_clock::now();
    baseline.sample_count = 1;

    baselines_[test_name] = baseline;
  }

  void update_baseline(const std::string& test_name, const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(baseline_mutex_);

    auto it = baselines_.find(test_name);
    if (it == baselines_.end()) {
      set_baseline(test_name, metrics);
      return;
    }

    auto& baseline = it->second;

    // Use exponential moving average to update baseline
    double alpha = 0.1;  // Weight for new sample

    auto new_time_ns = static_cast<double>(metrics.execution_time.count());
    auto old_time_ns = static_cast<double>(baseline.baseline_time.count());
    auto updated_time_ns = old_time_ns * (1.0 - alpha) + new_time_ns * alpha;

    baseline.baseline_time = std::chrono::nanoseconds(static_cast<long long>(updated_time_ns));
    baseline.baseline_cpu_percent =
        baseline.baseline_cpu_percent * (1.0 - alpha) + metrics.cpu_stats.total_percent * alpha;
    baseline.baseline_memory_kb = static_cast<size_t>(baseline.baseline_memory_kb * (1.0 - alpha) +
                                                      metrics.peak_memory_kb * alpha);
    baseline.baseline_cache_miss_rate = baseline.baseline_cache_miss_rate * (1.0 - alpha) +
                                        metrics.cache_stats.cache_miss_rate * alpha;
    baseline.sample_count++;
  }

  [[nodiscard]] std::vector<RegressionAlert> get_regression_alerts() const {
    std::lock_guard<std::mutex> lock(alert_mutex_);
    return regression_alerts_;
  }

  void clear_regression_alerts() {
    std::lock_guard<std::mutex> lock(alert_mutex_);
    regression_alerts_.clear();
  }

  void save_baselines(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(baseline_mutex_);

    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "# Performance Baselines\\n";
    file << "# Format: test_name time_ns cpu_percent memory_kb cache_miss_rate sample_count\\n";

    for (const auto& [test_name, baseline] : baselines_) {
      file << test_name << " " << baseline.baseline_time.count() << " "
           << baseline.baseline_cpu_percent << " " << baseline.baseline_memory_kb << " "
           << baseline.baseline_cache_miss_rate << " " << baseline.sample_count << "\\n";
    }
  }

  void load_baselines(const std::string& filename) {
    std::lock_guard<std::mutex> lock(baseline_mutex_);

    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#') continue;

      std::istringstream iss(line);
      std::string test_name;
      long long time_ns;
      double cpu_percent, cache_miss_rate;
      size_t memory_kb, sample_count;

      if (iss >> test_name >> time_ns >> cpu_percent >> memory_kb >> cache_miss_rate >>
          sample_count) {
        PerformanceBaseline baseline;
        baseline.test_name = test_name;
        baseline.baseline_time = std::chrono::nanoseconds(time_ns);
        baseline.baseline_cpu_percent = cpu_percent;
        baseline.baseline_memory_kb = memory_kb;
        baseline.baseline_cache_miss_rate = cache_miss_rate;
        baseline.sample_count = sample_count;
        baseline.created_at = std::chrono::system_clock::now();

        baselines_[test_name] = baseline;
      }
    }
  }

  [[nodiscard]] std::vector<PerformanceMetrics> get_test_history(
      const std::string& test_name) const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    auto it = test_metrics_.find(test_name);
    if (it != test_metrics_.end()) {
      return it->second;
    }
    return {};
  }

 private:
  struct TestMonitoringState {
    std::chrono::high_resolution_clock::time_point start_time;
    long long start_cpu_time = 0;
    size_t start_memory = 0;
    size_t peak_memory = 0;
  };

  mutable std::mutex monitor_mutex_;
  mutable std::mutex baseline_mutex_;
  mutable std::mutex alert_mutex_;

  std::unordered_map<std::string, TestMonitoringState> active_tests_;
  std::unordered_map<std::string, PerformanceBaseline> baselines_;
  std::unordered_map<std::string, std::vector<PerformanceMetrics>> test_metrics_;
  mutable std::vector<RegressionAlert> regression_alerts_;

  SystemResourceMonitor system_monitor_;

  [[nodiscard]] long long get_cpu_time() const {
#ifdef __APPLE__ || __linux__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
      return (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000000LL +
             (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) * 1000LL;
    }
#endif
    return 0;
  }

  [[nodiscard]] size_t get_memory_usage() const {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &info_count) ==
        KERN_SUCCESS) {
      return info.resident_size / 1024;
    }
#elif __linux__
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
      if (line.find("VmRSS:") == 0) {
        std::istringstream iss(line);
        std::string key, value, unit;
        iss >> key >> value >> unit;
        return std::stoul(value);
      }
    }
#endif
    return 0;
  }

  void estimate_cache_stats(PerformanceMetrics& metrics) const {
    // This is a simplified estimation - real implementation would use
    // performance counters or profiling tools

    // Estimate based on memory access patterns and execution time
    double memory_intensity = static_cast<double>(metrics.memory_allocated_kb) /
                              std::max(1.0, metrics.execution_time_ms());

    // Higher memory intensity typically correlates with more cache misses
    metrics.cache_stats.cache_miss_rate = std::min(0.5, memory_intensity * 0.001);
    metrics.cache_stats.cache_references =
        static_cast<uint64_t>(metrics.execution_time_ms() * 1000);  // Rough estimate
    metrics.cache_stats.cache_misses = static_cast<uint64_t>(metrics.cache_stats.cache_references *
                                                             metrics.cache_stats.cache_miss_rate);
  }

  void check_for_regressions(const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> baseline_lock(baseline_mutex_);
    std::lock_guard<std::mutex> alert_lock(alert_mutex_);

    auto it = baselines_.find(metrics.test_name);
    if (it == baselines_.end()) return;

    const auto& baseline = it->second;

    if (baseline.is_regression(metrics)) {
      RegressionAlert alert;
      alert.test_name = metrics.test_name;
      alert.regression_factor = baseline.get_regression_factor(metrics);
      alert.current_metrics = metrics;
      alert.baseline = baseline;
      alert.detected_at = std::chrono::system_clock::now();

      // Determine primary regression type
      double time_ratio = baseline.get_regression_factor(metrics);
      double memory_ratio =
          baseline.baseline_memory_kb > 0
              ? static_cast<double>(metrics.peak_memory_kb) / baseline.baseline_memory_kb
              : 1.0;

      if (time_ratio > memory_ratio) {
        alert.metric_type = "time";
        alert.description =
            "Execution time regression: " + std::to_string(time_ratio) + "x slower than baseline";
      } else {
        alert.metric_type = "memory";
        alert.description = "Memory usage regression: " + std::to_string(memory_ratio) +
                            "x more memory than baseline";
      }

      regression_alerts_.push_back(alert);
    }
  }
};

// Convenience macros for performance monitoring
#define START_PERFORMANCE_MONITORING(test_name) \
  ComprehensivePerformanceMonitor::instance().start_test_monitoring(test_name)

#define STOP_PERFORMANCE_MONITORING(test_name) \
  ComprehensivePerformanceMonitor::instance().stop_test_monitoring(test_name)

#define PERFORMANCE_TEST(test_name, code)                                             \
  do {                                                                                \
    START_PERFORMANCE_MONITORING(test_name);                                          \
    try {                                                                             \
      code;                                                                           \
    } catch (...) {                                                                   \
      STOP_PERFORMANCE_MONITORING(test_name);                                         \
      throw;                                                                          \
    }                                                                                 \
    auto metrics = STOP_PERFORMANCE_MONITORING(test_name);                            \
    std::cout << "⚡ " << test_name << " - " << metrics.execution_time_ms() << "ms, " \
              << metrics.peak_memory_kb << "KB\\n";                                   \
  } while (0)

}  // namespace SolarSystem::Testing
