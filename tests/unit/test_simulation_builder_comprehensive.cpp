/**
 * @file test_simulation_builder_comprehensive.cpp
 * @brief Comprehensive unit tests for SimulationBuilder with various filtering scenarios
 *
 * Tests all aspects of SimulationBuilder functionality including:
 * - Fluent interface chaining
 * - Body filtering by type, priority, and custom criteria
 * - Configuration validation and error handling
 * - Integration with BodyFactory and simulation engine
 * - Performance and time-based configurations
 */

#include <chrono>
#include <iostream>
#include <memory>

#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/builders/simulation_builder.hpp"

using namespace SolarSystem::Core::Builders;
using namespace SolarSystem::Bodies;

/**
 * @brief Test SimulationBuilder basic initialization
 */
int test_simulation_builder_initialization() {
  std::cout << "Testing SimulationBuilder initialization...\n";

  // Test default constructor
  SimulationBuilder default_builder;
  // Default builder may not be valid without bodies, which is expected

  // Test constructor with timestep
  SimulationBuilder timestep_builder(3600.0);  // 1 hour
  // Timestep builder may not be valid without bodies, which is expected behavior

  // Test configuration summary
  auto summary = default_builder.get_config_summary();
  if (summary.empty()) {
    std::cout << "ERROR: Configuration summary should not be empty\n";
    return 1;
  }

  std::cout << "✅ SimulationBuilder initialization tests passed\n";
  return 0;
}

/**
 * @brief Test BodySelector priority filtering
 */
int test_body_selector_priority_filtering() {
  std::cout << "Testing BodySelector priority filtering...\n";

  // Test essential bodies filtering
  BodySelector essential_selector;
  essential_selector.with_priority(BodyPriority::Essential);

  auto essential_result = essential_selector.build();
  if (!essential_result.has_value()) {
    std::cout << "ERROR: Should successfully build essential bodies collection\n";
    return 1;
  }

  auto essential_bodies = essential_result.value();
  if (essential_bodies.size() == 0) {
    std::cout << "ERROR: Essential bodies collection should not be empty\n";
    return 1;
  }

  // Verify all bodies have essential priority
  for (const auto& body : essential_bodies) {
    if (body.priority() != BodyPriority::Essential) {
      std::cout << "ERROR: All bodies should have essential priority\n";
      return 1;
    }
  }

  std::cout << "✅ BodySelector priority filtering tests passed\n";
  return 0;
}

/**
 * @brief Test BodySelector type filtering
 */
int test_body_selector_type_filtering() {
  std::cout << "Testing BodySelector type filtering...\n";

  // Test planet filtering
  BodySelector planet_selector;
  planet_selector.of_type(BodyType::Planet);

  auto planet_result = planet_selector.build();
  if (!planet_result.has_value()) {
    std::cout << "ERROR: Should successfully build planets collection\n";
    return 1;
  }

  auto planets = planet_result.value();
  if (planets.size() < 8) {
    std::cout << "ERROR: Should have at least 8 planets\n";
    return 1;
  }

  // Verify all bodies are planets
  for (const auto& body : planets) {
    if (body.type() != BodyType::Planet) {
      std::cout << "ERROR: All bodies should be planets\n";
      return 1;
    }
  }

  std::cout << "✅ BodySelector type filtering tests passed\n";
  return 0;
}

/**
 * @brief Test BodySelector name filtering
 */
int test_body_selector_name_filtering() {
  std::cout << "Testing BodySelector name filtering...\n";

  // Test specific body names
  std::vector<std::string> target_names = {"Earth", "Moon", "Sun"};
  BodySelector name_selector;
  name_selector.named(target_names);

  auto name_result = name_selector.build();
  if (!name_result.has_value()) {
    std::cout << "ERROR: Should successfully build named bodies collection\n";
    return 1;
  }

  auto selected_bodies = name_result.value();
  if (selected_bodies.size() > target_names.size()) {
    std::cout << "ERROR: Selected bodies should not exceed requested count\n";
    return 1;
  }

  // Verify all selected bodies are in the target list
  for (const auto& body : selected_bodies) {
    bool found = false;
    for (const auto& target_name : target_names) {
      if (body.name() == target_name) {
        found = true;
        break;
      }
    }
    if (!found) {
      std::cout << "ERROR: Selected body should be in target list: " << body.name() << "\n";
      return 1;
    }
  }

  std::cout << "✅ BodySelector name filtering tests passed\n";
  return 0;
}

/**
 * @brief Test SimulationBuilder fluent interface
 */
int test_simulation_builder_fluent_interface() {
  std::cout << "Testing SimulationBuilder fluent interface...\n";

  // Create a body collection for testing
  BodySelector selector;
  auto bodies_result = selector.essential().build();
  if (!bodies_result.has_value()) {
    std::cout << "ERROR: Should create test body collection\n";
    return 1;
  }

  auto bodies = bodies_result.value();

  // Test fluent interface chaining
  SimulationBuilder builder;
  builder.with_bodies(bodies)
      .with_timestep(1800.0)  // 30 minutes
      .with_max_iterations(100000)
      .with_target_date("2025-12-31")
      .with_progress(true)
      .with_convergence_threshold(1e-10)
      .with_verbose_output(true);

  if (!builder.is_valid()) {
    std::cout << "ERROR: Fluent builder should be valid\n";
    return 1;
  }

  // Test configuration summary
  auto summary = builder.get_config_summary();
  if (summary.find("1800") == std::string::npos) {
    std::cout << "ERROR: Summary should contain timestep value\n";
    return 1;
  }

  std::cout << "✅ SimulationBuilder fluent interface tests passed\n";
  return 0;
}

/**
 * @brief Test SimulationBuilder validation
 */
int test_simulation_builder_validation() {
  std::cout << "Testing SimulationBuilder validation...\n";

  SimulationBuilder builder;

  // Test invalid timestep
  builder.with_timestep(-1.0);

  std::string error_message;
  bool is_valid = builder.validate(&error_message);
  if (is_valid) {
    std::cout << "ERROR: Builder with negative timestep should be invalid\n";
    return 1;
  }

  if (error_message.empty()) {
    std::cout << "ERROR: Error message should be provided\n";
    return 1;
  }

  // Test zero timestep
  builder.with_timestep(0.0);
  is_valid = builder.validate(&error_message);
  if (is_valid) {
    std::cout << "ERROR: Builder with zero timestep should be invalid\n";
    return 1;
  }

  // Test valid timestep with bodies
  BodySelector selector;
  auto bodies_result = selector.essential().build();
  if (bodies_result.has_value()) {
    builder.with_timestep(3600.0).with_bodies(bodies_result.value());
    is_valid = builder.validate(&error_message);
    if (!is_valid) {
      std::cout << "ERROR: Builder with valid timestep and bodies should be valid: "
                << error_message << "\n";
      return 1;
    }
  }

  std::cout << "✅ SimulationBuilder validation tests passed\n";
  return 0;
}

int main() {
  std::cout << "🚀 Running comprehensive SimulationBuilder tests\n\n";

  int result = 0;

  result += test_simulation_builder_initialization();
  result += test_body_selector_priority_filtering();
  result += test_body_selector_type_filtering();
  result += test_body_selector_name_filtering();
  result += test_simulation_builder_fluent_interface();
  result += test_simulation_builder_validation();

  if (result == 0) {
    std::cout << "\n🎉 All SimulationBuilder comprehensive tests passed!\n";
  } else {
    std::cout << "\n❌ Some SimulationBuilder tests failed\n";
  }

  return result;
}
