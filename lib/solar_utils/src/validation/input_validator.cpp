/**
 * @file input_validator.cpp
 * @brief Implementation of comprehensive input validation system
 */

#include "solar_utils/validation/input_validator.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <chrono>
#include <climits>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace SolarSystem::Utils::Validation {

// DateTimeValidator static members
const std::vector<std::regex> DateTimeValidator::date_patterns_ = {
    std::regex(R"(^(\d{4})-(\d{2})-(\d{2})$)"),      // ISO 8601: YYYY-MM-DD
    std::regex(R"(^(\d{1,2})/(\d{1,2})/(\d{4})$)"),  // US format: MM/DD/YYYY
    std::regex(R"(^(\d{1,2})/(\d{1,2})/(\d{4})$)"),  // European: DD/MM/YYYY (same pattern, context
                                                     // dependent)
    std::regex(R"(^(\d{8})$)"),                      // Compact: YYYYMMDD
    std::regex(R"(^(today|tomorrow|yesterday|now)$)", std::regex::icase),  // Named dates
    std::regex(R"(^([+-]?\d+)\s*days?$)", std::regex::icase)               // Relative: +/-N days
};

const std::vector<std::string> DateTimeValidator::format_descriptions_ = {
    "YYYY-MM-DD (ISO 8601)", "MM/DD/YYYY (US format)",          "DD/MM/YYYY (European format)",
    "YYYYMMDD (compact)",    "today, tomorrow, yesterday, now", "+/-N days (relative)"};

// DateTimeValidator implementation
ValidationResult DateTimeValidator::validate_date(const std::string& date_str) {
  std::string trimmed = date_str;
  // Trim whitespace
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed.empty()) {
    return ValidationResult("Empty date string", get_supported_formats());
  }

  // Try eachtsern
  for (size_t i = 0; i < date_patterns_.size(); ++i) {
    std::smatch matches;
    if (std::regex_match(trimmed, matches, date_patterns_[i])) {
      try {
        // Basic validation - just check if we can parse it
        if (i == 0) {  // ISO format
          int year = std::stoi(matches[1]);
          int month = std::stoi(matches[2]);
          int day = std::stoi(matches[3]);

          if (year < 1600 || year > 2200 || month < 1 || month > 12 || day < 1 || day > 31) {
            ValidationResult result;
            result.is_valid = false;
            result.error_message = "Date values out of valid range";
            result.expected_formats = get_supported_formats();
            return result;
          }
        } else if (i == 4) {  // Named dates
          // Always valid
        } else if (i == 5) {  // Relative dates
          int days = std::stoi(matches[1]);
          if (std::abs(days) > 36500) {  // ~100 years
            ValidationResult result;
            result.is_valid = false;
            result.error_message = "Relative date too far in the future/past";
            result.expected_formats = get_supported_formats();
            return result;
          }
        }

        return ValidationResult(true, trimmed);
      } catch (const std::exception& e) {
        continue;  // Try next pattern
      }
    }
  }

  // No pattern matched - provide suggestions
  ValidationResult result;
  result.is_valid = false;
  result.error_message = "Invalid date format";
  result.expected_formats = get_supported_formats();

  // Simple suggestions
  if (date_str.find('/') != std::string::npos) {
    std::string iso_suggestion = date_str;
    std::replace(iso_suggestion.begin(), iso_suggestion.end(), '/', '-');
    result.suggestions.push_back(iso_suggestion);
  }

  // Add current date as suggestion
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::tm* tm_now = std::gmtime(&time_t_now);
  if (tm_now) {
    std::ostringstream oss;
    oss << std::put_time(tm_now, "%Y-%m-%d");
    result.suggestions.push_back(oss.str() + " (today)");
  }

  return result;
}

