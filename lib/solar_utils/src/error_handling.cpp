/**
 * @file error_handling.cpp
 * @brief Implementation of comprehensive error handling system
 */

#include "solar_utils/error_handling.hpp"
#include "solar_utils/network_resource_manager.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

// Platform-specific includes for syslog
#ifdef _WIN32
#include <windows.h>
#else
#include <syslog.h>
#endif

namespace SolarSystem::Utils {

// DetailedError implementation
ErrorCategory DetailedError::get_category_for_code(ErrorCode code) {
  int code_value = static_cast<int>(code);

  if (code_value >= 1000 && code_value < 2000) return ErrorCategory::Validation;
  if (code_value >= 2000 && code_value < 3000) return ErrorCategory::Network;
  if (code_value >= 3000 && code_value < 4000) return ErrorCategory::FileSystem;
  if (code_value >= 4000 && code_value < 5000) return ErrorCategory::Memory;
  if (code_value >= 5000 && code_value < 6000) return ErrorCategory::Configuration;
  if (code_value >= 6000 && code_value < 7000) return ErrorCategory::Runtime;
  if (code_value >= 7000 && code_value < 8000) return ErrorCategory::Resource;
  if (code_value >= 8000 && code_value < 9000) return ErrorCategory::Security;
  if (code_value >= 9000 && code_value < 10000) return ErrorCategory::Performance;

  return ErrorCategory::Unknown;
}

std::string DetailedError::to_string() const {
  std::ostringstream oss;
  oss << "[" << ErrorUtils::error_severity_to_string(severity) << "] "
      << ErrorUtils::error_category_to_string(category) << " Error " << static_cast<int>(code)
      << ": " << message;

  if (!context.empty()) {
    oss << " (Context: " << context << ")";
  }

  if (!source_location.empty()) {
    oss << " at " << source_location;
  }

  if (!suggestions.empty()) {
    oss << "\nSuggestions:";
    for (const auto& suggestion : suggestions) {
      oss << "\n  - " << suggestion;
    }
  }

  if (recovery_action.has_value()) {
    oss << "\nRecovery: " << recovery_action.value();
  }

  return oss.str();
}

bool DetailedError::is_recoverable() const {
  // Fatal errors are not recoverable
  if (severity == ErrorSeverity::Fatal) {
    return false;
  }

  // Some specific error codes are not recoverable
  switch (code) {
    case ErrorCode::OutOfMemory:
    case ErrorCode::SecurityViolation:
    case ErrorCode::InvalidCredentials:
      return false;
    default:
      return true;
  }
}

RecoveryStrategy DetailedError::get_recovery_strategy() const {
  if (!is_recoverable()) {
    return RecoveryStrategy::FailFast;
  }

  switch (category) {
    case ErrorCategory::Network:
      return RecoveryStrategy::RetryWithExponentialBackoff;
    case ErrorCategory::FileSystem:
      return RecoveryStrategy::Retry;
    case ErrorCategory::Validation:
      return RecoveryStrategy::UserInterventionRequired;
    case ErrorCategory::Resource:
      return RecoveryStrategy::GracefulDegradation;
    case ErrorCategory::Configuration:
      return RecoveryStrategy::Fallback;
    default:
      return RecoveryStrategy::Retry;
  }
}

// ValidationResult implementation
void ValidationResult::add_error(const DetailedError& error) {
  errors.push_back(error);
  is_valid = false;
}

void ValidationResult::add_error(ErrorCode code, const std::string& message, ErrorSeverity severity,
                                 const std::string& context) {
  DetailedError error(code, message, severity, context);
  add_error(error);
}

void ValidationResult::add_warning(const DetailedError& warning) { warnings.push_back(warning); }

void ValidationResult::add_warning(ErrorCode code, const std::string& message,
                                   const std::string& context) {
  DetailedError warning(code, message, ErrorSeverity::Warning, context);
  add_warning(warning);
}

bool ValidationResult::has_errors_of_severity(ErrorSeverity severity) const {
  return std::any_of(errors.begin(), errors.end(),
                     [severity](const DetailedError& error) { return error.severity == severity; });
}

std::vector<DetailedError> ValidationResult::get_errors_by_category(ErrorCategory category) const {
  std::vector<DetailedError> filtered_errors;
  std::copy_if(errors.begin(), errors.end(), std::back_inserter(filtered_errors),
               [category](const DetailedError& error) { return error.category == category; });
  return filtered_errors;
}

void ValidationResult::generate_summary() {
  if (is_valid) {
    summary = "Validation passed successfully";
    if (!warnings.empty()) {
      summary.value() += " with " + std::to_string(warnings.size()) + " warnings";
    }
  } else {
    std::ostringstream oss;
    oss << "Validation failed with " << errors.size() << " errors";
    if (!warnings.empty()) {
      oss << " and " << warnings.size() << " warnings";
    }

    // Count errors by severity
    std::unordered_map<ErrorSeverity, size_t> severity_counts;
    for (const auto& error : errors) {
      severity_counts[error.severity]++;
    }

    if (!severity_counts.empty()) {
      oss << " (";
      bool first = true;
      for (const auto& [severity, count] : severity_counts) {
        if (!first) oss << ", ";
        oss << count << " " << ErrorUtils::error_severity_to_string(severity);
        first = false;
      }
      oss << ")";
    }

    summary = oss.str();
  }
}

std::string ValidationResult::to_string() const {
  std::ostringstream oss;

  if (summary.has_value()) {
    oss << summary.value() << "\n";
  }

  if (!errors.empty()) {
    oss << "\nErrors:\n";
    for (size_t i = 0; i < errors.size(); ++i) {
      oss << "  " << (i + 1) << ". " << errors[i].to_string() << "\n";
    }
  }

  if (!warnings.empty()) {
    oss << "\nWarnings:\n";
    for (size_t i = 0; i < warnings.size(); ++i) {
      oss << "  " << (i + 1) << ". " << warnings[i].to_string() << "\n";
    }
  }

  return oss.str();
}

// ErrorStatistics implementation
void ErrorStatistics::record_error(const DetailedError& error) {
  total_errors++;
  errors_by_severity[static_cast<int>(error.severity)]++;
  errors_by_category[error.category]++;
  errors_by_code[error.code]++;

  if (total_errors == 1) {
    first_error_time = error.timestamp;
  }
  last_error_time = error.timestamp;
}

void ErrorStatistics::record_recovery_attempt(bool successful, std::chrono::milliseconds duration) {
  total_recovery_time += duration;
  if (successful) {
    successful_recoveries++;
  } else {
    failed_recoveries++;
  }
}

double ErrorStatistics::get_error_rate() const {
  if (total_errors == 0) return 0.0;

  auto duration =
      std::chrono::duration_cast<std::chrono::minutes>(last_error_time - first_error_time);

  if (duration.count() == 0) return 0.0;

  return static_cast<double>(total_errors) / duration.count();
}

double ErrorStatistics::get_recovery_success_rate() const {
  size_t total_recoveries = successful_recoveries + failed_recoveries;
  if (total_recoveries == 0) return 0.0;

  return static_cast<double>(successful_recoveries) / total_recoveries;
}

std::string ErrorStatistics::generate_report() const {
  std::ostringstream oss;

  oss << "=== Error Statistics Report ===\n";
  oss << "Total errors: " << total_errors << "\n";
  oss << "Error rate: " << std::fixed << std::setprecision(2) << get_error_rate()
      << " errors/minute\n";

  oss << "\nErrors by severity:\n";
  const char* severity_names[] = {"Info", "Warning", "Error", "Critical", "Fatal"};
  for (int i = 0; i < 5; ++i) {
    if (errors_by_severity[i] > 0) {
      oss << "  " << severity_names[i] << ": " << errors_by_severity[i] << "\n";
    }
  }

  oss << "\nErrors by category:\n";
  for (const auto& [category, count] : errors_by_category) {
    oss << "  " << ErrorUtils::error_category_to_string(category) << ": " << count << "\n";
  }

  oss << "\nRecovery statistics:\n";
  oss << "  Successful recoveries: " << successful_recoveries << "\n";
  oss << "  Failed recoveries: " << failed_recoveries << "\n";
  oss << "  Recovery success rate: " << std::fixed << std::setprecision(1)
      << (get_recovery_success_rate() * 100) << "%\n";
  oss << "  Total recovery time: " << total_recovery_time.count() << "ms\n";

  return oss.str();
}

// ErrorPattern implementation
bool ErrorPattern::matches(const std::vector<DetailedError>& errors) const {
  if (errors.size() < error_sequence.size()) {
    return false;
  }

  // Check if the last N errors match our sequence
  size_t start_index = errors.size() - error_sequence.size();
  for (size_t i = 0; i < error_sequence.size(); ++i) {
    if (errors[start_index + i].code != error_sequence[i]) {
      return false;
    }
  }

  return true;
}

void ErrorPattern::update_occurrence() {
  occurrence_count++;
  last_seen = std::chrono::system_clock::now();

  if (occurrence_count == 1) {
    first_seen = last_seen;
  }

  // Update confidence score bon frequency and recency
  auto age =
      std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now() - first_seen);

