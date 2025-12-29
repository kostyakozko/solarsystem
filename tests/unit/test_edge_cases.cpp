/**
 * @file test_edge_cases.cpp
 * @brief Comprehensive edge case and boundary testing for Solar System Suite
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_utils/argument_parser.hpp"
#include <limits>
#include <thread>
#include <vector>

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Simulation;
using namespace SolarSystem::Math;
using namespace SolarSystem::Utils;
  // ===== Numeric Boundary Tests =====

TEST(EdgeCaseandBoundaryTests, Vector3d___Zero_values) {
    Vector3d zero(0.0, 0.0, 0.0);
    ASSERT_EQ(0.0, zero.x());
    ASSERT_EQ(0.0, zero.y());
    ASSERT_EQ(0.0, zero.z());
    ASSERT_EQ(0.0, zero.magnitude());
    ASSERT_TRUE(zero.is_zero());
}

TEST(EdgeCaseandBoundaryTests, Vector3d___Very_large_values) {
    double large = 1e100;
    Vector3d v(large, large, large);
    ASSERT_EQ(large, v.x());
    ASSERT_TRUE(v.magnitude() > 0.0);
}

TEST(EdgeCaseandBoundaryTests, Vector3d___Very_small_values) {
    double small = 1e-100;
    Vector3d v(small, small, small);
    ASSERT_EQ(small, v.x());
    ASSERT_TRUE(v.magnitude() > 0.0);
}

TEST(EdgeCaseandBoundaryTests, Vector3d___Negative_values) {
    Vector3d v(-1.0, -2.0, -3.0);
    ASSERT_EQ(-1.0, v.x());
    ASSERT_EQ(-2.0, v.y());
    ASSERT_EQ(-3.0, v.z());
}

TEST(EdgeCaseandBoundaryTests, CelestialBody___Minimum_mass) {
    CelestialBody::Properties props{
        .name = "Tiny",
        .mass = 1e-10,  // Very small but positive
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Asteroid,
        .priority = BodyPriority::Optional,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    CelestialBody body(props);
    ASSERT_TRUE(body.mass() > 0.0);
}

TEST(EdgeCaseandBoundaryTests, CelestialBody___Maximum_mass) {
    CelestialBody::Properties props{
        .name = "Massive",
        .mass = 1e50,  // Very large mass
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Star,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    CelestialBody body(props);
    ASSERT_EQ(1e50, body.mass());
}

TEST(EdgeCaseandBoundaryTests, CelestialBody___Extreme_positions) {
    CelestialBody::Properties props{
        .name = "Distant",
        .mass = 1e24,
        .position = Vector3d{1e15, 1e15, 1e15},  // Very far away
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Optional,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    CelestialBody body(props);
    ASSERT_TRUE(body.position().magnitude() > 1e15);
}

  // ===== Null and Empty Input Tests =====

TEST(EdgeCaseandBoundaryTests, CelestialBody___Empty_name_rejection) {
    CelestialBody::Properties props{
        .name = "",
        .mass = 1.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    bool exception_thrown = false;
    try {
      CelestialBody body(props);
    } catch (const std::invalid_argument&) {
      exception_thrown = true;
    }
    ASSERT_TRUE(exception_thrown);
}

TEST(EdgeCaseandBoundaryTests, CelestialBody___Whitespace_only_name) {
    CelestialBody::Properties props{
        .name = "   ",
        .mass = 1.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    // Implementation may or may not reject whitespace-only names
    // Just verify it doesn't crash
    try {
      CelestialBody body(props);
      // If it accepts it, that's okay
    } catch (const std::invalid_argument&) {
      // If it rejects it, that's also okay
    }
}

TEST(EdgeCaseandBoundaryTests, BodyCollection___Empty_collection) {
    BodyCollection collection;
    ASSERT_TRUE(collection.empty());
    ASSERT_EQ(0, collection.size());
    ASSERT_EQ(0.0, collection.total_mass());
}

TEST(EdgeCaseandBoundaryTests, BodyFactory___Empty_body_name) {
    BodyFactory factory;
    auto result = factory.create_body("");
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, BodyFactory___Null_like_body_name) {
    BodyFactory factory;
    auto result = factory.create_body("   ");
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, SimulationEngine___Empty_body_collection) {
    SimulationEngine engine;
    BodyCollection empty;

    auto result = engine.initialize(std::move(empty));
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, Date___Empty_string) {
    auto result = Date::from_string("");
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, Date___Whitespace_string) {
    auto result = Date::from_string("   ");
    ASSERT_FALSE(result.has_value());
}

  // ===== Boundary Value Tests =====

TEST(EdgeCaseandBoundaryTests, Date___Minimum_valid_date) {
    auto result = Date::from_string("1970-01-01");
    ASSERT_TRUE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, Date___Maximum_month) {
    auto result = Date::from_string("2025-12-31");
    ASSERT_TRUE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, Date___Invalid_month_13) {
    auto result = Date::from_string("2025-13-01");
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, Date___Invalid_month_0) {
    auto result = Date::from_string("2025-00-01");
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, Date___Invalid_day_32) {
    auto result = Date::from_string("2025-01-32");
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, Date___Invalid_day_0) {
    auto result = Date::from_string("2025-01-00");
    ASSERT_FALSE(result.has_value());
}

TEST(EdgeCaseandBoundaryTests, BodyCollection___Single_body) {
    BodyCollection collection;
    collection.add_body(CelestialBody::Properties{
        .name = "Single",
        .mass = 1.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()});

    ASSERT_EQ(1, collection.size());
}

TEST(EdgeCaseandBoundaryTests, BodyCollection___Many_bodies) {
    BodyCollection collection;

    for (int i = 0; i < 100; ++i) {
      collection.add_body(CelestialBody::Properties{
          .name = "Body" + std::to_string(i),
          .mass = 1e24,
          .position = Vector3d{static_cast<double>(i) * 1e11, 0.0, 0.0},
          .velocity = Vector3d{},
          .type = BodyType::Asteroid,
          .priority = BodyPriority::Optional,
          .jpl_id = "",
          .creation_date = std::chrono::system_clock::now()});
    }

    ASSERT_EQ(100, collection.size());
}

  // ===== Resource Limit Tests =====

TEST(EdgeCaseandBoundaryTests, Vector3d___Large_collection) {
    std::vector<Vector3d> vectors;
    vectors.reserve(1000);

    for (int i = 0; i < 1000; ++i) {
      vectors.emplace_back(i * 1.0, i * 2.0, i * 3.0);
    }

    ASSERT_EQ(1000, vectors.size());
}

TEST(EdgeCaseandBoundaryTests, CelestialBody___Long_name) {
    std::string long_name(1000, 'A');

    CelestialBody::Properties props{
        .name = long_name,
        .mass = 1.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    CelestialBody body(props);
    ASSERT_EQ(long_name, body.name());
}

TEST(EdgeCaseandBoundaryTests, CelestialBody___Special_characters_in_name) {
    CelestialBody::Properties props{
        .name = "Test-Body_123!@#",
        .mass = 1.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    CelestialBody body(props);
    ASSERT_EQ("Test-Body_123!@#", body.name());
}

  // ===== Concurrent Access Tests =====

TEST(EdgeCaseandBoundaryTests, BodyCollection___Concurrent_reads) {
    BodyCollection collection;
    collection.add_body(CelestialBody::Properties{
        .name = "Shared",
        .mass = 1.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()});

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 10; ++i) {
      threads.emplace_back([&collection, &success_count]() {
        for (int j = 0; j < 100; ++j) {
          if (collection.contains("Shared")) {
            success_count++;
          }
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    ASSERT_EQ(1000, success_count.load());
}

TEST(EdgeCaseandBoundaryTests, Vector3d___Concurrent_operations) {
    Vector3d v(1.0, 2.0, 3.0);
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 10; ++i) {
      threads.emplace_back([&v, &success_count]() {
        for (int j = 0; j < 100; ++j) {
          double mag = v.magnitude();
          if (mag > 0.0) {
            success_count++;
          }
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    ASSERT_EQ(1000, success_count.load());
}

  // ===== Invalid Input Tests =====

TEST(EdgeCaseandBoundaryTests, CelestialBody___Negative_mass_rejection) {
    CelestialBody::Properties props{
        .name = "Invalid",
        .mass = -1.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    bool exception_thrown = false;
    try {
      CelestialBody body(props);
    } catch (const std::invalid_argument&) {
      exception_thrown = true;
    }
    ASSERT_TRUE(exception_thrown);
}

TEST(EdgeCaseandBoundaryTests, CelestialBody___Zero_mass) {
    CelestialBody::Properties props{
        .name = "ZeroMass",
        .mass = 0.0,
        .position = Vector3d{},
        .velocity = Vector3d{},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential,
        .jpl_id = "",
        .creation_date = std::chrono::system_clock::now()};

    // Implementation may or may not reject zero mass
    // Just verify it doesn't crash
    try {
      CelestialBody body(props);
      // If it accepts it, that's okay (might be valid for massless particles)
    } catch (const std::invalid_argument&) {
      // If it rejects it, that's also okay
    }
}

TEST(EdgeCaseandBoundaryTests, SimulationEngine___Negative_timestep) {
    SolarSystem::Simulation::SimulationConfig config{.time_step = -1.0};
    SimulationEngine engine(config);

    // Should handle gracefully or reject
    // Implementation dependent
}

TEST(EdgeCaseandBoundaryTests, SimulationEngine___Zero_timestep) {
    SolarSystem::Simulation::SimulationConfig config{.time_step = 0.0};
    SimulationEngine engine(config);

    // Should handle gracefully or reject
    // Implementation dependent
}
