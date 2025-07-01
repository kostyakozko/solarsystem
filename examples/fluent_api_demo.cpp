/**
 * @file fluent_api_demo.cpp
 * @brief Demonstration of Phase 0.3 Fluent API and Builder Patterns
 *
 * This example showcases the modern, chainable API for building and configuring
 * solar system simulations with type safety, validation, and clean syntax.
 */

#include <chrono>
#include <iostream>

#include "solar_core/builders/simulation_builder.hpp"
#include "solar_utils/config.hpp"
#include "solar_utils/logging.hpp"

using namespace SolarSystem::Core::Builders;
using namespace SolarSystem::Utils;

/**
 * @brief Progress callback for simulation monitoring
 */
void progress_monitor(double progress) {
  static int last_percent = -1;
  int current_percent = static_cast<int>(progress * 100);

  if (current_percent != last_percent && current_percent % 10 == 0) {
    LOG_INFO("Progress", "Simulation " + std::to_string(current_percent) + "% complete");
    last_percent = current_percent;
  }
}

/**
 * @brief Custom validation for ensuring we have essential bodies
 */
bool validate_essential_bodies(const SolarSystem::Core::Bodies::BodyCollection& bodies) {
  // Check that we have at least the Sun and Earth
  bool has_sun = false, has_earth = false;

  for (const auto& body : bodies) {
    if (body.name() == "Sun") has_sun = true;
    if (body.name() == "Earth") has_earth = true;
  }

  return has_sun && has_earth;
}

