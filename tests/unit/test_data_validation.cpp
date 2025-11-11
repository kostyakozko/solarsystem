/**
 * @file test_data_validation.cpp
 * @brief Comprehensive test data validation system tests (Task 15)
 *
 * Tests data validation capabilities:
 * - Data consistency and integrity checking
 * - Format validation for all data types
 * - Cross-reference validation between related data
 * - Data quality assessment and reporting
 *
 * Requirements: 5.3
 */

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "test_framework.h"

/**
 * @brief Test data validation system
 */
class TestDataValidator {
 public:
  // Validation result
  struct ValidationResult {
    bool is_valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    double quality_score = 1.0; // 0.0 to 1.0
    std::string summary;
  };

  // Astronomical body data (matching generator)
  struct AstronomicalBody {
    std::string name;
    double mass_kg;
    double radius_m;
    double orbital_period_s;
    double semi_major_axis_m;
    double eccentricity;
    double inclination_deg;
  };

  // Ephemeris data point
  struct EphemerisData {
    double jd;
    double x, y, z;
    double vx, vy, vz;
    std::string body_id;
  };

  // Configuration data
  struct ConfigurationData {
    double timestep_s;
    double duration_s;
    std::string integrator;
    double tolerance;
    bool enable_relativity;
    int output_frequency;
  };

  // Validate astronomical body data
  ValidationResult validate_astronomical_body(const AstronomicalBody& body) {
    ValidationResult result;

    // Name validation
    if (body.name.empty()) {
      result.is_valid = false;
      result.errors.push_back("Body name is empty");
    }

    // Mass validation
    if (body.mass_kg <= 0.0) {
      result.is_valid = false;
      result.errors.push_back("Mass must be positive");
    } else if (body.mass_kg < 1e10) {
      result.warnings.push_back("Mass is very small (< 1e10 kg)");
      result.quality_score *= 0.9;
    } else if (body.mass_kg > 1e32) {
      result.warnings.push_back("Mass is very large (> 1e32 kg)");
      result.quality_score *= 0.9;
    }

    // Radius validation
    if (body.radius_m <= 0.0) {
      result.is_valid = false;
      result.errors.push_back("Radius must be positive");
    } else if (body.radius_m < 1e3) {
      result.warnings.push_back("Radius is very small (< 1 km)");
      result.quality_score *= 0.95;
    }

    // Orbital parameters validation
    if (body.orbital_period_s < 0.0) {
      result.is_valid = false;
      result.errors.push_back("Orbital period cannot be negative");
    }

    if (body.semi_major_axis_m < 0.0) {
      result.is_valid = false;
      result.errors.push_back("Semi-major axis cannot be negative");
    }

    // Eccentricity validation
    if (body.eccentricity < 0.0 || body.eccentricity >= 1.0) {
      result.is_valid = false;
      result.errors.push_back("Eccentricity must be in range [0, 1)");
    } else if (body.eccentricity > 0.9) {
      result.warnings.push_back("Very high eccentricity (> 0.9)");
      result.quality_score *= 0.95;
    }

    // Inclination validation
    if (body.inclination_deg < 0.0 || body.inclination_deg > 180.0) {
      result.is_valid = false;
      result.errors.push_back("Inclination must be in range [0, 180] degrees");
    }

    // Consistency checks
    if (body.orbital_period_s > 0.0 && body.semi_major_axis_m == 0.0) {
      result.warnings.push_back(
          "Body has orbital period but zero semi-major axis");
      result.quality_score *= 0.8;
    }

    result.summary = result.is_valid ? "Valid astronomical body"
                                     : "Invalid astronomical body";
    return result;
  }