  if (age.count() > 0) {
    confidence_score = static_cast<double>(occurrence_count) / age.count();
  } else {
    confidence_score = static_cast<double>(occurrence_count);
  }
}

// ErrorRecoveryManager implementation
RecoveryAction ErrorRecoveryManager::determine_recovery_strategy(const DetailedError& error) const {
  // Check for custom strategy first
  auto it = custom_strategies_.find(error.code);
  if (it != custom_strategies_.end()) {
    return it->second(error);
  }

  // Use default strategy
  return get_default_recovery_strategy(error);
}

bool ErrorRecoveryManager::attempt_recovery(const DetailedError&,
                                            const RecoveryAction& action) {
  auto start_time = std::chrono::steady_clock::now();
  bool success = false;

  try {
    if (action.action) {
      // Implement retry logic based on strategy
      for (size_t attempt = 0; attempt < action.max_attempts; ++attempt) {
        if (attempt > 0 && action.delay.count() > 0) {
          std::this_thread::sleep_for(action.delay);
        }

        success = action.action();
        if (success) {
          break;
        }

        // For exponential backoff, increase delay
        if (action.strategy == RecoveryStrategy::RetryWithExponentialBackoff) {
          const_cast<RecoveryAction&>(action).delay *= 2;
        }
      }
    }
  } catch (const std::exception&) {
    success = false;
  }

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Update statistics
  std::lock_guard<std::mutex> lock(statistics_mutex_);
  statistics_.record_recovery_attempt(success, duration);

  return success;
}

void ErrorRecoveryManager::register_recovery_strategy(
    ErrorCode error_code, std::function<RecoveryAction(const DetailedError&)> strategy_provider) {
  custom_strategies_[error_code] = strategy_provider;
}

void ErrorRecoveryManager::register_recovery_action(
    ErrorCode error_code, RecoveryStrategy strategy,
    std::function<bool(const DetailedError&)> action) {
  std::string key = std::to_string(static_cast<int>(error_code)) + "_" +
                    std::to_string(static_cast<int>(strategy));
  custom_actions_[key] = action;
}

ErrorStatistics ErrorRecoveryManager::get_recovery_statistics() const {
  std::lock_guard<std::mutex> lock(statistics_mutex_);
  return statistics_;
}

void ErrorRecoveryManager::clear_statistics() {
  std::lock_guard<std::mutex> lock(statistics_mutex_);
  statistics_ = ErrorStatistics{};
}

RecoveryAction ErrorRecoveryManager::get_default_recovery_strategy(
    const DetailedError& error) const {
  RecoveryStrategy strategy = error.get_recovery_strategy();

  RecoveryAction action(strategy,
                        "Default recovery for " + ErrorUtils::error_code_to_string(error.code));

  switch (strategy) {
    case RecoveryStrategy::Retry:
      action.max_attempts = 3;
      action.delay = std::chrono::milliseconds(100);
      break;
    case RecoveryStrategy::RetryWithDelay:
      action.max_attempts = 3;
      action.delay = std::chrono::milliseconds(1000);
      break;
    case RecoveryStrategy::RetryWithExponentialBackoff:
      action.max_attempts = 5;
      action.delay = std::chrono::milliseconds(100);
      break;
    default:
      action.max_attempts = 1;
      break;
  }

  return action;
}

// ErrorLogger implementation
void ErrorLogger::configure(LogLevel min_level, const std::vector<LogTarget>& targets) {
  std::lock_guard<std::mutex> lock(log_mutex_);
  min_level_ = min_level;
  targets_ = targets;
}

