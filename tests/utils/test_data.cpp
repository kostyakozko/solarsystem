/**
 * @file test_data.cpp
 * @brief Implementation of test data utilities
 */

#include "test_data.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>

namespace TestData {

// Physical constants (CODATA 2018 values)
double ReferenceData::get_gravitational_constant() {
  return 6.67430e-11;  // m³/kg/s²
}

double ReferenceData::get_au_in_km() {
  return 149597870.7;  // km (exact definition)
}

double ReferenceData::get_earth_mass() {
  return 5.9722e24;  // kg
}

double ReferenceData::get_sun_mass() {
  return 1.98847e30;  // kg
}

double ReferenceData::get_moon_mass() {
  return 7.342e22;  // kg
}

double ReferenceData::get_earth_orbital_period() {
  return 365.25636;  // days (sidereal year)
}

double ReferenceData::get_earth_orbital_radius() {
  return get_au_in_km();  // km
}

double ReferenceData::get_moon_orbital_period() {
  return 27.321661;  // days (sidereal month)
}

double ReferenceData::get_moon_orbital_radius() {
  return 384400.0;  // km (semi-major axis)
}

ReferenceState ReferenceData::get_j2000_state() {
  ReferenceState state;
  state.date = "2000-01-01 12:00:00";
  state.julian_day = 2451545.0;

  // Sun at origin
  ReferenceBody sun;
  sun.name = "Sun";
  sun.mass = get_sun_mass();
  sun.radius = 695700.0;  // km
  sun.position = {0.0, 0.0, 0.0};
  sun.velocity = {0.0, 0.0, 0.0};
  sun.orbital_period = 0.0;
  state.bodies.push_back(sun);

  // Earth (approximate J2000.0 position)
  ReferenceBody earth;
  earth.name = "Earth";
  earth.mass = get_earth_mass();
  earth.radius = 6371.0;                                 // km
  earth.position = {-26499893.0, 144692618.0, 62789.0};  // km (approximate)
  earth.velocity = {-29.7847, -5.5175, -0.0008};         // km/s (approximate)
  earth.orbital_period = get_earth_orbital_period();
  state.bodies.push_back(earth);

  // Moon (relative to Earth)
  ReferenceBody moon;
  moon.name = "Moon";
  moon.mass = get_moon_mass();
  moon.radius = 1737.4;  // km
  moon.position = {earth.position.x - 291608.0, earth.position.y - 266716.0,
                   earth.position.z - 76146.0};  // km (approximate)
  moon.velocity = {earth.velocity.x + 0.643, earth.velocity.y - 0.666,
                   earth.velocity.z - 0.137};  // km/s (approximate)
  moon.orbital_period = get_moon_orbital_period();
  state.bodies.push_back(moon);

  return state;
}

ReferenceState ReferenceData::get_current_state() {
  // This would typically be populated with current ephemeris data
  // For testing purposes, we'll use a simplified version
  ReferenceState state;
  state.date = "2025-01-01 00:00:00";
  state.julian_day = 2460676.5;

  // Simplified current state (would be replaced with actual ephemeris data)
  return get_j2000_state();  // Placeholder
}

TestScenario ReferenceData::get_earth_moon_scenario() {
  TestScenario scenario;
  scenario.name = "Earth-Moon System";
  scenario.description = "Two-body gravitational interaction between Earth and Moon";

  // Initial state: Earth at origin, Moon in circular orbit
  scenario.initial_state.date = "2025-01-01 00:00:00";
  scenario.initial_state.julian_day = 2460676.5;

  ReferenceBody earth;
  earth.name = "Earth";
  earth.mass = get_earth_mass();
  earth.radius = 6371.0;
  earth.position = {0.0, 0.0, 0.0};
  earth.velocity = {0.0, 0.0, 0.0};
  earth.orbital_period = 0.0;
  scenario.initial_state.bodies.push_back(earth);

  ReferenceBody moon;
  moon.name = "Moon";
  moon.mass = get_moon_mass();
  moon.radius = 1737.4;
  moon.position = {get_moon_orbital_radius(), 0.0, 0.0};

  // Circular orbital velocity: v = sqrt(GM/r)
  double orbital_velocity = std::sqrt(get_gravitational_constant() * get_earth_mass() /
                                      (get_moon_orbital_radius() * 1000.0)) /
                            1000.0;  // km/s
  moon.velocity = {0.0, orbital_velocity, 0.0};
  moon.orbital_period = get_moon_orbital_period();
  scenario.initial_state.bodies.push_back(moon);

  // Expected final state after one orbital period
  scenario.expected_final_state = scenario.initial_state;
  scenario.expected_final_state.date = "2025-01-28 07:43:12";  // ~27.32 days later

  scenario.simulation_duration_days = get_moon_orbital_period();
  scenario.time_step_seconds = 3600.0;      // 1 hour
  scenario.position_tolerance_km = 1000.0;  // 1000 km tolerance
  scenario.velocity_tolerance_kms = 0.1;    // 0.1 km/s tolerance

  return scenario;
}

TestScenario ReferenceData::get_sun_earth_moon_scenario() {
  TestScenario scenario;
  scenario.name = "Sun-Earth-Moon System";
  scenario.description = "Three-body gravitational interaction";

  scenario.initial_state = get_j2000_state();
  scenario.expected_final_state = scenario.initial_state;
  scenario.expected_final_state.date = "2000-02-01 12:00:00";  // 31 days later

  scenario.simulation_duration_days = 31.0;
  scenario.time_step_seconds = 3600.0;       // 1 hour
  scenario.position_tolerance_km = 10000.0;  // 10,000 km tolerance
  scenario.velocity_tolerance_kms = 0.5;     // 0.5 km/s tolerance

  return scenario;
}

TestScenario ReferenceData::get_solar_system_short_scenario() {
  TestScenario scenario;
  scenario.name = "Solar System Short Duration";
  scenario.description = "Full solar system simulation for 1 week";

  scenario.initial_state = get_j2000_state();
  scenario.expected_final_state = scenario.initial_state;
  scenario.expected_final_state.date = "2000-01-08 12:00:00";  // 7 days later

  scenario.simulation_duration_days = 7.0;
  scenario.time_step_seconds = 3600.0;       // 1 hour
  scenario.position_tolerance_km = 50000.0;  // 50,000 km tolerance
  scenario.velocity_tolerance_kms = 1.0;     // 1.0 km/s tolerance

  return scenario;
}

std::vector<TestScenario> ReferenceData::get_orbital_mechanics_scenarios() {
  std::vector<TestScenario> scenarios;

  // Add various orbital mechanics test cases
  scenarios.push_back(get_earth_moon_scenario());
  scenarios.push_back(get_sun_earth_moon_scenario());
  scenarios.push_back(get_solar_system_short_scenario());

  return scenarios;
}

// Data validation implementations
bool DataValidator::validate_energy_conservation(const ReferenceState& initial,
                                                 const ReferenceState& final,
                                                 double tolerance_percent) {
  // Calculate total energy for both states
  // This is a simplified implementation - would need full gravitational potential calculation
  double initial_kinetic = 0.0, final_kinetic = 0.0;

  for (const auto& body : initial.bodies) {
    double v_squared = body.velocity.x * body.velocity.x + body.velocity.y * body.velocity.y +
                       body.velocity.z * body.velocity.z;
    initial_kinetic += 0.5 * body.mass * v_squared * 1e6;  // Convert km²/s² to m²/s²
  }

  for (const auto& body : final.bodies) {
    double v_squared = body.velocity.x * body.velocity.x + body.velocity.y * body.velocity.y +
                       body.velocity.z * body.velocity.z;
    final_kinetic += 0.5 * body.mass * v_squared * 1e6;  // Convert km²/s² to m²/s²
  }

  double energy_change_percent =
      std::abs(final_kinetic - initial_kinetic) / initial_kinetic * 100.0;
  return energy_change_percent <= tolerance_percent;
}

bool DataValidator::validate_momentum_conservation(const ReferenceState& initial,
                                                   const ReferenceState& final,
                                                   double tolerance_percent) {
  // Calculate total momentum for both states
  SolarSystem::Math::Vector3d initial_momentum = {0.0, 0.0, 0.0};
  SolarSystem::Math::Vector3d final_momentum = {0.0, 0.0, 0.0};

  for (const auto& body : initial.bodies) {
    initial_momentum.x += body.mass * body.velocity.x;
    initial_momentum.y += body.mass * body.velocity.y;
    initial_momentum.z += body.mass * body.velocity.z;
  }

  for (const auto& body : final.bodies) {
    final_momentum.x += body.mass * body.velocity.x;
    final_momentum.y += body.mass * body.velocity.y;
    final_momentum.z += body.mass * body.velocity.z;
  }

  double initial_magnitude =
      std::sqrt(initial_momentum.x * initial_momentum.x + initial_momentum.y * initial_momentum.y +
                initial_momentum.z * initial_momentum.z);

  double final_magnitude =
      std::sqrt(final_momentum.x * final_momentum.x + final_momentum.y * final_momentum.y +
                final_momentum.z * final_momentum.z);

  if (initial_magnitude < 1e-10) return true;  // System at rest

  double momentum_change_percent =
      std::abs(final_magnitude - initial_magnitude) / initial_magnitude * 100.0;
  return momentum_change_percent <= tolerance_percent;
}

double DataValidator::calculate_position_error(const ReferenceState& computed,
                                               const ReferenceState& reference) {
  if (computed.bodies.size() != reference.bodies.size()) {
    return -1.0;  // Error: different number of bodies
  }

  double total_error = 0.0;
  for (size_t i = 0; i < computed.bodies.size(); ++i) {
    double dx = computed.bodies[i].position.x - reference.bodies[i].position.x;
    double dy = computed.bodies[i].position.y - reference.bodies[i].position.y;
    double dz = computed.bodies[i].position.z - reference.bodies[i].position.z;

    double error = std::sqrt(dx * dx + dy * dy + dz * dz);
    total_error += error;
  }

  return total_error / computed.bodies.size();  // Average error
}

double DataValidator::calculate_velocity_error(const ReferenceState& computed,
                                               const ReferenceState& reference) {
  if (computed.bodies.size() != reference.bodies.size()) {
    return -1.0;  // Error: different number of bodies
  }

  double total_error = 0.0;
  for (size_t i = 0; i < computed.bodies.size(); ++i) {
    double dvx = computed.bodies[i].velocity.x - reference.bodies[i].velocity.x;
    double dvy = computed.bodies[i].velocity.y - reference.bodies[i].velocity.y;
    double dvz = computed.bodies[i].velocity.z - reference.bodies[i].velocity.z;

    double error = std::sqrt(dvx * dvx + dvy * dvy + dvz * dvz);
    total_error += error;
  }

  return total_error / computed.bodies.size();  // Average error
}

// Mock JPL data generation for testing
std::string JPLDataValidator::generate_mock_jpl_response(const ReferenceBody& body,
                                                         const std::string& start_date,
                                                         const std::string&) {
  std::ostringstream response;

  response << "API VERSION: 1.2\n";
  response << "API SOURCE: NASA/JPL Horizons API\n";
  response << "\n";
  response << "*******************************************************************************\n";
  response << "Ephemeris / API_USER Mon Jun 30 10:00:00 2025 Pasadena, USA      / Horizons\n";
  response << "*******************************************************************************\n";
  response << "Target body name: " << body.name << "\n";
  response << "Center body name: Solar System Barycenter (0)\n";
  response << "Center-site name: BODY CENTER\n";
  response << "*******************************************************************************\n";
  response << "Initial IAU76/J2000 heliocentric ecliptic osculating elements (au, days):\n";
  response << "  EPOCH=  2451545.0 ! 2000-Jan-01.50 (TDB)         Residual RMS= .12345\n";
  response << "*******************************************************************************\n";
  response << "$$SOE\n";
  response << start_date << " 00:00:00.000,";
  response << std::fixed << std::setprecision(6);
  response << body.position.x << "," << body.position.y << "," << body.position.z << ",";
  response << body.velocity.x << "," << body.velocity.y << "," << body.velocity.z << ",\n";
  response << "$$EOE\n";
  response << "*******************************************************************************\n";

  return response.str();
}

}  // namespace TestData
