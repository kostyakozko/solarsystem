/**
 * @file test_diagnostics.hpp
 * @brief Enhanced diagnostic logging and debugging utilities for tests
 *
 * Provides comprehensive diagnostic information for test failures,
 * including system state, resource usage, and detailed error reporting.
 */

#pragma once

#include <chrono>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace TestUtils {

/**
 * @brief Diagnostic log levels
 */
enum class LogLevel { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3, CRITICAL = 4 };

/**
 * @brief Diagnostic log entry
 */
struct LogEntry {
  std::chrono::system_clock::time_point timestamp;
  LogLevel level;
  std::string test_name;
  std::string category;
  std::string message;
  std::map<std::string, std::string> context;
  std::string file;
  int line;
  std::string function;
};

/**
 * @brief Enhanced diagnostic logger for test environments
 *
 * Provides structured logging with context information, automatic
 * system sta and detailed failure analysis.
 */
class TestDiagnosticLogger {
 public:
  /**
   * @brief Logger configuration
   */
  struct Config {
    LogLevel min_level = LogLevel::INFO;
    bool log_to_console = true;
    bool log_to_file = true;
    std::string log_file_prefix = "test_diagnostics";
    bool capture_system_state = true;
    bool capture_resource_state = true;
    int max_log_files = 10;
    size_t max_log_file_size = 10 * 1024 * 1024;  // 10MB
  };

  // Singleton access
  static TestDiagnosticLogger& instance();

  // Configuration
  void configure(const Config& config);
  const Config& get_config() const { return config_; }

  // Logging methods
  void log(LogLevel level, const std::string& test_name, const std::string& category,
           const std::string& message, const std::map<std::string, std::string>& context = {},
           const std::string& file = "", int line = 0, const std::string& function = "");

  void debug(const std::string& test_name, const std::string& message,
             const std::map<std::string, std::string>& context = {}, const std::string& file = "",
             int line = 0, const std::string& function = "");

  void info(const std::string& test_name, const std::string& message,
            const std::map<std::string, std::string>& context = {}, const std::string& file = "",
            int line = 0, const std::string& function = "");

  void warning(const std::string& test_name, const std::string& message,
               const std::map<std::string, std::string>& context = {}, const std::string& file = "",
               int line = 0, const std::string& function = "");

  void error(const std::string& test_name, const std::string& message,
             const std::map<std::string, std::string>& context = {}, const std::string& file = "",
             int line = 0, const std::string& function = "");

  void critical(const std::string& test_name, const std::string& message,
                const std::map<std::string, std::string>& context = {},
                const std::string& file = "", int line = 0, const std::string& function = "");

  // Test lifecycle logging
  void test_started(const std::string& test_name, const std::string& test_suite = "");
  void test_completed(const std::string& test_name, bool success,
                      const std::string& error_message = "");
  void test_failed(const std::string& test_name, const std::string& error_message,
                   const std::string& file = "", int line = 0);

  // System state capture
  void capture_system_state(const std::string& test_name, const std::string& reason = "");
  void capture_resource_state(const std::string& test_name);
  void capture_network_state(const std::string& test_name);
  void capture_process_state(const std::string& test_name);

  // Report generation
  std::string generate_test_report(const std::string& test_name) const;
  std::string generate_failure_analysis(const std::string& test_name) const;
  std::string generate_system_summary() const;

  // Log management
  void flush();
  void rotate_logs();
  void cleanup_old_logs();
  std::vector<std::string> get_log_files() const;

 private:
  TestDiagnosticLogger() = default;
  ~TestDiagnosticLogger();

  // Non-copyable, non-movable
  TestDiagnosticLogger(const TestDiagnosticLogger&) = delete;
  TestDiagnosticLogger& operator=(const TestDiagnosticLogger&) = delete;

  // Internal state
  Config config_;
  mutable std::mutex log_mutex_;
  std::vector<LogEntry> log_entries_;
  std::unique_ptr<std::ofstream> log_file_;
  std::string current_log_filename_;
  size_t current_log_size_ = 0;

  // Helper methods
  void write_to_console(const LogEntry& entry);
  void write_to_file(const LogEntry& entry);
  std::string format_log_entry(const LogEntry& entry) const;
  std::string get_log_level_string(LogLevel level) const;
  std::string get_timestamp_string(const std::chrono::system_clock::time_point& time) const;
  void ensure_log_file_open();
  void rotate_log_file_if_needed();
};

/**
 * @brief System state capture utilities
 */
class SystemStateCapture {
 public:
  /**
   * @brief System state information
   */
  struct SystemState {
    std::string timestamp;
    std::string hostname;
    std::string os_info;
    std::string cpu_info;
    std::string memory_info;
    std::string disk_info;
    std::string network_info;
    std::string process_info;
    std::map<std::string, std::string> environment_vars;
    std::vector<std::string> running_processes;
    std::vector<int> open_ports;
    std::map<std::string, std::string> system_limits;
  };

