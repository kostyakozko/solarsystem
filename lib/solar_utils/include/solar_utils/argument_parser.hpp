/**
 * @file argument_parser.hpp
 * @brief Modern C++20 argument parsing system for Solar System Suite
 *
 * This module provides a type-safe, extensible argument parsing system using modern C++20 features:
 * - RAII-based resource management
 * - std::expected for error handling
 * - Concepts for type safety
 * - Builder pattern for configuration
 * - Comprehensive validation
 */

#pragma once

#include <chrono>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "solar_core/utils/expected.hpp"
#include "solar_utils/validation/input_validator.hpp"

namespace SolarSystem::Utils {

/**
 * @brief Error types for argument parsing (enhanced with validation details)
 */
enum class ArgumentError {
  UnknownOption,
  MissingValue,
  InvalidValue,
  InvalidDateFormat,
  DateOutOfRange,
  ConflictingOptions,
  MissingRequiredOption,
  ValidationFailed,
  SanitizationFailed,
  FormatNotSupported
};

/**
 * @brief Detailed error information with intelligent suggestions
 */
struct DetailedArgumentError {
  ArgumentError error_code = ArgumentError::UnknownOption;
  std::string error_message;
  std::string context;
  std::vector<std::string> suggestions;
  std::vector<std::string> similar_options;
  std::optional<std::string> help_text;
  std::optional<std::string> usage_example;

  DetailedArgumentError() = default;

  DetailedArgumentError(ArgumentError code, const std::string& message)
      : error_code(code), error_message(message) {}

  DetailedArgumentError(ArgumentError code, const std::string& message, const std::string& ctx)
      : error_code(code), error_message(message), context(ctx) {}
};

/**
 * @brief Conflict detection result
 */
struct ConflictReport {
  bool has_conflicts = false;
  std::vector<std::pair<std::string, std::string>> conflicting_pairs;
  std::vector<std::string> resolution_suggestions;

  ConflictReport() = default;
  ConflictReport(bool conflicts) : has_conflicts(conflicts) {}
};

/**
 * @brief Convert ArgumentError to string for error messages
 */
std::string to_string(ArgumentError error);

/**
 * @brief Result type for argument parsing operations
 */
template <typename T>
using ArgumentResult = Expected<T, ArgumentError>;

/**
 * @brief Enhanced result type with detailed error information
 */
template <typename T>
using DetailedArgumentResult = Expected<T, DetailedArgumentError>;

/**
 * @brief Concept for types that can be parsed from string
 */
template <typename T>
concept Parseable = requires(const std::string& str) {
  { T::from_string(str) } -> std::convertible_to<ArgumentResult<T>>;
};

/**
 * @brief Enhanced date representation with comprehensive parsing capabilities
 */
class Date {
 public:
  Date() = default;
  explicit Date(std::chrono::system_clock::time_point tp) : time_point_(tp) {}
  explicit Date(std::time_t t) : time_point_(std::chrono::system_clock::from_time_t(t)) {}

  /**
   * @brief Parse date with automatic format detection and validation
   */
  static ArgumentResult<Date> from_string(const std::string& date_str);

  /**
   * @brief Parse date with specific format validation
   */
  static ArgumentResult<Date> from_string_format(const std::string& date_str,
                                                 const std::string& expected_format);

  /**
   * @brief Parse date with range validation
   */
  static ArgumentResult<Date> from_string_with_range(
      const std::string& date_str, const std::chrono::system_clock::time_point& min_date,
      const std::chrono::system_clock::time_point& max_date);

  /**
   * @brief Get current date
   */
  static Date now();

  /**
   * @brief Convert to time_t for legacy compatibility
   */
  std::time_t to_time_t() const;

  /**
   * @brief Convert to chrono time_point
   */
  std::chrono::system_clock::time_point to_time_point() const { return time_point_; }

  /**
   * @brief Convert to ISO string format
   */
  std::string to_string() const;

  /**
   * @brief Get supported date formats
   */
  static std::vector<std::string> get_supported_formats();

 private:
  std::chrono::system_clock::time_point time_point_{std::chrono::system_clock::now()};
};

/**
 * @brief Configuration structure for simulation arguments
 */
struct SimulationConfig {
  std::optional<Date> target_date;
  bool use_current_date{true};
  std::string date_string;
  bool update_data{false};
  bool rebuild_cache{false};
  bool test_storage{false};
  bool verbose{false};
  std::string body_set{"complete"};  // Default to complete set for main simulation

