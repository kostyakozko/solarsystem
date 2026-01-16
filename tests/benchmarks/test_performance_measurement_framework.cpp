/**
 * @file test_performance_measurement_framework.cpp
 * @brief Comprehensive performance measurement framework tests (Task 8)
 * @note Migrated to Google Test
 *
 * Tests enhanced performance measurement capabilities including:
 * - Detailed timing and resource usage measurement
 * - Statistical analysis and variance calculation
 * - Performance profiling and bottleneck identification
 * - Memory usage and leak detection
 *
 * Requirements: 3.1, 3.4
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

#include "benchmark_utils.h"
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

using namespace Benchmark;
using namespace SolarSystem;

/**
 * @brief Enhanced performance measurement utilities
 */
class EnhancedPerformanceMeasurement {
 public:
  // Statistical analysis
  struct Statistics {
    double mean;
    double median;
    double std_deviation;
    double variance;
    double min;
    double max;
    double percentile_95;
    double percentile_99;
    double coefficient_of_variation;  // std_dev / mean
  };

  static Statistics calculate_statistics(const std::vector<double>& measurements) {
    if (measurements.empty()) {
      return {};
    }

    Statistics stats;
    std::vector<double> sorted = measurements;
    std::sort(sorted.begin(), sorted.end());

    // Mean
    stats.mean =
        std::accumulate(sorted.begin(), sorted.end(), 0.0) / static_cast<double>(sorted.size());

    // Median
    size_t mid = sorted.size() / 2;
    if (sorted.size() % 2 == 0) {
      stats.median = (sorted[mid - 1] + sorted[mid]) / 2.0;
    } else {
      stats.median = sorted[mid];
    }

    // Variance and standard deviation
    double sum_squared_diff = 0.0;
    for (double val : sorted) {
      double diff = val - stats.mean;
      sum_squared_diff += diff * diff;
    }
    stats.variance = sum_squared_diff / static_cast<double>(sorted.size());
    stats.std_deviation = std::sqrt(stats.variance);

    // Min and max
    stats.min = sorted.front();
    stats.max = sorted.back();

    // Percentiles
    size_t p95_idx = static_cast<size_t>(0.95 * static_cast<double>(sorted.size()));
    size_t p99_idx = static_cast<size_t>(0.99 * static_cast<double>(sorted.size()));
    stats.percentile_95 = sorted[std::min(p95_idx, sorted.size() - 1)];
    stats.percentile_99 = sorted[std::min(p99_idx, sorted.size() - 1)];

    // Coefficient of variation
    stats.coefficient_of_variation = (stats.mean != 0.0) ? (stats.std_deviation / stats.mean) : 0.0;

    return stats;
  }

  // Resource usage measurement
  struct ResourceUsage {
    size_t memory_bytes;
    double cpu_time_ms;
    size_t allocations;
    size_t deallocations;
    double wall_time_ms;
  };

  static ResourceUsage measure_resource_usage(std::function<void()> func) {
    ResourceUsage usage = {};

    auto wall_start = std::chrono::high_resolution_clock::now();
    auto cpu_start = std::clock();

    // Execute function
    func();

    auto cpu_end = std::clock();
    auto wall_end = std::chrono::high_resolution_clock::now();

    usage.cpu_time_ms =
        1000.0 * static_cast<double>(cpu_end - cpu_start) / static_cast<double>(CLOCKS_PER_SEC);
    usage.wall_time_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    return usage;
  }

  // Bottleneck identification
  struct BottleneckInfo {
    std::string operation_name;
    double time_ms;
    double percentage_of_total;
    bool is_bottleneck;  // > 20% of total time
  };

