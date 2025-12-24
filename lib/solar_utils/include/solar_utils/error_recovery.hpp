/**
 * @file error_recovery.hpp
 * @brief Advanced error recovery mechanisms (Task 20)
 *
 * Implements advanced error recovery with:
 * - Automatic error recovery where possible
 * - User-guided error recovery workflows
 * - Error prevention and early detection
 * - Error pattern analysis and learning
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>

#include "error_handling.hpp"
#include "solar_utils/export.hpp"

namespace SolarSystem::Utils {

/**
 * @brief Recovery execution modes
 */
enum class RecoveryMode {
  Automatic,      // Fully automatic recovery
  SemiAutomatic,  // Automatic with user confirmation
  Manual,         // User-guided recovery
  Interactive     // Interactive recovery workflow
};

/**
 * @brief Recovery execution status
 */
enum class RecoveryStatus {
  NotStarted,
  InProgress,
  WaitingForUser,
  Completed,
  Failed,
  Cancelled,
  Timeout
};

/**
 * @brief User interaction types for recovery workflows
 */
enum class UserInteractionType {
  Confirmation,   // Yes/No confirmation
  Selection,      // Select from options
  Input,          // Text input
  FileSelection,  // File selection
  Retry,          // Retry operation
  Skip,           // Skip this step
  Abort           // Abort recovery
};

/**
 * @brief User interaction request
 */
struct UserInteractionRequest {
  std::string id;
  UserInteractionType type;
  std::string title;
  std::string message;
  std::vector<std::string> options;
  std::string default_value;
  bool is_required = true;
  std::chrono::seconds timeout{30};
  std::function<bool(const std::string&)> validator;
};

/**
 * @brief User interaction response
 */
struct UserInteractionResponse {
  std::string request_id;
  bool cancelled = false;
  bool timeout = false;
  std::string selected_option;
  std::string user_input;
  std::chrono::system_clock::time_point response_time;
};

/**
 * @brief Recovery workflow step
 */
struct RecoveryStep {
  std::string step_id;
  std::string description;
  std::function<bool()> action;
  std::function<bool()> validation;
  std::vector<UserInteractionRequest> user_interactions;
  RecoveryStrategy fallback_strategy = RecoveryStrategy::None;
  std::chrono::seconds timeout{60};
  size_t max_retries = 3;
  bool is_critical = false;
  std::unordered_set<std::string> dependencies;
};

/**
 * @brief Recovery workflow definition
 */
struct RecoveryWorkflow {
  std::string workflow_id;
  std::string name;
  std::string description;
  std::vector<RecoveryStep> steps;
  RecoveryMode mode = RecoveryMode::Automatic;
  std::chrono::seconds total_timeout{300};  // 5 minutes
  std::function<void(const std::string&)> progress_callback;
  std::function<void(const DetailedError&)> error_callback;
};

/**
 * @brief Recovery execution context
 */
struct RecoveryContext {
  std::string execution_id;
  std::string workflow_id;
  DetailedError original_error;
  RecoveryStatus status = RecoveryStatus::NotStarted;
  std::chrono::system_clock::time_point start_time;
  std::chrono::system_clock::time_point end_time;
  std::vector<std::string> completed_steps;
  std::string current_step;
  std::unordered_map<std::string, std::string> context_data;
  std::vector<DetailedError> recovery_errors;
  std::string failure_reason;
};

/**
 * @brief Error prevention rule
 */
struct PreventionRule {
  std::string rule_id;
  std::string name;
  std::string description;
  std::function<bool(const DetailedError&)> condition;
  std::function<bool()> prevention_action;
  ErrorCategory target_category = ErrorCategory::Unknown;
  std::vector<ErrorCode> target_codes;
  double confidence_threshold = 0.7;
  bool is_enabled = true;
  size_t trigger_count = 0;
  size_t success_count = 0;
};

/**
 * @brief Early detection monitor
 */
struct EarlyDetectionMonitor {
  std::string monitor_id;
  std::string name;
  std::string description;
  std::function<double()> health_check;
  std::function<std::vector<DetailedError>()> diagnostic;
  std::chrono::seconds check_interval{30};
  double warning_threshold = 0.3;
  double critical_threshold = 0.1;
  bool is_active = true;
  std::chrono::system_clock::time_point last_check;
  double last_health_score = 1.0;
};

