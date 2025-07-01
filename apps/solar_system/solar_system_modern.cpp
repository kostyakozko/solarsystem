/**
 * @file solar_system_modern.cpp
 * @brief Modern C++ Solar System simulation using SimulationEngine
 *
 * This is the modernized version of solar_system.cpp that demonstrates
 * the complete transformation from legacy C code to modern C++ architecture.
 *
 * Features:
 * - Modern C++ classes: BodyFactory, SimulationEngine, BodyCollection
 * - Multiple data sources: JPL HORIZONS, cached data, fallback data
 * - Advanced physics: Multiple integration methods, energy conservation
 * - Clean error handling: Expected<T,E> pattern throughout
 * - Real-time progress monitoring and statistics
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

// Modern C++ Solar System classes
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

// Legacy argument parsing and JPL functions (to be modernized later)
#include "args.h"
#include "jpl_data.h"

using namespace SolarSystem;

/**
 * @brief Print simulation configuration and status
 */
void print_simulation_info(const SimulationArgs& args, const Simulation::SimulationEngine& engine,
                           const Bodies::BodyCollection& bodies) {
  std::cout << "\n🌌 Modern Solar System Simulation\n";
  std::cout << "==================================\n";

  // Simulation configuration
  const auto& config = engine.get_config();
  std::cout << "Integration method: " << Simulation::to_string(engine.get_integration_method())
            << "\n";
  std::cout << "Time step: " << config.time_step << " seconds\n";
  std::cout << "Bodies in simulation: " << bodies.size() << "\n";
  std::cout << "Total system mass: " << std::scientific << bodies.total_mass() << " kg\n";

  // Target date information
  auto target_time = std::chrono::system_clock::from_time_t(args.target_date);
  auto target_time_t = std::chrono::system_clock::to_time_t(target_time);

  if (args.use_current_date) {
    std::cout << "Target: Current time ("
              << std::put_time(std::gmtime(&target_time_t), "%Y-%m-%d %H:%M:%S UTC") << ")\n";
  } else {
    std::cout << "Target: " << args.date_string << " ("
              << std::put_time(std::gmtime(&target_time_t), "%Y-%m-%d %H:%M:%S UTC") << ")\n";
  }

  std::cout << "\n";
}

/**
 * @brief Progress callback for simulation monitoring
 */
void simulation_progress_callback(const Simulation::SimulationState& state) {
  static size_t last_reported_iteration = 0;
  static auto last_report_time = std::chrono::steady_clock::now();

  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_report_time);

  // Report progress every 1000 iterations or every 5 seconds
  if (state.iteration_count - last_reported_iteration >= 1000 || elapsed.count() >= 5) {
    auto current_date = std::chrono::system_clock::from_time_t(
        std::chrono::system_clock::to_time_t(state.reference_time) +
        static_cast<time_t>(state.current_time));
    auto date_time_t = std::chrono::system_clock::to_time_t(current_date);

    std::cout << "Progress: " << state.iteration_count << " steps, "
              << "Date: " << std::put_time(std::gmtime(&date_time_t), "%Y-%m-%d")
              << ", Energy: " << std::scientific << std::setprecision(6) << state.total_energy
              << " J" << std::endl;

    last_reported_iteration = state.iteration_count;
    last_report_time = now;
  }
}

/**
 * @brief Print final simulation results
 */
