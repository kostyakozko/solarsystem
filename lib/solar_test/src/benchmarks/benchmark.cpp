#include "solar_test/benchmarks/benchmark.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <psapi.h>
#include <windows.h>
#endif

namespace SolarSystem::Testing {

// BenchmarkResult implementation
bool BenchmarkResult::passed_thresholds() const {
  // Check if any threshold violations are recorded in metadata
  auto it = metadata.find("threshold_violations");
  return it == metadata.end() || it->second.empty();
}

std::string BenchmarkResult::to_csv_row() const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(6);

  // Convert nanoseconds to milliseconds for CSV output
  double min_ms = static_cast<double>(min_time.count()) / 1e6;
  double max_ms = static_cast<double>(max_time.count()) / 1e6;
  double mean_ms = static_cast<double>(mean_time.count()) / 1e6;
  double std_dev_ms = static_cast<double>(std_dev.count()) / 1e6;

  oss << name << "," << mean_ms << "," << min_ms << "," << max_ms << "," << std_dev_ms << ","
      << iterations << "," << operations_per_second << "," << memory_usage_bytes;

  return oss.str();
}

std::string BenchmarkResult::csv_header() {
  return "Name,AvgDuration(ms),MinDuration(ms),MaxDuration(ms),StdDev(ms),Iterations,OpsPerSec,"
         "MemoryUsage(bytes)";
}

// Benchmark implementation
Benchmark::Benchmark(const std::string& name) : name_(name), config_({}) {}

Benchmark::Benchmark(const std::string& name, Configuration config)
    : name_(name), config_(std::move(config)) {}

void Benchmark::set_thresholds(const PerformanceThresholds& thresholds) {
  thresholds_ = thresholds;
}

void Benchmark::add_metadata(const std::string& key, const std::string& value) {
  metadata_[key] = value;
}

BenchmarkResult Benchmark::calculate_statistics(
    const std::vector<std::chrono::nanoseconds>& timings, size_t memory_usage) const {
  BenchmarkResult result;
  result.name = name_;
  result.iterations = timings.size();
  result.memory_usage_bytes = memory_usage;
  result.metadata = metadata_;

  if (timings.empty()) {
    return result;
  }

  // Calculate min, max
  auto [min_it, max_it] = std::minmax_element(timings.begin(), timings.end());
  result.min_time = *min_it;
  result.max_time = *max_it;

  // Calculate mean
  auto total_time = std::accumulate(timings.begin(), timings.end(), std::chrono::nanoseconds{0});
  result.mean_time = total_time / timings.size();

  // Calculate median
  std::vector<std::chrono::nanoseconds> sorted_timings = timings;
  std::sort(sorted_timings.begin(), sorted_timings.end());
  size_t mid = sorted_timings.size() / 2;
  if (sorted_timings.size() % 2 == 0) {
    result.median_time = (sorted_timings[mid - 1] + sorted_timings[mid]) / 2;
  } else {
    result.median_time = sorted_timings[mid];
  }

  // Calculate standard deviation
  double mean_ns = static_cast<double>(result.mean_time.count());
  double variance = 0.0;
  for (const auto& timing : timings) {
    double diff = static_cast<double>(timing.count()) - mean_ns;
    variance += diff * diff;
  }
  variance /= static_cast<double>(timings.size());
  result.std_dev = std::chrono::nanoseconds(static_cast<long long>(std::sqrt(variance)));

  // Calculate operations per second
  if (result.mean_time.count() > 0) {
    result.operations_per_second = 1e9 / static_cast<double>(result.mean_time.count());
  }

  return result;
}

size_t Benchmark::get_current_memory_usage() const {
  if (!config_.measure_memory) {
    return 0;
  }

#ifdef __APPLE__
  struct mach_task_basic_info info;
  mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;

  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info),
                &info_count) == KERN_SUCCESS) {
    return info.resident_size;
  }
  return 0;

#elif defined(__linux__)
  std::ifstream status_file("/proc/self/status");
  std::string line;

  while (std::getline(status_file, line)) {
    if (line.substr(0, 6) == "VmRSS:") {
      std::istringstream iss(line);
      std::string label;
      size_t value;
      std::string unit;

      if (iss >> label >> value >> unit) {
        // Convert from kB to bytes
        return value * 1024;
      }
    }
  }
  return 0;

