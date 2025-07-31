/**
 * @file test_modern_celestial_body.cpp
 * @brief Unit tests for modern CelestialBody class
 */

#include <chrono>

#include "../utils/test_framework.h"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/constants.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

int main() {
  TestSuite suite("Modern CelestialBody Tests");

  suite.run_test("Basic Construction", []() {
    CelestialBody::Properties props{.name = "Earth",
                                    .mass = 5.97219e24,
                                    .position = Vector3d{1.496e11, 0.0, 0.0},
                                    .velocity = Vector3d{0.0, 29780.0, 0.0},
                                    .type = BodyType::Planet,
                                    .priority = BodyPriority::Essential,
                                    .jpl_id = "399",
                                    .creation_date = std::chrono::system_clock::now()};

    CelestialBody earth(props);

    ASSERT_EQ("Earth", earth.name());
    ASSERT_EQ(5.97219e24, earth.mass());
    ASSERT_EQ(to_string(BodyType::Planet), to_string(earth.type()));
    ASSERT_EQ(to_string(BodyPriority::Essential), to_string(earth.priority()));
    ASSERT_TRUE(earth.jpl_id().has_value());
    ASSERT_EQ("399", earth.jpl_id().value());
  });

  suite.run_test("Invalid Construction - Negative Mass", []() {
    CelestialBody::Properties props{.name = "Invalid",
                                    .mass = -1.0,
                                    .position = Vector3d{},
                                    .velocity = Vector3d{},
                                    .type = BodyType::Planet,
                                    .priority = BodyPriority::Essential,
                                    .jpl_id = "",
                                    .creation_date = std::chrono::system_clock::now()};

    bool exception_thrown = false;
    try {
      CelestialBody invalid(props);
    } catch (const std::invalid_argument&) {
      exception_thrown = true;
    }

    ASSERT_TRUE(exception_thrown);
  });

  suite.run_test("Invalid Construction - Empty Name", []() {
    CelestialBody::Properties props{.name = "",
                                    .mass = 1.0,
                                    .position = Vector3d{},
                                    .velocity = Vector3d{},
                                    .type = BodyType::Planet,
                                    .priority = BodyPriority::Essential,
                                    .jpl_id = "",
                                    .creation_date = std::chrono::system_clock::now()};

    bool exception_thrown = false;
    try {
      CelestialBody invalid(props);
    } catch (const std::invalid_argument&) {
      exception_thrown = true;
    }

    ASSERT_TRUE(exception_thrown);
  });

  suite.run_test("State Modification", []() {
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

    ASSERT_TRUE(body.position() == new_pos);
    ASSERT_TRUE(body.velocity() == new_vel);

    // Test combined state setting
    Vector3d newer_pos{7.0, 8.0, 9.0};
    Vector3d newer_vel{10.0, 11.0, 12.0};
    body.set_state(newer_pos, newer_vel);

    ASSERT_TRUE(body.position() == newer_pos);
    ASSERT_TRUE(body.velocity() == newer_vel);
  });

  suite.run_test("Force Application", []() {
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
    ASSERT_EQ(5.0, body.acceleration().x());
    ASSERT_EQ(0.0, body.acceleration().y());
    ASSERT_EQ(0.0, body.acceleration().z());
  });

  suite.run_test("Position Update", []() {
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
    ASSERT_EQ(2.0, body.velocity().x());
    ASSERT_EQ(2.0, body.position().x());

    // Acceleration should be reset after update
    ASSERT_TRUE(body.acceleration().is_zero());
  });

  suite.run_test("Gravitational Force Calculation", []() {
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
    ASSERT_TRUE(force.x() > 0.0);
    ASSERT_EQ(0.0, force.y());
    ASSERT_EQ(0.0, force.z());

    // Calculate expected force: F = G * m1 * m2 / r^2
    double expected_magnitude = Constants::G * 1e24 * 1e24 / (1e6 * 1e6);
    ASSERT_TRUE(std::abs(force.magnitude() - expected_magnitude) < 1e-10);
  });

  suite.run_test("Distance Calculations", []() {
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

    ASSERT_EQ(5.0, distance);
    ASSERT_EQ(25.0, distance_sq);
  });

  suite.run_test("Availability Check", []() {
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
                                     .creation_date = std::chrono::system_clock::now()};

    CelestialBody always_available(props1);
    ASSERT_TRUE(always_available.is_available_at(past));
    ASSERT_TRUE(always_available.is_available_at(now));
    ASSERT_TRUE(always_available.is_available_at(future));

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
    ASSERT_FALSE(time_limited.is_available_at(past));
    ASSERT_TRUE(time_limited.is_available_at(now));
    ASSERT_TRUE(time_limited.is_available_at(future));
  });

  suite.run_test("Equality Comparison", []() {
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
    ASSERT_TRUE(earth1 == earth2);
    ASSERT_FALSE(earth1 == mars);
    ASSERT_TRUE(earth1 != mars);
  });

  suite.run_test("String Representation", []() {
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
    ASSERT_TRUE(str.find("Earth") != std::string::npos);
    ASSERT_TRUE(str.find("Planet") != std::string::npos);
    ASSERT_TRUE(str.find("Essential") != std::string::npos);
    ASSERT_TRUE(str.find("399") != std::string::npos);
  });

  suite.run_test("Enum String Conversion", []() {
    ASSERT_EQ("Planet", to_string(BodyType::Planet));
    ASSERT_EQ("Moon", to_string(BodyType::Moon));
    ASSERT_EQ("Spacecraft", to_string(BodyType::Spacecraft));

    ASSERT_EQ("Essential", to_string(BodyPriority::Essential));
    ASSERT_EQ("Important", to_string(BodyPriority::Important));
    ASSERT_EQ("Optional", to_string(BodyPriority::Optional));
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