  // Validate ephemeris data
  ValidationResult validate_ephemeris_data(const EphemerisData& data) {
    ValidationResult result;

    // Julian date validation
    if (data.jd < 2400000.0 || data.jd > 2500000.0) {
      result.warnings.push_back("Julian date outside typical range");
      result.quality_score *= 0.9;
    }

    // Body ID validation
    if (data.body_id.empty()) {
      result.is_valid = false;
      result.errors.push_back("Body ID is empty");
    }

    // Position validation
    if (!std::isfinite(data.x) || !std::isfinite(data.y) ||
        !std::isfinite(data.z)) {
      result.is_valid = false;
      result.errors.push_back("Position contains non-finite values");
    }

    // Velocity validation
    if (!std::isfinite(data.vx) || !std::isfinite(data.vy) ||
        !std::isfinite(data.vz)) {
      result.is_valid = false;
      result.errors.push_back("Velocity contains non-finite values");
    }

    // Magnitude checks
    double pos_magnitude =
        std::sqrt(data.x * data.x + data.y * data.y + data.z * data.z);
    double vel_magnitude =
        std::sqrt(data.vx * data.vx + data.vy * data.vy + data.vz * data.vz);

    if (pos_magnitude > 1e13) { // Beyond solar system
      result.warnings.push_back("Position magnitude very large");
      result.quality_score *= 0.9;
    }

    if (vel_magnitude > 1e6) { // Faster than escape velocity
      result.warnings.push_back("Velocity magnitude very large");
      result.quality_score *= 0.9;
    }

    result.summary =
        result.is_valid ? "Valid ephemeris data" : "Invalid ephemeris data";
    return result;
  }

  // Validate configuration data
  ValidationResult validate_configuration(const ConfigurationData& config) {
    ValidationResult result;

    // Timestep validation
    if (config.timestep_s <= 0.0) {
      result.is_valid = false;
      result.errors.push_back("Timestep must be positive");
    } else if (config.timestep_s < 0.01) {
      result.warnings.push_back("Very small timestep (< 0.01 s)");
      result.quality_score *= 0.95;
    } else if (config.timestep_s > 1e6) {
      result.warnings.push_back("Very large timestep (> 1e6 s)");
      result.quality_score *= 0.9;
    }

    // Duration validation
    if (config.duration_s <= 0.0) {
      result.is_valid = false;
      result.errors.push_back("Duration must be positive");
    } else if (config.duration_s < config.timestep_s) {
      result.warnings.push_back("Duration less than timestep");
      result.quality_score *= 0.8;
    }

    // Integrator validation
    std::set<std::string> valid_integrators = {"euler", "rk4", "leapfrog",
                                                "verlet", "rk45"};
    if (config.integrator.empty()) {
      result.is_valid = false;
      result.errors.push_back("Integrator name is empty");
    } else if (valid_integrators.find(config.integrator) ==
               valid_integrators.end()) {
      result.warnings.push_back("Unknown integrator: " + config.integrator);
      result.quality_score *= 0.9;
    }

    // Tolerance validation
    if (config.tolerance <= 0.0) {
      result.is_valid = false;
      result.errors.push_back("Tolerance must be positive");
    } else if (config.tolerance > 1e-3) {
      result.warnings.push_back("Large tolerance (> 1e-3)");
      result.quality_score *= 0.95;
    } else if (config.tolerance < 1e-16) {
      result.warnings.push_back("Very small tolerance (< 1e-16)");
      result.quality_score *= 0.95;
    }

    // Output frequency validation
    if (config.output_frequency <= 0) {
      result.is_valid = false;
      result.errors.push_back("Output frequency must be positive");
    }

    result.summary =
        result.is_valid ? "Valid configuration" : "Invalid configuration";
    return result;
  }

