/**
 * @file test_diagnostics.cpp
 * @brief Simplified implementation of diagnostic logging system
 */

#include "test_diagnostics.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef __APPLE__
#include <sys/resource.h>
#elif __linux__
#include <unistd.h>
#endif

#include "test_port_manager.hpp"

namespace TestUtils {

// TestDiagnosticLogger implementation
TestDiagnosticLogger& TestDiagnosticLogger::instance() {
  static TestDiagnosticLogger instance;
  return instance;
}

TestDiagnosticLogger::~TestDiagnosticLogger() { flush(); }

void TestDiagnosticLogger::configure(const Config& config) {
  std::lock_guard<std::mutex> lock(log_mutex_);
  config_ = config;
}

void TestDiagnosticLogger::log(LogLevel level, const std::string& test_name,
                               const std::string& category, const std::string& message,
                               const std::map<std::string, std::string>& context,
                               const std::string& file, int line, const std::string& function) {
  if (level < config_.min_level) {
    return;
  }

  std::lock_guard<std::mutex> lock(log_mutex_);

  LogEntry entry;
  entry.timestamp = std::chrono::system_clock::now();
  entry.level = level;
  entry.test_name = test_name;
  entry.category = category;
  entry.message = message;
  entry.context = context;
  entry.file = file;
  entry.line = line;
  entry.function = function;

  log_entries_.push_back(entry);

  if (config_.log_to_console) {
    write_to_console(entry);
  }
}

void TestDiagnosticLogger::debug(const std::string& test_name, const std::string& message,
                                 const std::map<std::string, std::string>& context,
                                 const std::string& file, int line, const std::string& function) {
  log(LogLevel::DEBUG, test_name, "DEBUG", message, context, file, line, function);
}

void TestDiagnosticLogger::info(const std::string& test_name, const std::string& message,
                                const std::map<std::string, std::string>& context,
                                const std::string& file, int line, const std::string& function) {
  log(LogLevel::INFO, test_name, "INFO", message, context, file, line, function);
}

void TestDiagnosticLogger::warning(const std::string& test_name, const std::string& message,
                                   const std::map<std::string, std::string>& context,
                                   const std::string& file, int line, const std::string& function) {
  log(LogLevel::WARNING, test_name, "WARNING", message, context, file, line, function);
}

void TestDiagnosticLogger::error(const std::string& test_name, const std::string& message,
                                 const std::map<std::string, std::string>& context,
                                 const std::string& file, int line, const std::string& function) {
  log(LogLevel::ERROR, test_name, "ERROR", message, context, file, line, function);
}

void TestDiagnosticLogger::critical(const std::string& test_name, const std::string& message,
                                    const std::map<std::string, std::string>& context,
                                    const std::string& file, int line,
                                    const std::string& function) {
  log(LogLevel::CRITICAL, test_name, "CRITICAL", message, context, file, line, function);
}

void TestDiagnosticLogger::test_started(const std::string& test_name,
                                        const std::string& test_suite) {
  std::map<std::string, std::string> context;
  if (!test_suite.empty()) {
    context["test_suite"] = test_suite;
  }

  info(test_name, "Test started", context);
}

void TestDiagnosticLogger::test_completed(const std::string& test_name, bool success,
                                          const std::string& error_message) {
  std::map<std::string, std::string> context;
  context["success"] = success ? "true" : "false";
  if (!error_message.empty()) {
    context["error"] = error_message;
  }

  if (success) {
    info(test_name, "Test completed successfully", context);
  } else {
    error(test_name, "Test failed: " + error_message, context);
  }
}

void TestDiagnosticLogger::test_failed(const std::string& test_name,
                                       const std::string& error_message, const std::string& file,
                                       int line) {
  std::map<std::string, std::string> context;
  context["error_message"] = error_message;
  if (!file.empty()) {
    context["source_file"] = file;
    context["source_line"] = std::to_string(line);
  }

  error(test_name, "Test assertion failed", context, file, line);
}

void TestDiagnosticLogger::capture_system_state(const std::string& test_name,
                                                const std::string& reason) {
  std::map<std::string, std::string> context;
  context["reason"] = reason;
  context["hostname"] = "localhost";  // Simplified

  info(test_name, "System state captured", context);
}

void TestDiagnosticLogger::capture_resource_state(const std::string& test_name) {
  std::map<std::string, std::string> context;

  // Port allocation state
  auto& port_manager = TestPortManager::instance();
  auto allocated_ports = port_manager.get_allocated_ports();
  context["allocated_ports_count"] = std::to_string(allocated_ports.size());

  // Resource manager state
  auto& resource_manager = TestResourceManager::instance();
  context["total_resources"] = std::to_string(resource_manager.get_resource_count());

  info(test_name, "Resource state captured", context);
}

void TestDiagnosticLogger::capture_network_state(const std::string& test_name) {
  std::map<std::string, std::string> context;
  context["network_info"] = "simplified";

  info(test_name, "Network state captured", context);
}

void TestDiagnosticLogger::capture_process_state(const std::string& test_name) {
  std::map<std::string, std::string> context;
  context["process_info"] = "simplified";

  info(test_name, "Process state captured", context);
}

std::string TestDiagnosticLogger::generate_test_report(const std::string& test_name) const {
  std::lock_guard<std::mutex> lock(log_mutex_);

  std::ostringstream report;
  report << "=== Test Diagnostic Report: " << test_name << " ===\n\n";

  // Filter entries for this test
  std::vector<LogEntry> test_entries;
  std::copy_if(log_entries_.begin(), log_entries_.end(), std::back_inserter(test_entries),
               [&test_name](const LogEntry& entry) { return entry.test_name == test_name; });

  if (test_entries.empty()) {
    report << "No diagnostic entries found for test: " << test_name << "\n";
    return report.str();
  }

  report << "Total log entries: " << test_entries.size() << "\n";

  return report.str();
}

std::string TestDiagnosticLogger::generate_failure_analysis(const std::string& test_name) const {
  std::lock_guard<std::mutex> lock(log_mutex_);

  std::ostringstream report;
  report << "=== Failure Analysis: " << test_name << " ===\n\n";
  report << "Simplified failure analysis - check logs for details\n";

  return report.str();
}

std::string TestDiagnosticLogger::generate_system_summary() const {
  return "System Summary: Simplified implementation\n";
}

void TestDiagnosticLogger::flush() {
  // Simplified implementation - nothing to flush
}

void TestDiagnosticLogger::rotate_logs() {
  // Simplified implementation
}

void TestDiagnosticLogger::cleanup_old_logs() {
  // Simplified implementation
}

std::vector<std::string> TestDiagnosticLogger::get_log_files() const {
  return {};  // Simplified implementation
}

void TestDiagnosticLogger::write_to_console(const LogEntry& entry) {
  if (!config_.log_to_console) {
    return;
  }

  std::string level_color;
  switch (entry.level) {
    case LogLevel::DEBUG:
      level_color = "\033[0;37m";
      break;  // White
    case LogLevel::INFO:
      level_color = "\033[0;36m";
      break;  // Cyan
    case LogLevel::WARNING:
      level_color = "\033[0;33m";
      break;  // Yellow
    case LogLevel::ERROR:
      level_color = "\033[0;31m";
      break;  // Red
    case LogLevel::CRITICAL:
      level_color = "\033[1;31m";
      break;  // Bold Red
  }

  std::cout << level_color << "[" << get_log_level_string(entry.level) << "]"
            << "\033[0m " << entry.test_name << ": " << entry.message << std::endl;
}

std::string TestDiagnosticLogger::get_log_level_string(LogLevel level) const {
  switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARN";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::CRITICAL:
      return "CRIT";
    default:
      return "UNKNOWN";
  }
}

