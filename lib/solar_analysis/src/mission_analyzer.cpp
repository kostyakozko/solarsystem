/**
 * @file mission_analyzer.cpp
 * @brief Implementation of mission planning and trajectory analysis
 */

#include "solar_analysis/mission_analyzer.hpp"

#include <algorithm>
#include <cmath>

namespace SolarSystem::Analysis {

MissionAnalyzer::MissionAnalyzer() = default;
MissionAnalyzer::~MissionAnalyzer() = default;

TrajectoryDeviation MissionAnalyzer::analyze_deviation(
    const std::vector<StateVector>& actual, const std::vector<StateVector>& planned) const {
  TrajectoryDeviation result;

  size_t n = std::min(actual.size(), planned.size());
  if (n == 0) return result;

  double sum_pos = 0.0, sum_vel = 0.0;

  for (size_t i = 0; i < n; ++i) {
    double pos_err = static_cast<double>((actual[i].position - planned[i].position).magnitude());
    double vel_err = static_cast<double>((actual[i].velocity - planned[i].velocity).magnitude());

    result.position_errors.push_back(pos_err);
    result.velocity_errors.push_back(vel_err);

    result.max_position_error = std::max(result.max_position_error, pos_err);
    result.max_velocity_error = std::max(result.max_velocity_error, vel_err);
    sum_pos += pos_err;
    sum_vel += vel_err;
  }

  result.mean_position_error = sum_pos / static_cast<double>(n);
  result.mean_velocity_error = sum_vel / static_cast<double>(n);

  return result;
}

std::vector<MissionEvent> MissionAnalyzer::extract_events(
    const std::vector<StateVector>& trajectory, double maneuver_threshold) const {
  std::vector<MissionEvent> events;
  if (trajectory.empty()) return events;

  // Launch event
  events.push_back({trajectory.front().timestamp, "launch", "Mission start", std::nullopt});

  // Detect maneuvers (sudden velocity changes)
  for (size_t i = 1; i < trajectory.size(); ++i) {
    double dv =
        static_cast<double>((trajectory[i].velocity - trajectory[i - 1].velocity).magnitude());
    if (dv > maneuver_threshold) {
      events.push_back({trajectory[i].timestamp, "maneuver", "Velocity change detected", dv});
    }
  }

  // Arrival event
  events.push_back({trajectory.back().timestamp, "arrival", "Mission end", std::nullopt});

  return events;
}

std::vector<EncounterAnalysis> MissionAnalyzer::analyze_encounters(
    const std::vector<StateVector>& spacecraft,
    const std::map<std::string, std::vector<StateVector>>& bodies, double threshold_km) const {
  std::vector<EncounterAnalysis> encounters;

  for (const auto& [name, body_traj] : bodies) {
    auto encounter = analyze_single_encounter(spacecraft, body_traj, name);
    if (encounter.closest_distance < threshold_km) {
      encounters.push_back(encounter);
    }
  }

  std::sort(encounters.begin(), encounters.end(),
            [](const EncounterAnalysis& a, const EncounterAnalysis& b) {
              return a.closest_approach_time < b.closest_approach_time;
            });

  return encounters;
}

EncounterAnalysis MissionAnalyzer::analyze_single_encounter(
    const std::vector<StateVector>& spacecraft, const std::vector<StateVector>& target,
    const std::string& target_name) const {
  EncounterAnalysis result;
  result.target_body = target_name;
  result.closest_distance = std::numeric_limits<double>::max();

  size_t n = std::min(spacecraft.size(), target.size());
  for (size_t i = 0; i < n; ++i) {
    double dist = static_cast<double>((spacecraft[i].position - target[i].position).magnitude());
    if (dist < result.closest_distance) {
      result.closest_distance = dist;
      result.closest_approach_time = spacecraft[i].timestamp;
      result.relative_velocity =
          static_cast<double>((spacecraft[i].velocity - target[i].velocity).magnitude());

      // Approach angle (angle between relative position and velocity)
      auto rel_pos = spacecraft[i].position - target[i].position;
      auto rel_vel = spacecraft[i].velocity - target[i].velocity;
      double dot = rel_pos.dot(rel_vel);
      double mag = static_cast<double>(rel_pos.magnitude() * rel_vel.magnitude());
      result.approach_angle = (mag > 1e-10) ? std::acos(dot / mag) : 0.0;
    }
  }

  // Check gravity assist viability (simplified)
  result.gravity_assist_viable = result.closest_distance > 1000 && result.closest_distance < 1e6;
  if (result.gravity_assist_viable) {
    // Simplified gravity assist delta-v estimate
    result.potential_delta_v =
        2.0 * result.relative_velocity * std::sin(result.approach_angle / 2.0);
  }

  return result;
}

bool MissionAnalyzer::is_collision_risk(const EncounterAnalysis& encounter,
                                        double body_radius_km) const {
  return encounter.closest_distance < body_radius_km * 1.5;  // 50% safety margin
}

TransferTrajectory MissionAnalyzer::calculate_transfer(double r1, double r2,
                                                       const std::string& from,
                                                       const std::string& to) const {
  TransferTrajectory result;
  result.departure_body = from;
  result.arrival_body = to;

  auto dv = trajectory_analyzer_.hohmann_transfer(r1, r2);
  result.total_delta_v = dv.magnitude;

  // Split into departure and arrival
  double v1 = std::sqrt(OrbitalCalculator::GM_SUN / r1);
  double v2 = std::sqrt(OrbitalCalculator::GM_SUN / r2);
  double a_transfer = (r1 + r2) / 2.0;
  double v_transfer_1 = std::sqrt(OrbitalCalculator::GM_SUN * (2.0 / r1 - 1.0 / a_transfer));
  double v_transfer_2 = std::sqrt(OrbitalCalculator::GM_SUN * (2.0 / r2 - 1.0 / a_transfer));

  result.departure_delta_v = std::abs(v_transfer_1 - v1);
  result.arrival_delta_v = std::abs(v2 - v_transfer_2);

  // Flight time (half orbital period of transfer orbit)
  result.flight_time = std::chrono::duration<double>(
      M_PI * std::sqrt(a_transfer * a_transfer * a_transfer / OrbitalCalculator::GM_SUN));

  // Transfer orbit elements
  result.transfer_orbit.semi_major_axis = a_transfer;
  result.transfer_orbit.eccentricity = std::abs(r2 - r1) / (r1 + r2);
  result.transfer_orbit.periapsis = std::min(r1, r2);
  result.transfer_orbit.apoapsis = std::max(r1, r2);

  return result;
}

std::vector<LaunchWindow> MissionAnalyzer::find_launch_windows(
    double departure_radius, double arrival_radius, std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end, int num_windows) const {
  std::vector<LaunchWindow> windows;

  // Synodic period approximation
  double T1 = 2.0 * M_PI * std::sqrt(std::pow(departure_radius, 3) / OrbitalCalculator::GM_SUN);
  double T2 = 2.0 * M_PI * std::sqrt(std::pow(arrival_radius, 3) / OrbitalCalculator::GM_SUN);
  double synodic = std::abs(T1 * T2 / (T1 - T2));

  auto transfer = calculate_transfer(departure_radius, arrival_radius, "", "");

  auto current = start;
  while (windows.size() < static_cast<size_t>(num_windows) && current < end) {
    LaunchWindow window;
    window.optimal_date = current;
    window.open = current - std::chrono::hours(24 * 15);   // 15 days before
    window.close = current + std::chrono::hours(24 * 15);  // 15 days after
    window.optimal_delta_v = transfer.total_delta_v;

    // C3 energy (escape energy)
    double v_escape = std::sqrt(2.0 * OrbitalCalculator::GM_SUN / departure_radius);
    window.c3_energy = calculate_c3(transfer.departure_delta_v, v_escape);

    windows.push_back(window);
    current += std::chrono::duration_cast<std::chrono::system_clock::duration>(
        std::chrono::duration<double>(synodic));
  }

  return windows;
}

double MissionAnalyzer::calculate_c3(double delta_v, double /*escape_velocity*/) const {
  double v_inf = delta_v;  // Simplified: excess velocity
  return v_inf * v_inf;
}

double MissionAnalyzer::estimate_fuel_mass(double delta_v, double dry_mass, double isp) const {
  double mass_ratio = calculate_mass_ratio(delta_v, isp);
  return dry_mass * (mass_ratio - 1.0);
}

double MissionAnalyzer::calculate_mass_ratio(double delta_v, double isp) const {
  // Tsiolkovsky rocket equation: delta_v = isp * g0 * ln(m0/m1)
  constexpr double g0 = 9.80665e-3;  // km/s²
  return std::exp(delta_v / (isp * g0));
}

}  // namespace SolarSystem::Analysis
