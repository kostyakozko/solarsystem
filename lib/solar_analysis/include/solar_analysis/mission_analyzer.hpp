#pragma once

/**
 * @file mission_analyzer.hpp
 * @brief Mission planning and trajectory analysis tools
 */

#include <chrono>
#include <map>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>
#include <solar_analysis/orbital_calculator.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Trajectory deviation result
 */
struct SOLAR_ANALYSIS_API TrajectoryDeviation {
  double max_position_error = 0.0;   // km
  double mean_position_error = 0.0;  // km
  double max_velocity_error = 0.0;   // km/s
  double mean_velocity_error = 0.0;  // km/s
  std::vector<double> position_errors;
  std::vector<double> velocity_errors;
};

/**
 * @brief Mission event
 */
struct SOLAR_ANALYSIS_API MissionEvent {
  std::chrono::system_clock::time_point time;
  std::string type;  // "launch", "maneuver", "flyby", "arrival"
  std::string description;
  std::optional<double> delta_v;  // km/s if maneuver
};

/**
 * @brief Encounter analysis result
 */
struct SOLAR_ANALYSIS_API EncounterAnalysis {
  std::string target_body;
  std::chrono::system_clock::time_point closest_approach_time;
  double closest_distance = 0.0;   // km
  double relative_velocity = 0.0;  // km/s
  double approach_angle = 0.0;     // radians
  bool gravity_assist_viable = false;
  double potential_delta_v = 0.0;  // km/s from gravity assist
};

/**
 * @brief Launch window
 */
struct SOLAR_ANALYSIS_API LaunchWindow {
  std::chrono::system_clock::time_point open;
  std::chrono::system_clock::time_point close;
  double optimal_delta_v = 0.0;  // km/s
  double c3_energy = 0.0;        // km²/s²
  std::chrono::system_clock::time_point optimal_date;
};

/**
 * @brief Transfer trajectory
 */
struct SOLAR_ANALYSIS_API TransferTrajectory {
  std::string departure_body;
  std::string arrival_body;
  double total_delta_v = 0.0;      // km/s
  double departure_delta_v = 0.0;  // km/s
  double arrival_delta_v = 0.0;    // km/s
  std::chrono::duration<double> flight_time{0};
  OrbitalElements transfer_orbit;
};

/**
 * @brief Mission analyzer for spacecraft trajectory and planning
 */
class SOLAR_ANALYSIS_API MissionAnalyzer {
 public:
  MissionAnalyzer();
  ~MissionAnalyzer();

  MissionAnalyzer(const MissionAnalyzer&) = delete;
  MissionAnalyzer& operator=(const MissionAnalyzer&) = delete;

  // Trajectory analysis
  [[nodiscard]] TrajectoryDeviation analyze_deviation(
      const std::vector<StateVector>& actual, const std::vector<StateVector>& planned) const;

  [[nodiscard]] std::vector<MissionEvent> extract_events(const std::vector<StateVector>& trajectory,
                                                         double maneuver_threshold = 0.1) const;

  // Encounter analysis
  [[nodiscard]] std::vector<EncounterAnalysis> analyze_encounters(
      const std::vector<StateVector>& spacecraft,
      const std::map<std::string, std::vector<StateVector>>& bodies, double threshold_km) const;

  [[nodiscard]] EncounterAnalysis analyze_single_encounter(
      const std::vector<StateVector>& spacecraft, const std::vector<StateVector>& target,
      const std::string& target_name) const;

  [[nodiscard]] bool is_collision_risk(const EncounterAnalysis& encounter,
                                       double body_radius_km) const;

  // Mission optimization
  [[nodiscard]] TransferTrajectory calculate_transfer(double r1, double r2, const std::string& from,
                                                      const std::string& to) const;

  [[nodiscard]] std::vector<LaunchWindow> find_launch_windows(
      double departure_radius, double arrival_radius, std::chrono::system_clock::time_point start,
      std::chrono::system_clock::time_point end, int num_windows = 5) const;

  [[nodiscard]] double calculate_c3(double delta_v, double escape_velocity) const;

  // Fuel analysis
  [[nodiscard]] double estimate_fuel_mass(double delta_v, double dry_mass, double isp) const;
  [[nodiscard]] double calculate_mass_ratio(double delta_v, double isp) const;

 private:
  OrbitalCalculator orbital_calc_;
  TrajectoryAnalyzer trajectory_analyzer_;
};

}  // namespace SolarSystem::Analysis
