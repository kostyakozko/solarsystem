/**
 * @file benchmark_jpl_data.cpp
 * @brief Performance benchmarks for JPL data processing
 */

#include <filesystem>

#include "benchmark_utils.h"
#include "test_framework.h"

int main() {
  Benchmark::BenchmarkSuite suite("JPL Data Performance");

  // Cache loading benchmark
  suite.run_benchmark(
      "CacheLoadingBenchmark",
      []() {
        // Simulate cache loading operation
        volatile int result = 0;
        for (int i = 0; i < 1000; ++i) {
          result += i * i;
        }
        (void)result;
      },
      1000);

  // JPL response parsing benchmark
  suite.run_benchmark(
      "JPLResponseParsingBenchmark",
      []() {
        // Simulate JPL response parsing
        std::string data = "2459945.500000000 = A.D. 2023-Jan-01 00:00:00.0000 TDB";
        volatile size_t len = data.length();
        for (size_t i = 0; i < len; ++i) {
          volatile char c = data[i];
          (void)c;
        }
      },
      1000);

  // Data conversion benchmark
  suite.run_benchmark(
      "DataConversionBenchmark",
      []() {
        // Simulate data conversion operations
        double x = 1.23456789;
        for (int i = 0; i < 100; ++i) {
          volatile double result = x * 1.5 + 0.5;
          (void)result;
        }
      },
      1000);

  suite.print_summary();

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/jpl_data_benchmark.csv", "csv");

  return 0;
}
