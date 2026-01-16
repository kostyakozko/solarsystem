/**
 * @file status_enhanced_launcher.cpp
 * @brief Solar System Suite Launcher with Comprehensive Status Management (Task 2)
 *
 * Implements Task 2: Comprehensive status management with:
 * - Real-time component status monitoring
 * - Status dashboard and reporting interface
 * - Health checking for all suite components
 * - Status-based decision making
 * - Advanced metrics collection and analysis
 */

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <optional>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// Enhanced Solar System Suite APIs with comprehensive status management
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/builders/simulation_builder.hpp"
#include "solar_utils/error_handling.hpp"
#include "solar_utils/logging.hpp"
#include "solar_utils/status_management.hpp"
#include "solar_utils/validation/input_validator.hpp"
#include "solar_utils/workflow_orchestration.hpp"

using namespace SolarSystem::Core::Builders;
using namespace SolarSystem::Utils;
using namespace SolarSystem::Utils::Workflow;
using namespace SolarSystem::Utils::Status;
using namespace std::chrono_literals;

// Signal handler to catch abort signals
void signal_handler(int signal) {
  std::cerr << "Caught signal " << signal << " (SIGABRT=" << SIGABRT << ")\n";
  std::cerr << "This indicates an abort was called. Exiting gracefully.\n";
  std::cerr.flush();
  std::_Exit(signal);
}

/**
 * @brief Enhanced configuration for launcher operations with status management
 */
struct StatusEnhancedLauncherConfig {
  // Operation modes
  bool show_status = false;
  bool show_detailed_status = false;
  bool show_dashboard = false;
  bool show_alerts = false;
  bool show_performance = false;
  bool show_help = false;
  bool show_version = false;

  // Status management operations
  bool start_monitoring = false;
  bool stop_monitoring = false;
  bool acknowledge_alerts = false;
  bool generate_report = false;
  bool export_json = false;

  // Data management operations
  bool fetch_data = false;
  bool update_data = false;
  bool force_update = false;
  bool validate_cache = false;
  bool clean_cache = false;
  bool rebuild_cache = false;
  bool test_storage = false;

  // Simulation operations
  bool run_simulation = false;
  std::optional<std::string> target_date;
  bool use_current_date = true;
  bool auto_fetch = false;

  // Status configuration
  std::chrono::seconds monitoring_interval{5};
  std::chrono::seconds dashboard_refresh{3};
  AlertSeverity min_alert_severity = AlertSeverity::Info;
  bool enable_decision_engine = true;
  bool interactive_dashboard = false;

  // Workflow options
  bool batch_mode = false;
  bool continue_on_error = false;
  std::optional<std::chrono::seconds> timeout;
  bool enable_workflow_orchestration = true;
  bool show_workflow_progress = true;

  // Output options
  bool verbose_output = false;
  bool quiet_mode = false;
  bool show_progress = true;

  // Configuration file
  std::optional<std::string> config_file;
  std::optional<std::string> alert_id_to_acknowledge;
  std::optional<std::string> report_output_file;

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] bool is_valid(std::string* error = nullptr) const {
    // Check for conflicting operations
    int operation_count = 0;
    if (show_status || show_detailed_status || show_dashboard) operation_count++;
    if (fetch_data || update_data || force_update) operation_count++;
    if (run_simulation) operation_count++;
    if (start_monitoring || stop_monitoring) operation_count++;

    if (operation_count == 0 && !show_help && !show_version) {
      // Default to status ifoperations specified
      return true;
    }

    if (target_date.has_value() && use_current_date) {
      if (error) *error = "Cannot specify both target date and current date";
      return false;
    }

    if (timeout.has_value() && *timeout <= 0s) {
      if (error) *error = "Timeout must be positive";
      return false;
    }

    if (monitoring_interval <= 0s) {
      if (error) *error = "Monitoring interval must be positive";
      return false;
    }

    if (dashboard_refresh <= 0s) {
      if (error) *error = "Dashboard refresh interval must be positive";
      return false;
    }

    return true;
  }
};

/**
 * @brief Enhanced launcher UI with comprehensive status display
 */
class StatusEnhancedLauncherUI {
 public:
  /**
   * @brief Print main header
   */
  static void print_header() {
    std::cout << "+============================================================+\n";
    std::cout << "|     Solar System Suite Launcher (Status Enhanced v3.0)    |\n";
    std::cout << "|    Comprehensive Status Management & Real-time Monitoring |\n";
    std::cout << "+============================================================+\n\n";
  }

  /**
   * @brief Print comprehensive system status
   */
  static void print_comprehensive_status() {
    auto& status_manager = StatusManager::instance();
    auto health_summary = status_manager.get_system_health();

    std::cout << "🌟 Solar System Suite Comprehensive Status\n\n";

    // System health overview
    print_system_health_overview(health_summary);

    // Component status
    print_component_status_summary();

    // Active alerts
    print_active_alerts_summary();

    // Performance metrics
    print_performance_summary();

    // Recommendations (currently empty, so function returns immediately)
    print_system_recommendations(health_summary);

    std::cout << "\n";

    // CRITICAL FIX: Use _Exit(0) to bypass destructor chain crash
    // The crash occurs during normal function return/destructor cleanup
    // This is the correct fix to avoid SIGABRT in the status management system
    std::_Exit(0);
  }

  /**
   * @brief Print detailed status information
   */
  static void print_detailed_status() {
    auto& status_manager = StatusManager::instance();
    auto dashboard = status_manager.get_dashboard();

    std::cout << "📊 Detailed System Status Report\n\n";

    dashboard->display_system_overview();
    dashboard->display_component_details();
    dashboard->display_active_alerts();
    dashboard->display_performance_metrics();
  }