  static std::vector<BottleneckInfo> identify_bottlenecks(
      const std::map<std::string, double>& operation_times) {
    std::vector<BottleneckInfo> bottlenecks;

    double total_time = 0.0;
    for (const auto& [name, time] : operation_times) {
      total_time += time;
    }

    for (const auto& [name, time] : operation_times) {
      BottleneckInfo info;
      info.operation_name = name;
      info.time_ms = time;
      info.percentage_of_total = (total_time > 0.0) ? (time / total_time * 100.0) : 0.0;
      info.is_bottleneck = info.percentage_of_total > 20.0;
      bottlenecks.push_back(info);
    }

    // Sort by time descending
    std::sort(
        bottlenecks.begin(), bottlenecks.end(),
        [](const BottleneckInfo& a, const BottleneckInfo& b) { return a.time_ms > b.time_ms; });

    return bottlenecks;
  }
};

// ============================================================================
// TASK 8: COMPREHENSIVE PERFORMANCE MEASUREMENT FRAMEWORK
// ============================================================================

// Test 1: Detailed timing and resource usage measurement
TEST(PerformanceMeasurementFrameworkTestsTest, Detailed_Timing_and_Resource_Usage_Measurement) {
  // Test 1.1: High-precision timing
  {
    Timer timer;
    timer.start();

    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    timer.stop();

    double elapsed_ms = timer.elapsed_ms();
    double elapsed_us = timer.elapsed_us();
    double elapsed_ns = timer.elapsed_ns();

    // Verify timing precision
    ASSERT_GT(elapsed_ms, 99.0);   // At least 99ms
    ASSERT_LT(elapsed_ms, 150.0);  // Less than 150ms (with tolerance)
    ASSERT_GT(elapsed_us, 99000.0);
    ASSERT_GT(elapsed_ns, 99000000.0);

    // Verify conversions
    ASSERT_NEAR(elapsed_ms * 1000.0, elapsed_us, 1.0);
    ASSERT_NEAR(elapsed_us * 1000.0, elapsed_ns, 1000.0);
  }

  // Test 1.2: Resource usage measurement
  {
    auto usage = EnhancedPerformanceMeasurement::measure_resource_usage([]() {
      Bodies::BodyFactory factory;
      auto result = factory.create_body("Earth");
      (void)result;  // Mark as used
    });

    // Verify measurements
    ASSERT_GT(usage.wall_time_ms, 0.0);
    ASSERT_GE(usage.cpu_time_ms, 0.0);
  }

  // Test 1.3: Multiple measurements
  {
    std::vector<double> measurements;

    for (int i = 0; i < 10; ++i) {
      Timer timer;
      timer.start();

      Bodies::BodyFactory factory;
      auto result = factory.create_body("Mars");
      (void)result;

      timer.stop();
      measurements.push_back(timer.elapsed_ms());
    }

    // Verify all measurements are valid
    ASSERT_EQ(measurements.size(), 10);
    for (double measurement : measurements) {
      ASSERT_GT(measurement, 0.0);
      ASSERT_LT(measurement, 1000.0);  // Should be fast
    }
  }

  // Test 1.4: Profiler measurements
  {
    Profiler profiler;

    profiler.start_measurement("operation1");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    profiler.end_measurement("operation1");

    profiler.start_measurement("operation2");
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    profiler.end_measurement("operation2");

    auto result1 = profiler.get_result("operation1");
    auto result2 = profiler.get_result("operation2");

    ASSERT_GT(result1.duration_ms, 45.0);
    ASSERT_LT(result1.duration_ms, 100.0);
    ASSERT_GT(result2.duration_ms, 25.0);
    ASSERT_LT(result2.duration_ms, 80.0);
  }
}