// SystemStateCapture implementation (simplified)
SystemStateCapture::SystemState SystemStateCapture::capture_current_state() {
  SystemState state;
  state.timestamp = "simplified";
  state.hostname = "localhost";
  state.os_info = "macOS/Linux";
  state.cpu_info = "CPU info not available";
  state.memory_info = "Memory info not available";
  state.disk_info = "Disk info not available";
  state.network_info = "Network info not available";

  return state;
}

std::string SystemStateCapture::format_system_state(const SystemState& state) {
  std::ostringstream oss;
  oss << "=== System State Report ===\n";
  oss << "Timestamp: " << state.timestamp << "\n";
  oss << "Hostname: " << state.hostname << "\n";
  oss << "OS: " << state.os_info << "\n";

  return oss.str();
}

std::string SystemStateCapture::get_os_info() { return "macOS/Linux"; }

std::string SystemStateCapture::get_cpu_info() { return "CPU info not available"; }

std::string SystemStateCapture::get_memory_info() {
  auto system_info = PlatformMemoryMonitor::get_system_memory_info();
  return PlatformMemoryMonitor::format_system_memory_info(system_info);
}

std::string SystemStateCapture::get_disk_info() { return "Disk info not available"; }

std::string SystemStateCapture::get_network_info() { return "Network info not available"; }

