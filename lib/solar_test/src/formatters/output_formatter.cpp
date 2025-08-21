/**
 * @file output_formatter.cpp
 * @brief Implementation of enhanced output formatting system
 */

#include "solar_test/formatters/output_formatter.hpp"

#include <algorithm>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stack>

namespace SolarSystem::Testing::Formatters {

// EnhancedXmlFormatter implementation
EnhancedXmlFormatter::EnhancedXmlFormatter(const FormatOptimization& optimization) {
  optimization_ = optimization;
  if (optimization_.cache_escaped_strings) {
    escaped_string_cache_.reserve(optimization_.string_cache_size);
  }
}

std::string EnhancedXmlFormatter::format_test_suite(const TestSuiteResult& result) {
  std::ostringstream xml;

  if (optimization_.include_xml_declaration) {
    xml << "<?xml version=\"1.0\" encoding=\"" << optimization_.xml_encoding << "\"?>\n";
  }

  // Calculate suite statistics
  size_t total_tests = result.test_results.size();
  size_t failures = result.failed_count;
  size_t errors = 0;  // We don't distinguish errors from failures in our model
  size_t skipped = result.skipped_count;

  // Calculate total duration
  std::chrono::milliseconds total_duration{0};
  for (const auto& test : result.test_results) {
    total_duration += test.execution_time;
  }

  // Create testsuite element
  std::unordered_map<std::string, std::string> suite_attrs = {
    {"name", result.suite_name},
    {"tests", std::to_string(total_tests)},
    {"failures", std::to_string(failures)},
    {"errors", std::to_string(errors)},
    {"skipped", std::to_string(skipped)},
    {"time", format_duration(total_duration)}
  };

  xml << "<testsuites>\n";
  xml << format_xml_element("testsuite", suite_attrs, "", false);
  xml << "\n";

  // Add individual test cases
  for (const auto& test : result.test_results) {
    xml << format_test_result(test);
  }

  xml << "  </testsuite>\n";
  xml << "</testsuites>\n";

  std::string output = xml.str();

  // Validate output if configured
  if (optimization_.validate_during_write) {
    auto validation = validate_output(output);
    if (!validation.is_valid) {
      output = recover_from_error(output, validation.error_message);
    }
  }

  return output;
}

std::string EnhancedXmlFormatter::format_test_result(const TestResult& result) {
  std::unordered_map<std::string, std::string> attrs = {
    {"name", result.test_name},
    {"classname", "TestSuite"},  // Could be enhanced to include actual class name
    {"time", format_duration(result.execution_time)}
  };

  std::string content;
  bool has_failure = false;

  // Add failure/error/skip elements based on status
  switch (result.status) {
    case TestResult::Status::Failed:
    case TestResult::Status::Error:
    case TestResult::Status::Timeout: {
      has_failure = true;
      std::unordered_map<std::string, std::string> failure_attrs = {
        {"type", result.status == TestResult::Status::Timeout ? "TestTimeout" : "AssertionFailure"},
        {"message", xml_escape(result.error_message)}
      };

      std::string failure_content;
      if (!result.assertion_failures.empty()) {
        failure_content = "<![CDATA[\n";
        for (const auto& failure : result.assertion_failures) {
          failure_content += failure + "\n";
        }
        failure_content += "]]>";
      }

      content += "    " + format_xml_element("failure", failure_attrs, failure_content) + "\n";
      break;
    }
    case TestResult::Status::Skipped: {
      has_failure = true;
      std::unordered_map<std::string, std::string> skip_attrs;
      if (!result.error_message.empty()) {
        skip_attrs["message"] = xml_escape(result.error_message);
      }
      content += "    " + format_xml_element("skipped", skip_attrs, "", true) + "\n";
      break;
    }
    default:
      break;
  }

  // Add system output if available
  if (result.has_metadata("captured_output")) {
    std::string output = result.get_metadata("captured_output");
    if (!output.empty()) {
      has_failure = true;
      content += "    <system-out><![CDATA[\n" + output + "\n]]></system-out>\n";
    }
  }

  std::string indent = optimization_.pretty_print ? "  " : "";

  if (has_failure) {
    return indent + format_xml_element("testcase", attrs, "\n" + content + "  ", false) + "\n";
  } else {
    return indent + format_xml_element("testcase", attrs, "", true) + "\n";
  }
}

std::string EnhancedXmlFormatter::format_header() {
  std::string header;
  if (optimization_.include_xml_declaration) {
    header = "<?xml version=\"1.0\" encoding=\"" + optimization_.xml_encoding + "\"?>\n";
  }
  header += "<testsuites>\n";
  return header;
}

std::string EnhancedXmlFormatter::format_footer() {
  return "</testsuites>\n";
}

void EnhancedXmlFormatter::start_streaming(std::ostream& output) {
  streaming_active_ = true;
  test_count_ = 0;
  output << format_header();
}

void EnhancedXmlFormatter::stream_test_result(const TestResult& result, std::ostream& output) {
  if (!streaming_active_) {
    throw std::runtime_error("Streaming not started");
  }

  // Start testsuite element on first test
  if (test_count_ == 0) {
    output << "  <testsuite name=\"StreamedTests\">\n";
  }

  output << format_test_result(result);
  test_count_++;

  // Flush periodically for large streams
  if (test_count_ % 100 == 0) {
    output.flush();
  }
}

void EnhancedXmlFormatter::end_streaming(std::ostream& output) {
  if (streaming_active_) {
    if (test_count_ > 0) {
      output << "  </testsuite>\n";
    }
    output << format_footer();
    streaming_active_ = false;
  }
}

FormatValidationResult EnhancedXmlFormatter::validate_output(const std::string& output) {
  FormatValidationResult result(true);

  try {
    validate_xml_structure(output);
  } catch (const std::exception& e) {
    result.is_valid = false;
    result.error_message = e.what();

    // Provide suggestions for common XML errors
    if (result.error_message.find("unclosed") != std::string::npos) {
      result.suggestions.push_back("Check for unclosed XML tags");
    }
    if (result.error_message.find("invalid character") != std::string::npos) {
      result.suggestions.push_back("Ensure all special characters are properly escaped");
    }
  }

  return result;
}

std::string EnhancedXmlFormatter::recover_from_error(const std::string& invalid_output,
                                                    const std::string& error_context) {
  std::string recovered = invalid_output;

  // Basic recovery strategies

  // 1. Escape unescaped special characters
  std::regex unescaped_ampersand(R"(&(?![a-zA-Z]+;))");
  recovered = std::regex_replace(recovered, unescaped_ampersand, "&amp;");

  // 2. Close unclosed tags (basic heuristic)
  if (error_context.find("unclosed") != std::string::npos) {
    if (recovered.find("<testsuite") != std::string::npos &&
        recovered.find("</testsuite>") == std::string::npos) {
      recovered += "\n  </testsuite>";
    }
    if (recovered.find("<testsuites") != std::string::npos &&
        recovered.find("</testsuites>") == std::string::npos) {
      recovered += "\n</testsuites>";
    }
  }

  // 3. Add XML declaration if missing
  if (optimization_.include_xml_declaration &&
      recovered.find("<?xml") == std::string::npos) {
    recovered = "<?xml version=\"1.0\" encoding=\"" + optimization_.xml_encoding + "\"?>\n" + recovered;
  }

  return recovered;
}

void EnhancedXmlFormatter::set_optimization(const FormatOptimization& optimization) {
  optimization_ = optimization;
  if (optimization_.cache_escaped_strings) {
    escaped_string_cache_.clear();
    escaped_string_cache_.reserve(optimization_.string_cache_size);
  }
}

void EnhancedXmlFormatter::set_streaming_config(const StreamingConfig& config) {
  streaming_config_ = config;
}

std::string EnhancedXmlFormatter::xml_escape(const std::string& text) {
  if (optimization_.cache_escaped_strings) {
    auto it = escaped_string_cache_.find(text);
    if (it != escaped_string_cache_.end()) {
      return it->second;
    }
  }

  std::string escaped;
  escaped.reserve(text.length() * 1.2);  // Reserve extra space for escaping

  for (char c : text) {
    switch (c) {
      case '<': escaped += "&lt;"; break;
      case '>': escaped += "&gt;"; break;
      case '&': escaped += "&amp;"; break;
      case '"': escaped += "&quot;"; break;
      case '\'': escaped += "&apos;"; break;
      default: escaped += c; break;
    }
  }

  if (optimization_.cache_escaped_strings &&
      escaped_string_cache_.size() < optimization_.string_cache_size) {
    escaped_string_cache_[text] = escaped;
  }

  return escaped;
}

std::string EnhancedXmlFormatter::format_xml_element(
    const std::string& name,
    const std::unordered_map<std::string, std::string>& attributes,
    const std::string& content,
    bool self_closing) {

  std::ostringstream element;
  element << "<" << name;

  // Add attributes
  for (const auto& [key, value] : attributes) {
    element << " " << key << "=\"" << xml_escape(value) << "\"";
  }

  if (self_closing) {
    element << "/>";
  } else {
    element << ">";
    if (!content.empty()) {
      element << content;
    }
    element << "</" << name << ">";
  }

  return element.str();
}

std::string EnhancedXmlFormatter::format_duration(std::chrono::milliseconds duration) {
  double seconds = static_cast<double>(duration.count()) / 1000.0;
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3) << seconds;
  return oss.str();
}