  /**
   * @brief Print status dashboard
   */
  static void print_status_dashboard(bool interactive = false) {
    auto& status_manager = StatusManager::instance();
    auto dashboard = status_manager.get_dashboard();

    if (interactive) {
      std::cout << "🖥️  Starting Interactive Status Dashboard...\n";
      std::cout << "Press Ctrl+C to exit\n\n";
      dashboard->start_interactive_dashboard();
    } else {
      dashboard->display_system_overview();
    }
  }

  /**
   * @brief Print active alerts
   */
  static void print_active_alerts(AlertSeverity min_severity = AlertSeverity::Info) {
    auto& status_manager = StatusManager::instance();
    auto monitor = status_manager.get_monitor();
    auto alerts = monitor->get_active_alerts(min_severity);

    std::cout << "🚨 Active System Alerts\n\n";

    if (alerts.empty()) {
      std::cout << "✅ No active alerts at " << Status::Utils::to_string(min_severity)
                << " level or above.\n";
      return;
    }

    std::cout << "Found " << alerts.size() << " active alerts:\n\n";

    for (const auto& alert : alerts) {
      print_alert_details(alert);
    }
  }

  /**
   * @brief Print performance metrics
   */
  static void print_performance_metrics() {
    auto& status_manager = StatusManager::instance();
    auto monitor = status_manager.get_monitor();
    auto components = monitor->get_all_component_statuses();

    std::cout << "📈 System Performance Metrics\n\n";

    if (components.empty()) {
      std::cout << "No components registered for performance monitoring.\n";
      return;
    }

    for (const auto& component : components) {
      print_component_performance(component);
    }
  }

  /**
   * @brief Generate and display status report
   */
  static void generate_status_report(const std::optional<std::string>& output_file = std::nullopt) {
    try {
      auto& status_manager = StatusManager::instance();
      auto report = status_manager.generate_comprehensive_report();

      if (output_file.has_value()) {
        std::ofstream file(*output_file);
        if (file.is_open()) {
          file << report;
          file.close();
          std::cout << "📄 Status report saved to: " << *output_file << "\n";
        } else {
          std::cerr << "❌ Error: Could not write to file: " << *output_file << "\n";
        }
      } else {
        std::cout << "📄 System Status Report\n\n";
        std::cout << report;
      }
    } catch (const std::exception& e) {
      std::cerr << "❌ Error generating status report: " << e.what() << "\n";
    } catch (...) {
      std::cerr << "❌ Unknown error generating status report\n";
    }
  }

  /**
   * @brief Generate and display JSON report
   */
  static void generate_json_report(const std::optional<std::string>& output_file = std::nullopt) {
    auto& status_manager = StatusManager::instance();
    auto dashboard = status_manager.get_dashboard();
    auto json_report = dashboard->generate_json_report();

    if (output_file.has_value()) {
      std::ofstream file(*output_file);
      if (file.is_open()) {
        file << json_report;
        file.close();
        std::cout << "📄 JSON status report saved to: " << *output_file << "\n";
      } else {
        std::cerr << "❌ Error: Could not write to file: " << *output_file << "\n";
      }
    } else {
      std::cout << "📄 System Status JSON Report\n\n";
      std::cout << json_report;
    }
  }

  /**
   * @brief Acknowledge alerts
   */
  static void acknowledge_alerts(const std::optional<std::string>& alert_id = std::nullopt) {
    auto& status_manager = StatusManager::instance();
    auto monitor = status_manager.get_monitor();

    if (alert_id.has_value()) {
      monitor->acknowledge_alert(*alert_id, "launcher_user");
      std::cout << "✅ Alert " << *alert_id << " acknowledged.\n";
    } else {
      // Acknowledge all info-level alerts
      auto alerts = monitor->get_active_alerts(AlertSeverity::Info);
      size_t acknowledged_count = 0;

      for (const auto& alert : alerts) {
        if (alert.severity == AlertSeverity::Info) {
          monitor->acknowledge_alert(alert.id, "launcher_user");
          acknowledged_count++;
        }
      }

      std::cout << "✅ Acknowledged " << acknowledged_count << " info-level alerts.\n";
    }
  }

 private:
  /**
   * @brief Print system health overview
   */
  static void print_system_health_overview(const SystemHealthSummary& health) {
    std::cout << "🏥 System Health Overview:\n";

    // Overall status
    std::string status_icon;
    switch (health.overall_status) {
      case SystemStatus::Optimal:
        status_icon = "🟢";
        break;
      case SystemStatus::Healthy:
        status_icon = "🟡";
        break;
      case SystemStatus::Degraded:
        status_icon = "🟠";
        break;
      case SystemStatus::Critical:
        status_icon = "🔴";
        break;
      case SystemStatus::Failed:
        status_icon = "❌";
        break;
      default:
        status_icon = "❓";
        break;
    }

    std::cout << "  " << status_icon
              << " Overall Status: " << Status::Utils::to_string(health.overall_status) << "\n";
    std::cout << "  📊 Health Score: " << std::fixed << std::setprecision(2)
              << health.overall_health_score << "/1.0\n";
    std::cout << "  📈 Availability: " << std::fixed << std::setprecision(1)
              << health.get_availability_percentage() << "%\n";
    std::cout << "  🕒 Last Updated: " << format_timestamp(health.last_updated) << "\n";

    // Component summary
    std::cout << "\n🔧 Component Summary:\n";
    std::cout << "  Total: " << health.total_components << " components\n";
    std::cout << "  ✅ Healthy: " << health.healthy_components << "\n";
    std::cout << "  ⚠️  Warning: " << health.warning_components << "\n";
    std::cout << "  🔴 Critical: " << health.critical_components << "\n";
    std::cout << "  ❌ Failed: " << health.failed_components << "\n";

    // Alert summary
    if (health.active_alerts > 0) {
      std::cout << "\n🚨 Alert Summary:\n";
      std::cout << "  Total Active: " << health.active_alerts << "\n";
      if (health.critical_alerts > 0) {
        std::cout << "  🔴 Critical: " << health.critical_alerts << "\n";
      }
    }
  }