std::vector<std::string> SystemStateCapture::get_running_processes() { return {}; }

std::vector<int> SystemStateCapture::get_open_ports() { return {}; }

std::map<std::string, std::string> SystemStateCapture::get_system_limits() { return {}; }

// FailureAnalyzer implementation (simplified)
FailureAnalyzer::AnalysisResult FailureAnalyzer::analyze_test_failure(
    const std::string& test_name, const std::string& error_message,
    const std::vector<LogEntry>& /* log_entries */) {
  AnalysisResult result;
  result.test_name = test_name;
  result.confidence_score = 0.5;

  // Simple pattern matching (case-insensitive)
  std::string lower_message = error_message;
  std::transform(lower_message.begin(), lower_message.end(), lower_message.begin(), ::tolower);

  if (lower_message.find("port") != std::string::npos) {
    result.failure_category = "Network/Port Issue";
    result.confidence_score = 0.8;
    result.root_cause_analysis = "Port-related issue detected";
    result.recommendations.push_back("Use TestPortManager for port allocation");
  } else if (lower_message.find("timeout") != std::string::npos) {
    result.failure_category = "Timeout Issue";
    result.confidence_score = 0.7;
    result.root_cause_analysis = "Timeout detected";
    result.recommendations.push_back("Increase timeout values");
  } else {
    result.failure_category = "Unknown Issue";
    result.confidence_score = 0.3;
    result.root_cause_analysis = "Unknown error pattern";
    result.recommendations.push_back("Manual investigation required");
  }

  return result;
}

FailureAnalyzer::AnalysisResult FailureAnalyzer::analyze_timeout_failure(
    const std::string& test_name, int timeout_seconds,
    const std::vector<LogEntry>& /* log_entries */) {
  AnalysisResult result;
  result.test_name = test_name;
  result.failure_category = "Timeout Failure";
  result.confidence_score = 0.9;
  result.root_cause_analysis =
      "Test exceeded timeout of " + std::to_string(timeout_seconds) + " seconds";
  result.recommendations.push_back("Increase timeout value");

  return result;
}

std::vector<std::string> FailureAnalyzer::identify_error_patterns(
    const std::string& error_message) {
  std::vector<std::string> patterns;

  if (error_message.find("port") != std::string::npos) {
    patterns.push_back("Port-related error");
  }
  if (error_message.find("timeout") != std::string::npos) {
    patterns.push_back("Timeout error");
  }

  return patterns;
}

std::vector<std::string> FailureAnalyzer::identify_resource_patterns(
    const std::vector<LogEntry>& /* entries */) {
  return {};  // Simplified
}

std::vector<std::string> FailureAnalyzer::identify_timing_patterns(
    const std::vector<LogEntry>& /* entries */) {
  return {};  // Simplified
}

// TestPerformanceMonitor implementation (simplified)
std::map<std::string, TestPerformanceMonitor::PerformanceMetrics>
    TestPerformanceMonitor::active_monitors_;
std::vector<TestPerformanceMonitor::PerformanceMetrics> TestPerformanceMonitor::completed_metrics_;
std::mutex TestPerformanceMonitor::monitor_mutex_;

void TestPerformanceMonitor::start_monitoring(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  PerformanceMetrics metrics;
  metrics.test_name = test_name;
  metrics.start_time = std::chrono::system_clock::now();
  metrics.start_memory = PlatformMemoryMonitor::get_current_memory_usage();
  metrics.peak_memory = metrics.start_memory;
  metrics.memory_leak_detected = false;
  metrics.leaked_bytes = 0;

  active_monitors_[test_name] = metrics;

  // Start memory leak detection
  PlatformMemoryMonitor::start_leak_detection(test_name);
}

