/**
 * @file error_messaging.cpp
 * @brief Implementation of comprehensive error messaging system
 */

#include "solar_core/error/error_messaging.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

// Platform-specific includes for command execution
#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// libcurl for HTTP error reporting
#ifdef CURL_VERSION_MAJOR
#include <curl/curl.h>
#endif

#include <cstdlib>  // For getenv

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

ErrorMessage ErrorMessageBuilder::build() const { return message_; }

// ErrorMessaging implementation

ErrorMessaging& ErrorMessaging::instance() {
  static ErrorMessaging instance;
  return instance;
}

void ErrorMessaging::register_error_template(const std::string& error_code,
                                             const ErrorMessage& template_msg) {
  error_templates_[error_code] = template_msg;
}

std::optional<ErrorMessage> ErrorMessaging::get_error_template(
    const std::string& error_code) const {
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
  oss << "║ Severity:   " << std::left << std::setw(48) << severity_to_string(error.severity)
      << " ║\n";
  oss << "║ Category:   " << std::left << std::setw(48) << category_to_string(error.category)
      << " ║\n";

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
  nlohmann::json j;

  j["error_code"] = error.error_code;
  j["title"] = error.title;
  j["description"] = error.description;
  j["severity"] = severity_to_string(error.severity);
  j["category"] = category_to_string(error.category);
  j["context"] = error.context;
  j["causes"] = error.causes;

  nlohmann::json recovery_actions = nlohmann::json::array();
  for (const auto& action : error.recovery_actions) {
    recovery_actions.push_back({{"description", action.description},
                                {"command", action.command},
                                {"automatic", action.automatic}});
  }
  j["recovery_actions"] = recovery_actions;

  j["related_docs"] = error.related_docs;
  j["technical_details"] = error.technical_details;
  j["user_reportable"] = error.user_reportable;

  return j.dump(2) + "\n";
}

void ErrorMessaging::report_error(const ErrorMessage& error) { error_history_.push_back(error); }

std::vector<ErrorMessage> ErrorMessaging::get_error_history() const { return error_history_; }

void ErrorMessaging::clear_error_history() { error_history_.clear(); }

std::vector<RecoveryAction> ErrorMessaging::suggest_recovery_actions(
    const std::string& error_code) const {
  auto error_template = get_error_template(error_code);
  if (error_template) {
    return error_template->recovery_actions;
  }
  return {};
}

bool ErrorMessaging::execute_recovery_action(const RecoveryAction& action) {
  if (!action.automatic || action.command.empty()) {
    return false;
  }

  // Security validation - only allow whitelisted commands
  static const std::vector<std::string> allowed_commands = {"restart", "clear_cache",
                                                            "reset_config", "reload", "cleanup"};

  // Extract command name (first word)
  std::string command_name = action.command.substr(0, action.command.find(' '));

  bool is_allowed = std::find(allowed_commands.begin(), allowed_commands.end(), command_name) !=
                    allowed_commands.end();

  if (!is_allowed) {
    // Log security violation
    return false;
  }

  try {
#ifdef _WIN32
    // Windows implementation using CreateProcess
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create mutable copy of command for CreateProcess
    std::string cmd_copy = action.command;

    // Start the child process with timeout
    if (!CreateProcessA(nullptr,           // No module name (use command line)
                        &cmd_copy[0],      // Command line (mutable)
                        nullptr,           // Process handle not inheritable
                        nullptr,           // Thread handle not inheritable
                        FALSE,             // Set handle inheritance to FALSE
                        CREATE_NO_WINDOW,  // No console window
                        nullptr,           // Use parent's environment block
                        nullptr,           // Use parent's starting directory
                        &si,               // Pointer to STARTUPINFO structure
                        &pi))              // Pointer to PROCESS_INFORMATION structure
    {
      return false;
    }

    // Wait for process to complete with timeout (30 seconds)
    DWORD wait_result = WaitForSingleObject(pi.hProcess, 30000);

    DWORD exit_code = 0;
    bool success = false;

    if (wait_result == WAIT_OBJECT_0) {
      // Process completed
      GetExitCodeProcess(pi.hProcess, &exit_code);
      success = (exit_code == 0);
    } else if (wait_result == WAIT_TIMEOUT) {
      // Timeout - terminate process
      TerminateProcess(pi.hProcess, 1);
      success = false;
    }

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return success;

#else
    // Unix/Linux/macOS implementation using fork/exec
    pid_t pid = fork();

    if (pid == -1) {
      // Fork failed
      return false;
    } else if (pid == 0) {
      // Child process

      // Parse command into arguments
      std::vector<std::string> args;
      std::istringstream iss(action.command);
      std::string arg;
      while (iss >> arg) {
        args.push_back(arg);
      }

      // Convert to char* array for execvp
      std::vector<char*> argv;
      for (auto& a : args) {
        argv.push_back(&a[0]);
      }
      argv.push_back(nullptr);

      // Execute command
      execvp(argv[0], argv.data());

      // If execvp returns, it failed
      _exit(1);
    } else {
      // Parent process - wait for child with timeout
      int status;
      int timeout_seconds = 30;

      // Use alarm for timeout
      alarm(static_cast<unsigned int>(timeout_seconds));

      pid_t result = waitpid(pid, &status, 0);

      alarm(0);  // Cancel alarm

      if (result == -1) {
        // Wait failed or timeout
        kill(pid, SIGKILL);        // Kill child process
        waitpid(pid, nullptr, 0);  // Clean up zombie
        return false;
      }

      // Check if process exited normally with success
      if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 0;
      }

      return false;
    }
#endif
  } catch (const std::exception&) {
    return false;
  }
}

void ErrorMessaging::set_language(const std::string& language) { current_language_ = language; }

std::string ErrorMessaging::get_language() const { return current_language_; }

size_t ErrorMessaging::get_error_count() const { return error_history_.size(); }

size_t ErrorMessaging::get_error_count_by_severity(ErrorSeverity severity) const {
  return static_cast<size_t>(
      std::count_if(error_history_.begin(), error_history_.end(),
                    [severity](const ErrorMessage& error) { return error.severity == severity; }));
}

size_t ErrorMessaging::get_error_count_by_category(ErrorCategory category) const {
  return static_cast<size_t>(
      std::count_if(error_history_.begin(), error_history_.end(),
                    [category](const ErrorMessage& error) { return error.category == category; }));
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

bool ErrorFeedback::send_error_report(const std::string& report) {
  // Check if error reporting is configured
  const char* report_url = std::getenv("SOLAR_ERROR_REPORT_URL");
  if (!report_url || std::string(report_url).empty()) {
    // No reporting URL configured, skip silently
    return true;
  }

#ifdef CURL_VERSION_MAJOR
  // Use libcurl for HTTP POST
  CURL* curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  bool success = false;

  try {
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, report_url);

    // Set POST data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, report.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(report.size()));

    // Set headers for JSON content
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: SolarSystemSuite/4.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set timeout (10 seconds)
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Disable SSL verification for internal servers (can be configured)
    const char* verify_ssl = std::getenv("SOLAR_ERROR_REPORT_VERIFY_SSL");
    if (verify_ssl && std::string(verify_ssl) == "false") {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      // Check HTTP response code
      long response_code = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      success = (response_code >= 200 && response_code < 300);
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

  } catch (...) {
    curl_easy_cleanup(curl);
    return false;
  }

  return success;
#else
  // libcurl not available, cannot send reports
  (void)report;  // Suppress unused parameter warning
  return false;
#endif
}

}  // namespace SolarSystem::Error
