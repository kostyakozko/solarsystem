/**
 * @file status_management.cpp
 * @brief Implementation of comprehensive status management system
 */

#include "solar_utils/status_management.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

namespace SolarSystem::Utils::Status {

// RealTimeStatusMonitor Implementation
RealTimeStatusMonitor::RealTimeStatusMonitor() = default;

RealTimeStatusMonitor::~RealTimeStatusMonitor() {
  // Don't call stop_monitoring in destructor to avoid race conditions
  // Just mark as inactive and detach thread if it exists
  monitoring_active_.store(false);
  if (monitoring_thread_ && monitoring_thread_->joinable()) {
    try {
      // Give thread a moment to see the flag change
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      // Detach instead of join to avoid blocking
      monitoring_thread_->detach();
    } catch (...) {
      // Suppress all exceptions in destructor
    }
  }
}

void RealTimeStatusMonitor::start_monitoring() {
  if (monitoring_active_.load()) {
    return;
  }

  monitoring_active_.store(true);
  monitoring_thread_ = std::make_unique<std::thread>(&RealTimeStatusMonitor::monitoring_loop, this);

  LOG_INFO("RealTimeStatusMonitor", "Started real-time status monitoring");
}

void RealTimeStatusMonitor::stop_monitoring() {
  if (!monitoring_active_.load()) {
    return;
  }

  monitoring_active_.store(false);

  // Give the monitoring loop a chance to see the flag change
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  if (monitoring_thread_ && monitoring_thread_->joinable()) {
    try {
      // Use a timeout to avoid hanging
      monitoring_thread_->join();
    } catch (const std::exception& e) {
      LOG_WARN("RealTimeStatusMonitor", "Error joining monitoring thread: " + std::string(e.what()));
    }
  }
  monitoring_thread_.reset();

  LOG_INFO("RealTimeStatusMonitor", "Stopped real-time status monitoring");
}

void RealTimeStatusMonitor::register_component(
    Workflow::ComponentType type,
    const std::string& name,
    std::function<Workflow::ComponentStatus()> health_check,
    std::function<PerformanceMetrics()> performance_check) {

  std::lock_guard<std::mutex> lock(monitor_mutex_);

  std::string key = get_component_key(type, name);
  ComponentMonitorInfo info(type, name);
  info.health_check = health_check;
  info.performance_check = performance_check;

  // Perform initial health check immediately
  if (health_check) {
    try {
      auto initial_status = health_check();
      info.status.base_status = initial_status;
      LOG_INFO("RealTimeStatusMonitor", "Initial health check for " + name + ": " +
               Workflow::Utils::to_string(initial_status.health));
    } catch (const std::exception& e) {
      LOG_WARN("RealTimeStatusMonitor", "Initial health check failed for " + name + ": " + e.what());
      info.status.base_status.health = Workflow::ComponentHealth::Failed;
      info.status.base_status.status_message = "Initial health check failed: " + std::string(e.what());
    }
  }

  // Perform initial performance check if available
  if (performance_check) {
    try {
      auto initial_metrics = performance_check();
      info.status.update_performance(initial_metrics);
      LOG_INFO("RealTimeStatusMonitor", "Initial performance check completed for " + name);
    } catch (const std::exception& e) {
      LOG_WARN("RealTimeStatusMonitor", "Initial performance check failed for " + name + ": " + e.what());
    }
  }

  monitored_components_[key] = std::move(info);

  LOG_INFO("RealTimeStatusMonitor", "Registered component for enhanced monitoring: " + name);
}

void RealTimeStatusMonitor::unregister_component(Workflow::ComponentType type, const std::string& name) {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  std::string key = get_component_key(type, name);
  auto it = monitored_components_.find(key);
  if (it != monitored_components_.end()) {
    monitored_components_.erase(it);
    LOG_INFO("RealTimeStatusMonitor", "Unregistered component: " + name);
  }
}

std::optional<EnhancedComponentStatus> RealTimeStatusMonitor::get_component_status(
    Workflow::ComponentType type, const std::string& name) const {

  std::lock_guard<std::mutex> lock(monitor_mutex_);

  std::string key = get_component_key(type, name);
  auto it = monitored_components_.find(key);
  if (it != monitored_components_.end()) {
    return it->second.status;
  }

  return std::nullopt;
}

std::vector<EnhancedComponentStatus> RealTimeStatusMonitor::get_all_component_statuses() const {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  std::vector<EnhancedComponentStatus> statuses;
  statuses.reserve(monitored_components_.size());

  for (const auto& [key, info] : monitored_components_) {
    statuses.push_back(info.status);
  }

  return statuses;
}

SystemHealthSummary RealTimeStatusMonitor::get_system_health_summary() const {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  SystemHealthSummary summary;
  summary.last_updated = std::chrono::system_clock::now();

  std::vector<EnhancedComponentStatus> components;
  for (const auto& [key, info] : monitored_components_) {
    components.push_back(info.status);
  }

  summary.total_components = components.size();

  // Count components by health status
  for (const auto& component : components) {
    switch (component.base_status.health) {
      case Workflow::ComponentHealth::Healthy:
        summary.healthy_components++;
        break;
      case Workflow::ComponentHealth::Warning:
        summary.warning_components++;
        break;
      case Workflow::ComponentHealth::Critical:
        summary.critical_components++;
        break;
      case Workflow::ComponentHealth::Failed:
        summary.failed_components++;
        break;
      default:
        break;
    }

    // Count active alerts
    summary.active_alerts += component.active_alerts.size();
    for (const auto& alert : component.active_alerts) {
      if (alert.severity == AlertSeverity::Critical || alert.severity == AlertSeverity::Emergency) {
        summary.critical_alerts++;
      }
    }
  }

  // Calculate overall health score
  if (!components.empty()) {
    double total_score = 0.0;
    for (const auto& component : components) {
      total_score += component.get_comprehensive_health_score();
    }
    summary.overall_health_score = total_score / components.size();
  } else {
    summary.overall_health_score = 1.0;
  }

  // Determine overall system status
  summary.overall_status = calculate_system_status(components);

  // Generate top issues and recommendations
  if (summary.failed_components > 0) {
    summary.top_issues.push_back(std::to_string(summary.failed_components) + " components have failed");
    summary.recommendations.push_back("Investigate and restart failed components");
  }
  if (summary.critical_components > 0) {
    summary.top_issues.push_back(std::to_string(summary.critical_components) + " components are in critical state");
    summary.recommendations.push_back("Check component logs and resolve critical issues");
  }
  if (summary.critical_alerts > 0) {
    summary.top_issues.push_back(std::to_string(summary.critical_alerts) + " critical alerts active");
    summary.recommendations.push_back("Review and address critical alerts immediately");
  }

  return summary;
}

void RealTimeStatusMonitor::add_alert(const StatusAlert& alert) {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  // Add alert to global list
  alerts_.push_back(alert);

  // Add alert to specific component if applicable
  std::string key = get_component_key(alert.component_type, alert.component_name);
  auto it = monitored_components_.find(key);
  if (it != monitored_components_.end()) {
    it->second.status.add_alert(alert);
  }

  LOG_INFO("RealTimeStatusMonitor", "Added alert: " + alert.title + " for component: " + alert.component_name);
}

std::vector<StatusAlert> RealTimeStatusMonitor::get_active_alerts(AlertSeverity min_severity) const {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  std::vector<StatusAlert> active_alerts;
  // auto now = std::chrono::system_clock::now();  // Unused variable

  for (const auto& alert : alerts_) {
    if (alert.is_active() && alert.severity >= min_severity) {
      active_alerts.push_back(alert);
    }
  }

  // Sort by severity and timestamp
  std::sort(active_alerts.begin(), active_alerts.end(),
            [](const StatusAlert& a, const StatusAlert& b) {
              if (a.severity != b.severity) {
                return a.severity > b.severity;  // Higher severity first
              }
              return a.timestamp > b.timestamp;  // Newer first
            });

  return active_alerts;
}

void RealTimeStatusMonitor::acknowledge_alert(const std::string& alert_id, const std::string& user) {
  std::lock_guard<std::mutex> lock(monitor_mutex_);

  // Acknowledge in global alert list
  for (auto& alert : alerts_) {
    if (alert.id == alert_id) {
      alert.acknowledge(user);
      LOG_INFO("RealTimeStatusMonitor", "Alert acknowledged: " + alert_id + " by " + user);
      break;
    }
  }

  // Acknowledge in component-specific alerts
  for (auto& [key, info] : monitored_components_) {
    for (auto& alert : info.status.active_alerts) {
      if (alert.id == alert_id) {
        alert.acknowledge(user);
        break;
      }
    }
  }
}

void RealTimeStatusMonitor::set_monitoring_interval(std::chrono::seconds interval) {
  monitoring_interval_ = interval;
  LOG_INFO("RealTimeStatusMonitor", "Monitoring interval set to " + std::to_string(interval.count()) + " seconds");
}

void RealTimeStatusMonitor::monitoring_loop() {
  while (monitoring_active_.load()) {
    try {
      std::lock_guard<std::mutex> lock(monitor_mutex_);

      auto now = std::chrono::system_clock::now();

      for (auto& [key, info] : monitored_components_) {
        // Check health if interval has passed
        if (now - info.last_health_check >= info.status.check_interval) {
          check_component_health(info);
          info.last_health_check = now;
        }

        // Check performance if interval has passed and performance check is available
        if (info.performance_check &&
            now - info.last_performance_check >= info.status.check_interval) {
          check_component_performance(info);
          info.last_performance_check = now;
        }

        // Generate alerts based on status changes
        generate_health_alerts(info);
      }

      // Clean up expired alerts
      auto now_time = std::chrono::system_clock::now();
      alerts_.erase(
        std::remove_if(alerts_.begin(), alerts_.end(),
                       [now_time](const StatusAlert& alert) {
                         return now_time >= alert.expires_at;
                       }),
        alerts_.end());

    } catch (const std::exception& e) {
      LOG_ERROR("RealTimeStatusMonitor", "Error in monitoring loop: " + std::string(e.what()));
    }

    std::this_thread::sleep_for(monitoring_interval_);
  }
}

void RealTimeStatusMonitor::check_component_health(ComponentMonitorInfo& info) {
  try {
    if (info.health_check) {
      auto new_status = info.health_check();

      // Update base status
      auto old_health = info.status.base_status.health;
      info.status.base_status = new_status;

      // Log health changes
      if (old_health != new_status.health) {
        LOG_INFO("RealTimeStatusMonitor",
                 "Component " + info.status.base_status.name + " health changed from " +
                 Workflow::Utils::to_string(old_health) + " to " +
                 Workflow::Utils::to_string(new_status.health));
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("RealTimeStatusMonitor",
              "Error checking health for component " + info.status.base_status.name + ": " + e.what());

    info.status.base_status.health = Workflow::ComponentHealth::Failed;
    info.status.base_status.status_message = "Health check failed: " + std::string(e.what());
  }
}

void RealTimeStatusMonitor::check_component_performance(ComponentMonitorInfo& info) {
  try {
    if (info.performance_check) {
      auto new_metrics = info.performance_check();
      info.status.update_performance(new_metrics);

      // Log performance issues
      if (new_metrics.has_performance_issues()) {
        LOG_WARN("RealTimeStatusMonitor",
                 "Performance issues detected for component " + info.status.base_status.name);
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("RealTimeStatusMonitor",
              "Error checking performance for component " + info.status.base_status.name + ": " + e.what());
  }
}

void RealTimeStatusMonitor::generate_health_alerts(const ComponentMonitorInfo& info) {
  // Generate alerts based on component health and performance
  if (info.status.base_status.health == Workflow::ComponentHealth::Failed) {
    StatusAlert alert = Utils::create_alert(
      AlertType::ComponentHealth,
      AlertSeverity::Critical,
      "Component Failed",
      "Component " + info.status.base_status.name + " has failed: " + info.status.base_status.status_message,
      info.status.base_status.name,
      info.status.base_status.type
    );

    // Check if this alert already exists
    bool alert_exists = false;
    for (const auto& existing_alert : info.status.active_alerts) {
      if (existing_alert.type == alert.type &&
          existing_alert.component_name == alert.component_name &&
          existing_alert.is_active()) {
        alert_exists = true;
        break;
      }
    }

    if (!alert_exists) {
      const_cast<RealTimeStatusMonitor*>(this)->add_alert(alert);
    }
  }

  if (info.status.performance.has_performance_issues()) {
    StatusAlert alert = Utils::create_alert(
      AlertType::PerformanceIssue,
      AlertSeverity::Warning,
      "Performance Degradation",
      "Component " + info.status.base_status.name + " is experiencing performance issues",
      info.status.base_status.name,
      info.status.base_status.type
    );

    // Check if this alert already exists
    bool alert_exists = false;
    for (const auto& existing_alert : info.status.active_alerts) {
      if (existing_alert.type == alert.type &&
          existing_alert.component_name == alert.component_name &&
          existing_alert.is_active()) {
        alert_exists = true;
        break;
      }
    }

    if (!alert_exists) {
      const_cast<RealTimeStatusMonitor*>(this)->add_alert(alert);
    }
  }
}

std::string RealTimeStatusMonitor::get_component_key(Workflow::ComponentType type, const std::string& name) const {
  return Workflow::Utils::to_string(type) + "::" + name;
}

SystemStatus RealTimeStatusMonitor::calculate_system_status(const std::vector<EnhancedComponentStatus>& components) const {
  if (components.empty()) {
    return SystemStatus::Unknown;
  }

  size_t failed_count = 0;
  size_t critical_count = 0;
  size_t warning_count = 0;
  size_t healthy_count = 0;
  (void)healthy_count;  // Suppress unused variable warning

  for (const auto& component : components) {
    switch (component.base_status.health) {
      case Workflow::ComponentHealth::Failed:
        failed_count++;
        break;
      case Workflow::ComponentHealth::Critical:
        critical_count++;
        break;
      case Workflow::ComponentHealth::Warning:
        warning_count++;
        break;
      case Workflow::ComponentHealth::Healthy:
        healthy_count++;
        break;
      default:
        break;
    }
  }

  // Determine overall status based on component health distribution
  double total = components.size();
  double failed_ratio = failed_count / total;
  double critical_ratio = critical_count / total;
  double warning_ratio = warning_count / total;

  if (failed_ratio > 0.5) {
    return SystemStatus::Failed;
  } else if (failed_ratio > 0.2 || critical_ratio > 0.3) {
    return SystemStatus::Critical;
  } else if (failed_ratio > 0.0 || critical_ratio > 0.1 || warning_ratio > 0.3) {
    return SystemStatus::Degraded;
  } else if (warning_ratio > 0.0) {
    return SystemStatus::Healthy;
  } else {
    return SystemStatus::Optimal;
  }
}

// StatusDashboard Implementation
StatusDashboard::StatusDashboard(std::shared_ptr<RealTimeStatusMonitor> monitor)
    : monitor_(monitor) {}

void StatusDashboard::set_configuration(const DashboardConfig& config) {
  config_ = config;
}

void StatusDashboard::display_system_overview() const {
  auto summary = monitor_->get_system_health_summary();

  display_header();
  std::cout << "\n";

  display_system_health_bar(summary);
  std::cout << "\n";

  // System metrics
  std::cout << "📊 System Metrics:\n";
  std::cout << "  Total Components: " << summary.total_components << "\n";
  std::cout << "  Healthy: " << summary.healthy_components << " ✅\n";
  std::cout << "  Warning: " << summary.warning_components << " ⚠️\n";
  std::cout << "  Critical: " << summary.critical_components << " 🔴\n";
  std::cout << "  Failed: " << summary.failed_components << " ❌\n";
  std::cout << "  Availability: " << std::fixed << std::setprecision(1)
            << summary.get_availability_percentage() << "%\n";
  std::cout << "  Health Score: " << std::fixed << std::setprecision(2)
            << summary.overall_health_score << "/1.0\n";

  // Active alerts summary
  if (summary.active_alerts > 0) {
    std::cout << "\n🚨 Active Alerts: " << summary.active_alerts;
    if (summary.critical_alerts > 0) {
      std::cout << " (🔴 " << summary.critical_alerts << " critical)";
    }
    std::cout << "\n";
  }

  // Top issues
  if (!summary.top_issues.empty()) {
    std::cout << "\n⚠️  Top Issues:\n";
    for (const auto& issue : summary.top_issues) {
      std::cout << "  • " << issue << "\n";
    }
  }

  // Recommendations
  if (!summary.recommendations.empty()) {
    std::cout << "\n💡 Recommendations:\n";
    for (const auto& recommendation : summary.recommendations) {
      std::cout << "  • " << recommendation << "\n";
    }
  }

  std::cout << "\n";
}

void StatusDashboard::display_component_details() const {
  auto components = monitor_->get_all_component_statuses();

  if (components.empty()) {
    std::cout << "No components registered for monitoring.\n";
    return;
  }

  std::cout << "🔧 Component Details:\n\n";
  display_component_table(components);
}

void StatusDashboard::display_active_alerts() const {
  auto alerts = monitor_->get_active_alerts(config_.min_alert_severity);

  if (alerts.empty()) {
    std::cout << "✅ No active alerts.\n";
    return;
  }

  std::cout << "🚨 Active Alerts:\n\n";
  display_alert_table(alerts);
}

void StatusDashboard::display_performance_metrics() const {
  if (!config_.show_performance_metrics) {
    return;
  }

  auto components = monitor_->get_all_component_statuses();

  std::cout << "📈 Performance Metrics:\n\n";

  for (const auto& component : components) {
    const auto& metrics = component.performance;

    std::cout << "Component: " << component.base_status.name << "\n";
    std::cout << "  Response Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(metrics.response_time).count() << "ms\n";
    std::cout << "  CPU Usage: " << std::fixed << std::setprecision(1) << metrics.cpu_usage_percent << "%\n";
    std::cout << "  Memory Usage: " << (metrics.memory_usage_bytes / 1024 / 1024) << " MB\n";
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1) << (metrics.get_success_rate() * 100) << "%\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(2) << metrics.throughput_operations_per_second << " ops/sec\n";

    if (metrics.has_performance_issues()) {
      std::cout << "  ⚠️  Performance Issues Detected\n";
    }

    std::cout << "\n";
  }
}

std::string StatusDashboard::generate_status_report() const {
  std::ostringstream report;

  auto summary = monitor_->get_system_health_summary();
  auto components = monitor_->get_all_component_statuses();
  auto alerts = monitor_->get_active_alerts();

  report << "Solar System Suite - Status Report\n";
  auto time_t_val = std::chrono::system_clock::to_time_t(summary.last_updated);
  report << "Generated: " << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d %H:%M:%S") << "\n\n";

  report << "System Overview:\n";
  report << "  Status: " << Utils::to_string(summary.overall_status) << "\n";
  report << "  Health Score: " << std::fixed << std::setprecision(2) << summary.overall_health_score << "/1.0\n";
  report << "  Availability: " << std::fixed << std::setprecision(1) << summary.get_availability_percentage() << "%\n";
  report << "  Components: " << summary.total_components << " total, "
         << summary.healthy_components << " healthy, "
         << summary.warning_components << " warning, "
         << summary.critical_components << " critical, "
         << summary.failed_components << " failed\n";
  report << "  Active Alerts: " << summary.active_alerts << " (" << summary.critical_alerts << " critical)\n\n";

  if (!components.empty()) {
    report << "Component Status:\n";
    for (const auto& component : components) {
      report << "  " << component.base_status.name << " (" << Workflow::Utils::to_string(component.base_status.type) << "): "
             << format_health_status(component.base_status.health);
      if (!component.base_status.status_message.empty()) {
        report << " - " << component.base_status.status_message;
      }
      report << "\n";
    }
    report << "\n";
  }

  if (!alerts.empty()) {
    report << "Active Alerts:\n";
    for (const auto& alert : alerts) {
      report << "  [" << format_alert_severity(alert.severity) << "] "
             << alert.title << " (" << alert.component_name << ")\n";
      report << "    " << alert.description << "\n";
      report << "    Created: " << format_duration(alert.timestamp) << " ago\n";
    }
    report << "\n";
  }

  return report.str();
}

std::string StatusDashboard::generate_json_report() const {
  // Simplified JSON generation - in production would use a proper JSON library
  std::ostringstream json;

  auto summary = monitor_->get_system_health_summary();
  auto components = monitor_->get_all_component_statuses();
  auto alerts = monitor_->get_active_alerts();

  json << "{\n";
  json << "  \"timestamp\": \"" << std::chrono::duration_cast<std::chrono::seconds>(summary.last_updated.time_since_epoch()).count() << "\",\n";
  json << "  \"system_status\": \"" << Utils::to_string(summary.overall_status) << "\",\n";
  json << "  \"health_score\": " << summary.overall_health_score << ",\n";
  json << "  \"availability_percentage\": " << summary.get_availability_percentage() << ",\n";
  json << "  \"components\": {\n";
  json << "    \"total\": " << summary.total_components << ",\n";
  json << "    \"healthy\": " << summary.healthy_components << ",\n";
  json << "    \"warning\": " << summary.warning_components << ",\n";
  json << "    \"critical\": " << summary.critical_components << ",\n";
  json << "    \"failed\": " << summary.failed_components << "\n";
  json << "  },\n";
  json << "  \"alerts\": {\n";
  json << "    \"total\": " << summary.active_alerts << ",\n";
  json << "    \"critical\": " << summary.critical_alerts << "\n";
  json << "  }\n";
  json << "}\n";

  return json.str();
}

void StatusDashboard::start_interactive_dashboard() {
  if (dashboard_active_.load()) {
    return;
  }

  dashboard_active_.store(true);
  dashboard_thread_ = std::make_unique<std::thread>(&StatusDashboard::interactive_dashboard_loop, this);

  LOG_INFO("StatusDashboard", "Started interactive dashboard");
}

void StatusDashboard::stop_interactive_dashboard() {
  if (!dashboard_active_.load()) {
    return;
  }

  dashboard_active_.store(false);

  // Give the dashboard loop a chance to see the flag change
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  if (dashboard_thread_ && dashboard_thread_->joinable()) {
    try {
      dashboard_thread_->join();
    } catch (const std::exception& e) {
      LOG_WARN("StatusDashboard", "Error joining dashboard thread: " + std::string(e.what()));
    }
  }
  dashboard_thread_.reset();

  LOG_INFO("StatusDashboard", "Stopped interactive dashboard");
}

void StatusDashboard::interactive_dashboard_loop() {
  while (dashboard_active_.load()) {
    // Clear screen (simplified)
    std::cout << "\033[2J\033[H";

    display_system_overview();

    if (config_.show_component_details) {
      display_component_details();
    }

    if (config_.show_alerts) {
      display_active_alerts();
    }

    if (config_.show_performance_metrics) {
      display_performance_metrics();
    }

    std::cout << "Press Ctrl+C to exit dashboard\n";
    std::cout << "Refreshing in " << config_.refresh_interval.count() << " seconds...\n";

    std::this_thread::sleep_for(config_.refresh_interval);
  }
}

void StatusDashboard::display_header() const {
  std::cout << "+============================================================+\n";
  std::cout << "|              Solar System Suite Status Dashboard           |\n";
  std::cout << "|                Real-time System Monitoring                |\n";
  std::cout << "+============================================================+\n";
}

void StatusDashboard::display_system_health_bar(const SystemHealthSummary& summary) const {
  std::cout << "🏥 System Health: ";

  switch (summary.overall_status) {
    case SystemStatus::Optimal:
      std::cout << "🟢 OPTIMAL";
      break;
    case SystemStatus::Healthy:
      std::cout << "🟡 HEALTHY";
      break;
    case SystemStatus::Degraded:
      std::cout << "🟠 DEGRADED";
      break;
    case SystemStatus::Critical:
      std::cout << "🔴 CRITICAL";
      break;
    case SystemStatus::Failed:
      std::cout << "❌ FAILED";
      break;
    default:
      std::cout << "❓ UNKNOWN";
      break;
  }

  std::cout << " (Score: " << std::fixed << std::setprecision(2) << summary.overall_health_score << "/1.0)\n";
}

void StatusDashboard::display_component_table(const std::vector<EnhancedComponentStatus>& components) const {
  if (components.empty()) {
    return;
  }

  // Table header
  std::cout << std::left << std::setw(20) << "Component"
            << std::setw(15) << "Type"
            << std::setw(12) << "Health"
            << std::setw(8) << "Score"
            << std::setw(8) << "Alerts"
            << "Status Message\n";
  std::cout << std::string(80, '-') << "\n";

  // Table rows
  size_t displayed = 0;
  for (const auto& component : components) {
    if (displayed >= config_.max_components_displayed) {
      std::cout << "... and " << (components.size() - displayed) << " more components\n";
      break;
    }

    std::cout << std::left << std::setw(20) << component.base_status.name.substr(0, 19)
              << std::setw(15) << Workflow::Utils::to_string(component.base_status.type).substr(0, 14)
              << std::setw(12) << format_health_status(component.base_status.health)
              << std::setw(8) << std::fixed << std::setprecision(2) << component.get_comprehensive_health_score()
              << std::setw(8) << component.active_alerts.size()
              << component.base_status.status_message.substr(0, 30) << "\n";

    displayed++;
  }
  std::cout << "\n";
}

void StatusDashboard::display_alert_table(const std::vector<StatusAlert>& alerts) const {
  if (alerts.empty()) {
    return;
  }

  // Table header
  std::cout << std::left << std::setw(10) << "Severity"
            << std::setw(20) << "Component"
            << std::setw(25) << "Title"
            << std::setw(12) << "Age"
            << "Description\n";
  std::cout << std::string(80, '-') << "\n";

  // Table rows
  size_t displayed = 0;
  for (const auto& alert : alerts) {
    if (displayed >= config_.max_alerts_displayed) {
      std::cout << "... and " << (alerts.size() - displayed) << " more alerts\n";
      break;
    }

    std::cout << std::left << std::setw(10) << format_alert_severity(alert.severity)
              << std::setw(20) << alert.component_name.substr(0, 19)
              << std::setw(25) << alert.title.substr(0, 24)
              << std::setw(12) << format_duration(alert.timestamp)
              << alert.description.substr(0, 30) << "\n";

    displayed++;
  }
  std::cout << "\n";
}

std::string StatusDashboard::format_health_status(Workflow::ComponentHealth health) const {
  switch (health) {
    case Workflow::ComponentHealth::Healthy: return "✅ Healthy";
    case Workflow::ComponentHealth::Warning: return "⚠️  Warning";
    case Workflow::ComponentHealth::Critical: return "🔴 Critical";
    case Workflow::ComponentHealth::Failed: return "❌ Failed";
    default: return "❓ Unknown";
  }
}

std::string StatusDashboard::format_alert_severity(AlertSeverity severity) const {
  switch (severity) {
    case AlertSeverity::Info: return "ℹ️  Info";
    case AlertSeverity::Warning: return "⚠️  Warning";
    case AlertSeverity::Error: return "🔴 Error";
    case AlertSeverity::Critical: return "🚨 Critical";
    case AlertSeverity::Emergency: return "🆘 Emergency";
    default: return "❓ Unknown";
  }
}

std::string StatusDashboard::format_duration(std::chrono::system_clock::time_point start) const {
  auto now = std::chrono::system_clock::now();
  auto duration = now - start;

  auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
  auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));

  if (hours.count() > 0) {
    return std::to_string(hours.count()) + "h " + std::to_string(minutes.count()) + "m";
  } else if (minutes.count() > 0) {
    return std::to_string(minutes.count()) + "m " + std::to_string(seconds.count()) + "s";
  } else {
    return std::to_string(seconds.count()) + "s";
  }
}

// StatusDecisionEngine Implementation
StatusDecisionEngine::StatusDecisionEngine(std::shared_ptr<RealTimeStatusMonitor> monitor)
    : monitor_(monitor) {
  setup_default_rules();
}

void StatusDecisionEngine::add_decision_rule(const StatusDecisionRule& rule) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  decision_rules_[rule.id] = rule;
  LOG_INFO("StatusDecisionEngine", "Added decision rule: " + rule.name);
}

void StatusDecisionEngine::remove_decision_rule(const std::string& rule_id) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  auto it = decision_rules_.find(rule_id);
  if (it != decision_rules_.end()) {
    decision_rules_.erase(it);
    LOG_INFO("StatusDecisionEngine", "Removed decision rule: " + rule_id);
  }
}

