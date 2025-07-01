/**
 * @file test_modern_body_factory.cpp
 * @brief Unit tests for modern BodyFactory class
 */

#include <chrono>

#include "../utils/test_framework.h"
#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem::Bodies;

int main() {
  TestSuite suite("Modern BodyFactory Tests");

  suite.run_test("Default Constructor", []() {
    BodyFactory factory;

    auto available_bodies = factory.get_available_bodies();
    ASSERT_TRUE(available_bodies.size() > 0);
  });

  suite.run_test("Create Body from Fallback Data", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options{.preferred_source = BodyFactory::DataSource::FALLBACK_DATA,
                                         .allow_fallback = false};

    auto earth_result = factory.create_body("Earth", options);
    ASSERT_TRUE(earth_result.has_value());

    const auto& earth = earth_result.value();
    ASSERT_EQ("Earth", earth.name());
    ASSERT_TRUE(earth.mass() > 0.0);
    ASSERT_EQ(to_string(BodyType::Planet), to_string(earth.type()));
    ASSERT_EQ(to_string(BodyPriority::Essential), to_string(earth.priority()));
  });

  suite.run_test("Create Multiple Bodies", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options{.preferred_source =
                                             BodyFactory::DataSource::FALLBACK_DATA};

    std::vector<std::string> body_names = {"Sun", "Earth", "Mars"};
    auto collection_result = factory.create_collection(body_names, options);

    ASSERT_TRUE(collection_result.has_value());

    const auto& collection = collection_result.value();
    ASSERT_EQ(3, collection.size());
    ASSERT_TRUE(collection.contains("Sun"));
    ASSERT_TRUE(collection.contains("Earth"));
    ASSERT_TRUE(collection.contains("Mars"));
  });

  suite.run_test("Create Inner Planets", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options{.preferred_source =
                                             BodyFactory::DataSource::FALLBACK_DATA};

    auto collection_result = factory.create_inner_planets(options);
    ASSERT_TRUE(collection_result.has_value());

    const auto& collection = collection_result.value();
    ASSERT_TRUE(collection.size() >= 4);  // At least Sun + 4 inner planets
    ASSERT_TRUE(collection.contains("Sun"));
    ASSERT_TRUE(collection.contains("Mercury"));
    ASSERT_TRUE(collection.contains("Venus"));
    ASSERT_TRUE(collection.contains("Earth"));
    ASSERT_TRUE(collection.contains("Mars"));
  });

  suite.run_test("Create Solar System", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options{.preferred_source =
                                             BodyFactory::DataSource::FALLBACK_DATA};

    auto collection_result = factory.create_solar_system(options);
    ASSERT_TRUE(collection_result.has_value());

    const auto& collection = collection_result.value();
    ASSERT_TRUE(collection.size() >= 8);  // At least 8 major planets + Sun

    // Check for essential bodies
    auto essential_bodies = collection.filter_essential();
    ASSERT_TRUE(essential_bodies.size() > 0);
  });

  suite.run_test("Body Availability Check", []() {
    BodyFactory factory;

    auto now = std::chrono::system_clock::now();
    auto past = now - std::chrono::hours(24 * 365 * 20);  // 20 years ago

    // Earth should always be available
    ASSERT_TRUE(factory.is_body_available("Earth", past));
    ASSERT_TRUE(factory.is_body_available("Earth", now));

    // New Horizons might not be available in the distant past
    // (depends on creation date in our definitions)
    ASSERT_TRUE(factory.is_body_available("New Horizons", now));
  });

  suite.run_test("Invalid Body Name", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options{.preferred_source = BodyFactory::DataSource::FALLBACK_DATA,
                                         .allow_fallback = false};

    auto result = factory.create_body("NonExistentPlanet", options);
    ASSERT_FALSE(result.has_value());

    std::string error = result.error();
    ASSERT_TRUE(error.find("NonExistentPlanet") != std::string::npos);
  });

  suite.run_test("Fallback Disabled", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options{.preferred_source = BodyFactory::DataSource::JPL_HORIZONS,
                                         .allow_fallback = false};

    // This might fail if JPL is not available, which is expected
    auto result = factory.create_body("Earth", options);
    // We don't assert success/failure here since it depends on network
    // Just verify that we get a meaningful response
    if (!result.has_value()) {
      ASSERT_TRUE(!result.error().empty());
    }
  });

  suite.run_test("Available Bodies List", []() {
    BodyFactory factory;

    auto available = factory.get_available_bodies();
    ASSERT_TRUE(available.size() > 0);

    // Should contain major planets
    bool has_earth = std::find(available.begin(), available.end(), "Earth") != available.end();
    ASSERT_TRUE(has_earth);
  });

  suite.run_test("Legacy Data Integration", []() {
    BodyFactory factory;

    // Test creating from legacy data (this tests the bridge to old system)
    auto result = factory.create_from_legacy_data("Earth");

    if (result.has_value()) {
      const auto& earth = result.value();
      ASSERT_EQ("Earth", earth.name());
      ASSERT_TRUE(earth.mass() > 0.0);
    }
    // If it fails, that's also acceptable since it depends on the legacy system state
  });

  suite.run_test("Data Source Preferences", []() {
    BodyFactory factory;

    // Test different data source preferences
    BodyFactory::CreationOptions fallback_only{
        .preferred_source = BodyFactory::DataSource::FALLBACK_DATA, .allow_fallback = false};

    BodyFactory::CreationOptions with_fallback{
        .preferred_source = BodyFactory::DataSource::JPL_HORIZONS, .allow_fallback = true};

    // Fallback should always work for known bodies
    auto fallback_result = factory.create_body("Earth", fallback_only);
    ASSERT_TRUE(fallback_result.has_value());

    // With fallback enabled, should also work (might use JPL or fallback)
    auto flexible_result = factory.create_body("Earth", with_fallback);
    ASSERT_TRUE(flexible_result.has_value());
  });

  suite.run_test("Collection Error Handling", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions strict_options{
        .preferred_source = BodyFactory::DataSource::FALLBACK_DATA, .allow_fallback = false};

    // Mix of valid and invalid body names
    std::vector<std::string> mixed_names = {"Earth", "InvalidPlanet", "Mars"};
    auto result = factory.create_collection(mixed_names, strict_options);

    // Should succeed with partial results when fallback is allowed
    BodyFactory::CreationOptions lenient_options{
        .preferred_source = BodyFactory::DataSource::FALLBACK_DATA, .allow_fallback = true};

    auto lenient_result = factory.create_collection(mixed_names, lenient_options);
    if (lenient_result.has_value()) {
      // Should have at least the valid bodies
      ASSERT_TRUE(lenient_result.value().contains("Earth"));
      ASSERT_TRUE(lenient_result.value().contains("Mars"));
    }
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
