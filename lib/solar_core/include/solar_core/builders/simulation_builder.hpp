#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_utils/logging.hpp"

namespace SolarSystem::Core::Builders {

/**
 * @brief Enhanced error reporting for simulation builder validation
 */
enum class ValidationErrorCode {
  INVALID_TIMESTEP,
  INVALID_MAX_ITERATIONS,
  INVALID_CONVERGENCE_THRESHOLD,
  INVALID_GRAVITATIONAL_CONSTANT,
  NO_BODIES_SPECIFIED,
  EMPTY_BODY_COLLECTION,
  INVALID_DATE_FORMAT,
  DATE_OUT_OF_RANGE,
  INVALID_TIMEZONE,
  AMBIGUOUS_DATE_FORMAT,
  TIMESTEP_TOO_LARGE,
  TIMESTEP_TOO_SMALL,
  CONFLICTING_PARAMETERS,
  INSUFFICIENT_BODIES_FOR_SIMULATION,
  EXCESSIVE_COMPUTATIONAL_LOAD,
  INVALID_BODY_CONFIGURATION,
  CUSTOM_VALIDATION_FAILED
};

/**
 * @brief Severity levels for validation errors
 */
enum class ValidationSeverity {
  WARNING,  // Non-critical issues that may affect performance
  ERROR,    // Critical issues that prevent simulation
  FATAL     // Severe issues that indicate configuration corruption
};

/**
 * @brief Detailed validation error with context and suggestions
 */
struct ValidationError {
  ValidationErrorCode code;
  ValidationSeverity severity;
  std::string message;
  std::string context;
  std::vector<std::string> suggestions;
  std::optional<std::string> recovery_action;
  std::chrono::system_clock::time_point timestamp;

  ValidationError(ValidationErrorCode error_code, ValidationSeverity sev, const std::string& msg,
                  const std::string& ctx = "")
      : code(error_code),
        severity(sev),
        message(msg),
        context(ctx),
        timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Comprehensive validation result with detailed error reporting
 */
struct ValidationResult {
  bool is_valid;
  std::vector<ValidationError> errors;
  std::vector<ValidationError> warnings;
  std::optional<std::string> summary;
  std::unordered_map<std::string, std::string> parameter_status;

  ValidationResult() : is_valid(true) {}

  void add_error(const ValidationError& error) {
    if (error.severity == ValidationSeverity::ERROR ||
        error.severity == ValidationSeverity::FATAL) {
      errors.push_back(error);
      is_valid = false;
    } else {
      warnings.push_back(error);
    }
  }

  void add_error(ValidationErrorCode code, ValidationSeverity severity, const std::string& message,
                 const std::string& context = "") {
    add_error(ValidationError(code, severity, message, context));
  }

  [[nodiscard]] size_t total_issues() const { return errors.size() + warnings.size(); }
  [[nodiscard]] bool has_warnings() const { return !warnings.empty(); }
  [[nodiscard]] bool has_errors() const { return !errors.empty(); }
};

/**
 * @brief Supported date formats for parsing
 */
enum class DateFormat {
  ISO_8601,         // 2025-12-31T23:59:59Z
  ISO_8601_DATE,    // 2025-12-31
  US_FORMAT,        // 12/31/2025
  EUROPEAN_FORMAT,  // 31/12/2025
  LONG_FORMAT,      // December 31, 2025
  UNIX_TIMESTAMP,   // 1735689599
  JULIAN_DAY,       // 2460676.5
  AUTO_DETECT       // Automatically detect format
};

/**
 * @brief Timezone information for date parsing
 */
struct TimezoneInfo {
  std::string name;
  std::string abbreviation;
  int offset_hours;
  int offset_minutes;
  bool is_dst;

  TimezoneInfo() : offset_hours(0), offset_minutes(0), is_dst(false) {}

  TimezoneInfo(const std::string& tz_name, const std::string& abbr, int hours, int minutes = 0,
               bool dst = false)
      : name(tz_name),
        abbreviation(abbr),
        offset_hours(hours),
        offset_minutes(minutes),
        is_dst(dst) {}

