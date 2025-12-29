/**
 * @file test_error_messaging.cpp
 * @brief Unit tests for comprehensive error messaging system
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>
#include "solar_core/error/error_messaging.hpp"

using namespace SolarSystem::Error;
TEST(ErrorMessagingTests, Error_Message_Builder) {
    ErrorMessage error = ErrorMessageBuilder()
      .code("TEST001")
      .title("Test Error")
      .description("This is a test error")
      .severity(ErrorSeverity::ERROR)
      .category(ErrorCategory::CONFIGURATION)
      .context("Test context")
      .add_cause("Test cause 1")
      .add_cause("Test cause 2")
      .technical_details("Technical info")
      .user_reportable(true)
      .build();

    if (error.error_code != "TEST001") throw std::runtime_error("Error code mismatch");
    if (error.title != "Test Error") throw std::runtime_error("Title mismatch");
    if (error.severity != ErrorSeverity::ERROR) throw std::runtime_error("Severity mismatch");
    if (error.causes.size() != 2) throw std::runtime_error("Causes count mismatch");
}
TEST(ErrorMessagingTests, Register_Error_Template) {
    auto& messaging = ErrorMessaging::instance();

    ErrorMessage template_error = ErrorMessageBuilder()
      .code("TEMPLATE001")
      .title("Template Error")
      .description("Template description")
      .severity(ErrorSeverity::WARNING)
      .category(ErrorCategory::INPUT_VALIDATION)
      .build();

    messaging.register_error_template("TEMPLATE001", template_error);

    auto retrieved = messaging.get_error_template("TEMPLATE001");
    if (!retrieved) throw std::runtime_error("Template not found");
    if (retrieved->title != "Template Error") throw std::runtime_error("Template title mismatch");
}
TEST(ErrorMessagingTests, Format_Error_Simple) {
    auto& messaging = ErrorMessaging::instance();

    ErrorMessage error = ErrorMessageBuilder()
      .code("FORMAT001")
      .title("Format Test")
      .description("Test description")
      .severity(ErrorSeverity::INFO)
      .category(ErrorCategory::COMPUTATION)
      .build();

    std::string formatted = messaging.format_error_simple(error);
    if (formatted.empty()) throw std::runtime_error("Formatted output is empty");
    if (formatted.find("Format Test") == std::string::npos) throw std::runtime_error("Title not in output");
}
TEST(ErrorMessagingTests, Format_Error_Detailed) {
    auto& messaging = ErrorMessaging::instance();

    RecoveryAction action;
    action.description = "Try this fix";
    action.command = "fix_command";
    action.automatic = false;

    ErrorMessage error = ErrorMessageBuilder()
      .code("DETAIL001")
      .title("Detailed Error")
      .description("Detailed description")
      .severity(ErrorSeverity::CRITICAL)
      .category(ErrorCategory::FILE_SYSTEM)
      .add_cause("Cause 1")
      .add_recovery_action(action)
      .add_related_doc("doc1.md")
      .build();

    std::string formatted = messaging.format_error_detailed(error);
    if (formatted.empty()) throw std::runtime_error("Detailed output is empty");
    if (formatted.find("Detailed Error") == std::string::npos) throw std::runtime_error("Title not in output");
    if (formatted.find("Try this fix") == std::string::npos) throw std::runtime_error("Recovery action not in output");
}
TEST(ErrorMessagingTests, Format_Error_JSON) {
    auto& messaging = ErrorMessaging::instance();

    ErrorMessage error = ErrorMessageBuilder()
      .code("JSON001")
      .title("JSON Error")
      .description("JSON description")
      .severity(ErrorSeverity::ERROR)
      .category(ErrorCategory::NETWORK)
      .build();

    std::string json = messaging.format_error_json(error);
    if (json.empty()) throw std::runtime_error("JSON output is empty");
    if (json.find("\"error_code\"") == std::string::npos) throw std::runtime_error("JSON format invalid");
}
TEST(ErrorMessagingTests, Error_Reporting) {
    auto& messaging = ErrorMessaging::instance();

    messaging.clear_error_history();

    ErrorMessage error = ErrorMessageBuilder()
      .code("REPORT001")
      .title("Report Test")
      .severity(ErrorSeverity::WARNING)
      .category(ErrorCategory::MEMORY)
      .build();

    messaging.report_error(error);

    auto history = messaging.get_error_history();
    if (history.empty()) throw std::runtime_error("Error history is empty");
    if (history[0].error_code != "REPORT001") throw std::runtime_error("Error code mismatch in history");
}
TEST(ErrorMessagingTests, Error_Statistics) {
    auto& messaging = ErrorMessaging::instance();

    messaging.clear_error_history();

    ErrorMessage error1 = ErrorMessageBuilder()
      .code("STAT001")
      .severity(ErrorSeverity::ERROR)
      .category(ErrorCategory::CONFIGURATION)
      .build();

    ErrorMessage error2 = ErrorMessageBuilder()
      .code("STAT002")
      .severity(ErrorSeverity::ERROR)
      .category(ErrorCategory::INPUT_VALIDATION)
      .build();

    ErrorMessage error3 = ErrorMessageBuilder()
      .code("STAT003")
      .severity(ErrorSeverity::WARNING)
      .category(ErrorCategory::CONFIGURATION)
      .build();

    messaging.report_error(error1);
    messaging.report_error(error2);
    messaging.report_error(error3);

    if (messaging.get_error_count() != 3) throw std::runtime_error("Total error count mismatch");
    if (messaging.get_error_count_by_severity(ErrorSeverity::ERROR) != 2) throw std::runtime_error("Error severity count mismatch");
    if (messaging.get_error_count_by_category(ErrorCategory::CONFIGURATION) != 2) throw std::runtime_error("Error category count mismatch");
}
TEST(ErrorMessagingTests, Recovery_Actions) {
    auto& messaging = ErrorMessaging::instance();

    RecoveryAction action1;
    action1.description = "Action 1";
    action1.command = "cmd1";
    action1.automatic = true;

    RecoveryAction action2;
    action2.description = "Action 2";
    action2.command = "cmd2";
    action2.automatic = false;

    ErrorMessage template_error = ErrorMessageBuilder()
      .code("RECOVERY001")
      .add_recovery_action(action1)
      .add_recovery_action(action2)
      .build();

    messaging.register_error_template("RECOVERY001", template_error);

    auto actions = messaging.suggest_recovery_actions("RECOVERY001");
    if (actions.size() != 2) throw std::runtime_error("Recovery actions count mismatch");
    if (actions[0].description != "Action 1") throw std::runtime_error("Recovery action description mismatch");
}
TEST(ErrorMessagingTests, Error_Feedback) {
    auto& feedback = ErrorFeedback::instance();

    feedback.submit_feedback("FEEDBACK001", "This error is confusing");
    feedback.submit_feedback("FEEDBACK001", "Needs better explanation");

    auto feedback_list = feedback.get_feedback("FEEDBACK001");
    if (feedback_list.size() != 2) throw std::runtime_error("Feedback count mismatch");
}
TEST(ErrorMessagingTests, Error_Report_Generation) {
    auto& feedback = ErrorFeedback::instance();

    ErrorMessage error = ErrorMessageBuilder()
      .code("REPORT_GEN001")
      .title("Report Generation Test")
      .description("Test description")
      .severity(ErrorSeverity::CRITICAL)
      .category(ErrorCategory::INTERNAL)
      .context("Test context")
      .technical_details("Technical info")
      .build();

    std::string report = feedback.generate_error_report(error);
    if (report.empty()) throw std::runtime_error("Report is empty");
    if (report.find("Report Generation Test") == std::string::npos) throw std::runtime_error("Title not in report");
}

