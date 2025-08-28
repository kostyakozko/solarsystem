/**
 * @file workflow_orchestration.hpp
 * @brief Comprehensive workflow orchestration system (Task 1)
 *
 * Implements advanced workflow orchestration with:
 * - Component coordination and communication
 * - Workflow progress tracking and reporting
 * - Comprehensive error handling and recovery
 * - JPL connectivity issue resolution
 * - Workflow definition and execution engine
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "solar_utils/error_handling.hpp"
#include "solar_utils/error_recovery.hpp"
#include "solar_utils/logging.hpp"

namespace SolarSystem::Utils::Workflow {

/**
 * @brief Workflow execution status
 */
enum class WorkflowStatus {
  NotStarted,
  Initializing,
  Running,
  Paused,
  Completed,
  Failed,
  Cancelled,
  Timeout
};

/**
 * @brief Component types in the Solar System Suite
 */
enum class ComponentType {
  Launcher,
  Fetch,
  Simulation,
  Realtime,
  WebServer,
  JPLClient,
  CacheManager,
  DataValidator
};

/**
 * @brief Component health status
 */
enum class ComponentHealth {
  Healthy,
  Warning,
  Critical,
  Failed,
  Unknown
};

/**
 * @brief Workflow step execution priority
 */
enum class StepPriority {
  Low = 0,
  Normal = 1,
  High = 2,
  Critical = 3
};

/**
 * @brief Progress information for workflow steps
 */
struct ProgressInfo {
  std::string step_id;
  std::string description;
  double completion_percentage = 0.0;
  std::chrono::system_clock::time_point start_time;
  std::chrono::system_clock::time_point estimated_completion;
  std::optional<std::string> current_operation;
  std::unordered_map<std::string, std::string> metadata;

  ProgressInfo() : start_time(std::chrono::system_clock::now()) {}
};

/**
 * @brief Component status information
 */
struct ComponentStatus {
  ComponentType type;
  std::string name;
  ComponentHealth health = ComponentHealth::Unknown;
  std::string status_message;
  std::chrono::system_clock::time_point last_check;
  std::unordered_map<std::string, std::string> metrics;
  std::vector<DetailedError> recent_errors;
  double health_score = 0.0;

  ComponentStatus(ComponentType type, const std::string& name)
      : type(type), name(name), last_check(std::chrono::system_clock::now()) {}
};

/**
 * @brief Workflow step definition
 */
struct WorkflowStep {
  std::string id;
  std::string name;
  std::string description;
  StepPriority priority = StepPriority::Normal;
  std::vector<std::string> dependencies;
  std::vector<ComponentType> required_components;
  std::chrono::seconds timeout{300};  // 5 minutes default
  size_t max_retries = 3;
  std::chrono::milliseconds retry_delay{1000};
  bool allow_failure = false;
  bool is_critical = false;

  // Execution function
  std::function<bool(const ProgressInfo&, std::function<void(const ProgressInfo&)>)> execute;

  // Validation function (optional)
  std::function<bool()> validate;

  // Cleanup function (optional)
  std::function<void()> cleanup;

  // Recovery function (optional)
  std::function<bool(const DetailedError&)> recover;

  WorkflowStep(const std::string& id, const std::string& name)
      : id(id), name(name) {}
};

/**
 * @brief Workflow definition
 */
struct WorkflowDefinition {
  std::string id;
  std::string name;
  std::string description;
  std::vector<WorkflowStep> steps;
  std::chrono::seconds total_timeout{1800};  // 30 minutes default
  bool continue_on_error = false;
  bool enable_parallel_execution = false;
  size_t max_parallel_steps = 3;

  // Workflow-level callbacks
  std::function<void(const std::string&)> on_progress;
  std::function<void(const DetailedError&)> on_error;
  std::function<void(WorkflowStatus)> on_status_change;

  WorkflowDefinition(const std::string& id, const std::string& name)
      : id(id), name(name) {}
};

/**
 * @brief Workflow execution context
 */
struct WorkflowContext {
  std::string execution_id;
  std::string workflow_id;
  WorkflowStatus status = WorkflowStatus::NotStarted;
  std::chrono::system_clock::time_point start_time;
  std::chrono::system_clock::time_point end_time;
  std::vector<std::string> completed_steps;
  std::string current_step;
  std::unordered_map<std::string, std::string> variables;
  std::vector<DetailedError> errors;
  std::vector<ProgressInfo> progress_history;
  double overall_progress = 0.0;
  std::optional<std::string> failure_reason;

