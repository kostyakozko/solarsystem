/**
 * @file status_management.hpp
 * @brief Comprehensive Status Management System (Task 2)
 *
 * Implements comprehensive status management with:
 * - Real-time component status monitoring
 * - Status dashboard and reporting interface
 * - Health checking for all suite components
 * - Status-based decision making
 * - Advanced metrics collection and analysis
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "solar_utils/error_handling.hpp"
#include "solar_utils/export.hpp"
#include "solar_utils/logging.hpp"
#include "solar_utils/workflow_orchestration.hpp"

namespace SolarSystem::Utils::Status {

/**
 * @brief System-wide status levels
 */
enum class SystemStatus {
  Optimal,   // All components healthy, no issues
  Healthy,   // Minor warnings but fully functional
  Degraded,  // Some components have issues but system operational
  Critical,  // Major issues affecting functionality
  Failed,    // System not operational
  Unknown    // Status cannot be determined
};

/**
 * @brief Status alert severity levels
 */
enum class AlertSeverity {
  Info,      // Informational messages
  Warning,   // Potential issues that should be monitored
  Error,     // Issues that affect functionality
  Critical,  // Severe issues requiring immediate attention
  Emergency  // System-threatening issues
};

/**
 * @brief Status alert types
 */
enum class AlertType {
  ComponentHealth,     // Component health changes
  PerformanceIssue,    // Performance degradation
  ConnectivityIssue,   // Network/service connectivity problems
  ResourceExhaustion,  // Resource usage issues
  SecurityEvent,       // Security-related events
  ConfigurationIssue,  // Configuration problems
  DataIntegrity,       // Data validation/integrity issues
  WorkflowFailure      // Workflow execution failures
};

/**
 * @brief Performance metrics for components
 */
struct PerformanceMetrics {
  std::chrono::nanoseconds response_time{0};
  double cpu_usage_percent = 0.0;
  size_t memory_usage_bytes = 0;
  size_t disk_usage_bytes = 0;
  size_t network_bytes_sent = 0;
  size_t network_bytes_received = 0;
  double throughput_operations_per_second = 0.0;
  size_t error_count = 0;
  size_t success_count = 0;
  std::chrono::system_clock::time_point measurement_time;

  PerformanceMetrics() : measurement_time(std::chrono::system_clock::now()) {}

  /**
   * @brief Calculate success rate
   */
  [[nodiscard]] double get_success_rate() const {
    size_t total = success_count + error_count;
    return total > 0 ? static_cast<double>(success_count) / static_cast<double>(total) : 1.0;
  }

  /**
   * @brief Check if metrics indicate performance issues
   */
  [[nodiscard]] bool has_performance_issues() const {
    return cpu_usage_percent > 80.0 || response_time > std::chrono::milliseconds(5000) ||
           get_success_rate() < 0.95;
  }
};

/**
 * @brief Status alert information
 */
struct StatusAlert {
  std::string id;
  AlertType type;
  AlertSeverity severity;
  std::string title;
  std::string description;
  std::string component_name;
  Workflow::ComponentType component_type;
  std::chrono::system_clock::time_point timestamp;
  std::chrono::system_clock::time_point expires_at;
  bool acknowledged = false;
  std::string acknowledged_by;
  std::chrono::system_clock::time_point acknowledged_at;
  std::unordered_map<std::string, std::string> metadata;
  std::vector<std::string> suggested_actions;

  StatusAlert()
      : timestamp(std::chrono::system_clock::now()),
        expires_at(timestamp + std::chrono::hours(24)) {}

  // Make the struct movable and copyable
  StatusAlert(const StatusAlert&) = default;
  StatusAlert& operator=(const StatusAlert&) = default;
  StatusAlert(StatusAlert&&) = default;
  StatusAlert& operator=(StatusAlert&&) = default;

  /**
   * @brief Check if alert is still active
   */
  [[nodiscard]] bool is_active() const {
    auto now = std::chrono::system_clock::now();
    return !acknowledged && now < expires_at;
  }