void StatusDecisionEngine::set_rule_enabled(const std::string& rule_id, bool enabled) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  auto it = decision_rules_.find(rule_id);
  if (it != decision_rules_.end()) {
    it->second.enabled = enabled;
    LOG_INFO("StatusDecisionEngine", "Rule " + rule_id + (enabled ? " enabled" : " disabled"));
  }
}

void StatusDecisionEngine::start_engine() {
  if (engine_active_.load()) {
    return;
  }

  engine_active_.store(true);
  engine_thread_ = std::make_unique<std::thread>(&StatusDecisionEngine::decision_engine_loop, this);

  LOG_INFO("StatusDecisionEngine", "Started status decision engine");
}

void StatusDecisionEngine::stop_engine() {
  if (!engine_active_.load()) {
    return;
  }

  engine_active_.store(false);

  // Give the engine loop a chance to see the flag change
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  if (engine_thread_ && engine_thread_->joinable()) {
    try {
      engine_thread_->join();
    } catch (const std::exception& e) {
      LOG_WARN("StatusDecisionEngine", "Error joining engine thread: " + std::string(e.what()));
    }
  }
  engine_thread_.reset();

  LOG_INFO("StatusDecisionEngine", "Stopped status decision engine");
}

void StatusDecisionEngine::evaluate_rules() {
  std::lock_guard<std::mutex> lock(rules_mutex_);

  auto summary = monitor_->get_system_health_summary();
  auto components = monitor_->get_all_component_statuses();

  for (auto& [rule_id, rule] : decision_rules_) {
    evaluate_rule(rule);
  }
}

