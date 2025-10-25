/**
 * @file input_sanitizer.cpp
 * @brief Implementation of input sanitization
 */

#include "solar_core/security/input_sanitizer.hpp"

#include <algorithm>
#include <regex>

namespace SolarSystem::Security {

std::string InputSanitizer::sanitize_sql(const std::string& input) {
  std::string result = input;
  // Escape single quotes
  size_t pos = 0;
  while ((pos = result.find('\'', pos)) != std::string::npos) {
    result.replace(pos, 1, "''");
    pos += 2;
  }
  return result;
}

std::string InputSanitizer::sanitize_html(const std::string& input) {
  std::string result = input;
  // Replace dangerous HTML characters
  size_t pos = 0;
  while ((pos = result.find('<', pos)) != std::string::npos) {
    result.replace(pos, 1, "&lt;");
    pos += 4;
  }
  pos = 0;
  while ((pos = result.find('>', pos)) != std::string::npos) {
    result.replace(pos, 1, "&gt;");
    pos += 4;
  }
  pos = 0;
  while ((pos = result.find('&', pos)) != std::string::npos) {
    if (result.substr(pos, 4) != "&lt;" && result.substr(pos, 4) != "&gt;") {
      result.replace(pos, 1, "&amp;");
      pos += 5;
    } else {
      pos++;
    }
  }
  return result;
}

std::string InputSanitizer::sanitize_path(const std::string& input) {
  std::string result = input;
  // Remove path traversal patterns
  while (result.find("..") != std::string::npos) {
    result.erase(result.find(".."), 2);
  }
  return result;
}

std::string InputSanitizer::sanitize_command(const std::string& input) {
  std::string result;
  // Only allow alphanumeric, space, dash, underscore
  for (char c : input) {
    if (std::isalnum(c) || c == ' ' || c == '-' || c == '_') {
      result += c;
    }
  }
  return result;
}

ValidationResult InputSanitizer::validate_email(const std::string& email) {
  static const std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");

  ValidationResult result;
  result.is_valid = std::regex_match(email, email_regex);
  result.sanitized_value = email;

  if (!result.is_valid) {
    result.error_message = "Invalid email format";
  }

  return result;
}

ValidationResult InputSanitizer::validate_username(const std::string& username) {
  ValidationResult result;
  result.sanitized_value = username;

  if (username.length() < 3 || username.length() > 32) {
    result.error_message = "Username must be between 3 and 32 characters";
    return result;
  }

  // Only alphanumeric and underscore
  for (char c : username) {
    if (!std::isalnum(c) && c != '_') {
      result.error_message = "Username can only contain letters, numbers, and underscores";
      return result;
    }
  }

  result.is_valid = true;
  return result;
}

ValidationResult InputSanitizer::validate_password(const std::string& password, size_t min_length) {
  ValidationResult result;
  result.sanitized_value = password;

  if (password.length() < min_length) {
    result.error_message = "Password must be at least " + std::to_string(min_length) + " characters";
    return result;
  }

  // Check for at least one uppercase, lowercase, and digit
  bool has_upper = false, has_lower = false, has_digit = false;
  for (char c : password) {
    if (std::isupper(c)) has_upper = true;
    if (std::islower(c)) has_lower = true;
    if (std::isdigit(c)) has_digit = true;
  }

  if (!has_upper || !has_lower || !has_digit) {
    result.error_message = "Password must contain uppercase, lowercase, and digit";
    return result;
  }

  result.is_valid = true;
  return result;
}

ValidationResult InputSanitizer::validate_url(const std::string& url) {
  static const std::regex url_regex(R"(^https?://[^\s/$.?#].[^\s]*$)");

  ValidationResult result;
  result.is_valid = std::regex_match(url, url_regex);
  result.sanitized_value = url;

  if (!result.is_valid) {
    result.error_message = "Invalid URL format";
  }

  return result;
}

ValidationResult InputSanitizer::validate_ip_address(const std::string& ip) {
  static const std::regex ip_regex(R"(^(\d{1,3}\.){3}\d{1,3}$)");

  ValidationResult result;
  result.is_valid = std::regex_match(ip, ip_regex);
  result.sanitized_value = ip;

  if (!result.is_valid) {
    result.error_message = "Invalid IP address format";
  }

  return result;
}

ValidationResult InputSanitizer::validate_port(int port) {
  ValidationResult result;
  result.is_valid = (port > 0 && port <= 65535);
  result.sanitized_value = std::to_string(port);

  if (!result.is_valid) {
    result.error_message = "Port must be between 1 and 65535";
  }

  return result;
}

bool InputSanitizer::contains_sql_injection(const std::string& input) {
  static const std::vector<std::string> patterns = {
      "' OR '", "' AND '", "DROP TABLE", "DELETE FROM", "INSERT INTO",
      "UPDATE ", "UNION SELECT", "--", "/*", "*/"
  };

  std::string upper_input = input;
  std::transform(upper_input.begin(), upper_input.end(), upper_input.begin(), ::toupper);

  for (const auto& pattern : patterns) {
    if (upper_input.find(pattern) != std::string::npos) {
      return true;
    }
  }

  return false;
}

bool InputSanitizer::contains_xss(const std::string& input) {
  static const std::vector<std::string> patterns = {
      "<script", "javascript:", "onerror=", "onload=", "<iframe"
  };

  std::string lower_input = input;
  std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

  for (const auto& pattern : patterns) {
    if (lower_input.find(pattern) != std::string::npos) {
      return true;
    }
  }

  return false;
}

bool InputSanitizer::contains_path_traversal(const std::string& input) {
  return input.find("..") != std::string::npos ||
         input.find("./") != std::string::npos ||
         input.find("\\") != std::string::npos;
}

bool InputSanitizer::contains_command_injection(const std::string& input) {
  static const std::vector<char> dangerous_chars = {';', '|', '&', '$', '`', '\n'};

  for (char c : dangerous_chars) {
    if (input.find(c) != std::string::npos) {
      return true;
    }
  }

  return false;
}

std::string InputSanitizer::escape_special_chars(const std::string& input) {
  std::string result;
  for (char c : input) {
    if (c == '\\' || c == '\'' || c == '"') {
      result += '\\';
    }
    result += c;
  }
  return result;
}

std::string InputSanitizer::remove_dangerous_chars(const std::string& input) {
  std::string result;
  for (char c : input) {
    if (std::isalnum(c) || c == ' ' || c == '-' || c == '_' || c == '.') {
      result += c;
    }
  }
  return result;
}

ValidationResult InputSanitizer::validate_json(const std::string& json) {
  ValidationResult result;
  result.sanitized_value = json;

  // Simple JSON validation - check balanced braces
  int brace_count = 0;
  for (char c : json) {
    if (c == '{' || c == '[') brace_count++;
    if (c == '}' || c == ']') brace_count--;
  }

  result.is_valid = (brace_count == 0);
  if (!result.is_valid) {
    result.error_message = "Invalid JSON format";
  }

  return result;
}

ValidationResult InputSanitizer::validate_integer_range(int value, int min, int max) {
  ValidationResult result;
  result.is_valid = (value >= min && value <= max);
  result.sanitized_value = std::to_string(value);

  if (!result.is_valid) {
    result.error_message = "Value must be between " + std::to_string(min) +
                          " and " + std::to_string(max);
  }

  return result;
}

ValidationResult InputSanitizer::validate_string_length(
    const std::string& str,
    size_t min_length,
    size_t max_length) {
  ValidationResult result;
  result.sanitized_value = str;
  result.is_valid = (str.length() >= min_length && str.length() <= max_length);

  if (!result.is_valid) {
    result.error_message = "String length must be between " +
                          std::to_string(min_length) + " and " +
                          std::to_string(max_length);
  }

  return result;
}

// RequestValidator implementation
bool RequestValidator::is_valid_http_method(const std::string& method) {
  static const std::vector<std::string> valid_methods = {
      "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"
  };

  return std::find(valid_methods.begin(), valid_methods.end(), method) != valid_methods.end();
}

ValidationResult RequestValidator::validate_header(
    const std::string& /* name */,
    const std::string& value) {
  ValidationResult result;
  result.sanitized_value = value;

  // Check for header injection
  if (value.find('\r') != std::string::npos || value.find('\n') != std::string::npos) {
    result.error_message = "Header value contains invalid characters";
    return result;
  }

  result.is_valid = true;
  return result;
}

bool RequestValidator::is_valid_content_type(const std::string& content_type) {
  static const std::vector<std::string> valid_types = {
      "application/json",
      "application/x-www-form-urlencoded",
      "multipart/form-data",
      "text/plain",
      "text/html"
  };

  for (const auto& type : valid_types) {
    if (content_type.find(type) != std::string::npos) {
      return true;
    }
  }

  return false;
}

bool RequestValidator::is_valid_request_size(size_t size, size_t max_size) {
  return size <= max_size;
}

std::string RequestValidator::sanitize_query_params(const std::string& params) {
  return InputSanitizer::sanitize_html(params);
}

ValidationResult RequestValidator::validate_token_format(const std::string& token) {
  ValidationResult result;
  result.sanitized_value = token;

  // Token should be alphanumeric and of reasonable length
  if (token.length() < 16 || token.length() > 128) {
    result.error_message = "Invalid token length";
    return result;
  }

  for (char c : token) {
    if (!std::isalnum(c) && c != '-' && c != '_') {
      result.error_message = "Token contains invalid characters";
      return result;
    }
  }

  result.is_valid = true;
  return result;
}

}  // namespace SolarSystem::Security