TestPerformanceMonitor::PerformanceMetrics TestPerformanceMonitor::stop_monitoring(
    const std::string& test_name) {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  auto it = active_monitors_.find(test_name);
  if (it == active_monitors_.end()) {
    return PerformanceMetrics{};
  }

  PerformanceMetrics metrics = it->second;
  metrics.end_time = std::chrono::system_clock::now();
  metrics.end_memory = PlatformMemoryMonitor::get_current_memory_usage();
  metrics.duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(metrics.end_time - metrics.start_time);

  // Update peak memory if current is higher
  if (metrics.end_memory.resident_set_size > metrics.peak_memory.resident_set_size) {
    metrics.peak_memory = metrics.end_memory;
  }

  // Set legacy fields for compatibility
  metrics.peak_memory_usage = metrics.peak_memory.resident_set_size;
  metrics.average_memory_usage = (metrics.start_memory.resident_set_size + metrics.end_memory.resident_set_size) / 2;

  // Stop memory leak detection
  auto leak_info = PlatformMemoryMonitor::stop_leak_detection(test_name);
  metrics.memory_leak_detected = leak_info.leak_detected;
  metrics.leaked_bytes = leak_info.leaked_bytes;

  completed_metrics_.push_back(metrics);
  active_monitors_.erase(it);

  return metrics;
}

void TestPerformanceMonitor::add_custom_metric(const std::string& test_name,
                                               const std::string& metric_name, double value) {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  auto it = active_monitors_.find(test_name);
  if (it != active_monitors_.end()) {
    it->second.custom_metrics[metric_name] = value;
  }
}

std::vector<TestPerformanceMonitor::PerformanceMetrics> TestPerformanceMonitor::get_all_metrics() {
  std::lock_guard<std::mutex> lock(monitor_mutex_);
  return completed_metrics_;
}

std::optional<TestPerformanceMonitor::PerformanceMetrics>
TestPerformanceMonitor::get_metrics_for_test(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  for (const auto& metrics : completed_metrics_) {
    if (metrics.test_name == test_name) {
      return metrics;
    }
  }

  return std::nullopt;
}

std::string TestPerformanceMonitor::generate_performance_report() {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  std::ostringstream report;
  report << "=== Performance Report ===\n\n";
  report << "Total Tests Monitored: " << completed_metrics_.size() << "\n";

  return report.str();
}

bool TestPerformanceMonitor::detect_performance_regression(const std::string& /* test_name */,
                                                           const PerformanceMetrics& /* baseline */,
                                                           double /* threshold */) {
  return false;  // Simplified implementation
}

// === Platform-Specific Memory Monitoring Implementation ===

// Static member initialization
std::map<std::string, PlatformMemoryMonitor::MemoryInfo> PlatformMemoryMonitor::baseline_memory_;
std::map<std::string, std::vector<PlatformMemoryMonitor::MemoryInfo>> PlatformMemoryMonitor::memory_profiles_;
std::map<std::string, PlatformMemoryMonitor::MemoryLeakInfo> PlatformMemoryMonitor::leak_detection_state_;
std::mutex PlatformMemoryMonitor::memory_monitor_mutex_;

PlatformMemoryMonitor::MemoryInfo PlatformMemoryMonitor::get_current_memory_usage() {
  std::string platform = get_platform_name();

  if (platform == "macOS") {
    return get_memory_usage_macos();
  } else if (platform == "Linux") {
    return get_memory_usage_linux();
  } else if (platform == "Windows") {
    return get_memory_usage_windows();
  } else {
    // Fallback implementation
    MemoryInfo info;
    info.measurement_time = std::chrono::system_clock::now();
    return info;
  }
}

PlatformMemoryMonitor::SystemMemoryInfo PlatformMemoryMonitor::get_system_memory_info() {
  std::string platform = get_platform_name();

  if (platform == "macOS") {
    return get_system_memory_macos();
  } else if (platform == "Linux") {
    return get_system_memory_linux();
  } else if (platform == "Windows") {
    return get_system_memory_windows();
  } else {
    // Fallback implementation
    SystemMemoryInfo info;
    info.measurement_time = std::chrono::system_clock::now();
    return info;
  }
}