#elif defined(_WIN32)
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    return pmc.WorkingSetSize;
  }
  return 0;

#else
  // Fallback: use getrusage if available
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
// ru_maxrss is in kilobytes on Linux, bytes on macOS
#ifdef __linux__
    return usage.ru_maxrss * 1024;
#else
    return usage.ru_maxrss;
#endif
  }
  return 0;
#endif
}

void Benchmark::validate_thresholds(BenchmarkResult& result) const {
  if (!thresholds_) {
    return;
  }

  std::vector<std::string> violations;

  // Check time threshold
  if (thresholds_->max_time && result.mean_time > *thresholds_->max_time) {
    std::ostringstream oss;
    oss << "Mean time " << result.mean_time.count() << "ns exceeds threshold "
        << thresholds_->max_time->count() << "ns";
    violations.push_back(oss.str());
  }

  // Check memory threshold
  if (thresholds_->max_memory_bytes && result.memory_usage_bytes > *thresholds_->max_memory_bytes) {
    std::ostringstream oss;
    oss << "Memory usage " << result.memory_usage_bytes << " bytes exceeds threshold "
        << *thresholds_->max_memory_bytes << " bytes";
    violations.push_back(oss.str());
  }

  // Check operations per second threshold
  if (thresholds_->min_operations_per_second &&
      result.operations_per_second < *thresholds_->min_operations_per_second) {
    std::ostringstream oss;
    oss << "Operations per second " << result.operations_per_second << " below threshold "
        << *thresholds_->min_operations_per_second;
    violations.push_back(oss.str());
  }

  // Check standard deviation threshold
  if (thresholds_->max_std_dev_percentage && result.mean_time.count() > 0) {
    double std_dev_percentage = (static_cast<double>(result.std_dev.count()) /
                                 static_cast<double>(result.mean_time.count())) *
                                100.0;

    if (std_dev_percentage > *thresholds_->max_std_dev_percentage) {
      std::ostringstream oss;
      oss << "Standard deviation " << std_dev_percentage << "% exceeds threshold "
          << *thresholds_->max_std_dev_percentage << "%";
      violations.push_back(oss.str());
    }
  }

  // Store violations in metadata
  if (!violations.empty()) {
    std::ostringstream oss;
    for (size_t i = 0; i < violations.size(); ++i) {
      if (i > 0) oss << "; ";
      oss << violations[i];
    }
    result.metadata["threshold_violations"] = oss.str();
  }
}

// BenchmarkSuite implementation
BenchmarkSuite::BenchmarkSuite(const std::string& name) : name_(name) {}

void BenchmarkSuite::add_benchmark(std::unique_ptr<Benchmark> benchmark) {
  benchmarks_.push_back(std::move(benchmark));
}

std::vector<BenchmarkResult> BenchmarkSuite::run_all() {
  std::vector<BenchmarkResult> results;
  results.reserve(benchmarks_.size());

  for (const auto& benchmark : benchmarks_) {
    // For now, we'll create a simple lambda that does nothing
    // In practice, this would be replaced by the actual benchmark function
    auto dummy_func = []() {
      // Simulate some work
      volatile int sum = 0;
      for (int i = 0; i < 1000; ++i) {
        sum = sum + i;  // Avoid compound assignment with volatile
      }
    };

    auto result = benchmark->measure(dummy_func);
    results.push_back(std::move(result));
  }

  return results;
}

std::optional<BenchmarkResult> BenchmarkSuite::run_benchmark(const std::string& name) {
  for (const auto& benchmark : benchmarks_) {
    if (benchmark->name() == name) {
      // For now, we'll create a simple lambda that does nothing
      auto dummy_func = []() {
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) {
          sum = sum + i;  // Avoid compound assignment with volatile
        }
      };

      return benchmark->measure(dummy_func);
    }
  }
  return std::nullopt;
}

void BenchmarkSuite::generate_csv_report(const std::string& output_path,
                                         const std::vector<BenchmarkResult>& results) {
  std::ofstream file(output_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open output file: " + output_path);
  }

  // Write CSV header
  file << BenchmarkResult::csv_header() << "\n";

  // Write results
  for (const auto& result : results) {
    file << result.to_csv_row() << "\n";
  }

  file.close();
}

}  // namespace SolarSystem::Testing
