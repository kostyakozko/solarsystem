/**
 * @file benchmark_utils.cpp
 * @brief Implementation of performance benchmarking utilities
 */

#include "benchmark_utils.h"

#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

namespace Benchmark {

// Timer implementation
Timer::Timer() : is_running(false) {}

void Timer::start() {
  start_time = std::chrono::high_resolution_clock::now();
  is_running = true;
}

void Timer::stop() {
  end_time = std::chrono::high_resolution_clock::now();
  is_running = false;
}

void Timer::reset() { is_running = false; }

double Timer::elapsed_ms() const {
  auto end = is_running ? std::chrono::high_resolution_clock::now() : end_time;
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time);
  return static_cast<double>(duration.count()) / 1000.0;
}

double Timer::elapsed_us() const {
  auto end = is_running ? std::chrono::high_resolution_clock::now() : end_time;
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time);
  return static_cast<double>(duration.count());
}

double Timer::elapsed_ns() const {
  auto end = is_running ? std::chrono::high_resolution_clock::now() : end_time;
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_time);
  return static_cast<double>(duration.count());
}

// Profiler implementation
void Profiler::start_measurement(const std::string& name) { active_timers[name].start(); }

void Profiler::end_measurement(const std::string& name) {
  auto it = active_timers.find(name);
  if (it != active_timers.end()) {
    it->second.stop();
    measurements[name].push_back(it->second.elapsed_ms());
    active_timers.erase(it);
  }
}

void Profiler::add_measurement(const std::string& name, double value_ms) {
  measurements[name].push_back(value_ms);
}

PerformanceResult Profiler::get_result(const std::string& name) const {
  PerformanceResult result;
  result.name = name;

  auto it = measurements.find(name);
  if (it == measurements.end() || it->second.empty()) {
    result.duration_ms = 0.0;
    result.min_duration_ms = 0.0;
    result.max_duration_ms = 0.0;
    result.avg_duration_ms = 0.0;
    result.std_deviation_ms = 0.0;
    result.iterations = 0;
    result.operations_per_second = 0.0;
    return result;
  }

  const auto& values = it->second;
  result.iterations = values.size();

  result.min_duration_ms = *std::min_element(values.begin(), values.end());
  result.max_duration_ms = *std::max_element(values.begin(), values.end());
  result.avg_duration_ms =
      std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());

  // Calculate standard deviation
  double variance = 0.0;
  for (double value : values) {
    variance += (value - result.avg_duration_ms) * (value - result.avg_duration_ms);
  }
  result.std_deviation_ms = std::sqrt(variance / static_cast<double>(values.size()));

  result.duration_ms = result.avg_duration_ms;
  result.operations_per_second = result.avg_duration_ms > 0 ? 1000.0 / result.avg_duration_ms : 0.0;

  return result;
}

std::vector<PerformanceResult> Profiler::get_all_results() const {
  std::vector<PerformanceResult> results;
  for (const auto& pair : measurements) {
    results.push_back(get_result(pair.first));
  }
  return results;
}

void Profiler::clear() {
  measurements.clear();
  active_timers.clear();
}

void Profiler::print_summary() const {
  std::cout << "\n=== Performance Summary ===" << std::endl;
  std::cout << std::left << std::setw(30) << "Benchmark" << std::setw(12) << "Avg (ms)"
            << std::setw(12) << "Min (ms)" << std::setw(12) << "Max (ms)" << std::setw(12)
            << "Std Dev" << std::setw(10) << "Ops/sec" << std::setw(8) << "Runs" << std::endl;
  std::cout << std::string(96, '-') << std::endl;

  for (const auto& pair : measurements) {
    auto result = get_result(pair.first);
    std::cout << std::left << std::setw(30) << result.name << std::fixed << std::setprecision(3)
              << std::setw(12) << result.avg_duration_ms << std::setw(12) << result.min_duration_ms
              << std::setw(12) << result.max_duration_ms << std::setw(12) << result.std_deviation_ms
              << std::setw(10) << static_cast<int>(result.operations_per_second) << std::setw(8)
              << result.iterations << std::endl;
  }
  std::cout << std::string(96, '=') << std::endl;
}

// Scoped measurement implementation
Profiler::ScopedMeasurement::ScopedMeasurement(Profiler& p, const std::string& n)
    : profiler(p), name(n) {
  profiler.start_measurement(name);
}

Profiler::ScopedMeasurement::~ScopedMeasurement() { profiler.end_measurement(name); }

Profiler::ScopedMeasurement Profiler::measure(const std::string& name) {
  return ScopedMeasurement(*this, name);
}

// Memory monitor implementation
MemoryMonitor::MemoryMonitor() : baseline_memory(0), peak_memory(0), allocation_count(0) {}

void MemoryMonitor::start_monitoring() {
  baseline_memory = get_current_usage();
  peak_memory = baseline_memory;
}

void MemoryMonitor::stop_monitoring() {
  // Implementation would depend on platform-specific memory tracking
}

MemoryStats MemoryMonitor::get_stats() const {
  MemoryStats stats;
  stats.peak_memory_bytes = peak_memory;
  stats.current_memory_bytes = get_current_usage();
  stats.allocations_count = allocation_count;
  stats.deallocations_count = 0;    // Would need custom allocator tracking
  stats.fragmentation_ratio = 0.0;  // Would need detailed heap analysis
  return stats;
}

