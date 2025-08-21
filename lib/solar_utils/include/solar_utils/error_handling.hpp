/**
 * @file error_handling.hpp
 * @brief Comprehensive error handling system (Task 19)
 *
 * Implements comprehensive error handling with:
 * - Detailed error context and reporting
 * - Error categorization and severity levels
 * - Error recovery strategies and mechanisms
 * - Error logging and analysis capabilities
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace SolarSystem::Utils {

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
  Info,        // Informational messages
  Warning,     // Warnings that don't prevent operation
  Error,       // Errors that prevent current operation
  Critical,    // Critical errors that may affect system stability
  Fatal        // Fatal errors that require immediate shutdown
};

/**
 * @brief Error categories for classification
 */
enum class ErrorCategory {
  Validation,     // Input validation errors
  Network,        // Network-related errors
  FileSystem,     // File system and I/O errors
  Memory,         // Memory allocation and management errors
  Configuration,  // Configuration and setup errors
  Runtime,        // Runtime execution errors
  Resource,       // Resource management errors
  Security,       // Security-related errors
  Performance,    // Performance-related issues
  Unknown         // Uncategorized errors
};

/**
 * @brief Specific error codes within categories
 */
enum class ErrorCode {
  // Validation errors (1000-1999)
  InvalidInput = 1000,
  InvalidFormat = 1001,
  InvalidRange = 1002,
  MissingRequired = 1003,
  ConflictingParameters = 1004,

  // Network errors (2000-2999)
  ConnectionFailed = 2000,
  ConnectionTimeout = 2001,
  NetworkUnavailable = 2002,
  InvalidResponse = 2003,
  AuthenticationFailed = 2004,

  // File system errors (3000-3999)
  FileNotFound = 3000,
  FileAccessDenied = 3001,
  FileCorrupted = 3002,
  DiskFull = 3003,
  DirectoryNotFound = 3004,

  // Memory errors (4000-4999)
  OutOfMemory = 4000,
  MemoryLeak = 4001,
  InvalidPointer = 4002,
  BufferOverflow = 4003,

  // Configuration errors (5000-5999)
  ConfigNotFound = 5000,
  ConfigInvalid = 5001,
  ConfigMissing = 5002,
  ConfigConflict = 5003,

  // Runtime errors (6000-6999)
  OperationFailed = 6000,
  StateInvalid = 6001,
  ResourceUnavailable = 6002,
  TimeoutExpired = 6003,

  // Resource errors (7000-7999)
  ResourceExhausted = 7000,
  ResourceLocked = 7001,
  ResourceCorrupted = 7002,
  ResourceConflict = 7003,

  // Security errors (8000-8999)
  AccessDenied = 8000,
  InvalidCredentials = 8001,
  SecurityViolation = 8002,

  // Performance errors (9000-9999)
  PerformanceDegraded = 9000,
  ResourceContention = 9001,

  // Unknown
  Unknown = 0
};

/**
 * @brief Recovery strategies for different error types
 */
enum class RecoveryStrategy {
  None,                        // No recovery possible
  Retry,                       // Retry the operation
  RetryWithDelay,             // Retry after a delay
  RetryWithExponentialBackoff, // Retry with exponential backoff
  Fallback,                   // Use fallback mechanism
  GracefulDegradation,        // Continue with reduced functionality
  FailFast,                   // Fail immediately
  UserInterventionRequired,   // Require user intervention
  AutomaticRecovery,          // Attempt automatic recovery
  RestartComponent,           // Restart the failing component
  RestartSystem              // Restart the entire system
};

/**
 * @brief Validation levels for error checking
 */
enum class ValidationLevel {
  None,        // No validation
  Basic,       // Basic validation only
  Standard,    // Standard validation (default)
  Strict,      // Strict validation
  Paranoid     // Maximum validation
};

/**
 * @brief Detailed error information
 */
struct DetailedError {
  ErrorCode code = ErrorCode::Unknown;
  ErrorCategory category = ErrorCategory::Unknown;
  ErrorSeverity severity = ErrorSeverity::Error;
  std::string message;
  std::string context;
  std::vector<std::string> suggestions;
  std::optional<std::string> recovery_action;
  std::chrono::system_clock::time_point timestamp;
  std::string source_location;  // File:line where error occurred
  std::unordered_map<std::string, std::string> metadata;

