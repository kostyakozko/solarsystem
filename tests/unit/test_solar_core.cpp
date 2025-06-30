/**
 * @file test_solar_core.cpp
 * @brief Comprehensive unit tests for solar_core library
 *
 * Tests all core simulation functionality including:
 * - Gravitational calculations
 * - N-body simulation
 * - Numerical integration
 * - Physical constants
 * - Data structures
 */

#include "../../lib/solar_core/include/constants.h"
#include "../../lib/solar_core/include/model.h"
#include "../../lib/solar_core/include/simulation.h"
#include "../../lib/solar_core/include/types.h"
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

    // Test coordinate operations
    coord vel = {0.5, 1.5, 2.5};
    coord sum = {pos.x + vel.x, pos.y + vel.y, pos.z + vel.z};
    ASSERT_NEAR(sum.x, 1.5, 1e-10);
    ASSERT_NEAR(sum.y, 3.5, 1e-10);
    ASSERT_NEAR(sum.z, 5.5, 1e-10);
  });

  // Test planet structure
  TEST_CASE("Planet Structure") {
    planet earth;
    earth.name = "Earth";
    earth.mass = 5.972e24;  // kg
    earth.radius = 6371.0;  // km
    earth.position = {0.0, 0.0, 0.0};
    earth.velocity = {0.0, 0.0, 0.0};

    ASSERT_EQ(earth.name, "Earth");
    ASSERT_NEAR(earth.mass, 5.972e24, 1e20);
    ASSERT_NEAR(earth.radius, 6371.0, 1e-6);
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

  // Test gravitational force calculation
  TEST_CASE("Gravitational Force Calculation") {
    planet sun, earth;

    // Sun at origin
    sun.mass = SUN_MASS;
    sun.position = {0.0, 0.0, 0.0};

    // Earth at 1 AU
    earth.mass = EARTH_MASS;
    earth.position = {AU / 1000.0, 0.0, 0.0};  // Convert to km

    // Calculate gravitational force
    coord force = calculate_gravitational_force(earth, sun);

    // Expected force magnitude: F = GMm/r²
    double expected_magnitude = G * SUN_MASS * EARTH_MASS / (AU * AU);
    double actual_magnitude = sqrt(force.x * force.x + force.y * force.y + force.z * force.z);

    // Convert units properly (force should be in Newtons)
    ASSERT_NEAR(actual_magnitude, expected_magnitude, expected_magnitude * 0.01);  // 1% tolerance

    // Force should point toward the Sun (negative x direction)
    ASSERT_LT(force.x, 0.0);
    ASSERT_NEAR(force.y, 0.0, 1e10);
    ASSERT_NEAR(force.z, 0.0, 1e10);
  });

  // Test distance calculation
  TEST_CASE("Distance Calculation") {
    coord pos1 = {0.0, 0.0, 0.0};
    coord pos2 = {3.0, 4.0, 0.0};

    double distance = calculate_distance(pos1, pos2);
    ASSERT_NEAR(distance, 5.0, 1e-10);  // 3-4-5 triangle

    // Test with 3D coordinates
    coord pos3 = {1.0, 2.0, 2.0};
    coord pos4 = {4.0, 6.0, 2.0};
    double distance_3d = calculate_distance(pos3, pos4);
    ASSERT_NEAR(distance_3d, 5.0, 1e-10);  // sqrt((4-1)² + (6-2)² + (2-2)²) = sqrt(9+16+0) = 5
  });

  // Test system initialization
  TEST_CASE("System Initialization") {
    std::vector<planet> system;

    // Create a simple two-body system
    planet sun, earth;
    sun.name = "Sun";
    sun.mass = SUN_MASS;
    sun.position = {0.0, 0.0, 0.0};
    sun.velocity = {0.0, 0.0, 0.0};

    earth.name = "Earth";
    earth.mass = EARTH_MASS;
    earth.position = {AU / 1000.0, 0.0, 0.0};  // 1 AU in km
    earth.velocity = {0.0, 29.78, 0.0};        // Approximate orbital velocity in km/s

    system.push_back(sun);
    system.push_back(earth);

    ASSERT_EQ(system.size(), 2);
    ASSERT_EQ(system[0].name, "Sun");
    ASSERT_EQ(system[1].name, "Earth");
  });

  // Test numerical integration step
  TEST_CASE("Numerical Integration Step") {
    std::vector<planet> system;

    // Create Earth-Moon system for testing
    planet earth, moon;
    earth.name = "Earth";
    earth.mass = EARTH_MASS;
    earth.position = {0.0, 0.0, 0.0};
    earth.velocity = {0.0, 0.0, 0.0};

    moon.name = "Moon";
    moon.mass = 7.342e22;                  // kg
    moon.position = {384400.0, 0.0, 0.0};  // km
    moon.velocity = {0.0, 1.022, 0.0};     // km/s (approximate)

    system.push_back(earth);
    system.push_back(moon);

    // Store initial positions
    coord initial_moon_pos = moon.position;

    // Perform one integration step
    double dt = 3600.0;  // 1 hour in seconds
    integrate_system(system, dt);

    // Moon should have moved
    ASSERT_NE(system[1].position.x, initial_moon_pos.x);
    ASSERT_NE(system[1].position.y, initial_moon_pos.y);

    // Positions should be reasonable (not NaN or infinite)
    ASSERT_TRUE(std::isfinite(system[1].position.x));
    ASSERT_TRUE(std::isfinite(system[1].position.y));
    ASSERT_TRUE(std::isfinite(system[1].position.z));
  });

  // Test energy conservation (approximate)
  TEST_CASE("Energy Conservation") {
    std::vector<planet> system;

    // Simple two-body system
    planet body1, body2;
    body1.mass = 1e24;
    body1.position = {-1000.0, 0.0, 0.0};
    body1.velocity = {0.0, -0.5, 0.0};

    body2.mass = 1e24;
    body2.position = {1000.0, 0.0, 0.0};
    body2.velocity = {0.0, 0.5, 0.0};

    system.push_back(body1);
    system.push_back(body2);

    // Calculate initial energy
    double initial_energy = calculate_total_energy(system);

    // Integrate for several steps
    double dt = 100.0;  // seconds
    for (int i = 0; i < 100; ++i) {
      integrate_system(system, dt);
    }

    // Calculate final energy
    double final_energy = calculate_total_energy(system);

    // Energy should be approximately conserved (within numerical precision)
    double energy_change_percent =
        std::abs(final_energy - initial_energy) / std::abs(initial_energy) * 100.0;
    ASSERT_LT(energy_change_percent, 5.0);  // Allow 5% change due to numerical errors
  });

  // Test momentum conservation
  TEST_CASE("Momentum Conservation") {
    std::vector<planet> system;

    // Create system with zero initial momentum
    planet body1, body2;
    body1.mass = 2e24;
    body1.position = {-500.0, 0.0, 0.0};
    body1.velocity = {1.0, 0.0, 0.0};

    body2.mass = 1e24;
    body2.position = {1000.0, 0.0, 0.0};
    body2.velocity = {-2.0, 0.0, 0.0};  // Momentum: 2e24 * 1 + 1e24 * (-2) = 0

    system.push_back(body1);
    system.push_back(body2);

    // Calculate initial momentum
    coord initial_momentum = calculate_total_momentum(system);

    // Integrate for several steps
    double dt = 100.0;
    for (int i = 0; i < 50; ++i) {
      integrate_system(system, dt);
    }

    // Calculate final momentum
    coord final_momentum = calculate_total_momentum(system);

    // Momentum should be conserved
    ASSERT_NEAR(final_momentum.x, initial_momentum.x, 1e20);  // Allow for numerical precision
    ASSERT_NEAR(final_momentum.y, initial_momentum.y, 1e20);
    ASSERT_NEAR(final_momentum.z, initial_momentum.z, 1e20);
  });

  // Test simulation stability
  TEST_CASE("Simulation Stability") {
    std::vector<planet> system;

    // Create a stable circular orbit system
    planet central, orbiter;
    central.mass = 1e30;  // Large central mass
    central.position = {0.0, 0.0, 0.0};
    central.velocity = {0.0, 0.0, 0.0};

    double orbit_radius = 1e8;                                                            // km
    double orbital_velocity = sqrt(G * central.mass / (orbit_radius * 1000.0)) / 1000.0;  // km/s

    orbiter.mass = 1e20;  // Small orbiting mass
    orbiter.position = {orbit_radius, 0.0, 0.0};
    orbiter.velocity = {0.0, orbital_velocity, 0.0};

    system.push_back(central);
    system.push_back(orbiter);

    // Integrate for one orbital period
    double orbital_period =
        2 * M_PI * sqrt(pow(orbit_radius * 1000.0, 3) / (G * central.mass));  // seconds
    double dt = orbital_period / 1000.0;  // 1000 steps per orbit

    for (int i = 0; i < 1000; ++i) {
      integrate_system(system, dt);

      // Check that positions remain finite
      ASSERT_TRUE(std::isfinite(system[1].position.x));
      ASSERT_TRUE(std::isfinite(system[1].position.y));
      ASSERT_TRUE(std::isfinite(system[1].position.z));

      // Check that the orbiter doesn't fly away or crash
      double distance = calculate_distance(system[0].position, system[1].position);
      ASSERT_GT(distance, orbit_radius * 0.5);  // Not too close
      ASSERT_LT(distance, orbit_radius * 2.0);  // Not too far
    }
  });

  return current_suite->all_passed() ? 0 : 1;
}
