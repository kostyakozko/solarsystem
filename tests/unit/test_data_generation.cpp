/**
 * @file test_data_generation.cpp
 * @brief Realistic test data generation system tests (Task 14)
 * @note Migrated to Google Test
 *
 * Tests data generation capabilities:
 * - Astronomical data generators with realistic characteristics
 * - Ephemeris data generation with proper orbital mechanics
 * - Configuration data generation with valid parameter ranges
 * - User input data generation with edge cases and invalid inputs
 *
 * Requirements: 5.1, 5.4
 */

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

/**
 * @brief Test data generation system
 */
class TestDataGenerator {
 public:
  // Astronomical body data
  struct AstronomicalBody {
    std::string name;
    double mass_kg;           // Mass in kilograms
    double radius_m;          // Radius in meters
    double orbital_period_s;  // Orbital period in seconds
    double semi_major_axis_m; // Semi-major axis in meters
    double eccentricity;      // Orbital eccentricity (0-1)
    double inclination_deg;   // Orbital inclination in degrees
  };

  // Ephemeris data point
  struct EphemerisData {
    double jd;           // Julian date
    double x, y, z;      // Position in meters
    double vx, vy, vz;   // Velocity in m/s
    std::string body_id; // Body identifier
  };

  // Configuration data
  struct ConfigurationData {
    double timestep_s;        // Simulation timestep in seconds
    double duration_s;        // Simulation duration in seconds
    std::string integrator;   // Integration method
    double tolerance;         // Numerical tolerance
    bool enable_relativity;   // Relativistic corrections
    int output_frequency;     // Output every N steps
  };

  // User input data
  struct UserInput {
    std::string command;
    std::vector<std::string> arguments;
    bool is_valid;
    std::string error_message;
  };

 private:
  std::mt19937 rng_;
  std::uniform_real_distribution<double> unit_dist_{0.0, 1.0};

 public:
  TestDataGenerator() : rng_(std::random_device{}()) {}

  explicit TestDataGenerator(unsigned int seed) : rng_(seed) {}

  // Generate realistic astronomical body
  AstronomicalBody generate_astronomical_body(const std::string& type) {
    AstronomicalBody body;

    if (type == "planet") {
      body.name = "TestPlanet_" + std::to_string(rng_());
      body.mass_kg = 1e24 + unit_dist_(rng_) * 1e27;  // Earth to Jupiter mass
      body.radius_m = 6e6 + unit_dist_(rng_) * 6e7;   // Earth to Jupiter radius
      body.orbital_period_s = 3e7 + unit_dist_(rng_) * 3e8;  // ~1-10 years
      body.semi_major_axis_m = 1e11 + unit_dist_(rng_) * 7e11; // 1-8 AU
      body.eccentricity = unit_dist_(rng_) * 0.2;     // Low eccentricity
      body.inclination_deg = unit_dist_(rng_) * 10.0; // Low inclination
    } else if (type == "moon") {
      body.name = "TestMoon_" + std::to_string(rng_());
      body.mass_kg = 1e20 + unit_dist_(rng_) * 1e23;  // Small moon to large moon
      body.radius_m = 1e5 + unit_dist_(rng_) * 2e6;   // 100km to 2000km
      body.orbital_period_s = 1e5 + unit_dist_(rng_) * 1e7;  // Days to months
      body.semi_major_axis_m = 1e8 + unit_dist_(rng_) * 1e9; // 100k to 1M km
      body.eccentricity = unit_dist_(rng_) * 0.1;     // Very low eccentricity
      body.inclination_deg = unit_dist_(rng_) * 5.0;  // Very low inclination
    } else if (type == "asteroid") {
      body.name = "TestAsteroid_" + std::to_string(rng_());
      body.mass_kg = 1e15 + unit_dist_(rng_) * 1e20;  // Small to large asteroid
      body.radius_m = 1e3 + unit_dist_(rng_) * 5e5;   // 1km to 500km
      body.orbital_period_s = 1e8 + unit_dist_(rng_) * 2e8;  // 3-6 years
      body.semi_major_axis_m = 2e11 + unit_dist_(rng_) * 3e11; // 2-5 AU
      body.eccentricity = unit_dist_(rng_) * 0.5;     // Higher eccentricity
      body.inclination_deg = unit_dist_(rng_) * 30.0; // Higher inclination
    } else {
      // Default: star
      body.name = "TestStar_" + std::to_string(rng_());
      body.mass_kg = 1e30 + unit_dist_(rng_) * 1e30;  // Solar mass range
      body.radius_m = 5e8 + unit_dist_(rng_) * 5e8;   // Solar radius range
      body.orbital_period_s = 0.0;                     // Stars don't orbit
      body.semi_major_axis_m = 0.0;
      body.eccentricity = 0.0;
      body.inclination_deg = 0.0;
    }

    return body;
  }

