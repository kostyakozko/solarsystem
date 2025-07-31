/**
 * @file benchmark_execution_demo.cpp
 * @brief Demonstration of the benchmark execution engine
 *
 * This example shows how to use the benchmark framework to measure performance
 * of various operations with statistical analysis and threshold validation.
 */

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;

/**
 * @brief Example benchmark for vector operations
 */
void demonstrate_vector_benchmark() {
  std::cout << "\n=== Vector Operations Benchmark ===\n";

  // Create benchmark with custom configuration
  Benchmark::Configuration config;
  config.default_iterations = 1000;
  config.warmup_iterations = 50;
  config.measure_memory = true;

  Benchmark vector_benchmark("VectorOperations", config);

  // Set performance thresholds
  PerformanceThresholds thresholds;
  thresholds.max_time = std::chrono::microseconds(100);  // 100 microseconds max
  thresholds.max_memory_bytes = 1024 * 1024;             // 1MB max
  thresholds.min_operations_per_second = 10000.0;        // 10k ops/sec min
  thresholds.max_std_dev_percentage = 20.0;              // 20% max std dev

  vector_benchmark.set_thresholds(thresholds);
  vector_benchmark.add_metadata("category", "data_structures");
  vector_benchmark.add_metadata("operation", "sort");

  // Benchmark vector sorting
  auto sort_benchmark = [&]() {
    std::vector<int> data(1000);
    std::generate(data.begin(), data.end(), []() { return rand() % 10000; });
    std::sort(data.begin(), data.end());
  };

  auto result = vector_benchmark.measure(sort_benchmark);

  // Display results
  std::cout << "Benchmark: " << result.name << "\n";
  std::cout << "Iterations: " << result.iterations << "\n";
  std::cout << "Mean time: " << result.mean_time.count() / 1000.0 << " μs\n";
  std::cout << "Min time: " << result.min_time.count() / 1000.0 << " μs\n";
  std::cout << "Max time: " << result.max_time.count() / 1000.0 << " μs\n";
  std::cout << "Std dev: " << result.std_dev.count() / 1000.0 << " μs\n";
  std::cout << "Ops/sec: " << result.operations_per_second << "\n";
  std::cout << "Memory: " << result.memory_usage_bytes << " bytes\n";
  std::cout << "Passed thresholds: " << (result.passed_thresholds() ? "YES" : "NO") << "\n";

  if (!result.passed_thresholds()) {
    auto violations = result.metadata.find("threshold_violations");
    if (violations != result.metadata.end()) {
      std::cout << "Violations: " << violations->second << "\n";
    }
  }
}

/**
 * @brief Example benchmark with setup and teardown
 */
void demonstrate_lifecycle_benchmark() {
  std::cout << "\n=== Lifecycle Benchmark ===\n";

  Benchmark lifecycle_benchmark("LifecycleTest");

  // Setup function - prepare test data
  auto setup = []() {
    // Simulate setup work
    volatile int setup_work = 0;
    for (int i = 0; i < 100; ++i) {
      setup_work += i;
    }
  };

  // Main benchmark function
  auto benchmark_func = []() {
    // Simulate the actual work being benchmarked
    std::vector<int> data(500);
    std::iota(data.begin(), data.end(), 0);
    std::reverse(data.begin(), data.end());
  };

  // Teardown function - cleanup
  auto teardown = []() {
    // Simulate cleanup work
    volatile int cleanup_work = 0;
    for (int i = 0; i < 50; ++i) {
      cleanup_work += i;
    }
  };

  auto result = lifecycle_benchmark.measure_with_lifecycle(setup, benchmark_func, teardown, 500);

  std::cout << "Lifecycle benchmark completed:\n";
  std::cout << "Mean time: " << result.mean_time.count() / 1000.0 << " μs\n";
  std::cout << "Operations/sec: " << result.operations_per_second << "\n";
}

/**
 * @brief Example benchmark suite
 */
void demonstrate_benchmark_suite() {
  std::cout << "\n=== Benchmark Suite ===\n";

  BenchmarkSuite suite("DataStructureSuite");

  // Add multiple benchmarks to the suite
  auto vector_benchmark = std::make_unique<Benchmark>("VectorPushBack");
  vector_benchmark->add_metadata("data_structure", "vector");

  auto list_benchmark = std::make_unique<Benchmark>("ListOperations");
  list_benchmark->add_metadata("data_structure", "list");

  suite.add_benchmark(std::move(vector_benchmark));
  suite.add_benchmark(std::move(list_benchmark));

  // Run all benchmarks (note: this uses dummy functions for now)
  auto results = suite.run_all();

  std::cout << "Suite '" << suite.name() << "' completed with " << results.size()
            << " benchmarks\n";

  // Generate CSV report
  try {
    suite.generate_csv_report("benchmark_results.csv", results);
    std::cout << "CSV report generated: benchmark_results.csv\n";
  } catch (const std::exception& e) {
    std::cout << "Failed to generate CSV report: " << e.what() << "\n";
  }
}

/**
 * @brief Example performance monitoring
 */
void demonstrate_performance_monitoring() {
  std::cout << "\n=== Performance Monitoring ===\n";

  PerformanceMonitor::Configuration config;
  config.track_memory = true;
  config.track_cpu = true;
  config.sample_interval = std::chrono::milliseconds(5);

  PerformanceMonitor monitor(config);

  // Monitor a function execution
  auto test_function = []() {
    std::vector<int> large_vector(100000);
    std::iota(large_vector.begin(), large_vector.end(), 0);
    std::sort(large_vector.begin(), large_vector.end(), std::greater<int>());
  };

  auto metrics = monitor.monitor_execution(test_function);

  std::cout << "Performance metrics:\n";
  std::cout << "Wall time: " << metrics.wall_time.count() / 1e6 << " ms\n";
  std::cout << "CPU usage: " << metrics.cpu_usage_percentage << "%\n";
  std::cout << "Peak memory: " << metrics.peak_memory_usage_bytes / 1024 << " KB\n";
}

/**
 * @brief Example performance profiler
 */
void demonstrate_performance_profiler() {
  std::cout << "\n=== Performance Profiler ===\n";

  PerformanceProfiler profiler;

  profiler.add_profile_point("Start");

  // Simulate some work with profile points
  std::vector<int> data(10000);
  profiler.add_profile_point("DataCreated");

  std::iota(data.begin(), data.end(), 0);
  profiler.add_profile_point("DataInitialized");

  std::sort(data.begin(), data.end(), std::greater<int>());
  profiler.add_profile_point("DataSorted");

  std::reverse(data.begin(), data.end());
  profiler.add_profile_point("DataReversed");

  // Generate and display report
  std::cout << profiler.generate_report();
}

/**
 * @brief Main demonstration function
 */
int main() {
  std::cout << "Solar System Testing Framework - Benchmark Execution Engine Demo\n";
  std::cout << "================================================================\n";

  try {
    demonstrate_vector_benchmark();
    demonstrate_lifecycle_benchmark();
    demonstrate_benchmark_suite();
    demonstrate_performance_monitoring();
    demonstrate_performance_profiler();

    std::cout << "\n=== Demo completed successfully ===\n";
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Error during benchmark demo: " << e.what() << "\n";
    return 1;
  }
}