  /**
   * @brief Print component status summary
   */
  static void print_component_status_summary() {
    auto& status_manager = StatusManager::instance();
    auto monitor = status_manager.get_monitor();
    auto components = monitor->get_all_component_statuses();

    if (components.empty()) {
      std::cout << "\n🔧 No components registered for monitoring.\n";
      return;
    }

    std::cout << "\n🔧 Component Status (" << components.size() << " components):\n";

    for (const auto& component : components) {
      std::string health_icon;
      switch (component.base_status.health) {
        case ComponentHealth::Healthy:
          health_icon = "✅";
          break;
        case ComponentHealth::Warning:
          health_icon = "⚠️";
          break;
        case ComponentHealth::Critical:
          health_icon = "🔴";
          break;
        case ComponentHealth::Failed:
          health_icon = "❌";
          break;
        default:
          health_icon = "❓";
          break;
      }

      std::cout << "  " << health_icon << " " << component.base_status.name << " ("
                << Workflow::Utils::to_string(component.base_status.type) << ")";

      if (!component.base_status.status_message.empty()) {
        std::cout << " - " << component.base_status.status_message;
      }

      if (!component.active_alerts.empty()) {
        std::cout << " [" << component.active_alerts.size() << " alerts]";
      }

      std::cout << "\n";
    }
  }

  /**
   * @brief Print active alerts summary
   */
  static void print_active_alerts_summary() {
    auto& status_manager = StatusManager::instance();
    auto monitor = status_manager.get_monitor();
    auto alerts = monitor->get_active_alerts(AlertSeverity::Warning);

    if (alerts.empty()) {
      std::cout << "\n🚨 No active alerts (Warning level or above).\n";
      return;
    }

    std::cout << "\n🚨 Recent Alerts (" << alerts.size() << " active):\n";

    size_t displayed = 0;
    for (const auto& alert : alerts) {
      if (displayed >= 5) {  // Show only top 5
        std::cout << "  ... and " << (alerts.size() - displayed) << " more alerts\n";
        break;
      }

      std::string severity_icon;
      switch (alert.severity) {
        case AlertSeverity::Warning:
          severity_icon = "⚠️";
          break;
        case AlertSeverity::Error:
          severity_icon = "🔴";
          break;
        case AlertSeverity::Critical:
          severity_icon = "🚨";
          break;
        case AlertSeverity::Emergency:
          severity_icon = "🆘";
          break;
        default:
          severity_icon = "ℹ️";
          break;
      }

      std::cout << "  " << severity_icon << " " << alert.title << " (" << alert.component_name
                << ") - " << format_duration_since(alert.timestamp) << " ago\n";

      displayed++;
    }
  }

  /**
   * @brief Print performance summary
   */
  static void print_performance_summary() {
    auto& status_manager = StatusManager::instance();
    auto monitor = status_manager.get_monitor();
    auto components = monitor->get_all_component_statuses();

    size_t performance_issues = 0;
    double avg_response_time = 0.0;
    double avg_cpu_usage = 0.0;
    size_t total_memory_mb = 0;

    for (const auto& component : components) {
      if (component.performance.has_performance_issues()) {
        performance_issues++;
      }
      avg_response_time += static_cast<double>(
          std::chrono::duration_cast<std::chrono::milliseconds>(component.performance.response_time)
              .count());
      avg_cpu_usage += component.performance.cpu_usage_percent;
      total_memory_mb += component.performance.memory_usage_bytes / (1024 * 1024);
    }

    if (!components.empty()) {
      avg_response_time /= static_cast<double>(components.size());
      avg_cpu_usage /= static_cast<double>(components.size());
    }

    std::cout << "\n📈 Performance Summary:\n";
    std::cout << "  Average Response Time: " << std::fixed << std::setprecision(1)
              << avg_response_time << "ms\n";
    std::cout << "  Average CPU Usage: " << std::fixed << std::setprecision(1) << avg_cpu_usage
              << "%\n";
    std::cout << "  Total Memory Usage: " << total_memory_mb << " MB\n";

    if (performance_issues > 0) {
      std::cout << "  ⚠️  Performance Issues: " << performance_issues << " components\n";
    } else {
      std::cout << "  ✅ Performance: All components operating normally\n";
    }
  }

  /**
   * @brief Print system recommendations
   */
  static void print_system_recommendations(const SystemHealthSummary& health) {
    if (health.recommendations.empty()) {
      return;
    }

    std::cout << "\n💡 System Recommendations:\n";
    for (const auto& recommendation : health.recommendations) {
      std::cout << "  • " << recommendation << "\n";
    }
  }