  /**
   * @brief Acknowledge the alert
   */
  void acknowledge(const std::string& user = "system") {
    acknowledged = true;
    acknowledged_by = user;
    acknowledged_at = std::chrono::system_clock::now();
  }
};

/**
 * @brief Enhanced component status with detailed metrics
 */
struct EnhancedComponentStatus {
  Workflow::ComponentStatus base_status;
  PerformanceMetrics performance;
  std::vector<StatusAlert> active_alerts;
  std::chrono::system_clock::time_point last_performance_check;
  std::chrono::seconds check_interval{30};
  bool monitoring_enabled = true;
  double availability_percentage = 100.0;
  std::chrono::system_clock::time_point last_availability_calculation;

  EnhancedComponentStatus(Workflow::ComponentType type, const std::string& name)
      : base_status(type, name),
        last_performance_check(std::chrono::system_clock::now()),
        last_availability_calculation(std::chrono::system_clock::now()) {}

  /**
   * @brief Update performance metrics
   */
  void update_performance(const PerformanceMetrics& metrics) {
    performance = metrics;
    last_performance_check = std::chrono::system_clock::now();
  }

  /**
   * @brief Add status alert
   */
  void add_alert(const StatusAlert& alert) {
    // Remove expired alerts
    active_alerts.erase(std::remove_if(active_alerts.begin(), active_alerts.end(),
                                       [](const StatusAlert& a) { return !a.is_active(); }),
                        active_alerts.end());

    // Add new alert
    active_alerts.push_back(alert);
  }

  /**
   * @brief Get overall health score including performance
   */
  [[nodiscard]] double get_comprehensive_health_score() const {
    double base_score = base_status.health_score;
    double performance_score = performance.has_performance_issues() ? 0.7 : 1.0;
    double alert_score = active_alerts.empty() ? 1.0 : 0.8;

    return (base_score + performance_score + alert_score) / 3.0;
  }

  /**
   * @brief Check if component needs attention
   */
  [[nodiscard]] bool needs_attention() const {
    return base_status.health == Workflow::ComponentHealth::Critical ||
           base_status.health == Workflow::ComponentHealth::Failed ||
           performance.has_performance_issues() || !active_alerts.empty();
  }
};

/**
 * @brief System health summary
 */
struct SystemHealthSummary {
  SystemStatus overall_status = SystemStatus::Unknown;
  double overall_health_score = 0.0;
  size_t total_components = 0;
  size_t healthy_components = 0;
  size_t warning_components = 0;
  size_t critical_components = 0;
  size_t failed_components = 0;
  size_t active_alerts = 0;
  size_t critical_alerts = 0;
  std::chrono::system_clock::time_point last_updated;
  std::vector<std::string> top_issues;
  std::vector<std::string> recommendations;

  SystemHealthSummary() : last_updated(std::chrono::system_clock::now()) {}

  /**
   * @brief Check if system is operational
   */
  [[nodiscard]] bool is_operational() const {
    return overall_status == SystemStatus::Optimal || overall_status == SystemStatus::Healthy ||
           overall_status == SystemStatus::Degraded;
  }

  /**
   * @brief Get availability percentage
   */
  [[nodiscard]] double get_availability_percentage() const {
    return total_components > 0 ? static_cast<double>(healthy_components + warning_components) /
                                      static_cast<double>(total_components) * 100.0
                                : 100.0;
  }
};

/**
 * @brief Status dashboard configuration
 */
struct DashboardConfig {
  std::chrono::seconds refresh_interval{5};
  bool show_performance_metrics = true;
  bool show_alerts = true;
  bool show_component_details = true;
  bool show_historical_data = false;
  size_t max_alerts_displayed = 10;
  size_t max_components_displayed = 20;
  AlertSeverity min_alert_severity = AlertSeverity::Warning;
  std::vector<Workflow::ComponentType> component_filter;
  bool auto_acknowledge_info_alerts = true;
  std::chrono::hours alert_retention_period{24};
};

/**
 * @brief Status decision rules for automated responses
 */
