/**
 * @file json_validator.cpp
 * @brief Implementation of Comprehensive JSON Validation System
 */

#include "solar_utils/validation/json_validator.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <stack>

namespace SolarSystem::Utils::Validation {

/**
 * @brief Validate JSON syntax
 */
JSONValidationResult JSONValidator::validate_syntax(const std::string& json) {
  JSONValidationResult result;

  if (json.empty()) {
    JSONValidationError error;
    error.message = "Empty JSON string";
    error.line = 1;
    error.column = 1;
    result.errors.push_back(error);
    return result;
  }

  // Check balanced delimiters
  if (!check_balanced_delimiters(json)) {
    JSONValidationError error;
    error.message = "Unbalanced brackets or braces";
    result.errors.push_back(error);
    return result;
  }

  // Check valid start/end characters
  std::string trimmed = json;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed.empty()) {
    JSONValidationError error;
    error.message = "JSON contains only whitespace";
    result.errors.push_back(error);
    return result;
  }

  char first = trimmed.front();
  char last = trimmed.back();

  bool valid_structure =
      (first == '{' && last == '}') ||
      (first == '[' && last == ']') ||
      (first == '"' && last == '"') ||
      (trimmed == "null") ||
      (trimmed == "true") ||
      (trimmed == "false") ||
      is_valid_json_number(trimmed);

  if (!valid_structure) {
    JSONValidationError error;
    error.message = "Invalid JSON structure";
    result.errors.push_back(error);
    return result;
  }

  result.is_valid = true;
  result.sanitized_json = trimmed;
  return result;
}

/**
 * @brief Validate JSON structure
 */
JSONValidationResult JSONValidator::validate_structure(const std::string& json) {
  auto syntax_result = validate_syntax(json);
  if (!syntax_result.is_valid) {
    return syntax_result;
  }

  JSONValidationResult result;
  result.is_valid = true;
  result.sanitized_json = syntax_result.sanitized_json;

  // Additional structure validation
  size_t depth = calculate_depth(json);
  if (depth > 100) {
    result.warnings.push_back("JSON depth exceeds 100 levels");
  }

  return result;
}

/**
 * @brief Validate JSON against schema
 */
JSONValidationResult JSONValidator::validate_schema(
    const std::string& json, const JSONSchema& schema) {

  // First validate syntax
  auto syntax_result = validate_syntax(json);
  if (!syntax_result.is_valid) {
    return syntax_result;
  }

  JSONValidationResult result;
  result.is_valid = true;
  result.sanitized_json = syntax_result.sanitized_json;

  // Get the JSON type
  auto json_type = get_type(json);
  if (!json_type) {
    JSONValidationError error;
    error.message = "Unable to determine JSON type";
    result.errors.push_back(error);
    result.is_valid = false;
    return result;
  }

  // Validate type matches schema
  if (*json_type != schema.type) {
    JSONValidationError error;
    error.message = "Type mismatch: expected " + type_to_string(schema.type) +
                   ", got " + type_to_string(*json_type);
    result.errors.push_back(error);
    result.is_valid = false;
    return result;
  }

  // Type-specific validation
  std::string trimmed = json;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  switch (schema.type) {
    case JSONType::String:
      if (!validate_string_schema(trimmed, schema, result)) {
        result.is_valid = false;
      }
      break;

    case JSONType::Number:
      if (!validate_number_schema(trimmed, schema, result)) {
        result.is_valid = false;
      }
      break;

    case JSONType::Array:
      if (!validate_array_schema(trimmed, schema, result)) {
        result.is_valid = false;
      }
      break;

    case JSONType::Object:
      if (!validate_object_schema(trimmed, schema, result)) {
        result.is_valid = false;
      }
      break;

    case JSONType::Boolean:
    case JSONType::Null:
      // No additional validation needed
      break;
  }

  // Validate enum values if specified
  if (!schema.enum_values.empty()) {
    bool found = false;
    for (const auto& enum_val : schema.enum_values) {
      if (trimmed == enum_val || trimmed == "\"" + enum_val + "\"") {
        found = true;
        break;
      }
    }
    if (!found) {
      JSONValidationError error;
      error.message = "Value not in allowed enum values";
      result.errors.push_back(error);
      result.is_valid = false;
    }
  }

  return result;
}

/**
 * @brief Sanitize JSON for security
 */
