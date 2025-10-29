/**
 * @file workflow_coordinator.hpp
 * @brief Advanced workflow coordination with transactions and rollback
 *
 * Extends workflow orchestration with:
 * - Transaction-like semantics
 * - Rollback and recovery capabilities
 * - Workflow monitoring and debugging
 * - Distributed workflow execution
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/utils/expected.hpp"

namespace SolarSystem::Workflow {

/**
 * @brief Workflow transaction state
 */
enum class TransactionState {
  NOT_STARTED,
  IN_PROGRESS,
  COMMITTED,
  ROLLED_BACK,
  FAILED
};

/**
 * @brief Workflow step result
 */
struct StepResult {
  bool success = false;
  std::string step_id;
  std::string output;
  std::chrono::milliseconds duration{0};
  std::optional<std::string> error_message;
};

/**
 * @brief Workflow transaction
 */
class WorkflowTransaction {
 public:
  using StepFunction = std::function<StepResult()>;
  using RollbackFunction = std::function<void()>;

  WorkflowTransaction(std::string id);
  ~WorkflowTransaction() = default;

  /**
   * @brief Add step to transaction
   */
  void add_step(const std::string& step_id, StepFunction step_func,
               RollbackFunction rollback_func = nullptr);

  /**
   * @brief Execute transaction
   */
  [[nodiscard]] SolarSystem::Utils::Expected<void, std::string> execute();

  /**
   * @brief Rollback transaction
   */
  void rollback();

  /**
   * @brief Get transaction state
   */
  [[nodiscard]] TransactionState get_state() const { return state_; }

  /**
   * @brief Get transaction ID
   */
  [[nodiscard]] std::string get_id() const { return id_; }

  /**
   * @brief Get executed steps
   */
  [[nodiscard]] std::vector<StepResult> get_executed_steps() const { return executed_steps_; }

  /**
   * @brief Get current step
   */
  [[nodiscard]] std::optional<std::string> get_current_step() const;

 private:
  std::string id_;
  TransactionState state_ = TransactionState::NOT_STARTED;
  std::vector<std::pair<std::string, StepFunction>> steps_;
  std::vector<RollbackFunction> rollback_functions_;
  std::vector<StepResult> executed_steps_;
};

/**
 * @brief Workflow coordinator with transaction support
 */
class WorkflowCoordinator {
 public:
  WorkflowCoordinator();
  ~WorkflowCoordinator();

  // Non-copyable, movable
  WorkflowCoordinator(const WorkflowCoordinator&) = delete;
  WorkflowCoordinator& operator=(const WorkflowCoordinator&) = delete;
  WorkflowCoordinator(WorkflowCoordinator&&) noexcept;
  WorkflowCoordinator& operator=(WorkflowCoordinator&&) noexcept;

  /**
   * @brief Create new transaction
   */
  [[nodiscard]] std::shared_ptr<WorkflowTransaction> create_transaction(const std::string& id);

  /**
   * @brief Execute transaction
   */
  [[nodiscard]] SolarSystem::Utils::Expected<void, std::string> execute_transaction(
      const std::string& transaction_id);

  /**
   * @brief Rollback transaction
   */
  void rollback_transaction(const std::string& transaction_id);

  /**
   * @brief Get transaction
   */
  [[nodiscard]] std::shared_ptr<WorkflowTransaction> get_transaction(
      const std::string& transaction_id);

  /**
   * @brief Get all transactions
   */
  [[nodiscard]] std::vector<std::shared_ptr<WorkflowTransaction>> get_all_transactions() const;

  /**
   * @brief Monitor transaction progress
   */
  using ProgressCallback = std::function<void(const std::string&, const StepResult&)>;
  void set_progress_callback(ProgressCallback callback);

  /**
   * @brief Enable debugging
   */
  void enable_debugging(bool enabled);

  /**
   * @brief Get workflow statistics
   */
  struct WorkflowStats {
    size_t total_transactions = 0;
    size_t successful_transactions = 0;
    size_t failed_transactions = 0;
    size_t rolled_back_transactions = 0;
    std::chrono::milliseconds avg_execution_time{0};
  };

  [[nodiscard]] WorkflowStats get_statistics() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Distributed workflow executor
 */
class DistributedWorkflowExecutor {
 public:
  DistributedWorkflowExecutor();
  ~DistributedWorkflowExecutor();

  /**
   * @brief Execute workflow across multiple nodes
   */
  [[nodiscard]] SolarSystem::Utils::Expected<void, std::string> execute_distributed(
      std::shared_ptr<WorkflowTransaction> transaction,
      const std::vector<std::string>& node_ids);

  /**
   * @brief Check node availability
   */
  [[nodiscard]] bool is_node_available(const std::string& node_id) const;

  /**
   * @brief Get available nodes
   */
  [[nodiscard]] std::vector<std::string> get_available_nodes() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Workflow