struct StatusDecisionRule {
  std::string id;
  std::string name;
  std::string description;
  std::function<bool(const SystemHealthSummary&, const std::vector<EnhancedComponentStatus>&)>
      condition;
  std::function<void(const SystemHealthSummary&, const std::vector<EnhancedComponentStatus>&)>
      action;
  bool enabled = true;
  std::chrono::seconds cooldown_period{300};  // 5 minutes
  std::chrono::system_clock::time_point last_triggered;
  size_t trigger_count = 0;
  size_t max_triggers_per_hour = 10;

  StatusDecisionRule() : last_triggered(std::chrono::system_clock::time_point::min()) {}

  StatusDecisionRule(const std::string& rule_id, const std::string& rule_name)
      : id(rule_id),
        name(rule_name),
        last_triggered(std::chrono::system_clock::time_point::min()) {}

  /**
   * @brief Check if rule can be triggered
   */
  [[nodiscard]] bool can_trigger() const {
    if (!enabled) return false;

    auto now = std::chrono::system_clock::now();
    if (now - last_triggered < cooldown_period) return false;

    // Check trigger rate limiting
    auto hour_ago = now - std::chrono::hours(1);
    if (last_triggered > hour_ago && trigger_count >= max_triggers_per_hour) {
      return false;
    }

    return true;
  }

  /**
   * @brief Mark rule as triggered
   */
  void mark_triggered() {
    auto now = std::chrono::system_clock::now();
    auto hour_ago = now - std::chrono::hours(1);

    // Reset counter if more than an hour has passed
    if (last_triggered < hour_ago) {
      trigger_count = 0;
    }

    last_triggered = now;
    trigger_count++;
  }
};

/**
 * @brief Real-time status monitor
 */
class SOLAR_UTILS_API RealTimeStatusMonitor {
 public:
  /**
   * @brief Constructor
   */
  RealTimeStatusMonitor();

  /**
   * @brief Destructor
   */
  ~RealTimeStatusMonitor();

  /**
   * @brief Start real-time monitoring
   */
  void start_monitoring();

  /**
   * @brief Stop real-time monitoring
   */
  void stop_monitoring();

  /**
   * @brief Register component for enhanced monitoring
   */
  void register_component(Workflow::ComponentType type, const std::string& name,
                          std::function<Workflow::ComponentStatus()> health_check,
                          std::function<PerformanceMetrics()> performance_check = nullptr);

  /**
   * @brief Unregister component
   */
  void unregister_component(Workflow::ComponentType type, const std::string& name);

  /**
   * @brief Get enhanced component status
   */
  [[nodiscard]] std::optional<EnhancedComponentStatus> get_component_status(
      Workflow::ComponentType type, const std::string& name) const;

  /**
   * @brief Get all enhanced component statuses
   */
  [[nodiscard]] std::vector<EnhancedComponentStatus> get_all_component_statuses() const;

  /**
   * @brief Get system health summary
   */
  [[nodiscard]] SystemHealthSummary get_system_health_summary() const;

  /**
   * @brief Add status alert
   */
  void add_alert(const StatusAlert& alert);

  /**
   * @brief Get active alerts
   */
  [[nodiscard]] std::vector<StatusAlert> get_active_alerts(
      AlertSeverity min_severity = AlertSeverity::Info) const;

  /**
   * @brief Acknowledge alert
   */
  void acknowledge_alert(const std::string& alert_id, const std::string& user = "system");

  /**
   * @brief Set monitoring interval
   */
  void set_monitoring_interval(std::chrono::seconds interval);

 private:
  struct ComponentMonitorInfo {
    EnhancedComponentStatus status;
    std::function<Workflow::ComponentStatus()> health_check;
    std::function<PerformanceMetrics()> performance_check;
    std::chrono::system_clock::time_point last_health_check;
    std::chrono::system_clock::time_point last_performance_check;

    ComponentMonitorInfo()
        : status(Workflow::ComponentType::Launcher, ""),
          last_health_check(std::chrono::system_clock::now()),
          last_performance_check(std::chrono::system_clock::now()) {}

    ComponentMonitorInfo(Workflow::ComponentType type, const std::string& name)
        : status(type, name),
          last_health_check(std::chrono::system_clock::now()),
          last_performance_check(std::chrono::system_clock::now()) {}
  };

