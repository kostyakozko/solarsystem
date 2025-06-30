/**
 * @file benchmark_utils.h
 * @brief Performance benchmarking utilities for Solar System Suite
 *
 * Provides comprehensive performance measurement and analysis tools
 * for evaluating simulation performance, memory usage, and scalability.
 */

#ifndef BENCHMARK_UTILS_H
#define BENCHMARK_UTILS_H

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Benchmark {

/**
 * @brief Performance measurement result
 */
struct PerformanceResult {
  std::string name;
  double duration_ms;
  double min_duration_ms;
  double max_duration_ms;
  double avg_duration_ms;
  double std_deviation_ms;
  size_t iterations;
  size_t memory_usage_bytes;
  double operations_per_second;
  std::map<std::string, double> custom_metrics;
};

/**
 * @brief Memory usage statistics
 */
struct MemoryStats {
  size_t peak_memory_bytes;
  size_t current_memory_bytes;
  size_t allocations_count;
  size_t deallocations_count;
  double fragmentation_ratio;
};

/**
 * @brief High-precision timer for performance measurement
 */
class Timer {
 private:
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
  bool is_running;

 public:
  Timer();

  void start();
  void stop();
  void reset();

  double elapsed_ms() const;
  double elapsed_us() const;
  double elapsed_ns() const;

  bool running() const { return is_running; }
};

/**
 * @brief Performance profiler for detailed analysis
 */
class Profiler {
 private:
  std::map<std::string, std::vector<double>> measurements;
  std::map<std::string, Timer> active_timers;

 public:
  void start_measurement(const std::string& name);
  void end_measurement(const std::string& name);

  void add_measurement(const std::string& name, double value_ms);

  PerformanceResult get_result(const std::string& name) const;
  std::vector<PerformanceResult> get_all_results() const;

  void clear();
  void print_summary() const;

  // Scoped measurement helper
  class ScopedMeasurement {
   private:
    Profiler& profiler;
    std::string name;

   public:
    ScopedMeasurement(Profiler& p, const std::string& n);
    ~ScopedMeasurement();
  };

  ScopedMeasurement measure(const std::string& name);
};

/**
 * @brief Memory usage monitor
 */
class MemoryMonitor {
 private:
  size_t baseline_memory;
  size_t peak_memory;
  size_t allocation_count;

 public:
  MemoryMonitor();

  void start_monitoring();
  void stop_monitoring();

  MemoryStats get_stats() const;
  size_t get_current_usage() const;
  size_t get_peak_usage() const;

  void reset();
};

/**
 * @brief Benchmark suite for comprehensive performance testing
 */
class BenchmarkSuite {
 private:
  std::string suite_name;
  std::vector<PerformanceResult> results;
  Profiler profiler;
  MemoryMonitor memory_monitor;

 public:
  BenchmarkSuite(const std::string& name);

  // Run benchmark with specified iterations
  void run_benchmark(const std::string& name, std::function<void()> benchmark_func,
                     size_t iterations = 1000);

  // Run memory benchmark
  void run_memory_benchmark(const std::string& name, std::function<void()> benchmark_func,
                            size_t iterations = 100);

  // Run scalability benchmark (varying input sizes)
  void run_scalability_benchmark(const std::string& name,
                                 std::function<void(size_t)> benchmark_func,
                                 const std::vector<size_t>& input_sizes,
                                 size_t iterations_per_size = 10);

  // Specialized benchmarks for Solar System Suite
  void benchmark_simulation_performance(size_t num_bodies, size_t time_steps,
                                        double time_step_size);

  void benchmark_jpl_data_loading(const std::string& cache_file);

  void benchmark_web_server_response(int num_requests);

  // Results and reporting
  std::vector<PerformanceResult> get_results() const;
  void print_summary() const;
  void export_results(const std::string& filename, const std::string& format = "csv") const;

  // Comparison utilities
  void compare_with_baseline(const std::string& baseline_file);
  void generate_performance_report(const std::string& output_file) const;
};

/**
 * @brief Performance regression detector
 */
class RegressionDetector {
 private:
  std::map<std::string, PerformanceResult> baseline_results;
  double regression_threshold_percent;

 public:
  RegressionDetector(double threshold_percent = 10.0);

  void load_baseline(const std::string& baseline_file);
  void save_baseline(const std::vector<PerformanceResult>& results,
                     const std::string& baseline_file) const;

  struct RegressionReport {
    std::string benchmark_name;
    double baseline_performance;
    double current_performance;
    double change_percent;
    bool is_regression;
    std::string status;
  };

  std::vector<RegressionReport> detect_regressions(
      const std::vector<PerformanceResult>& current_results) const;

  void print_regression_report(const std::vector<RegressionReport>& report) const;
};

/**
 * @brief System resource monitor
 */
class SystemMonitor {
 public:
  struct SystemStats {
    double cpu_usage_percent;
    size_t memory_usage_bytes;
    size_t available_memory_bytes;
    double disk_io_rate_mbps;
    double network_io_rate_mbps;
    int active_threads;
  };

  static SystemStats get_current_stats();
  static bool is_system_under_load(double cpu_threshold = 80.0, double memory_threshold = 90.0);

  // Continuous monitoring
  void start_continuous_monitoring(double interval_seconds = 1.0);
  void stop_continuous_monitoring();
  std::vector<SystemStats> get_monitoring_history() const;
};

// Utility macros for easy benchmarking
#define BENCHMARK_FUNCTION(profiler, name, func) \
  do {                                           \
    auto scoped = profiler.measure(name);        \
    func();                                      \
  } while (0)

#define BENCHMARK_BLOCK(profiler, name) auto BENCHMARK_VAR(scoped) = profiler.measure(name);

#define BENCHMARK_VAR(name) benchmark_scoped_##name

}  // namespace Benchmark

#endif  // BENCHMARK_UTILS_H
