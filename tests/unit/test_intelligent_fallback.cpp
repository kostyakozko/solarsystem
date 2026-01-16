/**
 * @file test_intelligent_fallback.cpp
 * @brief Unit tests for intelligent fallback strategies (Task 6)
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>

#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Math;
TEST(IntelligentFallbackStrategiesTestsTask6, Data_Source_Assessment) {
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
}
TEST(IntelligentFallbackStrategiesTestsTask6, Graceful_Fallback_Strategy) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.fallback_strategy = BodyFactory::FallbackStrategy::GRACEFUL;
  options.preferred_source = BodyFactory::DataSource::JPL_HORIZONS;
  options.allow_fallback = true;

  auto result = factory.create_body("Earth", options);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ("Earth", result.value().name());
  ASSERT_TRUE(result.value().mass() > 0.0);
}
TEST(IntelligentFallbackStrategiesTestsTask6, Intelligent_Fallback_Strategy) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.fallback_strategy = BodyFactory::FallbackStrategy::INTELLIGENT;
  options.minimum_quality = BodyFactory::DataQuality::ACCEPTABLE;
  options.prefer_recent_data = true;

  auto result = factory.create_body("Mars", options);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ("Mars", result.value().name());
  ASSERT_TRUE(result.value().mass() > 0.0);
}
TEST(IntelligentFallbackStrategiesTestsTask6, Strict_Fallback_Strategy) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.fallback_strategy = BodyFactory::FallbackStrategy::STRICT;
  options.preferred_source = BodyFactory::DataSource::FALLBACK_DATA;
  options.allow_fallback = false;

  auto result = factory.create_body("Venus", options);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ("Venus", result.value().name());
}
TEST(IntelligentFallbackStrategiesTestsTask6, Partial_Data_Allowed_Strategy) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.fallback_strategy = BodyFactory::FallbackStrategy::PARTIAL_ALLOWED;
  options.allow_partial_data = true;
  options.minimum_quality = BodyFactory::DataQuality::POOR;  // Accept any quality

  auto result = factory.create_body("Mercury", options);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ("Mercury", result.value().name());
}
TEST(IntelligentFallbackStrategiesTestsTask6, Hybrid_Approach_Strategy) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.fallback_strategy = BodyFactory::FallbackStrategy::HYBRID;
  options.minimum_quality = BodyFactory::DataQuality::ACCEPTABLE;

  auto result = factory.create_body("Jupiter", options);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ("Jupiter", result.value().name());
}
TEST(IntelligentFallbackStrategiesTestsTask6, Data_Quality_Assessment) {
  BodyFactory factory;

  // Create a body from fallback data
  auto result = factory.create_body("Saturn");
  ASSERT_TRUE(result.has_value());

  auto quality = factory.assess_data_quality(result.value(), BodyFactory::DataSource::FALLBACK_DATA,
                                             std::chrono::system_clock::now());

  // Fallback data should be at least acceptable quality
  EXPECT_GE(static_cast<int>(quality), static_cast<int>(BodyFactory::DataQuality::ACCEPTABLE));
}
TEST(IntelligentFallbackStrategiesTestsTask6, Data_Source_Prioritization) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.prefer_recent_data = true;

  auto sources = factory.get_prioritized_sources("Uranus", options);
  ASSERT_FALSE(sources.empty());

  // Should prioritize JPL HORIZONS first when prefer_recent_data is true
  if (sources.size() > 1) {
    ASSERT_EQ(static_cast<int>(BodyFactory::DataSource::JPL_HORIZONS),
              static_cast<int>(sources[0]));
  }
}
TEST(IntelligentFallbackStrategiesTestsTask6, Data_Source_Availability) {
  BodyFactory factory;

  // Fallback data should always be available for known bodies
  ASSERT_TRUE(factory.is_data_source_available(BodyFactory::DataSource::FALLBACK_DATA, "Neptune"));

  // Unknown body should not be available in fallback
  ASSERT_FALSE(
      factory.is_data_source_available(BodyFactory::DataSource::FALLBACK_DATA, "UnknownBody"));
}
TEST(IntelligentFallbackStrategiesTestsTask6, Fallback_Result_Execution) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.fallback_strategy = BodyFactory::FallbackStrategy::GRACEFUL;

  auto result = factory.execute_fallback_strategy("Pluto", options);
  ASSERT_TRUE(result.success);
  ASSERT_FALSE(result.fallback_chain.empty());
  ASSERT_TRUE(result.total_time.count() >= 0);
}
TEST(IntelligentFallbackStrategiesTestsTask6, Minimum_Quality_Filtering) {
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
}
TEST(IntelligentFallbackStrategiesTestsTask6, Data_Age_Considerations) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.max_data_age = std::chrono::hours(1);  // Very strict age requirement
  options.prefer_recent_data = true;

  auto sources = factory.assess_data_sources("Vesta", options);
  // Should still have some sources available (at least fallback)
  ASSERT_FALSE(sources.empty());
}
TEST(IntelligentFallbackStrategiesTestsTask6, Error_Handling___No_Available_Sources) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;
  options.fallback_strategy = BodyFactory::FallbackStrategy::STRICT;
  options.preferred_source = BodyFactory::DataSource::FALLBACK_DATA;
  options.allow_fallback = false;

  // Try to create a body that doesn't exist in fallback data
  auto result = factory.create_body("NonExistentBody", options);
  ASSERT_FALSE(result.has_value());
  ASSERT_FALSE(result.error().empty());
}
TEST(IntelligentFallbackStrategiesTestsTask6, Backward_Compatibility) {
  BodyFactory factory;
  BodyFactory::CreationOptions options;  // Default options

  // Should work the same as before for default options
  auto result = factory.create_body("Pluto", options);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ("Pluto", result.value().name());
}
