#include "solar_test/benchmarks/performance_monitor.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <thread>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#include <sys/time.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <psapi.h>
#include <windows.h>
#endif

namespace SolarSystem::Testing {

// MemoryTracker implementation
void MemoryTracker::start_tracking() {
  tracking_ = true;
  start_time_ = std::chrono::high_resolution_clock::now();
  start_memory_ = get_current_usage();
  peak_memory_ = start_memory_;
  allocated_bytes_ = 0;
  deallocated_bytes_ = 0;
}

PerformanceMetrics MemoryTracker::stop_tracking() {
  if (!tracking_) {
    return {};
  }

  tracking_ = false;
  auto end_time = std::chrono::high_resolution_clock::now();

  PerformanceMetrics metrics;
  metrics.wall_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
  metrics.memory_allocated_bytes = allocated_bytes_;
  metrics.memory_deallocated_bytes = deallocated_bytes_;
  metrics.peak_memory_usage_bytes = peak_memory_;

  return metrics;
}

size_t MemoryTracker::get_current_usage() const {
#ifdef __APPLE__
  struct mach_task_basic_info info;
  mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;

  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &info_count) ==
      KERN_SUCCESS) {
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
        return value * 1024;  // Convert from kB to bytes
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
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
#ifdef __linux__
    return usage.ru_maxrss * 1024;
#else
    return usage.ru_maxrss;
#endif
  }
  return 0;
#endif
}

void MemoryTracker::reset() {
  tracking_ = false;
  start_memory_ = 0;
  peak_memory_ = 0;
  allocated_bytes_ = 0;
  deallocated_bytes_ = 0;
}

// CpuMonitor implementation
void CpuMonitor::start_monitoring() {
  monitoring_ = true;
  start_time_ = std::chrono::high_resolution_clock::now();

#if defined(__APPLE__) || defined(__linux__)
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    start_cpu_time_ = std::chrono::seconds(usage.ru_utime.tv_sec) +
                      std::chrono::microseconds(usage.ru_utime.tv_usec) +
                      std::chrono::seconds(usage.ru_stime.tv_sec) +
                      std::chrono::microseconds(usage.ru_stime.tv_usec);
  }
#elif defined(_WIN32)
  FILETIME creation_time, exit_time, kernel_time, user_time;
  if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time)) {
    ULARGE_INTEGER kernel_time_int, user_time_int;
    kernel_time_int.LowPart = kernel_time.dwLowDateTime;
    kernel_time_int.HighPart = kernel_time.dwHighDateTime;
    user_time_int.LowPart = user_time.dwLowDateTime;
    user_time_int.HighPart = user_time.dwHighDateTime;

    // Convert from 100-nanosecond intervals to nanoseconds
    start_cpu_time_ =
        std::chrono::nanoseconds((kernel_time_int.QuadPart + user_time_int.QuadPart) * 100);
  }
#endif
}

double CpuMonitor::stop_monitoring() {
  if (!monitoring_) {
    return 0.0;
  }

  monitoring_ = false;
  auto end_time = std::chrono::high_resolution_clock::now();
  auto wall_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);

  std::chrono::nanoseconds end_cpu_time{0};

#if defined(__APPLE__) || defined(__linux__)
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    end_cpu_time = std::chrono::seconds(usage.ru_utime.tv_sec) +
                   std::chrono::microseconds(usage.ru_utime.tv_usec) +
                   std::chrono::seconds(usage.ru_stime.tv_sec) +
                   std::chrono::microseconds(usage.ru_stime.tv_usec);
  }
#elif defined(_WIN32)
  FILETIME creation_time, exit_time, kernel_time, user_time;
  if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time)) {
    ULARGE_INTEGER kernel_time_int, user_time_int;
    kernel_time_int.LowPart = kernel_time.dwLowDateTime;
    kernel_time_int.HighPart = kernel_time.dwHighDateTime;
    user_time_int.LowPart = user_time.dwLowDateTime;
    user_time_int.HighPart = user_time.dwHighDateTime;

    end_cpu_time =
        std::chrono::nanoseconds((kernel_time_int.QuadPart + user_time_int.QuadPart) * 100);
  }
#endif

  auto cpu_time_used = end_cpu_time - start_cpu_time_;

  if (wall_time.count() > 0) {
    return (static_cast<double>(cpu_time_used.count()) / static_cast<double>(wall_time.count())) *
           100.0;
  }

  return 0.0;
}

double CpuMonitor::get_current_usage() const {
  // Get current CPU usage by sampling over a short period
  static auto last_sample_time = std::chrono::steady_clock::now();
  static std::chrono::nanoseconds last_cpu_time{0};

  auto current_time = std::chrono::steady_clock::now();
  std::chrono::nanoseconds current_cpu_time{0};

#if defined(__APPLE__) || defined(__linux__)
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    current_cpu_time = std::chrono::seconds(usage.ru_utime.tv_sec) +
                       std::chrono::microseconds(usage.ru_utime.tv_usec) +
                       std::chrono::seconds(usage.ru_stime.tv_sec) +
                       std::chrono::microseconds(usage.ru_stime.tv_usec);
  }
#elif defined(_WIN32)
  FILETIME creation_time, exit_time, kernel_time, user_time;
  if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time)) {
    ULARGE_INTEGER kernel_time_int, user_time_int;
    kernel_time_int.LowPart = kernel_time.dwLowDateTime;
    kernel_time_int.HighPart = kernel_time.dwHighDateTime;
    user_time_int.LowPart = user_time.dwLowDateTime;
    user_time_int.HighPart = user_time.dwHighDateTime;
    current_cpu_time =
        std::chrono::nanoseconds((kernel_time_int.QuadPart + user_time_int.QuadPart) * 100);
  }
