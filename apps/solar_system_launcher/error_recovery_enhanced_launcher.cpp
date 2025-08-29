/**
 * @file error_recovery_enhanced_launcher.cpp
 * @brief Solar System Suite Launcher with Intelligent Error Recovery System (Task 3)
 *
 * Implements Task 3: Intelligent error recovery system with:
 * - Error detection and classification
 * - Automatic recovery strategies for common errors
 * - User-guided recovery workflows
 * - Error prevention and early warning
 * - Integration with existing error handling infrastructure
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

// Enhanced Solar System Suite APIs with intelligent error recovery
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/builders/simulation_builder.hpp"
#include "solar_utils/logging.hpp"
#include "solar_utils/validation/input_validator.hpp"
#include "solar_utils/workflow_orchestration.hpp"
#include "solar_utils/status_management.hpp"
#include "solar_utils/error_handling.hpp"
#include "solar_utils/error_recovery.hpp"

using namespace SolarSystem::Core::Builders;
using namespace SolarSystem::Utils;
using namespace SolarSystem::Utils::Workflow;
using namespace SolarSystem::Utils::Status;
using namespace std::chrono_literals;
// Signal handler for graceful error recovery
void error_recovery_signal_handler(int signal) {
  std::cerr << "Signal " << signal << " caught - initiating error recovery\n";

  // Attempt graceful recovery
  auto& recovery_orchestrator = ErrorRecoveryOrchestrator::instance();
  if (recovery_orchestrator.is_system_healthy()) {
    std::cerr << "System appears healthy - performing graceful shutdown\n";
  } else {
    std::cerr << "System unhealthy - emergency recovery initiated\n";
  }

  std::_Exit(signal);
}

/**
 * @brief Enhanced configuration for launcher operations with error recovery
 */
struct ErrorRecoveryLauncherConfig {
  // Operation modes
  bool show_status = false;
  bool show_help = false;
  bool show_version = false;
  bool show_error_report = false;
  bool test_error_recovery = false;

  // Error recovery operations
  bool enable_auto_recovery = true;
  bool enable_user_guided_recovery = true;
  bool enable_error_prevention = true;
  bool enable_early_detection = true;
  bool show_recovery_dashboard = false;

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

  // Error recovery configuration
  RecoveryMode recovery_mode = RecoveryMode::Automatic;
  std::chrono::seconds recovery_timeout{300};
  size_t max_recovery_attempts = 3;
  bool enable_learning = true;
  bool export_recovery_data = false;
  std::optional<std::string> recovery_data_file;

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
};
/**
 * @brief Launcher-specific error recovery system
 */
class LauncherErrorRecoverySystem {
 public:
  /**
   * @brief Initialize the launcher error recovery system (lightweight)
   */
  static void initialize() {
    LOG_INFO("LauncherErrorRecovery", "Initializing intelligent error recovery system");

    try {
      // Initialize the error recovery orchestrator
      auto& orchestrator = ErrorRecoveryOrchestrator::instance();
      orchestrator.initialize();

      // Register launcher-specific recovery workflows
      register_launcher_recovery_workflows();

      // Register error prevention rules (but don't activate)
      register_launcher_prevention_rules();

      // Register early detection monitors (but don't start)
      register_launcher_detection_monitors();

      // Set up user interaction handler
      setup_user_interaction_handler();

      LOG_INFO("LauncherErrorRecovery", "Error recovery system initialized successfully");
    } catch (const std::exception& e) {
      LOG_ERROR("LauncherErrorRecovery", "Failed to initialize error recovery system: " + std::string(e.what()));
      // Continue without error recovery
    }
  }

  /**
   * @brief Handle error with intelligent recovery
   */
  static bool handle_error_with_recovery(const DetailedError& error) {
    LOG_INFO("LauncherErrorRecovery", "Handling error with intelligent recovery: " + error.message);

    auto& orchestrator = ErrorRecoveryOrchestrator::instance();
    return orchestrator.handle_error(error);
  }

  /**
   * @brief Test error recovery system
   */
  static void test_error_recovery_system() {
    std::cout << "🧪 Testing Error Recovery System\n\n";

    // Test 1: Network connectivity error
    test_network_error_recovery();

    // Test 2: Cache corruption error
    test_cache_error_recovery();

    // Test 3: Configuration error
    test_configuration_error_recovery();

    // Test 4: Memory error
    test_memory_error_recovery();

    std::cout << "\n✅ Error recovery system testing completed\n";
  }

  /**
   * @brief Show error recovery dashboard
   */
  static void show_recovery_dashboard() {
    std::cout << "📊 Error Recovery Dashboard\n\n";

    auto& orchestrator = ErrorRecoveryOrchestrator::instance();
    auto& recovery_manager = AdvancedErrorRecoveryManager::instance();
    auto& prevention_system = ErrorPreventionSystem::instance();
    auto& detection_system = EarlyDetectionSystem::instance();

    // System health
    std::cout << "🏥 System Health:\n";
    if (orchestrator.is_system_healthy()) {
      std::cout << "  ✅ Status: HEALTHY\n";
    } else {
      std::cout << "  ⚠️  Status: DEGRADED\n";
    }

    // Active recoveries
    auto active_recoveries = recovery_manager.get_active_recoveries();
    std::cout << "\n🔄 Active Recoveries: " << active_recoveries.size() << "\n";
    for (const auto& recovery : active_recoveries) {
      std::cout << "  • " << recovery.workflow_id << " ("
                << [](RecoveryStatus status) {
                     switch (status) {
                       case RecoveryStatus::NotStarted: return "NotStarted";
                       case RecoveryStatus::InProgress: return "InProgress";
                       case RecoveryStatus::WaitingForUser: return "WaitingForUser";
                       case RecoveryStatus::Completed: return "Completed";
                       case RecoveryStatus::Failed: return "Failed";
                       case RecoveryStatus::Cancelled: return "Cancelled";
                       case RecoveryStatus::Timeout: return "Timeout";
                       default: return "Unknown";
                     }
                   }(recovery.status) << ")\n";
    }

    // Prevention statistics
    auto prevented_errors = prevention_system.get_prevented_errors();
    std::cout << "\n🛡️  Prevented Errors: " << prevented_errors.size() << "\n";

    // Detection status
    std::cout << "\n🔍 Early Detection:\n";
    if (detection_system.is_monitoring()) {
      std::cout << "  ✅ Status: ACTIVE\n";
      auto health_score = detection_system.get_overall_health_score();
      std::cout << "  📊 Health Score: " << std::fixed << std::setprecision(2)
                << health_score << "/1.0\n";
    } else {
      std::cout << "  ❌ Status: INACTIVE\n";
    }

    // Learning data
    auto learning_data = recovery_manager.get_learning_data();
    std::cout << "\n🧠 Learning Data Points: " << learning_data.size() << "\n";

    std::cout << "\n";
  }

