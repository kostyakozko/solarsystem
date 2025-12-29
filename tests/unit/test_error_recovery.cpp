/**
 * @file test_error_recovery.cpp
 * @brief Error message and recovery validation tests (Task 19)
 * @note Migrated to Google Test
 *
 * Tests error message and recovery capabilities:
 * - Error message quality and usefulness testing
 * - Error recovery mechanism validation
 * - User guidance and help message testing
 * - Error logging and diagnostic information validation
 *
 * Requirements: 6.4, 6.5
 */

#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

/**
 * @brief Error message and recovery validation system
 */
class ErrorRecoveryValidator {
 public:
  // Error message quality metrics
  struct MessageQuality {
    bool has_error_code = false;
    bool has_description = false;
    bool has_context = false;
    bool has_suggestion = false;
    bool is_actionable = false;
    bool is_clear = false;
    double quality_score = 0.0; // 0.0 to 1.0
  };

  // Recovery mechanism result
  struct RecoveryResult {
    bool attempted = false;
    bool succeeded = false;
    int attempts = 0;
    std::string recovery_action;
    std::string final_state;
  };

  // Error log entry
  struct LogEntry {
    std::string timestamp;
    std::string level;      // ERROR, WARNING, INFO
    std::string message;
    std::string context;
    std::string stack_trace;
  };

  // Validate error message quality
  MessageQuality validate_error_message(const std::string& error_msg) {
    MessageQuality quality;

    // Check for error code (e.g., ERR_001, ERROR-123)
    std::regex error_code_pattern(R"(ERR[_-]?\d+|ERROR[_-]?\d+)");
    quality.has_error_code = std::regex_search(error_msg, error_code_pattern);

    // Check for description (message should be reasonably long)
    quality.has_description = error_msg.length() > 20;

    // Check for context (file, line, function names)
    quality.has_context =
        (error_msg.find("file") != std::string::npos ||
         error_msg.find("line") != std::string::npos ||
         error_msg.find("function") != std::string::npos ||
         error_msg.find("at") != std::string::npos);

    // Check for suggestions (try, check, ensure, verify)
    quality.has_suggestion =
        (error_msg.find("try") != std::string::npos ||
         error_msg.find("check") != std::string::npos ||
         error_msg.find("ensure") != std::string::npos ||
         error_msg.find("verify") != std::string::npos ||
         error_msg.find("should") != std::string::npos);

    // Check if actionable (contains action verbs)
    quality.is_actionable =
        (error_msg.find("retry") != std::string::npos ||
         error_msg.find("restart") != std::string::npos ||
         error_msg.find("contact") != std::string::npos ||
         error_msg.find("update") != std::string::npos);

    // Check clarity (no excessive technical jargon, reasonable length)
    quality.is_clear = (error_msg.length() < 500 && error_msg.length() > 10);

    // Calculate quality score
    int score = 0;
    if (quality.has_error_code) score++;
    if (quality.has_description) score++;
    if (quality.has_context) score++;
    if (quality.has_suggestion) score++;
    if (quality.is_actionable) score++;
    if (quality.is_clear) score++;

    quality.quality_score = score / 6.0;

    return quality;
  }

  // Test recovery mechanism
  RecoveryResult test_recovery_mechanism(
      std::function<bool()> operation,
      std::function<bool()> recovery,
      int max_attempts = 3) {
    RecoveryResult result;
    result.attempted = true;

    for (int i = 0; i < max_attempts; ++i) {
      result.attempts++;

      try {
        if (operation()) {
          result.succeeded = true;
          result.final_state = "Success";
          return result;
        }
      } catch (...) {
        // Operation failed, try recovery
        try {
          if (recovery && recovery()) {
            result.recovery_action = "Recovery successful on attempt " +
                                     std::to_string(i + 1);
          }
        } catch (...) {
          result.recovery_action = "Recovery failed";
        }
      }
    }

    result.succeeded = false;
    result.final_state = "Failed after " + std::to_string(max_attempts) +
                         " attempts";
    return result;
  }

