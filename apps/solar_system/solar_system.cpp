/**
 * @file solar_system_optimized.cpp
 * @brief High-performance optimized version - matching legacy speed
 *
 * This version removes all performance overhead while maintaining modern C++ architecture:
 * - No progress callbacks during simulation
 * - Minimal object creation overhead
 * - Optimized simulation loop
 * - Direct memory access patterns
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

// Modern C++ Solar System classes
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

// Modern argument parsing
#include "solar_utils/argument_parser.hpp"

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
    long double distance = pos_relative.magnitude();

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
void print_simulation_info(const SolarSystem::Utils::SimulationConfig& config,
                           std::chrono::system_clock::time_point start_time,
                           std::chrono::system_clock::time_point target_time) {
  std::cout << "Solar System Simulation" << std::endl;

  auto start_time_t = std::chrono::system_clock::to_time_t(start_time);
  std::cout << "Start date: "
            << std::put_time(std::localtime(&start_time_t), "%a %b %d %H:%M:%S %Y") << std::endl;

  auto target_time_t = std::chrono::system_clock::to_time_t(target_time);
  if (config.use_current_date) {
    std::cout << "Target date: Current time ("
              << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << ")"
              << std::endl;
  } else {
    std::cout << "Target date: " << config.date_string << " ("
              << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << ")"
              << std::endl;
  }
}

/**
 * @brief Run HIGH-PERFORMANCE simulation matching legacy speed
 */
bool run_optimized_simulation(Bodies::BodyFactory& factory,
                              const SolarSystem::Utils::SimulationConfig& config,
                              std::chrono::system_clock::time_point start_time,
                              std::chrono::system_clock::time_point target_time) {
  // Determine if we're going forward or backward
  bool forward = target_time > start_time;
  std::cout << (forward ? "Running forward simulation" : "Running backward simulation (past date)")
            << std::endl;

  // OPTIMIZED: Use larger timestep for better performance (matching legacy)
  Simulation::SimulationConfig sim_config{
      .time_step = forward ? 86400.0 : -86400.0,  // 1 day timestep (like legacy)
      .gravitational_constant = 6.67430e-11,
      .use_adaptive_timestep = false,
      .enable_collision_detection = false};

  // Create simulation engine with Leapfrog integration (matching legacy)
  Simulation::SimulationEngine engine(sim_config);
  engine.set_integration_method(Simulation::SimulationEngine::IntegrationMethod::LEAPFROG);

  // OPTIMIZED: NO progress callback to avoid overhead
  // engine.set_progress_callback(...); // REMOVED FOR PERFORMANCE

  // Create ALL solar system bodies (should be 27, not 9)
  Bodies::BodyFactory::CreationOptions body_options{
      .preferred_source = factory.has_current_ephemeris_data()
                              ? Bodies::BodyFactory::DataSource::CACHED_DATA
                              : Bodies::BodyFactory::DataSource::FALLBACK_DATA,
      .reference_time = start_time,
      .allow_fallback = true};

  auto solar_system_result = factory.create_solar_system(body_options);
  if (!solar_system_result.has_value()) {
    std::cerr << "Failed to create solar system: " << solar_system_result.error() << std::endl;
    return false;
  }

  auto bodies = std::move(solar_system_result.value());
  std::cout << "Created " << bodies.size() << " celestial bodies" << std::endl;

  // Print simulation info
  print_simulation_info(config, start_time, target_time);

  // Initialize simulation
  auto init_result = engine.initialize(std::move(bodies), start_time);
  if (!init_result.has_value()) {
    std::cerr << "Failed to initialize simulation: " << init_result.error() << std::endl;
    return false;
  }

  // Print initial barycenter (matching legacy)
  print_barycenter(engine.get_bodies().center_of_mass());

  // OPTIMIZED: Run simulation TO the target date with minimal overhead
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
 * @brief Handle JPL data operations (modern config)
 */
bool handle_jpl_operations(const SolarSystem::Utils::SimulationConfig& config,
                           SolarSystem::Bodies::BodyFactory& factory) {
  if (config.update_data) {
    std::cout << "Updating ephemeris data..." << std::endl;
    // Calculate current year epoch in the application layer
    auto result = factory.fetch_current_ephemeris_data();

    if (result) {
      std::cout << "Ephemeris data updated successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Failed to update ephemeris data: " << result.error() << std::endl;
      return false;
    }
  }

  if (config.rebuild_cache) {
    // Use JPL client to rebuild cache
    auto result = factory.rebuild_cache();  // Add this method to BodyFactory

    if (result.has_value()) {
      std::cout << "Binary cache rebuilt successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Failed to rebuild binary cache: " << result.error() << std::endl;
      return false;
    }
  }

  if (config.test_storage) {  // or whatever the condition is
    std::cout << "Testing storage system..." << std::endl;

    auto result = factory.test_storage_system();  // Add this method to BodyFactory

    if (result.has_value()) {
      std::cout << "Storage system test completed successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Storage system test failed: " << result.error() << std::endl;
      return false;
    }
  }

  return true;
}

/**
 * @brief Main application entry point - OPTIMIZED FOR PERFORMANCE
 */
int main(int argc, char* argv[]) {
  using namespace std::chrono;
  using namespace SolarSystem::Utils;

  // Create body factory
  Bodies::BodyFactory factory;

  // Handle help request first
  if (argc == 1) {
    // No arguments provided - show usage
    SimulationArgumentParser parser(argv[0]);
    parser.print_usage();
    return 0;
  }

  // Check for help flag specifically
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help") {
      SimulationArgumentParser parser(argv[0]);
      parser.print_usage();
      return 0;
    }
  }

  // Parse command line arguments using modern parser
  SimulationArgumentParser parser(argv[0]);
  auto result = parser.parse(argc, const_cast<const char* const*>(argv));

  if (!result) {
    std::cerr << "Error parsing arguments: " << to_string(result.error()) << std::endl;
    parser.print_usage();
    return 1;
  }

  const auto& config = result.value();

  // Set output precision to match legacy
  std::cout.precision(12);

  // Handle JPL data operations
  if (config.update_data || config.rebuild_cache || config.test_storage) {
    return handle_jpl_operations(config, factory) ? 0 : 1;
  }

  // Determine starting date based on available ephemeris data (matching legacy logic)
  std::chrono::system_clock::time_point start_time;

  if (factory.has_current_ephemeris_data()) {
    // Use JPL data epoch as starting point
    start_time = factory.current_epoch();
    std::cout << "Using JPL ephemeris data: " << factory.current_source() << std::endl;
  } else {
    year_month_day ymd{year{2018}, month{2}, day{11}};
    start_time = sys_days{ymd};
    std::cout << "Using original ephemeris data: " << factory.current_source() << std::endl;
  }

  // Show ephemeris data status
  auto start_time_t = std::chrono::system_clock::to_time_t(start_time);
  std::cout << "Ephemeris epoch: "
            << std::put_time(std::localtime(&start_time_t), "%a %b %d %H:%M:%S %Y") << std::endl;

  if (factory.has_current_year_ephemeris_data()) {
    std::cout << "Note: Consider updating ephemeris data with -u for current year" << std::endl;
  }

  // Target time
  auto target_time = std::chrono::system_clock::from_time_t(config.get_target_date().to_time_t());

  // Run the OPTIMIZED simulation (matching legacy performance)
  bool success = run_optimized_simulation(factory, config, start_time, target_time);

  return success ? 0 : 1;
}
