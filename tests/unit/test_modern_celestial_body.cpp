/**
 * @file test_modern_celestial_body.cpp
 * @brief Unit tests for modern CelestialBody class
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>

#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/constants.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

// ============================================================================
// Basic Construction Tests
// ============================================================================

TEST(ModernCelestialBody, BasicConstruction) {
  CelestialBody::Properties props{.name = "Earth",
                                  .mass = 5.97219e24,
                                  .position = Vector3d{1.496e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "399",
                                  .creation_date = std::chrono::system_clock::now()};

  CelestialBody earth(props);

  EXPECT_EQ("Earth", earth.name());
  EXPECT_EQ(5.97219e24, earth.mass());
  EXPECT_EQ(to_string(BodyType::Planet), to_string(earth.type()));
  EXPECT_EQ(to_string(BodyPriority::Essential), to_string(earth.priority()));
  ASSERT_TRUE(earth.jpl_id().has_value());
  EXPECT_EQ("399", earth.jpl_id().value());
}

TEST(ModernCelestialBody, InvalidConstructionNegativeMass) {
  CelestialBody::Properties props{.name = "Invalid",
                                  .mass = -1.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()};

  EXPECT_THROW(CelestialBody invalid(props), std::invalid_argument);
}

TEST(ModernCelestialBody, InvalidConstructionEmptyName) {
  CelestialBody::Properties props{.name = "",
                                  .mass = 1.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()};

  EXPECT_THROW(CelestialBody invalid(props), std::invalid_argument);
}

// ============================================================================
// State Modification Tests
// ============================================================================

TEST(ModernCelestialBody, StateModification) {
  CelestialBody::Properties props{.name = "Test",
                                  .mass = 1.0,
                                  .position = Vector3d{0.0, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 0.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()};

  CelestialBody body(props);

  Vector3d new_pos{1.0, 2.0, 3.0};
  Vector3d new_vel{4.0, 5.0, 6.0};

  body.set_position(new_pos);
  body.set_velocity(new_vel);

  EXPECT_TRUE(body.position() == new_pos);
  EXPECT_TRUE(body.velocity() == new_vel);

  // Test combined state setting
  Vector3d newer_pos{7.0, 8.0, 9.0};
  Vector3d newer_vel{10.0, 11.0, 12.0};
  body.set_state(newer_pos, newer_vel);

  EXPECT_TRUE(body.position() == newer_pos);
  EXPECT_TRUE(body.velocity() == newer_vel);
}

// ============================================================================
// Force Application Tests
// ============================================================================

TEST(ModernCelestialBody, ForceApplication) {
  CelestialBody::Properties props{.name = "Test",
                                  .mass = 2.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()};

  CelestialBody body(props);

  Vector3d force{10.0, 0.0, 0.0};
  body.apply_force(force, 1.0);

  // F = ma, so a = F/m = 10/2 = 5
  EXPECT_EQ(5.0, body.acceleration().x());
  EXPECT_EQ(0.0, body.acceleration().y());
  EXPECT_EQ(0.0, body.acceleration().z());
}

TEST(ModernCelestialBody, PositionUpdate) {
  CelestialBody::Properties props{.name = "Test",
                                  .mass = 1.0,
                                  .position = Vector3d{0.0, 0.0, 0.0},
                                  .velocity = Vector3d{1.0, 0.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()};

  CelestialBody body(props);

  // Apply constant force
  Vector3d force{1.0, 0.0, 0.0};
  body.apply_force(force, 1.0);

  // Update position with dt = 1.0
  body.update_position(1.0);

  // v = v0 + a*dt = 1.0 + 1.0*1.0 = 2.0
  // x = x0 + v*dt = 0.0 + 2.0*1.0 = 2.0
  EXPECT_EQ(2.0, body.velocity().x());
  EXPECT_EQ(2.0, body.position().x());

  // Acceleration should be reset after update
  EXPECT_TRUE(body.acceleration().is_zero());
}

// ============================================================================
// Gravitational Force Tests
// ============================================================================

TEST(ModernCelestialBody, GravitationalForceCalculation) {
  // Create two bodies
  CelestialBody::Properties props1{.name = "Body1",
                                   .mass = 1e24,
                                   .position = Vector3d{0.0, 0.0, 0.0},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Planet,
                                   .priority = BodyPriority::Essential,
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::now()};

  CelestialBody::Properties props2{.name = "Body2",
                                   .mass = 1e24,
                                   .position = Vector3d{1e6, 0.0, 0.0},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Planet,
                                   .priority = BodyPriority::Essential,
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::now()};

  CelestialBody body1(props1);
  CelestialBody body2(props2);

  Vector3d force = body1.gravitational_force_to(body2);

  // Force should be in positive x direction
  EXPECT_GT(force.x(), 0.0);
  EXPECT_EQ(0.0, force.y());
  EXPECT_EQ(0.0, force.z());

  // Calculate expected force: F = G * m1 * m2 / r^2
  double expected_magnitude = Constants::G * 1e24 * 1e24 / (1e6 * 1e6);
  EXPECT_NEAR(expected_magnitude, force.magnitude(), 1e-8);
}

// ============================================================================
// Distance Calculation Tests
// ============================================================================

TEST(ModernCelestialBody, DistanceCalculations) {
  CelestialBody::Properties props1{.name = "Body1",
                                   .mass = 1.0,
                                   .position = Vector3d{0.0, 0.0, 0.0},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Planet,
                                   .priority = BodyPriority::Essential,
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::now()};

  CelestialBody::Properties props2{.name = "Body2",
                                   .mass = 1.0,
                                   .position = Vector3d{3.0, 4.0, 0.0},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Planet,
                                   .priority = BodyPriority::Essential,
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::now()};

  CelestialBody body1(props1);
  CelestialBody body2(props2);

  double distance = static_cast<double>(body1.distance_to(body2));
  double distance_sq = static_cast<double>(body1.distance_squared_to(body2));

  EXPECT_EQ(5.0, distance);
  EXPECT_EQ(25.0, distance_sq);
}

// ============================================================================
// Availability Check Tests
// ============================================================================

TEST(ModernCelestialBody, AvailabilityCheck) {
  auto now = std::chrono::system_clock::now();
  auto past = now - std::chrono::hours(24);
  auto future = now + std::chrono::hours(24);

  // Body with no creation date (always available)
  CelestialBody::Properties props1{.name = "AlwaysAvailable",
                                   .mass = 1.0,
                                   .position = Vector3d{},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Planet,
                                   .priority = BodyPriority::Essential,
                                   .jpl_id = "",
                                   .creation_date = std::nullopt};

  CelestialBody always_available(props1);
  EXPECT_TRUE(always_available.is_available_at(past));
  EXPECT_TRUE(always_available.is_available_at(now));
  EXPECT_TRUE(always_available.is_available_at(future));

  // Body with creation date
  CelestialBody::Properties props2{.name = "TimeLimited",
                                   .mass = 1.0,
                                   .position = Vector3d{},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Spacecraft,
                                   .priority = BodyPriority::Optional,
                                   .jpl_id = "",
                                   .creation_date = now};

  CelestialBody time_limited(props2);
  EXPECT_FALSE(time_limited.is_available_at(past));
  EXPECT_TRUE(time_limited.is_available_at(now));
  EXPECT_TRUE(time_limited.is_available_at(future));

  // Edge case: Body with epoch time (should be treated as always available)
  CelestialBody::Properties props3{.name = "EpochBody",
                                   .mass = 1.0,
                                   .position = Vector3d{},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Spacecraft,
                                   .priority = BodyPriority::Optional,
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::time_point{}};

  CelestialBody epoch_body(props3);
  EXPECT_TRUE(epoch_body.is_available_at(past));
  EXPECT_TRUE(epoch_body.is_available_at(now));
  EXPECT_TRUE(epoch_body.is_available_at(future));
}

// ============================================================================
// Equality Comparison Tests
// ============================================================================

TEST(ModernCelestialBody, EqualityComparison) {
  CelestialBody::Properties props1{.name = "Earth",
                                   .mass = 1.0,
                                   .position = Vector3d{},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Planet,
                                   .priority = BodyPriority::Essential,
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::now()};

  CelestialBody::Properties props2{.name = "Earth",
                                   .mass = 2.0,                          // Different mass
                                   .position = Vector3d{1.0, 0.0, 0.0},  // Different position
                                   .velocity = Vector3d{},
                                   .type = BodyType::Moon,               // Different type
                                   .priority = BodyPriority::Important,  // Different priority
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::now()};

  CelestialBody::Properties props3{.name = "Mars",
                                   .mass = 1.0,
                                   .position = Vector3d{},
                                   .velocity = Vector3d{},
                                   .type = BodyType::Planet,
                                   .priority = BodyPriority::Essential,
                                   .jpl_id = "",
                                   .creation_date = std::chrono::system_clock::now()};

  CelestialBody earth1(props1);
  CelestialBody earth2(props2);
  CelestialBody mars(props3);

  // Equality is based on name only
  EXPECT_TRUE(earth1 == earth2);
  EXPECT_FALSE(earth1 == mars);
  EXPECT_TRUE(earth1 != mars);
}

// ============================================================================
// String Representation Tests
// ============================================================================

TEST(ModernCelestialBody, StringRepresentation) {
  CelestialBody::Properties props{.name = "Earth",
                                  .mass = 5.97219e24,
                                  .position = Vector3d{1.496e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "399",
                                  .creation_date = std::chrono::system_clock::now()};

  CelestialBody earth(props);
  std::string str = earth.to_string();

  // Should contain key information
  EXPECT_NE(std::string::npos, str.find("Earth"));
  EXPECT_NE(std::string::npos, str.find("Planet"));
  EXPECT_NE(std::string::npos, str.find("Essential"));
  EXPECT_NE(std::string::npos, str.find("399"));
}

TEST(ModernCelestialBody, EnumStringConversion) {
  EXPECT_EQ("Planet", to_string(BodyType::Planet));
  EXPECT_EQ("Moon", to_string(BodyType::Moon));
  EXPECT_EQ("Spacecraft", to_string(BodyType::Spacecraft));

  EXPECT_EQ("Essential", to_string(BodyPriority::Essential));
  EXPECT_EQ("Important", to_string(BodyPriority::Important));
  EXPECT_EQ("Optional", to_string(BodyPriority::Optional));
}