  WorkflowContext(const std::string& execution_id, const std::string& workflow_id)
      : execution_id(execution_id), workflow_id(workflow_id),
        start_time(std::chrono::system_clock::now()) {}
};

/**
 * @brief JPL connectivity manager for handling JPL-specific issues
 */
class JPLConnectivityManager {
 public:
  /**
   * @brief Check JPL service availability
   */
  [[nodiscard]] bool is_jpl_service_available() const;

  /**
   * @brief Test JPL connectivity with diagnostics
   */
  [[nodiscard]] ComponentStatus test_jpl_connectivity();

  /**
   * @brief Attempt to resolve JPL connectivity issues
   */
  [[nodiscard]] bool resolve_jpl_connectivity_issues();

  /**
   * @brief Get JPL service health metrics
   */
  [[nodiscard]] std::unordered_map<std::string, std::string> get_jpl_health_metrics() const;

  /**
   * @brief Enable/disable JPL fallback mode
   */
  void set_fallback_mode(bool enabled);

  /**
   * @brief Check if fallback mode is active
   */
  [[nodiscard]] bool is_fallback_mode_active() const;

 private:
  mutable std::mutex jpl_mutex_;
  std::atomic<bool> fallback_mode_active_{false};
  std::chrono::system_clock::time_point last_connectivity_check_;
  std::chrono::seconds connectivity_check_interval_{60};  // 1 minute

  [[nodiscard]] bool test_jpl_endpoint(const std::string& endpoint) const;
  [[nodiscard]] bool validate_jpl_response(const std::string& response) const;
};

/**
 * @brief Component coordinator for managing suite components
 */
class ComponentCoordinator {
 public:
  /**
   * @brief Register a component for monitoring
   */
  void register_component(ComponentType type, const std::string& name,
                          std::function<ComponentStatus()> health_check);

  /**
   * @brief Unregister a component
   */
  void unregister_component(ComponentType type, const std::string& name);

  /**
   * @brief Get status of all components
   */
  [[nodiscard]] std::vector<ComponentStatus> get_all_component_status() const;

  /**
   * @brief Get status of specific component
   */
  [[nodiscard]] std::optional<ComponentStatus> get_component_status(
      ComponentType type, const std::string& name) const;

  /**
   * @brief Check if required components are available
   */
  [[nodiscard]] bool are_components_available(
      const std::vector<ComponentType>& required_components) const;

  /**
   * @brief Start component health monitoring
   */
  void start_monitoring();

  /**
   * @brief Stop component health monitoring
   */
  void stop_monitoring();

  /**
   * @brief Get overall system health score
   */
  [[nodiscard]] double get_system_health_score() const;

 private:
  struct ComponentInfo {
    ComponentType type;
    std::string name;
    std::function<ComponentStatus()> health_check;
    ComponentStatus last_status;
    std::chrono::system_clock::time_point last_check;

    ComponentInfo() : type(ComponentType::Launcher), last_status(ComponentType::Launcher, ""),
                      last_check(std::chrono::system_clock::now()) {}
  };

  mutable std::mutex components_mutex_;
  std::unordered_map<std::string, ComponentInfo> components_;
  std::atomic<bool> monitoring_active_{false};
  std::unique_ptr<std::thread> monitoring_thread_;
  std::chrono::seconds monitoring_interval_{30};  // 30 seconds

  void monitoring_loop();
  std::string get_component_key(ComponentType type, const std::string& name) const;
};

/**
 * @brief Workflow progress tracker
 */
class ProgressTracker {
 public:
  /**
   * @brief Start tracking progress for a workflow
   */
  void start_tracking(const std::string& execution_id, const WorkflowDefinition& workflow);

  /**
   * @brief Update step progress
   */
  void update_step_progress(const std::string& execution_id, const ProgressInfo& progress);

  /**
   * @brief Complete a step
   */
  void complete_step(const std::string& execution_id, const std::string& step_id);

  /**
   * @brief Fail a step
   */
  void fail_step(const std::string& execution_id, const std::string& step_id,
                 const DetailedError& error);

  /**
   * @brief Get current progress for a workflow
   */
  [[nodiscard]] std::optional<ProgressInfo> get_current_progress(
      const std::string& execution_id) const;

  /**
   * @brief Get overall workflow progress percentage
   */
  [[nodiscard]] double get_overall_progress(const std::string& execution_id) const;