  [[nodiscard]] int total_offset_minutes() const { return offset_hours * 60 + offset_minutes; }
};

/**
 * @brief Date parsing result with detailed information
 */
struct DateParseResult {
  bool success;
  std::chrono::system_clock::time_point time_point;
  DateFormat detected_format;
  TimezoneInfo timezone;
  std::string error_message;
  std::vector<std::string> suggestions;

  DateParseResult() : success(false), detected_format(DateFormat::AUTO_DETECT) {}

  DateParseResult(std::chrono::system_clock::time_point tp, DateFormat format)
      : success(true), time_point(tp), detected_format(format) {}
};

/**
 * @brief Date range constraints for validation
 */
struct DateConstraints {
  std::optional<std::chrono::system_clock::time_point> min_date;
  std::optional<std::chrono::system_clock::time_point> max_date;
  std::vector<DateFormat> allowed_formats;
  std::vector<std::string> allowed_timezones;
  bool require_timezone;

  DateConstraints() : require_timezone(false) {}
};

/**
 * @brief Physical constraints for simulation parameters
 */
struct PhysicalConstraints {
  // Timestep constraints (seconds)
  static constexpr double MIN_TIMESTEP = 0.001;               // 1 millisecond
  static constexpr double MAX_TIMESTEP = 86400.0;             // 1 day
  static constexpr double RECOMMENDED_MIN_TIMESTEP = 1.0;     // 1 second
  static constexpr double RECOMMENDED_MAX_TIMESTEP = 3600.0;  // 1 hour

  // Iteration constraints
  static constexpr size_t MIN_ITERATIONS = 1;
  static constexpr size_t MAX_ITERATIONS = 100000000;            // 100 million
  static constexpr size_t RECOMMENDED_MAX_ITERATIONS = 1000000;  // 1 million

  // Convergence threshold constraints
  static constexpr double MIN_CONVERGENCE_THRESHOLD = 1e-20;
  static constexpr double MAX_CONVERGENCE_THRESHOLD = 1e-6;
  static constexpr double RECOMMENDED_CONVERGENCE_THRESHOLD = 1e-12;

  // Gravitational constant constraints (m³/kg/s²)
  static constexpr double MIN_GRAVITATIONAL_CONSTANT = 6.67e-12;  // 10% below standard
  static constexpr double MAX_GRAVITATIONAL_CONSTANT = 6.67e-10;  // 10% above standard
  static constexpr double STANDARD_GRAVITATIONAL_CONSTANT = 6.67430e-11;

  // Body collection constraints
  static constexpr size_t MIN_BODIES_FOR_SIMULATION = 2;
  static constexpr size_t MAX_BODIES_FOR_PERFORMANCE = 1000;
  static constexpr size_t RECOMMENDED_MAX_BODIES = 100;

  // Date constraints
  static constexpr int MIN_YEAR = 1900;
  static constexpr int MAX_YEAR = 2200;
};

/**
 * @brief Fluent interface for building and configuring simulations
 *
 * Provides a modern, chainable API for setting up complex simulations
 * with comprehensive validation, type safety, and integration with the configuration system.
 *
 * Example usage:
 * @code
 * auto validation_result = SimulationBuilder()
 *     .with_bodies(BodySelector().essential().important())
 *     .with_timestep(3600.0)
 *     .with_target_date("2025-12-31")
 *     .with_progress_callback([](double progress) {
 *         LOG_INFO("Simulation", "Progress: " + std::to_string(progress * 100) + "%");
 *     })
 *     .validate_comprehensive();
 *
 * if (validation_result.is_valid) {
 *     auto simulation = builder.build();
 * } else {
 *     // Handle validation errors with detailed feedback
 *     for (const auto& error : validation_result.errors) {
 *         LOG_ERROR("Validation", error.message);
 *     }
 * }
 * @endcode
 */
class SimulationBuilder {
 public:
  /**
   * @brief Progress callback function type
   */
  using ProgressCallback = std::function<void(double progress)>;

  /**
   * @brief Validation callback function type
   */
  using ValidationCallback = std::function<bool(const Bodies::BodyCollection&)>;

  /**
   * @brief Default constructor with sensible defaults
   */
  SimulationBuilder();

  /**
   * @brief Constructor with configuration integration
   */
  explicit SimulationBuilder(double timestep);

  // === FLUENT INTERFACE METHODS ===

  /**
   * @brief Set the celestial bodies for simulation
   */
  SimulationBuilder& with_bodies(Bodies::BodyCollection bodies);