  // System state capture
  static SystemState capture_current_state();
  static std::string format_system_state(const SystemState& state);

  // Specific state components
  static std::string get_os_info();
  static std::string get_cpu_info();
  static std::string get_memory_info();
  static std::string get_disk_info();
  static std::string get_network_info();
  static std::vector<std::string> get_running_processes();
  static std::vector<int> get_open_ports();
  static std::map<std::string, std::string> get_system_limits();

 private:
  SystemStateCapture() = delete;
};

/**
 * @brief Test failure analysis utilities
 */
class FailureAnalyzer {
 public:
  /**
   * @brief Failure analysis result
   */
  struct AnalysisResult {
    std::string test_name;
    std::string failure_category;
    std::string root_cause_analysis;
    std::vector<std::string> contributing_factors;
    std::vector<std::string> recommendations;
    std::map<std::string, std::string> diagnostic_data;
    double confidence_score;
  };

  // Analysis methods
  static AnalysisResult analyze_test_failure(const std::string& test_name,
                                             const std::string& error_message,
                                             const std::vector<LogEntry>& log_entries);

  static AnalysisResult analyze_timeout_failure(const std::string& test_name, int timeout_seconds,
                                                const std::vector<LogEntry>& log_entries);

  static AnalysisResult analyze_resource_failure(const std::string& test_name,
                                                 const std::string& resource_type,
                                                 const std::vector<LogEntry>& log_entries);

  static AnalysisResult analyze_network_failure(const std::string& test_name,
                                                const std::string& network_error,
                                                const std::vector<LogEntry>& log_entries);

  // Pattern recognition
  static std::vector<std::string> identify_error_patterns(const std::string& error_message);
  static std::vector<std::string> identify_resource_patterns(const std::vector<LogEntry>& entries);
  static std::vector<std::string> identify_timing_patterns(const std::vector<LogEntry>& entries);

 private:
  FailureAnalyzer() = delete;
};

/**
 * @brief Performance monitoring for tests
 */
class TestPerformanceMonitor {
 public:
  /**
   * @brief Performance metrics
   */
  struct PerformanceMetrics {
    std::string test_name;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::milliseconds duration;
    size_t peak_memory_usage;
    size_t average_memory_usage;
    double cpu_usage_percent;
    size_t disk_io_bytes;
    size_t network_io_bytes;
    int context_switches;
    std::map<std::string, double> custom_metrics;
  };

  // Monitoring control
  static void start_monitoring(const std::string& test_name);
  static PerformanceMetrics stop_monitoring(const std::string& test_name);
  static void add_custom_metric(const std::string& test_name, const std::string& metric_name,
                                double value);

  // Metrics access
  static std::vector<PerformanceMetrics> get_all_metrics();
  static std::optional<PerformanceMetrics> get_metrics_for_test(const std::string& test_name);

  // Analysis
  static std::string generate_performance_report();
  static bool detect_performance_regression(const std::string& test_name,
                                            const PerformanceMetrics& baseline,
                                            double threshold = 0.1);

 private:
  TestPerformanceMonitor() = delete;

  static std::map<std::string, PerformanceMetrics> active_monitors_;
  static std::vector<PerformanceMetrics> completed_metrics_;
  static std::mutex monitor_mutex_;
};

}  // namespace TestUtils

// Convenience macros for diagnostic logging
#define TEST_LOG_DEBUG(test_name, message, ...)                                                  \
  TestUtils::TestDiagnosticLogger::instance().debug(test_name, message, ##__VA_ARGS__, __FILE__, \
                                                    __LINE__, __FUNCTION__)

#define TEST_LOG_INFO(test_name, message, ...)                                                  \
  TestUtils::TestDiagnosticLogger::instance().info(test_name, message, ##__VA_ARGS__, __FILE__, \
                                                   __LINE__, __FUNCTION__)

#define TEST_LOG_WARNING(test_name, message, ...)                                                  \
  TestUtils::TestDiagnosticLogger::instance().warning(test_name, message, ##__VA_ARGS__, __FILE__, \
                                                      __LINE__, __FUNCTION__)

#define TEST_LOG_ERROR(test_name, message, ...)                                                  \
  TestUtils::TestDiagnosticLogger::instance().error(test_name, message, ##__VA_ARGS__, __FILE__, \
                                                    __LINE__, __FUNCTION__)

#define TEST_LOG_CRITICAL(test_name, message, ...)                                        \
  TestUtils::TestDiagnosticLogger::instance().critical(test_name, message, ##__VA_ARGS__, \
                                                       __FILE__, __LINE__, __FUNCTION__)

#define TEST_CAPTURE_STATE(test_name, reason) \
  TestUtils::TestDiagnosticLogger::instance().capture_system_state(test_name, reason)

#define TEST_START_MONITORING(test_name) \
  TestUtils::TestPerformanceMonitor::start_monitoring(test_name)

#define TEST_STOP_MONITORING(test_name) \
  TestUtils::TestPerformanceMonitor::stop_monitoring(test_name)
