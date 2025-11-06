/**
 * @file test_args.cpp
 * @brief Comprehensive unit tests for modern C++20 argument parsing
 */

#include "../utils/test_framework.h"
#include "solar_utils/argument_parser.hpp"

using namespace SolarSystem::Utils;

int main() {
  TestSuite suite("Modern Argument Parser Tests");

  // Date parsing tests
  suite.run_test("Date parsing - Valid ISO format", []() {
    auto result = Date::from_string("2025-01-01");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ("2025-01-01", result.value().to_string());
  });

  suite.run_test("Date parsing - Invalid format", []() {
    auto result = Date::from_string("invalid-date");
    ASSERT_FALSE(result.has_value());
    auto error_str = to_string(result.error());
    auto expected_str = to_string(ArgumentError::InvalidDateFormat);
    ASSERT_EQ(expected_str, error_str);
  });

  suite.run_test("Date parsing - Empty string", []() {
    auto result = Date::from_string("");
    ASSERT_FALSE(result.has_value());
  });

  suite.run_test("Date parsing - Different valid formats", []() {
    auto result1 = Date::from_string("2025-12-31");
    ASSERT_TRUE(result1.has_value());

    auto result2 = Date::from_string("2025-01-15");
    ASSERT_TRUE(result2.has_value());

    auto result3 = Date::from_string("2025-06-30");
    ASSERT_TRUE(result3.has_value());
  });

  suite.run_test("Date parsing - Invalid month", []() {
    auto result = Date::from_string("2025-13-01");
    ASSERT_FALSE(result.has_value());
  });

  suite.run_test("Date parsing - Invalid day", []() {
    auto result = Date::from_string("2025-01-32");
    ASSERT_FALSE(result.has_value());
  });

  // SimulationArgumentParser tests
  suite.run_test("SimulationArgumentParser - Basic usage", []() {
    const char* argv[] = {"test_program", "--date", "2025-12-31", "--verbose"};
    int argc = 4;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_TRUE(result.has_value());
    auto config = result.value();
    ASSERT_TRUE(config.target_date.has_value());
    ASSERT_EQ("2025-12-31", config.date_string);
    ASSERT_TRUE(config.verbose);
    ASSERT_FALSE(config.use_current_date);
  });

  suite.run_test("SimulationArgumentParser - No arguments", []() {
    const char* argv[] = {"test_program"};
    int argc = 1;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_TRUE(result.has_value());
    auto config = result.value();
    ASSERT_FALSE(config.verbose);
  });

  suite.run_test("SimulationArgumentParser - Verbose flag", []() {
    const char* argv[] = {"test_program", "--verbose"};
    int argc = 2;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result.value().verbose);
  });

  suite.run_test("SimulationArgumentParser - Date only", []() {
    const char* argv[] = {"test_program", "--date", "2025-06-15"};
    int argc = 3;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_TRUE(result.has_value());
    auto config = result.value();
    ASSERT_TRUE(config.target_date.has_value());
    ASSERT_EQ("2025-06-15", config.date_string);
    ASSERT_FALSE(config.verbose);
  });

  suite.run_test("SimulationArgumentParser - Invalid date argument", []() {
    const char* argv[] = {"test_program", "--date", "invalid"};
    int argc = 3;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_FALSE(result.has_value());
  });

  suite.run_test("SimulationArgumentParser - Missing date value", []() {
    const char* argv[] = {"test_program", "--date"};
    int argc = 2;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_FALSE(result.has_value());
  });

  suite.run_test("SimulationArgumentParser - Unknown flag", []() {
    const char* argv[] = {"test_program", "--unknown-flag"};
    int argc = 2;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    // Should either ignore or fail gracefully
    // Implementation dependent
  });

  suite.run_test("SimulationArgumentParser - Multiple flags", []() {
    const char* argv[] = {"test_program", "--verbose", "--date", "2025-03-20"};
    int argc = 4;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_TRUE(result.has_value());
    auto config = result.value();
    ASSERT_TRUE(config.verbose);
    ASSERT_TRUE(config.target_date.has_value());
    ASSERT_EQ("2025-03-20", config.date_string);
  });

  suite.run_test("SimulationArgumentParser - Program name", []() {
    const char* argv[] = {"my_program"};
    int argc = 1;

    SimulationArgumentParser parser("my_program");
    auto result = parser.parse(argc, argv);

    ASSERT_TRUE(result.has_value());
  });

  // ArgumentError enum tests
  suite.run_test("ArgumentError enum values", []() {
    ASSERT_TRUE(ArgumentError::InvalidDateFormat == ArgumentError::InvalidDateFormat);
    ASSERT_FALSE(ArgumentError::InvalidDateFormat == ArgumentError::MissingValue);
    ASSERT_FALSE(ArgumentError::UnknownOption == ArgumentError::InvalidValue);
  });

  // Date to_string tests
  suite.run_test("Date to_string consistency", []() {
    auto result = Date::from_string("2025-07-04");
    ASSERT_TRUE(result.has_value());

    std::string str = result.value().to_string();
    ASSERT_EQ("2025-07-04", str);
  });

  suite.run_test("Date round-trip", []() {
    std::string original = "2025-11-22";
    auto parsed = Date::from_string(original);
    ASSERT_TRUE(parsed.has_value());

    std::string round_trip = parsed.value().to_string();
    ASSERT_EQ(original, round_trip);
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