  mutable std::mutex monitor_mutex_;
  std::unordered_map<std::string, ComponentMonitorInfo> monitored_components_;
  std::vector<StatusAlert> alerts_;
  std::atomic<bool> monitoring_active_{false};
  std::unique_ptr<std::thread> monitoring_thread_;
  std::chrono::seconds monitoring_interval_{5};  // 5 seconds default

  void monitoring_loop();
  void check_component_health(ComponentMonitorInfo& info);
  void check_component_performance(ComponentMonitorInfo& info);
  void generate_health_alerts(const ComponentMonitorInfo& info);
  std::string get_component_key(Workflow::ComponentType type, const std::string& name) const;
  SystemStatus calculate_system_status(
      const std::vector<EnhancedComponentStatus>& components) const;
};

/**
 * @brief Status dashboard for displaying system status
 */
class SOLAR_UTILS_API StatusDashboard {
 public:
  /**
   * @brief Constructor
   */
  explicit StatusDashboard(std::shared_ptr<RealTimeStatusMonitor> monitor);

  /**
   * @brief Set dashboard configuration
   */
  void set_configuration(const DashboardConfig& config);

  /**
   * @brief Display system status overview
   */
  void display_system_overview() const;

  /**
   * @brief Display component details
   */
  void display_component_details() const;

  /**
   * @brief Display active alerts
   */
  void display_active_alerts() const;

  /**
   * @brief Display performance metrics
   */
  void display_performance_metrics() const;

  /**
   * @brief Generate status report
   */
  [[nodiscard]] std::string generate_status_report() const;

  /**
   * @brief Generate JSON status report
   */
  [[nodiscard]] std::string generate_json_report() const;

  /**
   * @brief Start interactive dashboard
   */
  void start_interactive_dashboard();

  /**
   * @brief Stop interactive dashboard
   */
  void stop_interactive_dashboard();

 private:
  std::shared_ptr<RealTimeStatusMonitor> monitor_;
  DashboardConfig config_;
  std::atomic<bool> dashboard_active_{false};
  std::unique_ptr<std::thread> dashboard_thread_;

  void interactive_dashboard_loop();
  void display_header() const;
  void display_system_health_bar(const SystemHealthSummary& summary) const;
  void display_component_table(const std::vector<EnhancedComponentStatus>& components) const;
  void display_alert_table(const std::vector<StatusAlert>& alerts) const;
  std::string format_health_status(Workflow::ComponentHealth health) const;
  std::string format_alert_severity(AlertSeverity severity) const;
  std::string format_duration(std::chrono::system_clock::time_point start) const;
};

/**
 * @brief Status-based decision engine
 */
class SOLAR_UTILS_API StatusDecisionEngine {
 public:
  /**
   * @brief Constructor
   */
  explicit StatusDecisionEngine(std::shared_ptr<RealTimeStatusMonitor> monitor);

  /**
   * @brief Add decision rule
   */
  void add_decision_rule(const StatusDecisionRule& rule);

  /**
   * @brief Remove decision rule
   */
  void remove_decision_rule(const std::string& rule_id);

  /**
   * @brief Enable/disable decision rule
   */
  void set_rule_enabled(const std::string& rule_id, bool enabled);

  /**
   * @brief Start decision engine
   */
  void start_engine();

  /**
   * @brief Stop decision engine
   */
  void stop_engine();

  /**
   * @brief Evaluate all rules manually
   */
  void evaluate_rules();

  /**
   * @brief Get rule statistics
   */
  [[nodiscard]] std::vector<StatusDecisionRule> get_rule_statistics() const;

 private:
  std::shared_ptr<RealTimeStatusMonitor> monitor_;
  mutable std::mutex rules_mutex_;
  std::unordered_map<std::string, StatusDecisionRule> decision_rules_;
  std::atomic<bool> engine_active_{false};
  std::unique_ptr<std::thread> engine_thread_;
  std::chrono::seconds evaluation_interval_{10};  // 10 seconds

  void decision_engine_loop();
  void evaluate_rule(StatusDecisionRule& rule);
  void setup_default_rules();
};

/**
 * @brief Comprehensive status management system
 */