  /**
   * @brief Get effective target date (current if not specified)
   */
  Date get_target_date() const;
};

/**
 * @brief Extended configuration for specialized applications
 */
struct ExtendedConfig {
  // Simulation options
  std::optional<Date> target_date;
  bool use_current_date{true};

  // Data management options
  bool update_data{false};
  bool force_update{false};
  std::optional<int> target_year;
  bool rebuild_cache{false};
  bool test_storage{false};
  bool validate_cache{false};
  bool clean_cache{false};
  bool show_status{false};

  // Output options
  bool verbose{false};
  bool show_help{false};

  /**
   * @brief Get effective target date (current if not specified)
   */
  Date get_target_date() const;
};

/**
 * @brief Option definition for argument parser
 */
class Option {
 public:
  Option(std::string_view short_name, std::string_view long_name, std::string_view description)
      : short_name_(short_name), long_name_(long_name), description_(description) {}

  /**
   * @brief Set option as requiring a value
   */
  Option& requires_value(bool required = true) {
    requires_value_ = required;
    return *this;
  }

  /**
   * @brief Set option as flag (no value required)
   */
  Option& as_flag() {
    requires_value_ = false;
    return *this;
  }

  /**
   * @brief Set validation function (legacy)
   */
  Option& validate(std::function<bool(const std::string&)> validator) {
    validator_ = std::move(validator);
    return *this;
  }

  /**
   * @brief Set validation type for comprehensive validation
   */
  Option& validate_as(const std::string& validation_type) {
    validation_type_ = validation_type;
    return *this;
  }

  /**
   * @brief Set allowed values for choice validation
   */
  Option& allow_values(const std::vector<std::string>& values) {
    allowed_values_ = values;
    return *this;
  }

  /**
   * @brief Set numeric range for validation
   */
  Option& set_range(int min_val, int max_val) {
    min_int_ = min_val;
    max_int_ = max_val;
    return *this;
  }

  /**
   * @brief Set numeric range for validation (double)
   */
  Option& set_range(double min_val, double max_val) {
    min_double_ = min_val;
    max_double_ = max_val;
    return *this;
  }

  /**
   * @brief Set action to perform when option is found
   */
  template <typename F>
  Option& action(F&& func) {
    action_ = std::forward<F>(func);
    return *this;
  }

  // Getters
  std::string_view short_name() const { return short_name_; }
  std::string_view long_name() const { return long_name_; }
  std::string_view description() const { return description_; }
  bool requires_value() const { return requires_value_; }

  /**
   * @brief Check if this option matches the given argument
   */
  bool matches(std::string_view arg) const;

  /**
   * @brief Validate the value for this option (legacy)
   */
  bool is_valid(const std::string& value) const;

  /**
   * @brief Comprehensive validation with detailed error reporting
   */
  Validation::ValidationResult validate_comprehensive_value(const std::string& value) const;

  /**
   * @brief Execute the action for this option
   */
  void execute(const std::optional<std::string>& value = std::nullopt) const;

 private:
  std::string short_name_;
  std::string long_name_;
  std::string description_;
  bool requires_value_{false};
  std::function<bool(const std::string&)> validator_;
  std::string validation_type_;
  std::vector<std::string> allowed_values_;
  int min_int_ = INT_MIN;
  int max_int_ = INT_MAX;
  double min_double_ = -DBL_MAX;
  double max_double_ = DBL_MAX;
  std::function<void(const std::optional<std::string>&)> action_;
};

/**
 * @brief Modern C++20 argument parser with intelligent error reporting
 */
class ArgumentParser {
 public:
  explicit ArgumentParser(std::string_view program_name) : program_name_(program_name) {}

  /**
   * @brief Add an option to the parser
   */
  ArgumentParser& add_option(Option option);

  /**
   * @brief Parse command line arguments
   */
  ArgumentResult<void> parse(std::span<const char* const> args);

  /**
   * @brief Parse command line arguments (traditional interface)
   */
  ArgumentResult<void> parse(int argc, const char* const argv[]);

