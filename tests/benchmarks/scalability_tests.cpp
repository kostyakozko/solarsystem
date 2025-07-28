/**
 * @file scalability_tests.cpp
 * @brief Scalability analysis tests
 */

#include <filesystem>

#include "benchmark_utils.h"
#include "test_framework.h"

int main() {
  Benchmark::BenchmarkSuite suite("Scalability Analysis");

  // Test scalability with different input sizes
  std::vector<size_t> input_sizes = {10, 100, 1000, 5000};

  suite.run_scalability_benchmark(
      "DataProcessingScalability",
      [](size_t size) {
        // Simulate data processing that scales with input size
        std::vector<double> data(size);
        for (size_t i = 0; i < size; ++i) {
          data[i] = std::sin(i * 0.001);
        }

        volatile double sum = 0.0;
        for (double val : data) {
          sum += val;
        }
        (void)sum;
      },
      input_sizes, 10);

  // Test memory scalability
  suite.run_scalability_benchmark(
      "MemoryScalability",
      [](size_t size) {
        // Simulate memory allocation scaling
        std::vector<int> data(size);
        std::fill(data.begin(), data.end(), 42);
        volatile size_t total = data.size();
        (void)total;
      },
      input_sizes, 10);

  suite.print_summary();

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/scalability_analysis.csv", "csv");

  return 0;
}
