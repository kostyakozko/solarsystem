/**
 * @file monitoring_integration.cpp
 * @brief Comprehensive monitoring integration implementation
 */

#include "solar_core/performance/monitoring_integration.hpp"

#include <sstream>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <sys/statvfs.h>
#include <sys/sysinfo.h>

#include <fstream>
#endif

namespace SolarSystem::Performance {

// ApplicationMonitor implementation
ApplicationMonitor::ApplicationMonitor(ApplicationType type, const std::string& name)
    : type_(type), name_(name) {}

void ApplicationMonitor::start() {
  running_.store(true);
  start_time_ = std::chrono::system_clock::now();
  start_steady_ = std::chrono::steady_clock::now();
}

void ApplicationMonitor::stop() { running_.store(false); }

bool ApplicationMonitor::is_running() const { return running_.load(); }

void ApplicationMonitor::record_startup_time(double seconds) {
  std::lock_guard<std::mutex> lock(mutex_);
  startup_time_ = seconds;
}

void ApplicationMonitor::record_operation(const std::string& /* operation */, double duration_ms) {
  std::lock_guard<std::mutex> lock(mutex_);
  total_op_time_ += duration_ms;
  op_count_++;
}

void ApplicationMonitor::record_error(const std::string& /* error_type */) {
  std::lock_guard<std::mutex> lock(mutex_);
  error_count_++;
}

void ApplicationMonitor::record_user_action(const std::string& /* action */, double latency_ms) {
  std::lock_guard<std::mutex> lock(mutex_);
  total_op_time_ += latency_ms;
  op_count_++;
}

std::string ApplicationMonitor::get_correlation_id() const { return correlation_id_; }

void ApplicationMonitor::set_correlation_id(const std::string& id) { correlation_id_ = id; }

ApplicationMonitor::Stats ApplicationMonitor::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  Stats stats;
  stats.type = type_;
  stats.name = name_;
  stats.startup_time_s = startup_time_;
  stats.total_operations = op_count_;
  stats.error_count = error_count_;
  stats.avg_operation_time_ms = op_count_ > 0 ? total_op_time_ / static_cast<double>(op_count_) : 0;
  stats.start_time = start_time_;
  if (running_.load()) {
    stats.uptime = std::chrono::steady_clock::now() - start_steady_;
  }
  return stats;
}

// SystemMonitor implementation
SystemMonitor& SystemMonitor::instance() {
  static SystemMonitor instance;
  return instance;
}

SystemResources SystemMonitor::collect_resources() const {
  SystemResources res;

#ifdef __APPLE__
  // Memory info
  mach_port_t host = mach_host_self();
  vm_size_t page_size;
  host_page_size(host, &page_size);

  vm_statistics64_data_t vm_stats;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  if (host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm_stats), &count) ==
      KERN_SUCCESS) {
    uint64_t used = (vm_stats.active_count + vm_stats.wire_count) * page_size;
    uint64_t free = vm_stats.free_count * page_size;
    res.memory_used_bytes = used;
    res.memory_available_bytes = free;
    res.memory_usage_percent = 100.0 * used / (used + free);
  }

  // Disk info
  struct statfs fs;
  if (statfs("/", &fs) == 0) {
    res.disk_available_bytes = static_cast<size_t>(fs.f_bavail) * fs.f_bsize;
    res.disk_used_bytes = static_cast<size_t>(fs.f_blocks - fs.f_bfree) * fs.f_bsize;
    size_t total = static_cast<size_t>(fs.f_blocks) * fs.f_bsize;
    res.disk_usage_percent = total > 0 ? 100.0 * res.disk_used_bytes / total : 0;
  }

  // CPU usage via getrusage (process-level user + system time since last sample)
  {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
      double user_sec = static_cast<double>(usage.ru_utime.tv_sec) + usage.ru_utime.tv_usec / 1e6;
      double sys_sec = static_cast<double>(usage.ru_stime.tv_sec) + usage.ru_stime.tv_usec / 1e6;
      double total_cpu_sec = user_sec + sys_sec;
      auto now = std::chrono::steady_clock::now();
      static auto last_time = now;
      static double last_cpu_sec = total_cpu_sec;
      double wall_elapsed = std::chrono::duration<double>(now - last_time).count();
      if (wall_elapsed > 0.01) {
        res.cpu_usage_percent = 100.0 * (total_cpu_sec - last_cpu_sec) / wall_elapsed;
      }
      last_time = now;
      last_cpu_sec = total_cpu_sec;
    }
  }

