/**
 * @file test_args.cpp
 * @brief Unit tests for modern C++20 argument parsing
 */

#include "solar_utils/argument_parser.hpp"
#include "test_framework.h"

using namespace SolarSystem::Utils;

int main() {
  TEST_SUITE("Modern Argument Parser Tests");

  TEST_CASE("Date parsing") {
    auto result = Date::from_string("2025-01-01");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().to_string(), "2025-01-01");
  });

  TEST_CASE("Invalid date format") {
    auto result = Date::from_string("invalid-date");
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), ArgumentError::InvalidDateFormat);
  });

  TEST_CASE("SimulationArgumentParser basic usage") {
    const char* argv[] = {"test_program", "--date", "2025-12-31", "--verbose"};
    int argc = 4;

    SimulationArgumentParser parser("test_program");
    auto result = parser.parse(argc, argv);

    ASSERT_TRUE(result.has_value());
    auto config = result.value();
    ASSERT_TRUE(config.target_date.has_value());
    ASSERT_EQ(config.date_string, "2025-12-31");
    ASSERT_TRUE(config.verbose);
    ASSERT_FALSE(config.use_current_date);
  });

  TEST_CASE("Legacy compatibility") {
    const char* argv[] = {"test_program", "--date", "2025-01-01"};
    int argc = 3;

    // Test legacy parse_arguments function
    auto args = Legacy::parse_arguments(argc, const_cast<char**>(argv));
    ASSERT_FALSE(args.use_current_date);
    ASSERT_EQ(args.date_string, "2025-01-01");
  });

  return current_suite->all_passed() ? 0 : 1;
}
