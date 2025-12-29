/**
 * @file regression_detector.cpp
 * @brief Performance regression detection
 * @note Migrated to Google Test
 */

#include <filesystem>

#include "benchmark_utils.h"
#include <gtest/gtest.h>
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

  return 0;
