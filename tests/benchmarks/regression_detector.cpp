/**
 * @file regression_detector.cpp
 * @brief Performance regression detection
 * @note Migrated to Google Test
 */

#include <filesystem>
#include <iostream>
#include <vector>

#include "benchmark_utils.h"
#include <gtest/gtest.h>

// Regression Detection Benchmark Test
TEST(RegressionDetectorBenchmark, PerformanceStabilityValidation) {
  Benchmark::BenchmarkSuite suite("Regression Detection Benchmarks");

  // Baseline performance test
  suite.run_benchmark(
      "BaselinePerformanceTest",
      []() {
        // Simulate consistent workload
        volatile double result = 0.0;
        for (int i = 0; i < 1000; ++i) {
          result = result + static_cast<double>(i) * 0.001;
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
          sum = sum + static_cast<size_t>(val);
        }
        (void)sum;
      },
      1000);

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/regression_detection.csv", "csv");

  // Validate performance stability
  auto results = suite.get_results();
  bool all_stable = true;

  std::cout << "\n=== Regression Detection Validation ===" << std::endl;

  for (const auto& result : results) {
    std::cout << result.name << ": " << result.avg_duration_ms << " ms avg, "
              << "std_dev: " << result.std_deviation_ms << " ms" << std::endl;

    // Check for high variance (potential instability)
    if (result.avg_duration_ms > 0 &&
        result.std_deviation_ms / result.avg_duration_ms > 0.5) {
      std::cout << "WARNING: High variance detected in " << result.name << std::endl;
      all_stable = false;
    }
  }

  std::cout << "\nOverall Regression Detection: " << (all_stable ? "PASS" : "FAIL") << std::endl;

  EXPECT_TRUE(all_stable);
}