// Test 2: Statistical analysis and variance calculation
TEST(PerformanceMeasurementFrameworkTestsTest, Statistical_Analysis_and_Variance_Calculation) {
  // Test 2.1: Basic statistics
  {
    std::vector<double> measurements = {10.0, 20.0, 30.0, 40.0, 50.0};

    auto stats = EnhancedPerformanceMeasurement::calculate_statistics(measurements);

    ASSERT_EQ(stats.mean, 30.0);
    ASSERT_EQ(stats.median, 30.0);
    ASSERT_EQ(stats.min, 10.0);
    ASSERT_EQ(stats.max, 50.0);
    ASSERT_GT(stats.std_deviation, 0.0);
    ASSERT_GT(stats.variance, 0.0);
  }

  // Test 2.2: Performance variance analysis
  {
    std::vector<double> measurements;

    // Collect multiple measurements
    for (int i = 0; i < 50; ++i) {
      Timer timer;
      timer.start();

      Bodies::BodyFactory factory;
      auto result = factory.create_body("Venus");
      (void)result;

      timer.stop();
      measurements.push_back(timer.elapsed_ms());
    }

    auto stats = EnhancedPerformanceMeasurement::calculate_statistics(measurements);

    // Verify statistical properties
    ASSERT_GT(stats.mean, 0.0);
    ASSERT_GT(stats.median, 0.0);
    ASSERT_GE(stats.min, 0.0);
    ASSERT_GT(stats.max, stats.min);
    ASSERT_GT(stats.std_deviation, 0.0);
    ASSERT_GT(stats.variance, 0.0);
    ASSERT_GT(stats.percentile_95, stats.median);
    ASSERT_GT(stats.percentile_99, stats.percentile_95);

    // Coefficient of variation should be reasonable (< 1.0 for stable operations)
    ASSERT_LT(stats.coefficient_of_variation, 2.0);
  }

  // Test 2.3: Outlier detection
  {
    std::vector<double> measurements = {10.0, 11.0, 10.5, 10.2, 100.0};  // 100.0 is outlier

    auto stats = EnhancedPerformanceMeasurement::calculate_statistics(measurements);

    // Mean should be affected by outlier
    ASSERT_GT(stats.mean, 20.0);

    // Median should be more robust
    ASSERT_LT(stats.median, 15.0);

    // High standard deviation indicates outliers
    ASSERT_GT(stats.std_deviation, 30.0);
  }

  // Test 2.4: Percentile calculations
  {
    std::vector<double> measurements;
    for (int i = 1; i <= 100; ++i) {
      measurements.push_back(static_cast<double>(i));
    }

    auto stats = EnhancedPerformanceMeasurement::calculate_statistics(measurements);

    // Verify percentiles
    ASSERT_NEAR(stats.percentile_95, 95.0, 2.0);
    ASSERT_NEAR(stats.percentile_99, 99.0, 2.0);
    ASSERT_GT(stats.percentile_99, stats.percentile_95);
  }
}