ValidationResult DateTimeValidator::validate_date_with_range(
    const std::string& date_str,
    const std::chrono::system_clock::time_point& min_date,
    const std::chrono::system_clock::time_point& max_date) {

  auto basic_result = validate_date(date_str);
  if (!basic_result.is_valid) {
    return basic_result;
  }

  // Parse the date string to a time_point for range checking
  std::tm tm = {};
  std::istringstream ss(basic_result.normalized_value);

  // Try ISO format first (YYYY-MM-DD)
  ss >> std::get_time(&tm, "%Y-%m-%d");

  if (ss.fail()) {
    // Try other formats
    ss.clear();
    ss.str(basic_result.normalized_value);
    ss >> std::get_time(&tm, "%Y/%m/%d");
  }

  if (ss.fail()) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Could not parse date for range validation";
    return result;
  }

  // Convert to time_point
  auto parsed_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));

  // Check range
  if (parsed_time < min_date) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Date is before minimum allowed date";

    // Format min_date for error message
    auto min_time_t = std::chrono::system_clock::to_time_t(min_date);
    std::tm* min_tm = std::gmtime(&min_time_t);
    if (min_tm) {
      std::ostringstream oss;
      oss << std::put_time(min_tm, "%Y-%m-%d");
      result.error_message += " (" + oss.str() + ")";
    }

    return result;
  }

  if (parsed_time > max_date) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Date is after maximum allowed date";

    // Format max_date for error message
    auto max_time_t = std::chrono::system_clock::to_time_t(max_date);
    std::tm* max_tm = std::gmtime(&max_time_t);
    if (max_tm) {
      std::ostringstream oss;
      oss << std::put_time(max_tm, "%Y-%m-%d");
      result.error_message += " (" + oss.str() + ")";
    }

    return result;
  }

  // Date is within range
  return basic_result;
}

/**
 * @brief Validate date with timezone information
 */
ValidationResult DateTimeValidator::validate_date_with_timezone(
    const std::string& date_str, const std::string& timezone) {

  auto basic_result = validate_date(date_str);
  if (!basic_result.is_valid) {
    return basic_result;
  }

  // Validate timezone string
  // Common timezone formats: UTC, GMT, EST, PST, +0000, -0500, etc.
  std::regex tz_pattern(R"(^(UTC|GMT|[A-Z]{3}|[+-]\d{4}|[+-]\d{2}:\d{2})$)", std::regex::icase);

  if (!std::regex_match(timezone, tz_pattern)) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Invalid timezone format: " + timezone;
    result.suggestions.push_back("UTC");
    result.suggestions.push_back("GMT");
    result.suggestions.push_back("+0000");
    result.suggestions.push_back("-0500");
    return result;
  }

  // Timezone is valid
  ValidationResult result = basic_result;
  result.normalized_value += " " + timezone;
  return result;
}

/**
 * @brief Check if year is a leap year
 */
bool DateTimeValidator::is_leap_year(int year) {
  // Leap year rules:
  // 1. Divisible by 4
  // 2. If divisible by 100, must also be divisible by 400
  if (year % 4 != 0) {
    return false;
  }
  if (year % 100 != 0) {
    return true;
  }
  return (year % 400 == 0);
}

/**
 * @brief Validate day of month for given year and month
 */
bool DateTimeValidator::is_valid_day_of_month(int year, int month, int day) {
  if (month < 1 || month > 12) {
    return false;
  }

  if (day < 1) {
    return false;
  }

  // Days in each month
  static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  int max_day = days_in_month[month - 1];

  // Adjust for leap year in February
  if (month == 2 && is_leap_year(year)) {
    max_day = 29;
  }

  return day <= max_day;
}

/**
 * @brief Convert between calendar systems
 */