  // Validate help message
  bool validate_help_message(const std::string& help_msg) {
    if (help_msg.empty()) return false;

    // Should contain usage information
    bool has_usage = (help_msg.find("usage") != std::string::npos ||
                      help_msg.find("Usage") != std::string::npos ||
                      help_msg.find("USAGE") != std::string::npos);

    // Should contain examples
    bool has_examples = (help_msg.find("example") != std::string::npos ||
                         help_msg.find("Example") != std::string::npos);

    // Should contain options/flags
    bool has_options = (help_msg.find("option") != std::string::npos ||
                        help_msg.find("--") != std::string::npos ||
                        help_msg.find("-") != std::string::npos);

    return has_usage || has_examples || has_options;
  }

  // Create error log entry
  LogEntry create_log_entry(const std::string& level,
                            const std::string& message,
                            const std::string& context = "") {
    LogEntry entry;
    entry.timestamp = "2024-01-01T12:00:00Z";
    entry.level = level;
    entry.message = message;
    entry.context = context;

    if (level == "ERROR") {
      entry.stack_trace = "at function() [file.cpp:123]";
    }

    return entry;
  }

  // Validate log entry
  bool validate_log_entry(const LogEntry& entry) {
    if (entry.timestamp.empty()) return false;
    if (entry.level.empty()) return false;
    if (entry.message.empty()) return false;

    // Level should be valid
    if (entry.level != "ERROR" && entry.level != "WARNING" &&
        entry.level != "INFO") {
      return false;
    }

    return true;
  }

  // Generate user guidance
  std::string generate_user_guidance(const std::string& error_type) {
    std::string guidance;

    if (error_type == "file_not_found") {
      guidance = "File not found. Please check:\n";
      guidance += "1. Verify the file path is correct\n";
      guidance += "2. Ensure the file exists\n";
      guidance += "3. Check file permissions\n";
    } else if (error_type == "network_error") {
      guidance = "Network error occurred. Try:\n";
      guidance += "1. Check your internet connection\n";
      guidance += "2. Verify the server is accessible\n";
      guidance += "3. Retry the operation\n";
    } else if (error_type == "invalid_input") {
      guidance = "Invalid input provided. Please:\n";
      guidance += "1. Check the input format\n";
      guidance += "2. Refer to the documentation\n";
      guidance += "3. Use --help for usage information\n";
    } else {
      guidance = "An error occurred. Contact support for assistance.";
    }

    return guidance;
  }