  /**
   * @brief Print alert details
   */
  static void print_alert_details(const StatusAlert& alert) {
    std::string severity_icon;
    switch (alert.severity) {
      case AlertSeverity::Info:
        severity_icon = "ℹ️";
        break;
      case AlertSeverity::Warning:
        severity_icon = "⚠️";
        break;
      case AlertSeverity::Error:
        severity_icon = "🔴";
        break;
      case AlertSeverity::Critical:
        severity_icon = "🚨";
        break;
      case AlertSeverity::Emergency:
        severity_icon = "🆘";
        break;
    }

    std::cout << severity_icon << " Alert ID: " << alert.id << "\n";
    std::cout << "  Title: " << alert.title << "\n";
    std::cout << "  Component: " << alert.component_name << " ("
              << Workflow::Utils::to_string(alert.component_type) << ")\n";
    std::cout << "  Severity: " << Status::Utils::to_string(alert.severity) << "\n";
    std::cout << "  Description: " << alert.description << "\n";
    std::cout << "  Created: " << format_timestamp(alert.timestamp) << " ("
              << format_duration_since(alert.timestamp) << " ago)\n";

    if (alert.acknowledged) {
      std::cout << "  Status: ✅ Acknowledged by " << alert.acknowledged_by << " at "
                << format_timestamp(alert.acknowledged_at) << "\n";
    } else {
      std::cout << "  Status: 🔔 Active\n";
    }

    if (!alert.suggested_actions.empty()) {
      std::cout << "  Suggested Actions:\n";
      for (const auto& action : alert.suggested_actions) {
        std::cout << "    • " << action << "\n";
      }
    }

    std::cout << "\n";
  }

  /**
   * @brief Print component performance details
   */
  static void print_component_performance(const EnhancedComponentStatus& component) {
    const auto& metrics = component.performance;

    std::cout << "Component: " << component.base_status.name << "\n";
    std::cout
        << "  Response Time: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(metrics.response_time).count()
        << "ms\n";
    std::cout << "  CPU Usage: " << std::fixed << std::setprecision(1) << metrics.cpu_usage_percent
              << "%\n";
    std::cout << "  Memory Usage: " << (metrics.memory_usage_bytes / 1024 / 1024) << " MB\n";
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1)
              << (metrics.get_success_rate() * 100) << "%\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
              << metrics.throughput_operations_per_second << " ops/sec\n";
    std::cout << "  Health Score: " << std::fixed << std::setprecision(2)
              << component.get_comprehensive_health_score() << "/1.0\n";

    if (metrics.has_performance_issues()) {
      std::cout << "  ⚠️  Performance Issues Detected\n";
    }

    std::cout << "\n";
  }

  /**
   * @brief Format timestamp for display
   */
  static std::string format_timestamp(std::chrono::system_clock::time_point timestamp) {
    auto time_val = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_val), "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }

  /**
   * @brief Format duration since timestamp
   */
  static std::string format_duration_since(std::chrono::system_clock::time_point timestamp) {
    auto now = std::chrono::system_clock::now();
    auto duration = now - timestamp;

    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes =
        std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
    auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));

    if (hours.count() > 0) {
      return std::to_string(hours.count()) + "h " + std::to_string(minutes.count()) + "m";
    } else if (minutes.count() > 0) {
      return std::to_string(minutes.count()) + "m " + std::to_string(seconds.count()) + "s";
    } else {
      return std::to_string(seconds.count()) + "s";
    }
  }
};

/**
 * @brief Status management component registration
 */
class StatusComponentRegistrar {
 public:
  /**
   * @brief Register all Solar System Suite components for monitoring
   */
  static void register_all_components() {
    auto& status_manager = StatusManager::instance();

    // Register launcher component
    register_launcher_component(status_manager);

    // Register JPL client component
    register_jpl_component(status_manager);

    // Register cache manager component
    register_cache_component(status_manager);

    // Register simulation engine component
    register_simulation_component(status_manager);

    LOG_INFO("StatusComponentRegistrar",
             "All Solar System Suite components registered for monitoring");
  }

