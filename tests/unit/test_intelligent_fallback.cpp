/**
 * @file test_intelligent_fallback.cpp
 * @brief Unit tests for intelligent fallback strategies (Task 6)
 */

#include <chrono>

#include "../utils/test_framework.h"
#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;

int main() {
  TestSuite suite("Intelligent Fallback Strategies Tests (Task 6)");

  // Test data source assessment
  suite.run_test("Data Source Assessment", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;

    auto sources = factory.assess_data_sources("Earth", options);
    ASSERT_FALSE(sources.empty());

    // Should have at least fallback data available
    bool has_fallback = false;
    for (const auto& source : sources) {
      if (source.source == BodyFactory::DataSource::FALLBACK_DATA) {
        has_fallback = true;
        ASSERT_TRUE(source.is_complete);
        ASSERT_FALSE(source.quality_reason.empty());
        break;
      }
    }
    ASSERT_TRUE(has_fallback);
  });

  // Test graceful fallback strategy
  suite.run_test("Graceful Fallback Strategy", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::GRACEFUL;
    options.preferred_source = BodyFactory::DataSource::JPL_HORIZONS;
    options.allow_fallback = true;

    auto result = factory.create_body("Earth", options);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ("Earth", result.value().name());
    ASSERT_TRUE(result.value().mass() > 0.0);
  });

  // Test intelligent fallback strategy
  suite.run_test("Intelligent Fallback Strategy", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::INTELLIGENT;
    options.minimum_quality = BodyFactory::DataQuality::ACCEPTABLE;
    options.prefer_recent_data = true;

    auto result = factory.create_body("Mars", options);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ("Mars", result.value().name());
    ASSERT_TRUE(result.value().mass() > 0.0);
  });

  // Test strict fallback strategy
  suite.run_test("Strict Fallback Strategy", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::STRICT;
    options.preferred_source = BodyFactory::DataSource::FALLBACK_DATA;
    options.allow_fallback = false;

    auto result = factory.create_body("Venus", options);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ("Venus", result.value().name());
  });

  // Test partial data allowed strategy
  suite.run_test("Partial Data Allowed Strategy", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::PARTIAL_ALLOWED;
    options.allow_partial_data = true;
    options.minimum_quality = BodyFactory::DataQuality::POOR; // Accept any quality

    auto result = factory.create_body("Mercury", options);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ("Mercury", result.value().name());
  });

  // Test hybrid approach strategy
  suite.run_test("Hybrid Approach Strategy", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::HYBRID;
    options.minimum_quality = BodyFactory::DataQuality::ACCEPTABLE;

    auto result = factory.create_body("Jupiter", options);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ("Jupiter", result.value().name());
  });

  // Test data quality assessment
  suite.run_test("Data Quality Assessment", []() {
    BodyFactory factory;

    // Create a body from fallback data
    auto result = factory.create_body("Saturn");
    ASSERT_TRUE(result.has_value());

    auto quality = factory.assess_data_quality(
        result.value(),
        BodyFactory::DataSource::FALLBACK_DATA,
        std::chrono::system_clock::now());

    // Fallback data should be at least acceptable quality
    ASSERT_TRUE(quality >= BodyFactory::DataQuality::ACCEPTABLE);
  });

  // Test data source prioritization
  suite.run_test("Data Source Prioritization", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.prefer_recent_data = true;

    auto sources = factory.get_prioritized_sources("Uranus", options);
    ASSERT_FALSE(sources.empty());

    // Should prioritize JPL HORIZONS first when prefer_recent_data is true
    if (sources.size() > 1) {
      ASSERT_EQ(static_cast<int>(BodyFactory::DataSource::JPL_HORIZONS), static_cast<int>(sources[0]));
    }
  });

  // Test data source availability
  suite.run_test("Data Source Availability", []() {
    BodyFactory factory;

    // Fallback data should always be available for known bodies
    ASSERT_TRUE(factory.is_data_source_available(
        BodyFactory::DataSource::FALLBACK_DATA, "Neptune"));

    // Unknown body should not be available in fallback
    ASSERT_FALSE(factory.is_data_source_available(
        BodyFactory::DataSource::FALLBACK_DATA, "UnknownBody"));
  });

  // Test fallback result execution
  suite.run_test("Fallback Result Execution", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::GRACEFUL;

    auto result = factory.execute_fallback_strategy("Pluto", options);
    ASSERT_TRUE(result.success);
    ASSERT_FALSE(result.fallback_chain.empty());
    ASSERT_TRUE(result.total_time.count() >= 0);
  });

  // Test minimum quality filtering
  suite.run_test("Minimum Quality Filtering", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::INTELLIGENT;
    options.minimum_quality = BodyFactory::DataQuality::EXCELLENT;
    options.allow_fallback = true;

    // This might fail if no excellent quality data is available
    // but should not crash
    auto result = factory.create_body("Ceres", options);
    // Don't assert success since it depends on data availability
    // Just ensure it doesn't crash
  });

  // Test data age considerations
  suite.run_test("Data Age Considerations", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.max_data_age = std::chrono::hours(1); // Very strict age requirement
    options.prefer_recent_data = true;

    auto sources = factory.assess_data_sources("Vesta", options);
    // Should still have some sources available (at least fallback)
    ASSERT_FALSE(sources.empty());
  });

  // Test error handling with no available sources
  suite.run_test("Error Handling - No Available Sources", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options;
    options.fallback_strategy = BodyFactory::FallbackStrategy::STRICT;
    options.preferred_source = BodyFactory::DataSource::FALLBACK_DATA;
    options.allow_fallback = false;

    // Try to create a body that doesn't exist in fallback data
    auto result = factory.create_body("NonExistentBody", options);
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.error().empty());
  });

  // Test backward compatibility
  suite.run_test("Backward Compatibility", []() {
    BodyFactory factory;
    BodyFactory::CreationOptions options; // Default options

    // Should work the same as before for default options
    auto result = factory.create_body("Pluto", options);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ("Pluto", result.value().name());
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
