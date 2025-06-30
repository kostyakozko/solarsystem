/**
 * @file benchmark_simulation.cpp
 * @brief Performance benchmarks for simulation engine
 *
 * Comprehensive performance testing of the N-body simulation engine
 * including scalability analysis and optimization validation.
 */

#include "../../lib/solar_core/include/model.h"
#include "../../lib/solar_core/include/simulation.h"
#include "benchmark_utils.h"
#include "test_data.h"
#include "test_framework.h"

int main() {
  Benchmark::BenchmarkSuite suite("Simulation Engine Performance");

  // Benchmark basic gravitational force calculation
  suite.run_benchmark(
      "Gravitational Force Calculation",
      []() {
        planet sun, earth;
        sun.mass = SUN_MASS;
        sun.position = {0.0, 0.0, 0.0};

        earth.mass = EARTH_MASS;
        earth.position = {AU / 1000.0, 0.0, 0.0};

        // Calculate force (this should be very fast)
        coord force = calculate_gravitational_force(earth, sun);

        // Prevent optimization from removing the calculation
        volatile double result = force.x + force.y + force.z;
        (void)result;
      },
      100000);

  // Benchmark distance calculation
  suite.run_benchmark(
      "Distance Calculation",
      []() {
        coord pos1 = {1000.0, 2000.0, 3000.0};
        coord pos2 = {4000.0, 5000.0, 6000.0};

        double distance = calculate_distance(pos1, pos2);
        volatile double result = distance;
        (void)result;
      },
      100000);

  // Benchmark single integration step
  suite.run_benchmark(
      "Single Integration Step",
      []() {
        std::vector<planet> system;

        // Create Earth-Moon system
        planet earth, moon;
        earth.mass = EARTH_MASS;
        earth.position = {0.0, 0.0, 0.0};
        earth.velocity = {0.0, 0.0, 0.0};

        moon.mass = 7.342e22;
        moon.position = {384400.0, 0.0, 0.0};
        moon.velocity = {0.0, 1.022, 0.0};

        system.push_back(earth);
        system.push_back(moon);

        integrate_system(system, 3600.0);
      },
      10000);

  // Benchmark N-body simulation scalability
  std::vector<size_t> body_counts = {2, 5, 10, 20, 50};
  suite.run_scalability_benchmark(
      "N-Body Simulation Scaling",
      [](size_t num_bodies) {
        std::vector<planet> system;

        // Create system with specified number of bodies
        for (size_t i = 0; i < num_bodies; ++i) {
          planet body;
          body.mass = 1e24 + i * 1e23;  // Varying masses
          body.position = {static_cast<double>(i * 1e6), static_cast<double>(i * 1e5), 0.0};
          body.velocity = {static_cast<double>(i * 0.1), static_cast<double>(i * 0.2), 0.0};
          system.push_back(body);
        }

        // Perform one integration step
        integrate_system(system, 3600.0);
      },
      body_counts, 100);

  // Benchmark time step sensitivity
  std::vector<size_t> time_steps = {100, 500, 1000, 3600, 7200};
  suite.run_scalability_benchmark(
      "Time Step Performance",
      [](size_t time_step) {
        std::vector<planet> system;

        // Simple two-body system
        planet body1, body2;
        body1.mass = 1e30;
        body1.position = {0.0, 0.0, 0.0};
        body1.velocity = {0.0, 0.0, 0.0};

        body2.mass = 1e24;
        body2.position = {1e8, 0.0, 0.0};
        body2.velocity = {0.0, 10.0, 0.0};

        system.push_back(body1);
        system.push_back(body2);

        integrate_system(system, static_cast<double>(time_step));
      },
      time_steps, 1000);

  // Benchmark full solar system simulation
  suite.run_benchmark(
      "Full Solar System Step",
      []() {
        std::vector<planet> system;

        // Create simplified solar system (Sun + 8 planets)
        std::vector<std::string> names = {"Sun",     "Mercury", "Venus",  "Earth",  "Mars",
                                          "Jupiter", "Saturn",  "Uranus", "Neptune"};
        std::vector<double> masses = {SUN_MASS, 3.3e23,   4.87e24,  EARTH_MASS, 6.39e23,
                                      1.898e27, 5.683e26, 8.681e25, 1.024e26};
        std::vector<double> distances = {0.0, 0.39, 0.72, 1.0, 1.52, 5.2, 9.5, 19.2, 30.1};  // AU

        for (size_t i = 0; i < names.size(); ++i) {
          planet body;
          body.name = names[i];
          body.mass = masses[i];
          body.position = {distances[i] * AU / 1000.0, 0.0, 0.0};  // Convert to km

          if (i > 0) {
            // Approximate circular orbital velocity
            double orbital_velocity = sqrt(G * SUN_MASS / (distances[i] * AU)) / 1000.0;  // km/s
            body.velocity = {0.0, orbital_velocity, 0.0};
          } else {
            body.velocity = {0.0, 0.0, 0.0};
          }

          system.push_back(body);
        }

        integrate_system(system, 3600.0);  // 1 hour step
      },
      1000);

  // Benchmark energy conservation calculation
  suite.run_benchmark(
      "Energy Conservation Check",
      []() {
        std::vector<planet> system;

        // Create test system
        planet sun, earth;
        sun.mass = SUN_MASS;
        sun.position = {0.0, 0.0, 0.0};
        sun.velocity = {0.0, 0.0, 0.0};

        earth.mass = EARTH_MASS;
        earth.position = {AU / 1000.0, 0.0, 0.0};
        earth.velocity = {0.0, 29.78, 0.0};

        system.push_back(sun);
        system.push_back(earth);

        double energy = calculate_total_energy(system);
        volatile double result = energy;
        (void)result;
      },
      10000);

  // Benchmark momentum conservation calculation
  suite.run_benchmark(
      "Momentum Conservation Check",
      []() {
        std::vector<planet> system;

        // Create test system
        planet body1, body2;
        body1.mass = 1e24;
        body1.position = {-1000.0, 0.0, 0.0};
        body1.velocity = {1.0, 0.0, 0.0};

        body2.mass = 2e24;
        body2.position = {500.0, 0.0, 0.0};
        body2.velocity = {-0.5, 0.0, 0.0};

        system.push_back(body1);
        system.push_back(body2);

        coord momentum = calculate_total_momentum(system);
        volatile double result = momentum.x + momentum.y + momentum.z;
        (void)result;
      },
      10000);

  // Memory usage benchmark
  suite.run_memory_benchmark(
      "Memory Usage - Large System",
      []() {
        std::vector<planet> system;

        // Create large system (100 bodies)
        for (int i = 0; i < 100; ++i) {
          planet body;
          body.mass = 1e20 + i * 1e19;
          body.position = {static_cast<double>(i * 1e6), static_cast<double>(i * 1e5), 0.0};
          body.velocity = {static_cast<double>(i * 0.1), static_cast<double>(i * 0.2), 0.0};
          system.push_back(body);
        }

        // Perform integration
        integrate_system(system, 3600.0);
      },
      100);

  // Long-duration simulation benchmark
  suite.run_benchmark(
      "Long Duration Simulation",
      []() {
        std::vector<planet> system;

        // Simple Earth-Moon system
        planet earth, moon;
        earth.mass = EARTH_MASS;
        earth.position = {0.0, 0.0, 0.0};
        earth.velocity = {0.0, 0.0, 0.0};

        moon.mass = 7.342e22;
        moon.position = {384400.0, 0.0, 0.0};
        moon.velocity = {0.0, 1.022, 0.0};

        system.push_back(earth);
        system.push_back(moon);

        // Simulate for 24 hours (24 steps of 1 hour each)
        for (int i = 0; i < 24; ++i) {
          integrate_system(system, 3600.0);
        }
      },
      100);

  // Print results
  suite.print_summary();

  // Export results for regression analysis
  suite.export_results("benchmark_results/simulation_performance.csv");

  // Generate performance report
  suite.generate_performance_report("benchmark_results/simulation_report.txt");

  return 0;
}
