/**
 * @file input_sanitizer.hpp
 * @brief Input validation and sanitization for security
 *
 * Provides comprehensive input validation to prevent:
 * - SQL injection
 * - XSS attacks
 * - Path traversal
 * - Command injection
 * - Buffer overflows
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "solar_core/export.hpp"

namespace SolarSystem::Security {

/**
 * @brief Input validation result
 */
struct ValidationResult {
  bool is_valid = false;
  std::string error_message;
  std::string sanitized_value;
};

/**
 * @brief Input sanitizer for security
 */
class SOLAR_CORE_API InputSanitizer {
 public:
  /**
   * @brief Sanitize string for SQL
   */
  [[nodiscard]] static std::string sanitize_sql(const std::string& input);

  /**
   * @brief Sanitize string for HTML/XSS prevention
   */
  [[nodiscard]] static std::string sanitize_html(const std::string& input);

  /**
   * @brief Sanitize file path
   */
  [[nodiscard]] static std::string sanitize_path(const std::string& input);

  /**
   * @brief Sanitize command line argument
   */
  [[nodiscard]] static std::string sanitize_command(const std::string& input);

  /**
   * @brief Validate email address
   */
  [[nodiscard]] static ValidationResult validate_email(const std::string& email);

  /**
   * @brief Validate username
   */
  [[nodiscard]] static ValidationResult validate_username(const std::string& username);

  /**
   * @brief Validate password strength
   */
  [[nodiscard]] static ValidationResult validate_password(const std::string& password,
                                                          size_t min_length = 8);

  /**
   * @brief Validate URL
   */
  [[nodiscard]] static ValidationResult validate_url(const std::string& url);

  /**
   * @brief Validate IP address
   */
  [[nodiscard]] static ValidationResult validate_ip_address(const std::string& ip);

  /**
   * @brief Validate port number
   */
  [[nodiscard]] static ValidationResult validate_port(int port);

  /**
   * @brief Check for SQL injection patterns
   */
  [[nodiscard]] static bool contains_sql_injection(const std::string& input);

  /**
   * @brief Check for XSS patterns
   */
  [[nodiscard]] static bool contains_xss(const std::string& input);

  /**
   * @brief Check for path traversal patterns
   */
  [[nodiscard]] static bool contains_path_traversal(const std::string& input);

  /**
   * @brief Check for command injection patterns
   */
  [[nodiscard]] static bool contains_command_injection(const std::string& input);

  /**
   * @brief Escape special characters
   */
  [[nodiscard]] static std::string escape_special_chars(const std::string& input);

  /**
   * @brief Remove dangerous characters
   */
  [[nodiscard]] static std::string remove_dangerous_chars(const std::string& input);

  /**
   * @brief Validate JSON string
   */
  [[nodiscard]] static ValidationResult validate_json(const std::string& json);

  /**
   * @brief Validate integer range
   */
  [[nodiscard]] static ValidationResult validate_integer_range(int value, int min, int max);

  /**
   * @brief Validate string length
   */
  [[nodiscard]] static ValidationResult validate_string_length(const std::string& str,
                                                               size_t min_length,
                                                               size_t max_length);
};

/**
 * @brief Request validator for HTTP requests
 */
class RequestValidator {
 public:
  /**
   * @brief Validate HTTP method
   */
  [[nodiscard]] static bool is_valid_http_method(const std::string& method);

  /**
   * @brief Validate HTTP header
   */
  [[nodiscard]] static ValidationResult validate_header(const std::string& name,
                                                        const std::string& value);

  /**
   * @brief Validate content type
   */
  [[nodiscard]] static bool is_valid_content_type(const std::string& content_type);

  /**
   * @brief Validate request size
   */
  [[nodiscard]] static bool is_valid_request_size(size_t size,
                                                  size_t max_size = 10485760);  // 10MB default

  /**
   * @brief Sanitize query parameters
   */
  [[nodiscard]] static std::string sanitize_query_params(const std::string& params);

  /**
   * @brief Validate authentication token format
   */
  [[nodiscard]] static ValidationResult validate_token_format(const std::string& token);
};

}  // namespace SolarSystem::Security
