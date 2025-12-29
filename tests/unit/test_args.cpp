/**
 * @file test_args.cpp
 * @brief Comprehensive unit tests for modern C++20 argument parsing
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_utils/argument_parser.hpp"

using namespace SolarSystem::Utils;

// ============================================================================
// Date Parsing Tests
// ============================================================================

TEST(DateParsing, ValidISOFormat) {
  auto result = Date::from_string("2025-01-01");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ("2025-01-01", result.value().to_string());
}

TEST(DateParsing, InvalidFormat) {
  auto result = Date::from_string("invalid-date");
  ASSERT_FALSE(result.has_value());
  auto error_str = to_string(result.error());
  auto expected_str = to_string(ArgumentError::InvalidDateFormat);
  EXPECT_EQ(expected_str, error_str);
}

TEST(DateParsing, EmptyString) {
  auto result = Date::from_string("");
  EXPECT_FALSE(result.has_value());
}

TEST(DateParsing, DifferentValidFormats) {
  auto result1 = Date::from_string("2025-12-31");
  EXPECT_TRUE(result1.has_value());

  auto result2 = Date::from_string("2025-01-15");
  EXPECT_TRUE(result2.has_value());

  auto result3 = Date::from_string("2025-06-30");
  EXPECT_TRUE(result3.has_value());
}

TEST(DateParsing, InvalidMonth) {
  auto result = Date::from_string("2025-13-01");
  EXPECT_FALSE(result.has_value());
}

TEST(DateParsing, InvalidDay) {
  auto result = Date::from_string("2025-01-32");
  EXPECT_FALSE(result.has_value());
}

// ============================================================================
// SimulationArgumentParser Tests
// ============================================================================

TEST(SimulationArgumentParser, BasicUsage) {
  const char* argv[] = {"test_program", "--date", "2025-12-31", "--verbose"};
  int argc = 4;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  ASSERT_TRUE(result.has_value());
  auto config = result.value();
  EXPECT_TRUE(config.target_date.has_value());
  EXPECT_EQ("2025-12-31", config.date_string);
  EXPECT_TRUE(config.verbose);
  EXPECT_FALSE(config.use_current_date);
}

TEST(SimulationArgumentParser, NoArguments) {
  const char* argv[] = {"test_program"};
  int argc = 1;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  ASSERT_TRUE(result.has_value());
  auto config = result.value();
  EXPECT_FALSE(config.verbose);
}

TEST(SimulationArgumentParser, VerboseFlag) {
  const char* argv[] = {"test_program", "--verbose"};
  int argc = 2;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().verbose);
}

TEST(SimulationArgumentParser, DateOnly) {
  const char* argv[] = {"test_program", "--date", "2025-06-15"};
  int argc = 3;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  ASSERT_TRUE(result.has_value());
  auto config = result.value();
  EXPECT_TRUE(config.target_date.has_value());
  EXPECT_EQ("2025-06-15", config.date_string);
  EXPECT_FALSE(config.verbose);
}

TEST(SimulationArgumentParser, InvalidDateArgument) {
  const char* argv[] = {"test_program", "--date", "invalid"};
  int argc = 3;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  EXPECT_FALSE(result.has_value());
}

TEST(SimulationArgumentParser, MissingDateValue) {
  const char* argv[] = {"test_program", "--date"};
  int argc = 2;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  EXPECT_FALSE(result.has_value());
}

TEST(SimulationArgumentParser, UnknownFlag) {
  const char* argv[] = {"test_program", "--unknown-flag"};
  int argc = 2;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  // Should either ignore or fail gracefully - implementation dependent
  // Just verify it doesn't crash
  (void)result;
}

TEST(SimulationArgumentParser, MultipleFlags) {
  const char* argv[] = {"test_program", "--verbose", "--date", "2025-03-20"};
  int argc = 4;

  SimulationArgumentParser parser("test_program");
  auto result = parser.parse(argc, argv);

  ASSERT_TRUE(result.has_value());
  auto config = result.value();
  EXPECT_TRUE(config.verbose);
  EXPECT_TRUE(config.target_date.has_value());
  EXPECT_EQ("2025-03-20", config.date_string);
}

TEST(SimulationArgumentParser, ProgramName) {
  const char* argv[] = {"my_program"};
  int argc = 1;

  SimulationArgumentParser parser("my_program");
  auto result = parser.parse(argc, argv);

  EXPECT_TRUE(result.has_value());
}

// ============================================================================
// ArgumentError Enum Tests
// ============================================================================

TEST(ArgumentError, EnumValues) {
  EXPECT_EQ(ArgumentError::InvalidDateFormat, ArgumentError::InvalidDateFormat);
  EXPECT_NE(ArgumentError::InvalidDateFormat, ArgumentError::MissingValue);
  EXPECT_NE(ArgumentError::UnknownOption, ArgumentError::InvalidValue);
}

// ============================================================================
// Date to_string Tests
// ============================================================================

TEST(DateToString, Consistency) {
  auto result = Date::from_string("2025-07-04");
  ASSERT_TRUE(result.has_value());

  std::string str = result.value().to_string();
  EXPECT_EQ("2025-07-04", str);
}

TEST(DateToString, RoundTrip) {
  std::string original = "2025-11-22";
  auto parsed = Date::from_string(original);
  ASSERT_TRUE(parsed.has_value());

  std::string round_trip = parsed.value().to_string();
  EXPECT_EQ(original, round_trip);
}
