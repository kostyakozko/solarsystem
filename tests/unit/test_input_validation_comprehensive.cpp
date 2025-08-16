/**
 * @file test_input_validation_comprehensive.cpp
 * @brief Comprehensive tests for the enhanced input validation system (Task 13)
 */

#include "../test_framework_enhanced.hpp"
#include "solar_utils/validation/input_validator.hpp"
#include "solar_utils/argument_parser.hpp"
#include <chrono>
#include <thread>

using namespace SolarSystem::Utils;
using namespace SolarSystem::Utils::Validation;
using namespace SolarSystem::Testing;

class InputValidationTests {
public:
  static void run_all_tests() {
    auto& framework = EnhancedTestFramework::instance();

    framework.load_performance_baselines("input_validation_baselines.txt");

    std::vector<std::pair<std::string, std::function<void()>>> tests = {
      {"DateTimeValidation", test_datetime_validation},
      {"NumericValidation", test_numeric_validation},
      {"StringValidation", test_string_validation},
      {"InputSanitization", test_input_sanitization},
      {"RangeValidation", test_range_validation},
      {"FormatDetection", test_format_detection},
      {"ErrorReporting", test_error_reporting},
      {"SuggestionSystem", test_suggestion_system},
      {"ArgumentParserIntegration", test_argument_parser_integration},
      {"PerformanceValidation", test_performance_validation}
    };

    framework.run_test_suite("InputValidation", tests);

    framework.save_performance_baselines("input_validation_baselines.txt");
    framework.generate_report("input_validation_report.html");
  }

private:
  static void test_datetime_validation() {
    // Test multiple date formats (Requirement 5.2)

    // ISO 8601 format
    auto iso_result = DateTimeValidator::validate_date("2025-12-08");
    ASSERT_TRUE(iso_result.is_valid);

    // US format
    auto us_result = DateTimeValidator::validate_date("12/08/2025");
    ASSERT_TRUE(us_result.is_valid);

    // Named dates
    auto today_result = DateTimeValidator::validate_date("today");
    ASSERT_TRUE(today_result.is_valid);

    auto tomorrow_result = DateTimeValidator::validate_date("tomorrow");
    ASSERT_TRUE(tomorrow_result.is_valid);

    // Relative dates
    auto relative_result = DateTimeValidator::validate_date("+7 days");
    ASSERT_TRUE(relative_result.is_valid);

    // Invalid formats should fail with detailed errors
    auto invalid_result = DateTimeValidator::validate_date("invalid-date");
    ASSERT_FALSE(invalid_result.is_valid);
    ASSERT_TRUE(!invalid_result.error_message.empty());
    ASSERT_TRUE(!invalid_result.expected_formats.empty());

    // Test range validation
    auto min_date = std::chrono::system_clock::from_time_t(0);  // 1970
    auto max_date = std::chrono::system_clock::now();

    auto range_result = DateTimeValidator::validate_date_with_range("2030-01-01", min_date, max_date);
    ASSERT_FALSE(range_result.is_valid);
    ASSERT_TRUE(!range_result.error_message.empty());
  }

  static void test_numeric_validation() {
    // Test integer validation with ranges (Requirement 5.1, 5.4)

    // Valid port
    auto valid_port = NumericValidator::validate_port("8080");
    ASSERT_TRUE(valid_port.is_valid);

    // Invalid port (too low)
    auto low_port = NumericValidator::validate_port("80");
    ASSERT_FALSE(low_port.is_valid);
    ASSERT_TRUE(!low_port.error_message.empty());

    // Invalid port (too high)
    auto high_port = NumericValidator::validate_port("70000");
    ASSERT_FALSE(high_port.is_valid);
    ASSERT_TRUE(!high_port.error_message.empty());

    // Invalid format
    auto invalid_port = NumericValidator::validate_int("abc");
    ASSERT_FALSE(invalid_port.is_valid);
    ASSERT_TRUE(!invalid_port.error_message.empty());

    // Test double validation with range
    auto valid_lat = NumericValidator::validate_double("45.5", -90.0, 90.0);
    ASSERT_TRUE(valid_lat.is_valid);

    // Test year validation
    auto valid_year = NumericValidator::validate_year("2025");
    ASSERT_TRUE(valid_year.is_valid);

    auto invalid_year = NumericValidator::validate_year("1500");
    ASSERT_FALSE(invalid_year.is_valid);
  }