void EnhancedXmlFormatter::validate_xml_structure(const std::string& xml) {
  // Basic XML structure validation
  std::stack<std::string> tag_stack;
  std::regex tag_regex(R"(<(/?)([a-zA-Z][a-zA-Z0-9_-]*)[^>]*(/?)>)");
  std::sregex_iterator iter(xml.begin(), xml.end(), tag_regex);
  std::sregex_iterator end;

  for (; iter != end; ++iter) {
    const std::smatch& match = *iter;
    bool is_closing = !match[1].str().empty();
    std::string tag_name = match[2].str();
    bool is_self_closing = !match[3].str().empty();

    if (is_closing) {
      if (tag_stack.empty() || tag_stack.top() != tag_name) {
        throw std::runtime_error("Mismatched closing tag: " + tag_name);
      }
      tag_stack.pop();
    } else if (!is_self_closing) {
      tag_stack.push(tag_name);
    }
  }

  if (!tag_stack.empty()) {
    throw std::runtime_error("Unclosed tag: " + tag_stack.top());
  }
}

// EnhancedJsonFormatter implementation
EnhancedJsonFormatter::EnhancedJsonFormatter(const FormatOptimization& optimization) {
  optimization_ = optimization;
}

std::string EnhancedJsonFormatter::format_test_suite(const TestSuiteResult& result) {
  std::ostringstream json;

  // Calculate suite statistics
  size_t total_tests = result.test_results.size();
  std::chrono::milliseconds total_duration{0};
  for (const auto& test : result.test_results) {
    total_duration += test.execution_time;
  }

  json << "{\n";
  json << "  \"suite_name\": " << json_escape(result.suite_name) << ",\n";
  json << "  \"total_tests\": " << total_tests << ",\n";
  json << "  \"passed\": " << result.passed_count << ",\n";
  json << "  \"failed\": " << result.failed_count << ",\n";
  json << "  \"skipped\": " << result.skipped_count << ",\n";
  json << "  \"duration_ms\": " << total_duration.count() << ",\n";
  json << "  \"tests\": [\n";

  for (size_t i = 0; i < result.test_results.size(); ++i) {
    json << format_test_result(result.test_results[i]);
    if (i < result.test_results.size() - 1) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ]\n";
  json << "}";

  std::string output = json.str();

  // Validate output if configured
  if (optimization_.validate_during_write) {
    auto validation = validate_output(output);
    if (!validation.is_valid) {
      output = recover_from_error(output, validation.error_message);
    }
  }

  return output;
}

