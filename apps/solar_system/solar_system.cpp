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
#include "solar_core/simulation/checkpoint.hpp"
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
 * @brief Run HIGH-PERFORMANCE simulation with checkpointing support
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

  // Setup checkpoint manager if checkpointing is enabled
  std::unique_ptr<Simulation::CheckpointManager> checkpoint_manager;
  std::unique_ptr<Simulation::CheckpointScheduler> checkpoint_scheduler;

  if (config.enable_checkpointing) {
    Simulation::CheckpointConfig checkpoint_config;
    checkpoint_config.checkpoint_directory = config.checkpoint_directory;
    checkpoint_config.checkpoint_interval =
        std::chrono::seconds(std::stoi(config.checkpoint_interval));

    checkpoint_manager = std::make_unique<Simulation::CheckpointManager>(checkpoint_config);
    checkpoint_scheduler =
        std::make_unique<Simulation::CheckpointScheduler>(*checkpoint_manager, checkpoint_config);

    std::cout << "Checkpointing enabled (interval: " << config.checkpoint_interval
              << "s, directory: " << config.checkpoint_directory << ")" << std::endl;
  }

  // OPTIMIZED: NO progress callback to avoid overhead unless checkpointing
  if (config.enable_checkpointing && checkpoint_scheduler) {
    engine.set_progress_callback(
        [&checkpoint_scheduler, &engine](const Simulation::SimulationState& state) {
          if (checkpoint_scheduler->should_checkpoint(state)) {
            auto result = checkpoint_scheduler->trigger_checkpoint(engine);
            if (result.has_value()) {
              std::cout << "Checkpoint saved: " << result.value() << std::endl;
            } else {
              std::cerr << "Checkpoint failed: " << to_string(result.error()) << std::endl;
            }
          }
        });
  }

  // Create bodies using configured body set (default: complete for comprehensive simulation)
  SolarSystem::Bodies::BodyFactory::DefaultBodySet body_set;
  if (config.body_set == "essential") {
    body_set = SolarSystem::Bodies::BodyFactory::DefaultBodySet::ESSENTIAL;
  } else if (config.body_set == "important") {
    body_set = SolarSystem::Bodies::BodyFactory::DefaultBodySet::IMPORTANT;
  } else {
    // Default to complete set for main simulation (handles "complete" and any other value)
    body_set = SolarSystem::Bodies::BodyFactory::DefaultBodySet::COMPLETE;
  }

  Bodies::BodyFactory::CreationOptions body_options{
      .preferred_source = factory.has_current_ephemeris_data()
                              ? Bodies::BodyFactory::DataSource::CACHED_DATA
                              : Bodies::BodyFactory::DataSource::FALLBACK_DATA,
      .reference_time = start_time,
      .allow_fallback = true,
      .default_body_set = body_set};

  auto solar_system_result = factory.create_default_bodies(body_options);
  if (!solar_system_result.has_value()) {
    std::cerr << "Failed to create solar system: " << solar_system_result.error() << std::endl;
    return false;
  }

  auto bodies = std::move(solar_system_result.value());
  std::cout << "Created " << bodies.size() << " celestial bodies" << std::endl;

  // Check if we should resume from checkpoint
  if (!config.resume_from_checkpoint.empty()) {
    std::cout << "Resuming from checkpoint: " << config.resume_from_checkpoint << std::endl;

    if (!checkpoint_manager) {
      Simulation::CheckpointConfig checkpoint_config;
      checkpoint_config.checkpoint_directory = config.checkpoint_directory;
      checkpoint_manager = std::make_unique<Simulation::CheckpointManager>(checkpoint_config);
    }

    auto resume_result =
        checkpoint_manager->resume_simulation(engine, config.resume_from_checkpoint);
    if (!resume_result.has_value()) {
      std::cerr << "Failed to resume from checkpoint: " << to_string(resume_result.error())
                << std::endl;
      return false;
    }

    std::cout << "Successfully resumed from checkpoint" << std::endl;
    std::cout << "Current simulation time: " << engine.get_current_time() << " seconds"
              << std::endl;

    // Start checkpoint scheduler if enabled
    if (checkpoint_scheduler) {
      checkpoint_scheduler->start_scheduling();
    }
  } else {
    // Normal initialization
    // Print simulation info
    print_simulation_info(config, start_time, target_time);

    // Initialize simulation
    auto init_result = engine.initialize(std::move(bodies), start_time);
    if (!init_result.has_value()) {
      std::cerr << "Failed to initialize simulation: " << init_result.error() << std::endl;
      return false;
    }

    // Start checkpoint scheduler if enabled
    if (checkpoint_scheduler) {
      checkpoint_scheduler->start_scheduling();
    }
  }

  // Print initial barycenter (matching legacy)
  print_barycenter(engine.get_bodies().center_of_mass());

  // OPTIMIZED: Run simulation TO the target date with minimal overhead
  auto sim_result = engine.simulate_to_date(target_time);
  if (!sim_result.has_value()) {
    std::cerr << "Simulation failed: " << sim_result.error() << std::endl;
    return false;
  }

  // Stop checkpoint scheduler
  if (checkpoint_scheduler) {
    checkpoint_scheduler->stop_scheduling();
  }

  // Save final checkpoint if checkpointing is enabled
  if (config.enable_checkpointing && checkpoint_manager) {
    auto final_checkpoint_result = checkpoint_manager->save_checkpoint(engine, "final");
    if (final_checkpoint_result.has_value()) {
      std::cout << "Final checkpoint saved: " << final_checkpoint_result.value() << std::endl;
    } else {
      std::cerr << "Failed to save final checkpoint: " << to_string(final_checkpoint_result.error())
                << std::endl;
    }
  }

  // Print final results in legacy format
  print_all_bodies(engine.get_bodies(), target_time);

  return true;
}