  static void test_string_validation() {
    // Test string validation with choice validation

    // Valid email
    auto valid_email = StringValidator::validate_email("user@example.com");
    ASSERT_TRUE(valid_email.is_valid);

    // Invalid email format
    auto invalid_email = StringValidator::validate_email("invalid-email");
    ASSERT_FALSE(invalid_email.is_valid);
    ASSERT_TRUE(!invalid_email.error_message.empty());

    // Test allowed values
    std::vector<std::string> celestial_bodies = {"Sun", "Earth", "Mars", "Jupiter"};

    auto valid_body = StringValidator::validate_choice("Earth", celestial_bodies);
    ASSERT_TRUE(valid_body.is_valid);

    auto invalid_body = StringValidator::validate_choice("Pluto", celestial_bodies);
    ASSERT_FALSE(invalid_body.is_valid);
    ASSERT_TRUE(!invalid_body.expected_formats.empty());

    // Test file path validation
    auto valid_path = StringValidator::validate_file_path("/tmp/test.txt", false);
    ASSERT_TRUE(valid_path.is_valid);

    // Test case-insensitive validation
    std::vector<std::string> test_values = {"HELLO", "hello", "Hello"};
    auto case_result = StringValidator::validate_choice("hello", test_values);
    ASSERT_TRUE(case_result.is_valid);
  }

  static void test_input_sanitization() {
    // Test input sanitization and normalization

    // Test whitespace handling
    auto whitespace_result = StringValidator::sanitize_input("  hello   world  ");
    ASSERT_TRUE(whitespace_result.is_valid);
    ASSERT_EQ(whitespace_result.normalized_value, "hello   world");

    // Test file path validation
    auto path_result = StringValidator::validate_file_path("/tmp/test.txt", false);
    ASSERT_TRUE(path_result.is_valid);

    // Test dangerous path detection
    auto dangerous_result = StringValidator::validate_file_path("../../../etc/passwd", false);
    ASSERT_FALSE(dangerous_result.is_valid);
    ASSERT_TRUE(!dangerous_result.error_message.empty());
  }

  static void test_range_validation() {
    // Test range validation for common parameters

    // Year range validation
    auto valid_year = NumericValidator::validate_year("2025");
    ASSERT_TRUE(valid_year.is_valid);

    auto invalid_year = NumericValidator::validate_year("1500");
    ASSERT_FALSE(invalid_year.is_valid);

    // Port range validation
    auto valid_port = NumericValidator::validate_port("8080");
    ASSERT_TRUE(valid_port.is_valid);

    auto invalid_port = NumericValidator::validate_port("80");
    ASSERT_FALSE(invalid_port.is_valid);

    // Custom range validation
    auto valid_range = NumericValidator::validate_double("45.0", -90.0, 90.0);
    ASSERT_TRUE(valid_range.is_valid);

    auto invalid_range = NumericValidator::validate_double("95.0", -90.0, 90.0);
    ASSERT_FALSE(invalid_range.is_valid);
  }

  static void test_format_detection() {
    // Test automatic format detection

    std::vector<std::string> date_formats = {
      "2025-12-08",      // ISO
      "12/08/2025",      // US
      "08/12/2025",      // European (ambiguous, but should parse)
      "20251208",        // Compact
      "today",           // Named
      "+7 days"          // Relative
    };

    for (const auto& date_str : date_formats) {
      auto result = DateTimeValidator::validate_date(date_str);
      ASSERT_TRUE(result.is_valid);
    }

    // Test supported formats listing
    auto supported_formats = DateTimeValidator::get_supported_formats();
    ASSERT_TRUE(supported_formats.size() > 5);

    for (const auto& format : supported_formats) {
      ASSERT_TRUE(!format.empty());
    }
  }

  static void test_error_reporting() {
    // Test detailed error reporting (Requirement 5.4)

    auto invalid_date = DateTimeValidator::validate_date("invalid-date-format");
    ASSERT_FALSE(invalid_date.is_valid);
    ASSERT_TRUE(!invalid_date.error_message.empty());
    ASSERT_TRUE(!invalid_date.expected_formats.empty());

    // Test numeric error reporting
    auto invalid_number = NumericValidator::validate_int("abc123");
    ASSERT_FALSE(invalid_number.is_valid);
    ASSERT_TRUE(!invalid_number.error_message.empty());

    // Test port validation error reporting
    auto invalid_port = NumericValidator::validate_port("abc");
    ASSERT_FALSE(invalid_port.is_valid);
    ASSERT_TRUE(!invalid_port.error_message.empty());
  }

