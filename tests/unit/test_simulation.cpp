/**
 * @file test_simulation.cpp
 * @brief Focused unit tests for simulation engine
 */

#include "../../lib/solar_core/include/simulation.h"
#include "test_data.h"
#include "test_framework.h"

int main() {
  TEST_SUITE("Simulation Engine Tests");

  // Test simulation initialization
  TEST_CASE("Simulation Initialization") {
    SimulationArgs args;
    args.target_date = "2025-01-01";
    args.time_step = 3600.0;
    args.verbose = false;

    ASSERT_EQ(args.target_date, "2025-01-01");
    ASSERT_NEAR(args.time_step, 3600.0, 1e-6);
    ASSERT_FALSE(args.verbose);
  });

  // Test date parsing and Julian day conversion
  TEST_CASE("Date Conversion") {
    std::string date = "2000-01-01";
    double julian_day = date_to_julian_day(date);

    // J2000.0 epoch should be JD 2451545.0 at noon
    // Our date is at midnight, so it should be 2451544.5
    ASSERT_NEAR(julian_day, 2451544.5, 0.1);

    // Test another known date
    std::string date2 = "2025-01-01";
    double julian_day2 = date_to_julian_day(date2);
    ASSERT_GT(julian_day2, julian_day);  // Should be later
  });

  // Test simulation step execution
  TEST_CASE("Simulation Step") {
    std::vector<planet> system;

    // Create simple two-body system
    planet sun, earth;
    sun.mass = SUN_MASS;
    sun.position = {0.0, 0.0, 0.0};
    sun.velocity = {0.0, 0.0, 0.0};

    earth.mass = EARTH_MASS;
    earth.position = {AU / 1000.0, 0.0, 0.0};  // 1 AU in km
    earth.velocity = {0.0, 29.78, 0.0};        // km/s

    system.push_back(sun);
    system.push_back(earth);

    // Store initial state
    coord initial_earth_pos = earth.position;
    coord initial_earth_vel = earth.velocity;

    // Execute one simulation step
    double dt = 86400.0;  // 1 day in seconds
    bool success = simulate_step(system, dt);

    ASSERT_TRUE(success);

    // Earth should have moved
    ASSERT_NE(system[1].position.x, initial_earth_pos.x);
    ASSERT_NE(system[1].position.y, initial_earth_pos.y);

    // Velocity should have changed due to gravitational acceleration
    ASSERT_NE(system[1].velocity.x, initial_earth_vel.x);
    ASSERT_NE(system[1].velocity.y, initial_earth_vel.y);
  });

  // Test simulation convergence
  TEST_CASE("Simulation Convergence") {
    std::vector<planet> system;

    // Create Earth-Moon system
    auto scenario = TestData::ReferenceData::get_earth_moon_scenario();

    for (const auto& ref_body : scenario.initial_state.bodies) {
      planet body;
      body.name = ref_body.name;
      body.mass = ref_body.mass;
      body.position = ref_body.position;
      body.velocity = ref_body.velocity;
      system.push_back(body);
    }

    // Run simulation for short duration
    double total_time = 86400.0;  // 1 day
    double dt = 3600.0;           // 1 hour steps
    int steps = static_cast<int>(total_time / dt);

    bool success = true;
    for (int i = 0; i < steps && success; ++i) {
      success = simulate_step(system, dt);

      // Check for numerical instabilities
      for (const auto& body : system) {
        ASSERT_TRUE(std::isfinite(body.position.x));
        ASSERT_TRUE(std::isfinite(body.position.y));
        ASSERT_TRUE(std::isfinite(body.position.z));
        ASSERT_TRUE(std::isfinite(body.velocity.x));
        ASSERT_TRUE(std::isfinite(body.velocity.y));
        ASSERT_TRUE(std::isfinite(body.velocity.z));
      }
    }

    ASSERT_TRUE(success);
  });

  // Test different time step sizes
  TEST_CASE("Time Step Sensitivity") {
    std::vector<planet> system1, system2;

    // Create identical systems
    planet sun, earth;
    sun.mass = SUN_MASS;
    sun.position = {0.0, 0.0, 0.0};
    sun.velocity = {0.0, 0.0, 0.0};

    earth.mass = EARTH_MASS;
    earth.position = {AU / 1000.0, 0.0, 0.0};
    earth.velocity = {0.0, 29.78, 0.0};

    system1.push_back(sun);
    system1.push_back(earth);
    system2 = system1;  // Copy

    // Simulate with different time steps
    double total_time = 86400.0;  // 1 day

    // Large time step
    double dt1 = 3600.0;  // 1 hour
    int steps1 = static_cast<int>(total_time / dt1);
    for (int i = 0; i < steps1; ++i) {
      simulate_step(system1, dt1);
    }

    // Small time step
    double dt2 = 1800.0;  // 30 minutes
    int steps2 = static_cast<int>(total_time / dt2);
    for (int i = 0; i < steps2; ++i) {
      simulate_step(system2, dt2);
    }

    // Results should be similar but not identical
    double pos_diff = calculate_distance(system1[1].position, system2[1].position);
    ASSERT_GT(pos_diff, 0.0);     // Should be different
    ASSERT_LT(pos_diff, 1000.0);  // But not too different (within 1000 km)
  });

  // Test simulation with extreme conditions
  TEST_CASE("Extreme Conditions") {
    std::vector<planet> system;

    // Very massive central body
    planet massive_star;
    massive_star.mass = 1e32;  // 50 times solar mass
    massive_star.position = {0.0, 0.0, 0.0};
    massive_star.velocity = {0.0, 0.0, 0.0};

    // Small orbiting body
    planet small_body;
    small_body.mass = 1e20;
    small_body.position = {1e6, 0.0, 0.0};    // 1 million km
    small_body.velocity = {0.0, 100.0, 0.0};  // High velocity

    system.push_back(massive_star);
    system.push_back(small_body);

    // Should handle extreme mass ratios
    double dt = 100.0;  // Small time step for stability
    bool success = simulate_step(system, dt);
    ASSERT_TRUE(success);

    // Check that results are still finite
    ASSERT_TRUE(std::isfinite(system[1].position.x));
    ASSERT_TRUE(std::isfinite(system[1].velocity.x));
  });

  // Test simulation state management
  TEST_CASE("Simulation State") {
    SimulationState state;
    state.current_time = 0.0;
    state.total_steps = 0;
    state.energy_error = 0.0;

    // Initialize with test system
    planet sun;
    sun.mass = SUN_MASS;
    sun.position = {0.0, 0.0, 0.0};
    sun.velocity = {0.0, 0.0, 0.0};
    state.bodies.push_back(sun);

    ASSERT_EQ(state.bodies.size(), 1);
    ASSERT_NEAR(state.current_time, 0.0, 1e-10);
    ASSERT_EQ(state.total_steps, 0);
  });

  return current_suite->all_passed() ? 0 : 1;
}
