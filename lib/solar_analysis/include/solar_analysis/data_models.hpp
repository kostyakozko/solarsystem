#pragma once

/**
 * @file data_models.hpp
 * @brief Data structures and models for orbital analysis
 */

#include <chrono>
#include <cmath>
#include <optional>
#include <solar_analysis/export.hpp>
#include <solar_core/math/vector3.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Generic timestamped value container
 */
template <typename T>
struct TimestampedValue {
  std::chrono::system_clock::time_point timestamp;
  T value;

  [[nodiscard]] bool is_valid() const noexcept {
    return timestamp != std::chrono::system_clock::time_point{};
  }
};

/**
 * @brief Classical orbital elements (Keplerian elements)
 */
struct SOLAR_ANALYSIS_API OrbitalElements {
  double semi_major_axis = 0.0;     // a - km
  double eccentricity = 0.0;        // e - dimensionless
  double inclination = 0.0;         // i - radians
  double longitude_asc_node = 0.0;  // Ω (RAAN) - radians
  double argument_periapsis = 0.0;  // ω - radians
  double true_anomaly = 0.0;        // ν - radians
  double mean_anomaly = 0.0;        // M - radians
  double orbital_period = 0.0;      // T - seconds
  double periapsis = 0.0;           // q - km
  double apoapsis = 0.0;            // Q - km

  [[nodiscard]] bool is_valid() const noexcept {
    return semi_major_axis > 0 && eccentricity >= 0 && eccentricity < 1;
  }

  [[nodiscard]] double specific_angular_momentum() const noexcept {
    return std::sqrt(semi_major_axis * (1 - eccentricity * eccentricity));
  }
};

/**
 * @brief Reference frame types
 */
enum class ReferenceFrame { J2000_Ecliptic, J2000_Equatorial, ICRF, BodyFixed };

/**
 * @brief Coordinate system types
 */
enum class CoordinateSystem { Cartesian, Spherical, Cylindrical };

/**
 * @brief Coordinate conversion utilities
 */
class SOLAR_ANALYSIS_API CoordinateConverter {
 public:
  // Cartesian <-> Spherical (r, theta, phi)
  [[nodiscard]] static Math::Vector3d cartesian_to_spherical(const Math::Vector3d& cart);
  [[nodiscard]] static Math::Vector3d spherical_to_cartesian(const Math::Vector3d& sph);

  // Reference frame transformations
  [[nodiscard]] static Math::Vector3d transform(const Math::Vector3d& vec, ReferenceFrame from,
                                                ReferenceFrame to);

  // Ecliptic <-> Equatorial (J2000 obliquity = 23.439 degrees)
  [[nodiscard]] static Math::Vector3d ecliptic_to_equatorial(const Math::Vector3d& ecl);
  [[nodiscard]] static Math::Vector3d equatorial_to_ecliptic(const Math::Vector3d& equ);

 private:
  static constexpr double OBLIQUITY_J2000 = 0.409092804;  // 23.439 degrees in radians
};

/**
 * @brief Data serialization utilities
 */
class SOLAR_ANALYSIS_API DataSerializer {
 public:
  // JSON serialization
  [[nodiscard]] static std::string to_json(const OrbitalElements& elements);
  [[nodiscard]] static std::optional<OrbitalElements> orbital_elements_from_json(
      const std::string& json);

  // CSV serialization
  [[nodiscard]] static std::string to_csv_header();
  [[nodiscard]] static std::string to_csv(const OrbitalElements& elements);
};

}  // namespace SolarSystem::Analysis
