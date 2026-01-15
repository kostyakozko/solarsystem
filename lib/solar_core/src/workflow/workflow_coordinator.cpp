/**
 * @file workflow_coordinator.cpp
 * @brief Implementation of workflow coordination with transactions
 */

#include "solar_core/workflow/workflow_coordinator.hpp"

#include <algorithm>
#include <map>
#include <mutex>
#include <future>
#include <atomic>

namespace SolarSystem::Workflow {

// WorkflowTransaction implementation
WorkflowTransaction::WorkflowTransaction(std::string id) : id_(std::move(id)) {}

void WorkflowTransaction::add_step(const std::string& step_id, StepFunction step_func,
                                   RollbackFunction rollback_func) {
  steps_.emplace_back(step_id, std::move(step_func));
  if (rollback_func) {
    rollback_functions_.push_back(std::move(rollback_func));
  } else {
    rollback_functions_.push_back([]() {});  // No-op rollback
  }
}

SolarSystem::Utils::Expected<void, std::string> WorkflowTransaction::execute() {
  if (state_ != TransactionState::NOT_STARTED) {
    return SolarSystem::Utils::Expected<void, std::string>(
        "Transaction already executed or in progress");
  }

  state_ = TransactionState::IN_PROGRESS;
  executed_steps_.clear();

  for (size_t i = 0; i < steps_.size(); ++i) {
    const auto& [step_id, step_func] = steps_[i];

    auto start_time = std::chrono::steady_clock::now();

    try {
      StepResult result = step_func();
      auto end_time = std::chrono::steady_clock::now();
      result.duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

      executed_steps_.push_back(result);

      if (!result.success) {
        // Step failed, rollback
        rollback();
        state_ = TransactionState::FAILED;  // Set FAILED after rollback
        return SolarSystem::Utils::Expected<void, std::string>(
            "Step '" + step_id + "' failed: " +
            result.error_message.value_or("Unknown error"));
      }
    } catch (const std::exception& ex) {
      // Exception during step execution
      rollback();
      state_ = TransactionState::FAILED;  // Set FAILED after rollback
      return SolarSystem::Utils::Expected<void, std::string>(
          "Step '" + step_id + "' threw exception: " + ex.what());
    }
  }

  state_ = TransactionState::COMMITTED;
  return SolarSystem::Utils::Expected<void, std::string>();
}

void WorkflowTransaction::rollback() {
  if (state_ != TransactionState::IN_PROGRESS && state_ != TransactionState::FAILED &&
      state_ != TransactionState::COMMITTED) {
    return;  // Nothing to rollback
  }

  // Execute rollback functions in reverse order
  for (auto it = rollback_functions_.rbegin(); it != rollback_functions_.rend(); ++it) {
    try {
      (*it)();
    } catch (const std::exception&) {
      // Log error but continue rolling back
    }
  }

  state_ = TransactionState::ROLLED_BACK;
}

std::optional<std::string> WorkflowTransaction::get_current_step() const {
  if (state_ != TransactionState::IN_PROGRESS) {
    return std::nullopt;
  }

  if (executed_steps_.size() < steps_.size()) {
    return steps_[executed_steps_.size()].first;
  }

  return std::nullopt;
}

// WorkflowCoordinator implementation
struct WorkflowCoordinator::Impl {
  mutable std::mutex mutex;
  std::map<std::string, std::shared_ptr<WorkflowTransaction>> transactions;
  ProgressCallback progress_callback;
  bool debugging_enabled = false;
  WorkflowStats stats;
};

WorkflowCoordinator::WorkflowCoordinator() : impl_(std::make_unique<Impl>()) {}

WorkflowCoordinator::~WorkflowCoordinator() = default;

WorkflowCoordinator::WorkflowCoordinator(WorkflowCoordinator&&) noexcept = default;

WorkflowCoordinator& WorkflowCoordinator::operator=(WorkflowCoordinator&&) noexcept = default;

std::shared_ptr<WorkflowTransaction> WorkflowCoordinator::create_transaction(
    const std::string& id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto transaction = std::make_shared<WorkflowTransaction>(id);
  impl_->transactions[id] = transaction;
  impl_->stats.total_transactions++;

  return transaction;
}

SolarSystem::Utils::Expected<void, std::string> WorkflowCoordinator::execute_transaction(
    const std::string& transaction_id) {
  std::shared_ptr<WorkflowTransaction> transaction;

  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->transactions.find(transaction_id);
    if (it == impl_->transactions.end()) {
      return SolarSystem::Utils::Expected<void, std::string>("Transaction not found: " +
                                                             transaction_id);
    }
    transaction = it->second;
  }

  // Execute transaction (outside lock to allow progress callbacks)
  auto result = transaction->execute();

  // Update statistics
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (result) {
      impl_->stats.successful_transactions++;
    } else {
      impl_->stats.failed_transactions++;
    }

    // Update average execution time
    auto steps = transaction->get_executed_steps();
    std::chrono::milliseconds total_duration{0};
    for (const auto& step : steps) {
      total_duration += step.duration;
    }