size_t MemoryMonitor::get_current_usage() const {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
  // On macOS, ru_maxrss is in bytes
  return static_cast<size_t>(usage.ru_maxrss);
#else
  // On Linux, ru_maxrss is in kilobytes
  return static_cast<size_t>(usage.ru_maxrss) * 1024;
#endif
}

size_t MemoryMonitor::get_peak_usage() const { return peak_memory; }

void MemoryMonitor::reset() {
  baseline_memory = 0;
  peak_memory = 0;
  allocation_count = 0;
}

// Benchmark suite implementation
BenchmarkSuite::BenchmarkSuite(const std::string& name) : suite_name(name) {
  std::cout << "\n=== Benchmark Suite: " << suite_name << " ===" << std::endl;
}

void BenchmarkSuite::run_benchmark(const std::string& name, std::function<void()> benchmark_func,
                                   size_t iterations) {
  std::cout << "Running benchmark: " << name << " (" << iterations << " iterations)..."
            << std::endl;

  memory_monitor.start_monitoring();

  for (size_t i = 0; i < iterations; ++i) {
    auto scoped = profiler.measure(name);
    benchmark_func();
  }

  memory_monitor.stop_monitoring();

  auto result = profiler.get_result(name);
  result.memory_usage_bytes = memory_monitor.get_peak_usage();
  results.push_back(result);

  std::cout << "  Completed: " << std::fixed << std::setprecision(3) << result.avg_duration_ms
            << " ms avg, " << static_cast<int>(result.operations_per_second) << " ops/sec"
            << std::endl;
}

void BenchmarkSuite::run_memory_benchmark(const std::string& name,
                                          std::function<void()> benchmark_func, size_t iterations) {
  std::cout << "Running memory benchmark: " << name << " (" << iterations << " iterations)..."
            << std::endl;

  size_t initial_memory = memory_monitor.get_current_usage();

  for (size_t i = 0; i < iterations; ++i) {
    memory_monitor.start_monitoring();
    auto scoped = profiler.measure(name);
    benchmark_func();
    memory_monitor.stop_monitoring();
  }

  auto result = profiler.get_result(name);
  result.memory_usage_bytes = memory_monitor.get_peak_usage() - initial_memory;
  results.push_back(result);

  std::cout << "  Completed: " << std::fixed << std::setprecision(3) << result.avg_duration_ms
            << " ms avg, " << result.memory_usage_bytes / 1024 << " KB peak memory" << std::endl;
}

void BenchmarkSuite::run_scalability_benchmark(const std::string& name,
                                               std::function<void(size_t)> benchmark_func,
                                               const std::vector<size_t>& input_sizes,
                                               size_t iterations_per_size) {
  std::cout << "Running scalability benchmark: " << name << std::endl;

  for (size_t input_size : input_sizes) {
    std::string size_name = name + "_size_" + std::to_string(input_size);

    for (size_t i = 0; i < iterations_per_size; ++i) {
      auto scoped = profiler.measure(size_name);
      benchmark_func(input_size);
    }

    auto result = profiler.get_result(size_name);
    result.custom_metrics["input_size"] = static_cast<double>(input_size);
    results.push_back(result);

    std::cout << "  Size " << input_size << ": " << std::fixed << std::setprecision(3)
              << result.avg_duration_ms << " ms avg" << std::endl;
  }
}

std::vector<PerformanceResult> BenchmarkSuite::get_results() const { return results; }

void BenchmarkSuite::print_summary() const { profiler.print_summary(); }

void BenchmarkSuite::export_results(const std::string& filename, const std::string& format) const {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
    return;
  }

  if (format == "csv") {
    file << "Name,AvgDuration(ms),MinDuration(ms),MaxDuration(ms),StdDev(ms),Iterations,OpsPerSec,"
            "MemoryUsage(bytes)\n";
    for (const auto& result : results) {
      file << result.name << "," << result.avg_duration_ms << "," << result.min_duration_ms << ","
           << result.max_duration_ms << "," << result.std_deviation_ms << "," << result.iterations
           << "," << result.operations_per_second << "," << result.memory_usage_bytes << "\n";
    }
  }

  file.close();
  std::cout << "Results exported to " << filename << std::endl;
}

// System monitor implementation
SystemMonitor::SystemStats SystemMonitor::get_current_stats() {
  SystemStats stats;

  // Get memory usage
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
  // On macOS, ru_maxrss is in bytes
  stats.memory_usage_bytes = static_cast<size_t>(usage.ru_maxrss);
#else
  // On Linux, ru_maxrss is in kilobytes
  stats.memory_usage_bytes = static_cast<size_t>(usage.ru_maxrss) * 1024;
#endif

  // Get available memory (simplified)
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  stats.available_memory_bytes = static_cast<size_t>(pages) * static_cast<size_t>(page_size);

  // Other stats would require platform-specific implementations
  stats.cpu_usage_percent = 0.0;
  stats.disk_io_rate_mbps = 0.0;
  stats.network_io_rate_mbps = 0.0;
  stats.active_threads = 1;

  return stats;
}

bool SystemMonitor::is_system_under_load(double cpu_threshold, double memory_threshold) {
  auto stats = get_current_stats();
  double memory_usage_percent = static_cast<double>(stats.memory_usage_bytes) /
                                static_cast<double>(stats.available_memory_bytes) * 100.0;

  return stats.cpu_usage_percent > cpu_threshold || memory_usage_percent > memory_threshold;
}

}  // namespace Benchmark