std::vector<StatusDecisionRule> StatusDecisionEngine::get_rule_statistics() const {
  std::lock_guard<std::mutex> lock(rules_mutex_);

  std::vector<StatusDecisionRule> rules;
  for (const auto& [rule_id, rule] : decision_rules_) {
    rules.push_back(rule);
  }

  return rules;
}

void StatusDecisionEngine::decision_engine_loop() {
  while (engine_active_.load()) {
    try {
      evaluate_rules();
    } catch (const std::exception& e) {
      LOG_ERROR("StatusDecisionEngine", "Error in decision engine loop: " + std::string(e.what()));
    }

    std::this_thread::sleep_for(evaluation_interval_);
  }
}

void StatusDecisionEngine::evaluate_rule(StatusDecisionRule& rule) {
  if (!rule.can_trigger()) {
    return;
  }

  try {
    auto summary = monitor_->get_system_health_summary();
    auto components = monitor_->get_all_component_statuses();

    if (rule.condition && rule.condition(summary, components)) {
      LOG_INFO("StatusDecisionEngine", "Triggering rule: " + rule.name);

      if (rule.action) {
        rule.action(summary, components);
      }

      rule.mark_triggered();
    }
  } catch (const std::exception& e) {
    LOG_ERROR("StatusDecisionEngine", "Error evaluating rule " + rule.id + ": " + e.what());
  }
}