  /**
   * @brief Set the simulation timestep in seconds
   */
  SimulationBuilder& with_timestep(double timestep_seconds);

  /**
   * @brief Set the maximum number of iterations
   */
  SimulationBuilder& with_max_iterations(size_t max_iterations);

  /**
   * @brief Set the target date for simulation (legacy method)
   */
  SimulationBuilder& with_target_date(const std::string& iso_date);

  /**
   * @brief Set the target date using time_t
   */
  SimulationBuilder& with_target_date(std::time_t target_time);

  /**
   * @brief Set the target date using chrono time_point
   */
  SimulationBuilder& with_target_date(std::chrono::system_clock::time_point target);

  /**
   * @brief Set target date with enhanced parsing and format detection
   */
  SimulationBuilder& with_target_date_enhanced(const std::string& date_str,
                                               DateFormat format = DateFormat::AUTO_DETECT);

  /**
   * @brief Set target date with timezone support
   */
  SimulationBuilder& with_target_date_timezone(const std::string& date_str,
                                               const std::string& timezone = "UTC",
                                               DateFormat format = DateFormat::AUTO_DETECT);

  /**
   * @brief Set target date with comprehensive validation
   */
  SimulationBuilder& with_target_date_validated(const std::string& date_str,
                                                const DateConstraints& constraints);

  /**
   * @brief Set date range for simulation
   */
  SimulationBuilder& with_date_range(const std::string& start_date, const std::string& end_date,
                                     DateFormat format = DateFormat::AUTO_DETECT);

  /**
   * @brief Set date constraints for validation
   */
  SimulationBuilder& with_date_constraints(const DateConstraints& constraints);

  /**
   * @brief Enable or disable progress reporting
   */
  SimulationBuilder& with_progress(bool enable_progress = true);

  /**
   * @brief Set custom progress callback
   */
  SimulationBuilder& with_progress_callback(ProgressCallback callback);

  /**
   * @brief Set convergence threshold for simulation accuracy
   */
  SimulationBuilder& with_convergence_threshold(double threshold);

  /**
   * @brief Enable verbose output
   */
  SimulationBuilder& with_verbose_output(bool verbose = true);

  /**
   * @brief Set custom validation callback
   */
  SimulationBuilder& with_validation(ValidationCallback validator);

  /**
   * @brief Use configuration from global config
   */
  SimulationBuilder& from_defaults();

  /**
   * @brief Use configuration from specific config
   */
  SimulationBuilder& from_timestep(double timestep);

  /**
   * @brief Reset to default configuration
   */
  SimulationBuilder& reset_to_defaults();

  // === VALIDATION AND BUILDING ===

  /**
   * @brief Validate current configuration (legacy method)
   */
  bool validate(std::string* error_message = nullptr) const;

  /**
   * @brief Comprehensive validation with detailed error reporting
   */
  [[nodiscard]] ValidationResult validate_comprehensive() const;

  /**
   * @brief Validate with specific validation level
   */
  [[nodiscard]] ValidationResult validate_with_level(ValidationSeverity min_severity) const;

  /**
   * @brief Detect parameter conflicts and inconsistencies
   */
  [[nodiscard]] ValidationResult detect_conflicts() const;

  /**
   * @brief Validate physical constraints for all parameters
   */
  [[nodiscard]] ValidationResult validate_physical_constraints() const;

  /**
   * @brief Cross-parameter validation for parameter interactions
   */
  [[nodiscard]] ValidationResult validate_cross_parameters() const;

  /**
   * @brief Build the configured simulation engine
   */
  std::unique_ptr<Simulation::SimulationEngine> build(std::string* error_message = nullptr);

  /**
   * @brief Build and immediately run the simulation
   */
  std::optional<Bodies::BodyCollection> build_and_run(std::string* error_message = nullptr);

  // === INSPECTION METHODS ===

  /**
   * @brief Get current configuration summary
   */
  std::string get_config_summary() const;

  /**
   * @brief Check if configuration is valid
   */
  bool is_valid() const;

  /**
   * @brief Get estimated simulation duration
   */
  std::chrono::seconds get_estimated_duration() const;

  // === PUBLIC DATE PARSING UTILITIES ===