void ErrorLogger::set_log_file(const std::string& file_path) {
  std::lock_guard<std::mutex> lock(log_mutex_);
  log_file_path_ = file_path;
}

void ErrorLogger::set_network_endpoint(const std::string& endpoint) {
  std::lock_guard<std::mutex> lock(log_mutex_);
  network_endpoint_ = endpoint;
}

void ErrorLogger::log_error(const DetailedError& error) {
  LogLevel level;
  switch (error.severity) {
    case ErrorSeverity::Info:
      level = LogLevel::Info;
      break;
    case ErrorSeverity::Warning:
      level = LogLevel::Warning;
      break;
    case ErrorSeverity::Error:
      level = LogLevel::Error;
      break;
    case ErrorSeverity::Critical:
    case ErrorSeverity::Fatal:
      level = LogLevel::Critical;
      break;
  }

  if (level < min_level_) {
    return;
  }

  std::string message = format_log_message(error);

  std::lock_guard<std::mutex> lock(log_mutex_);
  log_statistics_.record_error(error);

  for (LogTarget target : targets_) {
    switch (target) {
      case LogTarget::Console:
        log_to_console(message, level);
        break;
      case LogTarget::File:
        log_to_file(message);
        break;
      case LogTarget::Syslog:
        log_to_syslog(message, level);
        break;
      case LogTarget::Network:
        log_to_network(message);
        break;
      case LogTarget::Memory:
        log_to_memory(message);
        break;
    }
  }
}

void ErrorLogger::log_validation_result(const ValidationResult& result) {
  if (result.is_valid && result.warnings.empty()) {
    return;  // Nothing to log
  }

  std::string message = format_log_message(result);
  LogLevel level = result.is_valid ? LogLevel::Warning : LogLevel::Error;

  if (level < min_level_) {
    return;
  }

  std::lock_guard<std::mutex> lock(log_mutex_);

  for (LogTarget target : targets_) {
    switch (target) {
      case LogTarget::Console:
        log_to_console(message, level);
        break;
      case LogTarget::File:
        log_to_file(message);
        break;
      case LogTarget::Syslog:
        log_to_syslog(message, level);
        break;
      case LogTarget::Network:
        log_to_network(message);
        break;
      case LogTarget::Memory:
        log_to_memory(message);
        break;
    }
  }
}

void ErrorLogger::log_recovery_attempt(const DetailedError& error, const RecoveryAction& action,
                                       bool success) {
  std::ostringstream oss;
  oss << "Recovery attempt for error " << static_cast<int>(error.code) << " using strategy "
      << ErrorUtils::recovery_strategy_to_string(action.strategy) << ": "
      << (success ? "SUCCESS" : "FAILED");

  LogLevel level = success ? LogLevel::Info : LogLevel::Warning;

  if (level < min_level_) {
    return;
  }

  std::lock_guard<std::mutex> lock(log_mutex_);

  for (LogTarget target : targets_) {
    switch (target) {
      case LogTarget::Console:
        log_to_console(oss.str(), level);
        break;
      case LogTarget::File:
        log_to_file(oss.str());
        break;
      case LogTarget::Syslog:
        log_to_syslog(oss.str(), level);
        break;
      case LogTarget::Network:
        log_to_network(oss.str());
        break;
      case LogTarget::Memory:
        log_to_memory(oss.str());
        break;
    }
  }
}

ErrorStatistics ErrorLogger::get_log_statistics() const {
  std::lock_guard<std::mutex> lock(log_mutex_);
  return log_statistics_;
}

void ErrorLogger::flush() {
  std::lock_guard<std::mutex> lock(log_mutex_);

  // Flush file stream if logging to file
  if (!log_file_path_.empty()) {
    std::ofstream file(log_file_path_, std::ios::app);
    if (file.is_open()) {
      file.flush();
    }
  }

  // Flush console streams
  std::cout.flush();
  std::cerr.flush();

  // Flush syslog (on Unix systems, syslog is typically unbuffered)
#ifndef _WIN32
  // No explicit flush needed for syslog
#endif
}

void ErrorLogger::log_to_console(const std::string& message, LogLevel level) {
  std::ostream& stream = (level >= LogLevel::Error) ? std::cerr : std::cout;

  // Add timestamp
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);

  stream << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
         << "[" << log_level_to_string(level) << "] " << message << std::endl;
}

void ErrorLogger::log_to_file(const std::string& message) {
  if (log_file_path_.empty()) {
    return;
  }

  std::ofstream file(log_file_path_, std::ios::app);
  if (file.is_open()) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    file << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] " << message
         << std::endl;
  }
}

void ErrorLogger::log_to_syslog(const std::string& message, LogLevel level) {
#ifdef _WIN32
  // Windows Event Log implementation
  HANDLE hEventLog = RegisterEventSourceA(nullptr, "SolarSystemSuite");
  if (hEventLog) {
    WORD event_type;
    switch (level) {
      case LogLevel::Debug:
      case LogLevel::Info:
        event_type = EVENTLOG_INFORMATION_TYPE;
        break;
      case LogLevel::Warning:
        event_type = EVENTLOG_WARNING_TYPE;
        break;
      case LogLevel::Error:
      case LogLevel::Critical:
        event_type = EVENTLOG_ERROR_TYPE;
        break;
      default:
        event_type = EVENTLOG_INFORMATION_TYPE;
    }

    const char* strings[1] = {message.c_str()};
    ReportEventA(hEventLog, event_type, 0, 0, nullptr, 1, 0, strings, nullptr);
    DeregisterEventSource(hEventLog);
  } else {
    // Fallback to console if Event Log unavailable
    log_to_console("SYSLOG: " + message, level);
  }
#else
  // Unix/Linux/macOS syslog implementation
  static bool syslog_opened = false;
  if (!syslog_opened) {
    openlog("SolarSystemSuite", LOG_PID | LOG_CONS, LOG_USER);
    syslog_opened = true;
  }

  // Map LogLevel to syslog priority
  int priority;
  switch (level) {
    case LogLevel::Debug:
      priority = LOG_DEBUG;
      break;
    case LogLevel::Info:
      priority = LOG_INFO;
      break;
    case LogLevel::Warning:
      priority = LOG_WARNING;
      break;
    case LogLevel::Error:
      priority = LOG_ERR;
      break;
    case LogLevel::Critical:
      priority = LOG_CRIT;
      break;
    default:
      priority = LOG_INFO;
  }

  // Write to syslog
  syslog(priority, "%s", message.c_str());
#endif
}