void StatusDecisionEngine::setup_default_rules() {
  // Rule: Alert on system critical status
  StatusDecisionRule critical_system_rule("critical_system", "Critical System Status Alert");
  critical_system_rule.description = "Alert when system status becomes critical";
  critical_system_rule.condition = [](const SystemHealthSummary& summary, const std::vector<EnhancedComponentStatus>&) {
    return summary.overall_status == SystemStatus::Critical || summary.overall_status == SystemStatus::Failed;
  };
  critical_system_rule.action = [this](const SystemHealthSummary& summary, const std::vector<EnhancedComponentStatus>&) {
    StatusAlert alert = Utils::create_alert(
      AlertType::ComponentHealth,
      AlertSeverity::Critical,
      "System Status Critical",
      "System status is " + Utils::to_string(summary.overall_status) + " - immediate attention required",
      "System",
      Workflow::ComponentType::Launcher
    );
    monitor_->add_alert(alert);
  };
  add_decision_rule(critical_system_rule);

  // Rule: Alert on multiple component failures
  StatusDecisionRule multiple_failures_rule("multiple_failures", "Multiple Component Failures");
  multiple_failures_rule.description = "Alert when multiple components fail simultaneously";
  multiple_failures_rule.condition = [](const SystemHealthSummary& summary, const std::vector<EnhancedComponentStatus>&) {
    return summary.failed_components >= 2;
  };
  multiple_failures_rule.action = [this](const SystemHealthSummary& summary, const std::vector<EnhancedComponentStatus>&) {
    StatusAlert alert = Utils::create_alert(
      AlertType::ComponentHealth,
      AlertSeverity::Emergency,
      "Multiple Component Failures",
      std::to_string(summary.failed_components) + " components have failed - system stability at risk",
      "System",
      Workflow::ComponentType::Launcher
    );
    monitor_->add_alert(alert);
  };
  add_decision_rule(multiple_failures_rule);

  // Rule: Performance degradation alert
  StatusDecisionRule performance_rule("performance_degradation", "Performance Degradation Detection");
  performance_rule.description = "Alert when system performance degrades significantly";
  performance_rule.condition = [](const SystemHealthSummary& /* summary */, const std::vector<EnhancedComponentStatus>& components) {
    size_t performance_issues = 0;
    for (const auto& component : components) {
      if (component.performance.has_performance_issues()) {
        performance_issues++;
      }
    }
    return performance_issues > components.size() / 2;  // More than half have issues
  };
  performance_rule.action = [this](const SystemHealthSummary&, const std::vector<EnhancedComponentStatus>& components) {
    size_t affected_count = 0;
    for (const auto& component : components) {
      if (component.performance.has_performance_issues()) {
        affected_count++;
      }
    }

    StatusAlert alert = Utils::create_alert(
      AlertType::PerformanceIssue,
      AlertSeverity::Warning,
      "System Performance Degradation",
      std::to_string(affected_count) + " components experiencing performance issues",
      "System",
      Workflow::ComponentType::Launcher
    );
    monitor_->add_alert(alert);
  };
  add_decision_rule(performance_rule);
}