/**
 * @brief Learning data point for pattern analysis
 */
struct LearningDataPoint {
  DetailedError error;
  RecoveryAction attempted_recovery;
  bool recovery_successful;
  std::chrono::milliseconds recovery_time;
  std::unordered_map<std::string, std::string> context_factors;
  std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Advanced error recovery manager
 */
class SOLAR_UTILS_API AdvancedErrorRecoveryManager {
 public:
  static AdvancedErrorRecoveryManager& instance();

  // Workflow management
  void register_recovery_workflow(const RecoveryWorkflow& workflow);
  void unregister_recovery_workflow(const std::string& workflow_id);
  std::vector<std::string> get_available_workflows() const;
  RecoveryWorkflow get_workflow(const std::string& workflow_id) const;

  // Recovery execution
  std::future<bool> execute_recovery_async(const DetailedError& error,
                                           const std::string& workflow_id = "",
                                           RecoveryMode mode = RecoveryMode::Automatic);
  bool execute_recovery_sync(const DetailedError& error, const std::string& workflow_id = "",
                             RecoveryMode mode = RecoveryMode::Automatic);

  // User interaction handling
  void set_user_interaction_handler(
      std::function<UserInteractionResponse(const UserInteractionRequest&)> handler);
  UserInteractionResponse request_user_interaction(const UserInteractionRequest& request);

  // Recovery monitoring
  std::vector<RecoveryContext> get_active_recoveries() const;
  RecoveryContext get_recovery_status(const std::string& execution_id) const;
  void cancel_recovery(const std::string& execution_id);

  // Automatic recovery configuration
  void enable_automatic_recovery(bool enabled = true);
  void set_automatic_recovery_timeout(std::chrono::seconds timeout);
  void set_max_concurrent_recoveries(size_t max_concurrent);

  // Recovery statistics and learning
  void record_recovery_outcome(const DetailedError& error, const RecoveryAction& action,
                               bool successful, std::chrono::milliseconds duration);
  std::vector<LearningDataPoint> get_learning_data() const;
  void update_recovery_strategies_from_learning();

 private:
  AdvancedErrorRecoveryManager() = default;
  ~AdvancedErrorRecoveryManager();

  // Disable copy and move
  AdvancedErrorRecoveryManager(const AdvancedErrorRecoveryManager&) = delete;
  AdvancedErrorRecoveryManager& operator=(const AdvancedErrorRecoveryManager&) = delete;

  mutable std::mutex workflows_mutex_;
  mutable std::mutex recoveries_mutex_;
  mutable std::mutex learning_mutex_;

  std::unordered_map<std::string, RecoveryWorkflow> workflows_;
  std::unordered_map<std::string, RecoveryContext> active_recoveries_;
  std::vector<LearningDataPoint> learning_data_;

  std::function<UserInteractionResponse(const UserInteractionRequest&)> user_interaction_handler_;

  bool automatic_recovery_enabled_ = true;
  std::chrono::seconds automatic_recovery_timeout_{300};
  size_t max_concurrent_recoveries_ = 5;
  size_t max_learning_data_points_ = 10000;

  // Internal methods
  std::string generate_execution_id() const;
  std::string select_best_workflow(const DetailedError& error) const;
  bool execute_workflow(const RecoveryWorkflow& workflow, RecoveryContext& context);
  bool execute_step(const RecoveryStep& step, RecoveryContext& context);
  void cleanup_completed_recoveries();
  void update_workflow_effectiveness();
};

/**
 * @brief Error prevention system
 */
class SOLAR_UTILS_API ErrorPreventionSystem {
 public:
  static ErrorPreventionSystem& instance();

  // Prevention rule management
  void register_prevention_rule(const PreventionRule& rule);
  void unregister_prevention_rule(const std::string& rule_id);
  void enable_prevention_rule(const std::string& rule_id, bool enabled = true);

  // Prevention execution
  bool check_and_prevent_errors(const std::vector<DetailedError>& recent_errors);
  std::vector<DetailedError> get_prevented_errors() const;

  // Rule effectiveness
  void update_rule_effectiveness();
  std::vector<PreventionRule> get_most_effective_rules(size_t count = 10) const;

  // Configuration
  void set_prevention_enabled(bool enabled = true);
  void set_check_interval(std::chrono::seconds interval);