  // Generate ephemeris data with orbital mechanics
  EphemerisData generate_ephemeris_data(const AstronomicalBody& body,
                                        double jd) {
    EphemerisData data;
    data.jd = jd;
    data.body_id = body.name;

    if (body.orbital_period_s > 0) {
      // Calculate orbital position using simplified Kepler orbit
      double mean_anomaly =
          2.0 * M_PI * (jd - 2451545.0) * 86400.0 / body.orbital_period_s;
      double eccentric_anomaly = mean_anomaly; // Simplified (should iterate)

      // Position in orbital plane
      double a = body.semi_major_axis_m;
      double e = body.eccentricity;
      double r = a * (1.0 - e * std::cos(eccentric_anomaly));
      double theta = 2.0 * std::atan2(
          std::sqrt(1.0 + e) * std::sin(eccentric_anomaly / 2.0),
          std::sqrt(1.0 - e) * std::cos(eccentric_anomaly / 2.0));

      // Convert to Cartesian coordinates
      data.x = r * std::cos(theta);
      data.y = r * std::sin(theta);
      data.z = 0.0; // Simplified: ignore inclination

      // Velocity (simplified)
      double v = std::sqrt(1.989e30 * 6.674e-11 / r); // Vis-viva equation
      data.vx = -v * std::sin(theta);
      data.vy = v * std::cos(theta);
      data.vz = 0.0;
    } else {
      // Stationary body (star)
      data.x = data.y = data.z = 0.0;
      data.vx = data.vy = data.vz = 0.0;
    }

    return data;
  }

  // Generate valid configuration data
  ConfigurationData generate_valid_configuration() {
    ConfigurationData config;

    // Realistic timesteps: 1 second to 1 day
    config.timestep_s = std::pow(10.0, unit_dist_(rng_) * 5.0);

    // Duration: 1 day to 10 years
    config.duration_s = 86400.0 * (1.0 + unit_dist_(rng_) * 3650.0);

    // Random integrator
    std::vector<std::string> integrators = {"euler", "rk4", "leapfrog",
                                            "verlet"};
    config.integrator =
        integrators[rng_() % integrators.size()];

    // Tolerance: 1e-12 to 1e-6
    config.tolerance = std::pow(10.0, -12.0 + unit_dist_(rng_) * 6.0);

    // Random boolean
    config.enable_relativity = (unit_dist_(rng_) > 0.5);

    // Output frequency: 1 to 1000
    config.output_frequency = 1 + static_cast<int>(unit_dist_(rng_) * 999);

    return config;
  }

  // Generate configuration with edge cases
  ConfigurationData generate_edge_case_configuration(
      const std::string& edge_case) {
    ConfigurationData config = generate_valid_configuration();

    if (edge_case == "min_timestep") {
      config.timestep_s = 0.001; // Very small timestep
    } else if (edge_case == "max_timestep") {
      config.timestep_s = 86400.0; // One day timestep
    } else if (edge_case == "min_duration") {
      config.duration_s = 1.0; // One second
    } else if (edge_case == "max_duration") {
      config.duration_s = 3.156e9; // 100 years
    } else if (edge_case == "high_tolerance") {
      config.tolerance = 1e-3; // Low precision
    } else if (edge_case == "low_tolerance") {
      config.tolerance = 1e-15; // Very high precision
    }

    return config;
  }