std::string PlatformMemoryMonitor::format_memory_info(const MemoryInfo& info) {
  std::ostringstream oss;
  oss << "Memory Usage Report:\n";
  oss << "  RSS: " << (info.resident_set_size / 1024 / 1024) << " MB\n";
  oss << "  Virtual: " << (info.virtual_memory_size / 1024 / 1024) << " MB\n";
  oss << "  Peak RSS: " << (info.peak_resident_set_size / 1024 / 1024) << " MB\n";
  oss << "  Heap: " << (info.heap_size / 1024 / 1024) << " MB\n";
  oss << "  Usage: " << std::fixed << std::setprecision(2) << info.memory_usage_percent << "%\n";
  return oss.str();
}

std::string PlatformMemoryMonitor::format_system_memory_info(const SystemMemoryInfo& info) {
  std::ostringstream oss;
  oss << "System Memory Report:\n";
  oss << "  Total Physical: " << (info.total_physical_memory / 1024 / 1024) << " MB\n";
  oss << "  Available Physical: " << (info.available_physical_memory / 1024 / 1024) << " MB\n";
  oss << "  Used Physical: " << (info.used_physical_memory / 1024 / 1024) << " MB\n";
  oss << "  Memory Pressure: " << std::fixed << std::setprecision(2) << (info.memory_pressure * 100) << "%\n";
  oss << "  Page Size: " << info.page_size << " bytes\n";
  return oss.str();
}

void PlatformMemoryMonitor::start_leak_detection(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);

  MemoryLeakInfo leak_info;
  leak_info.test_name = test_name;
  leak_info.baseline_memory = get_current_memory_usage();
  leak_info.detection_time = std::chrono::system_clock::now();

  leak_detection_state_[test_name] = leak_info;
}

PlatformMemoryMonitor::MemoryLeakInfo PlatformMemoryMonitor::stop_leak_detection(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);

  auto it = leak_detection_state_.find(test_name);
  if (it == leak_detection_state_.end()) {
    MemoryLeakInfo empty_info;
    empty_info.test_name = test_name;
    return empty_info;
  }

  MemoryLeakInfo& leak_info = it->second;
  leak_info.final_memory = get_current_memory_usage();

  // Calculate leaked bytes
  if (leak_info.final_memory.resident_set_size > leak_info.baseline_memory.resident_set_size) {
    leak_info.leaked_bytes = leak_info.final_memory.resident_set_size - leak_info.baseline_memory.resident_set_size;
    leak_info.leak_detected = leak_info.leaked_bytes > (1024 * 1024); // 1MB threshold

    // Calculate leak rate
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        leak_info.final_memory.measurement_time - leak_info.baseline_memory.measurement_time);
    if (duration.count() > 0) {
      leak_info.leak_rate_per_second = static_cast<double>(leak_info.leaked_bytes) / duration.count();
    }
  }

  MemoryLeakInfo result = leak_info;
  leak_detection_state_.erase(it);
  return result;
}

std::vector<PlatformMemoryMonitor::MemoryLeakInfo> PlatformMemoryMonitor::get_all_leak_reports() {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);

  std::vector<MemoryLeakInfo> reports;
  for (const auto& pair : leak_detection_state_) {
    reports.push_back(pair.second);
  }
  return reports;
}

void PlatformMemoryMonitor::start_memory_profiling(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);

  memory_profiles_[test_name] = std::vector<MemoryInfo>();
  memory_profiles_[test_name].push_back(get_current_memory_usage());
}

std::vector<PlatformMemoryMonitor::MemoryInfo> PlatformMemoryMonitor::stop_memory_profiling(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);

  auto it = memory_profiles_.find(test_name);
  if (it == memory_profiles_.end()) {
    return {};
  }

  // Add final measurement
  it->second.push_back(get_current_memory_usage());

  std::vector<MemoryInfo> result = it->second;
  memory_profiles_.erase(it);
  return result;
}

std::string PlatformMemoryMonitor::generate_memory_profile_report(const std::string& test_name) {
  auto profile = stop_memory_profiling(test_name);

  std::ostringstream report;
  report << "Memory Profile Report for " << test_name << ":\n";
  report << "Total measurements: " << profile.size() << "\n";

  if (!profile.empty()) {
    size_t min_rss = profile[0].resident_set_size;
    size_t max_rss = profile[0].resident_set_size;
    size_t total_rss = 0;

    for (const auto& info : profile) {
      min_rss = std::min(min_rss, info.resident_set_size);
      max_rss = std::max(max_rss, info.resident_set_size);
      total_rss += info.resident_set_size;
    }

    report << "RSS - Min: " << (min_rss / 1024 / 1024) << " MB, ";
    report << "Max: " << (max_rss / 1024 / 1024) << " MB, ";
    report << "Avg: " << (total_rss / profile.size() / 1024 / 1024) << " MB\n";
  }

  return report.str();
}

