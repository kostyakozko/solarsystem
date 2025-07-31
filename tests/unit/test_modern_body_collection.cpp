/**
 * @file test_modern_body_collection.cpp
 * @brief Unit tests for modern BodyCollection class
 */

#include <chrono>

#include "../utils/test_framework.h"
#include "solar_core/bodies/body_collection.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

int main() {
  TestSuite suite("Modern BodyCollection Tests");

  suite.run_test("Default Constructor", []() {
    BodyCollection collection;
    ASSERT_TRUE(collection.empty());
    ASSERT_EQ(0, collection.size());
  });

  suite.run_test("Add Single Body", []() {
    BodyCollection collection;

    CelestialBody::Properties props{.name = "Earth",
                                    .mass = 5.97219e24,
                                    .position = Vector3d{1.496e11, 0.0, 0.0},
                                    .velocity = Vector3d{0.0, 29780.0, 0.0},
                                    .type = BodyType::Planet,
                                    .priority = BodyPriority::Essential,
                                    .jpl_id = "399",
                                    .creation_date = std::chrono::system_clock::now()};

    collection.add_body(CelestialBody{props});

    ASSERT_FALSE(collection.empty());
    ASSERT_EQ(1, collection.size());
    ASSERT_TRUE(collection.contains("Earth"));
  });

  suite.run_test("Add Multiple Bodies", []() {
    BodyCollection collection;

    // Add Sun
    collection.add_body(
        CelestialBody::Properties{.name = "Sun",
                                  .mass = 1.98847e30,
                                  .position = Vector3d{0.0, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 0.0, 0.0},
                                  .type = BodyType::Star,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    // Add Earth
    collection.add_body(
        CelestialBody::Properties{.name = "Earth",
                                  .mass = 5.97219e24,
                                  .position = Vector3d{1.496e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    ASSERT_EQ(2, collection.size());
    ASSERT_TRUE(collection.contains("Sun"));
    ASSERT_TRUE(collection.contains("Earth"));
    ASSERT_FALSE(collection.contains("Mars"));
  });

  suite.run_test("Duplicate Name Prevention", []() {
    BodyCollection collection;

    CelestialBody::Properties props{.name = "Earth",
                                    .mass = 5.97219e24,
                                    .position = Vector3d{1.496e11, 0.0, 0.0},
                                    .velocity = Vector3d{0.0, 29780.0, 0.0},
                                    .type = BodyType::Planet,
                                    .priority = BodyPriority::Essential,
                                    .jpl_id = "",
                                    .creation_date = std::chrono::system_clock::now()};

    collection.add_body(CelestialBody{props});

    bool exception_thrown = false;
    try {
      collection.add_body(CelestialBody{props});  // Same name
    } catch (const std::invalid_argument&) {
      exception_thrown = true;
    }

    ASSERT_TRUE(exception_thrown);
    ASSERT_EQ(1, collection.size());  // Should still be 1
  });

  suite.run_test("Body Lookup", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Mars",
                                  .mass = 6.41693e23,
                                  .position = Vector3d{2.279e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 24077.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    auto found = collection.find_body("Mars");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ("Mars", found->get().name());
    ASSERT_EQ(6.41693e23, found->get().mass());

    auto not_found = collection.find_body("Jupiter");
    ASSERT_FALSE(not_found.has_value());
  });

  suite.run_test("Body Removal", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Venus",
                                  .mass = 4.86732e24,
                                  .position = Vector3d{1.082e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 35020.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Mercury",
                                  .mass = 3.30104e23,
                                  .position = Vector3d{5.79e10, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 47362.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    ASSERT_EQ(2, collection.size());
    ASSERT_TRUE(collection.contains("Venus"));

    bool removed = collection.remove_body("Venus");
    ASSERT_TRUE(removed);
    ASSERT_EQ(1, collection.size());
    ASSERT_FALSE(collection.contains("Venus"));
    ASSERT_TRUE(collection.contains("Mercury"));

    bool not_removed = collection.remove_body("Jupiter");
    ASSERT_FALSE(not_removed);
    ASSERT_EQ(1, collection.size());
  });

  suite.run_test("Filter by Type", []() {
    BodyCollection collection;

    // Add different types
    collection.add_body(
        CelestialBody::Properties{.name = "Sun",
                                  .mass = 1.98847e30,
                                  .position = Vector3d{0.0, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 0.0, 0.0},
                                  .type = BodyType::Star,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Earth",
                                  .mass = 5.97219e24,
                                  .position = Vector3d{1.496e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Moon",
                                  .mass = 7.342e22,
                                  .position = Vector3d{1.496e11 + 3.844e8, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0 + 1022.0, 0.0},
                                  .type = BodyType::Moon,
                                  .priority = BodyPriority::Important,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    auto stars = collection.filter_by_type(BodyType::Star);
    ASSERT_EQ(1, stars.size());
    ASSERT_EQ("Sun", stars[0].get().name());

    auto planets = collection.filter_by_type(BodyType::Planet);
    ASSERT_EQ(1, planets.size());
    ASSERT_EQ("Earth", planets[0].get().name());

    auto moons = collection.filter_by_type(BodyType::Moon);
    ASSERT_EQ(1, moons.size());
    ASSERT_EQ("Moon", moons[0].get().name());

    auto asteroids = collection.filter_by_type(BodyType::Asteroid);
    ASSERT_EQ(0, asteroids.size());
  });

  suite.run_test("Filter by Priority", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Earth",
                                  .mass = 5.97219e24,
                                  .position = Vector3d{1.496e11, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0, 0.0},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Moon",
                                  .mass = 7.342e22,
                                  .position = Vector3d{1.496e11 + 3.844e8, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 29780.0 + 1022.0, 0.0},
                                  .type = BodyType::Moon,
                                  .priority = BodyPriority::Important,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Voyager1",
                                  .mass = 722.0,
                                  .position = Vector3d{2.0e13, 0.0, 0.0},
                                  .velocity = Vector3d{0.0, 17000.0, 0.0},
                                  .type = BodyType::Spacecraft,
                                  .priority = BodyPriority::Optional,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    auto essential = collection.filter_essential();
    ASSERT_EQ(1, essential.size());
    ASSERT_EQ("Earth", essential[0].get().name());

    auto important = collection.filter_by_priority(BodyPriority::Important);
    ASSERT_EQ(1, important.size());
    ASSERT_EQ("Moon", important[0].get().name());

    auto optional = collection.filter_by_priority(BodyPriority::Optional);
    ASSERT_EQ(1, optional.size());
    ASSERT_EQ("Voyager1", optional[0].get().name());
  });

  suite.run_test("Count Operations", []() {
    BodyCollection collection;

    // Add various bodies
    collection.add_body(
        CelestialBody::Properties{.name = "Sun",
                                  .mass = 1.98847e30,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Star,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

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
        CelestialBody::Properties{.name = "Moon",
                                  .mass = 7.342e22,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Moon,
                                  .priority = BodyPriority::Important,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    ASSERT_EQ(1, collection.count_by_type(BodyType::Star));
    ASSERT_EQ(2, collection.count_by_type(BodyType::Planet));
    ASSERT_EQ(1, collection.count_by_type(BodyType::Moon));
    ASSERT_EQ(0, collection.count_by_type(BodyType::Spacecraft));

    ASSERT_EQ(3, collection.count_by_priority(BodyPriority::Essential));
    ASSERT_EQ(1, collection.count_by_priority(BodyPriority::Important));
    ASSERT_EQ(0, collection.count_by_priority(BodyPriority::Optional));
  });

  suite.run_test("Total Mass Calculation", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Body1",
                                  .mass = 1000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Body2",
                                  .mass = 2000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Body3",
                                  .mass = 3000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    double total = collection.total_mass();
    ASSERT_EQ(6000.0, total);
  });

  suite.run_test("Center of Mass Calculation", []() {
    BodyCollection collection;

    // Two equal masses at opposite positions
    collection.add_body(
        CelestialBody::Properties{.name = "Body1",
                                  .mass = 1000.0,
                                  .position = Vector3d{-10.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Body2",
                                  .mass = 1000.0,
                                  .position = Vector3d{10.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    auto com = collection.center_of_mass();
    ASSERT_TRUE(std::abs(com.x()) < 1e-10);  // Should be at origin
    ASSERT_TRUE(std::abs(com.y()) < 1e-10);
    ASSERT_TRUE(std::abs(com.z()) < 1e-10);
  });

  suite.run_test("Apply to All Bodies", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Body1",
                                  .mass = 1000.0,
                                  .position = Vector3d{1.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Body2",
                                  .mass = 2000.0,
                                  .position = Vector3d{2.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    // Move all bodies by (10, 0, 0)
    Vector3d offset{10.0, 0.0, 0.0};
    collection.apply_to_all(
        [&offset](CelestialBody& body) { body.set_position(body.position() + offset); });

    auto body1 = collection.find_body("Body1");
    auto body2 = collection.find_body("Body2");

    ASSERT_TRUE(body1.has_value());
    ASSERT_TRUE(body2.has_value());

    ASSERT_EQ(11.0, body1->get().position().x());
    ASSERT_EQ(12.0, body2->get().position().x());
  });

  suite.run_test("Apply to Filtered Bodies", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Planet1",
                                  .mass = 1000.0,
                                  .position = Vector3d{1.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Moon1",
                                  .mass = 100.0,
                                  .position = Vector3d{2.0, 0.0, 0.0},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Moon,
                                  .priority = BodyPriority::Important,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    // Only move planets
    Vector3d offset{5.0, 0.0, 0.0};
    collection.apply_to_filtered(
        [](const CelestialBody& body) { return body.type() == BodyType::Planet; },
        [&offset](CelestialBody& body) { body.set_position(body.position() + offset); });

    auto planet = collection.find_body("Planet1");
    auto moon = collection.find_body("Moon1");

    ASSERT_EQ(6.0, planet->get().position().x());  // Moved
    ASSERT_EQ(2.0, moon->get().position().x());    // Not moved
  });

  suite.run_test("Validation", []() {
    BodyCollection collection;

    // Valid collection
    collection.add_body(
        CelestialBody::Properties{.name = "ValidBody",
                                  .mass = 1000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    ASSERT_TRUE(collection.validate());
    ASSERT_TRUE(collection.get_validation_errors().empty());
  });

  suite.run_test("Iterator Support", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Body1",
                                  .mass = 1000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    collection.add_body(
        CelestialBody::Properties{.name = "Body2",
                                  .mass = 2000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    // Test range-based for loop
    size_t count = 0;
    for (const auto& body : collection) {
      ASSERT_TRUE(body.mass() > 0.0);
      count++;
    }
    ASSERT_EQ(2, count);

    // Test iterators
    auto it = collection.begin();
    ASSERT_TRUE(it != collection.end());
    ASSERT_EQ("Body1", it->name());

    ++it;
    ASSERT_TRUE(it != collection.end());
    ASSERT_EQ("Body2", it->name());

    ++it;
    ASSERT_TRUE(it == collection.end());
  });

  suite.run_test("Clear Collection", []() {
    BodyCollection collection;

    collection.add_body(
        CelestialBody::Properties{.name = "Body1",
                                  .mass = 1000.0,
                                  .position = Vector3d{},
                                  .velocity = Vector3d{},
                                  .type = BodyType::Planet,
                                  .priority = BodyPriority::Essential,
                                  .jpl_id = "",
                                  .creation_date = std::chrono::system_clock::now()});

    ASSERT_FALSE(collection.empty());
    ASSERT_EQ(1, collection.size());

    collection.clear();

    ASSERT_TRUE(collection.empty());
    ASSERT_EQ(0, collection.size());
    ASSERT_FALSE(collection.contains("Body1"));
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
