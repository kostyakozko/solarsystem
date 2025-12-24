/**
 * @file input_validator.hpp
 * @brief Comprehensive input validation system for Solar System Suite (Task 13)
 *
 * Implements requirements 5.1, 5.2, and 5.4:
 * - Detailed format checking
 * - Multiple date, time, and numeric formats
 * - Range validation with meaningful error messages
 */

#pragma once

#include <cfloat>
#include <chrono>
#include <climits>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "solar_utils/export.hpp"

namespace SolarSystem::Utils::Validation {

/**
 * @brief Validation result with detailed error information
 */
struct ValidationResult {
  bool is_valid = false;
  std::string error_message;
  std::vector<std::string> suggestions;
  std::vector<std::string> expected_formats;
  std::string normalized_value;

  ValidationResult() = default;

  // Constructor for successful validation
  ValidationResult(bool valid, const std::string& value = "")
      : is_valid(valid), normalized_value(value) {}

  // Constructor for failed validation with error message
  explicit ValidationResult(const std::string& error, const std::vector<std::string>& formats = {})
      : is_valid(false), error_message(error), expected_formats(formats) {}
};

/**
 * @brief Date/time validator with multiple format support (Requirement 5.2)
 */
class SOLAR_UTILS_API DateTimeValidator {
 public:
  /**
   * @brief Parse date with automatic format detection
   */
  static ValidationResult validate_date(const std::string& date_str);

  /**
   * @brief Parse date with range validation (Requirement 5.4)
   */
  static ValidationResult validate_date_with_range(
      const std::string& date_str, const std::chrono::system_clock::time_point& min_date,
      const std::chrono::system_clock::time_point& max_date);

  /**
   * @brief Validate date with timezone information
   */
  static ValidationResult validate_date_with_timezone(
      const std::string& date_str, const std::string& timezone = "UTC");

  /**
   * @brief Validate leap year
   */
  static bool is_leap_year(int year);

  /**
   * @brief Validate day of month for given year and month
   */
  static bool is_valid_day_of_month(int year, int month, int day);

  /**
   * @brief Convert between calendar systems (Gregorian/Julian)
   */
  static ValidationResult convert_calendar_system(
      const std::string& date_str, const std::string& from_calendar,
      const std::string& to_calendar);

  /**
   * @brief Get supported date formats
   */
  static std::vector<std::string> get_supported_formats();

 private:
  static const std::vector<std::regex> date_patterns_;
  static const std::vector<std::string> format_descriptions_;
};

/**
 * @brief Numeric validator with range checking (Requirements 5.1, 5.4)
 */
class SOLAR_UTILS_API NumericValidator {
 public:
  /**
   * @brief Validate integer with range checking
   */
  static ValidationResult validate_int(const std::string& str, int min_val = INT_MIN,
                                       int max_val = INT_MAX);

  /**
   * @brief Validate double with range checking
   */
  static ValidationResult validate_double(const std::string& str, double min_val = -DBL_MAX,
                                          double max_val = DBL_MAX);

  /**
   * @brief Validate port number (common use case)
   */
  static ValidationResult validate_port(const std::string& str);

  /**
   * @brief Validate year (common use case)
   */
  static ValidationResult validate_year(const std::string& str);
};

/**
 * @brief String validator with pattern matching (Requirement 5.1)
 */
class SOLAR_UTILS_API StringValidator {
 public:
  /**
   * @brief Validate string against allowed values
   */
  static ValidationResult validate_choice(const std::string& str,
                                          const std::vector<std::string>& allowed_values);

  /**
   * @brief Validate email format
   */
  static ValidationResult validate_email(const std::string& str);

  /**
   * @brief Validate file path
   */
  static ValidationResult validate_file_path(const std::string& str, bool must_exist = false);

  /**
   * @brief Sanitize input string
   */
  static ValidationResult sanitize_input(const std::string& str);

  /**
   * @brief Validate JSON syntax and structure
   */
  static ValidationResult validate_json(const std::string& str);
};

/**
 * @brief Input validator that combines all validation types
 */
class SOLAR_UTILS_API InputValidator {
 public:
  /**
   * @brief Validate argument based on expected type
   */
  static ValidationResult validate_argument(const std::string& arg_name, const std::string& value,
                                            const std::string& expected_type);

  /**
   * @brief Get validation suggestions for invalid input
   */
  static std::vector<std::string> get_suggestions(const std::string& invalid_input,
                                                  const std::vector<std::string>& valid_options);
};

}  // namespace SolarSystem::Utils::Validation