std::string EnhancedJsonFormatter::format_test_result(const TestResult& result) {
  std::ostringstream json;

  std::string indent = optimization_.pretty_print ? "    " : "";
  std::string newline = optimization_.pretty_print ? "\n" : "";
  std::string space = optimization_.pretty_print ? " " : "";

  json << indent << "{" << newline;
  json << indent << "  \"name\":" << space << json_escape(result.test_name) << "," << newline;
  json << indent << "  \"status\":" << space;

  switch (result.status) {
    case TestResult::Status::Passed:
      json << "\"passed\"";
      break;
    case TestResult::Status::Failed:
      json << "\"failed\"";
      break;
    case TestResult::Status::Error:
      json << "\"error\"";
      break;
    case TestResult::Status::Skipped:
      json << "\"skipped\"";
      break;
    case TestResult::Status::Timeout:
      json << "\"timeout\"";
      break;
    default:
      json << "\"unknown\"";
      break;
  }

  json << "," << newline;
  json << indent << "  \"duration_ms\":" << space << result.execution_time.count();

  if (!result.error_message.empty()) {
    json << "," << newline;
    json << indent << "  \"error_message\":" << space << json_escape(result.error_message);
  }

  if (!result.assertion_failures.empty()) {
    json << "," << newline;
    json << indent << "  \"assertion_failures\":" << space << "[" << newline;
    for (size_t i = 0; i < result.assertion_failures.size(); ++i) {
      json << indent << "    " << json_escape(result.assertion_failures[i]);
      if (i < result.assertion_failures.size() - 1) {
        json << ",";
      }
      json << newline;
    }
    json << indent << "  ]";
  }

  // Add metadata if configured
  if (optimization_.include_metadata && result.has_metadata("captured_output")) {
    json << "," << newline;
    json << indent << "  \"captured_output\":" << space
         << json_escape(result.get_metadata("captured_output"));
  }

  json << newline << indent << "}";

  return json.str();
}