  // Generate valid user input
  UserInput generate_valid_user_input(const std::string& command_type) {
    UserInput input;
    input.is_valid = true;

    if (command_type == "simulate") {
      input.command = "simulate";
      input.arguments = {"--timestep", "3600", "--duration", "86400"};
    } else if (command_type == "fetch") {
      input.command = "fetch";
      input.arguments = {"--body", "Earth", "--start", "2024-01-01"};
    } else if (command_type == "visualize") {
      input.command = "visualize";
      input.arguments = {"--port", "8080", "--bodies", "all"};
    } else {
      input.command = "help";
      input.arguments = {};
    }

    return input;
  }

  // Generate invalid user input
  UserInput generate_invalid_user_input(const std::string& error_type) {
    UserInput input;
    input.is_valid = false;

    if (error_type == "unknown_command") {
      input.command = "invalid_command_xyz";
      input.arguments = {};
      input.error_message = "Unknown command";
    } else if (error_type == "missing_argument") {
      input.command = "simulate";
      input.arguments = {"--timestep"}; // Missing value
      input.error_message = "Missing argument value";
    } else if (error_type == "invalid_value") {
      input.command = "simulate";
      input.arguments = {"--timestep", "not_a_number"};
      input.error_message = "Invalid numeric value";
    } else if (error_type == "negative_value") {
      input.command = "simulate";
      input.arguments = {"--timestep", "-100"};
      input.error_message = "Negative value not allowed";
    } else if (error_type == "empty_command") {
      input.command = "";
      input.arguments = {};
      input.error_message = "Empty command";
    }

    return input;
  }

  // Generate batch of astronomical bodies
  std::vector<AstronomicalBody> generate_solar_system(int num_planets,
                                                      int num_moons) {
    std::vector<AstronomicalBody> bodies;

    // Add star
    bodies.push_back(generate_astronomical_body("star"));

    // Add planets
    for (int i = 0; i < num_planets; ++i) {
      bodies.push_back(generate_astronomical_body("planet"));
    }

    // Add moons
    for (int i = 0; i < num_moons; ++i) {
      bodies.push_back(generate_astronomical_body("moon"));
    }

    return bodies;
  }

