#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_utils/logging.hpp"

namespace SolarSystem::Core::Builders {

/**
 * @brief Fluent interface for building and configuring simulations
 *
 * Provides a modern, chainable API for setting up complex simulations
 * with type safety, validation, and integration with the configuration system.
 *
 * Example usage:
 * @code
 * auto simulation = SimulationBuilder()
 *     .with_bodies(BodySelector().essential().important())
 *     .with_timestep(3600.0)
 *     .with_target_date("2025-12-31")
 *     .with_progress_callback([](double progress) {
 *         LOG_INFO("Simulation", "Progress: " + std::to_string(progress * 100) + "%");
 *     })
 *     .build();
 *
 * if (simulation.has_value()) {
 *     auto result = simulation->run();
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
   * @brief Validate current configuration
   */
  bool validate(std::string* error_message = nullptr) const;

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

  // Helper methods
  std::string format_time_point(std::chrono::system_clock::time_point tp) const;
  std::chrono::system_clock::time_point parse_iso_date(const std::string& iso_date) const;
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
