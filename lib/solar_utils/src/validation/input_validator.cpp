/**
 * @file input_validator.cpp
 * @brief Implementation of comprehensive input validation system
 */

#include "solar_utils/validation/input_validator.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <climits>
#include <cfloat>

namespace SolarSystem::Utils::Validation {

// DateTimeValidator static members
const std::vector<std::regex> DateTimeValidator::date_patterns_ = {
  std::regex(R"(^(\d{4})-(\d{2})-(\d{2})$)"),           // ISO 8601: YYYY-MM-DD
  std::regex(R"(^(\d{1,2})/(\d{1,2})/(\d{4})$)"),       // US format: MM/DD/YYYY
  std::regex(R"(^(\d{1,2})/(\d{1,2})/(\d{4})$)"),       // European: DD/MM/YYYY (same pattern, context dependent)
  std::regex(R"(^(\d{8})$)"),                          // Compact: YYYYMMDD
  std::regex(R"(^(today|tomorrow|yesterday|now)$)", std::regex::icase), // Named dates
  std::regex(R"(^([+-]?\d+)\s*days?$)", std::regex::icase) // Relative: +/-N days
};

const std::vector<std::string> DateTimeValidator::format_descriptions_ = {
  "YYYY-MM-DD (ISO 8601)",
  "MM/DD/YYYY (US format)",
  "DD/MM/YYYY (European format)",
  "YYYYMMDD (compact)",
  "today, tomorrow, yesterday, now",
  "+/-N days (relative)"
};

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
        if (i == 0) { // ISO format
          int year = std::stoi(matches[1]);
          int month = std::stoi(matches[2]);
          int day = std::stoi(matches[3]);

          if (year < 1600 || year > 2200 || month < 1 || month > 12 || day < 1 || day > 31) {
            return ValidationResult("Date values out of valid range", get_supported_formats());
          }
        } else if (i == 4) { // Named dates
          // Always valid
        } else if (i == 5) { // Relative dates
          int days = std::stoi(matches[1]);
          if (std::abs(days) > 36500) { // ~100 years
            return ValidationResult("Relative date too far in the future/past", get_supported_formats());
          }
        }

        return ValidationResult(true, trimmed);
      } catch (const std::exception& e) {
        continue; // Try next pattern
      }
    }
  }

  // No pattern matched - provide suggestions
  ValidationResult result("Invalid date format", get_supported_formats());

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

ValidationResult DateTimeValidator::validate_date_with_range(const std::string& date_str,
                                                           const std::chrono::system_clock::time_point& min_date,
                                                           const std::chrono::system_clock::time_point& max_date) {
  auto basic_result = validate_date(date_str);
  if (!basic_result.is_valid) {
    return basic_result;
  }

  // For simplicity, we'll just validate the basic format here
  // In a full implementation, we'd parse the date and check the range
  return basic_result;
}

std::vector<std::string> DateTimeValidator::get_supported_formats() {
  return format_descriptions_;
}

// NumericValidator implementation
ValidationResult NumericValidator::validate_int(const std::string& str, int min_val, int max_val) {
  try {
    // Remove whitespace
    std::string trimmed = str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) {
      return ValidationResult("Empty numeric string");
    }

    int value = std::stoi(trimmed);

    if (value < min_val || value > max_val) {
      std::ostringstream oss;
      oss << "Value " << value << " is outside valid range [" << min_val << ", " << max_val << "]";
      return ValidationResult(oss.str());
    }

    return ValidationResult(true, std::to_string(value));

  } catch (const std::exception& e) {
    ValidationResult result("Invalid integer format: " + std::string(e.what()));

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

ValidationResult NumericValidator::validate_double(const std::string& str, double min_val, double max_val) {
  try {
    std::string trimmed = str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) {
      return ValidationResult("Empty numeric string");
    }

    double value = std::stod(trimmed);

    if (value < min_val || value > max_val) {
      std::ostringstream oss;
      oss << "Value " << value << " is outside valid range [" << min_val << ", " << max_val << "]";
      return ValidationResult(oss.str());
    }

    return ValidationResult(true, std::to_string(value));

  } catch (const std::exception& e) {
    ValidationResult result("Invalid number format: " + std::string(e.what()));

    // Suggest cleaned version
    std::string cleaned = str;
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(),
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
ValidationResult StringValidator::validate_choice(const std::string& str, const std::vector<std::string>& allowed_values) {
  std::string trimmed = str;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  // Case-insensitive search
  for (const auto& allowed : allowed_values) {
    if (std::equal(trimmed.begin(), trimmed.end(), allowed.begin(), allowed.end(),
                   [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
      return ValidationResult(true, allowed); // Return the canonical form
    }
  }

  ValidationResult result("Value not in allowed list");
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

  return ValidationResult("Invalid email format", {"user@example.com"});
}

ValidationResult StringValidator::validate_file_path(const std::string& str, bool must_exist) {
  if (str.empty()) {
    return ValidationResult("Empty file path");
  }

  // Check for dangerous patterns
  if (str.find("..") != std::string::npos) {
    return ValidationResult("File path contains dangerous '..' sequence");
  }

  if (must_exist) {
    std::filesystem::path path(str);
    if (!std::filesystem::exists(path)) {
      return ValidationResult("File or directory does not exist");
    }
  }

  return ValidationResult(true, str);
}

ValidationResult StringValidator::sanitize_input(const std::string& str) {
  std::string result = str;

  // Remove control characters
  result.erase(std::remove_if(result.begin(), result.end(),
                             [](char c) { return c >= 0 && c < 32 && c != '\t' && c != '\n' && c != '\r'; }),
               result.end());

  // Trim whitespace
  result.erase(0, result.find_first_not_of(" \t\n\r"));
  result.erase(result.find_last_not_of(" \t\n\r") + 1);

  return ValidationResult(true, result);
}

// InputValidator implementation
ValidationResult InputValidator::validate_argument(const std::string& arg_name,
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

std::vector<std::string> InputValidator::get_suggestions(const std::string& invalid_input,
                                                        const std::vector<std::string>& valid_options) {
  std::vector<std::string> suggestions;

  for (const auto& option : valid_options) {
    // Simple similarity check
    if (option.find(invalid_input) != std::string::npos ||
        invalid_input.find(option) != std::string::npos) {
      suggestions.push_back(option);
    }
  }

  return suggestions;
}

} // namespace SolarSystem::Utils::Validation