// StatusManager Implementation
StatusManager& StatusManager::instance() {
  static StatusManager instance;
  return instance;
}

void StatusManager::initialize() {
  if (initialized_.load()) {
    return;
  }

  monitor_ = std::make_shared<RealTimeStatusMonitor>();
  dashboard_ = std::make_shared<StatusDashboard>(monitor_);
  decision_engine_ = std::make_shared<StatusDecisionEngine>(monitor_);

  // Set default dashboard configuration
  DashboardConfig config;
  dashboard_->set_configuration(config);

  // Start monitoring and decision engine
  monitor_->start_monitoring();
  decision_engine_->start_engine();

  // Setup default components and rules
  setup_default_components();
  setup_default_decision_rules();

  initialized_.store(true);

  LOG_INFO("StatusManager", "Status management system initialized");
}

void StatusManager::shutdown() {
  if (!initialized_.load()) {
    LOG_INFO("StatusManager", "Shutdown called but not initialized");
    return;
  }

  // Prevent double shutdown
  bool expected = true;
  if (!initialized_.compare_exchange_strong(expected, false)) {
    LOG_INFO("StatusManager", "Shutdown already in progress or completed");
    return;
  }

  LOG_INFO("StatusManager", "Starting status management system shutdown");

  // Stop components in reverse order of initialization
  try {
    if (decision_engine_) {
      LOG_INFO("StatusManager", "Stopping decision engine...");
      decision_engine_->stop_engine();
      LOG_INFO("StatusManager", "Decision engine stopped, resetting pointer");
      decision_engine_.reset();
      LOG_INFO("StatusManager", "Decision engine pointer reset");
    }
  } catch (const std::exception& e) {
    LOG_ERROR("StatusManager", "Error stopping decision engine: " + std::string(e.what()));
  }

  try {
    if (dashboard_) {
      LOG_INFO("StatusManager", "Stopping dashboard...");
      dashboard_->stop_interactive_dashboard();
      LOG_INFO("StatusManager", "Dashboard stopped, resetting pointer");
      dashboard_.reset();
      LOG_INFO("StatusManager", "Dashboard pointer reset");
    }
  } catch (const std::exception& e) {
    LOG_ERROR("StatusManager", "Error stopping dashboard: " + std::string(e.what()));
  }

  try {
    if (monitor_) {
      LOG_INFO("StatusManager", "Stopping monitor...");
      monitor_->stop_monitoring();
      LOG_INFO("StatusManager", "Monitor stopped, resetting pointer");
      monitor_.reset();
      LOG_INFO("StatusManager", "Monitor pointer reset");
    }
  } catch (const std::exception& e) {
    LOG_ERROR("StatusManager", "Error stopping monitor: " + std::string(e.what()));
  }

  // Give threads time to finish
  LOG_INFO("StatusManager", "Waiting for threads to finish...");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  LOG_INFO("StatusManager", "Status management system shutdown completed");
}

