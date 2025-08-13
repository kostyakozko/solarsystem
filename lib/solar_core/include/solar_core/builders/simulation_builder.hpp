#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

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

  ValidationError(ValidationErrorCode error_code, ValidationSeverity sev,
                  const std::string& msg, const std::string& ctx = "")
    : code(error_code), severity(sev), message(msg), context(ctx),
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

  void add_error(ValidationErrorCode code, ValidationSeverity severity,
                 const std::string& message, const std::string& context = "") {
    add_error(ValidationError(code, severity, message, context));
  }

  [[nodiscard]] size_t total_issues() const { return errors.size() + warnings.size(); }
  [[nodiscard]] bool has_warnings() const { return !warnings.empty(); }
  [[nodiscard]] bool has_errors() const { return !errors.empty(); }
};

/**
 * @brief Physical constraints for simulation parameters
 */
struct PhysicalConstraints {
  // Timestep constraints (seconds)
  static constexpr double MIN_TIMESTEP = 0.001;      // 1 millisecond
  static constexpr double MAX_TIMESTEP = 86400.0;    // 1 day
  static constexpr double RECOMMENDED_MIN_TIMESTEP = 1.0;     // 1 second
  static constexpr double RECOMMENDED_MAX_TIMESTEP = 3600.0;  // 1 hour

  // Iteration constraints
  static constexpr size_t MIN_ITERATIONS = 1;
  static constexpr size_t MAX_ITERATIONS = 100000000;  // 100 million
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
   * @brief Set the target date for simulation
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

  // Enhanced validation state
  mutable std::unordered_map<std::string, std::string> validation_cache_;
  mutable std::chrono::system_clock::time_point last_validation_time_;

  // Helper methods
  std::string format_time_point(std::chrono::system_clock::time_point tp) const;
  std::chrono::system_clock::time_point parse_iso_date(const std::string& iso_date) const;

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
 * @brief Configuration builder for simulation parameters
 *
 * Provides a fluent interface for building simulation configurations
 * that can be used with SimulationBuilder.
 */
class ConfigurationBuilder {
 public:
  /**
   * @brief Start with default configuration
   */
  ConfigurationBuilder();

  /**
   * @brief Start with existing configuration
   */
  explicit ConfigurationBuilder(const Simulation::SimulationConfig& base);

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

  // === PRESETS ===

  /**
   * @brief High accuracy preset
   */
  ConfigurationBuilder& high_accuracy();

  /**
   * @brief High performance preset
   */
  ConfigurationBuilder& high_performance();

  /**
   * @brief Balanced preset
   */
  ConfigurationBuilder& balanced();

  /**
   * @brief Real-time preset
   */
  ConfigurationBuilder& real_time();

  // === BUILDING ===

  /**
   * @brief Build the configuration
   */
  Simulation::SimulationConfig build() const;

  /**
   * @brief Validate configuration
   */
  bool validate(std::string* error_message = nullptr) const;

 private:
  Simulation::SimulationConfig config_;
};

}  // namespace SolarSystem::Core::Builders
