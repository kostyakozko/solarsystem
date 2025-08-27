# Enhanced API Usage Examples

This document provides practical, real-world examples of using the enhanced Solar System Library APIs.

## Table of Contents

1. [Basic Solar System Simulation](#basic-solar-system-simulation)
2. [Custom Body Creation](#custom-body-creation)
3. [Advanced Filtering and Analysis](#advanced-filtering-and-analysis)
4. [Performance Monitoring](#performance-monitoring)
5. [Error Handling Patterns](#error-handling-patterns)
6. [Integration Examples](#integration-examples)

## Basic Solar System Simulation

### Example 1: Simple Earth-Moon System

```cpp
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include <iostream>

int main() {
    using namespace SolarSystem;

    // Create factory with intelligent fallback
    Bodies::BodyFactory factory;

    // Create Earth and Moon
    auto earth_result = factory.create_body("Earth");
    auto moon_result = factory.create_body("Moon");

    if (!earth_result || !moon_result) {
        std::cerr << "Failed to create Earth-Moon system" << std::endl;
        return 1;
    }

    // Create collection
    Bodies::BodyCollection bodies;
    bodies.add_body(std::move(*earth_result));
    bodies.add_body(std::move(*moon_result));

    // Configure simulation for lunar orbit
    Simulation::SimulationConfig config{
        .time_step = 3600.0,              // 1 hour steps
        .use_adaptive_timestep = true,    // Adaptive for accuracy
        .enable_collision_detection = false
    };

    Simulation::SimulationEngine engine(config);

    // Monitor lunar distance
    engine.set_progress_callback([&bodies](const Simulation::SimulationState& state) {
        auto earth = bodies.find_body("Earth");
        auto moon = bodies.find_body("Moon");

        if (earth && moon) {
            double distance = earth->distance_to(*moon);
            std::cout << "Day " << (state.current_time / 86400.0)
                      << ": Earth-Moon distance = " << (distance / 1000.0) << " km" << std::endl;
        }
    });

    // Initialize and run for one month
    engine.initialize(std::move(bodies));
    engine.run_for_duration(30 * 24 * 3600);  // 30 days

    return 0;
}
```

### Example 2: Full Solar System with Progress Tracking

```cpp
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include <iomanip>

class SolarSystemSimulator {
private:
    Bodies::BodyFactory factory_;
    Simulation::SimulationEngine engine_;

public:
    SolarSystemSimulator() {
        // Configure factory for robust operation
        Bodies::BodyFactory::FactoryConfig factory_config{
            .preferred_source = Bodies::BodyFactory::DataSource::JPL_HORIZONS,
            .fallback_strategy = Bodies::BodyFactory::FallbackStrategy::INTELLIGENT,
            .enable_validation = true,
            .allow_partial_data = false
        };
        factory_ = Bodies::BodyFactory(factory_config);

        // Configure simulation for long-term stability
        Simulation::SimulationConfig sim_config{
            .time_step = 3600.0,              // 1 hour
            .use_adaptive_timestep = true,
            .max_timestep = 86400.0,          // Max 1 day
            .min_timestep = 60.0,             // Min 1 minute
            .tolerance = 1e-12
        };
        engine_ = Simulation::SimulationEngine(sim_config);
    }

    bool initialize() {
        // Create all essential bodies
        auto collection_result = factory_.create_essential_bodies();
        if (!collection_result) {
            std::cerr << "Failed to create solar system: "
                      << collection_result.error().message << std::endl;
            return false;
        }

        Bodies::BodyCollection bodies = std::move(*collection_result);
        std::cout << "Created " << bodies.size() << " celestial bodies" << std::endl;

        // Set up comprehensive monitoring
        engine_.set_progress_callback([this](const Simulation::SimulationState& state) {
            this->monitor_progress(state);
        });

        engine_.initialize(std::move(bodies));
        return true;
    }

    void run_simulation(double duration_years) {
        double duration_seconds = duration_years * 365.25 * 24 * 3600;

        std::cout << "Starting " << duration_years << "-year simulation..." << std::endl;
        std::cout << std::fixed << std::setprecision(2);

        engine_.run_for_duration(duration_seconds);

        std::cout << "Simulation completed!" << std::endl;
    }

private:
    void monitor_progress(const Simulation::SimulationState& state) {
        // Progress every 30 days
        if (statictate.current_time) % (30 * 24 * 3600) == 0) {
            double years = state.current_time / (365.25 * 24 * 3600);

            std::cout << "Year " << years << ": "
                      << "Energy = " << state.total_energy << " J, "
                      << "Iterations = " << state.iteration_count << std::endl;

            // Check energy conservation
            static long double initial_energy = 0.0;
            if (state.iteration_count == 1) {
                initial_energy = state.total_energy;
            }

            if (initial_energy != 0.0) {
                long double energy_drift = std::abs(state.total_energy - initial_energy) / initial_energy;
                if (energy_drift > 1e-6) {
                    std::cout << "  Warning: Energy drift = " << (energy_drift * 100) << "%" << std::endl;
                }
            }
        }
    }
};

int main() {
    SolarSystemSimulator simulator;

    if (!simulator.initialize()) {
        return 1;
    }

    // Run 10-year simulation
    simulator.run_simulation(10.0);

    return 0;
}
```

## Custom Body Creation

### Example 3: Creating Custom Asteroids

```cpp
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/math/vector3.hpp"

class AsteroidBeltSimulator {
public:
    Bodies::BodyCollection create_asteroid_belt(int num_asteroids = 100) {
        Bodies::BodyCollection asteroids;

        // Random number generation for realistic distribution
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> mass_dist(1e12, 1e18);      // kg
        std::uniform_real_distribution<double> distance_dist(2.1e11, 3.3e11); // 2.1-3.3 AU
        std::uniform_real_distribution<double> angle_dist(0, 2 * M_PI);
        std::uniform_real_distribution<double> inclination_dist(-0.1, 0.1); // Small inclinations

        for (int i = 0; i < num_asteroids; ++i) {
            // Generate orbital parameters
            double distance = distance_dist(gen);
            double angle = angle_dist(gen);
            double inclination = inclination_dist(gen);
            double mass = mass_dist(gen);

            // Calculate position (simplified circular orbit)
            Math::Vector3d position{
                distance * std::cos(angle),
                distance * std::sin(angle) * std::cos(inclination),
                distance * std::sin(angle) * std::sin(inclination)
            };

            // Calculate orbital velocity (simplified)
            double orbital_speed = std::sqrt(6.67430e-11 * 1.989e30 / distance); // G*M_sun/r
            Math::Vector3d velocity{
                -orbital_speed * std::sin(angle),
                orbital_speed * std::cos(angle) * std::cos(inclination),
                orbital_speed * std::cos(angle) * std::sin(inclination)
            };

            // Create asteroid properties
            Bodies::CelestialBody::Properties asteroid_props{
                .name = "Asteroid_" + std::to_string(i + 1),
                .mass = mass,
                .position = position,
                .velocity = velocity,
                .type = Bodies::BodyType::Asteroid,
                .priority = Bodies::BodyPriority::Optional,
                .jpl_id = std::nullopt,
                .creation_date = std::chrono::system_clock::now()
            };

            asteroids.add_body(Bodies::CelestialBody(asteroid_props));
        }

        std::cout << "Created " << num_asteroids << " asteroids in belt" << std::endl;
        return asteroids;
    }

    void analyze_belt_dynamics(const Bodies::BodyCollection& asteroids) {
        // Calculate belt statistics
        auto stats = asteroids.get_statistics();

        std::cout << "Asteroid Belt Analysis:" << std::endl;
        std::cout << "  Total mass: " << stats.total_mass << " kg" << std::endl;
        std::cout << "  Center of mass: " << stats.center_of_mass.to_string() << std::endl;
        std::cout << "  Bounding radius: " << (stats.bounding_radius / 1.496e11) << " AU" << std::endl;

        // Find most massive asteroids
        auto massive_asteroids = asteroids.filter([](const Bodies::CelestialBody& body) {
            return body.mass() > 1e16; // More than 10 petagrams
        });

        std::cout << "  Massive asteroids (>10 Pg): " << massive_asteroids.size() << std::endl;

        // Analyze orbital distribution
        std::map<double, int> distance_histogram;
        for (const auto& asteroid : asteroids) {
            double distance_au = asteroid.position().magnitude() / 1.496e11;
            int bin = static_cast<int>(distance_au * 10) / 10; // 0.1 AU bins
            distance_histogram[bin]++;
        }

        std::cout << "  Distance distribution (AU):" << std::endl;
        for (const auto& [distance, count] : distance_histogram) {
            std::cout << "    " << distance << "-" << (distance + 0.1)
                      << " AU: " << count << " asteroids" << std::endl;
        }
    }
};

int main() {
    AsteroidBeltSimulator simulator;

    // Create asteroid belt
    auto asteroids = simulator.create_asteroid_belt(200);

    // Analyze the belt
    simulator.analyze_belt_dynamics(asteroids);

    // Add major planets for gravitational influence
    Bodies::BodyFactory factory;
    auto jupiter = factory.create_body("Jupiter");
    auto mars = factory.create_body("Mars");

    if (jupiter && mars) {
        asteroids.add_body(std::move(*jupiter));
        asteroids.add_body(std::move(*mars));

        std::cout << "Added major planets for gravitational influence" << std::endl;
    }

    return 0;
}
```

## Advanced Filtering and Analysis

### Example 4: Planetary System Analysis

```cpp
#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/bodies/body_factory.hpp"
#include <ranges>
#include <algorithm>

class PlanetarySystemAnalyzer {
public:
    struct PlanetaryData {
        std::string name;
        double mass;
        double distance_from_sun;
        double orbital_velocity;
        Bodies::BodyType type;
    };

    std::vector<PlanetaryData> analyze_solar_system() {
        Bodies::BodyFactory factory;
        auto collection_result = factory.create_essential_bodies();

        if (!collection_result) {
            throw std::runtime_error("Failed to create solar system");
        }

        Bodies::BodyCollection bodies = std::move(*collection_result);

        // Find the Sun for distance calculations
        auto sun_opt = bodies.find_body("Sun");
        if (!sun_opt) {
            throw std::runtime_error("Sun not found in collection");
        }
        const auto& sun = *sun_opt;

        std::vector<PlanetaryData> planetary_data;

        // Analyze all planets using C++20 ranges
        auto planets = bodies | std::views::filter([](const Bodies::CelestialBody& body) {
            return body.type() == Bodies::BodyType::Planet;
        });

        for (const auto& planet : planets) {
            PlanetaryData data{
                .name = std::string(planet.name()),
                .mass = static_cast<double>(planet.mass()),
                .distance_from_sun = planet.distance_to(sun),
                .orbital_velocity = planet.velocity().magnitude(),
                .type = planet.type()
            };
            planetary_data.push_back(data);
        }

        // Sort by distance from Sun
        std::sort(planetary_data.begin(), planetary_data.end(),
                  [](const PlanetaryData& a, const PlanetaryData& b) {
                      return a.distance_from_sun < b.distance_from_sun;
                  });

        return planetary_data;
    }

    void print_planetary_analysis(const std::vector<PlanetaryData>& data) {
        std::cout << "Solar System Planetary Analysis:" << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        std::cout << std::left << std::setw(12) << "Planet"
                  << std::setw(15) << "Mass (kg)"
                  << std::setw(15) << "Distance (AU)"
                  << std::setw(15) << "Velocity (km/s)" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& planet : data) {
            std::cout << std::left << std::setw(12) << planet.name
                      << std::setw(15) << std::scientific << planet.mass
                      << std::setw(15) << std::fixed << std::setprecision(2)
                      << (planet.distance_from_sun / 1.496e11)  // Convert to AU
                      << std::setw(15) << std::setprecision(1)
                      << (planet.orbital_velocity / 1000.0)     // Convert to km/s
                      << std::endl;
        }

        // Calculate and display statistics
        double total_mass = std::accumulate(data.begin(), data.end(), 0.0,
            [](double sum, const PlanetaryData& planet) {
                return sum + planet.mass;
            });

        auto [min_distance, max_distance] = std::minmax_element(data.begin(), data.end(),
            [](const PlanetaryData& a, const PlanetaryData& b) {
                return a.distance_from_sun < b.distance_from_sun;
            });

        std::cout << std::string(80, '-') << std::endl;
        std::cout << "Statistics:" << std::endl;
        std::cout << "  Total planetary mass: " << std::scientific << total_mass << " kg" << std::endl;
        std::cout << "  Innermost planet: " << min_distance->name
                  << " (" << std::fixed << std::setprecision(2)
                  << (min_distance->distance_from_sun / 1.496e11) << " AU)" << std::endl;
        std::cout << "  Outermost planet: " << max_distance->name
                  << " (" << (max_distance->distance_from_sun / 1.496e11) << " AU)" << std::endl;
    }

    void analyze_orbital_resonances(const std::vector<PlanetaryData>& data) {
        std::cout << "\nOrbital Period Analysis:" << std::endl;
        std::cout << std::string(50, '=') << std::endl;

        // Calculate orbital periods using Kepler's third law
        const double G = 6.67430e-11;
        const double M_sun = 1.989e30;

        std::vector<std::pair<std::string, double>> periods;

        for (const auto& planet : data) {
            // T = 2π√(a³/GM) where a is semi-major axis (approximated as distance)
            double period_seconds = 2 * M_PI * std::sqrt(
                std::pow(planet.distance_from_sun, 3) / (G * M_sun)
            );
            double period_years = period_seconds / (365.25 * 24 * 3600);

            periods.emplace_back(planet.name, period_years);

            std::cout << planet.name << ": " << std::fixed << std::setprecision(2)
                      << period_years << " years" << std::endl;
        }

        // Look for approximate resonances
        std::cout << "\nPotential Orbital Resonances:" << std::endl;
        for (size_t i = 0; i < periods.size(); ++i) {
            for (size_t j = i + 1; j < periods.size(); ++j) {
                double ratio = periods[j].second / periods[i].second;

                // Check for simple integer ratios (within 5% tolerance)
                for (int n = 2; n <= 5; ++n) {
                    for (int m = 1; m < n; ++m) {
                        double expected_ratio = static_cast<double>(n) / m;
                        if (std::abs(ratio - expected_ratio) / expected_ratio < 0.05) {
                            std::cout << "  " << periods[i].first << ":" << periods[j].first
                                      << " ≈ " << m << ":" << n
                                      << " (actual ratio: " << std::setprecision(3) << ratio << ")"
                                      << std::endl;
                        }
                    }
                }
            }
        }
    }
};

int main() {
    try {
        PlanetarySystemAnalyzer analyzer;

        // Analyze the solar system
        auto planetary_data = analyzer.analyze_solar_system();

        // Print comprehensive analysis
        analyzer.print_planetary_analysis(planetary_data);
        analyzer.analyze_orbital_resonances(planetary_data);

    } catch (const std::exception& e) {
        std::cerr << "Analysis failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

## Performance Monitoring

### Example 5: Real-time Performance Monitoring

```cpp
#include "solar_core/simulation/simulation_engine.hpp"
#include "tests/utils/performance_regression_system.h"
#include <chrono>
#include <thread>

class PerformanceMonitoredSimulation {
private:
    Simulation::SimulationEngine engine_;
    std::chrono::high_resolution_clock::time_point start_time_;
    std::vector<double> frame_times_;

public:
    PerformanceMonitoredSimulation() {
        // Configure for performance monitoring
        Simulation::SimulationConfig config{
            .time_step = 3600.0,
            .use_adaptive_timestep = true
        };
        engine_ = Simulation::SimulationEngine(config);

        // Set up performance monitoring callback
        engine_.set_progress_callback([this](const Simulation::SimulationState& state) {
            this->monitor_performance(state);
        });
    }

    void run_monitored_simulation(Bodies::BodyCollection bodies, double duration) {
        std::cout << "Starting performance-monitored simulation..." << std::endl;

        // Initialize performance regression testing
        initialize_performance_regression_testing("performance_baselines.txt");

        start_time_ = std::chrono::high_resolution_clock::now();

        // Register this simulation run for performance tracking
        register_performance_test("SimulationEngine", "monitored_run", [&]() {
            auto start = std::chrono::high_resolution_clock::now();

            engine_.initialize(std::move(bodies));
            engine_.run_for_duration(duration);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            return create_performance_metrics(duration_ms.count(), get_memory_usage());
        });

        // Run performance regression tests
        auto alerts = run_performance_regression_tests();

        if (!alerts.empty()) {
            std::cout << "Performance alerts detected:" << std::endl;
            for (const auto& alert : alerts) {
                std::cout << "  " << alert.component << ": " << alert.message << std::endl;
            }
        }

        // Print performance summary
        print_performance_summary();
    }

private:
    void monitor_performance(const Simulation::SimulationState& state) {
        static auto last_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();

        auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            current_time - last_time
        );

        frame_times_.push_back(frame_duration.count() / 1000.0); // Convert to milliseconds

        // Print progress every 100 iterations
        if (state.iteration_count % 100 == 0) {
            double avg_frame_time = calculate_average_frame_time();
            double fps = 1000.0 / avg_frame_time; // Frames per second equivalent

            std::cout << "Iteration " << state.iteration_count
                      << ": Avg frame time = " << std::fixed << std::setprecision(2)
                      << avg_frame_time << "ms, "
                      << "Rate = " << fps << " iter/sec" << std::endl;

            // Check for performance degradation
            if (avg_frame_time > 100.0) { // More than 100ms per iteration
                std::cout << "  Warning: Performance degradation detected!" << std::endl;
            }
        }

        last_time = current_time;
    }

    double calculate_average_frame_time() {
        if (frame_times_.empty()) return 0.0;

        // Calculate average of last 100 frames
        size_t start_idx = frame_times_.size() > 100 ? frame_times_.size() - 100 : 0;
        double sum = 0.0;
        size_t count = 0;

        for (size_t i = start_idx; i < frame_times_.size(); ++i) {
            sum += frame_times_[i];
            count++;
        }

        return count > 0 ? sum / count : 0.0;
    }

    void print_performance_summary() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time_
        );

        std::cout << "\nPerformance Summary:" << std::endl;
        std::cout << "===================" << std::endl;
        std::cout << "Total simulation time: " << total_duration.count() << " seconds" << std::endl;

        if (!frame_times_.empty()) {
            auto [min_time, max_time] = std::minmax_element(frame_times_.begin(), frame_times_.end());
            double avg_time = std::accumulate(frame_times_.begin(), frame_times_.end(), 0.0) / frame_times_.size();

            std::cout << "Frame statistics:" << std::endl;
            std::cout << "  Average: " << std::fixed << std::setprecision(2) << avg_time << "ms" << std::endl;
            std::cout << "  Minimum: " << *min_time << "ms" << std::endl;
            std::cout << "  Maximum: " << *max_time << "ms" << std::endl;
            std::cout << "  Total frames: " << frame_times_.size() << std::endl;
        }
    }
};

int main() {
    try {
        // Create solar system
        Bodies::BodyFactory factory;
        auto bodies_result = factory.create_essential_bodies();

        if (!bodies_result) {
            std::cerr << "Failed to create solar system" << std::endl;
            return 1;
        }

        // Run performance-monitored simulation
        PerformanceMonitoredSimulation sim;
        sim.run_monitored_simulation(std::move(*bodies_result), 365.25 * 24 * 3600); // 1 year

    } catch (const std::exception& e) {
        std::cerr << "Simulation error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

These examples demonstrate practical usage patterns for the enhanced APIs, covering everything from basic simulations to advanced performance monitoring and analysis. Each example includes proper error handling, modern C++ features, and real-world scenarios that developers might encounter.
