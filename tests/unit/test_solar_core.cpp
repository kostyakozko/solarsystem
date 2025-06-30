/**
 * @file test_solar_core.cpp
 * @brief Unit tests for solar_core library (actual API)
 *
 * Tests the real functions available in the solar_core library
 */

#include "../../lib/solar_core/constants.h"
#include "../../lib/solar_core/model.h"
#include "../../lib/solar_core/simulation.h"
#include "../../lib/solar_core/types.h"
#include "test_data.h"
#include "test_framework.h"

int main() {
  TEST_SUITE("Solar Core Library Tests");

  // Test basic data structures
  TEST_CASE("Coordinate Structure") {
    coord pos = {1.0, 2.0, 3.0};
    ASSERT_EQ(pos.x, 1.0);
    ASSERT_EQ(pos.y, 2.0);
    ASSERT_EQ(pos.z, 3.0);
  });

  // Test physical constants
  TEST_CASE("Physical Constants") {
    // Test that constants are reasonable values
    ASSERT_GT(G, 6.6e-11);  // Gravitational constant
    ASSERT_LT(G, 6.7e-11);

    ASSERT_GT(AU, 1.4e11);  // Astronomical Unit
    ASSERT_LT(AU, 1.5e11);

    ASSERT_GT(EARTH_MASS, 5.9e24);
    ASSERT_LT(EARTH_MASS, 6.0e24);

    ASSERT_GT(SUN_MASS, 1.9e30);
    ASSERT_LT(SUN_MASS, 2.0e30);
  });

  // Test distance calculation
  TEST_CASE("Distance Calculation") {
    coord pos1 = {0.0, 0.0, 0.0};
    coord pos2 = {3.0, 4.0, 0.0};

    long double distance = dist(pos1, pos2);
    ASSERT_NEAR(distance, 5.0, 1e-10);  // 3-4-5 triangle

    // Test with 3D coordinates
    coord pos3 = {1.0, 2.0, 2.0};
    coord pos4 = {4.0, 6.0, 2.0};
    long double distance_3d = dist(pos3, pos4);
    ASSERT_NEAR(distance_3d, 5.0, 1e-10);  // sqrt((4-1)² + (6-2)² + (2-2)²) = 5
  });

  // Test body access functions
  TEST_CASE("Body Access Functions") {
    int body_count = get_body_count();
    ASSERT_GT(body_count, 0);  // Should have at least some bodies

    if (body_count > 0) {
      const planet& first_body = get_body(0);
      ASSERT_GT(first_body.mass, 0.0);  // Mass should be positive
      ASSERT_TRUE(std::isfinite(first_body.position.x));
      ASSERT_TRUE(std::isfinite(first_body.position.y));
      ASSERT_TRUE(std::isfinite(first_body.position.z));
    }
  });

  // Test barycenter calculation
  TEST_CASE("Barycenter Calculation") {
    coord barycenter = getBarycenter();

    // Barycenter should be finite
    ASSERT_TRUE(std::isfinite(barycenter.x));
    ASSERT_TRUE(std::isfinite(barycenter.y));
    ASSERT_TRUE(std::isfinite(barycenter.z));

    // For solar system, barycenter should be relatively close to Sun
    // (within a few solar radii)
    long double distance_from_origin = sqrtl(
        barycenter.x * barycenter.x + barycenter.y * barycenter.y + barycenter.z * barycenter.z);
    ASSERT_LT(distance_from_origin, 1e7);  // Within 10,000 km of origin
  });

  // Test gravitational attraction calculation
  TEST_CASE("Gravitational Attraction") {
    // Test the attractTo function indirectly by checking it doesn't crash
    coord test_position = {AU, 0.0, 0.0};  // 1 AU from origin
    acceleration delta = {0.0, 0.0, 0.0};

    // This should calculate attraction to body 0 (presumably the Sun)
    if (get_body_count() > 0) {
      attractTo(test_position, delta, 0);

      // Acceleration should be finite and non-zero
      ASSERT_TRUE(std::isfinite(delta.x));
      ASSERT_TRUE(std::isfinite(delta.y));
      ASSERT_TRUE(std::isfinite(delta.z));

      // For a body at 1 AU from Sun, acceleration should be toward Sun (negative x)
      ASSERT_LT(delta.x, 0.0);

      // Acceleration magnitude should be reasonable (Earth's orbital acceleration)
      long double acc_magnitude = sqrtl(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
      ASSERT_GT(acc_magnitude, 1e-3);  // Should be significant
      ASSERT_LT(acc_magnitude, 1e-1);  // But not too large
    }
  });

  // Test simulation time functions
  TEST_CASE("Simulation Time Functions") {
    // Initialize simulation
    initialize_simulation_to_current_time();

    // Get current simulation time
    time_t sim_time = get_simulation_time();
    ASSERT_GT(sim_time, 0);  // Should be a valid timestamp

    // Time should be reasonable (after year 2000, before year 2100)
    ASSERT_GT(sim_time, 946684800);   // 2000-01-01
    ASSERT_LT(sim_time, 4102444800);  // 2100-01-01
  });

  // Test simulation step (basic functionality)
  TEST_CASE("Simulation Step") {
    // Initialize simulation
    initialize_simulation_to_current_time();

    // Get initial time
    time_t initial_time = get_simulation_time();

    // Perform a small simulation step
    perform_simulation_step(3600.0);  // 1 hour

    // Time should have advanced
    time_t new_time = get_simulation_time();
    ASSERT_GT(new_time, initial_time);

    // Should have advanced by approximately 1 hour
    long double time_diff = new_time - initial_time;
    ASSERT_NEAR(time_diff, 3600.0, 10.0);  // Within 10 seconds tolerance
  });

  return current_suite->all_passed() ? 0 : 1;
}