void StatusManager::register_component(
    Workflow::ComponentType type,
    const std::string& name,
    std::function<Workflow::ComponentStatus()> health_check,
    std::function<PerformanceMetrics()> performance_check) {

  if (monitor_) {
    monitor_->register_component(type, name, health_check, performance_check);
  }
}

SystemHealthSummary StatusManager::get_system_health() const {
  if (monitor_) {
    return monitor_->get_system_health_summary();
  }
  return SystemHealthSummary{};
}

bool StatusManager::is_system_operational() const {
  auto health = get_system_health();
  return health.is_operational();
}

std::string StatusManager::generate_comprehensive_report() const {
  if (!initialized_.load()) {
    return "Status management system not initialized";
  }
  if (dashboard_ && monitor_) {
    try {
      return dashboard_->generate_status_report();
    } catch (const std::exception& e) {
      return "Error generating status report: " + std::string(e.what());
    } catch (...) {
      return "Unknown error generating status report";
    }
  }
  return "Status management system not available";
}

void StatusManager::setup_default_components() {
  // This will be called by applications to register their components
  // For now, we'll register a system component
  register_component(
    Workflow::ComponentType::Launcher,
    "StatusManager",
    []() {
      Workflow::ComponentStatus status(Workflow::ComponentType::Launcher, "StatusManager");
      status.health = Workflow::ComponentHealth::Healthy;
      status.status_message = "Status management system operational";
      status.health_score = 1.0;
      return status;
    },
    []() {
      PerformanceMetrics metrics;
      metrics.response_time = std::chrono::milliseconds(1);
      metrics.cpu_usage_percent = 5.0;
      metrics.memory_usage_bytes = 1024 * 1024;  // 1MB
      metrics.success_count = 100;
      metrics.error_count = 0;
      return metrics;
    }
  );
}