  // Validate diagnostic information
  bool validate_diagnostics(const std::string& diagnostics) {
    if (diagnostics.empty()) return false;

    // Should contain system information
    bool has_system_info =
        (diagnostics.find("system") != std::string::npos ||
         diagnostics.find("version") != std::string::npos ||
         diagnostics.find("platform") != std::string::npos);

    // Should contain error details
    bool has_error_details =
        (diagnostics.find("error") != std::string::npos ||
         diagnostics.find("failed") != std::string::npos);

    // Should contain timestamp
    bool has_timestamp = (diagnostics.find("time") != std::string::npos ||
                          diagnostics.find("date") != std::string::npos ||
                          std::regex_search(diagnostics, std::regex(R"(\d{4}-\d{2}-\d{2})")));

    return has_system_info || has_error_details || has_timestamp;
  }
};
  TEST_SUITE("Error Message and Recovery Validation Tests");

  // Test 1: Error message quality testing
  TEST_CASE("Error Message Quality") {
    ErrorRecoveryValidator validator;

    // Test 1.1: High-quality error message
    std::string error_msg1 =
        "ERR_001: File not found at line 42 in function load_data(). "
        "Please check the file path and try again.";
    auto quality1 = validator.validate_error_message(error_msg1);
    EXPECT_GT(quality1.has_error_code && quality1.has_description &&
                quality1.has_context && quality1.has_suggestion &&
                quality1.is_clear && quality1.quality_score , 0.7);

    // Test 1.2: Poor-quality error message
    std::string error_msg2 = "Error";
    auto quality2 = validator.validate_error_message(error_msg2);
    EXPECT_LT(!quality2.has_error_code && !quality2.has_description &&
                !quality2.has_context && quality2.quality_score , 0.5);

    // Test 1.3: Message with actionable guidance
    std::string error_msg3 = "Connection failed. Please retry or contact support.";
    auto quality3 = validator.validate_error_message(error_msg3);
    ASSERT_TRUE(quality3.is_actionable && quality3.has_description);

    // Test 1.4: Message with context
    std::string error_msg4 = "Invalid parameter at file config.cpp line 156";
    auto quality4 = validator.validate_error_message(error_msg4);
    ASSERT_TRUE(quality4.has_context);
  });

  // Test 2: Error recovery mechanism validation
  TEST_CASE("Error Recovery Mechanism") {
    ErrorRecoveryValidator validator;

    // Test 2.1: Successful recovery on first attempt
    {
      int attempt = 0;
      auto operation = [&attempt]() {
        attempt++;
        return attempt >= 1;
      };
      auto recovery = []() { return true; };

      auto result = validator.test_recovery_mechanism(operation, recovery);
      ASSERT_TRUE(result.attempted);
      ASSERT_TRUE(result.succeeded);
      ASSERT_EQ(result.attempts, 1);
    }

    // Test 2.2: Recovery after multiple attempts
    {
      int attempt = 0;
      auto operation = [&attempt]() {
        attempt++;
        return attempt >= 3;
      };
      auto recovery = []() { return true; };

      auto result = validator.test_recovery_mechanism(operation, recovery, 5);
      ASSERT_TRUE(result.succeeded);
      ASSERT_EQ(result.attempts, 3);
    }

    // Test 2.3: Failed recovery
    {
      auto operation = []() { return false; };
      auto recovery = []() { return false; };

      auto result = validator.test_recovery_mechanism(operation, recovery, 3);
      ASSERT_TRUE(result.attempted);
      ASSERT_FALSE(result.succeeded);
      ASSERT_EQ(result.attempts, 3);
    }

    // Test 2.4: Recovery with exception
    {
      auto operation = []() -> bool { throw std::runtime_error("Error"); };
      auto recovery = []() -> bool { return true; };

      auto result = validator.test_recovery_mechanism(operation, recovery, 2);
      ASSERT_TRUE(result.attempted);
      ASSERT_FALSE(result.succeeded);
      ASSERT_FALSE(result.recovery_action.empty());
    }
  });

  // Test 3: User guidance and help messages
  TEST_CASE("User Guidance and Help Messages") {
    ErrorRecoveryValidator validator;

    // Test 3.1: Validate comprehensive help message
    {
      std::string help_msg =
          "Usage: program [options]\n"
          "Options:\n"
          "  --help     Show this help message\n"
          "  --version  Show version\n"
          "Example: program --help";

      bool valid = validator.validate_help_message(help_msg);
      ASSERT_TRUE(valid);
    }

    // Test 3.2: Validate minimal help message
    {
      std::string help_msg = "Usage: program [options]";
      bool valid = validator.validate_help_message(help_msg);
      ASSERT_TRUE(valid);
    }

    // Test 3.3: Invalid help message (empty)
    {
      std::string help_msg = "";
      bool valid = validator.validate_help_message(help_msg);
      ASSERT_FALSE(valid);
    }

    // Test 3.4: Generate user guidance
    {
      auto guidance = validator.generate_user_guidance("file_not_found");
      ASSERT_FALSE(guidance.empty());
      EXPECT_NE(std::string::npos, guidance.find("file"));
      ASSERT_TRUE(guidance.find("check") != std::string::npos ||
                  guidance.find("verify") != std::string::npos);
    }

    // Test 3.5: Network error guidance
    {
      auto guidance = validator.generate_user_guidance("network_error");
      ASSERT_TRUE(guidance.find("network") != std::string::npos ||
                  guidance.find("connection") != std::string::npos);
    }

    // Test 3.6: Invalid input guidance
    {
      auto guidance = validator.generate_user_guidance("invalid_input");
      EXPECT_NE(std::string::npos, guidance.find("input"));
      EXPECT_NE(std::string::npos, guidance.find("help"));
    }
  });

  // Test 4: Error logging validation
  TEST_CASE("Error Logging Validation") {
    ErrorRecoveryValidator validator;

    // Test 4.1: Create and validate error log entry
    {
      auto entry = validator.create_log_entry("ERROR", "Test error message",
                                               "test_context");
      ASSERT_TRUE(validator.validate_log_entry(entry));
      ASSERT_EQ(entry.level, "ERROR");
      ASSERT_FALSE(entry.message.empty());
      ASSERT_FALSE(entry.timestamp.empty());
      ASSERT_FALSE(entry.stack_trace.empty());
    }

    // Test 4.2: Create warning log entry
    {
      auto entry = validator.create_log_entry("WARNING", "Test warning");
      ASSERT_TRUE(validator.validate_log_entry(entry));
      ASSERT_EQ(entry.level, "WARNING");
    }

    // Test 4.3: Create info log entry
    {
      auto entry = validator.create_log_entry("INFO", "Test info");
      ASSERT_TRUE(validator.validate_log_entry(entry));
      ASSERT_EQ(entry.level, "INFO");
    }

    // Test 4.4: Invalid log entry (empty message)
    {
      ErrorRecoveryValidator::LogEntry entry;
      entry.timestamp = "2024-01-01";
      entry.level = "ERROR";
      entry.message = "";

      ASSERT_FALSE(validator.validate_log_entry(entry));
    }

    // Test 4.5: Invalid log entry (invalid level)
    {
      ErrorRecoveryValidator::LogEntry entry;
      entry.timestamp = "2024-01-01";
      entry.level = "INVALID";
      entry.message = "Test";

      ASSERT_FALSE(validator.validate_log_entry(entry));
    }
  });

  // Test 5: Diagnostic information validation
  TEST_CASE("Diagnostic Information Validation") {
    ErrorRecoveryValidator validator;

    // Test 5.1: Comprehensive diagnostics
    {
      std::string diagnostics =
          "System: Linux 5.4.0\n"
          "Version: 1.0.0\n"
          "Error: Connection failed\n"
          "Time: 2024-01-01 12:00:00";

      bool valid = validator.validate_diagnostics(diagnostics);
      ASSERT_TRUE(valid);
    }

    // Test 5.2: Minimal diagnostics
    {
      std::string diagnostics = "Error occurred at 2024-01-01";
      bool valid = validator.validate_diagnostics(diagnostics);
      ASSERT_TRUE(valid);
    }

    // Test 5.3: System info only
    {
      std::string diagnostics = "System version: 2.0.0";
      bool valid = validator.validate_diagnostics(diagnostics);
      ASSERT_TRUE(valid);
    }

    // Test 5.4: Empty diagnostics
    {
      std::string diagnostics = "";
      bool valid = validator.validate_diagnostics(diagnostics);
      ASSERT_FALSE(valid);
    }
  });

  // Test 6: Integrated error handling scenarios
  TEST_CASE("Integrated Error Handling Scenarios") {
    ErrorRecoveryValidator validator;

    // Test 6.1: Complete error handling workflow
    std::string error_msg =
        "ERR_404: Resource not found at line 100. "
        "Please verify the resource path and try again.";

    auto msg_quality = validator.validate_error_message(error_msg);
    auto user_guidance = validator.generate_user_guidance("file_not_found");
    auto error_log_entry = validator.create_log_entry("ERROR", error_msg);
    auto operation = []() { return false; };
    auto recovery = []() { return true; };
    auto recovery_result = validator.test_recovery_mechanism(operation, recovery, 1);

    // Test 6.1 validation
    bool test61_passed = (msg_quality.quality_score > 0.5 && !user_guidance.empty() &&
                          validator.validate_log_entry(error_log_entry) &&
                          recovery_result.attempted);

    // Test 6.2: Multi-level error handling
    std::vector<ErrorRecoveryValidator::LogEntry> error_log;
    error_log.push_back(validator.create_log_entry("WARNING", "Low memory"));
    error_log.push_back(validator.create_log_entry("ERROR", "Connection failed"));
    error_log.push_back(validator.create_log_entry("INFO", "Retrying connection"));

    bool all_valid = true;
    for (const auto& entry : error_log) {
      if (!validator.validate_log_entry(entry)) {
        all_valid = false;
        break;
      }
    }
    bool test62_passed = (all_valid && error_log.size() == 3);

    // Test 6.3: Error recovery with logging
    std::vector<std::string> recovery_log;
    auto op3 = []() -> bool { throw std::runtime_error("Error"); };
    auto rec3 = [&recovery_log]() {
      recovery_log.push_back("Recovery attempted");
      return true;
    };
    auto result3 = validator.test_recovery_mechanism(op3, rec3, 3);
    bool test63_passed = (recovery_log.size() == 3);

    // Combined assertion for all sub-tests
    [&]() {
      ASSERT_TRUE(test61_passed);
      ASSERT_TRUE(test62_passed);
      ASSERT_TRUE(test63_passed);
    }();
  });

  return current_suite->all_passed() ? 0 : 1;
