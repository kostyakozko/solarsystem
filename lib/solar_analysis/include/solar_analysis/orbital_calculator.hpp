#pragma once

/**
 * @file orbital_calculator.hpp
 * @brief Orbital mechanics calculations and Kepler equation solving
 */

#include <optional>
#include <solar_analysis/data_models.hpp>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>

namespace SolarSystem::Analysis {

/**
 * @brief Orbital calculator for computing orbital elements and predictions
 */
class SOLAR_ANALYSIS_API OrbitalCalculator {
 public:
  // Standard gravitational parameter (GM) for the Sun in km³/s²
  static constexpr double GM_SUN = 1.32712440018e11;

  OrbitalCalculator() = default;
  explicit OrbitalCalculator(double central_body_gm);

  // Orbital elements from state vector
  [[nodiscard]] OrbitalElements calculate_elements(const StateVector& state) const;
  [[nodiscard]] OrbitalElements calculate_elements(const Math::Vector3d& position,
                                                   const Math::Vector3d& velocity) const;

  // Kepler equation solving
  [[nodiscard]] double solve_kepler(double mean_anomaly, double eccentricity,
                                    double tolerance = 1e-10) const;
  [[nodiscard]] double eccentric_to_true_anomaly(double eccentric_anomaly,
                                                 double eccentricity) const;
  [[nodiscard]] double true_to_eccentric_anomaly(double true_anomaly, double eccentricity) const;

  // Orbital parameters
  [[nodiscard]] double calculate_orbital_period(double semi_major_axis) const;
  [[nodiscard]] double calculate_periapsis(double semi_major_axis, double eccentricity) const;
  [[nodiscard]] double calculate_apoapsis(double semi_major_axis, double eccentricity) const;

  // Velocity calculations
  [[nodiscard]] double orbital_velocity_at_radius(double radius, double semi_major_axis) const;
  [[nodiscard]] double periapsis_velocity(double semi_major_axis, double eccentricity) const;
  [[nodiscard]] double apoapsis_velocity(double semi_major_axis, double eccentricity) const;

  // Position prediction
  [[nodiscard]] std::optional<StateVector> predict_state(const OrbitalElements& elements,
                                                         double time_since_epoch) const;

  // State vector from orbital elements
  [[nodiscard]] StateVector elements_to_state(const OrbitalElements& elements) const;

 private:
  double gm_ = GM_SUN;
};

/**
 * @brief Trajectory analyzer for spacecraft and close approaches
 */
class SOLAR_ANALYSIS_API TrajectoryAnalyzer {
 public:
  struct CloseApproach {
    std::chrono::system_clock::time_point time;
    double distance;  // km
    std::string body1;
    std::string body2;
  };

  struct DeltaV {
    Math::Vector3d vector;  // km/s
    double magnitude;       // km/s
    std::string description;
  };

  TrajectoryAnalyzer() = default;

  // Close approach detection
  [[nodiscard]] std::vector<CloseApproach> find_close_approaches(
      const std::vector<StateVector>& trajectory1, const std::vector<StateVector>& trajectory2,
      double threshold_km) const;

  // Delta-V calculations
  [[nodiscard]] DeltaV calculate_delta_v(const Math::Vector3d& v1, const Math::Vector3d& v2) const;
  [[nodiscard]] DeltaV hohmann_transfer(double r1, double r2,
                                        double gm = OrbitalCalculator::GM_SUN) const;

  // Trajectory generation
  [[nodiscard]] std::vector<StateVector> generate_trajectory(const OrbitalElements& elements,
                                                             const OrbitalCalculator& calc,
                                                             size_t num_points) const;
};

}  // namespace SolarSystem::Analysis
