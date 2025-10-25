/**
 * @file format_converter.cpp
 * @brief Implementation of format converter for migrating between output formats
 */

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

#include "solar_test/formatters/output_formatter.hpp"

namespace SolarSystem::Testing::Formatters {

// FormatConverter implementation
std::string FormatConverter::convert(const std::string& input, OutputFormat from_format,
                                     OutputFormat to_format) {
  if (from_format == to_format) {
    return input;
  }

  // Convert to intermediate representation first (parsed results)
  std::vector<ParsedTestResult> parsed_results;

  switch (from_format) {
    case OutputFormat::XML:
    case OutputFormat::JUnit:
      parsed_results = parse_xml_results(input);
      break;
    case OutputFormat::JSON:
      parsed_results = parse_json_results(input);
      break;
    default:
      throw std::runtime_error("Unsupported source format for conversion");
  }

  // Convert from intermediate representation to target format
  switch (to_format) {
    case OutputFormat::XML:
    case OutputFormat::JUnit:
      return convert_to_xml(parsed_results);
    case OutputFormat::JSON:
      return convert_to_json(parsed_results);
    case OutputFormat::HTML:
      return convert_to_html(parsed_results);
    case OutputFormat::CSV:
      return convert_to_csv(parsed_results);
    default:
      throw std::runtime_error("Unsupported target format for conversion");
  }
}

std::string FormatConverter::xml_to_json(const std::string& xml) {
  return convert(xml, OutputFormat::XML, OutputFormat::JSON);
}

std::string FormatConverter::json_to_xml(const std::string& json) {
  return convert(json, OutputFormat::JSON, OutputFormat::XML);
}

std::string FormatConverter::xml_to_html(const std::string& xml) {
  return convert(xml, OutputFormat::XML, OutputFormat::HTML);
}

std::string FormatConverter::json_to_csv(const std::string& json) {
  return convert(json, OutputFormat::JSON, OutputFormat::CSV);
}

FormatValidationResult FormatConverter::validate_conversion(const std::string& input,
                                                            const std::string& output,
                                                            OutputFormat from_format,
                                                            OutputFormat to_format) {
  FormatValidationResult result(true);

  try {
    // Validate input format
    auto input_validation = OutputValidator::validate_test_content(input, from_format);
    if (!input_validation.is_valid) {
      result.is_valid = false;
      result.error_message = "Invalid input format: " + input_validation.error_message;
      return result;
    }

    // Validate output format
    auto output_validation = OutputValidator::validate_test_content(output, to_format);
    if (!output_validation.is_valid) {
      result.is_valid = false;
      result.error_message = "Invalid output format: " + output_validation.error_message;
      return result;
    }

    // Validate content preservation (basic check)
    auto input_results =
        (from_format == OutputFormat::XML) ? parse_xml_results(input) : parse_json_results(input);
    auto output_results =
        (to_format == OutputFormat::XML) ? parse_xml_results(output) : parse_json_results(output);

    if (input_results.size() != output_results.size()) {
      result.warnings.push_back("Number of test results changed during conversion");
    }

    // Check for data loss in key fields
    for (size_t i = 0; i < std::min(input_results.size(), output_results.size()); ++i) {
      if (input_results[i].name != output_results[i].name) {
        result.warnings.push_back("Test name changed during conversion: " + input_results[i].name +
                                  " -> " + output_results[i].name);
      }
      if (input_results[i].status != output_results[i].status) {
        result.warnings.push_back("Test status changed during conversion for: " +
                                  input_results[i].name);
      }
    }

  } catch (const std::exception& e) {
    result.is_valid = false;
    result.error_message = "Conversion validation error: " + std::string(e.what());
  }

  return result;
}

std::vector<FormatValidationResult> FormatConverter::convert_batch(
    const std::vector<ConversionJob>& jobs) {
  std::vector<FormatValidationResult> results;
  results.reserve(jobs.size());

  for (const auto& job : jobs) {
    FormatValidationResult result(true);

    try {
      // Read input file
      std::ifstream input_file(job.input_file);
      if (!input_file) {
        result.is_valid = false;
        result.error_message = "Cannot open input file: " + job.input_file;
        results.push_back(result);
        continue;
      }

      std::string input_content((std::istreambuf_iterator<char>(input_file)),
                                std::istreambuf_iterator<char>());
      input_file.close();

      // Perform conversion
      std::string output_content = convert(input_content, job.from_format, job.to_format);

      // Write output file
      std::ofstream output_file(job.output_file);
      if (!output_file) {
        result.is_valid = false;
        result.error_message = "Cannot create output file: " + job.output_file;
        results.push_back(result);
        continue;
      }

      output_file << output_content;
      output_file.close();

      // Validate conversion
      result = validate_conversion(input_content, output_content, job.from_format, job.to_format);

    } catch (const std::exception& e) {
      result.is_valid = false;
      result.error_message = "Batch conversion error: " + std::string(e.what());
    }

    results.push_back(result);
  }

  return results;
}

std::vector<FormatConverter::ParsedTestResult> FormatConverter::parse_xml_results(
    const std::string& xml) {
  std::vector<ParsedTestResult> results;

  // Parse testcase elements
  std::regex testcase_regex(
      "<testcase[^>]*name=\"([^\"]*)\"[^>]*time=\"([^\"]*)\"[^>]*>(.*?)</testcase>");
  std::sregex_iterator iter(xml.begin(), xml.end(), testcase_regex);
  std::sregex_iterator end;

  for (; iter != end; ++iter) {
    const std::smatch& match = *iter;
    ParsedTestResult result;

    result.name = match[1].str();

    // Parse duration
    try {
      double seconds = std::stod(match[2].str());
      result.duration = std::chrono::milliseconds(static_cast<long>(seconds * 1000));
    } catch (...) {
      result.duration = std::chrono::milliseconds(0);
    }

    std::string content = match[3].str();

    // Determine status from content
    if (content.find("<failure") != std::string::npos) {
      result.status = "failed";

      // Extract failure message
      std::regex failure_regex("<failure[^>]*message=\"([^\"]*)\"");
      std::smatch failure_match;
      if (std::regex_search(content, failure_match, failure_regex)) {
        result.error_message = failure_match[1].str();
      }
    } else if (content.find("<error") != std::string::npos) {
      result.status = "error";

      // Extract error message
      std::regex error_regex("<error[^>]*message=\"([^\"]*)\"");
      std::smatch error_match;
      if (std::regex_search(content, error_match, error_regex)) {
        result.error_message = error_match[1].str();
      }
    } else if (content.find("<skipped") != std::string::npos) {
      result.status = "skipped";
    } else {
      result.status = "passed";
    }

    results.push_back(result);
  }

  return results;
}

std::vector<FormatConverter::ParsedTestResult> FormatConverter::parse_json_results(
    const std::string& json) {
  std::vector<ParsedTestResult> results;

  // Simple JSON parsing for test results
  // This is a basic implementation - a full implementation would use a proper JSON parser

  // Find the tests array
  size_t tests_pos = json.find("\"tests\":");
  if (tests_pos == std::string::npos) {
    return results;
  }

  // Find the opening bracket of the tests array
  size_t array_start = json.find("[", tests_pos);
  if (array_start == std::string::npos) {
    return results;
  }

  // Parse individual test objects
  std::regex test_regex(
      "\\{\\s*\"name\":\\s*\"([^\"]*)\"[^}]*\"status\":\\s*\"([^\"]*)\"[^}]*\"duration_ms\":\\s*("
      "\\d+)[^}]*(?:\"error_message\":\\s*\"([^\"]*)\")?[^}]*\\}");
  std::sregex_iterator iter(json.begin(), json.end(), test_regex);
  std::sregex_iterator end;

  for (; iter != end; ++iter) {
    const std::smatch& match = *iter;
    ParsedTestResult result;

    result.name = match[1].str();
    result.status = match[2].str();

    // Parse duration
    try {
      long ms = std::stol(match[3].str());
      result.duration = std::chrono::milliseconds(ms);
    } catch (...) {
      result.duration = std::chrono::milliseconds(0);
    }

    if (match[4].matched) {
      result.error_message = match[4].str();
    }

    results.push_back(result);
  }

  return results;
}

std::string FormatConverter::convert_to_xml(const std::vector<ParsedTestResult>& results) {
  std::ostringstream xml;

  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml << "<testsuites>\n";
  xml << "  <testsuite name=\"ConvertedTests\" tests=\"" << results.size() << "\">\n";

  for (const auto& result : results) {
    xml << "    <testcase name=\"" << xml_escape(result.name) << "\" ";
    xml << "time=\"" << std::fixed << std::setprecision(3)
        << (static_cast<double>(result.duration.count()) / 1000.0) << "\"";

    if (result.status == "passed") {
      xml << "/>\n";
    } else {
      xml << ">\n";

      if (result.status == "failed") {
        xml << "      <failure message=\"" << xml_escape(result.error_message) << "\"/>\n";
      } else if (result.status == "error") {
        xml << "      <error message=\"" << xml_escape(result.error_message) << "\"/>\n";
      } else if (result.status == "skipped") {
        xml << "      <skipped/>\n";
      }

      xml << "    </testcase>\n";
    }
  }

  xml << "  </testsuite>\n";
  xml << "</testsuites>\n";

  return xml.str();
}

std::string FormatConverter::convert_to_json(const std::vector<ParsedTestResult>& results) {
  std::ostringstream json;

  json << "{\n";
  json << "  \"suite_name\": \"ConvertedTests\",\n";
  json << "  \"total_tests\": " << results.size() << ",\n";
  json << "  \"tests\": [\n";

  for (size_t i = 0; i < results.size(); ++i) {
    const auto& result = results[i];

    json << "    {\n";
    json << "      \"name\": " << json_escape(result.name) << ",\n";
    json << "      \"status\": " << json_escape(result.status) << ",\n";
    json << "      \"duration_ms\": " << result.duration.count();

    if (!result.error_message.empty()) {
      json << ",\n      \"error_message\": " << json_escape(result.error_message);
    }

    json << "\n    }";

    if (i < results.size() - 1) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ]\n";
  json << "}";

  return json.str();
}

std::string FormatConverter::convert_to_html(const std::vector<ParsedTestResult>& results) {
  std::ostringstream html;

  html << "<!DOCTYPE html>\n";
  html << "<html>\n";
  html << "<head>\n";
  html << "  <title>Test Results</title>\n";
  html << "  <style>\n";
  html << "    table { border-collapse: collapse; width: 100%; }\n";
  html << "    th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
  html << "    th { background-color: #f2f2f2; }\n";
  html << "    .passed { color: green; }\n";
  html << "    .failed { color: red; }\n";
  html << "    .error { color: red; font-weight: bold; }\n";
  html << "    .skipped { color: orange; }\n";
  html << "  </style>\n";
  html << "</head>\n";
  html << "<body>\n";
  html << "  <h1>Test Results</h1>\n";
  html << "  <table>\n";
  html << "    <tr><th>Test Name</th><th>Status</th><th>Duration (ms)</th><th>Error "
          "Message</th></tr>\n";

  for (const auto& result : results) {
    html << "    <tr>\n";
    html << "      <td>" << html_escape(result.name) << "</td>\n";
    html << "      <td class=\"" << result.status << "\">" << result.status << "</td>\n";
    html << "      <td>" << result.duration.count() << "</td>\n";
    html << "      <td>" << html_escape(result.error_message) << "</td>\n";
    html << "    </tr>\n";
  }

  html << "  </table>\n";
  html << "</body>\n";
  html << "</html>\n";

  return html.str();
}

std::string FormatConverter::convert_to_csv(const std::vector<ParsedTestResult>& results) {
  std::ostringstream csv;

  // Header
  csv << "Test Name,Status,Duration (ms),Error Message\n";

  // Data rows
  for (const auto& result : results) {
    csv << csv_escape(result.name) << ",";
    csv << csv_escape(result.status) << ",";
    csv << result.duration.count() << ",";
    csv << csv_escape(result.error_message) << "\n";
  }

  return csv.str();
}

std::string FormatConverter::xml_escape(const std::string& text) {
  std::string escaped;
  escaped.reserve(static_cast<size_t>(text.length() * 1.2));

  for (char c : text) {
    switch (c) {
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '&':
        escaped += "&amp;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&apos;";
        break;
      default:
        escaped += c;
        break;
    }
  }

  return escaped;
}

std::string FormatConverter::json_escape(const std::string& text) {
  std::ostringstream escaped;
  escaped << "\"";

  for (char c : text) {
    switch (c) {
      case '"':
        escaped << "\\\"";
        break;
      case '\\':
        escaped << "\\\\";
        break;
      case '\b':
        escaped << "\\b";
        break;
      case '\f':
        escaped << "\\f";
        break;
      case '\n':
        escaped << "\\n";
        break;
      case '\r':
        escaped << "\\r";
        break;
      case '\t':
        escaped << "\\t";
        break;
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

std::string FormatConverter::html_escape(const std::string& text) {
  std::string escaped;
  escaped.reserve(static_cast<size_t>(text.length() * 1.2));

  for (char c : text) {
    switch (c) {
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '&':
        escaped += "&amp;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      default:
        escaped += c;
        break;
    }
  }

  return escaped;
}

std::string FormatConverter::csv_escape(const std::string& text) {
  if (text.find(',') == std::string::npos && text.find('"') == std::string::npos &&
      text.find('\n') == std::string::npos) {
    return text;
  }

  std::string escaped = "\"";
  for (char c : text) {
    if (c == '"') {
      escaped += "\"\"";  // Escape quotes by doubling them
    } else {
      escaped += c;
    }
  }
  escaped += "\"";

  return escaped;
}

}  // namespace SolarSystem::Testing::Formatters
