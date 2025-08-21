/**
 * @file output_validator.cpp
 * @brief Implementation of output format validator
 */

#include "solar_test/formatters/output_formatter.hpp"

#include <regex>
#include <stack>

namespace SolarSystem::Testing::Formatters {

// OutputValidator implementation
FormatValidationResult OutputValidator::validate_xml(const std::string& xml) {
  FormatValidationResult result(true);

  try {
    // Check for XML declaration
    if (xml.find("<?xml") == std::string::npos) {
      result.warnings.push_back("Missing XML declaration");
    }

    // Validate XML structure
    std::stack<std::string> tag_stack;
    std::regex tag_regex(R"(<(/?)([a-zA-Z][a-zA-Z0-9_-]*)[^>]*(/?)>)");
    std::sregex_iterator iter(xml.begin(), xml.end(), tag_regex);
    std::sregex_iterator end;

    size_t line_number = 1;
    size_t last_pos = 0;

    for (; iter != end; ++iter) {
      const std::smatch& match = *iter;

      // Update line number
      size_t current_pos = match.position();
      for (size_t i = last_pos; i < current_pos; ++i) {
        if (xml[i] == '\n') line_number++;
      }
      last_pos = current_pos;

      bool is_closing = !match[1].str().empty();
      std::string tag_name = match[2].str();
      bool is_self_closing = !match[3].str().empty();

      // Validate tag name
      if (!is_valid_xml_name(tag_name)) {
        result.is_valid = false;
        result.error_message = "Invalid XML tag name: " + tag_name;
        result.error_line = line_number;
        return result;
      }

      if (is_closing) {
        if (tag_stack.empty()) {
          result.is_valid = false;
          result.error_message = "Unexpected closing tag: " + tag_name;
          result.error_line = line_number;
          return result;
        }
        if (tag_stack.top() != tag_name) {
          result.is_valid = false;
          result.error_message = "Mismatched closing tag: expected " +
                                tag_stack.top() + ", got " + tag_name;
          result.error_line = line_number;
          return result;
        }
        tag_stack.pop();
      } else if (!is_self_closing) {
        tag_stack.push(tag_name);
      }
    }

    if (!tag_stack.empty()) {
      result.is_valid = false;
      result.error_message = "Unclosed tag: " + tag_stack.top();
      return result;
    }

    // Check for unescaped special characters in content
    std::regex unescaped_chars(R"([<>&](?![a-zA-Z]+;))");
    std::sregex_iterator char_iter(xml.begin(), xml.end(), unescaped_chars);
    if (char_iter != std::sregex_iterator()) {
      result.warnings.push_back("Potentially unescaped special characters found");
    }

  } catch (const std::exception& e) {
    result.is_valid = false;
    result.error_message = "XML parsing error: " + std::string(e.what());
  }

  return result;
}

FormatValidationResult OutputValidator::validate_json(const std::string& json) {
  FormatValidationResult result(true);

  try {
    std::stack<char> bracket_stack;
    bool in_string = false;
    bool escaped = false;
    size_t line_number = 1;
    size_t column = 1;

    for (size_t i = 0; i < json.length(); ++i) {
      char c = json[i];

      if (c == '\n') {
        line_number++;
        column = 1;
      } else {
        column++;
      }

      if (escaped) {
        escaped = false;
        continue;
      }

      if (c == '\\' && in_string) {
        escaped = true;
        continue;
      }

      if (c == '"') {
        in_string = !in_string;
        continue;
      }

      if (in_string) {
        // Validate string content
        if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
          result.warnings.push_back("Control character in string at line " +
                                   std::to_string(line_number));
        }
        continue;
      }

      switch (c) {
        case '{':
        case '[':
          bracket_stack.push(c);
          break;
        case '}':
          if (bracket_stack.empty() || bracket_stack.top() != '{') {
            result.is_valid = false;
            result.error_message = "Mismatched closing brace";
            result.error_line = line_number;
            result.error_column = column;
            return result;
          }
          bracket_stack.pop();
          break;
        case ']':
          if (bracket_stack.empty() || bracket_stack.top() != '[') {
            result.is_valid = false;
            result.error_message = "Mismatched closing bracket";
            result.error_line = line_number;
            result.error_column = column;
            return result;
          }
          bracket_stack.pop();
          break;
      }
    }

    if (!bracket_stack.empty()) {
      result.is_valid = false;
      result.error_message = "Unclosed bracket or brace";
      return result;
    }

    if (in_string) {
      result.is_valid = false;
      result.error_message = "Unclosed string";
      return result;
    }

    // Check for trailing commas
    std::regex trailing_comma(R"(,(\s*[}\]]))");
    if (std::regex_search(json, trailing_comma)) {
      result.warnings.push_back("Trailing commas found (may not be valid in strict JSON)");
    }

  } catch (const std::exception& e) {
    result.is_valid = false;
    result.error_message = "JSON parsing error: " + std::string(e.what());
  }

  return result;
}