  DetailedError() : timestamp(std::chrono::system_clock::now()) {}

  DetailedError(ErrorCode code, const std::string& message,
                ErrorSeverity severity = ErrorSeverity::Error,
                const std::string& context = "")
      : code(code), category(get_category_for_code(code)), severity(severity),
        message(message), context(context),
        timestamp(std::chrono::system_clock::now()) {}

  // Get category based on error code
  static ErrorCategory get_category_for_code(ErrorCode code);

  // Convert to string representation
  std::string to_string() const;

  // Check if error is recoverable
  bool is_recoverable() const;

  // Get recommended recovery strategy
  RecoveryStrategy get_recovery_strategy() const;
};

/**
 * @brief Validation result with errors and warnings
 */
struct ValidationResult {
  bool is_valid = true;
  std::vector<DetailedError> errors;
  std::vector<DetailedError> warnings;
  ValidationLevel level_used = ValidationLevel::Standard;
  std::optional<std::string> summary;
  std::chrono::system_clock::time_point validation_time;

  ValidationResult() : validation_time(std::chrono::system_clock::now()) {}

  // Add error
  void add_error(const DetailedError& error);
  void add_error(ErrorCode code, const std::string& message,
                 ErrorSeverity severity = ErrorSeverity::Error,
                 const std::string& context = "");

  // Add warning
  void add_warning(const DetailedError& warning);
  void add_warning(ErrorCode code, const std::string& message,
                   const std::string& context = "");

  // Check if has errors of specific severity
  bool has_errors_of_severity(ErrorSeverity severity) const;

  // Get errors by category
  std::vector<DetailedError> get_errors_by_category(ErrorCategory category) const;

  // Generate summary
  void generate_summary();

  // Convert to string
  std::string to_string() const;
};

/**
 * @brief Recovery action information
 */
struct RecoveryAction {
  RecoveryStrategy strategy = RecoveryStrategy::None;
  std::string description;
  std::function<bool()> action;
  std::chrono::milliseconds delay{0};
  size_t max_attempts = 1;
  std::unordered_map<std::string, std::string> parameters;

  RecoveryAction() = default;
  RecoveryAction(RecoveryStrategy strategy, const std::string& description,
                 std::function<bool()> action = nullptr)
      : strategy(strategy), description(description), action(action) {}
};

/**
 * @brief Error statistics and analysis
 */
struct ErrorStatistics {
  size_t total_errors = 0;
  size_t errors_by_severity[5] = {0}; // Index by ErrorSeverity
  std::unordered_map<ErrorCategory, size_t> errors_by_category;
  std::unordered_map<ErrorCode, size_t> errors_by_code;
  std::chrono::system_clock::time_point first_error_time;
  std::chrono::system_clock::time_point last_error_time;
  std::chrono::milliseconds total_recovery_time{0};
  size_t successful_recoveries = 0;
  size_t failed_recoveries = 0;

  // Update statistics with new error
  void record_error(const DetailedError& error);

  // Record recovery attempt
  void record_recovery_attempt(bool successful, std::chrono::milliseconds duration);

  // Get error rate (errors per minute)
  double get_error_rate() const;

  // Get recovery success rate
  double get_recovery_success_rate() const;

  // Generate report
  std::string generate_report() const;
};

/**
 * @brief Error pattern for analysis and learning
 */
struct ErrorPattern {
  std::string pattern_id;
  std::vector<ErrorCode> error_sequence;
  std::string description;
  RecoveryStrategy recommended_strategy;
  size_t occurrence_count = 0;
  std::chrono::system_clock::time_point first_seen;
  std::chrono::system_clock::time_point last_seen;
  double confidence_score = 0.0;

  // Check if error sequence matches this pattern
  bool matches(const std::vector<DetailedError>& errors) const;

  // Update pattern with new occurrence
  void update_occurrence();
};

/**
 * @brief Error recovery manager
 */
class ErrorRecoveryManager {
public:
  // Determine recovery strategy for an error
  [[nodiscard]] RecoveryAction determine_recovery_strategy(
      const DetailedError& error) const;