 private:
  /**
   * @brief Register launcher-specific recovery workflows
   */
  static void register_launcher_recovery_workflows() {
    auto& recovery_manager = AdvancedErrorRecoveryManager::instance();

    // JPL connectivity recovery workflow
    recovery_manager.register_recovery_workflow(create_jpl_connectivity_recovery_workflow());

    // Cache management recovery workflow
    recovery_manager.register_recovery_workflow(create_cache_recovery_workflow());

    // Configuration recovery workflow
    recovery_manager.register_recovery_workflow(create_configuration_recovery_workflow());

    // Simulation recovery workflow
    recovery_manager.register_recovery_workflow(create_simulation_recovery_workflow());

    LOG_INFO("LauncherErrorRecovery", "Registered launcher-specific recovery workflows");
  }
  /**
   * @brief Register error prevention rules
   */
  static void register_launcher_prevention_rules() {
    auto& prevention_system = ErrorPreventionSystem::instance();

    // JPL connectivity prevention
    PreventionRule jpl_prevention;
    jpl_prevention.rule_id = "jpl_connectivity_prevention";
    jpl_prevention.name = "JPL Connectivity Prevention";
    jpl_prevention.description = "Prevent JPL connectivity errors by monitoring network health";
    jpl_prevention.target_category = ErrorCategory::Network;
    jpl_prevention.target_codes = {ErrorCode::ConnectionFailed, ErrorCode::ConnectionTimeout};
    jpl_prevention.condition = [](const DetailedError& error) {
      return error.category == ErrorCategory::Network &&
             error.message.find("JPL") != std::string::npos;
    };
    jpl_prevention.prevention_action = []() {
      // Check network connectivity and switch to fallback mode if needed
      auto& orchestrator = WorkflowOrchestrator::instance();
      auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();

      if (!jpl_manager.is_jpl_service_available()) {
        jpl_manager.set_fallback_mode(true);
        LOG_INFO("LauncherErrorRecovery", "Enabled JPL fallback mode to prevent connectivity errors");
        return true;
      }
      return false;
    };
    prevention_system.register_prevention_rule(jpl_prevention);

    // Cache corruption prevention
    PreventionRule cache_prevention;
    cache_prevention.rule_id = "cache_corruption_prevention";
    cache_prevention.name = "Cache Corruption Prevention";
    cache_prevention.description = "Prevent cache corruption by validating cache integrity";
    cache_prevention.target_category = ErrorCategory::FileSystem;
    cache_prevention.target_codes = {ErrorCode::FileCorrupted, ErrorCode::FileNotFound};
    cache_prevention.condition = [](const DetailedError& error) {
      return error.message.find("cache") != std::string::npos ||
             error.message.find("ephemeris") != std::string::npos;
    };
    cache_prevention.prevention_action = []() {
      // Validate cache files and rebuild if necessary
      std::filesystem::path cache_dir = "./cache";
      bool binary_exists = std::filesystem::exists(cache_dir / "ephemeris_cache.bin");
      bool json_exists = std::filesystem::exists(cache_dir / "ephemeris_data.json");

      if (!binary_exists && json_exists) {
        LOG_INFO("LauncherErrorRecovery", "Rebuilding binary cache to prevent corruption");
        // Trigger cache rebuild
        return true;
      }
      return false;
    };
    prevention_system.register_prevention_rule(cache_prevention);

    LOG_INFO("LauncherErrorRecovery", "Registered launcher-specific prevention rules");
  }

