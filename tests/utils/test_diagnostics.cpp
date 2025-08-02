/**
 * @file test_diagnostics.cpp
 * @brief Simplified implementation of diagnostic logging system
 */

#include "test_diagnostics.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

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

std::string SystemStateCapture::get_memory_info() { return "Memory info not available"; }

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

  active_monitors_[test_name] = metrics;
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
  metrics.duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(metrics.end_time - metrics.start_time);

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

}  // namespace TestUtils