  // Attempt recovery for an error
  [[nodiscard]] bool attempt_recovery(
      const DetailedError& error,
      const RecoveryAction& action);

  // Register custom recovery strategy
  void register_recovery_strategy(
      ErrorCode error_code,
      std::function<RecoveryAction(const DetailedError&)> strategy_provider);

  // Register custom recovery action
  void register_recovery_action(
      ErrorCode error_code,
      RecoveryStrategy strategy,
      std::function<bool(const DetailedError&)> action);

  // Get recovery statistics
  ErrorStatistics get_recovery_statistics() const;

  // Clear recovery statistics
  void clear_statistics();

private:
  std::unordered_map<ErrorCode, std::function<RecoveryAction(const DetailedError&)>> custom_strategies_;
  std::unordered_map<std::string, std::function<bool(const DetailedError&)>> custom_actions_;
  ErrorStatistics statistics_;
  mutable std::mutex statistics_mutex_;

  // Default recovery strategies
  RecoveryAction get_default_recovery_strategy(const DetailedError& error) const;
};

/**
 * @brief Error logger with multiple output targets
 */
class ErrorLogger {
public:
  // Log levels
  enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
  };

  // Log targets
  enum class LogTarget {
    Console,
    File,
    Syslog,
    Network,
    Memory
  };

  // Configure logging
  void configure(LogLevel min_level = LogLevel::Warning,
                 const std::vector<LogTarget>& targets = {LogTarget::Console});

  // Set log file path
  void set_log_file(const std::string& file_path);

  // Set network logging endpoint
  void set_network_endpoint(const std::string& endpoint);

  // Log error
  void log_error(const DetailedError& error);

  // Log validation result
  void log_validation_result(const ValidationResult& result);

  // Log recovery attempt
  void log_recovery_attempt(const DetailedError& error, const RecoveryAction& action, bool success);

  // Get log statistics
  ErrorStatistics get_log_statistics() const;

  // Flush all log targets
  void flush();

private:
  LogLevel min_level_ = LogLevel::Warning;
  std::vector<LogTarget> targets_;
  std::string log_file_path_;
  std::string network_endpoint_;
  ErrorStatistics log_statistics_;
  mutable std::mutex log_mutex_;

  // Internal logging methods
  void log_to_console(const std::string& message, LogLevel level);
  void log_to_file(const std::string& message);
  void log_to_syslog(const std::string& message, LogLevel level);
  void log_to_network(const std::string& message);
  void log_to_memory(const std::string& message);

  // Format log message
  std::string format_log_message(const DetailedError& error) const;
  std::string format_log_message(const ValidationResult& result) const;

  // Convert log level to string
  std::string log_level_to_string(LogLevel level) const;
};

/**
 * @brief Error pattern analyzer for learning and prediction
 */
class ErrorPatternAnalyzer {
public:
  // Analyze error sequence for patterns
  std::vector<ErrorPattern> analyze_patterns(const std::vector<DetailedError>& errors);

  // Register known pattern
  void register_pattern(const ErrorPattern& pattern);

  // Predict likely next errors based on current sequence
  std::vector<std::pair<ErrorCode, double>> predict_next_errors(
      const std::vector<DetailedError>& recent_errors) const;

  // Get pattern statistics
  std::vector<ErrorPattern> get_frequent_patterns(size_t min_occurrences = 5) const;

  // Update patterns with new error
  void update_patterns(const DetailedError& error);

  // Clear pattern history
  void clear_patterns();

private:
  std::vector<ErrorPattern> known_patterns_;
  std::vector<DetailedError> error_history_;
  size_t max_history_size_ = 1000;
  mutable std::mutex patterns_mutex_;

  // Find matching patterns
  std::vector<size_t> find_matching_patterns(const std::vector<DetailedError>& errors) const;

  // Extract patterns from error sequence
  std::vector<ErrorPattern> extract_patterns(const std::vector<DetailedError>& errors) const;

  // Calculate pattern confidence
  double calculate_confidence(const ErrorPattern& pattern) const;
};

/**
 * @brief Comprehensive error handling system
 */
class ErrorHandlingSystem {
public:
  static ErrorHandlingSystem& instance();

  // Configure the error handling system
  void configure(const std::string& config_file = "");