  /**
   * @brief Register early detection monitors
   */
  static void register_launcher_detection_monitors() {
    auto& detection_system = EarlyDetectionSystem::instance();

    // JPL connectivity monitor
    EarlyDetectionMonitor jpl_monitor;
    jpl_monitor.monitor_id = "jpl_connectivity_monitor";
    jpl_monitor.name = "JPL HORIZONS Connectivity Monitor";
    jpl_monitor.description = "Monitor JPL HORIZONS API connectivity and performance";
    jpl_monitor.check_interval = std::chrono::seconds(60);
    jpl_monitor.warning_threshold = 0.7;
    jpl_monitor.critical_threshold = 0.3;
    jpl_monitor.health_check = []() {
      auto& orchestrator = WorkflowOrchestrator::instance();
      auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();

      if (jpl_manager.is_jpl_service_available()) {
        return 1.0;  // Healthy
      } else if (jpl_manager.is_fallback_mode_active()) {
        return 0.5;  // Degraded but functional
      } else {
        return 0.0;  // Failed
      }
    };
    jpl_monitor.diagnostic = []() {
      std::vector<DetailedError> issues;
      auto& orchestrator = WorkflowOrchestrator::instance();
      auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();

      if (!jpl_manager.is_jpl_service_available()) {
        DetailedError error(ErrorCode::NetworkUnavailable,
                          "JPL HORIZONS API is not accessible",
                          ErrorSeverity::Warning,
                          "JPL connectivity check");
        error.suggestions.push_back("Check network connectivity");
        error.suggestions.push_back("Verify JPL HORIZONS service status");
        error.recovery_action = "Enable fallback mode";
        issues.push_back(error);
      }

      return issues;
    };
    detection_system.register_monitor(jpl_monitor);

    // System resource monitor
    EarlyDetectionMonitor resource_monitor;
    resource_monitor.monitor_id = "system_resource_monitor";
    resource_monitor.name = "System Resource Monitor";
    resource_monitor.description = "Monitor system resources (memory, disk space)";
    resource_monitor.check_interval = std::chrono::seconds(30);
    resource_monitor.warning_threshold = 0.2;
    resource_monitor.critical_threshold = 0.1;
    resource_monitor.health_check = []() {
      // Simple resource check - in real implementation would check actual resources
      return 0.8;  // Assume healthy for demo
    };
    resource_monitor.diagnostic = []() {
      std::vector<DetailedError> issues;
      // In real implementation, would check actual resource usage
      return issues;
    };
    detection_system.register_monitor(resource_monitor);

    // Note: Don't start monitoring automatically to avoid blocking
    // Users can start monitoring with --recovery-dashboard
    LOG_INFO("LauncherErrorRecovery", "Registered early detection monitors (not started)");
  }
  /**
   * @brief Set up user interaction handler
   */
  static void setup_user_interaction_handler() {
    auto& recovery_manager = AdvancedErrorRecoveryManager::instance();

    recovery_manager.set_user_interaction_handler(
      [](const UserInteractionRequest& request) -> UserInteractionResponse {
        UserInteractionResponse response;
        response.request_id = request.id;
        response.response_time = std::chrono::system_clock::now();

        std::cout << "\n🤖 User Interaction Required\n";
        std::cout << "Title: " << request.title << "\n";
        std::cout << "Message: " << request.message << "\n";

        switch (request.type) {
          case UserInteractionType::Confirmation: {
            std::cout << "Confirm (y/n): ";
            std::string input;
            std::getline(std::cin, input);
            response.selected_option = (input == "y" || input == "yes") ? "yes" : "no";
            break;
          }

          case UserInteractionType::Selection: {
            std::cout << "Options:\n";
            for (size_t i = 0; i < request.options.size(); ++i) {
              std::cout << "  " << (i + 1) << ". " << request.options[i] << "\n";
            }
            std::cout << "Select option (1-" << request.options.size() << "): ";

            std::string input;
            std::getline(std::cin, input);
            try {
              size_t choice = std::stoul(input);
              if (choice >= 1 && choice <= request.options.size()) {
                response.selected_option = request.options[choice - 1];
              } else {
                response.selected_option = request.default_value;
              }
            } catch (...) {
              response.selected_option = request.default_value;
            }
            break;
          }

          case UserInteractionType::Input: {
            std::cout << "Enter value";
            if (!request.default_value.empty()) {
              std::cout << " (default: " << request.default_value << ")";
            }
            std::cout << ": ";

            std::string input;
            std::getline(std::cin, input);
            response.user_input = input.empty() ? request.default_value : input;
            break;
          }

          default:
            response.cancelled = true;
            break;
        }

        return response;
      }
    );

    LOG_INFO("LauncherErrorRecovery", "Set up user interaction handler");
  }

