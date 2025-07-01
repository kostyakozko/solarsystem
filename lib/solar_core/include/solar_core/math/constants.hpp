#pragma once

namespace SolarSystem::Math::Constants {

// Gravitational constant (m³/kg/s²)
inline constexpr double G = 6.67430e-11;

// Astronomical unit in meters
inline constexpr double AU = 1.495978707e11;

// Time constants
inline constexpr double SECONDS_PER_MINUTE = 60.0;
inline constexpr double SECONDS_PER_HOUR = 3600.0;
inline constexpr double SECONDS_PER_DAY = 86400.0;
inline constexpr double SECONDS_PER_YEAR = 31557600.0;  // Julian year

// Mathematical constants
inline constexpr double PI = 3.141592653589793238462643383279502884;
inline constexpr double TWO_PI = 2.0 * PI;
inline constexpr double HALF_PI = PI / 2.0;

// Conversion factors
inline constexpr double DEG_TO_RAD = PI / 180.0;
inline constexpr double RAD_TO_DEG = 180.0 / PI;

// Solar system specific constants
inline constexpr double SOLAR_MASS = 1.98847e30;  // kg
inline constexpr double EARTH_MASS = 5.97219e24;  // kg
inline constexpr double EARTH_RADIUS = 6.371e6;   // meters

}  // namespace SolarSystem::Math::Constants