  /**
   * @brief Get progress history
   */
  [[nodiscard]] std::vector<ProgressInfo> get_progress_history(
      const std::string& execution_id) const;

  /**
   * @brief Stop tracking progress
   */
  void stop_tracking(const std::string& execution_id);

 private:
  struct TrackingInfo {
    WorkflowDefinition workflow;
    std::vector<ProgressInfo> progress_history;
    std::unordered_map<std::string, bool> completed_steps;
    std::chrono::system_clock::time_point start_time;

    TrackingInfo() : workflow("", ""), start_time(std::chrono::system_clock::now()) {}
  };

  mutable std::mutex tracking_mutex_;
  std::unordered_map<std::string, TrackingInfo> tracking_data_;
};

/**
 * @brief Workflow execution engine
 */
class WorkflowExecutionEngine {
 public:
  /**
   * @brief Constructor
   */
  WorkflowExecutionEngine();

  /**
   * @brief Destructor
   */
  ~WorkflowExecutionEngine();

  /**
   * @brief Execute workflow synchronously
   */
  [[nodiscard]] bool execute_workflow_sync(const WorkflowDefinition& workflow,
                                           WorkflowContext& context);

  /**
   * @brief Execute workflow asynchronously
   */
  [[nodiscard]] std::future<bool> execute_workflow_async(const WorkflowDefinition& workflow,
                                                         std::shared_ptr<WorkflowContext> context);

  /**
   * @brief Cancel workflow execution
   */
  void cancel_workflow(const std::string& execution_id);

  /**
   * @brief Pause workflow execution
   */
  void pause_workflow(const std::string& execution_id);

  /**
   * @brief Resume workflow execution
   */
  void resume_workflow(const std::string& execution_id);

  /**
   * @brief Get active workflow contexts
   */
  [[nodiscard]] std::vector<std::shared_ptr<WorkflowContext>> get_active_workflows() const;

  /**
   * @brief Get workflow context by execution ID
   */
  [[nodiscard]] std::shared_ptr<WorkflowContext> get_workflow_context(
      const std::string& execution_id) const;

 private:
  mutable std::mutex execution_mutex_;
  std::unordered_map<std::string, std::shared_ptr<WorkflowContext>> active_workflows_;
  std::unordered_map<std::string, std::atomic<bool>> cancellation_flags_;
  std::unordered_map<std::string, std::atomic<bool>> pause_flags_;

  bool execute_step(const WorkflowStep& step, WorkflowContext& context);
  bool validate_step_dependencies(const WorkflowStep& step, const WorkflowContext& context);
  void update_workflow_status(WorkflowContext& context, WorkflowStatus status);
  std::string generate_execution_id() const;
};

/**
 * @brief Comprehensive workflow orchestrator
 */
class WorkflowOrchestrator {
 public:
  /**
   * @brief Get singleton instance
   */
  static WorkflowOrchestrator& instance();

  /**
   * @brief Initialize the orchestrator
   */
  void initialize();

  /**
   * @brief Shutdown the orchestrator
   */
  void shutdown();

  /**
   * @brief Register a workflow definition
   */
  void register_workflow(const WorkflowDefinition& workflow);

  /**
   * @brief Unregister a workflow definition
   */
  void unregister_workflow(const std::string& workflow_id);

  /**
   * @brief Execute a registered workflow
   */
  [[nodiscard]] std::future<bool> execute_workflow(const std::string& workflow_id,
                                                   const std::unordered_map<std::string, std::string>& variables = {});

  /**
   * @brief Execute a custom workflow
   */
  [[nodiscard]] std::future<bool> execute_custom_workflow(const WorkflowDefinition& workflow,
                                                          const std::unordered_map<std::string, std::string>& variables = {});

  /**
   * @brief Get workflow execution status
   */
  [[nodiscard]] std::optional<WorkflowStatus> get_workflow_status(
      const std::string& execution_id) const;

  /**
   * @brief Get workflow progress
   */
  [[nodiscard]] double get_workflow_progress(const std::string& execution_id) const;

  /**
   * @brief Cancel workflow execution
   */
  void cancel_workflow(const std::string& execution_id);

  /**
   * @brief Get component coordinator
   */
  [[nodiscard]] ComponentCoordinator& get_component_coordinator() { return component_coordinator_; }

  /**
   * @brief Get JPL connectivity manager
   */
  [[nodiscard]] JPLConnectivityManager& get_jpl_connectivity_manager() { return jpl_manager_; }

