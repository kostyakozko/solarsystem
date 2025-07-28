/**
 * @file regression_detector.cpp
 * @brief Performance regression detection
 */

#include <filesystem>

#include "benchmark_utils.h"
#include "test_framework.h"

int main() {
  Benchmark::BenchmarkSuite suite("Regression Detection");

  // Baseline performance test
  suite.run_benchmark(
      "BaselinePerformanceTest",
      []() {
        // Consistent baseline operation
        volatile double result = 0.0;
        for (int i = 0; i < 1000; ++i) {
          result += std::sin(i * 0.001);
        }
        (void)result;
      },
      1000);

  // Memory usage stability test
  suite.run_benchmark(
      "MemoryUsageStabilityTest",
      []() {
        // Consistent memory usage pattern
        std::vector<int> data(1000);
        for (size_t i = 0; i < data.size(); ++i) {
          data[i] = static_cast<int>(i);
        }
        volatile size_t sum = 0;
        for (int val : data) {
          sum += val;
        }
        (void)sum;
      },
      1000);

  suite.print_summary();

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/regression_detection.csv", "csv");

  return 0;
}
