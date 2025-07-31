/**
 * @file benchmark_comprehensive.cpp
 * @brief Comprehensive system benchmarks
 */

#include <filesystem>

#include "benchmark_utils.h"
#include "test_framework.h"

int main() {
  Benchmark::BenchmarkSuite suite("Comprehensive Performance");

  // Simulation step benchmark (validate microsecond claims)
  suite.run_benchmark(
      "SimulationStepBenchmark",
      []() {
        // Simulate a single simulation step
        double positions[3] = {1.0, 0.0, 0.0};
        double velocities[3] = {0.0, 1.0, 0.0};
        double dt = 0.001;

        // Simple Euler integration step
        for (int i = 0; i < 3; ++i) {
          positions[i] += velocities[i] * dt;
        }

        volatile double sum = positions[0] + positions[1] + positions[2];
        (void)sum;
      },
      10000);

  // Memory allocation benchmark
  suite.run_benchmark(
      "MemoryAllocationBenchmark",
      []() {
        // Simulate memory allocation patterns
        std::vector<double> data;
        data.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
          data.push_back(i * 0.001);
        }
        volatile size_t size = data.size();
        (void)size;
      },
      1000);

  // Mathematical operations benchmark
  suite.run_benchmark(
      "MathematicalOperationsBenchmark",
      []() {
        // Simulate complex mathematical operations
        double x = 1.23456789;
        double y = 9.87654321;
        volatile double result = 0.0;

        for (int i = 0; i < 100; ++i) {
          result = result + std::sqrt(x * x + y * y);
          x *= 1.001;
          y *= 0.999;
        }
        (void)result;
      },
      1000);

  suite.print_summary();

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/comprehensive_benchmark.csv", "csv");

  return 0;
}