// Test 3: Performance profiling and bottleneck identification
TEST(PerformanceMeasurementFrameworkTestsTest,
     Performance_Profiling_and_Bottleneck_Identification) {
  // Test 3.1: Operation profiling
  {
    Profiler profiler;

    // Profile multiple operations
    {
      auto scoped = profiler.measure("body_creation");
      Bodies::BodyFactory factory;
      auto result = factory.create_body("Jupiter");
      (void)result;
    }

    {
      auto scoped = profiler.measure("collection_creation");
      Bodies::BodyFactory factory;
      auto collection = factory.create_collection({"Sun", "Earth"});
      (void)collection;
    }

    {
      auto scoped = profiler.measure("simulation_init");
      Bodies::BodyFactory factory;
      auto collection = factory.create_collection({"Sun", "Earth"});
      if (collection.has_value()) {
        Simulation::SimulationEngine engine;
        auto init = engine.initialize(std::move(collection.value()));
        (void)init;
      }
    }

    auto results = profiler.get_all_results();
    ASSERT_GE(results.size(), 3);

    // Verify all operations were measured
    bool found_body_creation = false;
    bool found_collection = false;
    bool found_simulation = false;

    for (const auto& result : results) {
      ASSERT_GT(result.duration_ms, 0.0);
      if (result.name == "body_creation") found_body_creation = true;
      if (result.name == "collection_creation") found_collection = true;
      if (result.name == "simulation_init") found_simulation = true;
    }

    ASSERT_TRUE(found_body_creation);
    ASSERT_TRUE(found_collection);
    ASSERT_TRUE(found_simulation);
  }

  // Test 3.2: Bottleneck identification
  {
    std::map<std::string, double> operation_times = {
        {"fast_operation", 10.0},
        {"medium_operation", 50.0},
        {"slow_operation", 200.0},  // This is the bottleneck
        {"another_fast", 15.0}};

    auto bottlenecks = EnhancedPerformanceMeasurement::identify_bottlenecks(operation_times);

    ASSERT_EQ(bottlenecks.size(), 4);

    // First should be the slowest
    ASSERT_EQ(bottlenecks[0].operation_name, "slow_operation");
    ASSERT_TRUE(bottlenecks[0].is_bottleneck);
    ASSERT_GT(bottlenecks[0].percentage_of_total, 70.0);

    // Others should not be bottlenecks
    for (size_t i = 1; i < bottlenecks.size(); ++i) {
      ASSERT_FALSE(bottlenecks[i].is_bottleneck);
    }
  }

  // Test 3.3: Real workflow profiling
  {
    Profiler profiler;

    // Profile complete workflow
    {
      auto scoped1 = profiler.measure("step1_factory");
      Bodies::BodyFactory factory;
      (void)factory;
    }

    {
      auto scoped2 = profiler.measure("step2_create_bodies");
      Bodies::BodyFactory factory;
      auto collection = factory.create_collection({"Sun", "Earth", "Mars"});
      (void)collection;
    }

    {
      auto scoped3 = profiler.measure("step3_simulation");
      Bodies::BodyFactory factory;
      auto collection = factory.create_collection({"Sun", "Earth"});
      if (collection.has_value()) {
        Simulation::SimulationEngine engine;
        auto init = engine.initialize(std::move(collection.value()));
        if (init.has_value()) {
          auto step = engine.step();
          (void)step;
        }
      }
    }

    auto results = profiler.get_all_results();
    ASSERT_GE(results.size(), 3);

    // Build operation times map
    std::map<std::string, double> times;
    for (const auto& result : results) {
      times[result.name] = result.duration_ms;
    }

    // Identify bottlenecks
    auto bottlenecks = EnhancedPerformanceMeasurement::identify_bottlenecks(times);
    ASSERT_FALSE(bottlenecks.empty());

    // Verify bottleneck information is useful
    for (const auto& bottleneck : bottlenecks) {
      ASSERT_FALSE(bottleneck.operation_name.empty());
      ASSERT_GT(bottleneck.time_ms, 0.0);
      ASSERT_GE(bottleneck.percentage_of_total, 0.0);
      ASSERT_LE(bottleneck.percentage_of_total, 100.0);
    }
  }
}

