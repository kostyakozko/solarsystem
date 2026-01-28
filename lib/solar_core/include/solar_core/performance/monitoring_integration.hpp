/**
 * @file monitoring_integration.hpp
 * @brief Comprehensive monitoring integration for Solar System Suite
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "solar_core/export.hpp"
#include "solar_core/performance/component_monitors.hpp"
#include "solar_core/performance/performance_monitor.hpp"

namespace SolarSystem::Performance {

/**
 * @brief Application types in Solar System Suite
 */
enum class ApplicationType { LAUNCHER, FETCH, SIMULATION, REALTIME, WEB };

/**
 * @brief Application-specific monitoring context (Task 10.1)
 */
class SOLAR_CORE_API ApplicationMonitor {
 public:
  explicit ApplicationMonitor(ApplicationType type, const std::string& name);

  // Lifecycle
  void start();
  void stop();
  bool is_running() const;

  // Application metrics
  void record_startup_time(double seconds);
  void record_operation(const std::string& operation, double duration_ms);
  void record_error(const std::string& error_type);
  void record_user_action(const std::string& action, double latency_ms);

  // Cross-application correlation
  std::string get_correlation_id() const;
  void set_correlation_id(const std::string& id);

  // Statistics
  struct Stats {
    ApplicationType type;
    std::string name;
    double startup_time_s = 0;
    size_t total_operations = 0;
    size_t error_count = 0;
    double avg_operation_time_ms = 0;
    std::chrono::system_clock::time_point start_time;
    std::chrono::steady_clock::duration uptime{0};
  };
  Stats get_stats() const;

 private:
  ApplicationType type_;
  std::string name_;
  std::string correlation_id_;
  std::atomic<bool> running_{false};
  std::chrono::system_clock::time_point start_time_;
  std::chrono::steady_clock::time_point start_steady_;

  mutable std::mutex mutex_;
  double startup_time_ = 0;
  double total_op_time_ = 0;
  size_t op_count_ = 0;
  size_t error_count_ = 0;
};

/**
 * @brief System resource metrics
 */
struct SystemResources {
  double cpu_usage_percent = 0;
  size_t memory_used_bytes = 0;
  size_t memory_available_bytes = 0;
  double memory_usage_percent = 0;
  size_t disk_used_bytes = 0;
  size_t disk_available_bytes = 0;
  double disk_usage_percent = 0;
};

/**
 * @brief System-wide resource monitor (Task 10.2)
 */
class SOLAR_CORE_API SystemMonitor {
 public:
  static SystemMonitor& instance();

  // Resource monitoring
  SystemResources get_current_resources() const;
  void record_sample();

  // Thresholds
  void set_cpu_threshold(double percent);
  void set_memory_threshold(double percent);
  void set_disk_threshold(double percent);

  // Alerts
  struct ResourceAlert {
    std::string resource;
    double current_value;
    double threshold;
    std::chrono::system_clock::time_point timestamp;
  };
  std::vector<ResourceAlert> check_thresholds() const;

  // History
  std::vector<SystemResources> get_history(size_t limit = 100) const;

 private:
  SystemMonitor() = default;
  SystemResources collect_resources() const;

  mutable std::mutex mutex_;
  std::vector<SystemResources> history_;
  double cpu_threshold_ = 90.0;
  double memory_threshold_ = 90.0;
  double disk_threshold_ = 90.0;
  static constexpr size_t MAX_HISTORY = 1000;
};

/**
 * @brief Cross-application correlation tracker
 */
class SOLAR_CORE_API CorrelationTracker {
 public:
  static CorrelationTracker& instance();

  // Correlation management
  std::string generate_correlation_id();
  void register_application(const std::string& correlation_id, ApplicationType type);
  void record_handoff(const std::string& correlation_id, ApplicationType from, ApplicationType to);

  // Query
  struct CorrelationEntry {
    std::string correlation_id;
    std::vector<ApplicationType> app_chain;
    std::chrono::system_clock::time_point start_time;
    double total_duration_ms = 0;
  };
  std::vector<CorrelationEntry> get_recent_correlations(size_t limit = 50) const;

  void clear();

 private:
  CorrelationTracker() = default;

  mutable std::mutex mutex_;
  std::map<std::string, CorrelationEntry> correlations_;
  std::atomic<uint64_t> id_counter_{0};
};

// Convenience macros
#define APP_MONITOR_START(monitor) (monitor).start()
#define APP_MONITOR_STOP(monitor) (monitor).stop()
#define APP_MONITOR_OP(monitor, op, duration) (monitor).record_operation(op, duration)

}  // namespace SolarSystem::Performance