  // Error reporting
  void report_error(const DetailedError& error);
  void report_validation_result(const ValidationResult& result);

  // Error recovery
  bool attempt_error_recovery(const DetailedError& error);
  void register_recovery_strategy(ErrorCode code,
                                 std::function<RecoveryAction(const DetailedError&)> strategy);

  // Error analysis
  ErrorStatistics get_error_statistics() const;
  std::vector<ErrorPattern> get_error_patterns() const;
  std::vector<std::pair<ErrorCode, double>> predict_errors() const;

  // Logging
  void set_log_level(ErrorLogger::LogLevel level);
  void add_log_target(ErrorLogger::LogTarget target);
  void set_log_file(const std::string& file_path);

  // System health
  bool is_system_healthy() const;
  double get_system_health_score() const;
  std::string generate_health_report() const;

  // Maintenance
  void cleanup_old_errors(std::chrono::hours max_age = std::chrono::hours(24));
  void export_error_data(const std::string& file_path) const;
  void import_error_data(const std::string& file_path);

private:
  ErrorHandlingSystem() = default;
  ~ErrorHandlingSystem() = default;

  // Disable copy and move
  ErrorHandlingSystem(const ErrorHandlingSystem&) = delete;
  ErrorHandlingSystem& operator=(const ErrorHandlingSystem&) = delete;

  std::unique_ptr<ErrorRecoveryManager> recovery_manager_;
  std::unique_ptr<ErrorLogger> logger_;
  std::unique_ptr<ErrorPatternAnalyzer> pattern_analyzer_;

  std::vector<DetailedError> recent_errors_;
  size_t max_recent_errors_ = 100;
  mutable std::mutex system_mutex_;

  // Initialize components
  void initialize_components();

  // Update system health metrics
  void update_health_metrics();

  // Calculate health score
  double calculate_health_score() const;
};

/**
 * @brief Utility functions for error handling
 */
namespace ErrorUtils {
  // Convert error code to string
  std::string error_code_to_string(ErrorCode code);

  // Convert error category to string
  std::string error_category_to_string(ErrorCategory category);

  // Convert error severity to string
  std::string error_severity_to_string(ErrorSeverity severity);

  // Convert recovery strategy to string
  std::string recovery_strategy_to_string(RecoveryStrategy strategy);

  // Parse error code from string
  ErrorCode string_to_error_code(const std::string& code_str);

  // Create error from exception
  DetailedError create_error_from_exception(const std::exception& ex,
                                           const std::string& context = "");

  // Create validation error
  DetailedError create_validation_error(const std::string& field,
                                       const std::string& value,
                                       const std::string& expected);

  // Create network error
  DetailedError create_network_error(const std::string& endpoint,
                                    const std::string& operation,
                                    const std::string& details);

  // Create file system error
  DetailedError create_filesystem_error(const std::string& file_path,
                                       const std::string& operation,
                                       const std::string& details);
}

/**
 * @brief Macros for convenient error handling
 */
#define SOLAR_ERROR(code, message) \
  SolarSystem::Utils::DetailedError(code, message, SolarSystem::Utils::ErrorSeverity::Error, \
                                   __FILE__ ":" + std::to_string(__LINE__))

#define SOLAR_WARNING(code, message) \
  SolarSystem::Utils::DetailedError(code, message, SolarSystem::Utils::ErrorSeverity::Warning, \
                                   __FILE__ ":" + std::to_string(__LINE__))

#define SOLAR_CRITICAL(code, message) \
  SolarSystem::Utils::DetailedError(code, message, SolarSystem::Utils::ErrorSeverity::Critical, \
                                   __FILE__ ":" + std::to_string(__LINE__))

#define SOLAR_REPORT_ERROR(error) \
  SolarSystem::Utils::ErrorHandlingSystem::instance().report_error(error)

#define SOLAR_ATTEMPT_RECOVERY(error) \
  SolarSystem::Utils::ErrorHandlingSystem::instance().attempt_error_recovery(error)

#define SOLAR_VALIDATE_AND_REPORT(validation_result) \
  do { \
    if (!validation_result.is_valid) { \
      SolarSystem::Utils::ErrorHandlingSystem::instance().report_validation_result(validation_result); \
    } \
  } while(0)

} // namespace SolarSystem::Utils
