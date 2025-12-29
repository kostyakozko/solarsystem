/**
 * @file test_modern_body_collection.cpp
 * @brief Unit tests for modern BodyCollection class
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>

#include "solar_core/bodies/body_collection.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST(ModernBodyCollection, DefaultConstructor) {
  BodyCollection collection;
  EXPECT_TRUE(collection.empty());
  EXPECT_EQ(0, collection.size());
}

TEST(ModernBodyCollection, AddSingleBody) {
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

  EXPECT_FALSE(collection.empty());
  EXPECT_EQ(1, collection.size());
  EXPECT_TRUE(collection.contains("Earth"));
}

TEST(ModernBodyCollection, AddMultipleBodies) {
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

  EXPECT_EQ(2, collection.size());
  EXPECT_TRUE(collection.contains("Sun"));
  EXPECT_TRUE(collection.contains("Earth"));
  EXPECT_FALSE(collection.contains("Mars"));
}

TEST(ModernBodyCollection, DuplicateNamePrevention) {
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

  EXPECT_THROW(collection.add_body(CelestialBody{props}), std::invalid_argument);
  EXPECT_EQ(1, collection.size());  // Should still be 1
}

// ============================================================================
// Lookup and Removal Tests
// ============================================================================

TEST(ModernBodyCollection, BodyLookup) {
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
  EXPECT_EQ("Mars", found->get().name());
  EXPECT_EQ(6.41693e23, found->get().mass());

  auto not_found = collection.find_body("Jupiter");
  EXPECT_FALSE(not_found.has_value());
}

TEST(ModernBodyCollection, BodyRemoval) {
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

  EXPECT_EQ(2, collection.size());
  EXPECT_TRUE(collection.contains("Venus"));

  bool removed = collection.remove_body("Venus");
  EXPECT_TRUE(removed);
  EXPECT_EQ(1, collection.size());
  EXPECT_FALSE(collection.contains("Venus"));
  EXPECT_TRUE(collection.contains("Mercury"));

  bool not_removed = collection.remove_body("Jupiter");
  EXPECT_FALSE(not_removed);
  EXPECT_EQ(1, collection.size());
}

// ============================================================================
// Filter Tests
// ============================================================================

TEST(ModernBodyCollection, FilterByType) {
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
  EXPECT_EQ("Sun", stars[0].get().name());

  auto planets = collection.filter_by_type(BodyType::Planet);
  ASSERT_EQ(1, planets.size());
  EXPECT_EQ("Earth", planets[0].get().name());

  auto moons = collection.filter_by_type(BodyType::Moon);
  ASSERT_EQ(1, moons.size());
  EXPECT_EQ("Moon", moons[0].get().name());

  auto asteroids = collection.filter_by_type(BodyType::Asteroid);
  EXPECT_EQ(0, asteroids.size());
}

TEST(ModernBodyCollection, FilterByPriority) {
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
  EXPECT_EQ("Earth", essential[0].get().name());

  auto important = collection.filter_by_priority(BodyPriority::Important);
  ASSERT_EQ(1, important.size());
  EXPECT_EQ("Moon", important[0].get().name());

  auto optional = collection.filter_by_priority(BodyPriority::Optional);
  ASSERT_EQ(1, optional.size());
  EXPECT_EQ("Voyager1", optional[0].get().name());
}

// ============================================================================
// Count Operations Tests
// ============================================================================

TEST(ModernBodyCollection, CountOperations) {
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

  EXPECT_EQ(1, collection.count_by_type(BodyType::Star));
  EXPECT_EQ(2, collection.count_by_type(BodyType::Planet));
  EXPECT_EQ(1, collection.count_by_type(BodyType::Moon));
  EXPECT_EQ(0, collection.count_by_type(BodyType::Spacecraft));

  EXPECT_EQ(3, collection.count_by_priority(BodyPriority::Essential));
  EXPECT_EQ(1, collection.count_by_priority(BodyPriority::Important));
  EXPECT_EQ(0, collection.count_by_priority(BodyPriority::Optional));
}

// ============================================================================
// Mass Calculation Tests
// ============================================================================

TEST(ModernBodyCollection, TotalMassCalculation) {
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
  EXPECT_EQ(6000.0, total);
}

TEST(ModernBodyCollection, CenterOfMassCalculation) {
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
  EXPECT_LT(std::abs(com.x()), 1e-10);  // Should be at origin
  EXPECT_LT(std::abs(com.y()), 1e-10);
  EXPECT_LT(std::abs(com.z()), 1e-10);
}

// ============================================================================
// Apply Operations Tests
// ============================================================================

TEST(ModernBodyCollection, ApplyToAllBodies) {
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

  EXPECT_EQ(11.0, body1->get().position().x());
  EXPECT_EQ(12.0, body2->get().position().x());
}

TEST(ModernBodyCollection, ApplyToFilteredBodies) {
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

  EXPECT_EQ(6.0, planet->get().position().x());  // Moved
  EXPECT_EQ(2.0, moon->get().position().x());    // Not moved
}

// ============================================================================
// Validation and Iterator Tests
// ============================================================================

TEST(ModernBodyCollection, Validation) {
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

  EXPECT_TRUE(collection.validate());
  EXPECT_TRUE(collection.get_validation_errors().empty());
}

TEST(ModernBodyCollection, IteratorSupport) {
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
    EXPECT_GT(body.mass(), 0.0);
    count++;
  }
  EXPECT_EQ(2, count);

  // Test iterators
  auto it = collection.begin();
  EXPECT_NE(it, collection.end());
  EXPECT_EQ("Body1", it->name());

  ++it;
  EXPECT_NE(it, collection.end());
  EXPECT_EQ("Body2", it->name());

  ++it;
  EXPECT_EQ(it, collection.end());
}

TEST(ModernBodyCollection, ClearCollection) {
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

  EXPECT_FALSE(collection.empty());
  EXPECT_EQ(1, collection.size());

  collection.clear();

  EXPECT_TRUE(collection.empty());
  EXPECT_EQ(0, collection.size());
  EXPECT_FALSE(collection.contains("Body1"));
}
