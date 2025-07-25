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
    ASSERT_EQ(to_string(result.error()), to_string(ArgumentError::InvalidDateFormat));
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

  return current_suite->all_passed() ? 0 : 1;
}
