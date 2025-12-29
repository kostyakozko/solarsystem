/**
 * @file test_workflow_coordinator.cpp
 * @brief Unit tests for workflow coordination system
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/workflow/workflow_coordinator.hpp"

#include <chrono>
#include <thread>

using namespace SolarSystem::Workflow;
  // Basic transaction tests
TEST(WorkflowCoordinatorTests, Create_and_Execute_Simple_Transaction) {
    WorkflowCoordinator coordinator;

    auto transaction = coordinator.create_transaction("test_tx_1");
    if (!transaction) throw std::runtime_error("Failed to create transaction");

    // Add simple steps
    transaction->add_step(
        "step1",
        []() {
          StepResult result;
          result.success = true;
          result.step_id = "step1";
          result.output = "Step 1 completed";
          return result;
        },
        []() {});

    transaction->add_step(
        "step2",
        []() {
          StepResult result;
          result.success = true;
          result.step_id = "step2";
          result.output = "Step 2 completed";
          return result;
        },
        []() {});

    // Execute transaction
    auto result = coordinator.execute_transaction("test_tx_1");
    if (!result) {
      throw std::runtime_error("Transaction execution failed");
    }

    // Check state
    if (transaction->get_state() != TransactionState::COMMITTED) {
      throw std::runtime_error("Transaction should be committed");
    }

    // Check executed steps
    auto steps = transaction->get_executed_steps();
    if (steps.size() != 2) throw std::runtime_error("Should have 2 executed steps");
    if (!steps[0].success) throw std::runtime_error("Step 1 should succeed");
    if (!steps[1].success) throw std::runtime_error("Step 2 should succeed");
}

TEST(WorkflowCoordinatorTests, Transaction_with_Failing_Step) {
    WorkflowCoordinator coordinator;

    auto transaction = coordinator.create_transaction("test_tx_fail");

    transaction->add_step(
        "step1",
        []() {
          StepResult result;
          result.success = true;
          result.step_id = "step1";
          return result;
        },
        []() {});

    transaction->add_step(
        "step2_fail",
        []() {
          StepResult result;
          result.success = false;
          result.step_id = "step2_fail";
          result.error_message = "Intentional failure";
          return result;
        },
        []() {});

    // Execute transaction (should fail)
    auto result = coordinator.execute_transaction("test_tx_fail");
    if (result) throw std::runtime_error("Transaction should have failed");

    // Check state
    if (transaction->get_state() != TransactionState::FAILED) {
      throw std::runtime_error("Transaction should be in FAILED state");
    }
}

TEST(WorkflowCoordinatorTests, Transaction_Rollback) {
    WorkflowCoordinator coordinator;

    auto transaction = coordinator.create_transaction("test_tx_rollback");

    transaction->add_step(
        "step1",
        []() {
          StepResult result;
          result.success = true;
          result.step_id = "step1";
          return result;
        },
        []() {});

    transaction->add_step(
        "step2",
        []() {
          StepResult result;
          result.success = true;
          result.step_id = "step2";
          return result;
        },
        []() {});

    // Execute transaction
    auto result = coordinator.execute_transaction("test_tx_rollback");
    if (!result) throw std::runtime_error("Transaction should succeed");

    // Manually rollback
    coordinator.rollback_transaction("test_tx_rollback");

    // Check state
    if (transaction->get_state() != TransactionState::ROLLED_BACK) {
      throw std::runtime_error("Transaction should be in ROLLED_BACK state");
    }
}

TEST(WorkflowCoordinatorTests, Get_Transaction_by_ID) {
    WorkflowCoordinator coordinator;

    auto tx1 = coordinator.create_transaction("tx1");
    auto tx2 = coordinator.create_transaction("tx2");

    // Get existing transaction
    auto retrieved = coordinator.get_transaction("tx1");
    if (!retrieved) throw std::runtime_error("Should retrieve transaction");
    if (retrieved->get_id() != "tx1") throw std::runtime_error("Wrong transaction ID");

    // Get non-existent transaction
    auto not_found = coordinator.get_transaction("nonexistent");
    if (not_found) throw std::runtime_error("Should return nullptr for non-existent transaction");
}

TEST(WorkflowCoordinatorTests, Get_All_Transactions) {
    WorkflowCoordinator coordinator;

    auto t1 = coordinator.create_transaction("tx1");
    auto t2 = coordinator.create_transaction("tx2");
    auto t3 = coordinator.create_transaction("tx3");
    (void)t1; (void)t2; (void)t3;  // Suppress unused warnings

    auto all_transactions = coordinator.get_all_transactions();
    if (all_transactions.size() != 3) throw std::runtime_error("Should have 3 transactions");
}

TEST(WorkflowCoordinatorTests, Workflow_Statistics) {
    WorkflowCoordinator coordinator;

    auto stats = coordinator.get_statistics();
    if (stats.total_transactions != 0) throw std::runtime_error("Should start with 0 transactions");

    // Create and execute successful transaction
    auto tx1 = coordinator.create_transaction("tx1");
    tx1->add_step("step1", []() {
      StepResult result;
      result.success = true;
      result.step_id = "step1";
      return result;
    });
    auto r1 = coordinator.execute_transaction("tx1");
    if (!r1) throw std::runtime_error("Transaction should succeed");

    // Create and execute failing transaction
    auto tx2 = coordinator.create_transaction("tx2");
    tx2->add_step("step1", []() {
      StepResult result;
      result.success = false;
      result.step_id = "step1";
      return result;
    });
    auto r2 = coordinator.execute_transaction("tx2");
    if (r2) throw std::runtime_error("Transaction should fail");

    stats = coordinator.get_statistics();
    if (stats.total_transactions != 2) throw std::runtime_error("Should have 2 total transactions");
    if (stats.successful_transactions != 1) {
      throw std::runtime_error("Should have 1 successful transaction");
    }
    if (stats.failed_transactions != 1) throw std::runtime_error("Should have 1 failed transaction");
}

TEST(WorkflowCoordinatorTests, Distributed_Workflow___Local_Node) {
    DistributedWorkflowExecutor executor;

    // Check local node is available
    if (!executor.is_node_available("local")) {
      throw std::runtime_error("Local node should be available");
    }

    auto nodes = executor.get_available_nodes();
    if (nodes.empty()) throw std::runtime_error("Should have at least local node");
    if (nodes[0] != "local") throw std::runtime_error("First node should be local");
}

TEST(WorkflowCoordinatorTests, Distributed_Workflow___Execute_on_Local) {
    DistributedWorkflowExecutor executor;
    WorkflowTransaction transaction("dist_tx");

    transaction.add_step("step1", []() {
      StepResult result;
      result.success = true;
      result.step_id = "step1";
      return result;
    });

    auto tx_ptr = std::make_shared<WorkflowTransaction>(std::move(transaction));
    auto result = executor.execute_distributed(tx_ptr, {"local"});

    if (!result) throw std::runtime_error("Distributed execution should succeed");
}

TEST(WorkflowCoordinatorTests, Step_Duration_Tracking) {
    WorkflowTransaction transaction("test_duration");

    transaction.add_step("slow_step", []() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      StepResult result;
      result.success = true;
      result.step_id = "slow_step";
      return result;
    });

    auto result = transaction.execute();
    if (!result) throw std::runtime_error("Transaction should succeed");

    auto steps = transaction.get_executed_steps();
    if (steps.empty()) throw std::runtime_error("Should have executed steps");

    // Check duration was tracked
    if (steps[0].duration.count() < 50) {
      throw std::runtime_error("Duration should be at least 50ms");
    }
}

TEST(WorkflowCoordinatorTests, Transaction_State_Transitions) {
    WorkflowTransaction transaction("test_states");

    // Initial state
    if (transaction.get_state() != TransactionState::NOT_STARTED) {
      throw std::runtime_error("Initial state should be NOT_STARTED");
    }

    transaction.add_step("step1", []() {
      StepResult result;
      result.success = true;
      result.step_id = "step1";
      return result;
    });

    // Execute
    auto result = transaction.execute();
    if (!result) throw std::runtime_error("Transaction should succeed");

    // Final state
    if (transaction.get_state() != TransactionState::COMMITTED) {
      throw std::runtime_error("Final state should be COMMITTED");
    }

    // Try to execute again (should fail)
    result = transaction.execute();
    if (result) throw std::runtime_error("Should not be able to execute twice");
}

