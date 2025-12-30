/**
 * @file test_output_formatter.cpp
 * @brief Unit tests for comprehensive output formatting system
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/output/output_formatter.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

using namespace SolarSystem;
using namespace SolarSystem::Output;

// Helper function to create test bodies
Bodies::BodyCollection create_test_bodies() {
  Bodies::BodyCollection bodies;

  // Create Earth
  Bodies::CelestialBody::Properties earth_props;
  earth_props.name = "Earth";
  earth_props.mass = 5.972e24;
  earth_props.position = Math::Vector3d(1.496e11, 0, 0);
  earth_props.velocity = Math::Vector3d(0, 29780, 0);
  earth_props.type = Bodies::BodyType::Planet;
  earth_props.priority = Bodies::BodyPriority::Essential;
  bodies.add_body(Bodies::CelestialBody(earth_props));

  // Create Moon
  Bodies::CelestialBody::Properties moon_props;
  moon_props.name = "Moon";
  moon_props.mass = 7.342e22;
  moon_props.position = Math::Vector3d(1.496e11 + 3.844e8, 0, 0);
  moon_props.velocity = Math::Vector3d(0, 29780 + 1022, 0);
  moon_props.type = Bodies::BodyType::Moon;
  moon_props.priority = Bodies::BodyPriority::Important;
  bodies.add_body(Bodies::CelestialBody(moon_props));

  return bodies;
}

// Test format parsing
TEST(OutputFormatter, ParseFormatText) {
  auto result = OutputFormatter::parse_format("text");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value() == OutputFormat::TEXT);
}

TEST(OutputFormatter, ParseFormatJSON) {
  auto result = OutputFormatter::parse_format("json");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value() == OutputFormat::JSON);
}

TEST(OutputFormatter, ParseFormatInvalid) {
  auto result = OutputFormatter::parse_format("invalid");
  ASSERT_FALSE(result.has_value());
}

// Test output filter
TEST(OutputFormatter, FilterIncludeBodies) {
  OutputFilter filter;
  filter.include_bodies = {"Earth"};
  ASSERT_TRUE(filter.should_include_body("Earth"));
  ASSERT_FALSE(filter.should_include_body("Moon"));
}

// Test output options validation
TEST(OutputFormatter, ValidateOptionsValid) {
  OutputOptions options;
  options.format = OutputFormat::JSON;
  options.quality = QualityLevel::STANDARD;
  auto result = options.validate();
  ASSERT_TRUE(result.has_value());
}

// Test text format
TEST(OutputFormatter, FormatAsText) {
  auto bodies = create_test_bodies();
  auto time_point = std::chrono::system_clock::now();

  OutputOptions options;
  options.format = OutputFormat::TEXT;
  options.include_header = true;

  auto result = OutputFormatter::format_bodies(bodies, time_point, options);
  ASSERT_TRUE(result.has_value());

  const auto& output = result.value();
  ASSERT_FALSE(output.content.empty());
  EXPECT_NE(std::string::npos, output.content.find("Earth"));
}

// Test JSON format
TEST(OutputFormatter, FormatAsJSON) {
  auto bodies = create_test_bodies();
  auto time_point = std::chrono::system_clock::now();

  OutputOptions options;
  options.format = OutputFormat::JSON;
  options.include_metadata = true;

  auto result = OutputFormatter::format_bodies(bodies, time_point, options);
  ASSERT_TRUE(result.has_value());

  const auto& output = result.value();
  EXPECT_NE(std::string::npos, output.content.find("\"bodies\""));
}

// Test utility functions
TEST(OutputFormatter, ToStringOutputFormat) {
  ASSERT_TRUE(to_string(OutputFormat::TEXT) == "text");
  ASSERT_TRUE(to_string(OutputFormat::JSON) == "json");
  ASSERT_TRUE(to_string(OutputFormat::CSV) == "csv");
}