ValidationResult DateTimeValidator::convert_calendar_system(
    const std::string& date_str,
    const std::string& from_calendar,
    const std::string& to_calendar) {

  // Validate input date
  auto basic_result = validate_date(date_str);
  if (!basic_result.is_valid) {
    return basic_result;
  }

  // Parse the date
  std::tm tm = {};
  std::istringstream ss(basic_result.normalized_value);
  ss >> std::get_time(&tm, "%Y-%m-%d");

  if (ss.fail()) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Could not parse date for calendar conversion";
    return result;
  }

  int year = tm.tm_year + 1900;
  int month = tm.tm_mon + 1;
  int day = tm.tm_mday;

  // Support Gregorian and Julian calendars
  if (from_calendar == "Gregorian" && to_calendar == "Julian") {
    // Gregorian to Julian conversion
    // The Julian calendar is 13 days behind the Gregorian (as of 1900-2099)
    // This is a simplified conversion

    // Calculate Julian Day Number for Gregorian date
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    int jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;

    // Convert back to Julian calendar
    (void)0; // Placeholder
    int c = jdn + 32082;
    int d = (4 * c + 3) / 1461;
    int e = c - (1461 * d) / 4;
    int f = (5 * e + 2) / 153;

    int julian_day = e - (153 * f + 2) / 5 + 1;
    int julian_month = f + 3 - 12 * (f / 10);
    int julian_year = d - 4800 + f / 10;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << julian_year << "-"
        << std::setw(2) << julian_month << "-"
        << std::setw(2) << julian_day;

    ValidationResult result;
    result.is_valid = true;
    result.normalized_value = oss.str();
    return result;

  } else if (from_calendar == "Julian" && to_calendar == "Gregorian") {
    // Julian to Gregorian conversion
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    int jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - 32083;

    // Convert to Gregorian
    int b = jdn + 32044;
    int c = (4 * b + 3) / 146097;
    int d = b - (146097 * c) / 4;
    int e = (4 * d + 3) / 1461;
    int f = d - (1461 * e) / 4;
    int g = (5 * f + 2) / 153;

    int greg_day = f - (153 * g + 2) / 5 + 1;
    int greg_month = g + 3 - 12 * (g / 10);
    int greg_year = 100 * c + e - 4800 + g / 10;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << greg_year << "-"
        << std::setw(2) << greg_month << "-"
        << std::setw(2) << greg_day;

    ValidationResult result;
    result.is_valid = true;
    result.normalized_value = oss.str();
    return result;

  } else if (from_calendar == to_calendar) {
    // No conversion needed
    return basic_result;

  } else {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Unsupported calendar conversion: " + from_calendar + " to " + to_calendar;
    result.suggestions.push_back("Gregorian");
    result.suggestions.push_back("Julian");
    return result;
  }
}

std::vector<std::string> DateTimeValidator::get_supported_formats() { return format_descriptions_; }

// NumericValidator implementation
ValidationResult NumericValidator::validate_int(const std::string& str, int min_val, int max_val) {
  try {
    // Remove whitespace
    std::string trimmed = str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) {
      ValidationResult result;
      result.is_valid = false;
      result.error_message = "Empty numeric string";
      return result;
    }

    int value = std::stoi(trimmed);

    if (value < min_val || value > max_val) {
      std::ostringstream oss;
      oss << "Value " << value << " is outside valid range [" << min_val << ", " << max_val << "]";
      ValidationResult result;
      result.is_valid = false;
      result.error_message = oss.str();
      return result;
    }

    return ValidationResult(true, std::to_string(value));

  } catch (const std::exception& e) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Invalid integer format: " + std::string(e.what());

    // Suggest cleaned version
    std::string cleaned = str;
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(),
                                 [](char c) { return !std::isdigit(c) && c != '-' && c != '+'; }),
                  cleaned.end());
    if (!cleaned.empty() && cleaned != str) {
      result.suggestions.push_back(cleaned);
    }

    return result;
  }
}

ValidationResult NumericValidator::validate_double(const std::string& str, double min_val,
                                                   double max_val) {
  try {
    std::string trimmed = str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) {
      ValidationResult result;
      result.is_valid = false;
      result.error_message = "Empty numeric string";
      return result;
    }

    double value = std::stod(trimmed);

    if (value < min_val || value > max_val) {
      std::ostringstream oss;
      oss << "Value " << value << " is outside valid range [" << min_val << ", " << max_val << "]";
      ValidationResult result;
      result.is_valid = false;
      result.error_message = oss.str();
      return result;
    }

    return ValidationResult(true, std::to_string(value));

  } catch (const std::exception& e) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Invalid number format: " + std::string(e.what());

    // Suggest cleaned version
    std::string cleaned = str;
    cleaned.erase(
        std::remove_if(cleaned.begin(), cleaned.end(),
                       [](char c) { return !std::isdigit(c) && c != '.' && c != '-' && c != '+'; }),
        cleaned.end());
    if (!cleaned.empty() && cleaned != str) {
      result.suggestions.push_back(cleaned);
    }

    return result;
  }
}

ValidationResult NumericValidator::validate_port(const std::string& str) {
  return validate_int(str, 1024, 65535);
}

ValidationResult NumericValidator::validate_year(const std::string& str) {
  return validate_int(str, 1600, 2200);
}

