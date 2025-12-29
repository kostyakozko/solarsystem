/**
 * @file test_modern_simulation_engine.cpp
 * @brief Unit tests for modern SimulationEngine class
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>

#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

using namespace SolarSystem::Simulation;
using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

// ============================================================================
// Basic Construction Tests
// ============================================================================

TEST(ModernSimulationEngine, DefaultConstructor) {
  SimulationEngine engine;

  EXPECT_FALSE(engine.is_initialized());
  EXPECT_EQ(0.0, engine.get_current_time());
  EXPECT_EQ("Leapfrog", to_string(engine.get_integration_method()));
}

TEST(ModernSimulationEngine, Configuration) {
  SimulationConfig config{.time_step = 60.0,
                          .gravitational_constant = 6.67430e-11,
                          .use_adaptive_timestep = true,
                          .max_timestep = 7200.0};

  SimulationEngine engine(config);

  const auto& retrieved_config = engine.get_config();
  EXPECT_EQ(60.0, retrieved_config.time_step);
  EXPECT_EQ(6.67430e-11, retrieved_config.gravitational_constant);
  EXPECT_TRUE(retrieved_config.use_adaptive_timestep);
  EXPECT_EQ(7200.0, retrieved_config.max_timestep);
}

TEST(ModernSimulationEngine, IntegrationMethodSetting) {
  SimulationEngine engine;

  engine.set_integration_method(SimulationEngine::IntegrationMethod::EULER);
  EXPECT_EQ("Euler", to_string(engine.get_integration_method()));

  engine.set_integration_method(SimulationEngine::IntegrationMethod::VERLET);
  EXPECT_EQ("Verlet", to_string(engine.get_integration_method()));
}

// ============================================================================
// Initialization Tests
// ============================================================================

TEST(ModernSimulationEngine, EmptyBodyCollectionInitialization) {
  SimulationEngine engine;
  BodyCollection empty_collection;

  auto result = engine.initialize(std::move(empty_collection));
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(std::string::npos, result.error().find("empty"));
}

TEST(ModernSimulationEngine, SimpleTwoBodySystem) {
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
  EXPECT_TRUE(engine.is_initialized());
  EXPECT_EQ(2, engine.get_bodies().size());
}

// ============================================================================
// Simulation Step Tests
// ============================================================================

TEST(ModernSimulationEngine, SingleSimulationStep) {
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
  EXPECT_GT(engine.get_current_time(), initial_time);
  EXPECT_EQ(1, engine.get_state().iteration_count);
}

TEST(ModernSimulationEngine, EnergyConservationCheck) {
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
  EXPECT_LT(energy_change, 0.01);
}

// ============================================================================
// Time Simulation Tests
// ============================================================================

TEST(ModernSimulationEngine, SimulateToTime) {
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
  EXPECT_GE(engine.get_current_time(), target_time);
  EXPECT_GT(engine.get_state().iteration_count, 0);
}

TEST(ModernSimulationEngine, SimulateDuration) {
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
  EXPECT_GE(engine.get_current_time(), initial_time + duration);
}

// ============================================================================
// Integration Method Tests
// ============================================================================

TEST(ModernSimulationEngine, DifferentIntegrationMethods) {
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
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST(ModernSimulationEngine, StatisticsCalculation) {
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
  EXPECT_NE(0.0, state.total_energy);
  EXPECT_GT(state.kinetic_energy, 0.0);
  EXPECT_LT(state.potential_energy, 0.0);  // Gravitational PE is negative

  // Total momentum should be zero (equal and opposite velocities, accounting for mass)
  auto momentum = state.total_momentum;
  EXPECT_LT(momentum.magnitude(), 1e20);  // Allow for some numerical precision issues
}

TEST(ModernSimulationEngine, StatusSummary) {
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

  EXPECT_NE(std::string::npos, status.find("Initialized: Yes"));
  EXPECT_NE(std::string::npos, status.find("Bodies: 1"));
  EXPECT_NE(std::string::npos, status.find("Leapfrog"));
}

// ============================================================================
// Reset and Utility Tests
// ============================================================================

TEST(ModernSimulationEngine, ResetFunctionality) {
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

  EXPECT_TRUE(engine.is_initialized());
  EXPECT_GT(engine.get_current_time(), 0.0);

  engine.reset();

  EXPECT_FALSE(engine.is_initialized());
  EXPECT_EQ(0.0, engine.get_current_time());
}

TEST(ModernSimulationEngine, UtilityFunctions) {
  // Test integration method string conversion
  EXPECT_EQ("Euler", to_string(SimulationEngine::IntegrationMethod::EULER));
  EXPECT_EQ("Leapfrog", to_string(SimulationEngine::IntegrationMethod::LEAPFROG));
  EXPECT_EQ("Verlet", to_string(SimulationEngine::IntegrationMethod::VERLET));

  EXPECT_EQ("Euler", to_string(integration_method_from_string("Euler")));
  EXPECT_EQ("Leapfrog", to_string(integration_method_from_string("Leapfrog")));
  EXPECT_EQ("Verlet", to_string(integration_method_from_string("Verlet")));
}

TEST(ModernSimulationEngine, InvalidStateHandling) {
  SimulationEngine engine;

  // Try to step without initialization
  auto result = engine.step();
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(std::string::npos, result.error().find("not initialized"));

  // Try to simulate without initialization
  auto sim_result = engine.simulate_to_time(100.0);
  EXPECT_FALSE(sim_result.has_value());
  EXPECT_NE(std::string::npos, sim_result.error().find("not initialized"));
}