void ErrorLogger::log_to_network(const std::string& message) {
  if (network_endpoint_.empty()) {
    // No network endpoint configured, skip network logging
    return;
  }

  try {
    // Use the network resource manager we implemented in Task 1
    auto& network_manager = NetworkResourceManager::instance();
    auto connection = network_manager.get_pooled_connection(network_endpoint_, ConnectionType::HTTP);

    if (!connection) {
      // Failed to get connection, fallback to console
      log_to_console("NETWORK_ERROR: Failed to connect to " + network_endpoint_, LogLevel::Warning);
      return;
    }

    // Create JSON payload for the log message
    std::ostringstream json_payload;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    json_payload << "{"
                 << "\"timestamp\":\"" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\","
                 << "\"application\":\"SolarSystemSuite\","
                 << "\"message\":\"" << message << "\""
                 << "}";

    // Build HTTP POST request
    std::ostringstream request;
    request << "POST /api/logs HTTP/1.1\r\n"
            << "Host: " << network_endpoint_ << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << json_payload.str().size() << "\r\n"
            << "User-Agent: SolarSystemSuite/4.0.0\r\n"
            << "\r\n"
            << json_payload.str();

    // Send the request (fire and forget, don't wait for response)
    connection->send_request(request.str(), std::chrono::seconds(5));

  } catch (const std::exception& ex) {
    // Network logging failed, log to console as fallback
    log_to_console("NETWORK_ERROR: " + std::string(ex.what()), LogLevel::Warning);
  }
}

namespace {
// Shared memory log buffer state
struct MemoryLogBuffer {
  static const size_t MAX_SIZE = 10000;
  std::vector<std::string> buffer;
  size_t current_index = 0;
  bool is_full = false;
  std::mutex mutex;
};

MemoryLogBuffer& get_memory_log_buffer() {
  static MemoryLogBuffer instance;
  return instance;
}
}  // anonymous namespace

void ErrorLogger::log_to_memory(const std::string& message) {
  auto& mem_log = get_memory_log_buffer();
  std::lock_guard<std::mutex> lock(mem_log.mutex);

  // Add timestamp to message
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] " << message;
  std::string timestamped_message = oss.str();

  // Circular buffer implementation
  if (!mem_log.is_full && mem_log.buffer.size() < MemoryLogBuffer::MAX_SIZE) {
    // Buffer not yet full, just append
    mem_log.buffer.push_back(timestamped_message);
    mem_log.current_index = mem_log.buffer.size();
  } else {
    // Buffer is full, overwrite oldest entry
    if (mem_log.buffer.size() < MemoryLogBuffer::MAX_SIZE) {
      mem_log.buffer.resize(MemoryLogBuffer::MAX_SIZE);
    }
    mem_log.is_full = true;
    mem_log.buffer[mem_log.current_index % MemoryLogBuffer::MAX_SIZE] = timestamped_message;
    mem_log.current_index++;
  }
}

std::vector<std::string> ErrorLogger::get_memory_logs(size_t max_entries) {
  auto& mem_log = get_memory_log_buffer();
  std::lock_guard<std::mutex> lock(mem_log.mutex);

  std::vector<std::string> result;

  if (mem_log.buffer.empty()) {
    return result;
  }

  size_t entries_to_return = std::min(max_entries, mem_log.buffer.size());

  if (!mem_log.is_full) {
    // Buffer not full yet, return from beginning
    size_t start = mem_log.buffer.size() > entries_to_return
                   ? mem_log.buffer.size() - entries_to_return
                   : 0;
    result.assign(mem_log.buffer.begin() + static_cast<std::ptrdiff_t>(start),
                  mem_log.buffer.end());
  } else {
    // Buffer is full, return most recent entries in chronological order
    size_t start_idx = mem_log.current_index >= entries_to_return
                       ? (mem_log.current_index - entries_to_return) % mem_log.buffer.size()
                       : 0;

    for (size_t i = 0; i < entries_to_return; ++i) {
      size_t idx = (start_idx + i) % mem_log.buffer.size();
      if (!mem_log.buffer[idx].empty()) {
        result.push_back(mem_log.buffer[idx]);
      }
    }
  }

  return result;
}

void ErrorLogger::clear_memory_logs() {
  auto& mem_log = get_memory_log_buffer();
  std::lock_guard<std::mutex> lock(mem_log.mutex);

  mem_log.buffer.clear();
  mem_log.current_index = 0;
  mem_log.is_full = false;
}

std::string ErrorLogger::format_log_message(const DetailedError& error) const {
  std::ostringstream oss;
  oss << "ERROR " << static_cast<int>(error.code) << " ["
      << ErrorUtils::error_category_to_string(error.category) << "]: " << error.message;

  if (!error.context.empty()) {
    oss << " | Context: " << error.context;
  }

  if (!error.source_location.empty()) {
    oss << " | Location: " << error.source_location;
  }

  return oss.str();
}

std::string ErrorLogger::format_log_message(const ValidationResult& result) const {
  std::ostringstream oss;

  if (result.summary.has_value()) {
    oss << "VALIDATION: " << result.summary.value();
  } else {
    oss << "VALIDATION: " << (result.is_valid ? "PASSED" : "FAILED") << " (" << result.errors.size()
        << " errors, " << result.warnings.size() << " warnings)";
  }

  return oss.str();
}