 private:
  ErrorPreventionSystem() = default;
  ~ErrorPreventionSystem();

  // Disable copy and move
  ErrorPreventionSystem(const ErrorPreventionSystem&) = delete;
  ErrorPreventionSystem& operator=(const ErrorPreventionSystem&) = delete;

  mutable std::mutex rules_mutex_;
  std::unordered_map<std::string, PreventionRule> prevention_rules_;
  std::vector<DetailedError> prevented_errors_;

  bool prevention_enabled_ = true;
  std::chrono::seconds check_interval_{10};

  // Internal methods
  bool evaluate_prevention_rule(const PreventionRule& rule,
                                const std::vector<DetailedError>& errors);
  void record_prevention_attempt(const std::string& rule_id, bool successful);
};

/**
 * @brief Early detection system
 */
class SOLAR_UTILS_API EarlyDetectionSystem {
 public:
  static EarlyDetectionSystem& instance();

  // Monitor management
  void register_monitor(const EarlyDetectionMonitor& monitor);
  void unregister_monitor(const std::string& monitor_id);
  void enable_monitor(const std::string& monitor_id, bool enabled = true);

  // Detection execution
  void start_monitoring();
  void stop_monitoring();
  bool is_monitoring() const;

  // Health assessment
  double get_overall_health_score() const;
  std::vector<DetailedError> get_detected_issues() const;
  std::unordered_map<std::string, double> get_monitor_health_scores() const;

  // Configuration
  void set_monitoring_interval(std::chrono::seconds interval);
  void set_health_thresholds(double warning_threshold, double critical_threshold);

 private:
  EarlyDetectionSystem() = default;
  ~EarlyDetectionSystem();

  // Disable copy and move
  EarlyDetectionSystem(const EarlyDetectionSystem&) = delete;
  EarlyDetectionSystem& operator=(const EarlyDetectionSystem&) = delete;

  mutable std::mutex monitors_mutex_;
  std::unordered_map<std::string, EarlyDetectionMonitor> monitors_;
  std::vector<DetailedError> detected_issues_;

  std::atomic<bool> monitoring_active_{false};
  std::unique_ptr<std::thread> monitoring_thread_;
  std::chrono::seconds monitoring_interval_{30};
  double warning_threshold_ = 0.3;
  double critical_threshold_ = 0.1;

  // Internal methods
  void monitoring_loop();
  void check_monitor(EarlyDetectionMonitor& monitor);
  void process_monitor_results(const std::string& monitor_id, double health_score,
                               const std::vector<DetailedError>& issues);
};

/**
 * @brief Machine learning-based error pattern analyzer
 */
class SOLAR_UTILS_API MLErrorPatternAnalyzer {
 public:
  static MLErrorPatternAnalyzer& instance();

  // Learning and prediction
  void train_model(const std::vector<LearningDataPoint>& training_data);
  std::vector<std::pair<ErrorCode, double>> predict_likely_errors(
      const std::vector<DetailedError>& recent_errors,
      const std::unordered_map<std::string, std::string>& context) const;

  // Pattern analysis
  std::vector<ErrorPattern> discover_new_patterns(const std::vector<DetailedError>& errors);
  void update_pattern_confidence(const std::string& pattern_id, bool prediction_correct);

  // Model management
  void save_model(const std::string& file_path) const;
  void load_model(const std::string& file_path);
  void reset_model();

  // Statistics
  double get_prediction_accuracy() const;
  size_t get_training_data_size() const;
  std::chrono::system_clock::time_point get_last_training_time() const;

 private:
  MLErrorPatternAnalyzer() = default;
  ~MLErrorPatternAnalyzer() = default;

  // Disable copy and move
  MLErrorPatternAnalyzer(const MLErrorPatternAnalyzer&) = delete;
  MLErrorPatternAnalyzer& operator=(const MLErrorPatternAnalyzer&) = delete;

  mutable std::mutex model_mutex_;
  std::vector<LearningDataPoint> training_data_;
  std::unordered_map<std::string, double> pattern_weights_;
  std::chrono::system_clock::time_point last_training_time_;

  size_t correct_predictions_ = 0;
  size_t total_predictions_ = 0;