  // Validate cross-references between bodies and ephemeris
  ValidationResult validate_cross_references(
      const std::vector<AstronomicalBody>& bodies,
      const std::vector<EphemerisData>& ephemeris_list) {
    ValidationResult result;

    // Build set of body names
    std::set<std::string> body_names;
    for (const auto& body : bodies) {
      body_names.insert(body.name);
    }

    // Check that all ephemeris data references valid bodies
    std::set<std::string> referenced_bodies;
    for (const auto& eph : ephemeris_list) {
      referenced_bodies.insert(eph.body_id);

      if (body_names.find(eph.body_id) == body_names.end()) {
        result.errors.push_back("Ephemeris references unknown body: " +
                                eph.body_id);
        result.is_valid = false;
      }
    }

    // Check for bodies without ephemeris data
    for (const auto& name : body_names) {
      if (referenced_bodies.find(name) == referenced_bodies.end()) {
        result.warnings.push_back("Body has no ephemeris data: " + name);
        result.quality_score *= 0.95;
      }
    }

    result.summary = result.is_valid ? "Valid cross-references"
                                     : "Invalid cross-references";
    return result;
  }

  // Validate time series consistency
  ValidationResult validate_time_series(
      const std::vector<EphemerisData>& series) {
    ValidationResult result;

    if (series.empty()) {
      result.warnings.push_back("Empty time series");
      result.quality_score = 0.5;
      result.summary = "Empty time series";
      return result;
    }

    // Check time ordering
    for (size_t i = 1; i < series.size(); ++i) {
      if (series[i].jd <= series[i - 1].jd) {
        result.errors.push_back("Time series not monotonically increasing");
        result.is_valid = false;
        break;
      }
    }

    // Check for consistent body ID
    std::string first_body = series[0].body_id;
    for (const auto& data : series) {
      if (data.body_id != first_body) {
        result.errors.push_back("Inconsistent body IDs in time series");
        result.is_valid = false;
        break;
      }
    }

    // Check for reasonable time spacing
    if (series.size() > 1) {
      double min_spacing = series[1].jd - series[0].jd;
      double max_spacing = min_spacing;

      for (size_t i = 2; i < series.size(); ++i) {
        double spacing = series[i].jd - series[i - 1].jd;
        min_spacing = std::min(min_spacing, spacing);
        max_spacing = std::max(max_spacing, spacing);
      }

      if (max_spacing > min_spacing * 10.0) {
        result.warnings.push_back("Irregular time spacing in series");
        result.quality_score *= 0.9;
      }
    }

    result.summary =
        result.is_valid ? "Valid time series" : "Invalid time series";
    return result;
  }

  // Generate comprehensive quality report
  std::string generate_quality_report(
      const std::vector<ValidationResult>& results) {
    std::string report = "=== Data Quality Report ===\n";

    int total = static_cast<int>(results.size());
    int valid = 0;
    int with_warnings = 0;
    double avg_quality = 0.0;

    for (const auto& result : results) {
      if (result.is_valid) valid++;
      if (!result.warnings.empty()) with_warnings++;
      avg_quality += result.quality_score;
    }

    if (total > 0) {
      avg_quality /= total;
    }

    report += "Total validations: " + std::to_string(total) + "\n";
    report += "Valid: " + std::to_string(valid) + "\n";
    report += "Invalid: " + std::to_string(total - valid) + "\n";
    report += "With warnings: " + std::to_string(with_warnings) + "\n";
    report += "Average quality score: " + std::to_string(avg_quality) + "\n";

    return report;
  }
};