std::string ErrorLogger::log_level_to_string(LogLevel level) const {
  switch (level) {
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warning:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Critical:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

// ErrorPatternAnalyzer implementation
std::vector<ErrorPattern> ErrorPatternAnalyzer::analyze_patterns(
    const std::vector<DetailedError>& errors) {
  std::lock_guard<std::mutex> lock(patterns_mutex_);

  // Add errors to history
  for (const auto& error : errors) {
    error_history_.push_back(error);
  }

  // Maintain history size limit
  if (error_history_.size() > max_history_size_) {
    error_history_.erase(error_history_.begin(),
                         error_history_.begin() + static_cast<std::ptrdiff_t>(error_history_.size() - max_history_size_));
  }

  // Extract new patterns
  return extract_patterns(error_history_);
}

void ErrorPatternAnalyzer::register_pattern(const ErrorPattern& pattern) {
  std::lock_guard<std::mutex> lock(patterns_mutex_);
  known_patterns_.push_back(pattern);
}

std::vector<std::pair<ErrorCode, double>> ErrorPatternAnalyzer::predict_next_errors(
    const std::vector<DetailedError>& recent_errors) const {
  std::lock_guard<std::mutex> lock(patterns_mutex_);

  std::unordered_map<ErrorCode, double> predictions;

  // Find patterns that match recent errors
  for (const auto& pattern : known_patterns_) {
    if (pattern.matches(recent_errors)) {
      // Predict the next error in the sequence if pattern continues
      if (pattern.error_sequence.size() > recent_errors.size()) {
        ErrorCode next_error = pattern.error_sequence[recent_errors.size()];
        predictions[next_error] += pattern.confidence_score;
      }
    }
  }

  // Convert to vector and sort by confidence
  std::vector<std::pair<ErrorCode, double>> result;
  for (const auto& [code, confidence] : predictions) {
    result.emplace_back(code, confidence);
  }

  std::sort(result.begin(), result.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  return result;
}

std::vector<ErrorPattern> ErrorPatternAnalyzer::get_frequent_patterns(
    size_t min_occurrences) const {
  std::lock_guard<std::mutex> lock(patterns_mutex_);

  std::vector<ErrorPattern> frequent_patterns;
  std::copy_if(known_patterns_.begin(), known_patterns_.end(),
               std::back_inserter(frequent_patterns),
               [min_occurrences](const ErrorPattern& pattern) {
                 return pattern.occurrence_count >= min_occurrences;
               });

  // Sort by occurrence count
  std::sort(frequent_patterns.begin(), frequent_patterns.end(),
            [](const ErrorPattern& a, const ErrorPattern& b) {
              return a.occurrence_count > b.occurrence_count;
            });

  return frequent_patterns;
}

void ErrorPatternAnalyzer::update_patterns(const DetailedError& error) {
  std::lock_guard<std::mutex> lock(patterns_mutex_);

  error_history_.push_back(error);

  // Maintain history size
  if (error_history_.size() > max_history_size_) {
    error_history_.erase(error_history_.begin());
  }

  // Update existing patterns
  std::vector<DetailedError> recent_errors;
  if (error_history_.size() >= 2) {
    recent_errors.assign(error_history_.end() - 2, error_history_.end());
  }

  for (auto& pattern : known_patterns_) {
    if (pattern.matches(recent_errors)) {
      pattern.update_occurrence();
    }
  }
}

void ErrorPatternAnalyzer::clear_patterns() {
  std::lock_guard<std::mutex> lock(patterns_mutex_);
  known_patterns_.clear();
  error_history_.clear();
}

std::vector<size_t> ErrorPatternAnalyzer::find_matching_patterns(
    const std::vector<DetailedError>& errors) const {
  std::vector<size_t> matches;

  for (size_t i = 0; i < known_patterns_.size(); ++i) {
    if (known_patterns_[i].matches(errors)) {
      matches.push_back(i);
    }
  }

  return matches;
}

std::vector<ErrorPattern> ErrorPatternAnalyzer::extract_patterns(
    const std::vector<DetailedError>& errors) const {
  std::vector<ErrorPattern> patterns;

  // Extract patterns of length 2-5
  for (size_t pattern_length = 2; pattern_length <= 5 && pattern_length <= errors.size();
       ++pattern_length) {
    for (size_t start = 0; start <= errors.size() - pattern_length; ++start) {
      ErrorPattern pattern;
      pattern.pattern_id = "auto_" + std::to_string(patterns.size());

      for (size_t i = start; i < start + pattern_length; ++i) {
        pattern.error_sequence.push_back(errors[i].code);
      }

      pattern.description = "Auto-detected pattern of length " + std::to_string(pattern_length);
      pattern.recommended_strategy = RecoveryStrategy::Retry;
      pattern.first_seen = errors[start].timestamp;
      pattern.last_seen = errors[start + pattern_length - 1].timestamp;
      pattern.occurrence_count = 1;
      pattern.confidence_score = calculate_confidence(pattern);

      patterns.push_back(pattern);
    }
  }

  return patterns;
}

double ErrorPatternAnalyzer::calculate_confidence(const ErrorPattern& pattern) const {
  // Simple confidence calculation based on pattern length and frequency
  double base_confidence = static_cast<double>(pattern.error_sequence.size()) / 10.0;
  double frequency_bonus = static_cast<double>(pattern.occurrence_count) / 100.0;

  return std::min(1.0, base_confidence + frequency_bonus);
}

// ErrorHandlingSystem implementation
ErrorHandlingSystem& ErrorHandlingSystem::instance() {
  static ErrorHandlingSystem instance;
  return instance;
}

void ErrorHandlingSystem::configure(const std::string& config_file) {
  std::lock_guard<std::mutex> lock(system_mutex_);

  initialize_components();

  // Load configuration from file if provided
  if (!config_file.empty()) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
      // Configuration file not found, use defaults
      return;
    }

    // Simple line-based configuration parsing
    // Format: key=value
    std::string line;
    while (std::getline(file, line)) {
      // Skip comments and empty lines
      if (line.empty() || line[0] == '#') {
        continue;
      }

      size_t equals_pos = line.find('=');
      if (equals_pos == std::string::npos) {
        continue;
      }

      std::string key = line.substr(0, equals_pos);
      std::string value = line.substr(equals_pos + 1);

      // Trim whitespace
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      // Apply configuration
      if (key == "log_level") {
        if (value == "debug") logger_->configure(ErrorLogger::LogLevel::Debug, {ErrorLogger::LogTarget::Console});
        else if (value == "info") logger_->configure(ErrorLogger::LogLevel::Info, {ErrorLogger::LogTarget::Console});
        else if (value == "warning") logger_->configure(ErrorLogger::LogLevel::Warning, {ErrorLogger::LogTarget::Console});
        else if (value == "error") logger_->configure(ErrorLogger::LogLevel::Error, {ErrorLogger::LogTarget::Console});
        else if (value == "critical") logger_->configure(ErrorLogger::LogLevel::Critical, {ErrorLogger::LogTarget::Console});
      } else if (key == "log_file") {
        logger_->set_log_file(value);
      } else if (key == "network_endpoint") {
        logger_->set_network_endpoint(value);
      } else if (key == "max_recent_errors") {
        try {
          max_recent_errors_ = static_cast<size_t>(std::stoul(value));
        } catch (...) {
          // Invalid value, keep default
        }
      }
    }
  }
}

void ErrorHandlingSystem::report_error(const DetailedError& error) {
  std::lock_guard<std::mutex> lock(system_mutex_);

  // Add to recent errors
  recent_errors_.push_back(error);
  if (recent_errors_.size() > max_recent_errors_) {
    recent_errors_.erase(recent_errors_.begin());
  }

  // Log the error
  if (logger_) {
    logger_->log_error(error);
  }

  // Update pattern analysis
  if (pattern_analyzer_) {
    pattern_analyzer_->update_patterns(error);
  }

  // Update health metrics
  update_health_metrics();
}

void ErrorHandlingSystem::report_validation_result(const ValidationResult& result) {
  if (logger_) {
    logger_->log_validation_result(result);
  }

  // Report individual errors
  for (const auto& error : result.errors) {
    report_error(error);
  }

  for (const auto& warning : result.warnings) {
    report_error(warning);
  }
}

bool ErrorHandlingSystem::attempt_error_recovery(const DetailedError& error) {
  if (!recovery_manager_) {
    return false;
  }

  RecoveryAction action = recovery_manager_->determine_recovery_strategy(error);
  bool success = recovery_manager_->attempt_recovery(error, action);

  if (logger_) {
    logger_->log_recovery_attempt(error, action, success);
  }

  return success;
}

void ErrorHandlingSystem::register_recovery_strategy(
    ErrorCode code, std::function<RecoveryAction(const DetailedError&)> strategy) {
  if (recovery_manager_) {
    recovery_manager_->register_recovery_strategy(code, strategy);
  }
}

ErrorStatistics ErrorHandlingSystem::get_error_statistics() const {
  if (recovery_manager_) {
    return recovery_manager_->get_recovery_statistics();
  }
  return ErrorStatistics{};
}

std::vector<ErrorPattern> ErrorHandlingSystem::get_error_patterns() const {
  if (pattern_analyzer_) {
    return pattern_analyzer_->get_frequent_patterns();
  }
  return {};
}

std::vector<std::pair<ErrorCode, double>> ErrorHandlingSystem::predict_errors() const {
  if (pattern_analyzer_) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    return pattern_analyzer_->predict_next_errors(recent_errors_);
  }
  return {};
}