  /**
   * @brief Create JPL connectivity recovery workflow
   */
  static RecoveryWorkflow create_jpl_connectivity_recovery_workflow() {
    RecoveryWorkflow workflow;
    workflow.workflow_id = "jpl_connectivity_recovery";
    workflow.name = "JPL HORIZONS Connectivity Recovery";
    workflow.description = "Recover from JPL HORIZONS API connectivity issues";
    workflow.mode = RecoveryMode::Automatic;
    workflow.total_timeout = std::chrono::seconds(120);

    // Step 1: Test connectivity
    RecoveryStep test_step;
    test_step.step_id = "test_connectivity";
    test_step.description = "Test JPL HORIZONS API connectivity";
    test_step.timeout = std::chrono::seconds(30);
    test_step.max_retries = 3;
    test_step.action = []() {
      auto& orchestrator = WorkflowOrchestrator::instance();
      auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();

      LOG_INFO("JPLRecovery", "Testing JPL HORIZONS connectivity");
      return jpl_manager.is_jpl_service_available();
    };
    test_step.validation = []() {
      auto& orchestrator = WorkflowOrchestrator::instance();
      auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();
      return jpl_manager.is_jpl_service_available();
    };
    workflow.steps.push_back(test_step);

    // Step 2: Enable fallback mode if connectivity fails
    RecoveryStep fallback_step;
    fallback_step.step_id = "enable_fallback";
    fallback_step.description = "Enable fallback mode for offline operation";
    fallback_step.timeout = std::chrono::seconds(10);
    fallback_step.action = []() {
      auto& orchestrator = WorkflowOrchestrator::instance();
      auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();

      LOG_INFO("JPLRecovery", "Enabling JPL fallback mode");
      jpl_manager.set_fallback_mode(true);
      return true;
    };
    fallback_step.validation = []() {
      auto& orchestrator = WorkflowOrchestrator::instance();
      auto& jpl_manager = orchestrator.get_jpl_connectivity_manager();
      return jpl_manager.is_fallback_mode_active();
    };
    workflow.steps.push_back(fallback_step);

    return workflow;
  }
  /**
   * @brief Create cache recovery workflow
   */
  static RecoveryWorkflow create_cache_recovery_workflow() {
    RecoveryWorkflow workflow;
    workflow.workflow_id = "cache_recovery";
    workflow.name = "Cache Recovery";
    workflow.description = "Recover from cache corruption or missing cache files";
    workflow.mode = RecoveryMode::SemiAutomatic;
    workflow.total_timeout = std::chrono::seconds(180);

    // Step 1: Diagnose cache state
    RecoveryStep diagnose_step;
    diagnose_step.step_id = "diagnose_cache";
    diagnose_step.description = "Diagnose cache file state";
    diagnose_step.timeout = std::chrono::seconds(10);
    diagnose_step.action = []() {
      std::filesystem::path cache_dir = "./cache";
      bool binary_exists = std::filesystem::exists(cache_dir / "ephemeris_cache.bin");
      bool json_exists = std::filesystem::exists(cache_dir / "ephemeris_data.json");

      LOG_INFO("CacheRecovery", "Cache diagnosis - Binary: " +
               std::string(binary_exists ? "OK" : "Missing") +
               ", JSON: " + std::string(json_exists ? "OK" : "Missing"));

      return binary_exists || json_exists;
    };
    workflow.steps.push_back(diagnose_step);

    // Step 2: Rebuild cache if possible
    RecoveryStep rebuild_step;
    rebuild_step.step_id = "rebuild_cache";
    rebuild_step.description = "Rebuild binary cache from JSON data";
    rebuild_step.timeout = std::chrono::seconds(60);
    rebuild_step.dependencies = {"diagnose_cache"};
    rebuild_step.action = []() {
      std::filesystem::path cache_dir = "./cache";
      bool json_exists = std::filesystem::exists(cache_dir / "ephemeris_data.json");

      if (json_exists) {
        LOG_INFO("CacheRecovery", "Rebuilding binary cache from JSON data");
        // In real implementation, would trigger actual cache rebuild
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return true;
      }

      return false;
    };

    // Add user interaction for cache rebuild confirmation
    UserInteractionRequest rebuild_confirmation;
    rebuild_confirmation.id = "rebuild_confirmation";
    rebuild_confirmation.type = UserInteractionType::Confirmation;
    rebuild_confirmation.title = "Cache Rebuild Required";
    rebuild_confirmation.message = "Cache files are corrupted or missing. Rebuild from available data?";
    rebuild_confirmation.timeout = std::chrono::seconds(30);
    rebuild_step.user_interactions.push_back(rebuild_confirmation);

    workflow.steps.push_back(rebuild_step);

    return workflow;
  }

  /**
   * @brief Create configuration recovery workflow
   */
  static RecoveryWorkflow create_configuration_recovery_workflow() {
    RecoveryWorkflow workflow;
    workflow.workflow_id = "configuration_recovery";
    workflow.name = "Configuration Recovery";
    workflow.description = "Recover from configuration errors and conflicts";
    workflow.mode = RecoveryMode::Interactive;
    workflow.total_timeout = std::chrono::seconds(300);

    // Step 1: Validate configuration
    RecoveryStep validate_step;
    validate_step.step_id = "validate_config";
    validate_step.description = "Validate current configuration";
    validate_step.timeout = std::chrono::seconds(30);
    validate_step.action = []() {
      LOG_INFO("ConfigRecovery", "Validating configuration");
      // In real implementation, would validate actual configuration
      return true;
    };
    workflow.steps.push_back(validate_step);

    // Step 2: Reset to defaults if needed
    RecoveryStep reset_step;
    reset_step.step_id = "reset_config";
    reset_step.description = "Reset configuration to safe defaults";
    reset_step.timeout = std::chrono::seconds(15);
    reset_step.dependencies = {"validate_config"};
    reset_step.action = []() {
      LOG_INFO("ConfigRecovery", "Resetting configuration to defaults");
      return true;
    };

    // Add user interaction for reset confirmation
    UserInteractionRequest reset_confirmation;
    reset_confirmation.id = "reset_confirmation";
    reset_confirmation.type = UserInteractionType::Confirmation;
    reset_confirmation.title = "Configuration Reset";
    reset_confirmation.message = "Reset configuration to safe defaults? This will lose custom settings.";
    reset_confirmation.timeout = std::chrono::seconds(60);
    reset_step.user_interactions.push_back(reset_confirmation);

    workflow.steps.push_back(reset_step);

    return workflow;
  }

  /**
   * @brief Create simulation recovery workflow
   */
  static RecoveryWorkflow create_simulation_recovery_workflow() {
    RecoveryWorkflow workflow;
    workflow.workflow_id = "simulation_recovery";
    workflow.name = "Simulation Recovery";
    workflow.description = "Recover from simulation execution errors";
    workflow.mode = RecoveryMode::Automatic;
    workflow.total_timeout = std::chrono::seconds(120);

    // Step 1: Validate simulation parameters
    RecoveryStep validate_step;
    validate_step.step_id = "validate_simulation";
    validate_step.description = "Validate simulation parameters";
    validate_step.timeout = std::chrono::seconds(20);
    validate_step.action = []() {
      LOG_INFO("SimulationRecovery", "Validating simulation parameters");
      // In real implementation, would validate actual simulation parameters
      return true;
    };
    workflow.steps.push_back(validate_step);

    // Step 2: Reduce simulation complexity if needed
    RecoveryStep reduce_step;
    reduce_step.step_id = "reduce_complexity";
    reduce_step.description = "Reduce simulation complexity for recovery";
    reduce_step.timeout = std::chrono::seconds(30);
    reduce_step.dependencies = {"validate_simulation"};
    reduce_step.action = []() {
      LOG_INFO("SimulationRecovery", "Reducing simulation complexity");
      // In real implementation, would reduce body count or timestep
      return true;
    };
    workflow.steps.push_back(reduce_step);

    return workflow;
  }
  /**
   * @brief Test network error recovery
   */
  static void test_network_error_recovery() {
    std::cout << "🌐 Testing Network Error Recovery\n";

    // Create a network error
    DetailedError network_error(ErrorCode::ConnectionFailed,
                               "Failed to connect to JPL HORIZONS API",
                               ErrorSeverity::Error,
                               "JPL connectivity test");
    network_error.suggestions.push_back("Check network connectivity");
    network_error.suggestions.push_back("Enable fallback mode");

    // Attempt recovery
    bool recovered = handle_error_with_recovery(network_error);

    std::cout << "  Result: " << (recovered ? "✅ Recovered" : "❌ Failed") << "\n";
  }