  /**
   * @brief Parse with detailed error reporting
   */
  DetailedArgumentResult<void> parse_with_details(std::span<const char* const> args);

  /**
   * @brief Parse with detailed error reporting (traditional interface)
   */
  DetailedArgumentResult<void> parse_with_details(int argc, const char* const argv[]);

  /**
   * @brief Generate help text
   */
  std::string help() const;

  /**
   * @brief Print help to stdout
   */
  void print_help() const;

  /**
   * @brief Generate contextual help for specific option
   */
  std::string contextual_help(const std::string& option_name) const;

  /**
   * @brief Detect argument conflicts
   */
  ConflictReport detect_conflicts(const std::vector<std::string>& provided_args) const;

  /**
   * @brief Find similar options for spell-checking
   */
  std::vector<std::string> find_similar_options(const std::string& invalid_option) const;

  /**
   * @brief Generate usage examples
   */
  std::vector<std::string> generate_usage_examples() const;

  /**
   * @brief Get intelligent suggestions for invalid arguments
   */
  std::vector<std::string> get_intelligent_suggestions(const std::string& invalid_arg) const;

 private:
  std::string program_name_;
  std::vector<Option> options_;
  std::map<std::string, size_t> option_map_;  // Maps option names to indices
  std::map<std::string, std::vector<std::string>> conflicting_options_;  // Conflict rules

  /**
   * @brief Find option by name
   */
  std::optional<size_t> find_option(std::string_view name) const;

  /**
   * @brief Parse a single argument
   */
  ArgumentResult<size_t> parse_argument(std::span<const char* const> args, size_t index);

  /**
   * @brief Parse a single argument with detailed error reporting
   */
  DetailedArgumentResult<size_t> parse_argument_detailed(std::span<const char* const> args,
                                                         size_t index);

  /**
   * @brief Calculate string similarity for spell-checking
   */
  int calculate_edit_distance(const std::string& s1, const std::string& s2) const;

  /**
   * @brief Add conflict rule between options
   */
  void add_conflict_rule(const std::string& option1, const std::string& option2);
};

/**
 * @brief Builder for simulation argument parser
 */
class SimulationArgumentParser {
 public:
  explicit SimulationArgumentParser(std::string_view program_name);

  /**
   * @brief Parse arguments into SimulationConfig
   */
  ArgumentResult<SimulationConfig> parse(int argc, const char* const argv[]);

  /**
   * @brief Print usage information
   */
  void print_usage() const;

 private:
  ArgumentParser parser_;
  SimulationConfig config_;
};

/**
 * @brief Builder for extended argument parser
 */
class ExtendedArgumentParser {
 public:
  explicit ExtendedArgumentParser(std::string_view program_name);

  /**
   * @brief Parse arguments into ExtendedConfig
   */
  ArgumentResult<ExtendedConfig> parse(int argc, const char* const argv[]);

  /**
   * @brief Print usage information
   */
  void print_usage() const;

 private:
  ArgumentParser parser_;
  ExtendedConfig config_;
};

/**
 * @brief Configuration for real-time monitoring applications
 */
struct RealtimeConfig {
  // Display options
  bool show_positions{true};
  bool show_velocities{false};
  bool show_summary{true};
  bool continuous_mode{true};
  bool quiet_mode{false};
  bool verbose_output{false};

  // Timing configuration
  std::chrono::seconds update_interval{1};
  std::chrono::seconds display_interval{1};
  std::optional<std::chrono::seconds> duration_limit;

  // Body selection
  std::vector<std::string> selected_bodies;
  std::string body_set{"important"};  // Default to balanced "important" set
  bool auto_fetch_data{false};

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] bool is_valid(std::string* error = nullptr) const;
};

/**
 * @brief Builder for realtime argument parser
 */
class RealtimeArgumentParser {
 public:
  explicit RealtimeArgumentParser(std::string_view program_name);

  /**
   * @brief Parse arguments into RealtimeConfig
   */
  ArgumentResult<RealtimeConfig> parse(int argc, const char* const argv[]);

  /**
   * @brief Print usage information with beautiful formatting
   */
  void print_usage() const;

 private:
  ArgumentParser parser_;
  RealtimeConfig config_;
  std::string program_name_;
};

}  // namespace SolarSystem::Utils