// StringValidator implementation
ValidationResult StringValidator::validate_choice(const std::string& str,
                                                  const std::vector<std::string>& allowed_values) {
  std::string trimmed = str;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  // Case-insensitive search
  for (const auto& allowed : allowed_values) {
    if (std::equal(trimmed.begin(), trimmed.end(), allowed.begin(), allowed.end(),
                   [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
      return ValidationResult(true, allowed);  // Return the canonical form
    }
  }

  ValidationResult result;
  result.is_valid = false;
  result.error_message = "Value not in allowed list";
  result.expected_formats = allowed_values;

  // Find similar values
  for (const auto& allowed : allowed_values) {
    if (allowed.find(trimmed) != std::string::npos || trimmed.find(allowed) != std::string::npos) {
      result.suggestions.push_back(allowed);
    }
  }

  return result;
}

ValidationResult StringValidator::validate_email(const std::string& str) {
  std::regex email_pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");

  if (std::regex_match(str, email_pattern)) {
    return ValidationResult(true, str);
  }

  ValidationResult result;
  result.is_valid = false;
  result.error_message = "Invalid email format";
  result.expected_formats = {"user@example.com"};
  return result;
}

ValidationResult StringValidator::validate_file_path(const std::string& str, bool must_exist) {
  if (str.empty()) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Empty file path";
    return result;
  }

  // Check for dangerous patterns
  if (str.find("..") != std::string::npos) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "File path contains dangerous '..' sequence";
    return result;
  }

  if (must_exist) {
    std::filesystem::path path(str);
    if (!std::filesystem::exists(path)) {
      ValidationResult result;
      result.is_valid = false;
      result.error_message = "File or directory does not exist";
      return result;
    }
  }

  return ValidationResult(true, str);
}

ValidationResult StringValidator::sanitize_input(const std::string& str) {
  std::string result = str;

  // Remove control characters (ASCII 0-31 except tab, newline, carriage return)
  result.erase(std::remove_if(
                   result.begin(), result.end(),
                   [](char c) {
                     auto uc = static_cast<unsigned char>(c);
                     return uc < 32 && c != '\t' && c != '\n' && c != '\r';
                   }),
               result.end());

  // Trim whitespace
  result.erase(0, result.find_first_not_of(" \t\n\r"));
  result.erase(result.find_last_not_of(" \t\n\r") + 1);

  return ValidationResult(true, result);
}

