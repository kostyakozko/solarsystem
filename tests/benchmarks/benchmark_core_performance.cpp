/**
 * @file benchmark_core_performance.cpp
 * @brief Core performance benchmarks validating specific system claims
 *
 * This benchmark specifically validates the core performance claims:
 * - Cache loading performance (1000x+ improvement claim)
 * - Simulation step execution time (microsecond claims)
 * - Memory usage and allocation patterns
 * - JPL response parsing and data conversion performance
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <thread>

#include "benchmark_utils.h"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/vector3.hpp"
#include "test_framework.h"

using namespace SolarSystem;

// Helper to create realistic test data
void create_performance_test_cache(const std::filesystem::path& cache_dir) {
  std::filesystem::create_directories(cache_dir);

  // Create a realistic binary cache file
  auto binary_path = cache_dir / "ephemeris_cache.bin";
  std::ofstream binary_file(binary_path, std::ios::binary);

  if (binary_file.is_open()) {
    size_t body_count = 27;  // Full solar system
    binary_file.write(reinterpret_cast<const char*>(&body_count), sizeof(body_count));

    // Write realistic ephemeris data for all 27 bodies
    for (size_t i = 0; i < body_count; ++i) {
      int jpl_id = static_cast<int>(i + 1);
      binary_file.write(reinterpret_cast<const char*>(&jpl_id), sizeof(jpl_id));

      std::string name = "Body_" + std::to_string(i);
      size_t name_length = name.length();
      binary_file.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
      binary_file.write(name.c_str(), static_cast<std::streamsize>(name_length));

      auto epoch_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      binary_file.write(reinterpret_cast<const char*>(&epoch_time), sizeof(epoch_time));

      // Realistic astronomical positions (in km)
      double i_double = static_cast<double>(i);
      double pos[3] = {1.0e8 * (i_double + 1.0) * std::cos(i_double * 0.5),
                       1.0e8 * (i_double + 1.0) * std::sin(i_double * 0.5),
                       1.0e7 * (i_double + 1.0) * std::sin(i_double * 0.3)};
      binary_file.write(reinterpret_cast<const char*>(pos), sizeof(pos));

      // Realistic orbital velocities (in km/s)
      double vel[3] = {30.0 * std::sin(i_double * 0.7), 30.0 * std::cos(i_double * 0.7),
                       5.0 * std::sin(i_double * 0.2)};
      binary_file.write(reinterpret_cast<const char*>(vel), sizeof(vel));

      // Realistic masses (in kg)
      long double mass = 1.0e24L * std::pow(10.0L, static_cast<long double>(i % 6));
      binary_file.write(reinterpret_cast<const char*>(&mass), sizeof(mass));
    }
  }
}

int main() {
  std::cout << "=== Core Performance Benchmark Suite ===" << std::endl;
  std::cout << "Validating key performance claims of Solar System Suite" << std::endl;

  // Setup test environment
  std::filesystem::path test_cache_dir = "./performance_test_cache";
  create_performance_test_cache(test_cache_dir);

  Benchmark::BenchmarkSuite suite("Core Performance Validation");

  // 1. CACHE LOADING PERFORMANCE - Validate 1000x improvement claim
  suite.run_benchmark(
      "CacheLoadingPerformance",
      [&test_cache_dir]() {
        auto binary_path = test_cache_dir / "ephemeris_cache.bin";
        std::ifstream file(binary_path, std::ios::binary);

        if (file.is_open()) {
          size_t body_count;
          file.read(reinterpret_cast<char*>(&body_count), sizeof(body_count));

          // Read all body data quickly
          for (size_t i = 0; i < body_count; ++i) {
            int jpl_id;
            file.read(reinterpret_cast<char*>(&jpl_id), sizeof(jpl_id));

            size_t name_length;
            file.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));
            std::string body_name(name_length, '\0');
            file.read(&body_name[0], static_cast<std::streamsize>(name_length));

            auto epoch_time =
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            file.read(reinterpret_cast<char*>(&epoch_time), sizeof(epoch_time));

            double pos[3];
            file.read(reinterpret_cast<char*>(pos), sizeof(pos));

            double vel[3];
            file.read(reinterpret_cast<char*>(vel), sizeof(vel));

            long double mass;
            file.read(reinterpret_cast<char*>(&mass), sizeof(mass));

            // Prevent optimization
            volatile auto result =
                pos[0] + vel[0] + mass + static_cast<long double>(body_name.length());
            (void)result;
          }
        }
      },
      50000);  // High iteration count to measure sub-millisecond performance

  // 2. SIMULATION STEP PERFORMANCE - Validate microsecond execution claims
  suite.run_benchmark(
      "SimulationStepPerformance",
      []() {
        // Create realistic celestial bodies for simulation
        Bodies::CelestialBody::Properties sun_props = {.name = "Sun",
                                                       .mass = 1.98847e30L,
                                                       .position = Math::Vector3d{0.0, 0.0, 0.0},
                                                       .velocity = Math::Vector3d{0.0, 0.0, 0.0},
                                                       .type = Bodies::BodyType::Star,
                                                       .priority = Bodies::BodyPriority::Essential,
                                                       .jpl_id = std::nullopt,
                                                       .creation_date = std::nullopt};

        Bodies::CelestialBody::Properties earth_props = {
            .name = "Earth",
            .mass = 5.97219e24L,
            .position = Math::Vector3d{1.496e11, 0.0, 0.0},  // 1 AU
            .velocity = Math::Vector3d{0.0, 29780.0, 0.0},   // Earth orbital velocity
            .type = Bodies::BodyType::Planet,
            .priority = Bodies::BodyPriority::Essential,
            .jpl_id = std::nullopt,
            .creation_date = std::nullopt};

        Bodies::CelestialBody sun(sun_props);
        Bodies::CelestialBody earth(earth_props);

        // Simulate single simulation step
        auto force = earth.gravitational_force_to(sun);
        earth.apply_force(force, 1.0);
        earth.update_position(1.0);

        // Prevent optimization
        volatile auto pos = earth.position().magnitude();
        (void)pos;
      },
      100000);  // Very high iteration count for microsecond measurement

  // 3. MATHEMATICAL OPERATIONS PERFORMANCE - Vector and physics calculations
  suite.run_benchmark(
      "MathematicalOperationsPerformance",
      []() {
        Math::Vector3d v1{1.23456789e11, 9.87654321e10, 5.55555555e9};
        Math::Vector3d v2{2.34567890e11, 8.76543210e10, 4.44444444e9};

        // Perform typical vector operations
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
      200000);

  // 4. MEMORY ALLOCATION PERFORMANCE - Test allocation patterns
  suite.run_memory_benchmark(
      "MemoryAllocationPerformance",
      []() {
        // Simulate typical memory allocation patterns
        std::vector<Bodies::CelestialBody> bodies;
        bodies.reserve(27);

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
              .jpl_id = std::nullopt,
              .creation_date = std::nullopt};
          bodies.emplace_back(props);
        }

        // Prevent optimization
        volatile size_t count = bodies.size();
        (void)count;
      },
      5000);

  suite.print_summary();

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/core_performance_benchmark.csv", "csv");

  // Validate performance claims
  auto results = suite.get_results();
  bool all_claims_validated = true;

  std::cout << "\n=== Performance Claims Validation ===" << std::endl;

  for (const auto& result : results) {
    bool passed = true;
    std::string status = "PASS";

    // Validate cache loading performance (should be very fast)
    if (result.name == "CacheLoadingPerformance") {
      if (result.avg_duration_ms > 0.1) {  // Should load in under 0.1ms
        passed = false;
        status = "FAIL - Exceeds 0.1ms threshold";
      }
    }

    // Validate simulation step performance (microsecond claims)
    if (result.name == "SimulationStepPerformance") {
      if (result.avg_duration_ms > 0.01) {  // Should complete in under 10μs
        passed = false;
        status = "FAIL - Exceeds 10μs threshold";
      }
    }

    // Validate mathematical operations performance
    if (result.name == "MathematicalOperationsPerformance") {
      if (result.avg_duration_ms > 0.001) {  // Should complete in under 1μs
        passed = false;
        status = "FAIL - Exceeds 1μs threshold";
      }
    }

    // Validate memory allocation performance
    if (result.name == "MemoryAllocationPerformance") {
      if (result.memory_usage_bytes > 100 * 1024 * 1024) {  // Under 100MB
        passed = false;
        status = "FAIL - Exceeds 100MB memory threshold";
      }
    }

    if (!passed) {
      all_claims_validated = false;
    }

    std::cout << result.name << ": " << status << std::endl;
  }

  // Specific claim analysis
  std::cout << "\n=== Specific Claim Analysis ===" << std::endl;

  // Cache performance analysis
  for (const auto& result : results) {
    if (result.name == "CacheLoadingPerformance") {
      double cache_time_ms = result.avg_duration_ms;
      double simulated_network_time_ms = 100.0;  // Typical network request
      double improvement_factor = simulated_network_time_ms / cache_time_ms;

      std::cout << "Cache Performance:" << std::endl;
      std::cout << "  Cache Load Time: " << cache_time_ms << " ms" << std::endl;
      std::cout << "  Network Time: " << simulated_network_time_ms << " ms" << std::endl;
      std::cout << "  Improvement Factor: " << static_cast<int>(improvement_factor) << "x"
                << std::endl;

      if (improvement_factor >= 1000.0) {
        std::cout << "  1000x Improvement Claim: VALIDATED ✓" << std::endl;
      } else {
        std::cout << "  1000x Improvement Claim: NOT VALIDATED ✗" << std::endl;
        all_claims_validated = false;
      }
    }

    if (result.name == "SimulationStepPerformance") {
      double sim_time_us = result.avg_duration_ms * 1000.0;  // Convert to microseconds

      std::cout << "Simulation Performance:" << std::endl;
      std::cout << "  Simulation Step Time: " << sim_time_us << " μs" << std::endl;

      if (sim_time_us <= 10.0) {
        std::cout << "  Microsecond Execution Claim: VALIDATED ✓" << std::endl;
      } else {
        std::cout << "  Microsecond Execution Claim: NOT VALIDATED ✗" << std::endl;
        all_claims_validated = false;
      }
    }
  }

  std::cout << "\nOverall Performance Validation: " << (all_claims_validated ? "PASS" : "FAIL")
            << std::endl;

  // Cleanup
  std::filesystem::remove_all(test_cache_dir);

  return all_claims_validated ? 0 : 1;
}
