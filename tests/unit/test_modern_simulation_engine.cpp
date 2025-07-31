/**
 * @file test_modern_simulation_engine.cpp
 * @brief Unit tests for modern SimulationEngine class
 */

#include <chrono>
#include <cmath>

#include "../utils/test_framework.h"
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

using namespace SolarSystem::Simulation;
using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

int main() {
  TestSuite suite("Modern SimulationEngine Tests");

  suite.run_test("Default Constructor", []() {
    SimulationEngine engine;

    ASSERT_FALSE(engine.is_initialized());
    ASSERT_EQ(0.0, engine.get_current_time());
    ASSERT_EQ("Leapfrog", to_string(engine.get_integration_method()));
  });

  suite.run_test("Configuration", []() {
    SimulationConfig config{.time_step = 60.0,
                            .gravitational_constant = 6.67430e-11,
                            .use_adaptive_timestep = true,
                            .max_timestep = 7200.0};

    SimulationEngine engine(config);

    const auto& retrieved_config = engine.get_config();
    ASSERT_EQ(60.0, retrieved_config.time_step);
    ASSERT_EQ(6.67430e-11, retrieved_config.gravitational_constant);
    ASSERT_TRUE(retrieved_config.use_adaptive_timestep);
    ASSERT_EQ(7200.0, retrieved_config.max_timestep);
  });

  suite.run_test("Integration Method Setting", []() {
    SimulationEngine engine;

    engine.set_integration_method(SimulationEngine::IntegrationMethod::EULER);
    ASSERT_EQ("Euler", to_string(engine.get_integration_method()));

    engine.set_integration_method(SimulationEngine::IntegrationMethod::VERLET);
    ASSERT_EQ("Verlet", to_string(engine.get_integration_method()));
  });

  suite.run_test("Empty Body Collection Initialization", []() {
    SimulationEngine engine;
    BodyCollection empty_collection;

    auto result = engine.initialize(std::move(empty_collection));
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.error().find("empty") != std::string::npos);
  });

  suite.run_test("Simple Two-Body System", []() {
    SimulationEngine engine;

    // Create Earth-Moon system
    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "Earth",
                                              .mass = 5.97219e24,
                                              .position = Vector3d{0.0, 0.0, 0.0},
                                              .velocity = Vector3d{0.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    bodies.add_body(
        CelestialBody::Properties{.name = "Moon",
                                  .mass = 7.342e22,
                                  .position = Vector3d{3.844e8, 0.0, 0.0},  // 384,400 km
                                  .velocity = Vector3d{0.0, 1022.0, 0.0},   // Orbital velocity
                                  .type = BodyType::Moon,
                                  .priority = BodyPriority::Important,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());
    ASSERT_TRUE(engine.is_initialized());
    ASSERT_EQ(2, engine.get_bodies().size());
  });

  suite.run_test("Single Simulation Step", []() {
    SimulationEngine engine;

    // Create simple system
    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "Body1",
                                              .mass = 1e24,
                                              .position = Vector3d{0.0, 0.0, 0.0},
                                              .velocity = Vector3d{1000.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    bodies.add_body(CelestialBody::Properties{.name = "Body2",
                                              .mass = 1e24,
                                              .position = Vector3d{1e9, 0.0, 0.0},
                                              .velocity = Vector3d{-1000.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());

    double initial_time = engine.get_current_time();
    auto step_result = engine.step();

    ASSERT_TRUE(step_result.has_value());
    ASSERT_TRUE(engine.get_current_time() > initial_time);
    ASSERT_EQ(1, engine.get_state().iteration_count);
  });

  suite.run_test("Energy Conservation Check", []() {
    SimulationEngine engine;

    // Create Earth-Moon system for energy conservation test
    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "Earth",
                                              .mass = 5.97219e24,
                                              .position = Vector3d{0.0, 0.0, 0.0},
                                              .velocity = Vector3d{0.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    bodies.add_body(CelestialBody::Properties{.name = "Moon",
                                              .mass = 7.342e22,
                                              .position = Vector3d{3.844e8, 0.0, 0.0},
                                              .velocity = Vector3d{0.0, 1022.0, 0.0},
                                              .type = BodyType::Moon,
                                              .priority = BodyPriority::Important,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());

    double initial_energy = static_cast<double>(engine.get_state().total_energy);

    // Run several steps
    for (int i = 0; i < 10; ++i) {
      auto result = engine.step();
      ASSERT_TRUE(result.has_value());
    }

    double final_energy = static_cast<double>(engine.get_state().total_energy);

    // Energy should be approximately conserved (within 1% for this simple test)
    double energy_change = std::abs(final_energy - initial_energy) / std::abs(initial_energy);
    ASSERT_TRUE(energy_change < 0.01);
  });

  suite.run_test("Simulate to Time", []() {
    SimulationEngine engine;

    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "TestBody",
                                              .mass = 1e24,
                                              .position = Vector3d{0.0, 0.0, 0.0},
                                              .velocity = Vector3d{1000.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    bodies.add_body(CelestialBody::Properties{.name = "TestBody2",
                                              .mass = 1e24,
                                              .position = Vector3d{1e9, 0.0, 0.0},
                                              .velocity = Vector3d{0.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());

    double target_time = 3600.0;  // 1 hour
    auto result = engine.simulate_to_time(target_time);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(engine.get_current_time() >= target_time);
    ASSERT_TRUE(engine.get_state().iteration_count > 0);
  });

  suite.run_test("Simulate Duration", []() {
    SimulationEngine engine;

    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "TestBody",
                                              .mass = 1e24,
                                              .position = Vector3d{0.0, 0.0, 0.0},
                                              .velocity = Vector3d{100.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    bodies.add_body(CelestialBody::Properties{.name = "TestBody2",
                                              .mass = 1e24,
                                              .position = Vector3d{1e8, 0.0, 0.0},
                                              .velocity = Vector3d{0.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());

    double initial_time = engine.get_current_time();
    double duration = 1800.0;  // 30 minutes

    auto result = engine.simulate_duration(duration);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(engine.get_current_time() >= initial_time + duration);
  });

  suite.run_test("Different Integration Methods", []() {
    // Test that different integration methods work
    std::vector<SimulationEngine::IntegrationMethod> methods = {
        SimulationEngine::IntegrationMethod::EULER, SimulationEngine::IntegrationMethod::LEAPFROG,
        SimulationEngine::IntegrationMethod::VERLET};

    for (auto method : methods) {
      SimulationEngine engine;
      engine.set_integration_method(method);

      BodyCollection bodies;
      bodies.add_body(CelestialBody::Properties{.name = "TestBody",
                                                .mass = 1e24,
                                                .position = Vector3d{0.0, 0.0, 0.0},
                                                .velocity = Vector3d{1000.0, 0.0, 0.0},
                                                .type = BodyType::Planet,
                                                .priority = BodyPriority::Essential,
                                                .jpl_id = "",
                                                .creation_date = std::chrono::system_clock::now()});

      bodies.add_body(CelestialBody::Properties{.name = "TestBody2",
                                                .mass = 1e24,
                                                .position = Vector3d{1e8, 0.0, 0.0},
                                                .velocity = Vector3d{0.0, 1000.0, 0.0},
                                                .type = BodyType::Planet,
                                                .priority = BodyPriority::Essential,
                                                .jpl_id = "",
                                                .creation_date = std::chrono::system_clock::now()});

      auto init_result = engine.initialize(std::move(bodies));
      ASSERT_TRUE(init_result.has_value());

      auto step_result = engine.step();
      ASSERT_TRUE(step_result.has_value());
    }
  });

  suite.run_test("Statistics Calculation", []() {
    SimulationEngine engine;

    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "Body1",
                                              .mass = 2e24,
                                              .position = Vector3d{-1e8, 0.0, 0.0},
                                              .velocity = Vector3d{0.0, 1000.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    bodies.add_body(CelestialBody::Properties{.name = "Body2",
                                              .mass = 1e24,
                                              .position = Vector3d{2e8, 0.0, 0.0},
                                              .velocity = Vector3d{0.0, -2000.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());

    const auto& state = engine.get_state();

    // Check that statistics are calculated
    ASSERT_TRUE(state.total_energy != 0.0);
    ASSERT_TRUE(state.kinetic_energy > 0.0);
    ASSERT_TRUE(state.potential_energy < 0.0);  // Gravitational PE is negative

    // Total momentum should be zero (equal and opposite velocities, accounting for mass)
    // Body1: mass=2e24, velocity=(0, 1000, 0) -> momentum = (0, 2e27, 0)
    // Body2: mass=1e24, velocity=(0, -2000, 0) -> momentum = (0, -2e27, 0)
    // Total should be zero
    auto momentum = state.total_momentum;
    ASSERT_TRUE(momentum.magnitude() < 1e20);  // Allow for some numerical precision issues
  });

  suite.run_test("Status Summary", []() {
    SimulationEngine engine;

    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "TestBody",
                                              .mass = 1e24,
                                              .position = Vector3d{0.0, 0.0, 0.0},
                                              .velocity = Vector3d{1000.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());

    std::string status = engine.get_status_summary();

    ASSERT_TRUE(status.find("Initialized: Yes") != std::string::npos);
    ASSERT_TRUE(status.find("Bodies: 1") != std::string::npos);
    ASSERT_TRUE(status.find("Leapfrog") != std::string::npos);
  });

  suite.run_test("Reset Functionality", []() {
    SimulationEngine engine;

    BodyCollection bodies;
    bodies.add_body(CelestialBody::Properties{.name = "TestBody",
                                              .mass = 1e24,
                                              .position = Vector3d{0.0, 0.0, 0.0},
                                              .velocity = Vector3d{1000.0, 0.0, 0.0},
                                              .type = BodyType::Planet,
                                              .priority = BodyPriority::Essential,
                                              .jpl_id = "",
                                              .creation_date = std::chrono::system_clock::now()});

    auto init_result = engine.initialize(std::move(bodies));
    ASSERT_TRUE(init_result.has_value());
    auto step_result = engine.step();
    ASSERT_TRUE(step_result.has_value());

    ASSERT_TRUE(engine.is_initialized());
    ASSERT_TRUE(engine.get_current_time() > 0.0);

    engine.reset();

    ASSERT_FALSE(engine.is_initialized());
    ASSERT_EQ(0.0, engine.get_current_time());
  });

  suite.run_test("Utility Functions", []() {
    // Test integration method string conversion
    ASSERT_EQ("Euler", to_string(SimulationEngine::IntegrationMethod::EULER));
    ASSERT_EQ("Leapfrog", to_string(SimulationEngine::IntegrationMethod::LEAPFROG));
    ASSERT_EQ("Verlet", to_string(SimulationEngine::IntegrationMethod::VERLET));

    ASSERT_EQ("Euler", to_string(integration_method_from_string("Euler")));
    ASSERT_EQ("Leapfrog", to_string(integration_method_from_string("Leapfrog")));
    ASSERT_EQ("Verlet", to_string(integration_method_from_string("Verlet")));
  });

  suite.run_test("Invalid State Handling", []() {
    SimulationEngine engine;

    // Try to step without initialization
    auto result = engine.step();
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.error().find("not initialized") != std::string::npos);

    // Try to simulate without initialization
    auto sim_result = engine.simulate_to_time(100.0);
    ASSERT_FALSE(sim_result.has_value());
    ASSERT_TRUE(sim_result.error().find("not initialized") != std::string::npos);
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
