/**
 * @file error_messaging.hpp
 * @brief Comprehensive error messaging system
 */

#ifndef SOLAR_CORE_ERROR_ERROR_MESSAGING_HPP
#define SOLAR_CORE_ERROR_ERROR_MESSAGING_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>

namespace SolarSystem::Error {

/**
 * @brief Error severity level
 */
enum class ErrorSeverity {
  INFO,
  WARNING,
  ERROR,
  CRITICAL,
  FATAL
};

/**
 * @brief Error category
 */
enum class ErrorCategory {
  CONFIGURATION,
  INPUT_VALIDATION,
  FILE_SYSTEM,
  NETWORK,
  COMPUTATION,
  MEMORY,
  PERMISSION,
  DEPENDENCY,
  INTERNAL
};

/**
 * @brief Error recovery action
 */
struct RecoveryAction {
  std::string description;
  std::string command;
  bool automatic;
};

/**
 * @brief Comprehensive error message
 */
struct ErrorMessage {
  std::string error_code;
  std::string title;
  std::string description;
  ErrorSeverity severity;
  ErrorCategory category;
  std::string context;
  std::vector<std::string> causes;
  std::vector<RecoveryAction> recovery_actions;
  std::vector<std::string> related_docs;
  std::string technical_details;
  bool user_reportable;
};

/**
 * @brief Error message builder
 */
class ErrorMessageBuilder {
 public:
  ErrorMessageBuilder& code(const std::string& error_code);
  ErrorMessageBuilder& title(const std::string& title);
  ErrorMessageBuilder& description(const std::string& desc);
  ErrorMessageBuilder& severity(ErrorSeverity sev);
  ErrorMessageBuilder& category(ErrorCategory cat);
  ErrorMessageBuilder& context(const std::string& ctx);
  ErrorMessageBuilder& add_cause(const std::string& cause);
  ErrorMessageBuilder& add_recovery_action(const RecoveryAction& action);
  ErrorMessageBuilder& add_related_doc(const std::string& doc);
  ErrorMessageBuilder& technical_details(const std::string& details);
  ErrorMessageBuilder& user_reportable(bool reportable);

  ErrorMessage build() const;

 private:
  ErrorMessage message_;
};

/**
 * @brief Error messaging system
 */
class ErrorMessaging {
 public:
  static ErrorMessaging& instance();

  // Error registration
  void register_error_template(const std::string& error_code, const ErrorMessage& template_msg);
  std::optional<ErrorMessage> get_error_template(const std::string& error_code) const;

  // Error formatting
  std::string format_error(const ErrorMessage& error) const;
  std::string format_error_simple(const ErrorMessage& error) const;
  std::string format_error_detailed(const ErrorMessage& error) const;
  std::string format_error_json(const ErrorMessage& error) const;

  // Error reporting
  void report_error(const ErrorMessage& error);
  std::vector<ErrorMessage> get_error_history() const;
  void clear_error_history();

  // Error recovery
  std::vector<RecoveryAction> suggest_recovery_actions(const std::string& error_code) const;
  bool execute_recovery_action(const RecoveryAction& action);

  // Error localization
  void set_language(const std::string& language);
  std::string get_language() const;

  // Error statistics
  size_t get_error_count() const;
  size_t get_error_count_by_severity(ErrorSeverity severity) const;
  size_t get_error_count_by_category(ErrorCategory category) const;

  // Utility methods
  std::string severity_to_string(ErrorSeverity severity) const;
  std::string category_to_string(ErrorCategory category) const;

 private:
  ErrorMessaging() = default;
  ErrorMessaging(const ErrorMessaging&) = delete;
  ErrorMessaging& operator=(const ErrorMessaging&) = delete;

  std::map<std::string, ErrorMessage> error_templates_;
  std::vector<ErrorMessage> error_history_;
  std::string current_language_{"en"};
};

/**
 * @brief Error feedback system
 */
class ErrorFeedback {
 public:
  static ErrorFeedback& instance();

  // Feedback submission
  void submit_feedback(const std::string& error_code, const std::string& feedback);
  std::vector<std::string> get_feedback(const std::string& error_code) const;

  // Error reporting
  std::string generate_error_report(const ErrorMessage& error) const;
  bool send_error_report(const std::string& report);

 private:
  ErrorFeedback() = default;
  ErrorFeedback(const ErrorFeedback&) = delete;
  ErrorFeedback& operator=(const ErrorFeedback&) = delete;

  std::map<std::string, std::vector<std::string>> feedback_data_;
};

}  // namespace SolarSystem::Error

#endif  // SOLAR_CORE_ERROR_ERROR_MESSAGING_HPP