  // Generate time series of ephemeris data
  std::vector<EphemerisData> generate_ephemeris_time_series(
      const AstronomicalBody& body, double start_jd, double end_jd,
      double step_days) {
    std::vector<EphemerisData> series;

    for (double jd = start_jd; jd <= end_jd; jd += step_days) {
      series.push_back(generate_ephemeris_data(body, jd));
    }

    return series;
  }
};
  TEST_SUITE("Test Data Generation System Tests");

  // Test 1: Astronomical data generators
  TEST_CASE("Astronomical Data Generators") {
    TestDataGenerator generator(12345); // Fixed seed for reproducibility

    // Test 1.1: Generate planet data
    {
      auto planet = generator.generate_astronomical_body("planet");
      ASSERT_FALSE(planet.name.empty());
      EXPECT_GT(planet.mass_kg , = 1e24 && planet.mass_kg <= 1e27 + 1e24);
      EXPECT_GT(planet.radius_m , = 6e6 && planet.radius_m <= 6e7 + 6e6);
      EXPECT_GT(planet.eccentricity , = 0.0 && planet.eccentricity <= 0.2);
      EXPECT_GT(planet.inclination_deg , = 0.0 &&
                  planet.inclination_deg <= 10.0);
    }

    // Test 1.2: Generate moon data
    {
      auto moon = generator.generate_astronomical_body("moon");
      ASSERT_FALSE(moon.name.empty());
      EXPECT_GT(moon.mass_kg , = 1e20 && moon.mass_kg <= 1e23 + 1e20);
      EXPECT_GT(moon.radius_m , = 1e5 && moon.radius_m <= 2e6 + 1e5);
      EXPECT_GT(moon.eccentricity , = 0.0 && moon.eccentricity <= 0.1);
    }

    // Test 1.3: Generate asteroid data
    {
      auto asteroid = generator.generate_astronomical_body("asteroid");
      ASSERT_FALSE(asteroid.name.empty());
      EXPECT_GT(asteroid.mass_kg , = 1e15 && asteroid.mass_kg <= 1e20 + 1e15);
      EXPECT_GT(asteroid.eccentricity , = 0.0 && asteroid.eccentricity <= 0.5);
      EXPECT_GT(asteroid.inclination_deg , = 0.0 &&
                  asteroid.inclination_deg <= 30.0);
    }

    // Test 1.4: Generate star data
    {
      auto star = generator.generate_astronomical_body("star");
      ASSERT_FALSE(star.name.empty());
      EXPECT_GT(star.mass_kg , = 1e30);
      ASSERT_EQ(star.orbital_period_s, 0.0); // Stars don't orbit
      ASSERT_EQ(star.eccentricity, 0.0);
    }
  });

  // Test 2: Ephemeris data generation with orbital mechanics
  TEST_CASE("Ephemeris Data Generation") {
    TestDataGenerator generator(54321);

    // Test 2.1: Generate ephemeris for planet
    {
      auto planet = generator.generate_astronomical_body("planet");
      auto ephemeris = generator.generate_ephemeris_data(planet, 2451545.0);

      ASSERT_EQ(ephemeris.jd, 2451545.0);
      ASSERT_EQ(ephemeris.body_id, planet.name);
      ASSERT_TRUE(std::isfinite(ephemeris.x));
      ASSERT_TRUE(std::isfinite(ephemeris.y));
      ASSERT_TRUE(std::isfinite(ephemeris.z));
      ASSERT_TRUE(std::isfinite(ephemeris.vx));
      ASSERT_TRUE(std::isfinite(ephemeris.vy));
      ASSERT_TRUE(std::isfinite(ephemeris.vz));
    }

    // Test 2.2: Generate ephemeris for stationary body
    {
      auto star = generator.generate_astronomical_body("star");
      auto ephemeris = generator.generate_ephemeris_data(star, 2451545.0);

      ASSERT_EQ(ephemeris.x, 0.0);
      ASSERT_EQ(ephemeris.y, 0.0);
      ASSERT_EQ(ephemeris.z, 0.0);
      ASSERT_EQ(ephemeris.vx, 0.0);
      ASSERT_EQ(ephemeris.vy, 0.0);
      ASSERT_EQ(ephemeris.vz, 0.0);
    }

    // Test 2.3: Generate time series
    {
      auto planet = generator.generate_astronomical_body("planet");
      auto series = generator.generate_ephemeris_time_series(
          planet, 2451545.0, 2451555.0, 1.0);

      ASSERT_EQ(series.size(), 11); // 10 days + start
      ASSERT_EQ(series[0].jd, 2451545.0);
      ASSERT_EQ(series[10].jd, 2451555.0);

      // Verify continuity
      for (size_t i = 1; i < series.size(); ++i) {
        ASSERT_EQ(series[i].jd, series[i - 1].jd + 1.0);
      }
    }

    // Test 2.4: Verify orbital mechanics
    {
      auto planet = generator.generate_astronomical_body("planet");
      auto eph1 = generator.generate_ephemeris_data(planet, 2451545.0);
      auto eph2 = generator.generate_ephemeris_data(
          planet, 2451545.0 + planet.orbital_period_s / 86400.0);

      // After one orbital period, position should be similar
      double dx = eph1.x - eph2.x;
      double dy = eph1.y - eph2.y;
      double distance_diff = std::sqrt(dx * dx + dy * dy);
      double orbital_radius = planet.semi_major_axis_m;

      // Allow 10% difference due to simplified calculations
      ASSERT_LT(distance_diff, orbital_radius * 0.1);
    }
  });

  // Test 3: Configuration data generation
  TEST_CASE("Configuration Data Generation") {
    TestDataGenerator generator(99999);

    // Test 3.1: Generate valid configuration
    {
      auto config = generator.generate_valid_configuration();

      EXPECT_GT(config.timestep_s , 0.0);
      EXPECT_GT(config.duration_s , 0.0);
      ASSERT_FALSE(config.integrator.empty());
      EXPECT_GT(config.tolerance , 0.0 && config.tolerance < 1.0);
      EXPECT_GT(config.output_frequency , 0);
    }

    // Test 3.2: Generate multiple configurations
    {
      std::vector<std::string> integrators;
      for (int i = 0; i < 20; ++i) {
        auto config = generator.generate_valid_configuration();
        integrators.push_back(config.integrator);
      }

      // Should have variety in integrators
      std::sort(integrators.begin(), integrators.end());
      auto last = std::unique(integrators.begin(), integrators.end());
      integrators.erase(last, integrators.end());
      ASSERT_GT(integrators.size(), 1); // At least 2 different integrators
    }

    // Test 3.3: Edge case configurations
    {
      auto min_timestep =
          generator.generate_edge_case_configuration("min_timestep");
      ASSERT_EQ(min_timestep.timestep_s, 0.001);

      auto max_timestep =
          generator.generate_edge_case_configuration("max_timestep");
      ASSERT_EQ(max_timestep.timestep_s, 86400.0);

      auto low_tolerance =
          generator.generate_edge_case_configuration("low_tolerance");
      ASSERT_EQ(low_tolerance.tolerance, 1e-15);
    }
  });

  // Test 4: User input data generation
  TEST_CASE("User Input Data Generation") {
    TestDataGenerator generator(11111);

    // Test 4.1: Valid user inputs
    {
      auto simulate = generator.generate_valid_user_input("simulate");
      ASSERT_TRUE(simulate.is_valid);
      ASSERT_EQ(simulate.command, "simulate");
      ASSERT_FALSE(simulate.arguments.empty());

      auto fetch = generator.generate_valid_user_input("fetch");
      ASSERT_TRUE(fetch.is_valid);
      ASSERT_EQ(fetch.command, "fetch");

      auto visualize = generator.generate_valid_user_input("visualize");
      ASSERT_TRUE(visualize.is_valid);
      ASSERT_EQ(visualize.command, "visualize");
    }

    // Test 4.2: Invalid user inputs
    {
      auto unknown = generator.generate_invalid_user_input("unknown_command");
      ASSERT_FALSE(unknown.is_valid);
      ASSERT_FALSE(unknown.error_message.empty());

      auto missing = generator.generate_invalid_user_input("missing_argument");
      ASSERT_FALSE(missing.is_valid);
      ASSERT_EQ(missing.command, "simulate");

      auto invalid = generator.generate_invalid_user_input("invalid_value");
      ASSERT_FALSE(invalid.is_valid);
      EXPECT_NE(std::string::npos, invalid.error_message.find("Invalid"));

      auto negative = generator.generate_invalid_user_input("negative_value");
      ASSERT_FALSE(negative.is_valid);

      auto empty = generator.generate_invalid_user_input("empty_command");
      ASSERT_FALSE(empty.is_valid);
      ASSERT_TRUE(empty.command.empty());
    }
  });

  // Test 5: Batch data generation
  TEST_CASE("Batch Data Generation") {
    TestDataGenerator generator(77777);

    // Test 5.1: Generate solar system
    {
      auto system = generator.generate_solar_system(5, 10);
      ASSERT_EQ(system.size(), 16); // 1 star + 5 planets + 10 moons

      // First should be star
      ASSERT_EQ(system[0].orbital_period_s, 0.0);

      // Count body types
      int stars = 0, planets = 0, moons = 0;
      for (const auto& body : system) {
        if (body.orbital_period_s == 0.0)
          stars++;
        else if (body.mass_kg > 1e23)
          planets++;
        else
          moons++;
      }
      ASSERT_EQ(stars, 1);
      ASSERT_GE(planets, 1); // At least some planets
      ASSERT_GE(moons, 1);   // At least some moons
    }

    // Test 5.2: Generate large dataset
    {
      auto large_system = generator.generate_solar_system(20, 50);
      ASSERT_EQ(large_system.size(), 71); // 1 + 20 + 50

      // All should have valid data
      for (const auto& body : large_system) {
        ASSERT_FALSE(body.name.empty());
        ASSERT_GT(body.mass_kg, 0.0);
        ASSERT_GT(body.radius_m, 0.0);
      }
    }

    // Test 5.3: Generate ephemeris for multiple bodies
    {
      auto system = generator.generate_solar_system(3, 0);
      std::vector<std::vector<TestDataGenerator::EphemerisData>> all_ephemeris;

      for (const auto& body : system) {
        auto series = generator.generate_ephemeris_time_series(
            body, 2451545.0, 2451550.0, 1.0);
        all_ephemeris.push_back(series);
      }

      ASSERT_EQ(all_ephemeris.size(), 4); // 1 star + 3 planets
      for (const auto& series : all_ephemeris) {
        ASSERT_EQ(series.size(), 6); // 5 days + start
      }
    }
  });

  return current_suite->all_passed() ? 0 : 1;
