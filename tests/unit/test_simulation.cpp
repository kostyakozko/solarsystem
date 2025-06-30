/**
 * @file test_simulation.cpp
 * @brief Unit tests for simulation functions (actual API)
 */

#include "model.h"
#include "simulation.h"
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

    // Get initial position of a planet
    const planet& test_body = get_body(1);  // Mercury
    coord initial_pos = test_body.position;

    // Perform a simulation step
    perform_simulation_step(3600.0);  // 1 hour

    const planet& test_body_after = get_body(1);
    coord final_pos = test_body_after.position;

    // Position should have changed
    long double distance_moved = dist(initial_pos, final_pos);
    ASSERT_GT(distance_moved, 0.0);  // Should have moved
  });

  // Test multiple simulation steps
  TEST_CASE("Multiple Simulation Steps") {
    initialize_simulation_to_current_time();

    // Get initial position of a fast-moving planet
    const planet& mercury = get_body(1);  // Mercury moves fastest
    coord start_pos = mercury.position;

    // Perform multiple small steps
    for (int i = 0; i < 5; ++i) {
      perform_simulation_step(600.0);  // 10 minutes each
    }

    const planet& mercury_after = get_body(1);
    coord end_pos = mercury_after.position;

    // Should have moved a significant distance
    long double total_distance = dist(start_pos, end_pos);
    ASSERT_GT(total_distance, 1000.0);  // Should move at least 1000 km in 50 minutes
  });

  // Test simulation with different step sizes
  TEST_CASE("Variable Step Sizes") {
    initialize_simulation_to_current_time();

    // Get initial position
    const planet& earth = get_body(3);  // Earth
    coord initial_pos = earth.position;

    // Test small step
    perform_simulation_step(60.0);  // 1 minute
    const planet& earth_small = get_body(3);
    coord pos_after_small = earth_small.position;
    long double small_distance = dist(initial_pos, pos_after_small);

    // Test large step from same starting position
    // Reset to initial state (this is a limitation - we can't easily reset)
    // So let's just test that the function works with different step sizes
    perform_simulation_step(3600.0);  // 1 hour
    const planet& earth_large = get_body(3);
    coord pos_after_large = earth_large.position;
    long double large_distance = dist(pos_after_small, pos_after_large);

    // Both should result in movement
    ASSERT_GT(small_distance, 0.0);
    ASSERT_GT(large_distance, 0.0);
  });

  return current_suite->all_passed() ? 0 : 1;
}