std::string EnhancedJsonFormatter::format_header() {
  return "{\n  \"test_results\": [\n";
}

std::string EnhancedJsonFormatter::format_footer() {
  return "\n  ]\n}";
}

void EnhancedJsonFormatter::start_streaming(std::ostream& output) {
  streaming_active_ = true;
  first_result_ = true;
  output << format_header();
}

void EnhancedJsonFormatter::stream_test_result(const TestResult& result, std::ostream& output) {
  if (!streaming_active_) {
    throw std::runtime_error("Streaming not started");
  }

  if (!first_result_) {
    output << ",\n";
  }

  output << format_test_result(result);
  first_result_ = false;
}

void EnhancedJsonFormatter::end_streaming(std::ostream& output) {
  if (streaming_active_) {
    output << format_footer();
    streaming_active_ = false;
  }
}

FormatValidationResult EnhancedJsonFormatter::validate_output(const std::string& output) {
  FormatValidationResult result(true);

  try {
    validate_json_syntax(output);
  } catch (const std::exception& e) {
    result.is_valid = false;
    result.error_message = e.what();

    // Provide suggestions for common JSON errors
    if (result.error_message.find("comma") != std::string::npos) {
      result.suggestions.push_back("Check for trailing commas or missing commas");
    }
    if (result.error_message.find("quote") != std::string::npos) {
      result.suggestions.push_back("Ensure all strings are properly quoted");
    }
  }

  return result;
}

std::string EnhancedJsonFormatter::recover_from_error(const std::string& invalid_output,
                                                     const std::string& error_context) {
  (void)error_context;  // Suppress unused parameter warning
  std::string recovered = invalid_output;

  // Basic JSON recovery strategies

  // 1. Remove trailing commas
  std::regex trailing_comma(R"(,(\s*[}\]]))");
  recovered = std::regex_replace(recovered, trailing_comma, "$1");

  // 2. Basic quote escaping (simplified)
  // This is a basic approach - a full implementation would need proper parsing
  // For now, we'll skip this complex regex

  // 3. Ensure proper JSON structure
  if (recovered.find("{") == std::string::npos) {
    recovered = "{" + recovered + "}";
  }

  return recovered;
}