  /**
   * @brief Get progress tracker
   */
  [[nodiscard]] ProgressTracker& get_progress_tracker() { return progress_tracker_; }

  /**
   * @brief Get system health report
   */
  [[nodiscard]] std::string generate_system_health_report() const;

  /**
   * @brief Check if system is healthy
   */
  [[nodiscard]] bool is_system_healthy() const;

 private:
  WorkflowOrchestrator() = default;
  ~WorkflowOrchestrator() = default;

  // Disable copy and move
  WorkflowOrchestrator(const WorkflowOrchestrator&) = delete;
  WorkflowOrchestrator& operator=(const WorkflowOrchestrator&) = delete;

  std::atomic<bool> initialized_{false};
  mutable std::mutex workflows_mutex_;
  std::unordered_map<std::string, WorkflowDefinition> registered_workflows_;

  // Core components
  WorkflowExecutionEngine execution_engine_;
  ComponentCoordinator component_coordinator_;
  JPLConnectivityManager jpl_manager_;
  ProgressTracker progress_tracker_;

  void setup_default_workflows();
  void setup_component_monitoring();
};

/**
 * @brief Workflow builder for creating common workflows
 */
class WorkflowBuilder {
 public:
  /**
   * @brief Create JPL data management workflow
   */
  static WorkflowDefinition create_jpl_data_workflow();

  /**
   * @brief Create simulation workflow
   */
  static WorkflowDefinition create_simulation_workflow();

  /**
   * @brief Create complete system workflow
   */
  static WorkflowDefinition create_complete_workflow();

  /**
   * @brief Create JPL connectivity recovery workflow
   */
  static WorkflowDefinition create_jpl_recovery_workflow();

  /**
   * @brief Create cache management workflow
   */
  static WorkflowDefinition create_cache_management_workflow();

 private:
  static WorkflowStep create_jpl_connectivity_check_step();
  static WorkflowStep create_jpl_data_fetch_step();
  static WorkflowStep create_cache_validation_step();
  static WorkflowStep create_cache_rebuild_step();
  static WorkflowStep create_simulation_execution_step();
  static WorkflowStep create_error_recovery_step();
};

/**
 * @brief Utility functions for workflow orchestration
 */
namespace Utils {

/**
 * @brief Convert workflow status to string
 */
[[nodiscard]] std::string to_string(WorkflowStatus status);

/**
 * @brief Convert component type to string
 */
[[nodiscard]] std::string to_string(ComponentType type);

/**
 * @brief Convert component health to string
 */
[[nodiscard]] std::string to_string(ComponentHealth health);

/**
 * @brief Convert step priority to string
 */
[[nodiscard]] std::string to_string(StepPriority priority);

/**
 * @brief Create progress callback function
 */
[[nodiscard]] std::function<void(const ProgressInfo&)> create_progress_callback(
    const std::string& execution_id);

/**
 * @brief Create error callback function
 */
[[nodiscard]] std::function<void(const DetailedError&)> create_error_callback(
    const std::string& execution_id);

/**
 * @brief Validate workflow definition
 */
[[nodiscard]] ValidationResult validate_workflow_definition(const WorkflowDefinition& workflow);

/**
 * @brief Calculate estimated completion time
 */
[[nodiscard]] std::chrono::system_clock::time_point calculate_estimated_completion(
    const WorkflowDefinition& workflow, double current_progress);

}  // namespace Utils

/**
 * @brief Macros for convenient workflow operations
 */
#define WORKFLOW_REGISTER(workflow) \
  SolarSystem::Utils::Workflow::WorkflowOrchestrator::instance().register_workflow(workflow)

#define WORKFLOW_EXECUTE(workflow_id) \
  SolarSystem::Utils::Workflow::WorkflowOrchestrator::instance().execute_workflow(workflow_id)

#define WORKFLOW_CANCEL(execution_id) \
  SolarSystem::Utils::Workflow::WorkflowOrchestrator::instance().cancel_workflow(execution_id)

#define WORKFLOW_STATUS(execution_id) \
  SolarSystem::Utils::Workflow::WorkflowOrchestrator::instance().get_workflow_status(execution_id)

#define WORKFLOW_PROGRESS(execution_id) \
  SolarSystem::Utils::Workflow::WorkflowOrchestrator::instance().get_workflow_progress(execution_id)

}  // namespace SolarSystem::Utils::Workflow