#endif

  auto wall_time_diff =
      std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_sample_time);
  auto cpu_time_diff = current_cpu_time - last_cpu_time;

  // Update for next sample
  last_sample_time = current_time;
  last_cpu_time = current_cpu_time;

  if (wall_time_diff.count() > 0) {
    return (static_cast<double>(cpu_time_diff.count()) /
            static_cast<double>(wall_time_diff.count())) *
           100.0;
  }

  return 0.0;
}

// PerformanceMonitor implementation
PerformanceMonitor::PerformanceMonitor() : config_({}) {
  if (config_.track_memory) {
    memory_tracker_ = std::make_unique<MemoryTracker>();
  }
  if (config_.track_cpu) {
    cpu_monitor_ = std::make_unique<CpuMonitor>();
  }
}

PerformanceMonitor::PerformanceMonitor(Configuration config) : config_(std::move(config)) {
  if (config_.track_memory) {
    memory_tracker_ = std::make_unique<MemoryTracker>();
  }
  if (config_.track_cpu) {
    cpu_monitor_ = std::make_unique<CpuMonitor>();
  }
}

void PerformanceMonitor::start_monitoring() {
  monitoring_ = true;
  start_time_ = std::chrono::high_resolution_clock::now();

  if (memory_tracker_) {
    memory_tracker_->start_tracking();
  }
  if (cpu_monitor_) {
    cpu_monitor_->start_monitoring();
  }
}

PerformanceMetrics PerformanceMonitor::stop_monitoring() {
  if (!monitoring_) {
    return {};
  }

  monitoring_ = false;
  auto end_time = std::chrono::high_resolution_clock::now();

  PerformanceMetrics metrics;
  metrics.wall_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);

  if (memory_tracker_) {
    auto memory_metrics = memory_tracker_->stop_tracking();
    metrics.memory_allocated_bytes = memory_metrics.memory_allocated_bytes;
    metrics.memory_deallocated_bytes = memory_metrics.memory_deallocated_bytes;
    metrics.peak_memory_usage_bytes = memory_metrics.peak_memory_usage_bytes;
  }

  if (cpu_monitor_) {
    metrics.cpu_usage_percentage = cpu_monitor_->stop_monitoring();
  }

  return metrics;
}

PerformanceMetrics PerformanceMonitor::get_current_metrics() const {
  PerformanceMetrics metrics;

  if (monitoring_) {
    auto current_time = std::chrono::high_resolution_clock::now();
    metrics.wall_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time_);
  }

  if (memory_tracker_) {
    metrics.peak_memory_usage_bytes = memory_tracker_->get_current_usage();
  }

  if (cpu_monitor_) {
    metrics.cpu_usage_percentage = cpu_monitor_->get_current_usage();
  }

  return metrics;
}

// ScopedPerformanceMonitor implementation
ScopedPerformanceMonitor::ScopedPerformanceMonitor(PerformanceMonitor& monitor)
    : monitor_(monitor) {
  monitor_.start_monitoring();
}

ScopedPerformanceMonitor::~ScopedPerformanceMonitor() {
  if (!stopped_) {
    metrics_ = monitor_.stop_monitoring();
    stopped_ = true;
  }
}

const PerformanceMetrics& ScopedPerformanceMonitor::metrics() const { return metrics_; }

// PerformanceProfiler implementation
void PerformanceProfiler::add_profile_point(const std::string& name) {
  ProfilePoint point;
  point.name = name;
  point.timestamp = std::chrono::high_resolution_clock::now();
  point.metrics = monitor_.get_current_metrics();

  profile_points_.push_back(std::move(point));
}

void PerformanceProfiler::add_profile_point(const std::string& name,
                                            const PerformanceMetrics& metrics) {
  ProfilePoint point;
  point.name = name;
  point.timestamp = std::chrono::high_resolution_clock::now();
  point.metrics = metrics;

  profile_points_.push_back(std::move(point));
}

const std::vector<PerformanceProfiler::ProfilePoint>& PerformanceProfiler::get_profile_points()
    const {
  return profile_points_;
}

std::string PerformanceProfiler::generate_report() const {
  std::ostringstream oss;
  oss << "Performance Profile Report\n";
  oss << "==========================\n\n";

  if (profile_points_.empty()) {
    oss << "No profile points recorded.\n";
    return oss.str();
  }

  oss << std::fixed << std::setprecision(3);
  oss << std::setw(20) << "Profile Point" << std::setw(15) << "Wall Time (ms)" << std::setw(15)
      << "CPU Usage (%)" << std::setw(15) << "Memory (KB)"
      << "\n";
  oss << std::string(65, '-') << "\n";

  auto start_time = profile_points_[0].timestamp;

  for (const auto& point : profile_points_) {
    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(point.timestamp - start_time);

    oss << std::setw(20) << point.name << std::setw(15)
        << (static_cast<double>(elapsed.count()) / 1000.0) << std::setw(15)
        << point.metrics.cpu_usage_percentage << std::setw(15)
        << (point.metrics.peak_memory_usage_bytes / 1024) << "\n";
  }

  return oss.str();
}

void PerformanceProfiler::clear() { profile_points_.clear(); }

}  // namespace SolarSystem::Testing