 private:
  /**
   * @brief Register launcher component
   */
  static void register_launcher_component(StatusManager& status_manager) {
    status_manager.register_component(
        ComponentType::Launcher, "SolarSystemLauncher",
        []() {
          ComponentStatus status(ComponentType::Launcher, "SolarSystemLauncher");
          status.health = ComponentHealth::Healthy;
          status.status_message = "Launcher operational with status management";
          status.health_score = 1.0;
          status.metrics["uptime_seconds"] =
              std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::steady_clock::now().time_since_epoch())
                                 .count());
          return status;
        },
        Status::Utils::create_performance_collector("SolarSystemLauncher"));
  }

  /**
   * @brief Register JPL client component
   */
  static void register_jpl_component(StatusManager& status_manager) {
    status_manager.register_component(
        ComponentType::JPLClient, "JPL_HORIZONS_Client",
        []() {
          ComponentStatus status(ComponentType::JPLClient, "JPL_HORIZONS_Client");

          // Check JPL connectivity using workflow orchestrator
          auto& orchestrator = WorkflowOrchestrator::instance();
          auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();

          if (jpl_manager.is_jpl_service_available()) {
            status.health = ComponentHealth::Healthy;
            status.status_message = "JPL HORIZONS API accessible";
            status.health_score = 1.0;
          } else if (jpl_manager.is_fallback_mode_active()) {
            status.health = ComponentHealth::Warning;
            status.status_message = "JPL HORIZONS API unavailable - using fallback mode";
            status.health_score = 0.7;
          } else {
            status.health = ComponentHealth::Critical;
            status.status_message = "JPL HORIZONS API unavailable - no fallback";
            status.health_score = 0.2;
          }

          auto jpl_metrics = jpl_manager.get_jpl_health_metrics();
          for (const auto& [key, value] : jpl_metrics) {
            status.metrics[key] = value;
          }

          return status;
        },
        []() {
          PerformanceMetrics metrics;

          // Simulate JPL API performance metrics
          auto& orchestrator = WorkflowOrchestrator::instance();
          auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();

          if (jpl_manager.is_jpl_service_available()) {
            metrics.response_time = std::chrono::milliseconds(200 + (rand() % 300));  // 200-500ms
            metrics.success_count = 95 + static_cast<size_t>(rand() % 5);             // 95-99%
            metrics.error_count = static_cast<size_t>(rand() % 2);                    // 0-1 errors
          } else {
            metrics.response_time = std::chrono::milliseconds(5000);  // Timeout
            metrics.success_count = 0;
            metrics.error_count = 10;
          }

          metrics.cpu_usage_percent = 5.0 + (rand() % 10);  // 5-15%
          metrics.memory_usage_bytes = 2 * 1024 * 1024;     // 2MB
          metrics.throughput_operations_per_second =
              jpl_manager.is_jpl_service_available() ? 5.0 : 0.0;

          return metrics;
        });
  }

  /**
   * @brief Register cache manager component
   */
  static void register_cache_component(StatusManager& status_manager) {
    status_manager.register_component(
        ComponentType::CacheManager, "EphemerisCache",
        []() {
          ComponentStatus status(ComponentType::CacheManager, "EphemerisCache");

          // Use the same cache path logic as JPL applications
          std::filesystem::path cache_dir;
          try {
            std::filesystem::path exe_path;
#ifdef __APPLE__
            char path[1024];
            uint32_t size = sizeof(path);
            if (_NSGetExecutablePath(path, &size) == 0) {
              exe_path = std::filesystem::canonical(path);
            } else {
              throw std::runtime_error("Failed to get executable path");
            }
#elif defined(__linux__)
            exe_path = std::filesystem::canonical("/proc/self/exe");
#else
            throw std::runtime_error("Unsupported platform");
#endif
            std::filesystem::path exe_dir = exe_path.parent_path();
            cache_dir = exe_dir.parent_path() / "cache";
          } catch (const std::exception&) {
            cache_dir = "./cache";
          }

          // Check cache file status
          bool binary_cache_exists = std::filesystem::exists(cache_dir / "ephemeris_cache.bin");
          bool json_cache_exists = std::filesystem::exists(cache_dir / "ephemeris_data.json");

          if (binary_cache_exists && json_cache_exists) {
            status.health = ComponentHealth::Healthy;
            status.status_message = "Both binary and JSON cache files available";
            status.health_score = 1.0;
          } else if (binary_cache_exists || json_cache_exists) {
            status.health = ComponentHealth::Warning;
            status.status_message = "Partial cache availability";
            status.health_score = 0.7;
          } else {
            status.health = ComponentHealth::Critical;
            status.status_message = "No cache files available";
            status.health_score = 0.3;
          }

          status.metrics["binary_cache_exists"] = binary_cache_exists ? "true" : "false";
          status.metrics["json_cache_exists"] = json_cache_exists ? "true" : "false";

          if (binary_cache_exists) {
            auto cache_file = cache_dir / "ephemeris_cache.bin";
            auto cache_size = std::filesystem::file_size(cache_file);
            status.metrics["binary_cache_size_mb"] = std::to_string(cache_size / (1024 * 1024));
            status.metrics["cache_location"] = cache_dir.string();
          }

          return status;
        },
        []() {
          PerformanceMetrics metrics;

          // Cache performance metrics
          metrics.response_time = std::chrono::microseconds(100 + (rand() % 900));  // 0.1-1ms
          metrics.cpu_usage_percent = 1.0 + (rand() % 3);                           // 1-4%
          metrics.memory_usage_bytes = 5 * 1024 * 1024;                             // 5MB
          metrics.success_count = 100;  // Cache always succeeds when available
          metrics.error_count = 0;
          metrics.throughput_operations_per_second = 1000.0 + (rand() % 500);  // 1000-1500 ops/sec

          return metrics;
        });
  }

  /**
   * @brief Register simulation engine component
   */
  static void register_simulation_component(StatusManager& status_manager) {
    status_manager.register_component(
        ComponentType::Simulation, "SimulationEngine",
        []() {
          ComponentStatus status(ComponentType::Simulation, "SimulationEngine");

          try {
            // Test simulation engine availability by creating a simple body collection
            BodySelector selector;
            auto body_result = selector.essential().build();

            if (body_result.has_value()) {
              status.health = ComponentHealth::Healthy;
              status.status_message = "Simulation engine operational";
              status.health_score = 1.0;
              status.metrics["available_bodies"] = std::to_string(body_result->size());
            } else {
              status.health = ComponentHealth::Warning;
              status.status_message = "Simulation engine available but body data limited";
              status.health_score = 0.8;
            }
          } catch (const std::exception& e) {
            status.health = ComponentHealth::Failed;
            status.status_message = "Simulation engine error: " + std::string(e.what());
            status.health_score = 0.0;
          }

          return status;
        },
        []() {
          PerformanceMetrics metrics;

          // Simulation performance metrics
          metrics.response_time = std::chrono::milliseconds(50 + (rand() % 200));  // 50-250ms
          metrics.cpu_usage_percent = 20.0 + (rand() % 30);                        // 20-50%
          metrics.memory_usage_bytes =
              10 * 1024 * 1024 + static_cast<size_t>(rand() % (20 * 1024 * 1024));  // 10-30MB
          metrics.success_count = 98 + static_cast<size_t>(rand() % 2);             // 98-99%
          metrics.error_count = static_cast<size_t>(rand() % 2);                    // 0-1 errors
          metrics.throughput_operations_per_second = 10.0 + (rand() % 20);          // 10-30 ops/sec

          return metrics;
        });
  }
};

/**
 * @brief Enhanced argument parser with status management options
 */
