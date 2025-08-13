/**
 * @file test_enhanced_body_collection.cpp
 * @brief Unit tests for enhanced BodyCollection functionality (Task 5)
 */

#include <chrono>

#include "../utils/test_framework.h"
#include "solar_core/bodies/body_collection.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

int main() {
  TestSuite suite("Enhanced BodyCollection Tests (Task 5)");

  // Test comprehensive validation
  suite.run_test("Comprehensive Validation", []() {
    BodyCollection collection;

    // Add a valid body
    collection.add_body(
        CelestialBody::Properties{.name = "Earth",
                                  .mass = 5.97219e24,
                                  .position = Vector3d{1.496e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "399",
                                  .creation_date = std::chrono::system_clock::now()});

    ASSERT_TRUE(collection.validate_comprehensive());
    ASSERT_TRUE(collection.get_comprehensive_validation_errors().empty());
  });

  // Test consistency checking
  suite.run_test("Consistency Checking", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Mars",
                                  .mass = 6.41693e23,
                                  .position = Vector3d{2.279e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 24077.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "499",
                                  .creation_date = std::chrono::system_clock::now()});

    auto report = collection.check_consistency();
    ASSERT_TRUE(report.is_consistent);
    ASSERT_TRUE(report.issues.empty());
    ASSERT_EQ(1, report.bodies_checked);
  });

  // Test validated add operation
  suite.run_test("Validated Add Operation", []() {
    BodyCollection collection;

    CelestialBody valid_body{
        CelestialBody::Properties{.name = "Venus",
                                  .mass = 4.86732e24,
                                  .position = Vector3d{1.082e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 35020.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "299",
                                  .creation_date = std::chrono::system_clock::now()}};

    auto result = collection.add_body_validated(valid_body);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(1, result.affected_bodies);
    ASSERT_TRUE(result.error_message.empty());

    // Try to add duplicate
    auto duplicate_result = collection.add_body_validated(valid_body);
    ASSERT_FALSE(duplicate_result.success);
    ASSERT_EQ(0, duplicate_result.affected_bodies);
    ASSERT_FALSE(duplicate_result.error_message.empty());
  });

  // Test safe remove operation
  suite.run_test("Safe Remove Operation", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Mercury",
                                  .mass = 3.30104e23,
                                  .position = Vector3d{5.79e10, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 47362.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "199",
                                  .creation_date = std::chrono::system_clock::now()});

    ASSERT_EQ(1, collection.size());

    auto result = collection.remove_body_safe("Mercury");
    ASSERT_TRUE(result.success);
    ASSERT_EQ(1, result.affected_bodies);
    ASSERT_EQ(0, collection.size());

    // Try to remove non-existent body
    auto not_found_result = collection.remove_body_safe("Jupiter");
    ASSERT_FALSE(not_found_result.success);
    ASSERT_EQ(0, not_found_result.affected_bodies);
    ASSERT_FALSE(not_found_result.error_message.empty());
  });

  // Test update body operation
  suite.run_test("Update Body Operation", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "TestBody",
                                  .mass = 1000.0,
                                  .position = Vector3d{1.0, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 1.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "999",
                                  .creation_date = std::chrono::system_clock::now()});

    CelestialBody updated_body{
        CelestialBody::Properties{.name = "TestBody",
                                  .mass = 2000.0,
                                  .position = Vector3d{2.0, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 2.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "999",
                                  .creation_date = std::chrono::system_clock::now()}};

    auto result = collection.update_body("TestBody", updated_body);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(1, result.affected_bodies);

    auto found = collection.find_body("TestBody");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ(2000.0, found->get().mass());
    ASSERT_EQ(2.0, found->get().position().x());
  });

  // Test name pattern search
  suite.run_test("Name Pattern Search", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Earth",
                                  .mass = 5.97219e24,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Mars",
                                  .mass = 6.41693e23,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Europa",
                                  .mass = 4.8e22,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Moon,
                                  .priority = BodyPriority::Important,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    // Search for bodies containing "ar"
    auto results = collection.search_by_name_pattern("ar");
    ASSERT_EQ(2, results.size());  // Mars and Earth

    // Search for exact match
    auto exact_results = collection.search_by_name_pattern("^Earth$");
    ASSERT_EQ(1, exact_results.size());
    ASSERT_EQ("Earth", exact_results[0].get().name());
  });

  // Test mass range filtering
  suite.run_test("Mass Range Filtering", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "LightBody",
                                  .mass = 1000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Spacecraft,
                                  .priority = BodyPriority::Optional,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "MediumBody",
                                  .mass = 1.0e20,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Asteroid,
                                  .priority = BodyPriority::Optional,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "HeavyBody",
                                  .mass = 1.0e24,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    auto light_bodies = collection.filter_by_mass_range(100.0, 10000.0);
    ASSERT_EQ(1, light_bodies.size());
    ASSERT_EQ("LightBody", light_bodies[0].get().name());

    auto heavy_bodies = collection.filter_by_mass_range(1.0e23, 1.0e25);
    ASSERT_EQ(1, heavy_bodies.size());
    ASSERT_EQ("HeavyBody", heavy_bodies[0].get().name());
  });

  // Test distance filtering
  suite.run_test("Distance Filtering", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "NearBody",
                                  .mass = 1000.0,
                                  .position = Vector3d{1.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "FarBody",
                                  .mass = 1000.0,
                                  .position = Vector3d{100.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    Vector3d origin{0.0, 0.0, 0.0};
    auto near_bodies = collection.filter_by_distance_from_point(origin, 10.0);
    ASSERT_EQ(1, near_bodies.size());
    ASSERT_EQ("NearBody", near_bodies[0].get().name());

    auto all_bodies = collection.filter_by_distance_from_point(origin, 200.0);
    ASSERT_EQ(2, all_bodies.size());
  });

  // Test bulk add operation
  suite.run_test("Bulk Add Operation", []() {
    BodyCollection collection;

    std::vector<CelestialBody> bodies_to_add;

    for (int i = 0; i < 5; ++i) {
      bodies_to_add.emplace_back(CelestialBody::Properties{
          .name = "Body" + std::to_string(i),
          .mass = 1000.0 * (i + 1),
          .position = Vector3d{static_cast<long double>(i), 0.0, 0.0},
          .velocity = Vector3d{},
          .type = BodyType::Planet,
          .priority = BodyPriority::Essential,
          .jpl_id = "",
          .creation_date = std::chrono::system_clock::now()});
    }

    auto result = collection.bulk_add_bodies(bodies_to_add);
    ASSERT_EQ(5, result.total_operations);
    ASSERT_EQ(5, result.successful_operations);
    ASSERT_EQ(0, result.failed_operations);
    ASSERT_TRUE(result.errors.empty());
    ASSERT_EQ(5, collection.size());
  });

  // Test bulk remove operation
  suite.run_test("Bulk Remove Operation", []() {
    BodyCollection collection;

    // Add some bodies first
    for (int i = 0; i < 3; ++i) {
      collection.add_body(CelestialBody::Properties{
          .name = "RemoveMe" + std::to_string(i),
          .mass = 1000.0,
          .position = Vector3d{},
          .velocity = Vector3d{},
          .type = BodyType::Planet,
          .priority = BodyPriority::Essential,
          .jpl_id = "",
          .creation_date = std::chrono::system_clock::now()});
    }

    ASSERT_EQ(3, collection.size());

    std::vector<std::string> names_to_remove = {"RemoveMe0", "RemoveMe1", "NonExistent"};
    auto result = collection.bulk_remove_bodies(names_to_remove);

    ASSERT_EQ(3, result.total_operations);
    ASSERT_EQ(2, result.successful_operations);
    ASSERT_EQ(1, result.failed_operations);
    ASSERT_EQ(1, result.errors.size());
    ASSERT_EQ(1, collection.size());  // Only RemoveMe2 should remain
  });

  // Test bulk position update
  suite.run_test("Bulk Position Update", []() {
    BodyCollection collection;

    // Add some bodies
    for (int i = 0; i < 3; ++i) {
      collection.add_body(CelestialBody::Properties{
          .name = "UpdateMe" + std::to_string(i),
          .mass = 1000.0,
          .position = Vector3d{static_cast<long double>(i), 0.0, 0.0},
          .velocity = Vector3d{},
          .type = BodyType::Planet,
          .priority = BodyPriority::Essential,
          .jpl_id = "",
          .creation_date = std::chrono::system_clock::now()});
    }

    // Update all positions by adding (10, 0, 0)
    auto result = collection.bulk_update_positions([](const CelestialBody& body) {
      return body.position() + Vector3d{10.0, 0.0, 0.0};
    });

    ASSERT_EQ(3, result.total_operations);
    ASSERT_EQ(3, result.successful_operations);
    ASSERT_EQ(0, result.failed_operations);

    // Verify positions were updated
    for (int i = 0; i < 3; ++i) {
      auto body = collection.find_body("UpdateMe" + std::to_string(i));
      ASSERT_TRUE(body.has_value());
      ASSERT_EQ(static_cast<long double>(i + 10), body->get().position().x());
    }
  });

  // Test error handling with invalid data
  suite.run_test("Error Handling with Invalid Data", []() {
    BodyCollection collection;

    // Try to add body with negative mass - this should throw during construction
    bool exception_caught = false;
    try {
      CelestialBody invalid_body{
          CelestialBody::Properties{.name = "InvalidBody",
                                    .mass = -1000.0,  // Negative mass
                                    .position = Vector3d{1.0, 0.0, 0.0},
                                    .velocity = Vector3d{0.0, 1.0, 0.0},
                                    .type = BodyType::Planet,
                                    .priority = BodyPriority::Essential,
                                    .jpl_id = "",
                                    .creation_date = std::chrono::system_clock::now()}};
    } catch (const std::invalid_argument&) {
      exception_caught = true;
    }

    ASSERT_TRUE(exception_caught);
    ASSERT_EQ(0, collection.size());

    // Test validation with valid body but invalid properties after construction
    CelestialBody valid_body{
        CelestialBody::Properties{.name = "ValidBody",
                                  .mass = 1000.0,  // Valid mass
                                  .position = Vector3d{1.0, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 1.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()}};

    auto result = collection.add_body_validated(valid_body);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(1, result.affected_bodies);
    ASSERT_EQ(1, collection.size());
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