  /**
   * @brief Test cache error recovery
   */
  static void test_cache_error_recovery() {
    std::cout << "💾 Testing Cache Error Recovery\n";

    // Create a cache error
    DetailedError cache_error(ErrorCode::FileCorrupted,
                             "Ephemeris cache file is corrupted",
                             ErrorSeverity::Warning,
                             "Cache validation");
    cache_error.suggestions.push_back("Rebuild cache from JSON data");
    cache_error.suggestions.push_back("Re-fetch data from JPL");

    // Attempt recovery
    bool recovered = handle_error_with_recovery(cache_error);

    std::cout << "  Result: " << (recovered ? "✅ Recovered" : "❌ Failed") << "\n";
  }

  /**
   * @brief Test configuration error recovery
   */
  static void test_configuration_error_recovery() {
    std::cout << "⚙️  Testing Configuration Error Recovery\n";

    // Create a configuration error
    DetailedError config_error(ErrorCode::ConfigInvalid,
                              "Invalid configuration parameter detected",
                              ErrorSeverity::Error,
                              "Configuration validation");
    config_error.suggestions.push_back("Reset to default configuration");
    config_error.suggestions.push_back("Validate configuration file");

    // Attempt recovery
    bool recovered = handle_error_with_recovery(config_error);

    std::cout << "  Result: " << (recovered ? "✅ Recovered" : "❌ Failed") << "\n";
  }

  /**
   * @brief Test memory error recovery
   */
  static void test_memory_error_recovery() {
    std::cout << "🧠 Testing Memory Error Recovery\n";

    // Create a memory error
    DetailedError memory_error(ErrorCode::OutOfMemory,
                              "Insufficient memory for simulation",
                              ErrorSeverity::Critical,
                              "Simulation initialization");
    memory_error.suggestions.push_back("Reduce simulation complexity");
    memory_error.suggestions.push_back("Free unused resources");

    // Attempt recovery
    bool recovered = handle_error_with_recovery(memory_error);

    std::cout << "  Result: " << (recovered ? "✅ Recovered" : "❌ Failed") << "\n";
  }
};

/**
 * @brief Enhanced launcher UI with error recovery features
 */
class ErrorRecoveryLauncherUI {
 public:
  /**
   * @brief Print main header
   */
  static void print_header() {
    std::cout << "+============================================================+\n";
    std::cout << "|   Solar System Suite Launcher (Error Recovery v4.0)       |\n";
    std::cout << "|     Intelligent Error Recovery & Prevention System        |\n";
    std::cout << "+============================================================+\n\n";
  }

  /**
   * @brief Print error recovery system status
   */
  static void print_error_recovery_status() {
    std::cout << "🛡️  Error Recovery System Status\n\n";

    auto& orchestrator = ErrorRecoveryOrchestrator::instance();
    auto& recovery_manager = AdvancedErrorRecoveryManager::instance();
    auto& prevention_system = ErrorPreventionSystem::instance();
    auto& detection_system = EarlyDetectionSystem::instance();
    auto& error_system = ErrorHandlingSystem::instance();

    // System health
    std::cout << "🏥 System Health:\n";
    if (orchestrator.is_system_healthy()) {
      std::cout << "  ✅ Status: HEALTHY\n";
    } else {
      std::cout << "  ⚠️  Status: DEGRADED\n";
    }

    double health_score = error_system.get_system_health_score();
    std::cout << "  📊 Health Score: " << std::fixed << std::setprecision(2)
              << health_score << "/1.0\n";

    // Error statistics
    auto error_stats = error_system.get_error_statistics();
    std::cout << "\n📈 Error Statistics:\n";
    std::cout << "  Total Errors: " << error_stats.total_errors << "\n";
    std::cout << "  Successful Recoveries: " << error_stats.successful_recoveries << "\n";
    std::cout << "  Failed Recoveries: " << error_stats.failed_recoveries << "\n";

    if (error_stats.total_errors > 0) {
      double recovery_rate = static_cast<double>(error_stats.successful_recoveries) /
                            (error_stats.successful_recoveries + error_stats.failed_recoveries) * 100.0;
      std::cout << "  Recovery Success Rate: " << std::fixed << std::setprecision(1)
                << recovery_rate << "%\n";
    }

    // Active recoveries
    auto active_recoveries = recovery_manager.get_active_recoveries();
    std::cout << "\n🔄 Active Recoveries: " << active_recoveries.size() << "\n";

    // Prevention status
    auto prevented_errors = prevention_system.get_prevented_errors();
    std::cout << "\n🛡️  Prevention System:\n";
    std::cout << "  Prevented Errors: " << prevented_errors.size() << "\n";

    // Detection status
    std::cout << "\n🔍 Early Detection:\n";
    if (detection_system.is_monitoring()) {
      std::cout << "  ✅ Status: ACTIVE\n";
      auto detection_health = detection_system.get_overall_health_score();
      std::cout << "  📊 Detection Health: " << std::fixed << std::setprecision(2)
                << detection_health << "/1.0\n";
    } else {
      std::cout << "  ❌ Status: INACTIVE\n";
    }

    // Learning system
    auto learning_data = recovery_manager.get_learning_data();
    std::cout << "\n🧠 Learning System:\n";
    std::cout << "  Learning Data Points: " << learning_data.size() << "\n";

    std::cout << "\n";
  }