void print_simulation_results(const Simulation::SimulationEngine& engine,
                              const Bodies::BodyCollection& bodies) {
  const auto& state = engine.get_state();

  std::cout << "\n🎯 Simulation Results\n";
  std::cout << "====================\n";
  std::cout << "Total iterations: " << state.iteration_count << "\n";
  std::cout << "Simulation time: " << std::fixed << std::setprecision(2)
            << state.current_time / (24.0 * 3600.0) << " days\n";

  // Energy analysis
  std::cout << "\n⚡ Energy Analysis:\n";
  std::cout << "Total energy: " << std::scientific << std::setprecision(6) << state.total_energy
            << " J\n";
  std::cout << "Kinetic energy: " << std::scientific << std::setprecision(6) << state.kinetic_energy
            << " J\n";
  std::cout << "Potential energy: " << std::scientific << std::setprecision(6)
            << state.potential_energy << " J\n";

  // System properties
  std::cout << "\n🌍 System Properties:\n";
  std::cout << "Center of mass: " << Math::to_string(state.center_of_mass) << " m\n";
  std::cout << "Total momentum: " << Math::to_string(state.total_momentum) << " kg⋅m/s\n";
  std::cout << "Momentum magnitude: " << std::scientific << std::setprecision(3)
            << state.total_momentum.magnitude() << " kg⋅m/s\n";

  // Timestep statistics
  if (state.iteration_count > 0) {
    std::cout << "\n⏱️ Timestep Statistics:\n";
    std::cout << "Largest timestep: " << std::fixed << std::setprecision(2)
              << state.largest_timestep << " s\n";
    std::cout << "Smallest timestep: " << std::fixed << std::setprecision(2)
              << state.smallest_timestep << " s\n";
    std::cout << "Average timestep: " << std::fixed << std::setprecision(2)
              << state.current_time / state.iteration_count << " s\n";
  }

  // Final positions of key bodies
  std::cout << "\n🪐 Final Positions (key bodies):\n";
  std::vector<std::string> key_bodies = {"Sun", "Earth", "Mars", "Jupiter"};

  for (const auto& name : key_bodies) {
    auto body_ref = bodies.find_body(name);
    if (body_ref.has_value()) {
      const auto& body = body_ref->get();
      std::cout << std::setw(8) << name << ": " << Math::to_string(body.position()) << " m\n";
    }
  }
}

/**
 * @brief Run modern simulation to target date
 */
bool run_modern_simulation(Bodies::BodyFactory& factory, const SimulationArgs& args,
                           std::chrono::system_clock::time_point reference_time,
                           std::chrono::system_clock::time_point target_time) {
  std::cout << "🚀 Initializing modern simulation engine...\n";

  // Create simulation configuration
  Simulation::SimulationConfig config{
      .time_step = 30.0,  // 30 seconds - good balance of speed vs accuracy
      .gravitational_constant = 6.67430e-11,
      .use_adaptive_timestep = false,  // Keep simple for now
      .enable_collision_detection = false};

  // Create simulation engine with Leapfrog integration (good for orbital mechanics)
  Simulation::SimulationEngine engine(config);
  engine.set_integration_method(Simulation::SimulationEngine::IntegrationMethod::LEAPFROG);

  // Set up progress callback
  engine.set_progress_callback(simulation_progress_callback);

  // Create solar system bodies
  std::cout << "🌍 Creating solar system bodies...\n";

  // Determine best data source without triggering unnecessary fetches
  Bodies::BodyFactory::DataSource data_source;
  if (has_current_ephemeris_data()) {
    data_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
    std::cout << "Using cached JPL ephemeris data: " << get_ephemeris_source() << "\n";
  } else {
    data_source = Bodies::BodyFactory::DataSource::FALLBACK_DATA;
    std::cout << "Using fallback ephemeris data (no cached JPL data available)\n";
    if (!has_current_year_ephemeris_data()) {
      std::cout << "💡 Tip: Run with --update-data to fetch current JPL ephemeris data\n";
    }
  }

  Bodies::BodyFactory::CreationOptions body_options{
      .preferred_source = data_source, .allow_fallback = true, .reference_time = reference_time};

  auto solar_system_result = factory.create_solar_system(body_options);
  if (!solar_system_result.has_value()) {
    std::cerr << "❌ Failed to create solar system: " << solar_system_result.error() << std::endl;
    return false;
  }

  auto bodies = std::move(solar_system_result.value());
  std::cout << "✅ Created " << bodies.size() << " celestial bodies\n";

  // Print simulation info
  print_simulation_info(args, engine, bodies);

  // Initialize simulation
  std::cout << "⚙️ Initializing simulation state...\n";
  auto init_result = engine.initialize(std::move(bodies), reference_time);
  if (!init_result.has_value()) {
    std::cerr << "❌ Failed to initialize simulation: " << init_result.error() << std::endl;
    return false;
  }

  // Calculate simulation duration
  auto duration = target_time - reference_time;
  double duration_seconds = std::chrono::duration<double>(duration).count();

  std::cout << "🎯 Running simulation for " << std::fixed << std::setprecision(2)
            << duration_seconds / (24.0 * 3600.0) << " days...\n\n";

  // Run simulation
  auto start_time = std::chrono::steady_clock::now();
  auto sim_result = engine.simulate_to_date(target_time);
  auto end_time = std::chrono::steady_clock::now();

  if (!sim_result.has_value()) {
    std::cerr << "❌ Simulation failed: " << sim_result.error() << std::endl;
    return false;
  }

  // Calculate performance metrics
  auto wall_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  const auto& final_state = engine.get_state();

  std::cout << "\n✅ Simulation completed successfully!\n";
  std::cout << "Wall time: " << wall_time.count() << " ms\n";
  std::cout << "Performance: " << std::fixed << std::setprecision(1)
            << (final_state.iteration_count * 1000.0) / wall_time.count() << " steps/second\n";

  // Print detailed results
  print_simulation_results(engine, engine.get_bodies());

  return true;
}

