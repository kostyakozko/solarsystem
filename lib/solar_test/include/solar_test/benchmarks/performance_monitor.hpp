#pragma once

/**
 * @file performance_monitor.hpp
 * @brief Performance monitoring utilities for benchmarking
 *
 * This header provides utilities for monitoring system performance during benchmarks:
 * - CPU usage monitoring
 * - Memory allocation tracking
 * - System resource monitoring
 * - Performance profiling helpers
 */

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace SolarSystem::Testing {

/**
 * @brief System performance metrics
 */
struct PerformanceMetrics {
  double cpu_usage_percentage = 0.0;      ///< CPU usage during measurement
  size_t memory_allocated_bytes = 0;      ///< Memory allocated during measurement
  size_t memory_deallocated_bytes = 0;    ///< Memory deallocated during measurement
  size_t peak_memory_usage_bytes = 0;     ///< Peak memory usage
  std::chrono::nanoseconds wall_time{0};  ///< Wall clock time
  std::chrono::nanoseconds cpu_time{0};   ///< CPU time
  size_t context_switches = 0;            ///< Number of context switches
  size_t page_faults = 0;                 ///< Number of page faults
};

/**
 * @brief Memory allocation tracker
 */
class MemoryTracker {
 public:
  /**
   * @brief Start tracking memory allocations
   */
  void start_tracking();

  /**
   * @brief Stop tracking and return metrics
   */
  PerformanceMetrics stop_tracking();

  /**
   * @brief Get current memory usage
   */
  [[nodiscard]] size_t get_current_usage() const;

  /**
   * @brief Reset tracking counters
   */
  void reset();

 private:
  bool tracking_ = false;
  size_t start_memory_ = 0;
  size_t peak_memory_ = 0;
  size_t allocated_bytes_ = 0;
  size_t deallocated_bytes_ = 0;
  std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief CPU usage monitor
 */
class CpuMonitor {
 public:
  /**
   * @brief Start monitoring CPU usage
   */
  void start_monitoring();

  /**
   * @brief Stop monitoring and return average CPU usage
   */
  double stop_monitoring();

  /**
   * @brief Get current CPU usage percentage
   */
  [[nodiscard]] double get_current_usage() const;

 private:
  bool monitoring_ = false;
  std::chrono::high_resolution_clock::time_point start_time_;
  std::chrono::nanoseconds start_cpu_time_{0};
};

/**
 * @brief Comprehensive performance monitor
 */
class PerformanceMonitor {
 public:
  /**
   * @brief Monitor configuration
   */
  struct Configuration {
    bool track_memory = true;         ///< Enable memory tracking
    bool track_cpu = true;            ///< Enable CPU monitoring
    bool track_system_calls = false;  ///< Enable system call tracking
    std::chrono::milliseconds sample_interval = std::chrono::milliseconds(10);
  };

  /**
   * @brief Create performance monitor with default configuration
   */
  PerformanceMonitor();

  /**
   * @brief Create performance monitor with custom configuration
   */
  explicit PerformanceMonitor(Configuration config);

  /**
   * @brief Start monitoring performance
   */
  void start_monitoring();

  /**
   * @brief Stop monitoring and return metrics
   */
  PerformanceMetrics stop_monitoring();

  /**
   * @brief Monitor function execution and return metrics
   */
  template <typename Func>
  PerformanceMetrics monitor_execution(Func&& func);

  /**
   * @brief Get current performance snapshot
   */
  [[nodiscard]] PerformanceMetrics get_current_metrics() const;

 private:
  Configuration config_;
  bool monitoring_ = false;
  std::unique_ptr<MemoryTracker> memory_tracker_;
  std::unique_ptr<CpuMonitor> cpu_monitor_;
  std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief RAII performance measurement helper
 */
class ScopedPerformanceMonitor {
 public:
  /**
   * @brief Start monitoring on construction
   */
  explicit ScopedPerformanceMonitor(PerformanceMonitor& monitor);

  /**
   * @brief Stop monitoring on destruction
   */
  ~ScopedPerformanceMonitor();

  /**
   * @brief Get current metrics (monitoring must be stopped first)
   */
  [[nodiscard]] const PerformanceMetrics& metrics() const;

 private:
  PerformanceMonitor& monitor_;
  PerformanceMetrics metrics_;
  bool stopped_ = false;
};

/**
 * @brief Performance profiler for detailed analysis
 */
class PerformanceProfiler {
 public:
  /**
   * @brief Profile point data
   */
  struct ProfilePoint {
    std::string name;
    std::chrono::high_resolution_clock::time_point timestamp;
    PerformanceMetrics metrics;
  };

  /**
   * @brief Add profile point
   */
  void add_profile_point(const std::string& name);

  /**
   * @brief Add profile point with custom metrics
   */
  void add_profile_point(const std::string& name, const PerformanceMetrics& metrics);

  /**
   * @brief Get all profile points
   */
  [[nodiscard]] const std::vector<ProfilePoint>& get_profile_points() const;

  /**
   * @brief Generate performance report
   */
  [[nodiscard]] std::string generate_report() const;

  /**
   * @brief Clear all profile points
   */
  void clear();

 private:
  std::vector<ProfilePoint> profile_points_;
  PerformanceMonitor monitor_;
};

// Template implementations
template <typename Func>
PerformanceMetrics PerformanceMonitor::monitor_execution(Func&& func) {
  start_monitoring();
  func();
  return stop_monitoring();
}

// Convenience macros for performance monitoring
#define SOLAR_PROFILE_SCOPE(profiler, name) profiler.add_profile_point(name)

#define SOLAR_MONITOR_SCOPE(monitor) \
  SolarSystem::Testing::ScopedPerformanceMonitor _monitor_scope(monitor)

}  // namespace SolarSystem::Testing