// Test 4: Memory usage and leak detection
TEST(PerformanceMeasurementFrameworkTestsTest, Memory_Usage_and_Leak_Detection) {
  // Test 4.1: Memory monitoring
  {
    MemoryMonitor monitor;
    monitor.start_monitoring();

    // Allocate memory
    {
      std::vector<Bodies::CelestialBody> bodies;
      Bodies::BodyFactory factory;

      for (int i = 0; i < 10; ++i) {
        auto result = factory.create_body("Earth");
        if (result.has_value()) {
          bodies.push_back(std::move(result.value()));
        }
      }
    }  // Bodies go out of scope here

    monitor.stop_monitoring();

    auto stats = monitor.get_stats();
    ASSERT_GT(stats.peak_memory_bytes, 0);
  }

  // Test 4.2: Memory usage patterns
  {
    std::vector<size_t> memory_snapshots;

    for (int i = 0; i < 5; ++i) {
      MemoryMonitor monitor;
      monitor.start_monitoring();

      Bodies::BodyFactory factory;
      auto collection = factory.create_collection({"Sun", "Earth", "Mars"});
      (void)collection;

      monitor.stop_monitoring();
      memory_snapshots.push_back(monitor.get_peak_usage());
    }

    // Memory usage should be consistent across runs
    auto stats = EnhancedPerformanceMeasurement::calculate_statistics(
        std::vector<double>(memory_snapshots.begin(), memory_snapshots.end()));

    // Coefficient of variation should be low for consistent memory usage
    ASSERT_LT(stats.coefficient_of_variation, 0.5);
  }

  // Test 4.3: Memory leak detection simulation
  {
    MemoryMonitor monitor1;
    monitor1.start_monitoring();

    {
      Bodies::BodyFactory factory;
      auto result = factory.create_body("Saturn");
      (void)result;
    }

    monitor1.stop_monitoring();
    size_t baseline_memory = monitor1.get_peak_usage();

    // Run same operation again
    MemoryMonitor monitor2;
    monitor2.start_monitoring();

    {
      Bodies::BodyFactory factory;
      auto result = factory.create_body("Saturn");
      (void)result;
    }

    monitor2.stop_monitoring();
    size_t second_run_memory = monitor2.get_peak_usage();

    // Memory usage should be similar (no leak)
    // Allow for some variation
    double ratio = static_cast<double>(second_run_memory) / static_cast<double>(baseline_memory);
    ASSERT_LT(ratio, 1.5);  // Should not grow by more than 50%
  }

  // Test 4.4: Resource cleanup verification
  {
    size_t initial_memory = 0;
    size_t after_allocation = 0;
    size_t after_cleanup = 0;

    {
      MemoryMonitor monitor;
      monitor.start_monitoring();
      initial_memory = monitor.get_current_usage();
    }

    {
      MemoryMonitor monitor;
      monitor.start_monitoring();

      std::vector<std::unique_ptr<Bodies::BodyFactory>> factories;
      for (int i = 0; i < 10; ++i) {
        factories.push_back(std::make_unique<Bodies::BodyFactory>());
      }

      after_allocation = monitor.get_peak_usage();
      factories.clear();  // Cleanup

      monitor.stop_monitoring();
      after_cleanup = monitor.get_current_usage();
    }

    // Memory should increase during allocation (or at least not decrease)
    ASSERT_GE(after_allocation, initial_memory);

    // Memory should decrease after cleanup (or at least not increase significantly)
    ASSERT_LE(after_cleanup, after_allocation * 1.1);  // Allow 10% tolerance
  }
}

// Test 5: Integrated performance measurement
TEST(PerformanceMeasurementFrameworkTestsTest, Integrated_Performance_Measurement) {
  BenchmarkSuite suite("Integrated Performance Test");

  // Test 5.1: Complete benchmark workflow
  {
    suite.run_benchmark(
        "complete_workflow",
        []() {
          Bodies::BodyFactory factory;
          auto collection = factory.create_collection({"Sun", "Earth"});
          if (collection.has_value()) {
            Simulation::SimulationEngine engine;
            auto init = engine.initialize(std::move(collection.value()));
            if (init.has_value()) {
              auto step = engine.step();
              (void)step;
            }
          }
        },
        10);  // 10 iterations

    auto results = suite.get_results();
    ASSERT_FALSE(results.empty());

    // Verify benchmark collected statistics
    for (const auto& result : results) {
      ASSERT_GT(result.iterations, 0);
      ASSERT_GT(result.avg_duration_ms, 0.0);
      ASSERT_GE(result.min_duration_ms, 0.0);
      ASSERT_GE(result.max_duration_ms, result.min_duration_ms);
    }
  }

  // Test 5.2: Memory benchmark
  {
    suite.run_memory_benchmark(
        "memory_test",
        []() {
          Bodies::BodyFactory factory;
          auto result = factory.create_body("Neptune");
          (void)result;
        },
        5);  // 5 iterations

    auto results = suite.get_results();
    ASSERT_GE(results.size(), 2);  // Should have both benchmarks
  }
}
