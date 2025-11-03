/**
 * @file error_messaging.cpp
 * @brief Implementation of comprehensive error messaging system
 */

#include "solar_core/error/error_messaging.hpp"

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <chrono>

namespace SolarSystem::Error {

// ErrorMessageBuilder implementation

ErrorMessageBuilder& ErrorMessageBuilder::code(const std::string& error_code) {
  message_.error_code = error_code;
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::title(const std::string& title) {
  message_.title = title;
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::description(const std::string& desc) {
  message_.description = desc;
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::severity(ErrorSeverity sev) {
  message_.severity = sev;
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::category(ErrorCategory cat) {
  message_.category = cat;
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::context(const std::string& ctx) {
  message_.context = ctx;
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::add_cause(const std::string& cause) {
  message_.causes.push_back(cause);
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::add_recovery_action(const RecoveryAction& action) {
  message_.recovery_actions.push_back(action);
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::add_related_doc(const std::string& doc) {
  message_.related_docs.push_back(doc);
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::technical_details(const std::string& details) {
  message_.technical_details = details;
  return *this;
}

ErrorMessageBuilder& ErrorMessageBuilder::user_reportable(bool reportable) {
  message_.user_reportable = reportable;
  return *this;
}

ErrorMessage ErrorMessageBuilder::build() const {
  return message_;
}

// ErrorMessaging implementation

ErrorMessaging& ErrorMessaging::instance() {
  static ErrorMessaging instance;
  return instance;
}

void ErrorMessaging::register_error_template(const std::string& error_code,
                                            const ErrorMessage& template_msg) {
  error_templates_[error_code] = template_msg;
}

std::optional<ErrorMessage> ErrorMessaging::get_error_template(const std::string& error_code) const {
  auto it = error_templates_.find(error_code);
  if (it != error_templates_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::string ErrorMessaging::severity_to_string(ErrorSeverity severity) const {
  switch (severity) {
    case ErrorSeverity::INFO:
      return "INFO";
    case ErrorSeverity::WARNING:
      return "WARNING";
    case ErrorSeverity::ERROR:
      return "ERROR";
    case ErrorSeverity::CRITICAL:
      return "CRITICAL";
    case ErrorSeverity::FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

std::string ErrorMessaging::category_to_string(ErrorCategory category) const {
  switch (category) {
    case ErrorCategory::CONFIGURATION:
      return "Configuration";
    case ErrorCategory::INPUT_VALIDATION:
      return "Input Validation";
    case ErrorCategory::FILE_SYSTEM:
      return "File System";
    case ErrorCategory::NETWORK:
      return "Network";
    case ErrorCategory::COMPUTATION:
      return "Computation";
    case ErrorCategory::MEMORY:
      return "Memory";
    case ErrorCategory::PERMISSION:
      return "Permission";
    case ErrorCategory::DEPENDENCY:
      return "Dependency";
    case ErrorCategory::INTERNAL:
      return "Internal";
    default:
      return "Unknown";
  }
}

std::string ErrorMessaging::format_error(const ErrorMessage& error) const {
  return format_error_detailed(error);
}

std::string ErrorMessaging::format_error_simple(const ErrorMessage& error) const {
  std::ostringstream oss;

  oss << "[" << severity_to_string(error.severity) << "] ";
  oss << error.title;

  if (!error.description.empty()) {
    oss << ": " << error.description;
  }

  return oss.str();
}

std::string ErrorMessaging::format_error_detailed(const ErrorMessage& error) const {
  std::ostringstream oss;

  // Header
  oss << "╔═══════════════════════════════════════════════════════════════╗\n";
  oss << "║ " << std::left << std::setw(61) << error.title << " ║\n";
  oss << "╠═══════════════════════════════════════════════════════════════╣\n";

  // Error code and severity
  oss << "║ Error Code: " << std::left << std::setw(48) << error.error_code << " ║\n";
  oss << "║ Severity:   " << std::left << std::setw(48) << severity_to_string(error.severity) << " ║\n";
  oss << "║ Category:   " << std::left << std::setw(48) << category_to_string(error.category) << " ║\n";

  if (!error.context.empty()) {
    oss << "║ Context:    " << std::left << std::setw(48) << error.context << " ║\n";
  }

  oss << "╠═══════════════════════════════════════════════════════════════╣\n";

  // Description
  if (!error.description.empty()) {
    oss << "║ Description:                                                  ║\n";
    oss << "║ " << std::left << std::setw(61) << error.description << " ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════╣\n";
  }

  // Possible causes
  if (!error.causes.empty()) {
    oss << "║ Possible Causes:                                              ║\n";
    for (const auto& cause : error.causes) {
      oss << "║  • " << std::left << std::setw(59) << cause << " ║\n";
    }
    oss << "╠═══════════════════════════════════════════════════════════════╣\n";
  }

  // Recovery actions
  if (!error.recovery_actions.empty()) {
    oss << "║ Suggested Actions:                                            ║\n";
    for (size_t i = 0; i < error.recovery_actions.size(); ++i) {
      const auto& action = error.recovery_actions[i];
      oss << "║  " << (i + 1) << ". " << std::left << std::setw(57) << action.description << " ║\n";
      if (!action.command.empty()) {
        oss << "║     Command: " << std::left << std::setw(47) << action.command << " ║\n";
      }
    }
    oss << "╠═══════════════════════════════════════════════════════════════╣\n";
  }

  // Related documentation
  if (!error.related_docs.empty()) {
    oss << "║ Related Documentation:                                        ║\n";
    for (const auto& doc : error.related_docs) {
      oss << "║  • " << std::left << std::setw(59) << doc << " ║\n";
    }
    oss << "╠═══════════════════════════════════════════════════════════════╣\n";
  }

  // Technical details
  if (!error.technical_details.empty()) {
    oss << "║ Technical Details:                                            ║\n";
    oss << "║ " << std::left << std::setw(61) << error.technical_details << " ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════╣\n";
  }

  // Footer
  if (error.user_reportable) {
    oss << "║ This error can be reported for further assistance.           ║\n";
  }

  oss << "╚═══════════════════════════════════════════════════════════════╝\n";

  return oss.str();
}

std::string ErrorMessaging::format_error_json(const ErrorMessage& error) const {
  std::ostringstream oss;

  oss << "{\n";
  oss << "  \"error_code\": \"" << error.error_code << "\",\n";
  oss << "  \"title\": \"" << error.title << "\",\n";
  oss << "  \"description\": \"" << error.description << "\",\n";
  oss << "  \"severity\": \"" << severity_to_string(error.severity) << "\",\n";
  oss << "  \"category\": \"" << category_to_string(error.category) << "\",\n";
  oss << "  \"context\": \"" << error.context << "\",\n";

  oss << "  \"causes\": [";
  for (size_t i = 0; i < error.causes.size(); ++i) {
    oss << "\"" << error.causes[i] << "\"";
    if (i < error.causes.size() - 1) oss << ", ";
  }
  oss << "],\n";

  oss << "  \"recovery_actions\": [";
  for (size_t i = 0; i < error.recovery_actions.size(); ++i) {
    const auto& action = error.recovery_actions[i];
    oss << "{\"description\": \"" << action.description << "\", ";
    oss << "\"command\": \"" << action.command << "\", ";
    oss << "\"automatic\": " << (action.automatic ? "true" : "false") << "}";
    if (i < error.recovery_actions.size() - 1) oss << ", ";
  }
  oss << "],\n";

  oss << "  \"related_docs\": [";
  for (size_t i = 0; i < error.related_docs.size(); ++i) {
    oss << "\"" << error.related_docs[i] << "\"";
    if (i < error.related_docs.size() - 1) oss << ", ";
  }
  oss << "],\n";

  oss << "  \"technical_details\": \"" << error.technical_details << "\",\n";
  oss << "  \"user_reportable\": " << (error.user_reportable ? "true" : "false") << "\n";
  oss << "}\n";

  return oss.str();
}

void ErrorMessaging::report_error(const ErrorMessage& error) {
  error_history_.push_back(error);
}

std::vector<ErrorMessage> ErrorMessaging::get_error_history() const {
  return error_history_;
}

void ErrorMessaging::clear_error_history() {
  error_history_.clear();
}

std::vector<RecoveryAction> ErrorMessaging::suggest_recovery_actions(const std::string& error_code) const {
  auto error_template = get_error_template(error_code);
  if (error_template) {
    return error_template->recovery_actions;
  }
  return {};
}

bool ErrorMessaging::execute_recovery_action(const RecoveryAction& action) {
  if (action.automatic && !action.command.empty()) {
    // In a real implementation, this would execute the command
    // For now, just return true to indicate it would be executed
    return true;
  }
  return false;
}

void ErrorMessaging::set_language(const std::string& language) {
  current_language_ = language;
}

std::string ErrorMessaging::get_language() const {
  return current_language_;
}

size_t ErrorMessaging::get_error_count() const {
  return error_history_.size();
}

size_t ErrorMessaging::get_error_count_by_severity(ErrorSeverity severity) const {
  return static_cast<size_t>(std::count_if(error_history_.begin(), error_history_.end(),
                        [severity](const ErrorMessage& error) {
                          return error.severity == severity;
                        }));
}

size_t ErrorMessaging::get_error_count_by_category(ErrorCategory category) const {
  return static_cast<size_t>(std::count_if(error_history_.begin(), error_history_.end(),
                        [category](const ErrorMessage& error) {
                          return error.category == category;
                        }));
}

// ErrorFeedback implementation

ErrorFeedback& ErrorFeedback::instance() {
  static ErrorFeedback instance;
  return instance;
}

void ErrorFeedback::submit_feedback(const std::string& error_code, const std::string& feedback) {
  feedback_data_[error_code].push_back(feedback);
}

std::vector<std::string> ErrorFeedback::get_feedback(const std::string& error_code) const {
  auto it = feedback_data_.find(error_code);
  if (it != feedback_data_.end()) {
    return it->second;
  }
  return {};
}

std::string ErrorFeedback::generate_error_report(const ErrorMessage& error) const {
  std::ostringstream oss;

  oss << "=== Error Report ===\n\n";
  oss << "Error Code: " << error.error_code << "\n";
  oss << "Title: " << error.title << "\n";
  auto& messaging = ErrorMessaging::instance();
  oss << "Severity: " << messaging.severity_to_string(error.severity) << "\n";
  oss << "Category: " << messaging.category_to_string(error.category) << "\n\n";

  oss << "Description:\n" << error.description << "\n\n";

  if (!error.context.empty()) {
    oss << "Context:\n" << error.context << "\n\n";
  }

  if (!error.technical_details.empty()) {
    oss << "Technical Details:\n" << error.technical_details << "\n\n";
  }

  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  oss << "Timestamp: " << std::ctime(&time_t);

  return oss.str();
}

bool ErrorFeedback::send_error_report(const std::string& /* report */) {
  // In a real implementation, this would send the report to a server
  // For now, just return true to indicate success
  return true;
}

}  // namespace SolarSystem::Error