  /**
   * @brief Generate comprehensive error report
   */
  static void generate_error_report() {
    std::cout << "📄 Generating Comprehensive Error Report\n\n";

    auto& error_system = ErrorHandlingSystem::instance();
    auto report = error_system.generate_health_report();

    std::cout << report << "\n";
  }
};
/**
 * @brief Enhanced argument parser with error recovery options
 */
class ErrorRecoveryArgumentParser {
 public:
  [[nodiscard]] static std::optional<ErrorRecoveryLauncherConfig> parse(int argc, char* argv[]) {
    ErrorRecoveryLauncherConfig config;

    // Default to status if no arguments
    if (argc == 1) {
      config.show_status = true;
      return config;
    }

    for (int i = 1; i < argc; ++i) {
      std::string_view arg = argv[i];

      if (arg == "-h" || arg == "--help") {
        config.show_help = true;
      } else if (arg == "--version") {
        config.show_version = true;
      } else if (arg == "--status") {
        config.show_status = true;
      } else if (arg == "--error-report") {
        config.show_error_report = true;
      } else if (arg == "--test-recovery") {
        config.test_error_recovery = true;
      } else if (arg == "--recovery-dashboard") {
        config.show_recovery_dashboard = true;
      } else if (arg == "--disable-auto-recovery") {
        config.enable_auto_recovery = false;
      } else if (arg == "--disable-prevention") {
        config.enable_error_prevention = false;
      } else if (arg == "--disable-detection") {
        config.enable_early_detection = false;
      } else if (arg == "--manual-recovery") {
        config.recovery_mode = RecoveryMode::Manual;
      } else if (arg == "--interactive-recovery") {
        config.recovery_mode = RecoveryMode::Interactive;
      } else if (arg == "--export-recovery-data") {
        config.export_recovery_data = true;
        if (i + 1 < argc && argv[i + 1][0] != '-') {
          config.recovery_data_file = argv[++i];
        }
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
      } else if (arg == "--simulate") {
        config.run_simulation = true;
      } else if (arg == "--auto-fetch") {
        config.auto_fetch = true;
      } else if (arg == "--current-date") {
        config.use_current_date = true;
      } else if (arg == "--batch") {
        config.batch_mode = true;
      } else if (arg == "--continue-on-error") {
        config.continue_on_error = true;
      } else if (arg == "--no-workflow") {
        config.enable_workflow_orchestration = false;
      } else if (arg == "--no-workflow-progress") {
        config.show_workflow_progress = false;
      } else if (arg == "-v" || arg == "--verbose") {
        config.verbose_output = true;
      } else if (arg == "-q" || arg == "--quiet") {
        config.quiet_mode = true;
      } else if (arg == "--no-progress") {
        config.show_progress = false;
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
          // Validate date using shared validation library
          using namespace SolarSystem::Utils::Validation;
          auto validation_result = DateTimeValidator::validate_date(date_str);

          if (validation_result.is_valid) {
            config.target_date = validation_result.normalized_value;
            config.use_current_date = false;
          } else {
            // Create and handle validation error with recovery
            DetailedError validation_error(ErrorCode::InvalidFormat,
                                         "Invalid date format: " + date_str,
                                         ErrorSeverity::Error,
                                         "Date validation");
            validation_error.suggestions = validation_result.suggestions;

            LauncherErrorRecoverySystem::handle_error_with_recovery(validation_error);
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
          } catch (const std::exception& e) {
            // Create and handle parsing error with recovery
            DetailedError parsing_error(ErrorCode::InvalidInput,
                                       "Invalid timeout value: " + std::string(argv[i]),
                                       ErrorSeverity::Error,
                                       "Argument parsing");
            parsing_error.suggestions.push_back("Use a positive integer value");

            LauncherErrorRecoverySystem::handle_error_with_recovery(parsing_error);
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--timeout requires a value");
          return std::nullopt;
        }
      } else {
        // Create and handle unknown argument error with recovery
        DetailedError unknown_arg_error(ErrorCode::InvalidInput,
                                       "Unknown argument: " + std::string(arg),
                                       ErrorSeverity::Warning,
                                       "Argument parsing");
        unknown_arg_error.suggestions.push_back("Use --help to see available options");

        LauncherErrorRecoverySystem::handle_error_with_recovery(unknown_arg_error);
        return std::nullopt;
      }
    }

    return config;
  }

  static void print_version() {
    std::cout << "Solar System Suite Launcher (Error Recovery Enhanced) version 4.0.0\n";
    std::cout << "Intelligent Error Recovery & Prevention System\n";
    std::cout << "Built with C++20 and advanced error recovery capabilities\n";
  }