void PlatformMemoryMonitor::set_memory_baseline(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);
  baseline_memory_[test_name] = get_current_memory_usage();
}

bool PlatformMemoryMonitor::detect_memory_regression(const std::string& test_name, double threshold) {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);

  auto it = baseline_memory_.find(test_name);
  if (it == baseline_memory_.end()) {
    return false;
  }

  auto current_memory = get_current_memory_usage();
  double increase_ratio = static_cast<double>(current_memory.resident_set_size) / it->second.resident_set_size;

  return increase_ratio > (1.0 + threshold);
}

std::string PlatformMemoryMonitor::get_memory_regression_report(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(memory_monitor_mutex_);

  auto it = baseline_memory_.find(test_name);
  if (it == baseline_memory_.end()) {
    return "No baseline memory data available for " + test_name;
  }

  auto current_memory = get_current_memory_usage();

  std::ostringstream report;
  report << "Memory Regression Report for " << test_name << ":\n";
  report << "Baseline RSS: " << (it->second.resident_set_size / 1024 / 1024) << " MB\n";
  report << "Current RSS: " << (current_memory.resident_set_size / 1024 / 1024) << " MB\n";

  if (current_memory.resident_set_size > it->second.resident_set_size) {
    size_t increase = current_memory.resident_set_size - it->second.resident_set_size;
    double increase_percent = (static_cast<double>(increase) / it->second.resident_set_size) * 100.0;
    report << "Memory increase: " << (increase / 1024 / 1024) << " MB ("
           << std::fixed << std::setprecision(2) << increase_percent << "%)\n";
  }

  return report.str();
}

std::string PlatformMemoryMonitor::get_platform_name() {
#ifdef __APPLE__
  return "macOS";
#elif __linux__
  return "Linux";
#elif _WIN32
  return "Windows";
#else
  return "Unknown";
#endif
}

bool PlatformMemoryMonitor::is_memory_monitoring_available() {
  std::string platform = get_platform_name();
  return (platform == "macOS" || platform == "Linux" || platform == "Windows");
}

std::vector<std::string> PlatformMemoryMonitor::get_available_memory_metrics() {
  std::vector<std::string> metrics;

  if (is_memory_monitoring_available()) {
    metrics.push_back("resident_set_size");
    metrics.push_back("virtual_memory_size");
    metrics.push_back("peak_resident_set_size");
    metrics.push_back("heap_size");
    metrics.push_back("memory_usage_percent");
  } else {
    metrics.push_back("memory_monitoring_unavailable");
  }

  return metrics;
}

// Platform-specific implementations

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/host_info.h>
#include <sys/sysctl.h>

PlatformMemoryMonitor::MemoryInfo PlatformMemoryMonitor::get_memory_usage_macos() {
  MemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();

  task_t task = mach_task_self();
  struct mach_task_basic_info basic_info;
  mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;

  if (task_info(task, MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&basic_info), &info_count) == KERN_SUCCESS) {
    info.resident_set_size = basic_info.resident_size;
    info.virtual_memory_size = basic_info.virtual_size;
  }

  // Get peak memory usage
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    info.peak_resident_set_size = static_cast<size_t>(usage.ru_maxrss);
  }

  // Calculate memory usage percentage
  auto system_info = get_system_memory_macos();
  if (system_info.total_physical_memory > 0) {
    info.memory_usage_percent = (static_cast<double>(info.resident_set_size) / system_info.total_physical_memory) * 100.0;
  }

  return info;
}

PlatformMemoryMonitor::SystemMemoryInfo PlatformMemoryMonitor::get_system_memory_macos() {
  SystemMemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();

  // Get total physical memory
  int64_t total_memory;
  size_t size = sizeof(total_memory);
  if (sysctlbyname("hw.memsize", &total_memory, &size, NULL, 0) == 0) {
    info.total_physical_memory = static_cast<size_t>(total_memory);
  }

  // Get page size
  vm_size_t page_size;
  if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS) {
    info.page_size = page_size;
  }

  // Get VM statistics
  vm_statistics64_data_t vm_stat;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  if (host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm_stat), &count) == KERN_SUCCESS) {
    info.available_physical_memory = (vm_stat.free_count + vm_stat.inactive_count) * info.page_size;
    info.used_physical_memory = info.total_physical_memory - info.available_physical_memory;

    // Calculate memory pressure
    if (info.total_physical_memory > 0) {
      info.memory_pressure = static_cast<double>(info.used_physical_memory) / info.total_physical_memory;
    }
  }

  return info;
}