  /**
   * @brief Parse date with comprehensive format detection (public utility)
   */
  [[nodiscard]] DateParseResult parse_date_comprehensive(
      const std::string& date_str, DateFormat format = DateFormat::AUTO_DETECT) const;

  /**
   * @brief Get list of supported timezones
   */
  [[nodiscard]] std::vector<std::string> get_supported_timezones() const;

 private:
  // Configuration state
  std::optional<Bodies::BodyCollection> bodies_;
  double timestep_ = 3600.0;  // 1 hour default
  size_t max_iterations_ = 1000000;
  std::optional<std::chrono::system_clock::time_point> target_date_;
  bool enable_progress_ = true;
  std::optional<ProgressCallback> progress_callback_;
  double convergence_threshold_ = 1e-12;
  bool verbose_output_ = false;
  std::optional<ValidationCallback> validator_;

  // Enhanced date handling state
  std::optional<std::chrono::system_clock::time_point> start_date_;
  std::optional<std::chrono::system_clock::time_point> end_date_;
  DateConstraints date_constraints_;
  TimezoneInfo default_timezone_;

  // Enhanced validation state
  mutable std::unordered_map<std::string, std::string> validation_cache_;
  mutable std::chrono::system_clock::time_point last_validation_time_;

  // Helper methods
  std::string format_time_point(std::chrono::system_clock::time_point tp) const;
  std::chrono::system_clock::time_point parse_iso_date(const std::string& iso_date) const;

  // Enhanced date parsing helper methods (private)
  [[nodiscard]] DateParseResult parse_date_with_timezone(
      const std::string& date_str, const std::string& timezone,
      DateFormat format = DateFormat::AUTO_DETECT) const;
  [[nodiscard]] DateFormat detect_date_format(const std::string& date_str) const;
  [[nodiscard]] TimezoneInfo parse_timezone(const std::string& timezone_str) const;
  [[nodiscard]] std::chrono::system_clock::time_point apply_timezone_offset(
      std::chrono::system_clock::time_point tp, const TimezoneInfo& tz) const;
  [[nodiscard]] ValidationResult validate_date_constraints(
      std::chrono::system_clock::time_point tp, const DateConstraints& constraints) const;

  // Date format parsing methods
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_iso8601(
      const std::string& date_str) const;
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_us_format(
      const std::string& date_str) const;
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_european_format(
      const std::string& date_str) const;
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_long_format(
      const std::string& date_str) const;
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_unix_timestamp(
      const std::string& date_str) const;
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_julian_day(
      const std::string& date_str) const;

  // Timezone helper methods
  [[nodiscard]] TimezoneInfo get_timezone_info(const std::string& timezone_name) const;
  [[nodiscard]] std::string format_date_with_timezone(std::chrono::system_clock::time_point tp,
                                                      const TimezoneInfo& tz) const;

  // Enhanced validation helper methods
  [[nodiscard]] ValidationResult validate_timestep() const;
  [[nodiscard]] ValidationResult validate_max_iterations() const;
  [[nodiscard]] ValidationResult validate_convergence_threshold() const;
  [[nodiscard]] ValidationResult validate_bodies() const;
  [[nodiscard]] ValidationResult validate_target_date() const;
  [[nodiscard]] ValidationResult validate_computational_load() const;
  [[nodiscard]] ValidationResult validate_parameter_combinations() const;

  // Utility methods for validation
  [[nodiscard]] bool is_timestep_appropriate_for_bodies() const;
  [[nodiscard]] double estimate_computational_complexity() const;
  [[nodiscard]] std::vector<std::string> suggest_timestep_improvements() const;
  [[nodiscard]] std::vector<std::string> suggest_performance_improvements() const;
  [[nodiscard]] std::string get_validation_context() const;
};

/**
 * @brief Body selector for fluent body collection building
 *
 * Provides a chainable interface for selecting which celestial bodies
 * to include in the simulation based on various criteria.
 */
class BodySelector {
 public:
  /**
   * @brief Default constructor
   */
  BodySelector();

  /**
   * @brief Include essential bodies (Sun, planets)
   */
  BodySelector& essential();

  /**
   * @brief Include important bodies (major moons)
   */
  BodySelector& important();

  /**
   * @brief Include optional bodies (spacecraft, minor bodies)
   */
  BodySelector& optional();