void StatusManager::setup_default_decision_rules() {
  // Default rules are already set up in StatusDecisionEngine constructor
}

// Utility Functions
namespace Utils {

std::string to_string(SystemStatus status) {
  switch (status) {
    case SystemStatus::Optimal: return "Optimal";
    case SystemStatus::Healthy: return "Healthy";
    case SystemStatus::Degraded: return "Degraded";
    case SystemStatus::Critical: return "Critical";
    case SystemStatus::Failed: return "Failed";
    case SystemStatus::Unknown: return "Unknown";
    default: return "Unknown";
  }
}

std::string to_string(AlertSeverity severity) {
  switch (severity) {
    case AlertSeverity::Info: return "Info";
    case AlertSeverity::Warning: return "Warning";
    case AlertSeverity::Error: return "Error";
    case AlertSeverity::Critical: return "Critical";
    case AlertSeverity::Emergency: return "Emergency";
    default: return "Unknown";
  }
}

std::string to_string(AlertType type) {
  switch (type) {
    case AlertType::ComponentHealth: return "ComponentHealth";
    case AlertType::PerformanceIssue: return "PerformanceIssue";
    case AlertType::ConnectivityIssue: return "ConnectivityIssue";
    case AlertType::ResourceExhaustion: return "ResourceExhaustion";
    case AlertType::SecurityEvent: return "SecurityEvent";
    case AlertType::ConfigurationIssue: return "ConfigurationIssue";
    case AlertType::DataIntegrity: return "DataIntegrity";
    case AlertType::WorkflowFailure: return "WorkflowFailure";
    default: return "Unknown";
  }
}

std::function<PerformanceMetrics()> create_performance_collector(const std::string& component_name) {
  return [component_name]() {
    PerformanceMetrics metrics;

    // Simulate performance data collection
    // In a real implementation, this would collect actual system metrics
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> cpu_dist(0.0, 100.0);
    static std::uniform_int_distribution<> memory_dist(1024*1024, 100*1024*1024);  // 1MB to 100MB
    static std::uniform_int_distribution<> response_dist(1, 1000);  // 1ms to 1s

    metrics.cpu_usage_percent = cpu_dist(gen);
    metrics.memory_usage_bytes = static_cast<size_t>(memory_dist(gen));
    metrics.response_time = std::chrono::milliseconds(response_dist(gen));
    metrics.success_count = 95 + (gen() % 5);  // 95-99 successes
    metrics.error_count = gen() % 3;  // 0-2 errors
    metrics.throughput_operations_per_second = 10.0 + (gen() % 90);  // 10-100 ops/sec

    return metrics;
  };
}

StatusAlert create_alert(
    AlertType type,
    AlertSeverity severity,
    const std::string& title,
    const std::string& description,
    const std::string& component_name,
    Workflow::ComponentType component_type) {

  StatusAlert alert;

  // Generate unique ID
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(100000, 999999);
  alert.id = "alert_" + std::to_string(dis(gen));

  alert.type = type;
  alert.severity = severity;
  alert.title = title;
  alert.description = description;
  alert.component_name = component_name;
  alert.component_type = component_type;

  // Add suggested actions based on alert type
  switch (type) {
    case AlertType::ComponentHealth:
      alert.suggested_actions.push_back("Check component logs for errors");
      alert.suggested_actions.push_back("Restart the component if safe to do so");
      alert.suggested_actions.push_back("Verify component configuration");
      break;
    case AlertType::PerformanceIssue:
      alert.suggested_actions.push_back("Monitor resource usage");
      alert.suggested_actions.push_back("Check for resource bottlenecks");
      alert.suggested_actions.push_back("Consider scaling or optimization");
      break;
    case AlertType::ConnectivityIssue:
      alert.suggested_actions.push_back("Check network connectivity");
      alert.suggested_actions.push_back("Verify service endpoints");
      alert.suggested_actions.push_back("Test with fallback connections");
      break;
    default:
      alert.suggested_actions.push_back("Review component status and logs");
      break;
  }

  return alert;
}

double calculate_availability(
    const std::vector<Workflow::ComponentStatus>& status_history,
    std::chrono::hours window) {

  if (status_history.empty()) {
    return 100.0;
  }

  auto now = std::chrono::system_clock::now();
  auto window_start = now - window;

  size_t total_checks = 0;
  size_t healthy_checks = 0;

  for (const auto& status : status_history) {
    if (status.last_check >= window_start) {
      total_checks++;
      if (status.health == Workflow::ComponentHealth::Healthy ||
          status.health == Workflow::ComponentHealth::Warning) {
        healthy_checks++;
      }
    }
  }

  return total_checks > 0 ? (static_cast<double>(healthy_checks) / total_checks * 100.0) : 100.0;
}

}  // namespace Utils

}  // namespace SolarSystem::Utils::Status
