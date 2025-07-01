/**
 * @file solar_system.cpp
 * @brief Modern C++ Solar System simulation - Production Version
 *
 * High-performance N-body gravitational simulation using modern C++ architecture.
 * This is the main production application that replaces the legacy C implementation
 * while maintaining 100% compatibility with the original behavior and output format.
 *
 * Features:
 * - Modern C++ classes: BodyFactory, SimulationEngine, BodyCollection
 * - Multiple data sources: JPL HORIZONS, cached data, fallback data
 * - Advanced physics: Multiple integration methods, energy conservation
 * - Clean error handling: Expected<T,E> pattern throughout
 * - Complete legacy compatibility: Identical output format and behavior
 * - High performance: Optimized algorithms and data structures
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

// Modern C++ Solar System classes
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

// Legacy argument parsing and JPL functions
#include "args.h"
#include "jpl_data.h"

using namespace SolarSystem;

/**
 * @brief Print barycenter information (matching legacy format)
 */
void print_barycenter(const Math::Vector3d& barycenter) {
  std::cout << "Barycenter: " << Math::to_string(barycenter) << " m" << std::endl;
}

/**
 * @brief Print all bodies in exact legacy format
 */
void print_all_bodies(const Bodies::BodyCollection& bodies,
                      std::chrono::system_clock::time_point target_time) {
  // Calculate barycenter (center of mass)
  auto barycenter = bodies.center_of_mass();
  print_barycenter(barycenter);

  // Print target date in legacy format
  auto target_time_t = std::chrono::system_clock::to_time_t(target_time);
  std::cout << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << std::endl;

  // Print all bodies in legacy format: name, x, y, z (relative to barycenter), distance, vx, vy, vz
  for (const auto& body : bodies) {
    auto pos_relative = body.position() - barycenter;
    double distance = pos_relative.magnitude();

    std::cout << std::setw(15) << body.name() << std::setw(21) << std::scientific
              << pos_relative.x() << std::setw(21) << std::scientific << pos_relative.y()
              << std::setw(21) << std::scientific << pos_relative.z() << std::setw(21)
              << std::scientific << distance << std::setw(21) << std::scientific
              << body.velocity().x() << std::setw(21) << std::scientific << body.velocity().y()
              << std::setw(21) << std::scientific << body.velocity().z() << std::endl;
  }
}

/**
 * @brief Print simulation information (matching legacy format)
 */
void print_simulation_info(const SimulationArgs& args,
                           std::chrono::system_clock::time_point start_time,
                           std::chrono::system_clock::time_point target_time) {
  std::cout << "Solar System Simulation" << std::endl;

  auto start_time_t = std::chrono::system_clock::to_time_t(start_time);
  std::cout << "Start date: "
            << std::put_time(std::localtime(&start_time_t), "%a %b %d %H:%M:%S %Y") << std::endl;

  auto target_time_t = std::chrono::system_clock::to_time_t(target_time);
  if (args.use_current_date) {
    std::cout << "Target date: Current time ("
              << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << ")"
              << std::endl;
  } else {
    std::cout << "Target date: " << args.date_string << " ("
              << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << ")"
              << std::endl;
  }
}

/**
 * @brief Run modern simulation matching legacy behavior exactly
 */
bool run_modern_simulation(Bodies::BodyFactory& factory, const SimulationArgs& args,
                           std::chrono::system_clock::time_point start_time,
                           std::chrono::system_clock::time_point target_time) {
  // Determine if we're going forward or backward
  bool forward = target_time > start_time;
  std::cout << (forward ? "Running forward simulation" : "Running backward simulation (past date)")
            << std::endl;

  // Create simulation configuration matching legacy behavior
  Simulation::SimulationConfig config{
      .time_step = forward ? 30.0 : -30.0,  // Negative for backward simulation
      .gravitational_constant = 6.67430e-11,
      .use_adaptive_timestep = false,
      .enable_collision_detection = false};

  // Create simulation engine with Leapfrog integration (matching legacy)
  Simulation::SimulationEngine engine(config);
  engine.set_integration_method(Simulation::SimulationEngine::IntegrationMethod::LEAPFROG);

  // Create ALL solar system bodies (should be 27, not 9)
  Bodies::BodyFactory::CreationOptions body_options{
      .preferred_source = has_current_ephemeris_data()
                              ? Bodies::BodyFactory::DataSource::CACHED_DATA
                              : Bodies::BodyFactory::DataSource::FALLBACK_DATA,
      .allow_fallback = true,
      .reference_time = start_time};

  auto solar_system_result = factory.create_solar_system(body_options);
  if (!solar_system_result.has_value()) {
    std::cerr << "Failed to create solar system: " << solar_system_result.error() << std::endl;
    return false;
  }

  auto bodies = std::move(solar_system_result.value());
  std::cout << "Created " << bodies.size() << " celestial bodies" << std::endl;

  // Print simulation info
  print_simulation_info(args, start_time, target_time);

  // Initialize simulation
  auto init_result = engine.initialize(std::move(bodies), start_time);
  if (!init_result.has_value()) {
    std::cerr << "Failed to initialize simulation: " << init_result.error() << std::endl;
    return false;
  }

  // Print initial barycenter (matching legacy)
  print_barycenter(engine.get_bodies().center_of_mass());

  // Run simulation TO the target date (not for fixed duration)
  auto sim_result = engine.simulate_to_date(target_time);
  if (!sim_result.has_value()) {
    std::cerr << "Simulation failed: " << sim_result.error() << std::endl;
    return false;
  }

  // Print final results in legacy format
  print_all_bodies(engine.get_bodies(), target_time);

  return true;
}