  /**
   * @brief Include all available bodies
   */
  BodySelector& all();

  /**
   * @brief Include bodies by specific names
   */
  BodySelector& named(const std::vector<std::string>& names);

  /**
   * @brief Include bodies by type
   */
  BodySelector& of_type(Bodies::BodyType type);

  /**
   * @brief Include bodies by priority
   */
  BodySelector& with_priority(Bodies::BodyPriority priority);

  /**
   * @brief Exclude specific bodies by name
   */
  BodySelector& excluding(const std::vector<std::string>& names);

  /**
   * @brief Apply custom filter
   */
  BodySelector& where(std::function<bool(const Bodies::CelestialBody&)> predicate);

  /**
   * @brief Build the body collection
   */
  std::optional<Bodies::BodyCollection> build(std::string* error_message = nullptr);

  /**
   * @brief Get count of selected bodies
   */
  size_t count() const;

  /**
   * @brief Get names of selected bodies
   */
  std::vector<std::string> get_selected_names() const;

 private:
  std::vector<std::function<bool(const Bodies::CelestialBody&)>> filters_;

  void add_priority_filter(Bodies::BodyPriority priority);
  void add_type_filter(Bodies::BodyType type);
  void add_name_filter(const std::vector<std::string>& names, bool include);
};

/**
 * @brief Configuration template for predefined simulation scenarios
 */
struct ConfigurationTemplate {
  std::string name;
  std::string description;
  Simulation::SimulationConfig config;
  std::vector<std::string> recommended_bodies;
  std::unordered_map<std::string, std::string> metadata;

  ConfigurationTemplate() = default;

  ConfigurationTemplate(const std::string& template_name, const std::string& desc,
                        const Simulation::SimulationConfig& cfg)
      : name(template_name), description(desc), config(cfg) {}
};

/**
 * @brief Configuration conflict information
 */
struct ConfigurationConflict {
  std::string parameter1;
  std::string parameter2;
  std::string description;
  std::vector<std::string> resolution_options;
  std::string recommended_resolution;

  ConfigurationConflict() = default;

  ConfigurationConflict(const std::string& p1, const std::string& p2, const std::string& desc)
      : parameter1(p1), parameter2(p2), description(desc) {}
};

/**
 * @brief Configuration migration information
 */
struct ConfigurationMigration {
  int from_version;
  int to_version;
  std::string description;
  std::function<Simulation::SimulationConfig(const Simulation::SimulationConfig&)> migrate_function;

  ConfigurationMigration() = default;

  ConfigurationMigration(
      int from, int to, const std::string& desc,
      std::function<Simulation::SimulationConfig(const Simulation::SimulationConfig&)> func)
      : from_version(from), to_version(to), description(desc), migrate_function(func) {}
};

/**
 * @brief Enhanced configuration builder for simulation parameters
 *
 * Provides a fluent interface for building simulation configurations
 * with comprehensive validation, conflict detection, templates, and migration support.
 */
class ConfigurationBuilder {
 public:
  /**
   * @brief Configuration version for migration support
   */
  static constexpr int CURRENT_CONFIG_VERSION = 2;

  /**
   * @brief Start with default configuration
   */
  ConfigurationBuilder();

  /**
   * @brief Start with existing configuration
   */
  explicit ConfigurationBuilder(const Simulation::SimulationConfig& base);

  /**
   * @brief Start with configuration template
   */
  explicit ConfigurationBuilder(const std::string& template_name);

  // === PHYSICS PARAMETERS ===

  /**
   * @brief Set timestep in seconds
   */
  ConfigurationBuilder& timestep(double seconds);

  /**
   * @brief Set timestep in minutes
   */
  ConfigurationBuilder& timestep_minutes(double minutes);

  /**
   * @brief Set timestep in hours
   */
  ConfigurationBuilder& timestep_hours(double hours);

  /**
   * @brief Set gravitational constant
   */
  ConfigurationBuilder& gravitational_constant(double G);

  /**
   * @brief Set convergence threshold
   */
  ConfigurationBuilder& convergence_threshold(double threshold);

  /**
   * @brief Set adaptive timestep parameters
   */
  ConfigurationBuilder& adaptive_timestep(bool enable, double min_step = 1.0,
                                          double max_step = 3600.0);

