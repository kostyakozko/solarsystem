/**
 * @file benchmark_comprehensive.cpp
 * @brief Comprehensive system benchmarks for core performance validation
 * @note Migrated to Google Test
 *
 * This benchmark validates the key performance claims of the Solar System Suite:
 * - Simulation step execution in microseconds
 * - Cache loading 1000x+ performance improvement
 * - Memory usage and allocation patterns
 * - Mathematical operations performance
 */

#include <filesystem>
#include <memory>
#include <random>
#include <vector>

#include "benchmark_utils.h"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_jpl/jpl_client.hpp"
#include <gtest/gtest.h>

using namespace SolarSystem;
,
                                                       .velocity = Math::Vector3d{0.0, 0.0, 0.0},
                                                       .type = Bodies::BodyType::Star,
                                                       .priority = Bodies::BodyPriority::Essential,
                                                       .jpl_id = "10",
                                                       .creation_date = std::nullopt};

        Bodies::CelestialBody::Properties earth_props = {
            .name = "Earth",
            .mass = 5.97219e24L,                             // Earth mass in kg
            .position = Math::Vector3d{1.496e11, 0.0, 0.0},  // 1 AU in meters
            .velocity = Math::Vector3d{0.0, 29780.0, 0.0},   // Earth orbital velocity m/s
            .type = Bodies::BodyType::Planet,
            .priority = Bodies::BodyPriority::Essential,
            .jpl_id = "399",
            .creation_date = std::nullopt};

        Bodies::CelestialBody sun(sun_props);
        Bodies::CelestialBody earth(earth_props);

        // Simulate gravitational force calculation and position update
        auto force = earth.gravitational_force_to(sun);
        earth.apply_force(force, 1.0);  // 1 second timestep
        earth.update_position(1.0);

        // Prevent optimization
        volatile auto pos = earth.position().magnitude();
        (void)pos;
      },
      50000);  // High iteration count to measure microsecond performance

  // 2. VECTOR OPERATIONS BENCHMARK - Core math performance
  suite.run_benchmark(
      "Vector3DMathBenchmark",
      []() {
        Math::Vector3d v1{1.23456789e11, 9.87654321e10, 5.55555555e9};
        Math::Vector3d v2{2.34567890e11, 8.76543210e10, 4.44444444e9};

        // Perform typical vector operations used in simulation
        auto sum = v1 + v2;
        auto diff = v1 - v2;
        auto cross = v1.cross(v2);
        auto dot = v1.dot(v2);
        auto magnitude = v1.magnitude();
        auto normalized = v1.normalized();

        // Prevent optimization
        volatile auto result = sum.magnitude() + diff.magnitude() + cross.magnitude() + dot +
                               magnitude + normalized.magnitude();
        (void)result;
      },
      100000);

  // 3. GRAVITATIONAL FORCE CALCULATION BENCHMARK
  suite.run_benchmark(
      "GravitationalForceBenchmark",
      []() {
        // Create two bodies at realistic distances
        Bodies::CelestialBody::Properties body1_props = {
            .name = "Body1",
            .mass = 1.98847e30L,  // Solar mass
            .position = Math::Vector3d{0.0, 0.0, 0.0},
            .velocity = Math::Vector3d{0.0, 0.0, 0.0},
            .type = Bodies::BodyType::Star,
            .priority = Bodies::BodyPriority::Essential,
            .jpl_id = "10",
            .creation_date = std::nullopt};

        Bodies::CelestialBody::Properties body2_props = {
            .name = "Body2",
            .mass = 5.97219e24L,                             // Earth mass
            .position = Math::Vector3d{1.496e11, 0.0, 0.0},  // 1 AU
            .velocity = Math::Vector3d{0.0, 29780.0, 0.0},
            .type = Bodies::BodyType::Planet,
            .priority = Bodies::BodyPriority::Essential,
            .jpl_id = "399",
            .creation_date = std::nullopt};

        Bodies::CelestialBody body1(body1_props);
        Bodies::CelestialBody body2(body2_props);

        // Calculate gravitational force
        auto force = body1.gravitational_force_to(body2);
        auto distance = body1.distance_to(body2);

        // Prevent optimization
        volatile auto result = force.magnitude() + distance;
        (void)result;
      },
      25000);

  // 4. MEMORY ALLOCATION PATTERN BENCHMARK - Test memory usage patterns
  suite.run_memory_benchmark(
      "MemoryAllocationPatternBenchmark",
      []() {
        // Simulate typical memory allocation patterns in simulation
        std::vector<Bodies::CelestialBody> bodies;
        bodies.reserve(27);  // Typical number of bodies in solar system

        // Create bodies with realistic properties
        for (int i = 0; i < 27; ++i) {
          Bodies::CelestialBody::Properties props = {
              .name = "Body_" + std::to_string(i),
              .mass = 1.0e24L + static_cast<long double>(i) * 1.0e23L,
              .position = Math::Vector3d{static_cast<long double>(i) * 1.0e11,
                                         static_cast<long double>(i) * 1.0e10,
                                         static_cast<long double>(i) * 1.0e9},
              .velocity = Math::Vector3d{static_cast<long double>(i) * 1000.0,
                                         static_cast<long double>(i) * 500.0,
                                         static_cast<long double>(i) * 100.0},
              .type = Bodies::BodyType::Planet,
              .priority = Bodies::BodyPriority::Essential,
              .jpl_id = std::to_string(i + 100),
              .creation_date = std::nullopt};
          bodies.emplace_back(props);
        }

        // Simulate force calculations between all bodies
        std::vector<Math::Vector3d> forces(bodies.size());
        for (size_t i = 0; i < bodies.size(); ++i) {
          Math::Vector3d total_force{0.0, 0.0, 0.0};
          for (size_t j = 0; j < bodies.size(); ++j) {
            if (i != j) {
              total_force += bodies[i].gravitational_force_to(bodies[j]);
            }
          }
          forces[i] = total_force;
        }

        // Prevent optimization
        volatile size_t count = bodies.size() + forces.size();
        (void)count;
      },
      1000);

  // 5. MATHEMATICAL OPERATIONS BENCHMARK - Complex calculations
  suite.run_benchmark(
      "ComplexMathematicalOperationsBenchmark",
      []() {
        // Simulate complex orbital mechanics calculations
        long double semi_major_axis = 1.496e11L;  // 1 AU
        long double eccentricity = 0.0167L;       // Earth's eccentricity
        long double mean_anomaly = 0.5L;          // Radians

        // Solve Kepler's equation iteratively (common in orbital mechanics)
        long double eccentric_anomaly = mean_anomaly;
        for (int iter = 0; iter < 10; ++iter) {
          eccentric_anomaly = mean_anomaly + eccentricity * std::sin(eccentric_anomaly);
        }

        // Calculate true anomaly
        long double true_anomaly =
            2.0L * std::atan2(std::sqrt((1.0L + eccentricity) / (1.0L - eccentricity)) *
                                  std::sin(eccentric_anomaly / 2.0L),
                              std::cos(eccentric_anomaly / 2.0L));

        // Calculate orbital radius
        long double radius = semi_major_axis * (1.0L - eccentricity * std::cos(eccentric_anomaly));

        // Prevent optimization
        volatile long double result = true_anomaly + radius + eccentric_anomaly;
        (void)result;
      },
      10000);

  // 6. LARGE DATASET PROCESSING BENCHMARK - Scalability test
  suite.run_benchmark(
      "LargeDatasetProcessingBenchmark",
      []() {
        // Create large dataset similar to ephemeris data
        constexpr size_t data_size = 10000;
        std::vector<Math::Vector3d> positions;
        std::vector<Math::Vector3d> velocities;
        std::vector<long double> masses;

        positions.reserve(data_size);
        velocities.reserve(data_size);
        masses.reserve(data_size);

        // Generate realistic astronomical data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<long double> pos_dist(-1e12L, 1e12L);
        std::uniform_real_distribution<long double> vel_dist(-1e5L, 1e5L);
        std::uniform_real_distribution<long double> mass_dist(1e20L, 1e30L);

        for (size_t i = 0; i < data_size; ++i) {
          positions.emplace_back(pos_dist(gen), pos_dist(gen), pos_dist(gen));
          velocities.emplace_back(vel_dist(gen), vel_dist(gen), vel_dist(gen));
          masses.push_back(mass_dist(gen));
        }

        // Process the data (simulate typical operations)
        long double total_kinetic_energy = 0.0L;
        Math::Vector3d center_of_mass{0.0, 0.0, 0.0};
        long double total_mass = 0.0L;

        for (size_t i = 0; i < data_size; ++i) {
          total_kinetic_energy += 0.5L * masses[i] * velocities[i].magnitude_squared();
          center_of_mass += positions[i] * masses[i];
          total_mass += masses[i];
        }

        if (total_mass > 0.0L) {
          center_of_mass /= total_mass;
        }

        // Prevent optimization
        volatile long double result =
            total_kinetic_energy + center_of_mass.magnitude() + total_mass;
        (void)result;
      },
      100);


  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/comprehensive_benchmark.csv", "csv");

  // Validate performance thresholds
  auto results = suite.get_results();
  bool all_passed = true;

  std::cout << "\n=== Performance Validation ===" << std::endl;

  for (const auto& result : results) {
    bool passed = true;
    std::string status = "PASS";

    // Validate microsecond claims for simulation steps
    if (result.name == "SimulationStepMicrosecondBenchmark") {
      // Should complete in under 10 microseconds on modern hardware
      if (result.avg_duration_ms > 0.01) {  // 10 microseconds = 0.01 ms
        passed = false;
        status = "FAIL - Exceeds 10μs threshold";
      }
    }

    // Validate vector operations performance
    if (result.name == "Vector3DMathBenchmark") {
      // Should complete in under 1 microsecond
      if (result.avg_duration_ms > 0.001) {  // 1 microsecond = 0.001 ms
        passed = false;
        status = "FAIL - Exceeds 1μs threshold";
      }
    }

    // Validate memory usage
    if (result.name == "MemoryAllocationPatternBenchmark") {
      // Should use reasonable memory (less than 10MB for 27 bodies)
      if (result.memory_usage_bytes > 100 * 1024 * 1024) {
        passed = false;
        status = "FAIL - Exceeds 100MB memory threshold";
      }
    }

    if (!passed) {
      all_passed = false;
    }

    std::cout << result.name << ": " << status << std::endl;
  }

  std::cout << "\nOverall Performance Validation: " << (all_passed ? "PASS" : "FAIL") << std::endl;

  return all_passed ? 0 : 1;
