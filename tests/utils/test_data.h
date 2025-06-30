/**
 * @file test_data.h
 * @brief Test data generation and validation utilities
 *
 * Provides known test cases, reference data, and validation utilities
 * for comprehensive testing of the Solar System Suite.
 */

#ifndef TEST_DATA_H
#define TEST_DATA_H

#include <map>
#include <string>
#include <vector>

#include "types.h"

namespace TestData {

/**
 * @brief Known celestial body data for validation
 */
struct ReferenceBody {
  std::string name;
  double mass;            // kg
  double radius;          // km
  coord position;         // km
  coord velocity;         // km/s
  double orbital_period;  // days (if applicable)
};

/**
 * @brief Reference solar system state for specific dates
 */
struct ReferenceState {
  std::string date;
  std::vector<ReferenceBody> bodies;
  double julian_day;
};

/**
 * @brief Test scenario for simulation validation
 */
struct TestScenario {
  std::string name;
  std::string description;
  ReferenceState initial_state;
  ReferenceState expected_final_state;
  double simulation_duration_days;
  double time_step_seconds;
  double position_tolerance_km;
  double velocity_tolerance_kms;
};

// Reference data providers
class ReferenceData {
 public:
  // Get reference state for J2000.0 epoch (2000-01-01 12:00:00 TT)
  static ReferenceState get_j2000_state();

  // Get reference state for current epoch (approximate)
  static ReferenceState get_current_state();

  // Get simplified two-body test case (Earth-Moon system)
  static TestScenario get_earth_moon_scenario();

  // Get three-body test case (Sun-Earth-Moon system)
  static TestScenario get_sun_earth_moon_scenario();

  // Get full solar system test case (short duration)
  static TestScenario get_solar_system_short_scenario();

  // Get orbital mechanics validation cases
  static std::vector<TestScenario> get_orbital_mechanics_scenarios();

  // Physical constants for validation
  static double get_gravitational_constant();  // m³/kg/s²
  static double get_au_in_km();                // Astronomical Unit in km
  static double get_earth_mass();              // kg
  static double get_sun_mass();                // kg
  static double get_moon_mass();               // kg

  // Orbital parameters for validation
  static double get_earth_orbital_period();  // days
  static double get_earth_orbital_radius();  // km (semi-major axis)
  static double get_moon_orbital_period();   // days
  static double get_moon_orbital_radius();   // km (semi-major axis)
};

// Test data generators
class TestDataGenerator {
 public:
  // Generate random but physically reasonable solar system state
  static ReferenceState generate_random_state(int num_bodies = 10);

  // Generate test case for specific orbital mechanics validation
  static TestScenario generate_circular_orbit_test(double radius_km, double central_mass_kg);

  // Generate test case for elliptical orbit validation
  static TestScenario generate_elliptical_orbit_test(double semi_major_axis_km, double eccentricity,
                                                     double central_mass_kg);

  // Generate stress test scenarios
  static std::vector<TestScenario> generate_stress_test_scenarios();

  // Generate performance test data
  static std::vector<ReferenceState> generate_performance_test_states(int num_states,
                                                                      int bodies_per_state);
};

// Data validation utilities
class DataValidator {
 public:
  // Validate physical constraints
  static bool validate_energy_conservation(const ReferenceState& initial,
                                           const ReferenceState& final,
                                           double tolerance_percent = 0.1);

  static bool validate_momentum_conservation(const ReferenceState& initial,
                                             const ReferenceState& final,
                                             double tolerance_percent = 0.1);

  static bool validate_angular_momentum_conservation(const ReferenceState& initial,
                                                     const ReferenceState& final,
                                                     double tolerance_percent = 0.1);

  // Validate orbital mechanics
  static bool validate_orbital_period(const ReferenceBody& body, double central_mass_kg,
                                      double tolerance_percent = 1.0);

  static bool validate_escape_velocity(const ReferenceBody& body, double central_mass_kg,
                                       double tolerance_percent = 1.0);

  // Validate numerical stability
  static bool validate_position_bounds(const ReferenceState& state, double max_distance_au = 100.0);

  static bool validate_velocity_bounds(const ReferenceState& state,
                                       double max_velocity_kms = 1000.0);

  // Statistical validation
  static double calculate_position_error(const ReferenceState& computed,
                                         const ReferenceState& reference);

  static double calculate_velocity_error(const ReferenceState& computed,
                                         const ReferenceState& reference);

  static std::map<std::string, double> calculate_error_statistics(
      const std::vector<ReferenceState>& computed, const std::vector<ReferenceState>& reference);
};

// JPL data validation
class JPLDataValidator {
 public:
  // Validate JPL data format and content
  static bool validate_jpl_response(const std::string& jpl_response);

  // Compare computed positions with JPL reference
  static double compare_with_jpl_data(const ReferenceState& computed_state,
                                      const std::string& jpl_data_file);

  // Validate ephemeris data consistency
  static bool validate_ephemeris_consistency(const std::string& cache_file);

  // Generate mock JPL data for testing
  static std::string generate_mock_jpl_response(const ReferenceBody& body,
                                                const std::string& start_date,
                                                const std::string& end_date);
};

}  // namespace TestData

#endif  // TEST_DATA_H