  /**
   * @brief Set collision detection parameters
   */
  ConfigurationBuilder& collision_detection(bool enable, double threshold = 1e6);

  // === ENHANCED PRESETS ===

  /**
   * @brief High accuracy preset for research simulations
   */
  ConfigurationBuilder& high_accuracy();

  /**
   * @brief High performance preset for large-scale simulations
   */
  ConfigurationBuilder& high_performance();

  /**
   * @brief Balanced preset for general use
   */
  ConfigurationBuilder& balanced();

  /**
   * @brief Real-time preset for interactive applications
   */
  ConfigurationBuilder& real_time();

  /**
   * @brief Educational preset for teaching scenarios
   */
  ConfigurationBuilder& educational();

  /**
   * @brief Research preset for scientific studies
   */
  ConfigurationBuilder& research();

  /**
   * @brief Visualization preset for rendering applications
   */
  ConfigurationBuilder& visualization();

  // === TEMPLATE MANAGEMENT ===

  /**
   * @brief Load configuration from template
   */
  ConfigurationBuilder& from_template(const std::string& template_name);

  /**
   * @brief Save current configuration as template
   */
  bool save_as_template(const std::string& template_name, const std::string& description);

  /**
   * @brief Get available template names
   */
  static std::vector<std::string> get_available_templates();

  /**
   * @brief Get template details
   */
  static std::optional<ConfigurationTemplate> get_template(const std::string& name);

  // === VALIDATION AND CONFLICT DETECTION ===

  /**
   * @brief Comprehensive configuration validation
   */
  [[nodiscard]] ValidationResult validate_comprehensive() const;

  /**
   * @brief Detect configuration conflicts
   */
  [[nodiscard]] std::vector<ConfigurationConflict> detect_conflicts() const;

  /**
   * @brief Resolve configuration conflicts automatically
   */
  ConfigurationBuilder& resolve_conflicts_automatically();

  /**
   * @brief Apply specific conflict resolution
   */
  ConfigurationBuilder& resolve_conflict(const std::string& parameter1,
                                         const std::string& parameter2,
                                         const std::string& resolution);

  // === MIGRATION SUPPORT ===

  /**
   * @brief Migrate configuration from older version
   */
  ConfigurationBuilder& migrate_from_version(int version);

  /**
   * @brief Check if configuration needs migration
   */
  bool needs_migration() const;

  /**
   * @brief Get configuration version
   */
  int get_version() const;

  // === SERIALIZATION ===

  /**
   * @brief Export configuration to JSON string
   */
  std::string to_json() const;

  /**
   * @brief Import configuration from JSON string
   */
  ConfigurationBuilder& from_json(const std::string& json_str);

  /**
   * @brief Save configuration to file
   */
  bool save_to_file(const std::string& filename) const;

  /**
   * @brief Load configuration from file
   */
  ConfigurationBuilder& load_from_file(const std::string& filename);

  // === BUILDING ===

  /**
   * @brief Build the configuration
   */
  Simulation::SimulationConfig build() const;

  /**
   * @brief Validate configuration (legacy method)
   */
  bool validate(std::string* error_message = nullptr) const;

  /**
   * @brief Get configuration summary
   */
  std::string get_summary() const;

  /**
   * @brief Compare with another configuration
   */
  std::vector<std::string> compare_with(const ConfigurationBuilder& other) const;

 private:
  Simulation::SimulationConfig config_;
  int config_version_ = CURRENT_CONFIG_VERSION;
  std::unordered_map<std::string, std::string> metadata_;

  // Static template registry
  static std::unordered_map<std::string, ConfigurationTemplate> templates_;
  static std::vector<ConfigurationMigration> migrations_;
  static bool templates_initialized_;

  // Helper methods
  void initialize_templates();
  void initialize_migrations();
  ValidationResult validate_physics_parameters() const;
  ValidationResult validate_adaptive_timestep() const;
  ValidationResult validate_collision_detection() const;
  std::vector<ConfigurationConflict> check_timestep_conflicts() const;
  std::vector<ConfigurationConflict> check_adaptive_conflicts() const;
  std::vector<ConfigurationConflict> check_performance_conflicts() const;
};

}  // namespace SolarSystem::Core::Builders
