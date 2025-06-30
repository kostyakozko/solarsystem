/**
 * @file benchmark_simulation.cpp
 * @brief Performance benchmarks for simulation engine (actual API)
 */

#include "../../lib/solar_core/model.h"
#include "../../lib/solar_core/simulation.h"
#include "benchmark_utils.h"
#include "test_data.h"
#include "test_framework.h"

int main() {
  Benchmark::BenchmarkSuite suite("Simulation Engine Performance");

  // Benchmark distance calculation
  suite.run_benchmark(
      "Distance Calculation",
      []() {
        coord pos1 = {1000.0, 2000.0, 3000.0};
        coord pos2 = {4000.0, 5000.0, 6000.0};

        long double distance = dist(pos1, pos2);
        volatile long double result = distance;
        (void)result;
      },
      100000);

  // Benchmark gravitational attraction calculation
  suite.run_benchmark(
      "Gravitational Attraction",
      []() {
        coord test_position = {AU, 0.0, 0.0};
        acceleration delta = {0.0, 0.0, 0.0};

        // Calculate attraction to first body (if available)
        if (get_body_count() > 0) {
          attractTo(test_position, delta, 0);
        }

        volatile long double result = delta.x + delta.y + delta.z;
        (void)result;
      },
      10000);

  // Benchmark simulation step
  suite.run_benchmark(
      "Simulation Step",
      []() {
        initialize_simulation_to_current_time();
        perform_simulation_step(3600.0);  // 1 hour step
      },
      100);

  // Benchmark barycenter calculation
  suite.run_benchmark(
      "Barycenter Calculation",
      []() {
        coord barycenter = getBarycenter();
        volatile long double result = barycenter.x + barycenter.y + barycenter.z;
        (void)result;
      },
      1000);

  // Print results
  suite.print_summary();

  // Export results for regression analysis
  suite.export_results("benchmark_results/simulation_performance.csv");

  return 0;
}