/**
 * @brief Handle JPL data operations (legacy compatibility)
 */
bool handle_jpl_operations(const SimulationArgs& args) {
  if (args.update_data) {
    std::cout << "🔄 Updating ephemeris data...\n";
    // Use legacy JPL functions for data update (they handle it correctly)
    if (update_ephemeris_data()) {
      std::cout << "✅ Ephemeris data updated successfully\n";
      return true;
    } else {
      std::cerr << "❌ Failed to update ephemeris data\n";
      return false;
    }
  }

  if (args.rebuild_cache) {
    std::cout << "🔄 Rebuilding binary cache...\n";
    if (rebuild_binary_cache()) {
      std::cout << "✅ Binary cache rebuilt successfully\n";
      return true;
    } else {
      std::cerr << "❌ Failed to rebuild binary cache\n";
      return false;
    }
  }

  if (args.test_storage) {
    std::cout << "🧪 Testing storage system...\n";
    if (save_current_data_for_testing()) {
      std::cout << "✅ Storage system test completed successfully\n";
      return true;
    } else {
      std::cerr << "❌ Storage system test failed\n";
      return false;
    }
  }

  return true;
}

/**
 * @brief Main application entry point
 */
int main(int argc, char* argv[]) {
  std::cout << "🌌 Modern Solar System Simulation v4.0\n";
  std::cout << "======================================\n";
  std::cout << "Using modern C++ architecture with SimulationEngine\n\n";

  // Parse command line arguments (using legacy parser for now)
  SimulationArgs args = parse_arguments(argc, argv);

  // Initialize JPL data system (required for legacy compatibility)
  initialize_jpl_data();

  // Set output precision
  std::cout << std::fixed << std::setprecision(6);

  // Handle JPL data operations
  if (args.update_data || args.rebuild_cache || args.test_storage) {
    return handle_jpl_operations(args) ? 0 : 1;
  }

  // Create body factory
  Bodies::BodyFactory factory;

  // Determine reference time (start of simulation)
  auto reference_time = std::chrono::system_clock::now();
  if (!args.use_current_date) {
    // Use a reasonable reference time for historical simulations
    reference_time = std::chrono::system_clock::from_time_t(
        args.target_date - 365 * 24 * 3600);  // 1 year before target
  }

  // Target time
  auto target_time = std::chrono::system_clock::from_time_t(args.target_date);

  // Run the modern simulation
  bool success = run_modern_simulation(factory, args, reference_time, target_time);

  if (success) {
    std::cout << "\n🎉 Modern simulation completed successfully!\n";
    std::cout << "This demonstrates the complete modern C++ architecture:\n";
    std::cout << "  • BodyFactory for flexible data source management\n";
    std::cout << "  • SimulationEngine for accurate N-body physics\n";
    std::cout << "  • BodyCollection for efficient body management\n";
    std::cout << "  • Modern C++ patterns: RAII, move semantics, Expected<T,E>\n";
    return 0;
  } else {
    std::cerr << "\n❌ Simulation failed. Check error messages above.\n";
    return 1;
  }
}
