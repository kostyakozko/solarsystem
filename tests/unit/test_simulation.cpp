/**
 * @file test_simulation.cpp
 * @brief Unit tests for simulation functions (actual API)
 */

#include "../../lib/solar_core/simulation.h"
#include "test_data.h"
#include "test_framework.h"

int main() {
  TEST_SUITE("Simulation Engine Tests");

  // Test simulation initialization
  TEST_CASE("Simulation Initialization") {
    // Initialize simulation to current time
    initialize_simulation_to_current_time();

    // Should not crash and should set a valid time
    time_t sim_time = get_simulation_time();
    ASSERT_GT(sim_time, 0);
  });

  // Test simulation time management
  TEST_CASE("Simulation Time Management") {
    initialize_simulation_to_current_time();

    time_t initial_time = get_simulation_time();

    // Update simulation (should advance time)
    update_simulation_to_current_time();

    time_t updated_time = get_simulation_time();

    // Time should be valid
    ASSERT_GT(updated_time, 0);
    ASSERT_TRUE(std::abs(updated_time - initial_time) < 86400);  // Within 24 hours
  });

  // Test simulation step execution
  TEST_CASE("Simulation Step Execution") {
    initialize_simulation_to_current_time();

    time_t before_step = get_simulation_time();

    // Perform a simulation step
    perform_simulation_step(3600.0);  // 1 hour

    time_t after_step = get_simulation_time();

    // Time should have advanced
    ASSERT_GT(after_step, before_step);

    // Should have advanced by approximately the step size
    long double time_diff = after_step - before_step;
    ASSERT_GT(time_diff, 3500.0);  // At least 58 minutes
    ASSERT_LT(time_diff, 3700.0);  // At most 62 minutes
  });

  // Test multiple simulation steps
  TEST_CASE("Multiple Simulation Steps") {
    initialize_simulation_to_current_time();

    time_t start_time = get_simulation_time();

    // Perform multiple small steps
    for (int i = 0; i < 5; ++i) {
      perform_simulation_step(600.0);  // 10 minutes each
    }

    time_t end_time = get_simulation_time();

    // Should have advanced by approximately 50 minutes
    long double total_diff = end_time - start_time;
    ASSERT_GT(total_diff, 2900.0);  // At least 48 minutes
    ASSERT_LT(total_diff, 3100.0);  // At most 52 minutes
  });

  // Test simulation with different step sizes
  TEST_CASE("Variable Step Sizes") {
    initialize_simulation_to_current_time();

    // Test small step
    time_t before_small = get_simulation_time();
    perform_simulation_step(60.0);  // 1 minute
    time_t after_small = get_simulation_time();

    long double small_diff = after_small - before_small;
    ASSERT_GT(small_diff, 50.0);  // At least 50 seconds
    ASSERT_LT(small_diff, 70.0);  // At most 70 seconds

    // Test large step
    time_t before_large = get_simulation_time();
    perform_simulation_step(86400.0);  // 1 day
    time_t after_large = get_simulation_time();

    long double large_diff = after_large - before_large;
    ASSERT_GT(large_diff, 86300.0);  // At least 23h 58m
    ASSERT_LT(large_diff, 86500.0);  // At most 24h 2m
  });

  return current_suite->all_passed() ? 0 : 1;
}