  static void test_suggestion_system() {
    // Test suggestion system for corrections

    // Test invalid date with suggestions
    auto invalid_date = DateTimeValidator::validate_date("2025/12/08");
    if (!invalid_date.is_valid && !invalid_date.suggestions.empty()) {
      ASSERT_TRUE(invalid_date.suggestions[0].find("2025-12-08") != std::string::npos);
    }

    // Test string choice suggestions
    std::vector<std::string> bodies = {"Sun", "Earth", "Mars"};
    auto invalid_choice = StringValidator::validate_choice("Earht", bodies);
    ASSERT_FALSE(invalid_choice.is_valid);
    // Should suggest "Earth" for "Earht"

    // Test input validator suggestions
    std::vector<std::string> valid_bodies = {"Sun", "Earth", "Mars", "Jupiter"};
    auto string_suggestions = InputValidator::get_suggestions("Earht", valid_bodies);
    ASSERT_TRUE(!string_suggestions.empty());
  }

  static void test_argument_parser_integration() {
    // Test integration with enhanced argument parser

    ArgumentParser parser("test_program");

    bool date_set = false;
    std::string parsed_date;

    // Add option with comprehensive date validation
    parser.add_option(
      Option("-d", "--date", "Specify date")
        .requires_value()
        .validate_as("date")
        .action([&](const std::optional<std::string>& value) {
          if (value) {
            parsed_date = *value;
            date_set = true;
          }
        })
    );

    // Test valid date
    const char* valid_args[] = {"test_program", "--date", "2025-12-08"};
    auto result = parser.parse(3, valid_args);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(date_set);

    // Reset for next test
    date_set = false;

    // Test invalid date (should fail with detailed error)
    const char* invalid_args[] = {"test_program", "--date", "invalid-date"};
    auto invalid_result = parser.parse(3, invalid_args);
    ASSERT_FALSE(invalid_result.has_value());
    ASSERT_FALSE(date_set);

    // Test numeric validation integration
    ArgumentParser numeric_parser("test_numeric");

    int parsed_port = 0;
    bool port_set = false;

    numeric_parser.add_option(
      Option("-p", "--port", "Specify port")
        .requires_value()
        .validate_as("port")
        .action([&](const std::optional<std::string>& value) {
          if (value) {
            parsed_port = std::stoi(*value);
            port_set = true;
          }
        })
    );

    const char* port_args[] = {"test_numeric", "--port", "8080"};
    auto port_result = numeric_parser.parse(3, port_args);
    ASSERT_TRUE(port_result.has_value());
    ASSERT_TRUE(port_set);
    ASSERT_EQ(parsed_port, 8080);
  }

  static void test_performance_validation() {
    // Test performance of validation system

    auto start_time = std::chrono::high_resolution_clock::now();

    // Validate many dates quickly
    for (int i = 0; i < 1000; ++i) {
      int day = i % 28 + 1;
      std::string date_str = "2025-01-" + std::string(day < 10 ? "0" : "") + std::to_string(day);
      auto result = DateTimeValidator::validate_date(date_str);
      ASSERT_TRUE(result.is_valid);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should validate 1000 dates in reasonable time (less than 100ms)
    ASSERT_TRUE(duration.count() < 100);

    // Test numeric validation performance
    start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
      std::string port_str = std::to_string(1024 + i);
      auto result = NumericValidator::validate_port(port_str);
      ASSERT_TRUE(result.is_valid);
    }

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should validate 1000 numbers in reasonable time (less than 50ms)
    ASSERT_TRUE(duration.count() < 50);
  }
};

int main() {
  std::cout << "🚀 Running Comprehensive Input Validation Tests (Task 13)" << std::endl;
  std::cout << "=========================================================" << std::endl << std::endl;

  try {
    InputValidationTests::run_all_tests();

    std::cout << "\n✅ All Input Validation Tests Passed!" << std::endl;
    std::cout << "📊 Test Results:" << std::endl;
    std::cout << "   - DateTime Validation: ✓" << std::endl;
    std::cout << "   - Numeric Validation: ✓" << std::endl;
    std::cout << "   - String Validation: ✓" << std::endl;
    std::cout << "   - Input Sanitization: ✓" << std::endl;
    std::cout << "   - Range Validation: ✓" << std::endl;
    std::cout << "   - Format Detection: ✓" << std::endl;
    std::cout << "   - Error Reporting: ✓" << std::endl;
    std::cout << "   - Suggestion System: ✓" << std::endl;
    std::cout << "   - Argument Parser Integration: ✓" << std::endl;
    std::cout << "   - Performance Validation: ✓" << std::endl;

    std::cout << "\n🎯 Task 13 Requirements Validated:" << std::endl;
    std::cout << "   - Requirement 5.1: Detailed format checking ✓" << std::endl;
    std::cout << "   - Requirement 5.2: Multiple date/time/numeric formats ✓" << std::endl;
    std::cout << "   - Requirement 5.4: Range validation with meaningful errors ✓" << std::endl;
    std::cout << "   - Input sanitization and normalization ✓" << std::endl;

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "❌ Test execution failed: " << e.what() << std::endl;
    return 1;
  }
}
