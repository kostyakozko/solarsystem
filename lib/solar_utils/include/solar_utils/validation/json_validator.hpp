/**
 * @file json_validator.hpp
 * @brief Comprehensive JSON Validation System
 *
 * Provides robust JSON validation including:
 * - JSON schema validation and enforcement
 * - JSON structure and type checking
 * - JSON security validation and sanitization
 * - JSON performance optimization and streaming
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace SolarSystem::Utils::Validation {

/**
 * @brief JSON data types
 */
enum class JSONType {
  Null,
  Boolean,
  Number,
  String,
  Array,
  Object
};

/**
 * @brief JSON validation error
 */
struct JSONValidationError {
  std::string message;
  size_t line = 0;
  size_t column = 0;
  std::string path;  // JSON path to error location
};

/**
 * @brief JSON validation result
 */
struct JSONValidationResult {
  bool is_valid = false;
  std::vector<JSONValidationError> errors;
  std::vector<std::string> warnings;
  std::string sanitized_json;

  JSONValidationResult() = default;
  explicit JSONValidationResult(bool valid) : is_valid(valid) {}
};

/**
 * @brief JSON schema definition
 */
struct JSONSchema {
  JSONType type;
  bool required = false;
  std::optional<std::string> pattern;  // For string validation
  std::optional<double> minimum;       // For number validation
  std::optional<double> maximum;       // For number validation
  std::optional<size_t> min_length;    // For string/array validation
  std::optional<size_t> max_length;    // For string/array validation
  std::unordered_map<std::string, JSONSchema> properties;  // For object validation
  std::optional<JSONSchema> items;     // For array validation
  std::vector<std::string> enum_values;  // For enum validation
};

/**
 * @brief JSON Validator with comprehensive validation capabilities
 */
class JSONValidator {
 public:
  /**
   * @brief Validate JSON syntax
   */
  static JSONValidationResult validate_syntax(const std::string& json);

  /**
   * @brief Validate JSON against schema
   */
  static JSONValidationResult validate_schema(
      const std::string& json, const JSONSchema& schema);

  /**
   * @brief Validate JSON structure and types
   */
  static JSONValidationResult validate_structure(const std::string& json);

  /**
   * @brief Sanitize JSON for security
   */
  static JSONValidationResult sanitize_json(
      const std::string& json, size_t max_depth = 100, size_t max_size = 10485760);

  /**
   * @brief Check for JSON injection attacks
   */
  static bool has_injection_risk(const std::string& json);

  /**
   * @brief Validate JSON streaming (for large files)
   */
  static JSONValidationResult validate_streaming(
      const std::string& json, size_t chunk_size = 4096);

  /**
   * @brief Pretty print JSON
   */
  static std::string pretty_print(const std::string& json, size_t indent = 2);

  /**
   * @brief Minify JSON
   */
  static std::string minify(const std::string& json);

  /**
   * @brief Get JSON type from string
   */
  static std::optional<JSONType> get_type(const std::string& json);

  /**
   * @brief Validate specific JSON types
   */
  static bool is_valid_json_string(const std::string& str);
  static bool is_valid_json_number(const std::string& str);
  static bool is_valid_json_boolean(const std::string& str);
  static bool is_valid_json_null(const std::string& str);

 private:
  /**
   * @brief Check balanced brackets/braces
   */
  static bool check_balanced_delimiters(const std::string& json);

  /**
   * @brief Calculate JSON depth
   */
  static size_t calculate_depth(const std::string& json);

  /**
   * @brief Escape special characters
   */
  static std::string escape_string(const std::string& str);

  /**
   * @brief Unescape special characters
   */
  static std::string unescape_string(const std::string& str);

  /**
   * @brief Find matching closing bracket
   */
  static size_t find_matching_bracket(
      const std::string& json, size_t start, char open, char close);

  /**
   * @brief Convert JSONType to string
   */
  static std::string type_to_string(JSONType type);

  /**
   * @brief Schema validation helpers
   */
  static bool validate_string_schema(
      const std::string& json, const JSONSchema& schema, JSONValidationResult& result);
  static bool validate_number_schema(
      const std::string& json, const JSONSchema& schema, JSONValidationResult& result);
  static bool validate_array_schema(
      const std::string& json, const JSONSchema& schema, JSONValidationResult& result);
  static bool validate_object_schema(
      const std::string& json, const JSONSchema& schema, JSONValidationResult& result);
};

/**
 * @brief JSON Schema Builder for easy schema creation
 */
class JSONSchemaBuilder {
 public:
  JSONSchemaBuilder& type(JSONType t);
  JSONSchemaBuilder& required(bool req = true);
  JSONSchemaBuilder& pattern(const std::string& pat);
  JSONSchemaBuilder& minimum(double min);
  JSONSchemaBuilder& maximum(double max);
  JSONSchemaBuilder& min_length(size_t len);
  JSONSchemaBuilder& max_length(size_t len);
  JSONSchemaBuilder& property(const std::string& name, const JSONSchema& schema);
  JSONSchemaBuilder& items(const JSONSchema& schema);
  JSONSchemaBuilder& enum_values(const std::vector<std::string>& values);

  JSONSchema build() const;

 private:
  JSONSchema schema_;
};

}  // namespace SolarSystem::Utils::Validation