void ErrorHandlingSystem::set_log_level(ErrorLogger::LogLevel level) {
  if (logger_) {
    logger_->configure(level, {ErrorLogger::LogTarget::Console});
  }
}

void ErrorHandlingSystem::add_log_target(ErrorLogger::LogTarget target) {
  // This would add to existing targets
  if (logger_) {
    logger_->configure(ErrorLogger::LogLevel::Warning, {target});
  }
}

void ErrorHandlingSystem::set_log_file(const std::string& file_path) {
  if (logger_) {
    logger_->set_log_file(file_path);
  }
}

bool ErrorHandlingSystem::is_system_healthy() const {
  return get_system_health_score() > 0.7;  // 70% threshold
}

double ErrorHandlingSystem::get_system_health_score() const {
  std::lock_guard<std::mutex> lock(system_mutex_);
  return calculate_health_score();
}

std::string ErrorHandlingSystem::generate_health_report() const {
  std::ostringstream oss;

  oss << "=== System Health Report ===\n";
  oss << "Health Score: " << std::fixed << std::setprecision(1) << (get_system_health_score() * 100)
      << "%\n";
  oss << "System Status: " << (is_system_healthy() ? "HEALTHY" : "DEGRADED") << "\n";

  std::lock_guard<std::mutex> lock(system_mutex_);
  oss << "Recent Errors: " << recent_errors_.size() << "\n";

  if (recovery_manager_) {
    auto stats = recovery_manager_->get_recovery_statistics();
    oss << "\n" << stats.generate_report();
  }

  return oss.str();
}

void ErrorHandlingSystem::cleanup_old_errors(std::chrono::hours max_age) {
  std::lock_guard<std::mutex> lock(system_mutex_);

  auto cutoff_time = std::chrono::system_clock::now() - max_age;

  recent_errors_.erase(std::remove_if(recent_errors_.begin(), recent_errors_.end(),
                                      [cutoff_time](const DetailedError& error) {
                                        return error.timestamp < cutoff_time;
                                      }),
                       recent_errors_.end());
}

void ErrorHandlingSystem::export_error_data(const std::string& file_path) const {
  std::ofstream file(file_path);
  if (!file.is_open()) {
    return;
  }

  std::lock_guard<std::mutex> lock(system_mutex_);

  file << "# Solar System Suite Error Data Export\n";
  file << "# Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";

  for (const auto& error : recent_errors_) {
    file << "ERROR," << static_cast<int>(error.code) << "," << static_cast<int>(error.severity)
         << "," << error.timestamp.time_since_epoch().count() << "," << error.message << "\n";
  }
}

void ErrorHandlingSystem::import_error_data(const std::string& file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    return;
  }

  std::lock_guard<std::mutex> lock(system_mutex_);

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Parse CSV format: ERROR,code,severity,timestamp,message
    if (line.find("ERROR,") == 0) {
      std::istringstream iss(line.substr(6)); // Skip "ERROR,"
      std::string code_str, severity_str, timestamp_str, message;

      if (std::getline(iss, code_str, ',') &&
          std::getline(iss, severity_str, ',') &&
          std::getline(iss, timestamp_str, ',') &&
          std::getline(iss, message)) {

        try {
          ErrorCode code = static_cast<ErrorCode>(std::stoi(code_str));
          ErrorSeverity severity = static_cast<ErrorSeverity>(std::stoi(severity_str));

          DetailedError error(code, message, severity);

          // Parse timestamp
          try {
            auto timestamp_value = std::stoll(timestamp_str);
            error.timestamp = std::chrono::system_clock::time_point(
                std::chrono::system_clock::duration(timestamp_value));
          } catch (...) {
            // Use current time if timestamp parsing fails
          }

          recent_errors_.push_back(error);

          // Maintain size limit
          if (recent_errors_.size() > max_recent_errors_) {
            recent_errors_.erase(recent_errors_.begin());
          }
        } catch (...) {
          // Skip malformed entries
          continue;
        }
      }
    }
  }
}