#elif defined(__linux__)
  // Memory from /proc/meminfo
  std::ifstream meminfo("/proc/meminfo");
  std::string line;
  size_t mem_total = 0, mem_available = 0;
  while (std::getline(meminfo, line)) {
    if (line.find("MemTotal:") == 0) {
      sscanf(line.c_str(), "MemTotal: %zu kB", &mem_total);
    } else if (line.find("MemAvailable:") == 0) {
      sscanf(line.c_str(), "MemAvailable: %zu kB", &mem_available);
    }
  }
  res.memory_available_bytes = mem_available * 1024;
  res.memory_used_bytes = (mem_total - mem_available) * 1024;
  res.memory_usage_percent =
      mem_total > 0
          ? 100.0 * static_cast<double>(mem_total - mem_available) / static_cast<double>(mem_total)
          : 0;

  // Disk
  struct statvfs fs;
  if (statvfs("/", &fs) == 0) {
    res.disk_available_bytes = fs.f_bavail * fs.f_frsize;
    res.disk_used_bytes = (fs.f_blocks - fs.f_bfree) * fs.f_frsize;
    size_t total = fs.f_blocks * fs.f_frsize;
    res.disk_usage_percent =
        total > 0 ? 100.0 * static_cast<double>(res.disk_used_bytes) / static_cast<double>(total)
                  : 0;
  }

  // CPU usage from /proc/stat
  {
    std::ifstream stat_file("/proc/stat");
    std::string cpu_line;
    if (std::getline(stat_file, cpu_line) && cpu_line.substr(0, 3) == "cpu") {
      unsigned long long user, nice, system, idle, iowait, irq, softirq;
      sscanf(cpu_line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system,
             &idle, &iowait, &irq, &softirq);
      unsigned long long total = user + nice + system + idle + iowait + irq + softirq;
      unsigned long long busy = user + nice + system + irq + softirq;
      static unsigned long long last_total = total;
      static unsigned long long last_busy = busy;
      unsigned long long dtotal = total - last_total;
      if (dtotal > 0) {
        res.cpu_usage_percent =
            100.0 * static_cast<double>(busy - last_busy) / static_cast<double>(dtotal);
      }
      last_total = total;
      last_busy = busy;
    }
  }
#endif

  return res;
}

SystemResources SystemMonitor::get_current_resources() const { return collect_resources(); }

void SystemMonitor::record_sample() {
  auto res = collect_resources();
  std::lock_guard<std::mutex> lock(mutex_);
  if (history_.size() >= MAX_HISTORY) {
    history_.erase(history_.begin());
  }
  history_.push_back(res);
}

void SystemMonitor::set_cpu_threshold(double percent) { cpu_threshold_ = percent; }
void SystemMonitor::set_memory_threshold(double percent) { memory_threshold_ = percent; }
void SystemMonitor::set_disk_threshold(double percent) { disk_threshold_ = percent; }

std::vector<SystemMonitor::ResourceAlert> SystemMonitor::check_thresholds() const {
  auto res = collect_resources();
  std::vector<ResourceAlert> alerts;
  auto now = std::chrono::system_clock::now();

  if (res.cpu_usage_percent > cpu_threshold_) {
    alerts.push_back({"cpu", res.cpu_usage_percent, cpu_threshold_, now});
  }
  if (res.memory_usage_percent > memory_threshold_) {
    alerts.push_back({"memory", res.memory_usage_percent, memory_threshold_, now});
  }
  if (res.disk_usage_percent > disk_threshold_) {
    alerts.push_back({"disk", res.disk_usage_percent, disk_threshold_, now});
  }
  return alerts;
}

std::vector<SystemResources> SystemMonitor::get_history(size_t limit) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (history_.size() <= limit) return history_;
  return std::vector<SystemResources>(history_.end() - static_cast<ptrdiff_t>(limit),
                                      history_.end());
}

// CorrelationTracker implementation
CorrelationTracker& CorrelationTracker::instance() {
  static CorrelationTracker instance;
  return instance;
}

std::string CorrelationTracker::generate_correlation_id() {
  std::ostringstream oss;
  oss << "corr-" << std::hex << id_counter_.fetch_add(1);
  return oss.str();
}

void CorrelationTracker::register_application(const std::string& correlation_id,
                                              ApplicationType type) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto& entry = correlations_[correlation_id];
  if (entry.correlation_id.empty()) {
    entry.correlation_id = correlation_id;
    entry.start_time = std::chrono::system_clock::now();
  }
  entry.app_chain.push_back(type);
}

void CorrelationTracker::record_handoff(const std::string& correlation_id,
                                        ApplicationType /* from */, ApplicationType to) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = correlations_.find(correlation_id);
  if (it != correlations_.end()) {
    it->second.app_chain.push_back(to);
  }
}

std::vector<CorrelationTracker::CorrelationEntry> CorrelationTracker::get_recent_correlations(
    size_t limit) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<CorrelationEntry> result;
  result.reserve(std::min(limit, correlations_.size()));

  for (auto it = correlations_.rbegin(); it != correlations_.rend() && result.size() < limit;
       ++it) {
    result.push_back(it->second);
  }
  return result;
}

void CorrelationTracker::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  correlations_.clear();
}

}  // namespace SolarSystem::Performance