    if (impl_->stats.successful_transactions + impl_->stats.failed_transactions > 0) {
      auto total_count = impl_->stats.successful_transactions + impl_->stats.failed_transactions;
      impl_->stats.avg_execution_time =
          (impl_->stats.avg_execution_time * (total_count - 1) + total_duration) / total_count;
    }
  }

  // Call progress callback for each step
  if (impl_->progress_callback) {
    for (const auto& step : transaction->get_executed_steps()) {
      impl_->progress_callback(transaction_id, step);
    }
  }

  return result;
}

void WorkflowCoordinator::rollback_transaction(const std::string& transaction_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->transactions.find(transaction_id);
  if (it != impl_->transactions.end()) {
    it->second->rollback();
    impl_->stats.rolled_back_transactions++;
  }
}

std::shared_ptr<WorkflowTransaction> WorkflowCoordinator::get_transaction(
    const std::string& transaction_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->transactions.find(transaction_id);
  if (it != impl_->transactions.end()) {
    return it->second;
  }

  return nullptr;
}

std::vector<std::shared_ptr<WorkflowTransaction>> WorkflowCoordinator::get_all_transactions()
    const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  std::vector<std::shared_ptr<WorkflowTransaction>> result;
  result.reserve(impl_->transactions.size());

  for (const auto& [id, transaction] : impl_->transactions) {
    result.push_back(transaction);
  }

  return result;
}

void WorkflowCoordinator::set_progress_callback(ProgressCallback callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->progress_callback = std::move(callback);
}

void WorkflowCoordinator::enable_debugging(bool enabled) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->debugging_enabled = enabled;
}

WorkflowCoordinator::WorkflowStats WorkflowCoordinator::get_statistics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->stats;
}

// DistributedWorkflowExecutor implementation
struct DistributedWorkflowExecutor::Impl {
  mutable std::mutex mutex;
  std::map<std::string, bool> node_availability;  // node_id -> available
};

DistributedWorkflowExecutor::DistributedWorkflowExecutor() : impl_(std::make_unique<Impl>()) {
  // Initialize with local node
  impl_->node_availability["local"] = true;
}

DistributedWorkflowExecutor::~DistributedWorkflowExecutor() = default;

SolarSystem::Utils::Expected<void, std::string> DistributedWorkflowExecutor::execute_distributed(
    std::shared_ptr<WorkflowTransaction> transaction, const std::vector<std::string>& node_ids) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  // Check if all nodes are available
  for (const auto& node_id : node_ids) {
    auto it = impl_->node_availability.find(node_id);
    if (it == impl_->node_availability.end() || !it->second) {
      return SolarSystem::Utils::Expected<void, std::string>("Node not available: " + node_id);
    }
  }

  // Distributed execution implementation
  // Note: This is a basic implementation. Full production deployment would require:
  // - gRPC service definitions for node communication
  // - Raft consensus for coordination
  // - Persistent state management
  // - Network partition handling

  if (node_ids.empty()) {
    // No nodes specified, execute locally
    return transaction->execute();
  }

  // For single-node execution, just execute locally
  if (node_ids.size() == 1) {
    return transaction->execute();
  }

  // Multi-node execution: distribute steps across nodes
  // This is a simplified implementation that demonstrates the pattern

  // Get transaction steps (assuming transaction has a method to get steps)
  // In a real implementation, steps would be serialized using Protocol Buffers

  // Track results from each node
  std::vector<std::future<SolarSystem::Utils::Expected<void, std::string>>> futures;
  std::atomic<size_t> completed_steps{0};
  std::atomic<bool> has_error{false};
  std::string error_message;
  std::mutex error_mutex;

  // Distribute execution across nodes (simulated)
  // In production, this would use gRPC to send serialized steps to remote nodes
  for (size_t i = 0; i < node_ids.size() && !has_error.load(); ++i) {
    const auto& node_id = node_ids[i];

    // Launch async execution for this node
    futures.push_back(std::async(std::launch::async, [&, node_id]() -> SolarSystem::Utils::Expected<void, std::string> {
      // In production: serialize and send via gRPC
      // For now: execute portion of transaction locally

      // Simulate node execution
      auto result = transaction->execute();

      if (result.has_value()) {
        completed_steps++;
      } else {
        has_error.store(true);
        std::lock_guard<std::mutex> err_lock(error_mutex);
        if (error_message.empty()) {
          error_message = "Node " + node_id + " failed: " + result.error();
        }
      }

      return result;
    }));
  }

  // Wait for all nodes to complete
  for (auto& future : futures) {
    future.wait();
  }

  // Check for errors
  if (has_error.load()) {
    return SolarSystem::Utils::Expected<void, std::string>(error_message);
  }

  // All nodes completed successfully
  return SolarSystem::Utils::Expected<void, std::string>();
}

bool DistributedWorkflowExecutor::is_node_available(const std::string& node_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->node_availability.find(node_id);
  return it != impl_->node_availability.end() && it->second;
}

std::vector<std::string> DistributedWorkflowExecutor::get_available_nodes() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  std::vector<std::string> result;
  for (const auto& [node_id, available] : impl_->node_availability) {
    if (available) {
      result.push_back(node_id);
    }
  }

  return result;
}

}  // namespace SolarSystem::Workflow