void ErrorHandlingSystem::initialize_components() {
  if (!recovery_manager_) {
    recovery_manager_ = std::make_unique<ErrorRecoveryManager>();
  }

  if (!logger_) {
    logger_ = std::make_unique<ErrorLogger>();
    logger_->configure(ErrorLogger::LogLevel::Warning, {ErrorLogger::LogTarget::Console});
  }

  if (!pattern_analyzer_) {
    pattern_analyzer_ = std::make_unique<ErrorPatternAnalyzer>();
  }
}

void ErrorHandlingSystem::update_health_metrics() {
  // Health metrics are updated based on recent error patterns
  // This is called after each error is reported

  // Update statistics in recovery manager if available
  if (recovery_manager_ && !recent_errors_.empty()) {
    recovery_manager_->get_recovery_statistics();
  }

  // Trigger pattern analysis for recent errors
  if (pattern_analyzer_ && recent_errors_.size() >= 2) {
    std::vector<DetailedError> recent_subset;
    size_t start = recent_errors_.size() > 10 ? recent_errors_.size() - 10 : 0;
    recent_subset.assign(recent_errors_.begin() + static_cast<std::ptrdiff_t>(start), recent_errors_.end());
    pattern_analyzer_->analyze_patterns(recent_subset);
  }
}

double ErrorHandlingSystem::calculate_health_score() const {
  if (recent_errors_.empty()) {
    return 1.0;  // Perfect health if no recent errors
  }

  // Calculate health based on error severity and frequency
  double severity_penalty = 0.0;
  for (const auto& error : recent_errors_) {
    switch (error.severity) {
      case ErrorSeverity::Info:
        severity_penalty += 0.01;
        break;
      case ErrorSeverity::Warning:
        severity_penalty += 0.05;
        break;
      case ErrorSeverity::Error:
        severity_penalty += 0.1;
        break;
      case ErrorSeverity::Critical:
        severity_penalty += 0.3;
        break;
      case ErrorSeverity::Fatal:
        severity_penalty += 1.0;
        break;
    }
  }

  // Normalize penalty based on number of errors
  severity_penalty /= recent_errors_.size();

  return std::max(0.0, 1.0 - severity_penalty);
}