/**
 * @brief Handle JPL data operations (legacy compatibility)
 */
bool handle_jpl_operations(const SimulationArgs& args) {
  if (args.update_data) {
    std::cout << "Updating ephemeris data..." << std::endl;
    if (update_ephemeris_data()) {
      std::cout << "Ephemeris data updated successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Failed to update ephemeris data" << std::endl;
      return false;
    }
  }

  if (args.rebuild_cache) {
    std::cout << "Rebuilding binary cache..." << std::endl;
    if (rebuild_binary_cache()) {
      std::cout << "Binary cache rebuilt successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Failed to rebuild binary cache" << std::endl;
      return false;
    }
  }

  if (args.test_storage) {
    std::cout << "Testing storage system..." << std::endl;
    if (save_current_data_for_testing()) {
      std::cout << "Storage system test completed successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Storage system test failed" << std::endl;
      return false;
    }
  }

  return true;
}

/**
 * @brief Main application entry point
 */
int main(int argc, char* argv[]) {
  // Parse command line arguments (using legacy parser)
  SimulationArgs args = parse_arguments(argc, argv);

  // Initialize JPL data system (required for legacy compatibility)
  initialize_jpl_data();

  // Set output precision to match legacy
  std::cout.precision(12);

  // Handle JPL data operations
  if (args.update_data || args.rebuild_cache || args.test_storage) {
    return handle_jpl_operations(args) ? 0 : 1;
  }

  // Determine starting date based on available ephemeris data (matching legacy logic)
  std::chrono::system_clock::time_point start_time;
  if (has_current_ephemeris_data()) {
    // Use JPL data epoch as starting point
    auto epoch = get_ephemeris_epoch();
    start_time = std::chrono::system_clock::from_time_t(epoch);
    std::cout << "Using JPL ephemeris data: " << get_ephemeris_source() << std::endl;
  } else {
    // Fallback to original hardcoded date (Feb 11, 2018)
    struct tm start_timeinfo = {};
    start_timeinfo.tm_sec = 0;
    start_timeinfo.tm_min = 0;
    start_timeinfo.tm_hour = 0;
    start_timeinfo.tm_mday = 11;
    start_timeinfo.tm_mon = 1;  // February (0-based)
    start_timeinfo.tm_year = 2018 - 1900;
    start_timeinfo.tm_isdst = -1;
    auto start_time_t = mktime(&start_timeinfo);
    start_time = std::chrono::system_clock::from_time_t(start_time_t);
    std::cout << "Using original ephemeris data: " << get_ephemeris_source() << std::endl;
  }

  // Show ephemeris data status
  auto start_time_t = std::chrono::system_clock::to_time_t(start_time);
  std::cout << "Ephemeris epoch: "
            << std::put_time(std::localtime(&start_time_t), "%a %b %d %H:%M:%S %Y") << std::endl;

  if (!has_current_year_ephemeris_data()) {
    std::cout << "Note: Consider updating ephemeris data with -u for current year" << std::endl;
  }

  // Target time
  auto target_time = std::chrono::system_clock::from_time_t(args.target_date);

  // Create body factory
  Bodies::BodyFactory factory;

  // Run the modern simulation (matching legacy behavior exactly)
  bool success = run_modern_simulation(factory, args, start_time, target_time);

  return success ? 0 : 1;
}