int main() {
  TEST_SUITE("Test Data Validation System Tests");

  // Test 1: Data consistency and integrity checking
  TEST_CASE("Data Consistency and Integrity Checking") {
    TestDataValidator validator;

    // Test 1.1: Valid astronomical body
    {
      TestDataValidator::AstronomicalBody body;
      body.name = "Earth";
      body.mass_kg = 5.972e24;
      body.radius_m = 6.371e6;
      body.orbital_period_s = 31557600.0;
      body.semi_major_axis_m = 1.496e11;
      body.eccentricity = 0.0167;
      body.inclination_deg = 0.0;

      auto result = validator.validate_astronomical_body(body);
      ASSERT_TRUE(result.is_valid);
      ASSERT_TRUE(result.errors.empty());
      ASSERT_EQ(result.quality_score, 1.0);
    }

    // Test 1.2: Invalid astronomical body (negative mass)
    {
      TestDataValidator::AstronomicalBody body;
      body.name = "Invalid";
      body.mass_kg = -1000.0;
      body.radius_m = 1000.0;
      body.orbital_period_s = 0.0;
      body.semi_major_axis_m = 0.0;
      body.eccentricity = 0.0;
      body.inclination_deg = 0.0;

      auto result = validator.validate_astronomical_body(body);
      ASSERT_FALSE(result.is_valid);
      ASSERT_FALSE(result.errors.empty());
      ASSERT_TRUE(result.errors[0].find("positive") != std::string::npos);
    }

    // Test 1.3: Body with warnings
    {
      TestDataValidator::AstronomicalBody body;
      body.name = "HighEccentricity";
      body.mass_kg = 1e24;
      body.radius_m = 1e6;
      body.orbital_period_s = 1e7;
      body.semi_major_axis_m = 1e11;
      body.eccentricity = 0.95; // Very high
      body.inclination_deg = 5.0;

      auto result = validator.validate_astronomical_body(body);
      ASSERT_TRUE(result.is_valid);
      ASSERT_FALSE(result.warnings.empty());
      ASSERT_LT(result.quality_score, 1.0);
    }

    // Test 1.4: Invalid eccentricity
    {
      TestDataValidator::AstronomicalBody body;
      body.name = "BadEccentricity";
      body.mass_kg = 1e24;
      body.radius_m = 1e6;
      body.orbital_period_s = 1e7;
      body.semi_major_axis_m = 1e11;
      body.eccentricity = 1.5; // Invalid
      body.inclination_deg = 5.0;

      auto result = validator.validate_astronomical_body(body);
      ASSERT_FALSE(result.is_valid);
      ASSERT_TRUE(result.errors[0].find("Eccentricity") != std::string::npos);
    }
  });

  // Test 2: Format validation for all data types
  TEST_CASE("Format Validation") {
    TestDataValidator validator;

    // Test 2.1: Valid ephemeris data
    {
      TestDataValidator::EphemerisData data;
      data.jd = 2451545.0;
      data.x = 1.496e11;
      data.y = 0.0;
      data.z = 0.0;
      data.vx = 0.0;
      data.vy = 29780.0;
      data.vz = 0.0;
      data.body_id = "Earth";

      auto result = validator.validate_ephemeris_data(data);
      ASSERT_TRUE(result.is_valid);
      ASSERT_TRUE(result.errors.empty());
    }

    // Test 2.2: Invalid ephemeris (non-finite values)
    {
      TestDataValidator::EphemerisData data;
      data.jd = 2451545.0;
      data.x = std::numeric_limits<double>::infinity();
      data.y = 0.0;
      data.z = 0.0;
      data.vx = 0.0;
      data.vy = 0.0;
      data.vz = 0.0;
      data.body_id = "Invalid";

      auto result = validator.validate_ephemeris_data(data);
      ASSERT_FALSE(result.is_valid);
      ASSERT_TRUE(result.errors[0].find("non-finite") != std::string::npos);
    }

    // Test 2.3: Valid configuration
    {
      TestDataValidator::ConfigurationData config;
      config.timestep_s = 3600.0;
      config.duration_s = 86400.0;
      config.integrator = "rk4";
      config.tolerance = 1e-9;
      config.enable_relativity = false;
      config.output_frequency = 10;

      auto result = validator.validate_configuration(config);
      ASSERT_TRUE(result.is_valid);
      ASSERT_TRUE(result.errors.empty());
    }

    // Test 2.4: Invalid configuration (negative timestep)
    {
      TestDataValidator::ConfigurationData config;
      config.timestep_s = -100.0;
      config.duration_s = 86400.0;
      config.integrator = "rk4";
      config.tolerance = 1e-9;
      config.enable_relativity = false;
      config.output_frequency = 10;

      auto result = validator.validate_configuration(config);
      ASSERT_FALSE(result.is_valid);
      ASSERT_TRUE(result.errors[0].find("positive") != std::string::npos);
    }
  });

  // Test 3: Cross-reference validation
  TEST_CASE("Cross-Reference Validation") {
    TestDataValidator validator;

    // Test 3.1: Valid cross-references
    {
      std::vector<TestDataValidator::AstronomicalBody> bodies;
      TestDataValidator::AstronomicalBody earth;
      earth.name = "Earth";
      earth.mass_kg = 5.972e24;
      earth.radius_m = 6.371e6;
      earth.orbital_period_s = 31557600.0;
      earth.semi_major_axis_m = 1.496e11;
      earth.eccentricity = 0.0167;
      earth.inclination_deg = 0.0;
      bodies.push_back(earth);

      std::vector<TestDataValidator::EphemerisData> ephemeris;
      TestDataValidator::EphemerisData data;
      data.jd = 2451545.0;
      data.x = 1.496e11;
      data.y = 0.0;
      data.z = 0.0;
      data.vx = 0.0;
      data.vy = 29780.0;
      data.vz = 0.0;
      data.body_id = "Earth";
      ephemeris.push_back(data);

      auto result = validator.validate_cross_references(bodies, ephemeris);
      ASSERT_TRUE(result.is_valid);
      ASSERT_TRUE(result.errors.empty());
    }

    // Test 3.2: Invalid cross-reference (unknown body)
    {
      std::vector<TestDataValidator::AstronomicalBody> bodies;
      std::vector<TestDataValidator::EphemerisData> ephemeris;

      TestDataValidator::EphemerisData data;
      data.jd = 2451545.0;
      data.x = 0.0;
      data.y = 0.0;
      data.z = 0.0;
      data.vx = 0.0;
      data.vy = 0.0;
      data.vz = 0.0;
      data.body_id = "UnknownBody";
      ephemeris.push_back(data);

      auto result = validator.validate_cross_references(bodies, ephemeris);
      ASSERT_FALSE(result.is_valid);
      ASSERT_TRUE(result.errors[0].find("unknown body") != std::string::npos);
    }

    // Test 3.3: Warning for body without ephemeris
    {
      std::vector<TestDataValidator::AstronomicalBody> bodies;
      TestDataValidator::AstronomicalBody mars;
      mars.name = "Mars";
      mars.mass_kg = 6.39e23;
      mars.radius_m = 3.39e6;
      mars.orbital_period_s = 59355072.0;
      mars.semi_major_axis_m = 2.279e11;
      mars.eccentricity = 0.0934;
      mars.inclination_deg = 1.85;
      bodies.push_back(mars);

      std::vector<TestDataValidator::EphemerisData> ephemeris;

      auto result = validator.validate_cross_references(bodies, ephemeris);
      ASSERT_TRUE(result.is_valid);
      ASSERT_FALSE(result.warnings.empty());
      ASSERT_LT(result.quality_score, 1.0);
    }
  });

  // Test 4: Time series validation
  TEST_CASE("Time Series Validation") {
    TestDataValidator validator;

    // Test 4.1: Valid time series
    {
      std::vector<TestDataValidator::EphemerisData> series;
      for (int i = 0; i < 5; ++i) {
        TestDataValidator::EphemerisData data;
        data.jd = 2451545.0 + i;
        data.x = 1.496e11;
        data.y = 0.0;
        data.z = 0.0;
        data.vx = 0.0;
        data.vy = 29780.0;
        data.vz = 0.0;
        data.body_id = "Earth";
        series.push_back(data);
      }

      auto result = validator.validate_time_series(series);
      ASSERT_TRUE(result.is_valid);
      ASSERT_TRUE(result.errors.empty());
    }

    // Test 4.2: Invalid time series (not monotonic)
    {
      std::vector<TestDataValidator::EphemerisData> series;
      TestDataValidator::EphemerisData data1;
      data1.jd = 2451545.0;
      data1.body_id = "Earth";
      data1.x = data1.y = data1.z = 0.0;
      data1.vx = data1.vy = data1.vz = 0.0;

      TestDataValidator::EphemerisData data2;
      data2.jd = 2451544.0; // Earlier time!
      data2.body_id = "Earth";
      data2.x = data2.y = data2.z = 0.0;
      data2.vx = data2.vy = data2.vz = 0.0;

      series.push_back(data1);
      series.push_back(data2);

      auto result = validator.validate_time_series(series);
      ASSERT_FALSE(result.is_valid);
      ASSERT_TRUE(result.errors[0].find("monotonically") != std::string::npos);
    }

    // Test 4.3: Inconsistent body IDs
    {
      std::vector<TestDataValidator::EphemerisData> series;
      TestDataValidator::EphemerisData data1;
      data1.jd = 2451545.0;
      data1.body_id = "Earth";
      data1.x = data1.y = data1.z = 0.0;
      data1.vx = data1.vy = data1.vz = 0.0;

      TestDataValidator::EphemerisData data2;
      data2.jd = 2451546.0;
      data2.body_id = "Mars"; // Different body!
      data2.x = data2.y = data2.z = 0.0;
      data2.vx = data2.vy = data2.vz = 0.0;

      series.push_back(data1);
      series.push_back(data2);

      auto result = validator.validate_time_series(series);
      ASSERT_FALSE(result.is_valid);
      ASSERT_TRUE(result.errors[0].find("Inconsistent") != std::string::npos);
    }

    // Test 4.4: Empty time series
    {
      std::vector<TestDataValidator::EphemerisData> series;
      auto result = validator.validate_time_series(series);
      ASSERT_FALSE(result.warnings.empty());
      ASSERT_EQ(result.quality_score, 0.5);
    }
  });

  // Test 5: Data quality assessment and reporting
  TEST_CASE("Data Quality Assessment and Reporting") {
    TestDataValidator validator;

    // Test 5.1: Generate quality report
    {
      std::vector<TestDataValidator::ValidationResult> results;

      // Add some valid results
      for (int i = 0; i < 7; ++i) {
        TestDataValidator::ValidationResult result;
        result.is_valid = true;
        result.quality_score = 1.0;
        results.push_back(result);
      }

      // Add some results with warnings
      for (int i = 0; i < 2; ++i) {
        TestDataValidator::ValidationResult result;
        result.is_valid = true;
        result.warnings.push_back("Some warning");
        result.quality_score = 0.9;
        results.push_back(result);
      }

      // Add an invalid result
      TestDataValidator::ValidationResult invalid;
      invalid.is_valid = false;
      invalid.errors.push_back("Some error");
      invalid.quality_score = 0.0;
      results.push_back(invalid);

      std::string report = validator.generate_quality_report(results);

      ASSERT_TRUE(report.find("Total validations: 10") != std::string::npos);
      ASSERT_TRUE(report.find("Valid: 9") != std::string::npos);
      ASSERT_TRUE(report.find("Invalid: 1") != std::string::npos);
      ASSERT_TRUE(report.find("With warnings: 2") != std::string::npos);
    }

    // Test 5.2: Quality score calculation
    {
      std::vector<TestDataValidator::ValidationResult> results;

      TestDataValidator::ValidationResult r1;
      r1.quality_score = 1.0;
      results.push_back(r1);

      TestDataValidator::ValidationResult r2;
      r2.quality_score = 0.8;
      results.push_back(r2);

      std::string report = validator.generate_quality_report(results);
      ASSERT_TRUE(report.find("0.9") != std::string::npos); // Average
    }

    // Test 5.3: Empty report
    {
      std::vector<TestDataValidator::ValidationResult> results;
      std::string report = validator.generate_quality_report(results);
      ASSERT_TRUE(report.find("Total validations: 0") != std::string::npos);
    }
  });

  return current_suite->all_passed() ? 0 : 1;
}