JSONValidationResult JSONValidator::sanitize_json(
    const std::string& json, size_t max_depth, size_t max_size) {

  JSONValidationResult result;

  // Check size limit
  if (json.size() > max_size) {
    JSONValidationError error;
    error.message = "JSON exceeds maximum size limit";
    result.errors.push_back(error);
    return result;
  }

  // Validate syntax first
  auto syntax_result = validate_syntax(json);
  if (!syntax_result.is_valid) {
    return syntax_result;
  }

  // Check depth
  size_t depth = calculate_depth(json);
  if (depth > max_depth) {
    JSONValidationError error;
    error.message = "JSON depth exceeds maximum allowed depth";
    result.errors.push_back(error);
    return result;
  }

  // Check for injection risks
  if (has_injection_risk(json)) {
    result.warnings.push_back("Potential injection risk detected");
  }

  result.is_valid = true;
  result.sanitized_json = syntax_result.sanitized_json;
  return result;
}

/**
 * @brief Check for JSON injection attacks
 */
bool JSONValidator::has_injection_risk(const std::string& json) {
  // Check for suspicious patterns
  std::vector<std::string> suspicious_patterns = {
      "__proto__",
      "constructor",
      "prototype",
      "<script",
      "javascript:",
      "onerror=",
      "onload="
  };

  for (const auto& pattern : suspicious_patterns) {
    if (json.find(pattern) != std::string::npos) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Validate JSON streaming
 */
JSONValidationResult JSONValidator::validate_streaming(
    const std::string& json, size_t chunk_size) {

  JSONValidationResult result;

  // Process in chunks
  size_t offset = 0;
  std::stack<char> bracket_stack;
  bool in_string = false;
  bool escape_next = false;

  while (offset < json.size()) {
    size_t end = std::min(offset + chunk_size, json.size());

    for (size_t i = offset; i < end; ++i) {
      char c = json[i];

      if (escape_next) {
        escape_next = false;
        continue;
      }

      if (c == '\\') {
        escape_next = true;
        continue;
      }

      if (c == '"') {
        in_string = !in_string;
        continue;
      }

      if (!in_string) {
        if (c == '{' || c == '[') {
          bracket_stack.push(c);
        } else if (c == '}') {
          if (bracket_stack.empty() || bracket_stack.top() != '{') {
            JSONValidationError error;
            error.message = "Mismatched closing brace";
            error.column = i;
            result.errors.push_back(error);
            return result;
          }
          bracket_stack.pop();
        } else if (c == ']') {
          if (bracket_stack.empty() || bracket_stack.top() != '[') {
            JSONValidationError error;
            error.message = "Mismatched closing bracket";
            error.column = i;
            result.errors.push_back(error);
            return result;
          }
          bracket_stack.pop();
        }
      }
    }

    offset = end;
  }

  if (!bracket_stack.empty()) {
    JSONValidationError error;
    error.message = "Unclosed brackets or braces";
    result.errors.push_back(error);
    return result;
  }

  result.is_valid = true;
  return result;
}

/**
 * @brief Pretty print JSON
 */
std::string JSONValidator::pretty_print(const std::string& json, size_t indent) {
  std::ostringstream result;
  size_t current_indent = 0;
  bool in_string = false;
  bool escape_next = false;

  for (size_t i = 0; i < json.size(); ++i) {
    char c = json[i];

    if (escape_next) {
      result << c;
      escape_next = false;
      continue;
    }

    if (c == '\\') {
      result << c;
      escape_next = true;
      continue;
    }

    if (c == '"') {
      result << c;
      in_string = !in_string;
      continue;
    }

    if (!in_string) {
      if (c == '{' || c == '[') {
        result << c << '\n';
        current_indent += indent;
        result << std::string(current_indent, ' ');
      } else if (c == '}' || c == ']') {
        result << '\n';
        current_indent -= indent;
        result << std::string(current_indent, ' ') << c;
      } else if (c == ',') {
        result << c << '\n' << std::string(current_indent, ' ');
      } else if (c == ':') {
        result << c << ' ';
      } else if (!std::isspace(c)) {
        result << c;
      }
    } else {
      result << c;
    }
  }

  return result.str();
}

/**
 * @brief Minify JSON
 */
std::string JSONValidator::minify(const std::string& json) {
  std::ostringstream result;
  bool in_string = false;
  bool escape_next = false;

  for (char c : json) {
    if (escape_next) {
      result << c;
      escape_next = false;
      continue;
    }

    if (c == '\\') {
      result << c;
      escape_next = true;
      continue;
    }

    if (c == '"') {
      result << c;
      in_string = !in_string;
      continue;
    }

    if (in_string || !std::isspace(c)) {
      result << c;
    }
  }

  return result.str();
}

/**
 * @brief Get JSON type
 */
std::optional<JSONType> JSONValidator::get_type(const std::string& json) {
  std::string trimmed = json;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed.empty()) {
    return std::nullopt;
  }

  if (trimmed == "null") return JSONType::Null;
  if (trimmed == "true" || trimmed == "false") return JSONType::Boolean;
  if (trimmed.front() == '"') return JSONType::String;
  if (trimmed.front() == '{') return JSONType::Object;
  if (trimmed.front() == '[') return JSONType::Array;
  if (is_valid_json_number(trimmed)) return JSONType::Number;

  return std::nullopt;
}

/**
 * @brief Validate JSON string
 */
bool JSONValidator::is_valid_json_string(const std::string& str) {
  if (str.size() < 2) return false;
  if (str.front() != '"' || str.back() != '"') return false;

  bool escape_next = false;
  for (size_t i = 1; i < str.size() - 1; ++i) {
    if (escape_next) {
      escape_next = false;
      continue;
    }
    if (str[i] == '\\') {
      escape_next = true;
    }
  }

  return !escape_next;
}

/**
 * @brief Validate JSON number
 */
bool JSONValidator::is_valid_json_number(const std::string& str) {
  std::regex number_pattern(R"(^-?(0|[1-9]\d*)(\.\d+)?([eE][+-]?\d+)?$)");
  return std::regex_match(str, number_pattern);
}

/**
 * @brief Validate JSON boolean
 */
bool JSONValidator::is_valid_json_boolean(const std::string& str) {
  return str == "true" || str == "false";
}

/**
 * @brief Validate JSON null
 */
bool JSONValidator::is_valid_json_null(const std::string& str) {
  return str == "null";
}

// Private helper methods

bool JSONValidator::check_balanced_delimiters(const std::string& json) {
  std::stack<char> stack;
  bool in_string = false;
  bool escape_next = false;

  for (char c : json) {
    if (escape_next) {
      escape_next = false;
      continue;
    }

    if (c == '\\') {
      escape_next = true;
      continue;
    }

    if (c == '"') {
      in_string = !in_string;
      continue;
    }

    if (!in_string) {
      if (c == '{' || c == '[') {
        stack.push(c);
      } else if (c == '}') {
        if (stack.empty() || stack.top() != '{') return false;
        stack.pop();
      } else if (c == ']') {
        if (stack.empty() || stack.top() != '[') return false;
        stack.pop();
      }
    }
  }

  return stack.empty() && !in_string;
}

size_t JSONValidator::calculate_depth(const std::string& json) {
  size_t max_depth = 0;
  size_t current_depth = 0;
  bool in_string = false;
  bool escape_next = false;

  for (char c : json) {
    if (escape_next) {
      escape_next = false;
      continue;
    }

    if (c == '\\') {
      escape_next = true;
      continue;
    }

    if (c == '"') {
      in_string = !in_string;
      continue;
    }

    if (!in_string) {
      if (c == '{' || c == '[') {
        current_depth++;
        max_depth = std::max(max_depth, current_depth);
      } else if (c == '}' || c == ']') {
        if (current_depth > 0) current_depth--;
      }
    }
  }

  return max_depth;
}

std::string JSONValidator::type_to_string(JSONType type) {
  switch (type) {
    case JSONType::Null: return "null";
    case JSONType::Boolean: return "boolean";
    case JSONType::Number: return "number";
    case JSONType::String: return "string";
    case JSONType::Array: return "array";
    case JSONType::Object: return "object";
    default: return "unknown";
  }
}

bool JSONValidator::validate_string_schema(
    const std::string& json, const JSONSchema& schema, JSONValidationResult& result) {

  // Extract string content (remove quotes)
  if (json.size() < 2) return false;
  std::string content = json.substr(1, json.size() - 2);

  // Check length constraints
  if (schema.min_length && content.length() < *schema.min_length) {
    JSONValidationError error;
    error.message = "String length " + std::to_string(content.length()) +
                   " is less than minimum " + std::to_string(*schema.min_length);
    result.errors.push_back(error);
    return false;
  }

  if (schema.max_length && content.length() > *schema.max_length) {
    JSONValidationError error;
    error.message = "String length " + std::to_string(content.length()) +
                   " exceeds maximum " + std::to_string(*schema.max_length);
    result.errors.push_back(error);
    return false;
  }

  // Check pattern if specified
  if (schema.pattern) {
    std::regex pattern(*schema.pattern);
    if (!std::regex_match(content, pattern)) {
      JSONValidationError error;
      error.message = "String does not match required pattern";
      result.errors.push_back(error);
      return false;
    }
  }

  return true;
}

bool JSONValidator::validate_number_schema(
    const std::string& json, const JSONSchema& schema, JSONValidationResult& result) {

  double value = std::stod(json);

  // Check minimum
  if (schema.minimum && value < *schema.minimum) {
    JSONValidationError error;
    error.message = "Number " + std::to_string(value) +
                   " is less than minimum " + std::to_string(*schema.minimum);
    result.errors.push_back(error);
    return false;
  }

  // Check maximum
  if (schema.maximum && value > *schema.maximum) {
    JSONValidationError error;
    error.message = "Number " + std::to_string(value) +
                   " exceeds maximum " + std::to_string(*schema.maximum);
    result.errors.push_back(error);
    return false;
  }

  return true;
}

bool JSONValidator::validate_array_schema(
    const std::string& json, const JSONSchema& schema, JSONValidationResult& result) {

  // Count array elements
  size_t element_count = 0;
  size_t depth = 0;
  bool in_string = false;
  bool escape_next = false;

  for (size_t i = 1; i < json.size() - 1; ++i) {
    char c = json[i];

    if (escape_next) {
      escape_next = false;
      continue;
    }

    if (c == '\\') {
      escape_next = true;
      continue;
    }

    if (c == '"') {
      in_string = !in_string;
      continue;
    }

    if (!in_string) {
      if (c == '[' || c == '{') {
        depth++;
      } else if (c == ']' || c == '}') {
        depth--;
      } else if (c == ',' && depth == 0) {
        element_count++;
      }
    }
  }

  // If array is not empty, count includes the last element
  std::string trimmed = json.substr(1, json.size() - 2);
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  if (!trimmed.empty()) {
    element_count++;
  }

  // Check length constraints
  if (schema.min_length && element_count < *schema.min_length) {
    JSONValidationError error;
    error.message = "Array length " + std::to_string(element_count) +
                   " is less than minimum " + std::to_string(*schema.min_length);
    result.errors.push_back(error);
    return false;
  }

  if (schema.max_length && element_count > *schema.max_length) {
    JSONValidationError error;
    error.message = "Array length " + std::to_string(element_count) +
                   " exceeds maximum " + std::to_string(*schema.max_length);
    result.errors.push_back(error);
    return false;
  }

  // TODO: Validate individual array items against schema.items if specified
  // This would require parsing individual array elements

  return true;
}

bool JSONValidator::validate_object_schema(
    const std::string& json, const JSONSchema& schema, JSONValidationResult& result) {

  // TODO: Full object property validation would require a proper JSON parser
  // For now, we do basic validation

  // Check if required properties exist (simple string search)
  for (const auto& [prop_name, prop_schema] : schema.properties) {
    if (prop_schema.required) {
      std::string search_key = "\"" + prop_name + "\"";
      if (json.find(search_key) == std::string::npos) {
        JSONValidationError error;
        error.message = "Required property '" + prop_name + "' is missing";
        error.path = prop_name;
        result.errors.push_back(error);
        return false;
      }
    }
  }

  return true;
}

// JSONSchemaBuilder implementation

JSONSchemaBuilder& JSONSchemaBuilder::type(JSONType t) {
  schema_.type = t;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::required(bool req) {
  schema_.required = req;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::pattern(const std::string& pat) {
  schema_.pattern = pat;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::minimum(double min) {
  schema_.minimum = min;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::maximum(double max) {
  schema_.maximum = max;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::min_length(size_t len) {
  schema_.min_length = len;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::max_length(size_t len) {
  schema_.max_length = len;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::property(
    const std::string& name, const JSONSchema& schema) {
  schema_.properties[name] = schema;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::items(const JSONSchema& schema) {
  schema_.items = schema;
  return *this;
}

JSONSchemaBuilder& JSONSchemaBuilder::enum_values(const std::vector<std::string>& values) {
  schema_.enum_values = values;
  return *this;
}

JSONSchema JSONSchemaBuilder::build() const {
  return schema_;
}

}  // namespace SolarSystem::Utils::Validation

