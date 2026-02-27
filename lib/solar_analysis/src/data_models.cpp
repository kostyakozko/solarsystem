/**
 * @file data_models.cpp
 * @brief Implementation of data structures and coordinate conversions
 */

#include "solar_analysis/data_models.hpp"

#include <cmath>
#include <nlohmann/json.hpp>
#include <sstream>

namespace SolarSystem::Analysis {

Math::Vector3d CoordinateConverter::cartesian_to_spherical(const Math::Vector3d& cart) {
  double r = cart.magnitude();
  if (r < 1e-10) {
    return Math::Vector3d(0, 0, 0);
  }
  double theta = std::acos(cart.z() / r);       // polar angle
  double phi = std::atan2(cart.y(), cart.x());  // azimuthal angle
  return Math::Vector3d(r, theta, phi);
}

Math::Vector3d CoordinateConverter::spherical_to_cartesian(const Math::Vector3d& sph) {
  double r = sph.x();
  double theta = sph.y();
  double phi = sph.z();
  return Math::Vector3d(r * std::sin(theta) * std::cos(phi), r * std::sin(theta) * std::sin(phi),
                        r * std::cos(theta));
}

Math::Vector3d CoordinateConverter::transform(const Math::Vector3d& vec, ReferenceFrame from,
                                              ReferenceFrame to) {
  if (from == to) {
    return vec;
  }

  // Convert to J2000_Ecliptic as intermediate
  Math::Vector3d ecl = vec;
  if (from == ReferenceFrame::J2000_Equatorial) {
    ecl = equatorial_to_ecliptic(vec);
  }

  // Convert from J2000_Ecliptic to target
  if (to == ReferenceFrame::J2000_Equatorial) {
    return ecliptic_to_equatorial(ecl);
  }

  return ecl;
}

Math::Vector3d CoordinateConverter::ecliptic_to_equatorial(const Math::Vector3d& ecl) {
  double cos_e = std::cos(OBLIQUITY_J2000);
  double sin_e = std::sin(OBLIQUITY_J2000);
  return Math::Vector3d(ecl.x(), ecl.y() * cos_e - ecl.z() * sin_e,
                        ecl.y() * sin_e + ecl.z() * cos_e);
}

Math::Vector3d CoordinateConverter::equatorial_to_ecliptic(const Math::Vector3d& equ) {
  double cos_e = std::cos(OBLIQUITY_J2000);
  double sin_e = std::sin(OBLIQUITY_J2000);
  return Math::Vector3d(equ.x(), equ.y() * cos_e + equ.z() * sin_e,
                        -equ.y() * sin_e + equ.z() * cos_e);
}

std::string DataSerializer::to_json(const OrbitalElements& e) {
  nlohmann::json j;
  j["semi_major_axis"] = e.semi_major_axis;
  j["eccentricity"] = e.eccentricity;
  j["inclination"] = e.inclination;
  j["longitude_asc_node"] = e.longitude_asc_node;
  j["argument_periapsis"] = e.argument_periapsis;
  j["true_anomaly"] = e.true_anomaly;
  j["mean_anomaly"] = e.mean_anomaly;
  j["orbital_period"] = e.orbital_period;
  j["periapsis"] = e.periapsis;
  j["apoapsis"] = e.apoapsis;
  return j.dump();
}

std::optional<OrbitalElements> DataSerializer::orbital_elements_from_json(const std::string& json) {
  try {
    auto j = nlohmann::json::parse(json);
    if (!j.contains("semi_major_axis")) {
      return std::nullopt;
    }

    OrbitalElements e;
    e.semi_major_axis = j.value("semi_major_axis", 0.0);
    e.eccentricity = j.value("eccentricity", 0.0);
    e.inclination = j.value("inclination", 0.0);
    e.longitude_asc_node = j.value("longitude_asc_node", 0.0);
    e.argument_periapsis = j.value("argument_periapsis", 0.0);
    e.true_anomaly = j.value("true_anomaly", 0.0);
    e.mean_anomaly = j.value("mean_anomaly", 0.0);
    e.orbital_period = j.value("orbital_period", 0.0);
    e.periapsis = j.value("periapsis", 0.0);
    e.apoapsis = j.value("apoapsis", 0.0);
    return e;
  } catch (const nlohmann::json::exception&) {
    return std::nullopt;
  }
}

std::string DataSerializer::to_csv_header() {
  return "semi_major_axis,eccentricity,inclination,longitude_asc_node,"
         "argument_periapsis,true_anomaly,mean_anomaly,orbital_period,periapsis,apoapsis";
}

std::string DataSerializer::to_csv(const OrbitalElements& e) {
  std::ostringstream oss;
  oss << e.semi_major_axis << "," << e.eccentricity << "," << e.inclination << ","
      << e.longitude_asc_node << "," << e.argument_periapsis << "," << e.true_anomaly << ","
      << e.mean_anomaly << "," << e.orbital_period << "," << e.periapsis << "," << e.apoapsis;
  return oss.str();
}

}  // namespace SolarSystem::Analysis