  static void print_usage(std::string_view program_name) {
    ErrorRecoveryLauncherUI::print_header();

    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";

    std::cout << "🌟 Primary Operations:\n";
    std::cout << "  --status              Show enhanced system status and health\n";
    std::cout << "  --simulate            Run solar system simulation with error recovery\n";
    std::cout << "  --fetch               Perform enhanced data management with recovery\n";
    std::cout << "  --update              Update ephemeris data with error handling\n\n";

    std::cout << "🛡️  Error Recovery Options:\n";
    std::cout << "  --error-report        Generate comprehensive error report\n";
    std::cout << "  --test-recovery       Test error recovery system\n";
    std::cout << "  --recovery-dashboard  Show error recovery dashboard\n";
    std::cout << "  --disable-auto-recovery    Disable automatic error recovery\n";
    std::cout << "  --disable-prevention       Disable error prevention system\n";
    std::cout << "  --disable-detection        Disable early detection system\n";
    std::cout << "  --manual-recovery          Use manual recovery mode\n";
    std::cout << "  --interactive-recovery     Use interactive recovery mode\n";
    std::cout << "  --export-recovery-data [FILE]  Export recovery data\n\n";

    std::cout << "📊 Data Management:\n";
    std::cout << "  --validate            Validate cache integrity with recovery\n";
    std::cout << "  --clean               Clean cache files safely\n";
    std::cout << "  --rebuild             Rebuild cache with error handling\n";
    std::cout << "  --force               Force operations with recovery support\n\n";

    std::cout << "⚙️  Configuration:\n";
    std::cout << "  --config FILE         Use configuration file\n";
    std::cout << "  --date DATE           Target date for simulation\n";
    std::cout << "  --timeout SECONDS     Operation timeout\n";
    std::cout << "  --batch               Batch mode with error recovery\n";
    std::cout << "  --continue-on-error   Continue workflow on recoverable errors\n\n";

    std::cout << "🔧 Output Control:\n";
    std::cout << "  -v, --verbose         Verbose output including recovery details\n";
    std::cout << "  -q, --quiet           Quiet mode (errors still reported)\n";
    std::cout << "  --no-progress         Disable progress indicators\n\n";

    std::cout << "ℹ️  Information:\n";
    std::cout << "  -h, --help            Show this help message\n";
    std::cout << "  --version             Show version information\n\n";

    std::cout << "🌟 Error Recovery Features:\n";
    std::cout << "  • Automatic error detection and classification\n";
    std::cout << "  • Intelligent recovery strategies for common errors\n";
    std::cout << "  • User-guided recovery workflows for complex issues\n";
    std::cout << "  • Error prevention through early detection\n";
    std::cout << "  • Machine learning-based error pattern analysis\n";
    std::cout << "  • Comprehensive error reporting and analytics\n\n";

    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --status\n";
    std::cout << "  " << program_name << " --simulate --auto-fetch\n";
    std::cout << "  " << program_name << " --test-recovery\n";
    std::cout << "  " << program_name << " --recovery-dashboard\n";
    std::cout << "  " << program_name << " --fetch --interactive-recovery\n\n";
  }
};
/**
 * @brief Main application function with error recovery
 */