class SOLAR_UTILS_API StatusManager {
 public:
  /**
   * @brief Get singleton instance
   */
  static StatusManager& instance();

  /**
   * @brief Initialize status management system
   */
  void initialize();

  /**
   * @brief Shutdown status management system
   */
  void shutdown();

  /**
   * @brief Get real-time status monitor
   */
  [[nodiscard]] std::shared_ptr<RealTimeStatusMonitor> get_monitor() { return monitor_; }

  /**
   * @brief Get status dashboard
   */
  [[nodiscard]] std::shared_ptr<StatusDashboard> get_dashboard() { return dashboard_; }

  /**
   * @brief Get decision engine
   */
  [[nodiscard]] std::shared_ptr<StatusDecisionEngine> get_decision_engine() {
    return decision_engine_;
  }

  /**
   * @brief Register component with comprehensive monitoring
   */
  void register_component(Workflow::ComponentType type, const std::string& name,
                          std::function<Workflow::ComponentStatus()> health_check,
                          std::function<PerformanceMetrics()> performance_check = nullptr);

  /**
   * @brief Get system health summary
   */
  [[nodiscard]] SystemHealthSummary get_system_health() const;

  /**
   * @brief Check if system is operational
   */
  [[nodiscard]] bool is_system_operational() const;

  /**
   * @brief Generate comprehensive status report
   */
  [[nodiscard]] std::string generate_comprehensive_report() const;

 private:
  StatusManager() = default;
  ~StatusManager() {
    // Don't call shutdown in destructor to avoid race conditions
    // Just mark as not initialized to prevent further operations
    initialized_.store(false);
  }

  // Disable copy and move
  StatusManager(const StatusManager&) = delete;
  StatusManager& operator=(const StatusManager&) = delete;

  std::atomic<bool> initialized_{false};
  std::shared_ptr<RealTimeStatusMonitor> monitor_;
  std::shared_ptr<StatusDashboard> dashboard_;
  std::shared_ptr<StatusDecisionEngine> decision_engine_;

  void setup_default_components();
  void setup_default_decision_rules();
};

/**
 * @brief Utility functions for status management
 */
namespace Utils {

/**
 * @brief Convert system status to string
 */
[[nodiscard]] SOLAR_UTILS_API std::string to_string(SystemStatus status);

/**
 * @brief Convert alert severity to string
 */
[[nodiscard]] SOLAR_UTILS_API std::string to_string(AlertSeverity severity);

/**
 * @brief Convert alert type to string
 */
[[nodiscard]] SOLAR_UTILS_API std::string to_string(AlertType type);

/**
 * @brief Create performance metrics collector
 */
[[nodiscard]] SOLAR_UTILS_API std::function<PerformanceMetrics()> create_performance_collector(
    const std::string& component_name);

/**
 * @brief Create status alert
 */
[[nodiscard]] SOLAR_UTILS_API StatusAlert create_alert(AlertType type, AlertSeverity severity,
                                                       const std::string& title,
                                                       const std::string& description,
                                                       const std::string& component_name,
                                                       Workflow::ComponentType component_type);

/**
 * @brief Calculate component availability
 */
[[nodiscard]] SOLAR_UTILS_API double calculate_availability(
    const std::vector<Workflow::ComponentStatus>& status_history,
    std::chrono::hours window = std::chrono::hours(24));

}  // namespace Utils

/**
 * @brief Macros for convenient status management operations
 */
#define STATUS_REGISTER_COMPONENT(type, name, health_check) \
  SolarSystem::Utils::Status::StatusManager::instance().register_component(type, name, health_check)

#define STATUS_GET_SYSTEM_HEALTH() \
  SolarSystem::Utils::Status::StatusManager::instance().get_system_health()

#define STATUS_IS_OPERATIONAL() \
  SolarSystem::Utils::Status::StatusManager::instance().is_system_operational()

#define STATUS_CREATE_ALERT(type, severity, title, description, component, comp_type)            \
  SolarSystem::Utils::Status::Utils::create_alert(type, severity, title, description, component, \
                                                  comp_type)

}  // namespace SolarSystem::Utils::Status
