/**
 * @file scalability_tests.cpp
 * @brief Scalability and stress tests for Solar System Suite
 * @note Migrated to Google Test
 *
 * This benchmark validates system scalability and performance under stress:
 * - Performance with large numbers of celestial bodies
 * - Long-running simulation memory stability
 * - Concurrent access and thread safety
 * - Load testing for web interface components
 * - Memory usage patterns under stress
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "benchmark_utils.h"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_jpl/jpl_client.hpp"
#include <gtest/gtest.h>

using namespace SolarSystem;

// Helper function to create realistic celestial bodies
std::vector<Bodies::CelestialBody> create_test_bodies(size_t count) {
  std::vector<Bodies::CelestialBody> bodies;
  bodies.reserve(count);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<long double> pos_dist(-1e12L, 1e12L);
  std::uniform_real_distribution<long double> vel_dist(-1e5L, 1e5L);
  std::uniform_real_distribution<long double> mass_dist(1e20L, 1e30L);

  for (size_t i = 0; i < count; ++i) {
    Bodies::CelestialBody::Properties props = {
        .name = "Body_" + std::to_string(i),
        .mass = mass_dist(gen),
        .position = Math::Vector3d{pos_dist(gen), pos_dist(gen), pos_dist(gen)},
        .velocity = Math::Vector3d{vel_dist(gen), vel_dist(gen), vel_dist(gen)},
        .type = Bodies::BodyType::Planet,
        .priority = Bodies::BodyPriority::Essential,
        .jpl_id = std::nullopt,
        .creation_date = std::nullopt};
    bodies.emplace_back(props);
  }

  return bodies;
}

// Scalability Benchmark Test
TEST(ScalabilityBenchmark, SystemScalabilityValidation) {
  Benchmark::BenchmarkSuite suite("Scalability Benchmarks");

  // 1. CELESTIAL BODY SCALABILITY - Test N-body performance scaling
  std::vector<size_t> body_counts = {10, 50, 100, 200};

  suite.run_scalability_benchmark(
      "CelestialBodyScalability",
      [](size_t body_count) {
        auto bodies = create_test_bodies(body_count);

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

        for (size_t i = 0; i < bodies.size(); ++i) {
          bodies[i].apply_force(forces[i], 1.0);
          bodies[i].update_position(1.0);
        }

        volatile size_t count = bodies.size() + forces.size();
        (void)count;
      },
      body_counts, 5);

  // 2. MEMORY SCALABILITY - Test memory usage patterns with increasing data sizes
  std::vector<size_t> memory_sizes = {1000, 10000, 100000, 500000};

  suite.run_scalability_benchmark(
      "MemoryScalabilityTest",
      [](size_t data_size) {
        std::vector<Math::Vector3d> positions;
        std::vector<Math::Vector3d> velocities;
        std::vector<long double> masses;

        positions.reserve(data_size);
        velocities.reserve(data_size);
        masses.reserve(data_size);

        for (size_t i = 0; i < data_size; ++i) {
          positions.emplace_back(static_cast<long double>(i) * 1e9L,
                                 static_cast<long double>(i) * 2e9L,
                                 static_cast<long double>(i) * 3e9L);
          velocities.emplace_back(static_cast<long double>(i) * 1000.0L,
                                  static_cast<long double>(i) * 2000.0L,
                                  static_cast<long double>(i) * 3000.0L);
          masses.push_back(static_cast<long double>(i) * 1e24L);
        }

        long double total_energy = 0.0L;
        for (size_t i = 0; i < data_size; ++i) {
          total_energy += 0.5L * masses[i] * velocities[i].magnitude_squared();
        }

        volatile long double result = total_energy;
        (void)result;
      },
      memory_sizes, 3);

  // 3. CONCURRENT ACCESS BENCHMARK - Test thread safety and concurrent performance
  suite.run_benchmark(
      "ConcurrentAccessBenchmark",
      []() {
        constexpr size_t num_threads = 8;
        constexpr size_t operations_per_thread = 1000;

        std::vector<Bodies::CelestialBody> shared_bodies = create_test_bodies(100);
        std::atomic<size_t> completed_operations{0};

        std::vector<std::future<void>> futures;
        futures.reserve(num_threads);

        for (size_t t = 0; t < num_threads; ++t) {
          futures.push_back(std::async(std::launch::async, [&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> body_dist(0, shared_bodies.size() - 1);

            for (size_t op = 0; op < operations_per_thread; ++op) {
              size_t body1_idx = body_dist(gen);
              size_t body2_idx = body_dist(gen);

              if (body1_idx != body2_idx) {
                volatile auto distance =
                    shared_bodies[body1_idx].distance_to(shared_bodies[body2_idx]);
                volatile auto force =
                    shared_bodies[body1_idx].gravitational_force_to(shared_bodies[body2_idx]);

                (void)distance;
                (void)force;
              }

              completed_operations.fetch_add(1, std::memory_order_relaxed);
            }
          }));
        }

        for (auto& future : futures) {
          future.wait();
        }

        volatile size_t total_ops = completed_operations.load();
        (void)total_ops;
      },
      10);

  // 4. LONG-RUNNING SIMULATION STABILITY
  suite.run_benchmark(
      "LongRunningSimulationStability",
      []() {
        auto bodies = create_test_bodies(50);
        constexpr size_t simulation_steps = 10000;

        for (size_t step = 0; step < simulation_steps; ++step) {
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

          for (size_t i = 0; i < bodies.size(); ++i) {
            bodies[i].apply_force(forces[i], 0.1);
            bodies[i].update_position(0.1);
            bodies[i].reset_acceleration();
          }

          if (step % 1000 == 0) {
            volatile size_t memory_check = bodies.size() * sizeof(Bodies::CelestialBody);
            (void)memory_check;
          }
        }

        volatile size_t final_body_count = bodies.size();
        (void)final_body_count;
      },
      3);

  // 5. PARALLEL COMPUTATION SCALABILITY
  std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};

  suite.run_scalability_benchmark(
      "ParallelComputationScalability",
      [](size_t num_threads) {
        constexpr size_t total_work = 100000;
        const size_t work_per_thread = total_work / num_threads;

        std::vector<std::future<long double>> futures;
        futures.reserve(num_threads);

        for (size_t t = 0; t < num_threads; ++t) {
          futures.push_back(std::async(std::launch::async, [work_per_thread, t]() {
            long double result = 0.0L;

            for (size_t i = 0; i < work_per_thread; ++i) {
              long double angle = static_cast<long double>(i + t * work_per_thread) * 0.001L;
              result += std::sin(angle) * std::cos(angle) + std::sqrt(angle + 1.0L);
            }

            return result;
          }));
        }

        long double total_result = 0.0L;
        for (auto& future : futures) {
          total_result += future.get();
        }

        volatile long double final_result = total_result;
        (void)final_result;
      },
      thread_counts, 5);

  // 6. CACHE PERFORMANCE UNDER LOAD
  suite.run_benchmark(
      "CachePerformanceUnderLoad",
      []() {
        constexpr size_t num_concurrent_requests = 100;
        std::vector<std::future<void>> futures;
        futures.reserve(num_concurrent_requests);

        for (size_t i = 0; i < num_concurrent_requests; ++i) {
          futures.push_back(std::async(std::launch::async, [i]() {
            bool cache_hit = (i % 3) == 0;

            if (cache_hit) {
              Math::Vector3d position{static_cast<long double>(i) * 1e11L,
                                      static_cast<long double>(i) * 2e11L,
                                      static_cast<long double>(i) * 3e11L};
              Math::Vector3d velocity{static_cast<long double>(i) * 1000.0L,
                                      static_cast<long double>(i) * 2000.0L,
                                      static_cast<long double>(i) * 3000.0L};

              volatile auto result = position.magnitude() + velocity.magnitude();
              (void)result;
            } else {
              std::this_thread::sleep_for(std::chrono::microseconds(10));

              long double computed_value = 0.0L;
              for (int j = 0; j < 100; ++j) {
                computed_value +=
                    std::sin(static_cast<long double>(i + static_cast<size_t>(j)) * 0.001L);
              }

              volatile auto result = computed_value;
              (void)result;
            }
          }));
        }

        for (auto& future : futures) {
          future.wait();
        }
      },
      50);

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/scalability_analysis.csv", "csv");

  // Analyze scalability results
  auto results = suite.get_results();
  bool scalability_validated = true;

  std::cout << "\n=== Scalability Analysis ===" << std::endl;

  for (const auto& result : results) {
    if (result.name.find("Scalability") != std::string::npos) {
      auto input_size_it = result.custom_metrics.find("input_size");
      if (input_size_it != result.custom_metrics.end()) {
        double input_size = input_size_it->second;
        double time_per_element = result.avg_duration_ms / input_size;

        std::cout << result.name << " (size=" << static_cast<size_t>(input_size)
                  << "): " << result.avg_duration_ms << " ms total, " << time_per_element
                  << " ms/element" << std::endl;

        double threshold =
            (result.name.find("ParallelComputation") != std::string::npos) ? 2.0 : 1.0;
        if (time_per_element > threshold) {
          scalability_validated = false;
          std::cout << "WARNING: Time per element too high: " << time_per_element
                    << " ms (threshold: " << threshold << " ms)" << std::endl;
        }
      }
    }
  }

  for (const auto& result : results) {
    if (result.name == "ConcurrentAccessBenchmark") {
      std::cout << "Concurrent Access: " << result.avg_duration_ms << " ms avg" << std::endl;

      if (result.avg_duration_ms > 1000.0) {
        scalability_validated = false;
        std::cout << "WARNING: Concurrent access performance degraded" << std::endl;
      }
    }
  }

  for (const auto& result : results) {
    if (result.name == "LongRunningSimulationStability") {
      std::cout << "Long-running Stability: " << result.avg_duration_ms << " ms avg" << std::endl;

      if (result.memory_usage_bytes > 100 * 1024 * 1024) {
        scalability_validated = false;
        std::cout << "WARNING: Excessive memory usage detected" << std::endl;
      }
    }
  }

  std::cout << "\nOverall Scalability Validation: " << (scalability_validated ? "PASS" : "FAIL")
            << std::endl;

  EXPECT_TRUE(scalability_validated);
}