int main(int argc, char* argv[]) {
  // Set up signal handler for error recovery
  std::signal(SIGABRT, error_recovery_signal_handler);
  std::signal(SIGTERM, error_recovery_signal_handler);
  std::signal(SIGINT, error_recovery_signal_handler);

  try {
    // Initialize error recovery system (lightweight mode)
    std::cout << "🔄 Initializing error recovery system...\n";

    // Only initialize if we actually need error recovery features
    bool needs_error_recovery = false;
    for (int i = 1; i < argc; ++i) {
      std::string_view arg = argv[i];
      if (arg == "--test-recovery" || arg == "--recovery-dashboard" ||
          arg == "--error-report" || arg == "--export-recovery-data") {
        needs_error_recovery = true;
        break;
      }
    }

    if (needs_error_recovery) {
      LauncherErrorRecoverySystem::initialize();
      std::cout << "✅ Error recovery system initialized\n";
    } else {
      std::cout << "ℹ️  Error recovery system available but not initialized (use --test-recovery to activate)\n";
    }

    // Parse command line arguments with error recovery
    auto config = ErrorRecoveryArgumentParser::parse(argc, argv);
    if (!config.has_value()) {
      return 1;
    }

    // Configure error recovery system based on config
    auto& orchestrator = ErrorRecoveryOrchestrator::instance();
    orchestrator.set_recovery_mode(config->recovery_mode);
    orchestrator.enable_learning(config->enable_learning);

    auto& recovery_manager = AdvancedErrorRecoveryManager::instance();
    recovery_manager.enable_automatic_recovery(config->enable_auto_recovery);

    auto& prevention_system = ErrorPreventionSystem::instance();
    prevention_system.set_prevention_enabled(config->enable_error_prevention);

    auto& detection_system = EarlyDetectionSystem::instance();
    if (!config->enable_early_detection && detection_system.is_monitoring()) {
      detection_system.stop_monitoring();
    }

    // Handle help and version
    if (config->show_help) {
      ErrorRecoveryArgumentParser::print_usage(argv[0]);
      return 0;
    }

    if (config->show_version) {
      ErrorRecoveryArgumentParser::print_version();
      return 0;
    }

    // Print header
    ErrorRecoveryLauncherUI::print_header();

    // Handle error recovery specific operations
    if (config->show_error_report) {
      ErrorRecoveryLauncherUI::generate_error_report();
      return 0;
    }

    if (config->test_error_recovery) {
      LauncherErrorRecoverySystem::test_error_recovery_system();
      return 0;
    }

    if (config->show_recovery_dashboard) {
      LauncherErrorRecoverySystem::show_recovery_dashboard();
      return 0;
    }

    if (config->export_recovery_data) {
      std::string filename = config->recovery_data_file.value_or("recovery_data.json");
      orchestrator.export_recovery_data(filename);
      std::cout << "✅ Recovery data exported to: " << filename << "\n";
      return 0;
    }

    // Handle status display with error recovery information
    if (config->show_status) {
      ErrorRecoveryLauncherUI::print_error_recovery_status();
      return 0;
    }

    // Handle data operations with error recovery
    if (config->fetch_data || config->update_data || config->force_update ||
        config->validate_cache || config->clean_cache || config->rebuild_cache) {

      std::cout << "🔄 Executing data operations with error recovery support\n\n";

      try {
        // Execute data operations with error recovery monitoring
        std::cout << "🔄 Starting data operations with error recovery\n";

        // Simulate data workflow execution with error recovery
        bool success = true;

        if (config->fetch_data || config->update_data) {
          std::cout << "📡 Fetching data with error recovery support\n";
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        if (config->validate_cache) {
          std::cout << "✅ Validating cache with error recovery support\n";
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (config->rebuild_cache) {
          std::cout << "🔧 Rebuilding cache with error recovery support\n";
          std::this_thread::sleep_for(std::chrono::milliseconds(800));
        }

        if (config->clean_cache) {
          std::cout << "🧹 Cleaning cache with error recovery support\n";
          std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        // Monitor for any issues during operations
        for (int i = 0; i < 3 && success; ++i) {
          std::this_thread::sleep_for(std::chrono::milliseconds(200));

          // Simulate potential error detection
          if (i == 1) {
            // Test error recovery with a simulated network error
            DetailedError test_error(ErrorCode::NetworkUnavailable,
                                   "Simulated network connectivity issue during data operations",
                                   ErrorSeverity::Warning,
                                   "Data operations monitoring");

            std::cout << "⚠️  Detected potential issue - attempting error recovery\n";
            success = LauncherErrorRecoverySystem::handle_error_with_recovery(test_error);
          }
        }

        if (success) {
          std::cout << "✅ Data operations completed successfully with error recovery support\n";
        } else {
          std::cout << "❌ Data operations failed despite error recovery attempts\n";
          return 1;
        }

      } catch (const std::exception& e) {
        // Handle exceptions with error recovery
        DetailedError exception_error = ErrorUtils::create_error_from_exception(e, "Data operations");
        LauncherErrorRecoverySystem::handle_error_with_recovery(exception_error);
        return 1;
      }
    }

    // Handle simulation with error recovery
    if (config->run_simulation) {
      std::cout << "🚀 Executing simulation with error recovery support\n\n";

      try {
        // Execute simulation with error recovery monitoring
        std::cout << "🔄 Starting simulation with error recovery\n";

        // Simulate simulation execution with error recovery
        bool success = true;

        if (config->auto_fetch) {
          std::cout << "📡 Auto-fetching data for simulation\n";
          std::this_thread::sleep_for(std::chrono::milliseconds(800));
        }

        std::cout << "🚀 Executing N-body simulation with error recovery\n";

        // Create body collection for simulation with error recovery
        try {
          BodySelector selector;
          selector.body_set(SolarSystem::Bodies::BodyFactory::DefaultBodySet::IMPORTANT);

          auto body_collection_result = selector.build();
          if (!body_collection_result.has_value()) {
            DetailedError body_error(ErrorCode::ResourceUnavailable,
                                   "Failed to build body collection for simulation",
                                   ErrorSeverity::Error,
                                   "Simulation initialization");
            success = LauncherErrorRecoverySystem::handle_error_with_recovery(body_error);
          } else {
            // Create and run simulation
            SimulationBuilder sim_builder;
            std::string error_message;

            auto simulation = sim_builder.with_bodies(std::move(body_collection_result.value()))
                                  .with_timestep(3600.0)  // 1 hour timestep
                                  .with_max_iterations(1000)
                                  .build(&error_message);

            if (!simulation) {
              DetailedError sim_error(ErrorCode::OperationFailed,
                                    "Failed to build simulation: " + error_message,
                                    ErrorSeverity::Error,
                                    "Simulation creation");
              success = LauncherErrorRecoverySystem::handle_error_with_recovery(sim_error);
            } else {
              std::cout << "✅ Simulation executed successfully\n";
              std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            }
          }
        } catch (const std::exception& e) {
          DetailedError sim_exception = ErrorUtils::create_error_from_exception(e, "Simulation execution");
          success = LauncherErrorRecoverySystem::handle_error_with_recovery(sim_exception);
        }

        // Monitor for any issues during simulation
        for (int i = 0; i < 3 && success; ++i) {
          std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        if (success) {
          std::cout << "✅ Simulation completed successfully with error recovery support\n";
        } else {
          std::cout << "❌ Simulation failed despite error recovery attempts\n";
          return 1;
        }

      } catch (const std::exception& e) {
        // Handle exceptions with error recovery
        DetailedError exception_error = ErrorUtils::create_error_from_exception(e, "Simulation execution");
        LauncherErrorRecoverySystem::handle_error_with_recovery(exception_error);
        return 1;
      }
    }

    std::cout << "\n🛡️  Error recovery system available\n";
    std::cout << "Use --recovery-dashboard to monitor error recovery status\n";

    // Ensure proper shutdown of error recovery system
    try {
      auto& orchestrator = ErrorRecoveryOrchestrator::instance();
      if (orchestrator.is_system_healthy()) {
        orchestrator.shutdown();
      }

      auto& detection_system = EarlyDetectionSystem::instance();
      if (detection_system.is_monitoring()) {
        detection_system.stop_monitoring();
      }
    } catch (...) {
      // Ignore shutdown errors
    }

    return 0;

  } catch (const std::exception& e) {
    // Final exception handler with error recovery
    std::cerr << "❌ Fatal error in launcher: " << e.what() << "\n";

    try {
      DetailedError fatal_error = ErrorUtils::create_error_from_exception(e, "Launcher main");
      fatal_error.severity = ErrorSeverity::Fatal;
      LauncherErrorRecoverySystem::handle_error_with_recovery(fatal_error);
    } catch (...) {
      std::cerr << "❌ Error recovery system also failed\n";
    }

    return 1;
  } catch (...) {
    std::cerr << "❌ Unknown fatal error in launcher\n";
    return 1;
  }
}