int main() {
  std::cout << "🚀 Solar System Suite - Fluent API Demo (Phase 0.3)" << std::endl;
  std::cout << "===================================================" << std::endl;

  try {
    // Initialize logging
    Logger::Config log_config;
    log_config.min_level = Logger::Level::INFO;
    log_config.colored_output = true;
    log_config.include_timestamp = true;
    Logger::instance().configure(log_config);

    LOG_INFO("Demo", "Starting Fluent API demonstration");

    // === EXAMPLE 1: Basic Fluent Interface ===
    std::cout << "\n1️⃣ Basic Fluent Interface Example" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    auto basic_simulation = SimulationBuilder()
                                .with_bodies(BodySelector().essential().build().value())
                                .with_timestep(3600.0)  // 1 hour
                                .with_target_date("2025-12-31")
                                .with_progress(true)
                                .build();

    if (basic_simulation.has_value()) {
      std::cout << "✅ Basic simulation built successfully" << std::endl;
      std::cout << "   Bodies: " << basic_simulation.value()->get_body_count() << std::endl;
    } else {
      std::cout << "❌ Failed to build basic simulation: " << basic_simulation.error() << std::endl;
    }

    // === EXAMPLE 2: Advanced Configuration with Callbacks ===
    std::cout << "\n2️⃣ Advanced Configuration Example" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    auto advanced_bodies =
        BodySelector()
            .essential()
            .important()
            .excluding({"SpaceX Roadster", "New Horizons"})  // Exclude spacecraft
            .build();

    if (advanced_bodies.has_value()) {
      auto advanced_simulation = SimulationBuilder()
                                     .with_bodies(std::move(*advanced_bodies))
                                     .with_timestep(1800.0)  // 30 minutes for higher accuracy
                                     .with_target_date("2025-07-04")  // Independence Day 2025
                                     .with_progress_callback(progress_monitor)
                                     .with_convergence_threshold(1e-14)
                                     .with_verbose_output(true)
                                     .with_validation(validate_essential_bodies)
                                     .build_and_run();

      if (advanced_simulation.has_value()) {
        std::cout << "✅ Advanced simulation completed successfully" << std::endl;
        std::cout << "   Final body count: " << advanced_simulation->size() << std::endl;
      } else {
        std::cout << "❌ Advanced simulation failed: " << advanced_simulation.error() << std::endl;
      }
    }

    // === EXAMPLE 3: Configuration Builder Pattern ===
    std::cout << "\n3️⃣ Configuration Builder Pattern" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    auto high_accuracy_config = ConfigurationBuilder()
                                    .high_accuracy()
                                    .timestep_minutes(15)  // Override to 15 minutes
                                    .progress(true)
                                    .verbose(true)
                                    .build();

    auto config_simulation = SimulationBuilder(high_accuracy_config)
                                 .with_bodies(BodySelector().all().build().value())
                                 .with_target_date("2024-01-01")  // Historical date
                                 .build();

    if (config_simulation.has_value()) {
      std::cout << "✅ Configuration-based simulation built" << std::endl;
      std::cout << "   Configuration summary:" << std::endl;
      std::cout << "   - Timestep: " << high_accuracy_config.timestep << " seconds" << std::endl;
      std::cout << "   - Max iterations: " << high_accuracy_config.max_iterations << std::endl;
      std::cout << "   - Convergence threshold: " << high_accuracy_config.convergence_threshold
                << std::endl;
    }

    // === EXAMPLE 4: Body Selection Patterns ===
    std::cout << "\n4️⃣ Body Selection Patterns" << std::endl;
    std::cout << "-------------------------" << std::endl;

    // Select only planets
    auto planets_only = BodySelector().of_type(SolarSystem::Core::Bodies::BodyType::Planet).build();

    // Select by names
    auto specific_bodies =
        BodySelector().named({"Sun", "Earth", "Moon", "Mars", "Jupiter"}).build();

    // Custom filter
    auto large_bodies = BodySelector()
                            .where([](const auto& body) {
                              return body.mass() > 1e24;  // Bodies larger than 10^24 kg
                            })
                            .build();

    std::cout << "   Planets only: " << (planets_only.has_value() ? planets_only->size() : 0)
              << " bodies" << std::endl;
    std::cout << "   Specific selection: "
              << (specific_bodies.has_value() ? specific_bodies->size() : 0) << " bodies"
              << std::endl;
    std::cout << "   Large bodies: " << (large_bodies.has_value() ? large_bodies->size() : 0)
              << " bodies" << std::endl;

    // === EXAMPLE 5: Preset Configurations ===
    std::cout << "\n5️⃣ Preset Configurations" << std::endl;
    std::cout << "-----------------------" << std::endl;

    // High performance preset
    auto performance_config = ConfigurationBuilder().high_performance().build();

    // Real-time preset
    auto realtime_config = ConfigurationBuilder().real_time().build();

    // Balanced preset
    auto balanced_config = ConfigurationBuilder().balanced().build();

    std::cout << "   High Performance - Timestep: " << performance_config.timestep << "s"
              << std::endl;
    std::cout << "   Real-time - Timestep: " << realtime_config.timestep << "s" << std::endl;
    std::cout << "   Balanced - Timestep: " << balanced_config.timestep << "s" << std::endl;

    // === EXAMPLE 6: Validation and Error Handling ===
    std::cout << "\n6️⃣ Validation and Error Handling" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    // Try to build invalid configuration
    auto invalid_simulation = SimulationBuilder()
                                  .with_timestep(-100)     // Invalid negative timestep
                                  .with_max_iterations(0)  // Invalid zero iterations
                                  .build();

    if (!invalid_simulation.has_value()) {
      std::cout << "✅ Validation correctly caught invalid configuration:" << std::endl;
      std::cout << "   Error: " << invalid_simulation.error() << std::endl;
    }

    // === EXAMPLE 7: Integration with Global Configuration ===
    std::cout << "\n7️⃣ Global Configuration Integration" << std::endl;
    std::cout << "----------------------------------" << std::endl;

    // Initialize global config
    auto app_config = Config::get_default();
    app_config.simulation.timestep = 2700.0;  // 45 minutes
    app_config.simulation.verbose_output = true;
    GlobalConfig::initialize(app_config);

    auto global_simulation = SimulationBuilder()
                                 .from_global_config()  // Use global configuration
                                 .with_bodies(BodySelector().essential().build().value())
                                 .with_target_date("2025-06-21")  // Summer solstice
                                 .build();

    if (global_simulation.has_value()) {
      std::cout << "✅ Global configuration integration successful" << std::endl;
    }

    std::cout << "\n🎉 Fluent API Demo Complete!" << std::endl;
    std::cout << "All examples demonstrate the power and flexibility of the new API design."
              << std::endl;

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "❌ Demo failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
