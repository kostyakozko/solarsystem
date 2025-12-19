#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_utils/expected.hpp"

namespace SolarSystem::Simulation {

/**
 * @brief Configuration parameters for the simulation engine
 */
struct SimulationConfig {
  double time_step = 30.0;                      // Time step in seconds
  double gravitational_constant = 6.67430e-11;  // G in m³/kg/s²
  bool use_adaptive_timestep = false;           // Adaptive time stepping
  double max_timestep = 3600.0;                 // Maximum timestep (1 hour)
  double min_timestep = 1.0;                    // Minimum timestep (1 second)
  double tolerance = 1e-12;                     // Error tolerance for adaptive stepping
  bool enable_collision_detection = false;      // Collision detection
  double collision_threshold = 1e6;             // Collision distance threshold (m)
};

/**
 * @brief Statistics and information about the simulation state
 */
struct SimulationState {
  double current_time = 0.0;                             // Current simulation time (seconds)
  std::chrono::system_clock::time_point reference_time;  // Reference epoch
  size_t iteration_count = 0;                            // Number of iterations performed
  long double total_energy = 0.0;                        // Total system energy
  long double kinetic_energy = 0.0;                      // Total kinetic energy
  long double potential_energy = 0.0;                    // Total potential energy
  Math::Vector3d center_of_mass{};                       // System center of mass
  Math::Vector3d total_momentum{};                       // Total system momentum
  double largest_timestep = 0.0;                         // Largest timestep used
  double smallest_timestep = 0.0;                        // Smallest timestep used
};

/**
 * @brief Callback function types for simulation events
 */
using ProgressCallback = std::function<void(const SimulationState&)>;
using CollisionCallback =
    std::function<void(const Bodies::CelestialBody&, const Bodies::CelestialBody&)>;

/**
 * @brief Modern N-body gravitational simulation engine
 *
 * Provides high-performance simulation of celestial body interactions using
 * modern C++ design patterns and numerical integration methods.
 */
class SimulationEngine {
 public:
  // Integration methods
  enum class IntegrationMethod {
    EULER,          // Simple Euler method (fast, less accurate)
    LEAPFROG,       // Leapfrog integration (good for orbital mechanics)
    RUNGE_KUTTA_4,  // 4th order Runge-Kutta (high accuracy)
    VERLET          // Velocity Verlet (energy conserving)
  };

  // Constructor
  explicit SimulationEngine(SimulationConfig config = {});

  // Copy and move semantics
  SimulationEngine(const SimulationEngine&) = delete;
  SimulationEngine(SimulationEngine&&) = default;
  SimulationEngine& operator=(const SimulationEngine&) = delete;
  SimulationEngine& operator=(SimulationEngine&&) = default;

  // Destructor
  ~SimulationEngine() = default;

  // Configuration
  void set_config(const SimulationConfig& config) { config_ = config; }
  [[nodiscard]] const SimulationConfig& get_config() const noexcept { return config_; }

  void set_integration_method(IntegrationMethod method) { integration_method_ = method; }
  [[nodiscard]] IntegrationMethod get_integration_method() const noexcept {
    return integration_method_;
  }

  // Simulation control
  [[nodiscard]] Utils::Expected<void, std::string> initialize(
      Bodies::BodyCollection bodies,
      std::chrono::system_clock::time_point reference_time = std::chrono::system_clock::now());

  [[nodiscard]] Utils::Expected<void, std::string> step();
  [[nodiscard]] Utils::Expected<void, std::string> step(double custom_timestep);

  [[nodiscard]] Utils::Expected<void, std::string> simulate_to_time(double target_time_seconds);
  [[nodiscard]] Utils::Expected<void, std::string> simulate_to_date(
      std::chrono::system_clock::time_point target_date);

  [[nodiscard]] Utils::Expected<void, std::string> simulate_duration(double duration_seconds);

  // State access
  [[nodiscard]] const Bodies::BodyCollection& get_bodies() const;
  [[nodiscard]] Bodies::BodyCollection& get_bodies();
  [[nodiscard]] const SimulationState& get_state() const noexcept { return state_; }

  [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }
  [[nodiscard]] double get_current_time() const noexcept { return state_.current_time; }
  [[nodiscard]] std::chrono::system_clock::time_point get_current_date() const;

  // Callbacks
  void set_progress_callback(ProgressCallback callback) {
    progress_callback_ = std::move(callback);
  }
  void set_collision_callback(CollisionCallback callback) {
    collision_callback_ = std::move(callback);
  }

  // Analysis
  void update_statistics();
  [[nodiscard]] long double calculate_total_energy() const;
  [[nodiscard]] long double calculate_kinetic_energy() const;
  [[nodiscard]] long double calculate_potential_energy() const;
  [[nodiscard]] Math::Vector3d calculate_center_of_mass() const;
  [[nodiscard]] Math::Vector3d calculate_total_momentum() const;

  // Utility
  void reset();
  [[nodiscard]] std::string get_status_summary() const;

 private:
  SimulationConfig config_;
  IntegrationMethod integration_method_ = IntegrationMethod::LEAPFROG;

  Bodies::BodyCollection bodies_;
  SimulationState state_;
  bool initialized_ = false;

  // Callbacks
  ProgressCallback progress_callback_;
  CollisionCallback collision_callback_;

  // Integration methods
  [[nodiscard]] Utils::Expected<void, std::string> step_euler(double dt);
  [[nodiscard]] Utils::Expected<void, std::string> step_leapfrog(double dt);
  [[nodiscard]] Utils::Expected<void, std::string> step_runge_kutta_4(double dt);
  [[nodiscard]] Utils::Expected<void, std::string> step_verlet(double dt);

  // Force calculations
  void calculate_forces(std::vector<Math::Vector3d>& forces) const;
  [[nodiscard]] Math::Vector3d calculate_gravitational_force(
      const Bodies::CelestialBody& body1, const Bodies::CelestialBody& body2) const;

  // Adaptive timestep
  [[nodiscard]] double calculate_adaptive_timestep() const;

  // Collision detection
  void check_collisions();

  // Validation
  [[nodiscard]] Utils::Expected<void, std::string> validate_state() const;
};

// Utility functions
[[nodiscard]] std::string to_string(SimulationEngine::IntegrationMethod method);
[[nodiscard]] SimulationEngine::IntegrationMethod integration_method_from_string(
    std::string_view str);

}  // namespace SolarSystem::Simulation
