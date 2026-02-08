/**
 * @file orbital_calculator.cpp
 * @brief Implementation of orbital mechanics calculations
 */

#include "solar_analysis/orbital_calculator.hpp"

#include <algorithm>
#include <cmath>

namespace SolarSystem::Analysis {

OrbitalCalculator::OrbitalCalculator(double central_body_gm) : gm_(central_body_gm) {}

OrbitalElements OrbitalCalculator::calculate_elements(const StateVector& state) const {
  return calculate_elements(state.position, state.velocity);
}

OrbitalElements OrbitalCalculator::calculate_elements(const Math::Vector3d& pos,
                                                      const Math::Vector3d& vel) const {
  OrbitalElements e;

  double r = pos.magnitude();
  double v = vel.magnitude();

  // Specific orbital energy
  double energy = (v * v / 2.0) - (gm_ / r);

  // Semi-major axis
  e.semi_major_axis = -gm_ / (2.0 * energy);

  // Angular momentum vector
  Math::Vector3d h = pos.cross(vel);
  double h_mag = h.magnitude();

  // Eccentricity vector
  Math::Vector3d e_vec = (vel.cross(h) / gm_) - (pos / r);
  e.eccentricity = e_vec.magnitude();

  // Inclination
  e.inclination = std::acos(h.z() / h_mag);

  // Node vector (z-axis cross h)
  Math::Vector3d n(-h.y(), h.x(), 0.0);
  double n_mag = n.magnitude();

  // Longitude of ascending node
  if (n_mag > 1e-10) {
    e.longitude_asc_node = std::acos(n.x() / n_mag);
    if (n.y() < 0) {
      e.longitude_asc_node = 2.0 * M_PI - e.longitude_asc_node;
    }
  }

  // Argument of periapsis
  if (n_mag > 1e-10 && e.eccentricity > 1e-10) {
    e.argument_periapsis = std::acos(n.dot(e_vec) / (n_mag * e.eccentricity));
    if (e_vec.z() < 0) {
      e.argument_periapsis = 2.0 * M_PI - e.argument_periapsis;
    }
  }

  // True anomaly
  if (e.eccentricity > 1e-10) {
    double cos_nu = e_vec.dot(pos) / (e.eccentricity * r);
    cos_nu = std::clamp(cos_nu, -1.0, 1.0);
    e.true_anomaly = std::acos(cos_nu);
    if (pos.dot(vel) < 0) {
      e.true_anomaly = 2.0 * M_PI - e.true_anomaly;
    }
  }

  // Mean anomaly from true anomaly
  double E = true_to_eccentric_anomaly(e.true_anomaly, e.eccentricity);
  e.mean_anomaly = E - e.eccentricity * std::sin(E);

  // Derived quantities
  e.orbital_period = calculate_orbital_period(e.semi_major_axis);
  e.periapsis = calculate_periapsis(e.semi_major_axis, e.eccentricity);
  e.apoapsis = calculate_apoapsis(e.semi_major_axis, e.eccentricity);

  return e;
}

double OrbitalCalculator::solve_kepler(double M, double ecc, double tol) const {
  // Newton-Raphson iteration for Kepler's equation: M = E - e*sin(E)
  double E = M;  // Initial guess
  for (int i = 0; i < 50; ++i) {
    double f = E - ecc * std::sin(E) - M;
    double fp = 1.0 - ecc * std::cos(E);
    double dE = f / fp;
    E -= dE;
    if (std::abs(dE) < tol) break;
  }
  return E;
}

double OrbitalCalculator::eccentric_to_true_anomaly(double E, double ecc) const {
  return 2.0 * std::atan2(std::sqrt(1.0 + ecc) * std::sin(E / 2.0),
                          std::sqrt(1.0 - ecc) * std::cos(E / 2.0));
}

double OrbitalCalculator::true_to_eccentric_anomaly(double nu, double ecc) const {
  return 2.0 * std::atan2(std::sqrt(1.0 - ecc) * std::sin(nu / 2.0),
                          std::sqrt(1.0 + ecc) * std::cos(nu / 2.0));
}

double OrbitalCalculator::calculate_orbital_period(double a) const {
  return 2.0 * M_PI * std::sqrt(a * a * a / gm_);
}

double OrbitalCalculator::calculate_periapsis(double a, double ecc) const {
  return a * (1.0 - ecc);
}

double OrbitalCalculator::calculate_apoapsis(double a, double ecc) const { return a * (1.0 + ecc); }

double OrbitalCalculator::orbital_velocity_at_radius(double r, double a) const {
  return std::sqrt(gm_ * (2.0 / r - 1.0 / a));
}

double OrbitalCalculator::periapsis_velocity(double a, double ecc) const {
  double rp = calculate_periapsis(a, ecc);
  return orbital_velocity_at_radius(rp, a);
}

double OrbitalCalculator::apoapsis_velocity(double a, double ecc) const {
  double ra = calculate_apoapsis(a, ecc);
  return orbital_velocity_at_radius(ra, a);
}

std::optional<StateVector> OrbitalCalculator::predict_state(const OrbitalElements& elem,
                                                            double dt) const {
  if (!elem.is_valid()) return std::nullopt;

  // Mean motion
  double n = 2.0 * M_PI / elem.orbital_period;

  // New mean anomaly
  double M = elem.mean_anomaly + n * dt;
  M = std::fmod(M, 2.0 * M_PI);
  if (M < 0) M += 2.0 * M_PI;

  // Solve Kepler's equation
  double E = solve_kepler(M, elem.eccentricity);
  double nu = eccentric_to_true_anomaly(E, elem.eccentricity);

  // Create new elements with updated anomaly
  OrbitalElements new_elem = elem;
  new_elem.true_anomaly = nu;
  new_elem.mean_anomaly = M;

  return elements_to_state(new_elem);
}

StateVector OrbitalCalculator::elements_to_state(const OrbitalElements& e) const {
  StateVector state;
  state.timestamp = std::chrono::system_clock::now();

  // Distance from focus
  double r = e.semi_major_axis * (1.0 - e.eccentricity * e.eccentricity) /
             (1.0 + e.eccentricity * std::cos(e.true_anomaly));

  // Position in orbital plane
  double x_orb = r * std::cos(e.true_anomaly);
  double y_orb = r * std::sin(e.true_anomaly);

  // Velocity in orbital plane
  double p = e.semi_major_axis * (1.0 - e.eccentricity * e.eccentricity);
  double h = std::sqrt(gm_ * p);
  double vx_orb = -gm_ / h * std::sin(e.true_anomaly);
  double vy_orb = gm_ / h * (e.eccentricity + std::cos(e.true_anomaly));

  // Rotation matrices
  double cos_O = std::cos(e.longitude_asc_node);
  double sin_O = std::sin(e.longitude_asc_node);
  double cos_i = std::cos(e.inclination);
  double sin_i = std::sin(e.inclination);
  double cos_w = std::cos(e.argument_periapsis);
  double sin_w = std::sin(e.argument_periapsis);

  // Transform to inertial frame
  state.position = Math::Vector3d((cos_O * cos_w - sin_O * sin_w * cos_i) * x_orb +
                                      (-cos_O * sin_w - sin_O * cos_w * cos_i) * y_orb,
                                  (sin_O * cos_w + cos_O * sin_w * cos_i) * x_orb +
                                      (-sin_O * sin_w + cos_O * cos_w * cos_i) * y_orb,
                                  (sin_w * sin_i) * x_orb + (cos_w * sin_i) * y_orb);

  state.velocity = Math::Vector3d((cos_O * cos_w - sin_O * sin_w * cos_i) * vx_orb +
                                      (-cos_O * sin_w - sin_O * cos_w * cos_i) * vy_orb,
                                  (sin_O * cos_w + cos_O * sin_w * cos_i) * vx_orb +
                                      (-sin_O * sin_w + cos_O * cos_w * cos_i) * vy_orb,
                                  (sin_w * sin_i) * vx_orb + (cos_w * sin_i) * vy_orb);

  return state;
}

// TrajectoryAnalyzer implementation

std::vector<TrajectoryAnalyzer::CloseApproach> TrajectoryAnalyzer::find_close_approaches(
    const std::vector<StateVector>& t1, const std::vector<StateVector>& t2,
    double threshold) const {
  std::vector<CloseApproach> approaches;

  for (const auto& s1 : t1) {
    for (const auto& s2 : t2) {
      double dist = (s1.position - s2.position).magnitude();
      if (dist < threshold) {
        approaches.push_back({s1.timestamp, dist, "", ""});
      }
    }
  }

  std::sort(approaches.begin(), approaches.end(),
            [](const CloseApproach& a, const CloseApproach& b) { return a.distance < b.distance; });

  return approaches;
}

TrajectoryAnalyzer::DeltaV TrajectoryAnalyzer::calculate_delta_v(const Math::Vector3d& v1,
                                                                 const Math::Vector3d& v2) const {
  Math::Vector3d dv = v2 - v1;
  return {dv, static_cast<double>(dv.magnitude()), "Velocity change"};
}

TrajectoryAnalyzer::DeltaV TrajectoryAnalyzer::hohmann_transfer(double r1, double r2,
                                                                double gm) const {
  double v1 = std::sqrt(gm / r1);
  double v2 = std::sqrt(gm / r2);

  double a_transfer = (r1 + r2) / 2.0;
  double v_transfer_1 = std::sqrt(gm * (2.0 / r1 - 1.0 / a_transfer));
  double v_transfer_2 = std::sqrt(gm * (2.0 / r2 - 1.0 / a_transfer));

  double dv1 = v_transfer_1 - v1;
  double dv2 = v2 - v_transfer_2;
  double total_dv = std::abs(dv1) + std::abs(dv2);

  return {Math::Vector3d(dv1, dv2, 0), total_dv, "Hohmann transfer"};
}

std::vector<StateVector> TrajectoryAnalyzer::generate_trajectory(const OrbitalElements& elem,
                                                                 const OrbitalCalculator& calc,
                                                                 size_t num_points) const {
  std::vector<StateVector> trajectory;
  trajectory.reserve(num_points);

  double dt = elem.orbital_period / static_cast<double>(num_points);

  for (size_t i = 0; i < num_points; ++i) {
    auto state = calc.predict_state(elem, static_cast<double>(i) * dt);
    if (state) {
      trajectory.push_back(*state);
    }
  }

  return trajectory;
}

}  // namespace SolarSystem::Analysis