ValidationResult StringValidator::validate_json(const std::string& str) {
  if (str.empty()) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Empty JSON string";
    result.expected_formats = {"{ \"key\": \"value\" }"};
    return result;
  }

  // Trim whitespace
  std::string trimmed = str;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed.empty()) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Empty JSON content after trimming whitespace";
    result.expected_formats = {"{ \"key\": \"value\" }"};
    return result;
  }

  // Basic JSON structure validation
  if (!trimmed.starts_with('{') || !trimmed.ends_with('}')) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "JSON must start with '{' and end with '}'";
    result.expected_formats = {"{ \"key\": \"value\" }"};
    result.suggestions.push_back("Ensure your JSON is wrapped in curly braces");
    return result;
  }

  // Simplified JSON validation - check for common syntax errors
  int brace_count = 0;
  int bracket_count = 0;
  bool in_string = false;
  bool escaped = false;
  size_t line = 1;
  size_t column = 1;

  for (size_t i = 0; i < trimmed.length(); ++i) {
    char c = trimmed[i];

    if (c == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }

    if (in_string) {
      if (escaped) {
        escaped = false;
        continue;
      }
      if (c == '\\') {
        escaped = true;
        continue;
      }
      if (c == '"') {
        in_string = false;
      }
      continue;
    }

    switch (c) {
      case '"':
        in_string = true;
        break;
      case '{':
        brace_count++;
        break;
      case '}':
        brace_count--;
        if (brace_count < 0) {
          ValidationResult result;
          result.is_valid = false;
          result.error_message = "Unexpected '}' at line " + std::to_string(line) + ", column " + std::to_string(column);
          result.suggestions.push_back("Check for mismatched braces");
          return result;
        }
        break;
      case '[':
        bracket_count++;
        break;
      case ']':
        bracket_count--;
        if (bracket_count < 0) {
          ValidationResult result;
          result.is_valid = false;
          result.error_message = "Unexpected ']' at line " + std::to_string(line) + ", column " + std::to_string(column);
          result.suggestions.push_back("Check for mismatched brackets");
          return result;
        }
        break;
    }
  }

  if (brace_count != 0) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Mismatched braces - missing " + std::to_string(brace_count) + " closing brace(s)";
    result.suggestions.push_back("Ensure all '{' have matching '}'");
    return result;
  }

  if (bracket_count != 0) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Mismatched brackets - missing " + std::to_string(bracket_count) + " closing bracket(s)";
    result.suggestions.push_back("Ensure all '[' have matching ']'");
    return result;
  }

  if (in_string) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Unterminated string";
    result.suggestions.push_back("Ensure all strings are properly closed with quotes");
    return result;
  }

  // Check for common JSON syntax errors using simple string patterns
  // Look for missing commas (simplified check)
  if (trimmed.find("}\n  \"") != std::string::npos ||
      trimmed.find("}\r\n  \"") != std::string::npos ||
      trimmed.find("} \"") != std::string::npos) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Missing comma between key-value pairs";
    result.suggestions.push_back("Add commas between key-value pairs in JSON objects");
    return result;
  }

  // Look for missing commas between properties (improved pattern)
  // Check for pattern: value followed by newline and then key without comma
  size_t pos = 0;
  while ((pos = trimmed.find('\n', pos)) != std::string::npos) {
    // Skip whitespace after newline
    size_t next_pos = pos + 1;
    while (next_pos < trimmed.length() && (trimmed[next_pos] == ' ' || trimmed[next_pos] == '\t')) {
      next_pos++;
    }

    // Check if we have a quote (start of key) after whitespace
    if (next_pos < trimmed.length() && trimmed[next_pos] == '"') {
      // Look backwards from the newline to find the last non-whitespace character
      size_t prev_pos = pos - 1;
      while (prev_pos > 0 && (trimmed[prev_pos] == ' ' || trimmed[prev_pos] == '\t' || trimmed[prev_pos] == '\r')) {
        prev_pos--;
      }

      // If the last character before newline is not a comma or opening brace, we have an error
      if (prev_pos > 0 && trimmed[prev_pos] != ',' && trimmed[prev_pos] != '{') {
        ValidationResult result;
        result.is_valid = false;
        result.error_message = "Missing comma between object properties";
        result.suggestions.push_back("Add commas between object properties in JSON");
        return result;
      }
    }
    pos++;
  }

  return ValidationResult(true, trimmed);
}

// InputValidator implementation
ValidationResult InputValidator::validate_argument(const std::string& ,
                                                   const std::string& value,
                                                   const std::string& expected_type) {
  if (expected_type == "date") {
    return DateTimeValidator::validate_date(value);
  } else if (expected_type == "int") {
    return NumericValidator::validate_int(value);
  } else if (expected_type == "double" || expected_type == "float") {
    return NumericValidator::validate_double(value);
  } else if (expected_type == "port") {
    return NumericValidator::validate_port(value);
  } else if (expected_type == "year") {
    return NumericValidator::validate_year(value);
  } else if (expected_type == "email") {
    return StringValidator::validate_email(value);
  } else if (expected_type == "file") {
    return StringValidator::validate_file_path(value);
  } else {
    // Default string validation
    return StringValidator::sanitize_input(value);
  }
}

std::vector<std::string> InputValidator::get_suggestions(
    const std::string& invalid_input, const std::vector<std::string>& valid_options) {
  std::vector<std::string> suggestions;

  for (const auto& option : valid_options) {
    // Simple similarity check - substring matching
    if (option.find(invalid_input) != std::string::npos ||
        invalid_input.find(option) != std::string::npos) {
      suggestions.push_back(option);
      continue;
    }

    // Simple edit distance check for typos like "Earht" -> "Earth"
    if (invalid_input.length() == option.length()) {
      int differences = 0;
      for (size_t i = 0; i < invalid_input.length(); ++i) {
        if (std::tolower(invalid_input[i]) != std::tolower(option[i])) {
          differences++;
        }
      }
      // If only 1-2 characters are different, suggest it
      if (differences <= 2) {
        suggestions.push_back(option);
      }
    }
  }

  return suggestions;
}

}  // namespace SolarSystem::Utils::Validation