  // Internal methods
  std::vector<double> extract_features(
      const std::vector<DetailedError>& errors,
      const std::unordered_map<std::string, std::string>& context) const;
  double calculate_error_probability(const std::vector<double>& features,
                                     ErrorCode error_code) const;
  void update_weights(const std::vector<LearningDataPoint>& data);
};

/**
 * @brief Comprehensive error recovery orchestrator
 */
class SOLAR_UTILS_API ErrorRecoveryOrchestrator {
 public:
  static ErrorRecoveryOrchestrator& instance();

  // System initialization
  void initialize();
  void shutdown();

  // Main recovery interface
  bool handle_error(const DetailedError& error);

  // System configuration
  void configure_recovery_system(const std::string& config_file = "");
  void set_recovery_mode(RecoveryMode mode);
  void enable_learning(bool enabled = true);

  // System status
  bool is_system_healthy() const;
  std::string generate_system_report() const;
  void export_recovery_data(const std::string& file_path) const;

 private:
  ErrorRecoveryOrchestrator() = default;
  ~ErrorRecoveryOrchestrator() = default;

  // Disable copy and move
  ErrorRecoveryOrchestrator(const ErrorRecoveryOrchestrator&) = delete;
  ErrorRecoveryOrchestrator& operator=(const ErrorRecoveryOrchestrator&) = delete;

  std::atomic<bool> initialized_{false};
  RecoveryMode default_recovery_mode_ = RecoveryMode::Automatic;
  bool learning_enabled_ = true;

  // Internal methods
  void setup_default_workflows();
  void setup_default_prevention_rules();
  void setup_default_monitors();
  bool attempt_automatic_recovery(const DetailedError& error);
  bool attempt_guided_recovery(const DetailedError& error);
};

/**
 * @brief Utility functions for error recovery
 */
namespace ErrorRecoveryUtils {
// Workflow builders
RecoveryWorkflow create_network_recovery_workflow();
RecoveryWorkflow create_filesystem_recovery_workflow();
RecoveryWorkflow create_memory_recovery_workflow();
RecoveryWorkflow create_configuration_recovery_workflow();

// Prevention rule builders
PreventionRule create_memory_leak_prevention_rule();
PreventionRule create_network_timeout_prevention_rule();
PreventionRule create_disk_space_prevention_rule();

// Monitor builders
EarlyDetectionMonitor create_memory_usage_monitor();
EarlyDetectionMonitor create_network_health_monitor();
EarlyDetectionMonitor create_filesystem_health_monitor();
EarlyDetectionMonitor create_performance_monitor();

// User interaction helpers
UserInteractionRequest create_confirmation_request(const std::string& message);
UserInteractionRequest create_selection_request(const std::string& message,
                                                const std::vector<std::string>& options);
UserInteractionRequest create_input_request(const std::string& message,
                                            const std::string& default_value = "");

// Recovery action builders
std::function<bool()> create_retry_action(std::function<bool()> original_action,
                                          size_t max_retries = 3);
std::function<bool()> create_fallback_action(std::function<bool()> primary_action,
                                             std::function<bool()> fallback_action);
std::function<bool()> create_timeout_action(std::function<bool()> action,
                                            std::chrono::seconds timeout);
}  // namespace ErrorRecoveryUtils

/**
 * @brief Macros for convenient error recovery
 */
#define SOLAR_REGISTER_RECOVERY_WORKFLOW(workflow) \
  SolarSystem::Utils::AdvancedErrorRecoveryManager::instance().register_recovery_workflow(workflow)

#define SOLAR_REGISTER_PREVENTION_RULE(rule) \
  SolarSystem::Utils::ErrorPreventionSystem::instance().register_prevention_rule(rule)

#define SOLAR_REGISTER_DETECTION_MONITOR(monitor) \
  SolarSystem::Utils::EarlyDetectionSystem::instance().register_monitor(monitor)

#define SOLAR_ATTEMPT_AUTO_RECOVERY(error) \
  SolarSystem::Utils::ErrorRecoveryOrchestrator::instance().handle_error(error)

#define SOLAR_AUTO_RECOVERY(error)                                                    \
  SolarSystem::Utils::AdvancedErrorRecoveryManager::instance().execute_recovery_sync( \
      error, "", SolarSystem::Utils::RecoveryMode::Automatic)

}  // namespace SolarSystem::Utils