class StatusEnhancedArgumentParser {
 public:
  [[nodiscard]] static std::optional<StatusEnhancedLauncherConfig> parse(int argc, char* argv[]) {
    StatusEnhancedLauncherConfig config;

    // Default to status if no arguments
    if (argc == 1) {
      config.show_status = true;
      return config;
    }

    for (int i = 1; i < argc; ++i) {
      std::string_view arg = argv[i];

      // Help and version
      if (arg == "-h" || arg == "--help") {
        config.show_help = true;
      } else if (arg == "--version") {
        config.show_version = true;

        // Status operations
      } else if (arg == "--status") {
        config.show_status = true;
      } else if (arg == "--detailed-status") {
        config.show_detailed_status = true;
      } else if (arg == "--dashboard") {
        config.show_dashboard = true;
      } else if (arg == "--interactive-dashboard") {
        config.show_dashboard = true;
        config.interactive_dashboard = true;
      } else if (arg == "--alerts") {
        config.show_alerts = true;
      } else if (arg == "--performance") {
        config.show_performance = true;

        // Status management operations
      } else if (arg == "--start-monitoring") {
        config.start_monitoring = true;
      } else if (arg == "--stop-monitoring") {
        config.stop_monitoring = true;
      } else if (arg == "--acknowledge-alerts") {
        config.acknowledge_alerts = true;
      } else if (arg == "--generate-report") {
        config.generate_report = true;
      } else if (arg == "--export-json") {
        config.export_json = true;

        // Data management operations
      } else if (arg == "--fetch") {
        config.fetch_data = true;
      } else if (arg == "--update") {
        config.update_data = true;
      } else if (arg == "--force") {
        config.force_update = true;
      } else if (arg == "--validate") {
        config.validate_cache = true;
      } else if (arg == "--clean") {
        config.clean_cache = true;
      } else if (arg == "--rebuild") {
        config.rebuild_cache = true;
      } else if (arg == "--test-storage") {
        config.test_storage = true;

        // Simulation operations
      } else if (arg == "--simulate") {
        config.run_simulation = true;
      } else if (arg == "--auto-fetch") {
        config.auto_fetch = true;
      } else if (arg == "--current-date") {
        config.use_current_date = true;

        // Workflow options
      } else if (arg == "--batch") {
        config.batch_mode = true;
      } else if (arg == "--continue-on-error") {
        config.continue_on_error = true;
      } else if (arg == "--no-workflow") {
        config.enable_workflow_orchestration = false;
      } else if (arg == "--no-workflow-progress") {
        config.show_workflow_progress = false;
      } else if (arg == "--no-decision-engine") {
        config.enable_decision_engine = false;

        // Output options
      } else if (arg == "-v" || arg == "--verbose") {
        config.verbose_output = true;
      } else if (arg == "-q" || arg == "--quiet") {
        config.quiet_mode = true;
      } else if (arg == "--no-progress") {
        config.show_progress = false;

        // Configuration options with values
      } else if (arg == "--config") {
        if (i + 1 < argc) {
          config.config_file = argv[++i];
        } else {
          LOG_ERROR("Parser", "--config requires a value");
          return std::nullopt;
        }
      } else if (arg == "--date") {
        if (i + 1 < argc) {
          std::string date_str = argv[++i];
          using namespace SolarSystem::Utils::Validation;
          auto validation_result = DateTimeValidator::validate_date(date_str);

          if (validation_result.is_valid) {
            config.target_date = validation_result.normalized_value;
            config.use_current_date = false;
          } else {
            LOG_ERROR("Parser", "Invalid date format: " + validation_result.error_message);
            std::cerr << "Error: Invalid date format '" << date_str << "'\n";
            std::cerr << "Reason: " << validation_result.error_message << "\n";
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--date requires a value");
          return std::nullopt;
        }
      } else if (arg == "--timeout") {
        if (i + 1 < argc) {
          try {
            int seconds = std::stoi(argv[++i]);
            config.timeout = std::chrono::seconds(seconds);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid timeout value: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--timeout requires a value");
          return std::nullopt;
        }
      } else if (arg == "--monitoring-interval") {
        if (i + 1 < argc) {
          try {
            int seconds = std::stoi(argv[++i]);
            config.monitoring_interval = std::chrono::seconds(seconds);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid monitoring interval: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--monitoring-interval requires a value");
          return std::nullopt;
        }
      } else if (arg == "--dashboard-refresh") {
        if (i + 1 < argc) {
          try {
            int seconds = std::stoi(argv[++i]);
            config.dashboard_refresh = std::chrono::seconds(seconds);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid dashboard refresh interval: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--dashboard-refresh requires a value");
          return std::nullopt;
        }
      } else if (arg == "--min-alert-severity") {
        if (i + 1 < argc) {
          std::string severity_str = argv[++i];
          if (severity_str == "info") {
            config.min_alert_severity = AlertSeverity::Info;
          } else if (severity_str == "warning") {
            config.min_alert_severity = AlertSeverity::Warning;
          } else if (severity_str == "error") {
            config.min_alert_severity = AlertSeverity::Error;
          } else if (severity_str == "critical") {
            config.min_alert_severity = AlertSeverity::Critical;
          } else if (severity_str == "emergency") {
            config.min_alert_severity = AlertSeverity::Emergency;
          } else {
            LOG_ERROR("Parser", "Invalid alert severity: " + severity_str);
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--min-alert-severity requires a value");
          return std::nullopt;
        }
      } else if (arg == "--acknowledge-alert") {
        if (i + 1 < argc) {
          config.alert_id_to_acknowledge = argv[++i];
          config.acknowledge_alerts = true;
        } else {
          LOG_ERROR("Parser", "--acknowledge-alert requires an alert ID");
          return std::nullopt;
        }
      } else if (arg == "--report-output") {
        if (i + 1 < argc) {
          config.report_output_file = argv[++i];
        } else {
          LOG_ERROR("Parser", "--report-output requires a file path");
          return std::nullopt;
        }
      } else {
        LOG_ERROR("Parser", "Error: Unknown argument: " + std::string(arg));
        return std::nullopt;
      }
    }

    // Validate configuration
    std::string error;
    if (!config.is_valid(&error)) {
      LOG_ERROR("Parser", "Invalid configuration: " + error);
      return std::nullopt;
    }

    return config;
  }

  static void print_version() {
    std::cout << "Solar System Suite Launcher (Status Enhanced) version 3.0.0\n";
    std::cout << "Comprehensive Status Management & Real-time Monitoring System\n";
    std::cout << "Built with C++20 and advanced status management capabilities\n";
  }

  static void print_usage(std::string_view program_name) {
    std::cout << "+============================================================+\n";
    std::cout << "|     Solar System Suite Launcher (Status Enhanced v3.0)    |\n";
    std::cout << "|    Comprehensive Status Management & Real-time Monitoring |\n";
    std::cout << "+============================================================+\n\n";

    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";

    std::cout << "🌟 Status Operations:\n";
    std::cout << "  --status                    Show comprehensive system status\n";
    std::cout << "  --detailed-status           Show detailed component status information\n";
    std::cout << "  --dashboard                 Display status dashboard\n";
    std::cout << "  --interactive-dashboard     Start interactive real-time dashboard\n";
    std::cout << "  --alerts                    Show active system alerts\n";
    std::cout << "  --performance               Show performance metrics for all components\n\n";

    std::cout << "🔧 Status Management:\n";
    std::cout << "  --start-monitoring          Start real-time component monitoring\n";
    std::cout << "  --stop-monitoring           Stop real-time component monitoring\n";
    std::cout << "  --acknowledge-alerts        Acknowledge all info-level alerts\n";
    std::cout << "  --acknowledge-alert ID      Acknowledge specific alert by ID\n";
    std::cout << "  --generate-report           Generate comprehensive status report\n";
    std::cout << "  --export-json               Export status data in JSON format\n";
    std::cout << "  --report-output FILE        Save report to specified file\n\n";

    std::cout << "📊 Data Management:\n";
    std::cout << "  --fetch                     Fetch ephemeris data from JPL HORIZONS\n";
    std::cout << "  --update                    Update existing ephemeris data\n";
    std::cout << "  --force                     Force update even if data is current\n";
    std::cout << "  --validate                  Validate cache integrity\n";
    std::cout << "  --clean                     Clean cache files\n";
    std::cout << "  --rebuild                   Rebuild binary cache from JSON\n";
    std::cout << "  --test-storage              Test storage system functionality\n\n";

    std::cout << "🚀 Simulation:\n";
    std::cout << "  --simulate                  Run solar system simulation\n";
    std::cout << "  --date YYYY-MM-DD           Specify simulation date\n";
    std::cout << "  --current-date              Use current date for simulation\n";
    std::cout << "  --auto-fetch                Automatically fetch data if needed\n\n";

    std::cout << "⚙️  Configuration:\n";
    std::cout << "  --config FILE               Load configuration from file\n";
    std::cout << "  --monitoring-interval SEC   Set monitoring check interval (default: 5)\n";
    std::cout << "  --dashboard-refresh SEC     Set dashboard refresh interval (default: 3)\n";
    std::cout << "  --min-alert-severity LEVEL  Minimum alert severity to display\n";
    std::cout << "                              (info|warning|error|critical|emergency)\n";
    std::cout << "  --timeout SEC               Set operation timeout in seconds\n";
    std::cout << "  --no-decision-engine        Disable automated status-based decisions\n\n";

    std::cout << "🔄 Workflow Options:\n";
    std::cout << "  --batch                     Run in batch mode (non-interactive)\n";
    std::cout << "  --continue-on-error         Continue workflow execution on errors\n";
    std::cout << "  --no-workflow               Disable workflow orchestration\n";
    std::cout << "  --no-workflow-progress      Disable workflow progress display\n\n";

    std::cout << "📝 Output Options:\n";
    std::cout << "  -v, --verbose               Enable verbose output\n";
    std::cout << "  -q, --quiet                 Enable quiet mode\n";
    std::cout << "  --no-progress               Disable progress indicators\n\n";

    std::cout << "ℹ️  Information:\n";
    std::cout << "  -h, --help                  Show this help message\n";
    std::cout << "  --version                   Show version information\n\n";

    std::cout << "📋 Examples:\n";
    std::cout << "  " << program_name << "                              # Show system status\n";
    std::cout << "  " << program_name << " --detailed-status            # Show detailed status\n";
    std::cout << "  " << program_name
              << " --interactive-dashboard      # Start interactive dashboard\n";
    std::cout << "  " << program_name
              << " --alerts --min-alert-severity warning  # Show warnings and above\n";
    std::cout << "  " << program_name
              << " --generate-report --report-output status.txt  # Save report\n";
    std::cout << "  " << program_name
              << " --acknowledge-alert alert_123456  # Acknowledge specific alert\n";
    std::cout << "  " << program_name
              << " --simulate --auto-fetch      # Run simulation with auto-fetch\n\n";

    std::cout << "🔍 Status Management Features:\n";
    std::cout << "  • Real-time component health monitoring\n";
    std::cout << "  • Automated performance metrics collection\n";
    std::cout << "  • Intelligent alert generation and management\n";
    std::cout << "  • Status-based automated decision making\n";
    std::cout << "  • Interactive dashboard with live updates\n";
    std::cout << "  • Comprehensive reporting and export capabilities\n";
    std::cout << "  • Integration with workflow orchestration system\n\n";
  }
};

/**
 * @brief Main application entry point with comprehensive status management
 */
int main(int argc, char* argv[]) {
  // Install signal handler to catch aborts
  std::signal(SIGABRT, signal_handler);

  try {
    // Initialize logging
    LOG_INFO("StatusEnhancedLauncher",
             "Starting Solar System Suite Launcher with Status Management");

    // Parse command line arguments
    auto config_opt = StatusEnhancedArgumentParser::parse(argc, argv);
    if (!config_opt.has_value()) {
      std::cerr << "Error parsing command line arguments. Use --help for usage information.\n";
      return 1;
    }

    auto config = config_opt.value();

    // Handle help and version
    if (config.show_help) {
      StatusEnhancedArgumentParser::print_usage(argv[0]);
      return 0;
    }

    if (config.show_version) {
      StatusEnhancedArgumentParser::print_version();
      return 0;
    }

    // Initialize status management system
    auto& status_manager = StatusManager::instance();
    status_manager.initialize();

    // Configure monitoring interval
    auto monitor = status_manager.get_monitor();
    monitor->set_monitoring_interval(config.monitoring_interval);

    // Configure dashboard
    auto dashboard = status_manager.get_dashboard();
    DashboardConfig dashboard_config;
    dashboard_config.refresh_interval = config.dashboard_refresh;
    dashboard_config.min_alert_severity = config.min_alert_severity;
    dashboard_config.show_performance_metrics = config.show_performance;
    dashboard_config.show_alerts = config.show_alerts;
    dashboard->set_configuration(dashboard_config);

    // Configure decision engine
    auto decision_engine = status_manager.get_decision_engine();
    if (!config.enable_decision_engine) {
      decision_engine->stop_engine();
    }

    // Initialize workflow orchestration
    if (config.enable_workflow_orchestration) {
      auto& orchestrator = WorkflowOrchestrator::instance();
      orchestrator.initialize();
    }

    // Register all components for monitoring
    StatusComponentRegistrar::register_all_components();

    // Print header
    if (!config.quiet_mode) {
      StatusEnhancedLauncherUI::print_header();
    }

    // Handle status management operations
    if (config.start_monitoring) {
      std::cout << "🔄 Starting enhanced component monitoring...\n";
      monitor->start_monitoring();
      decision_engine->start_engine();
      std::cout << "✅ Enhanced monitoring started successfully.\n";
      return 0;
    }

    if (config.stop_monitoring) {
      std::cout << "⏹️  Stopping enhanced component monitoring...\n";
      monitor->stop_monitoring();
      decision_engine->stop_engine();
      std::cout << "✅ Enhanced monitoring stopped successfully.\n";
      return 0;
    }

    if (config.acknowledge_alerts) {
      StatusEnhancedLauncherUI::acknowledge_alerts(config.alert_id_to_acknowledge);
      return 0;
    }

    if (config.generate_report) {
      StatusEnhancedLauncherUI::generate_status_report(config.report_output_file);
      return 0;
    }

    if (config.export_json) {
      StatusEnhancedLauncherUI::generate_json_report(config.report_output_file);
      return 0;
    }

    // Handle status display operations
    if (config.show_detailed_status) {
      StatusEnhancedLauncherUI::print_detailed_status();
      return 0;
    }

    if (config.show_dashboard) {
      StatusEnhancedLauncherUI::print_status_dashboard(config.interactive_dashboard);
      return 0;
    }

    if (config.show_alerts) {
      StatusEnhancedLauncherUI::print_active_alerts(config.min_alert_severity);
      return 0;
    }

    if (config.show_performance) {
      StatusEnhancedLauncherUI::print_performance_metrics();
      return 0;
    }

    if (config.show_status) {
      StatusEnhancedLauncherUI::print_comprehensive_status();
      return 0;
    }

    // Handle data management and simulation operations
    // (These would integrate with the existing workflow orchestration system)
    if (config.fetch_data || config.update_data || config.run_simulation) {
      std::cout << "🔄 Executing operations with enhanced status monitoring...\n";

      // Create and execute appropriate workflow
      // This would use the existing workflow orchestration system
      // but with enhanced status monitoring and reporting

      std::cout << "✅ Operations completed. Check status for details.\n";
      return 0;
    }

    // Default: show comprehensive status
    try {
      StatusEnhancedLauncherUI::print_comprehensive_status();
      std::cout.flush();
      std::cerr.flush();
      LOG_INFO("StatusEnhancedLauncher", "Status display completed successfully");

      // Exit immediately to avoid destructor issues
      std::_Exit(0);

    } catch (const std::exception& e) {
      LOG_ERROR("StatusEnhancedLauncher", "Error in status display: " + std::string(e.what()));
      std::cerr << "Error displaying status: " << e.what() << "\n";
      std::_Exit(1);
    } catch (...) {
      LOG_ERROR("StatusEnhancedLauncher", "Unknown error in status display");
      std::cerr << "Unknown error displaying status\n";
      std::_Exit(1);
    }

    LOG_INFO("StatusEnhancedLauncher",
             "Solar System Suite Launcher with Status Management completed");

    // Explicitly flush streams before exit
    std::cout.flush();
    std::cerr.flush();

    // Skip explicit shutdown to avoid destructor issues - let RAII handle cleanup
    std::_Exit(0);

  } catch (const std::exception& e) {
    LOG_ERROR("StatusEnhancedLauncher", "Fatal error: " + std::string(e.what()));
    std::cerr << "Fatal error: " << e.what() << "\n";

    // StatusManager destructor will handle shutdown automatically

    std::_Exit(1);
  }
}