void EnhancedJsonFormatter::set_optimization(const FormatOptimization& optimization) {
  optimization_ = optimization;
}

void EnhancedJsonFormatter::set_streaming_config(const StreamingConfig& config) {
  streaming_config_ = config;
}

std::string EnhancedJsonFormatter::json_escape(const std::string& text) {
  std::ostringstream escaped;
  escaped << "\"";

  for (char c : text) {
    switch (c) {
      case '"': escaped << "\\\""; break;
      case '\\': escaped << "\\\\"; break;
      case '\b': escaped << "\\b"; break;
      case '\f': escaped << "\\f"; break;
      case '\n': escaped << "\\n"; break;
      case '\r': escaped << "\\r"; break;
      case '\t': escaped << "\\t"; break;
      default:
        if (c < 0x20) {
          escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
          escaped << c;
        }
        break;
    }
  }

  escaped << "\"";
  return escaped.str();
}

void EnhancedJsonFormatter::validate_json_syntax(const std::string& json) {
  // Basic JSON syntax validation
  std::stack<char> bracket_stack;
  bool in_string = false;
  bool escaped = false;

  for (size_t i = 0; i < json.length(); ++i) {
    char c = json[i];

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
      continue;
    }

    switch (c) {
      case '{':
      case '[':
        bracket_stack.push(c);
        break;
      case '}':
        if (bracket_stack.empty() || bracket_stack.top() != '{') {
          throw std::runtime_error("Mismatched closing brace at position " + std::to_string(i));
        }
        bracket_stack.pop();
        break;
      case ']':
        if (bracket_stack.empty() || bracket_stack.top() != '[') {
          throw std::runtime_error("Mismatched closing bracket at position " + std::to_string(i));
        }
        bracket_stack.pop();
        break;
    }
  }

  if (!bracket_stack.empty()) {
    throw std::runtime_error("Unclosed bracket or brace");
  }

  if (in_string) {
    throw std::runtime_error("Unclosed string");
  }
}

// OutputFormatterFactory implementation
std::unique_ptr<OutputFormatter> OutputFormatterFactory::create_formatter(
    OutputFormat format, const FormatOptimization& optimization) {

  switch (format) {
    case OutputFormat::XML:
    case OutputFormat::JUnit:
      return std::make_unique<EnhancedXmlFormatter>(optimization);
    case OutputFormat::JSON:
      return std::make_unique<EnhancedJsonFormatter>(optimization);
    default:
      throw std::runtime_error("Unsupported output format");
  }
}

std::string OutputFormatterFactory::format_to_string(OutputFormat format) {
  switch (format) {
    case OutputFormat::XML: return "xml";
    case OutputFormat::JSON: return "json";
    case OutputFormat::TAP: return "tap";
    case OutputFormat::JUnit: return "junit";
    case OutputFormat::HTML: return "html";
    case OutputFormat::CSV: return "csv";
    case OutputFormat::Plain: return "plain";
    case OutputFormat::Markdown: return "markdown";
    default: return "unknown";
  }
}

OutputFormat OutputFormatterFactory::string_to_format(const std::string& format_str) {
  std::string lower_format = format_str;
  std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);

  if (lower_format == "xml") return OutputFormat::XML;
  if (lower_format == "json") return OutputFormat::JSON;
  if (lower_format == "tap") return OutputFormat::TAP;
  if (lower_format == "junit") return OutputFormat::JUnit;
  if (lower_format == "html") return OutputFormat::HTML;
  if (lower_format == "csv") return OutputFormat::CSV;
  if (lower_format == "plain") return OutputFormat::Plain;
  if (lower_format == "markdown") return OutputFormat::Markdown;

  throw std::runtime_error("Unknown format: " + format_str);
}

} // namespace SolarSystem::Testing::Formatters