// ErrorUtils implementation
namespace ErrorUtils {

std::string error_code_to_string(ErrorCode code) {
  switch (code) {
    case ErrorCode::InvalidInput:
      return "InvalidInput";
    case ErrorCode::InvalidFormat:
      return "InvalidFormat";
    case ErrorCode::InvalidRange:
      return "InvalidRange";
    case ErrorCode::MissingRequired:
      return "MissingRequired";
    case ErrorCode::ConflictingParameters:
      return "ConflictingParameters";
    case ErrorCode::ConnectionFailed:
      return "ConnectionFailed";
    case ErrorCode::ConnectionTimeout:
      return "ConnectionTimeout";
    case ErrorCode::NetworkUnavailable:
      return "NetworkUnavailable";
    case ErrorCode::InvalidResponse:
      return "InvalidResponse";
    case ErrorCode::AuthenticationFailed:
      return "AuthenticationFailed";
    case ErrorCode::FileNotFound:
      return "FileNotFound";
    case ErrorCode::FileAccessDenied:
      return "FileAccessDenied";
    case ErrorCode::FileCorrupted:
      return "FileCorrupted";
    case ErrorCode::DiskFull:
      return "DiskFull";
    case ErrorCode::DirectoryNotFound:
      return "DirectoryNotFound";
    case ErrorCode::OutOfMemory:
      return "OutOfMemory";
    case ErrorCode::MemoryLeak:
      return "MemoryLeak";
    case ErrorCode::InvalidPointer:
      return "InvalidPointer";
    case ErrorCode::BufferOverflow:
      return "BufferOverflow";
    case ErrorCode::ConfigNotFound:
      return "ConfigNotFound";
    case ErrorCode::ConfigInvalid:
      return "ConfigInvalid";
    case ErrorCode::ConfigMissing:
      return "ConfigMissing";
    case ErrorCode::ConfigConflict:
      return "ConfigConflict";
    case ErrorCode::OperationFailed:
      return "OperationFailed";
    case ErrorCode::StateInvalid:
      return "StateInvalid";
    case ErrorCode::ResourceUnavailable:
      return "ResourceUnavailable";
    case ErrorCode::TimeoutExpired:
      return "TimeoutExpired";
    case ErrorCode::ResourceExhausted:
      return "ResourceExhausted";
    case ErrorCode::ResourceLocked:
      return "ResourceLocked";
    case ErrorCode::ResourceCorrupted:
      return "ResourceCorrupted";
    case ErrorCode::ResourceConflict:
      return "ResourceConflict";
    case ErrorCode::AccessDenied:
      return "AccessDenied";
    case ErrorCode::InvalidCredentials:
      return "InvalidCredentials";
    case ErrorCode::SecurityViolation:
      return "SecurityViolation";
    case ErrorCode::PerformanceDegraded:
      return "PerformanceDegraded";
    case ErrorCode::ResourceContention:
      return "ResourceContention";
    case ErrorCode::Unknown:
      return "Unknown";
    default:
      return "UnknownErrorCode";
  }
}

std::string error_category_to_string(ErrorCategory category) {
  switch (category) {
    case ErrorCategory::Validation:
      return "Validation";
    case ErrorCategory::Network:
      return "Network";
    case ErrorCategory::FileSystem:
      return "FileSystem";
    case ErrorCategory::Memory:
      return "Memory";
    case ErrorCategory::Configuration:
      return "Configuration";
    case ErrorCategory::Runtime:
      return "Runtime";
    case ErrorCategory::Resource:
      return "Resource";
    case ErrorCategory::Security:
      return "Security";
    case ErrorCategory::Performance:
      return "Performance";
    case ErrorCategory::Unknown:
      return "Unknown";
    default:
      return "UnknownCategory";
  }
}

std::string error_severity_to_string(ErrorSeverity severity) {
  switch (severity) {
    case ErrorSeverity::Info:
      return "Info";
    case ErrorSeverity::Warning:
      return "Warning";
    case ErrorSeverity::Error:
      return "Error";
    case ErrorSeverity::Critical:
      return "Critical";
    case ErrorSeverity::Fatal:
      return "Fatal";
    default:
      return "UnknownSeverity";
  }
}

std::string recovery_strategy_to_string(RecoveryStrategy strategy) {
  switch (strategy) {
    case RecoveryStrategy::None:
      return "None";
    case RecoveryStrategy::Retry:
      return "Retry";
    case RecoveryStrategy::RetryWithDelay:
      return "RetryWithDelay";
    case RecoveryStrategy::RetryWithExponentialBackoff:
      return "RetryWithExponentialBackoff";
    case RecoveryStrategy::Fallback:
      return "Fallback";
    case RecoveryStrategy::GracefulDegradation:
      return "GracefulDegradation";
    case RecoveryStrategy::FailFast:
      return "FailFast";
    case RecoveryStrategy::UserInterventionRequired:
      return "UserInterventionRequired";
    case RecoveryStrategy::AutomaticRecovery:
      return "AutomaticRecovery";
    case RecoveryStrategy::RestartComponent:
      return "RestartComponent";
    case RecoveryStrategy::RestartSystem:
      return "RestartSystem";
    default:
      return "UnknownStrategy";
  }
}

ErrorCode string_to_error_code(const std::string& error_string) {
  // Implement reverse lookup using a static map
  static const std::unordered_map<std::string, ErrorCode> error_code_map = {
      // Validation errors (1000-1999)
      {"InvalidInput", ErrorCode::InvalidInput},
      {"InvalidFormat", ErrorCode::InvalidFormat},
      {"InvalidRange", ErrorCode::InvalidRange},
      {"MissingRequired", ErrorCode::MissingRequired},
      {"ConflictingParameters", ErrorCode::ConflictingParameters},

      // Network errors (2000-2999)
      {"ConnectionFailed", ErrorCode::ConnectionFailed},
      {"ConnectionTimeout", ErrorCode::ConnectionTimeout},
      {"NetworkUnavailable", ErrorCode::NetworkUnavailable},
      {"InvalidResponse", ErrorCode::InvalidResponse},
      {"AuthenticationFailed", ErrorCode::AuthenticationFailed},

      // File system errors (3000-3999)
      {"FileNotFound", ErrorCode::FileNotFound},
      {"FileAccessDenied", ErrorCode::FileAccessDenied},
      {"FileCorrupted", ErrorCode::FileCorrupted},
      {"DiskFull", ErrorCode::DiskFull},
      {"DirectoryNotFound", ErrorCode::DirectoryNotFound},

      // Memory errors (4000-4999)
      {"OutOfMemory", ErrorCode::OutOfMemory},
      {"MemoryLeak", ErrorCode::MemoryLeak},
      {"InvalidPointer", ErrorCode::InvalidPointer},
      {"BufferOverflow", ErrorCode::BufferOverflow},

      // Configuration errors (5000-5999)
      {"ConfigNotFound", ErrorCode::ConfigNotFound},
      {"ConfigInvalid", ErrorCode::ConfigInvalid},
      {"ConfigMissing", ErrorCode::ConfigMissing},
      {"ConfigConflict", ErrorCode::ConfigConflict},

      // Runtime errors (6000-6999)
      {"OperationFailed", ErrorCode::OperationFailed},
      {"StateInvalid", ErrorCode::StateInvalid},
      {"ResourceUnavailable", ErrorCode::ResourceUnavailable},
      {"TimeoutExpired", ErrorCode::TimeoutExpired},

      // Resource errors (7000-7999)
      {"ResourceExhausted", ErrorCode::ResourceExhausted},
      {"ResourceLocked", ErrorCode::ResourceLocked},
      {"ResourceCorrupted", ErrorCode::ResourceCorrupted},
      {"ResourceConflict", ErrorCode::ResourceConflict},

      // Security errors (8000-8999)
      {"AccessDenied", ErrorCode::AccessDenied},
      {"InvalidCredentials", ErrorCode::InvalidCredentials},
      {"SecurityViolation", ErrorCode::SecurityViolation},

      // Performance errors (9000-9999)
      {"PerformanceDegraded", ErrorCode::PerformanceDegraded},
      {"ResourceContention", ErrorCode::ResourceContention},

      // Unknown
      {"Unknown", ErrorCode::Unknown}
  };

  auto it = error_code_map.find(error_string);
  if (it != error_code_map.end()) {
    return it->second;
  }

  // Try parsing as numeric code
  try {
    int code_value = std::stoi(error_string);
    return static_cast<ErrorCode>(code_value);
  } catch (...) {
    // Not a valid number, return Unknown
  }

  return ErrorCode::Unknown;
}

DetailedError create_error_from_exception(const std::exception& ex, const std::string& context) {
  DetailedError error(ErrorCode::OperationFailed, ex.what(), ErrorSeverity::Error, context);
  error.suggestions.push_back("Check the operation parameters and try again");
  error.recovery_action = "Retry the operation with different parameters";
  return error;
}

DetailedError create_validation_error(const std::string& field, const std::string& value,
                                      const std::string& expected) {
  std::string message =
      "Invalid value for field '" + field + "': got '" + value + "', expected " + expected;
  DetailedError error(ErrorCode::InvalidInput, message, ErrorSeverity::Error, "Input validation");
  error.suggestions.push_back("Provide a valid value for " + field);
  error.suggestions.push_back("Expected format: " + expected);
  return error;
}

DetailedError create_network_error(const std::string& endpoint, const std::string& operation,
                                   const std::string& details) {
  std::string message = "Network error during " + operation + " to " + endpoint + ": " + details;
  DetailedError error(ErrorCode::ConnectionFailed, message, ErrorSeverity::Error,
                      "Network operation");
  error.suggestions.push_back("Check network connectivity");
  error.suggestions.push_back("Verify endpoint is accessible: " + endpoint);
  error.suggestions.push_back("Try again later");
  return error;
}

DetailedError create_filesystem_error(const std::string& file_path, const std::string& operation,
                                      const std::string& details) {
  std::string message =
      "File system error during " + operation + " on " + file_path + ": " + details;
  DetailedError error(ErrorCode::FileAccessDenied, message, ErrorSeverity::Error,
                      "File system operation");
  error.suggestions.push_back("Check file permissions");
  error.suggestions.push_back("Verify file path exists: " + file_path);
  error.suggestions.push_back("Check available disk space");
  return error;
}

}  // namespace ErrorUtils

}  // namespace SolarSystem::Utils