#else
// Fallback implementations for non-macOS platforms

PlatformMemoryMonitor::MemoryInfo PlatformMemoryMonitor::get_memory_usage_macos() {
  MemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();
  return info;
}

PlatformMemoryMonitor::SystemMemoryInfo PlatformMemoryMonitor::get_system_memory_macos() {
  SystemMemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();
  return info;
}

#endif

#ifdef __linux__
#include <sys/resource.h>
#include <fstream>
#include <sstream>

PlatformMemoryMonitor::MemoryInfo PlatformMemoryMonitor::get_memory_usage_linux() {
  MemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();

  // Read from /proc/self/status
  std::ifstream status_file("/proc/self/status");
  std::string line;

  while (std::getline(status_file, line)) {
    if (line.find("VmRSS:") == 0) {
      std::istringstream iss(line);
      std::string key, value, unit;
      iss >> key >> value >> unit;
      info.resident_set_size = std::stoull(value) * 1024; // Convert KB to bytes
    } else if (line.find("VmSize:") == 0) {
      std::istringstream iss(line);
      std::string key, value, unit;
      iss >> key >> value >> unit;
      info.virtual_memory_size = std::stoull(value) * 1024; // Convert KB to bytes
    } else if (line.find("VmPeak:") == 0) {
      std::istringstream iss(line);
      std::string key, value, unit;
      iss >> key >> value >> unit;
      info.peak_resident_set_size = std::stoull(value) * 1024; // Convert KB to bytes
    }
  }

  // Calculate memory usage percentage
  auto system_info = get_system_memory_linux();
  if (system_info.total_physical_memory > 0) {
    info.memory_usage_percent = (static_cast<double>(info.resident_set_size) / system_info.total_physical_memory) * 100.0;
  }

  return info;
}

PlatformMemoryMonitor::SystemMemoryInfo PlatformMemoryMonitor::get_system_memory_linux() {
  SystemMemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();

  // Read from /proc/meminfo
  std::ifstream meminfo_file("/proc/meminfo");
  std::string line;

  while (std::getline(meminfo_file, line)) {
    if (line.find("MemTotal:") == 0) {
      std::istringstream iss(line);
      std::string key, value, unit;
      iss >> key >> value >> unit;
      info.total_physical_memory = std::stoull(value) * 1024; // Convert KB to bytes
    } else if (line.find("MemAvailable:") == 0) {
      std::istringstream iss(line);
      std::string key, value, unit;
      iss >> key >> value >> unit;
      info.available_physical_memory = std::stoull(value) * 1024; // Convert KB to bytes
    }
  }

  info.used_physical_memory = info.total_physical_memory - info.available_physical_memory;

  // Calculate memory pressure
  if (info.total_physical_memory > 0) {
    info.memory_pressure = static_cast<double>(info.used_physical_memory) / info.total_physical_memory;
  }

  // Get page size
  info.page_size = getpagesize();

  return info;
}

#else
// Fallback implementations for non-Linux platforms

PlatformMemoryMonitor::MemoryInfo PlatformMemoryMonitor::get_memory_usage_linux() {
  MemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();
  return info;
}

PlatformMemoryMonitor::SystemMemoryInfo PlatformMemoryMonitor::get_system_memory_linux() {
  SystemMemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();
  return info;
}

#endif

// Windows implementation placeholder
PlatformMemoryMonitor::MemoryInfo PlatformMemoryMonitor::get_memory_usage_windows() {
  MemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();
  // TODO: Implement Windows-specific memory monitoring using Windows API
  return info;
}

PlatformMemoryMonitor::SystemMemoryInfo PlatformMemoryMonitor::get_system_memory_windows() {
  SystemMemoryInfo info;
  info.measurement_time = std::chrono::system_clock::now();
  // TODO: Implement Windows-specific system memory monitoring
  return info;
}

}  // namespace TestUtils