/**
 * @brief Handle checkpoint operations
 */
bool handle_checkpoint_operations(const SolarSystem::Utils::SimulationConfig& config) {
  using namespace SolarSystem::Simulation;

  CheckpointConfig checkpoint_config;
  checkpoint_config.checkpoint_directory = config.checkpoint_directory;

  CheckpointManager manager(checkpoint_config);

  if (config.list_checkpoints) {
    std::cout << "Available checkpoints:" << std::endl;
    auto checkpoints = manager.list_checkpoints();

    if (checkpoints.empty()) {
      std::cout << "  No checkpoints found." << std::endl;
    } else {
      for (const auto& checkpoint : checkpoints) {
        auto created_time_t = std::chrono::system_clock::to_time_t(checkpoint.created_at);
        auto sim_time_t = std::chrono::system_clock::to_time_t(checkpoint.simulation_time);

        std::cout << "  ID: " << checkpoint.checkpoint_id << std::endl;
        std::cout << "    Created: "
                  << std::put_time(std::localtime(&created_time_t), "%Y-%m-%d %H:%M:%S")
                  << std::endl;
        std::cout << "    Simulation time: "
                  << std::put_time(std::localtime(&sim_time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
        std::cout << "    Bodies: " << checkpoint.body_count << std::endl;
        std::cout << "    Iterations: " << checkpoint.iteration_count << std::endl;
        std::cout << "    Size: " << checkpoint.compressed_size << " bytes" << std::endl;
        std::cout << std::endl;
      }
    }
    return true;
  }

  if (!config.delete_checkpoint.empty()) {
    std::cout << "Deleting checkpoint: " << config.delete_checkpoint << std::endl;
    auto result = manager.delete_checkpoint(config.delete_checkpoint);

    if (result.has_value()) {
      std::cout << "Checkpoint deleted successfully." << std::endl;
      return true;
    } else {
      std::cerr << "Failed to delete checkpoint: " << to_string(result.error()) << std::endl;
      return false;
    }
  }

  if (config.validate_checkpoint && !config.checkpoint_id.empty()) {
    std::cout << "Validating checkpoint: " << config.checkpoint_id << std::endl;
    auto result = manager.validate_checkpoint(config.checkpoint_id);

    if (result.has_value() && result.value()) {
      std::cout << "Checkpoint validation passed." << std::endl;
      return true;
    } else {
      std::cerr << "Checkpoint validation failed." << std::endl;
      return false;
    }
  }

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

  // Handle checkpoint operations
  if (config.list_checkpoints || !config.delete_checkpoint.empty() || config.validate_checkpoint) {
    return handle_checkpoint_operations(config) ? 0 : 1;
  }

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