FormatValidationResult OutputValidator::validate_junit_xml(const std::string& xml) {
  FormatValidationResult result = validate_xml(xml);

  if (!result.is_valid) {
    return result;
  }

  // Additional JUnit-specific validation

  // Check for required elements
  if (xml.find("<testsuites") == std::string::npos &&
      xml.find("<testsuite") == std::string::npos) {
    result.is_valid = false;
    result.error_message = "Missing required testsuite or testsuites element";
    return result;
  }

  // Validate required attributes
  std::regex testsuite_regex(R"(<testsuite[^>]*>)");
  std::sregex_iterator iter(xml.begin(), xml.end(), testsuite_regex);

  for (; iter != std::sregex_iterator(); ++iter) {
    std::string testsuite_tag = iter->str();

    // Check for required attributes
    if (testsuite_tag.find("name=") == std::string::npos) {
      result.warnings.push_back("testsuite element missing name attribute");
    }
    if (testsuite_tag.find("tests=") == std::string::npos) {
      result.warnings.push_back("testsuite element missing tests attribute");
    }
  }

  // Validate testcase elements
  std::regex testcase_regex(R"(<testcase[^>]*>)");
  std::sregex_iterator case_iter(xml.begin(), xml.end(), testcase_regex);

  for (; case_iter != std::sregex_iterator(); ++case_iter) {
    std::string testcase_tag = case_iter->str();

    if (testcase_tag.find("name=") == std::string::npos) {
      result.warnings.push_back("testcase element missing name attribute");
    }
  }

  return result;
}

FormatValidationResult OutputValidator::validate_test_content(const std::string& content,
                                                             OutputFormat format) {
  switch (format) {
    case OutputFormat::XML:
    case OutputFormat::JUnit:
      return validate_junit_xml(content);
    case OutputFormat::JSON:
      return validate_json(content);
    default:
      return FormatValidationResult(true); // No validation for other formats yet
  }
}

std::vector<std::string> OutputValidator::suggest_fixes(const FormatValidationResult& validation_result) {
  std::vector<std::string> suggestions;

  if (!validation_result.is_valid) {
    const std::string& error = validation_result.error_message;

    // XML-specific suggestions
    if (error.find("Unclosed tag") != std::string::npos) {
      suggestions.push_back("Add the missing closing tag");
      suggestions.push_back("Check for typos in tag names");
    } else if (error.find("Mismatched closing tag") != std::string::npos) {
      suggestions.push_back("Verify tag nesting is correct");
      suggestions.push_back("Check for missing opening or closing tags");
    } else if (error.find("Invalid XML tag name") != std::string::npos) {
      suggestions.push_back("Ensure tag names start with a letter or underscore");
      suggestions.push_back("Use only letters, numbers, hyphens, and underscores in tag names");
    }

    // JSON-specific suggestions
    else if (error.find("Mismatched closing brace") != std::string::npos) {
      suggestions.push_back("Check for missing opening braces");
      suggestions.push_back("Verify object structure is correct");
    } else if (error.find("Mismatched closing bracket") != std::string::npos) {
      suggestions.push_back("Check for missing opening brackets");
      suggestions.push_back("Verify array structure is correct");
    } else if (error.find("Unclosed string") != std::string::npos) {
      suggestions.push_back("Add missing closing quote");
      suggestions.push_back("Escape any quotes within the string");
    }

    // General suggestions
    if (suggestions.empty()) {
      suggestions.push_back("Check the format syntax");
      suggestions.push_back("Validate against a schema if available");
      suggestions.push_back("Use a format-specific validator tool");
    }
  }

  // Add suggestions from warnings
  for (const auto& warning : validation_result.warnings) {
    if (warning.find("trailing comma") != std::string::npos) {
      suggestions.push_back("Remove trailing commas for strict JSON compliance");
    } else if (warning.find("XML declaration") != std::string::npos) {
      suggestions.push_back("Add XML declaration: <?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    } else if (warning.find("unescaped") != std::string::npos) {
      suggestions.push_back("Escape special characters (&lt; &gt; &amp; &quot; &apos;)");
    }
  }

  return suggestions;
}

bool OutputValidator::is_valid_xml_name(const std::string& name) {
  if (name.empty()) {
    return false;
  }

  // First character must be letter or underscore
  char first = name[0];
  if (!std::isalpha(first) && first != '_') {
    return false;
  }

  // Remaining characters must be letters, digits, hyphens, underscores, or periods
  for (size_t i = 1; i < name.length(); ++i) {
    char c = name[i];
    if (!std::isalnum(c) && c != '-' && c != '_' && c != '.') {
      return false;
    }
  }

  return true;
}

bool OutputValidator::is_valid_json_string(const std::string& str) {
  bool escaped = false;

  for (char c : str) {
    if (escaped) {
      escaped = false;
      continue;
    }

    if (c == '\\') {
      escaped = true;
      continue;
    }

    if (c == '"') {
      return false; // Unescaped quote
    }

    if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
      return false; // Control character
    }
  }

  return !escaped; // Should not end with escape character
}

} // namespace SolarSystem::Testing::Formatters
