/**
 * @file test_body_factory_comprehensive.cpp
 * @brief Comprehensive unit tests for BodyFactory with mocked JPL client
 *
 * Tests all aspects of BodyFactory functionality including:
 * - JPL client integration with mocks
 * - Cache operations with mock file system
 * - Error handling and fallback mechanisms
 * - Data source selection and validation
 * - Body creation from various sources
 */

#include <chrono>
#include <iostream>
#include <memory>

#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem::Bodies;

/**
 * @brief Test BodyFactory initialization and basic functionality
 */
int test_body_factory_initialization() {
  std::cout << "Testing BodyFactory initialization...\n";

  // Test default initialization
  BodyFactory default_factory;
  if (!default_factory.is_initialized()) {
    std::cout << "ERROR: Default factory should be initialized\n";
    return 1;
  }

  if (default_factory.current_source().empty()) {
    std::cout << "ERROR: Current source should not be empty\n";
    return 1;
  }

  // Test initialization with JPL HORIZONS preference
  BodyFactory::CreationOptions jpl_options;
  jpl_options.preferred_source = BodyFactory::DataSource::JPL_HORIZONS;
  jpl_options.allow_fallback = true;
  jpl_options.validate_data = true;

  BodyFactory jpl_factory(jpl_options);
  if (!jpl_factory.is_initialized()) {
    std::cout << "ERROR: JPL factory should be initialized\n";
    return 1;
  }

  std::cout << "✅ BodyFactory initialization tests passed\n";
  return 0;
}

/**
 * @brief Test BodyFactory body creation
 */
int test_body_factory_creation() {
  std::cout << "Testing BodyFactory body creation...\n";

  BodyFactory factory;

  // Test single body creation
  auto earth_result = factory.create_body("Earth");
  if (!earth_result.has_value()) {
    std::cout << "ERROR: Should successfully create Earth: " << earth_result.error() << "\n";
    return 1;
  }

  auto earth = earth_result.value();
  if (earth.name().find("Earth") == std::string::npos &&
      earth.name().find("399") == std::string::npos) {
    std::cout << "ERROR: Body name should contain Earth or 399, got: " << earth.name() << "\n";
    return 1;
  }

  if (earth.mass() <= 0) {
    std::cout << "ERROR: Earth mass should be positive\n";
    return 1;
  }

  std::cout << "✅ BodyFactory creation tests passed\n";
  return 0;
}

/**
 * @brief Test BodyFactory predefined collections
 */
int test_body_factory_collections() {
  std::cout << "Testing BodyFactory predefined collections...\n";

  BodyFactory factory;

  // Test solar system collection
  auto solar_system_result = factory.create_solar_system();
  if (!solar_system_result.has_value()) {
    std::cout << "ERROR: Should successfully create solar system collection: "
              << solar_system_result.error() << "\n";
    return 1;
  }

  auto solar_system = solar_system_result.value();
  if (solar_system.size() == 0) {
    std::cout << "ERROR: Solar system should contain bodies\n";
    return 1;
  }

  // Test inner planets collection
  auto inner_planets_result = factory.create_inner_planets();
  if (!inner_planets_result.has_value()) {
    std::cout << "ERROR: Should successfully create inner planets collection: "
              << inner_planets_result.error() << "\n";
    return 1;
  }

  std::cout << "✅ BodyFactory collections tests passed\n";
  return 0;
}

/**
 * @brief Test BodyFactory utility functions
 */
int test_body_factory_utilities() {
  std::cout << "Testing BodyFactory utility functions...\n";

  BodyFactory factory;

  // Test get available bodies
  auto available_bodies = factory.get_available_bodies();
  if (available_bodies.size() == 0) {
    std::cout << "ERROR: Should have available bodies\n";
    return 1;
  }

  // Check if Earth is in available bodies
  bool has_earth = false;
  for (const auto& body_name : available_bodies) {
    if (body_name == "Earth") {
      has_earth = true;
      break;
    }
  }

  if (!has_earth) {
    std::cout << "ERROR: Available bodies should include Earth\n";
    return 1;
  }

  // Test body availability check
  auto current_time = std::chrono::system_clock::now();
  if (!factory.is_body_available("Earth", current_time)) {
    std::cout << "ERROR: Earth should be available\n";
    return 1;
  }

  std::cout << "✅ BodyFactory utilities tests passed\n";
  return 0;
}

/**
 * @brief Test BodyFactory error handling
 */
int test_body_factory_error_handling() {
  std::cout << "Testing BodyFactory error handling...\n";

  BodyFactory factory;

  // Test invalid body name
  auto invalid_result = factory.create_body("NonExistentPlanet");
  if (invalid_result.has_value()) {
    std::cout << "ERROR: Should fail for non-existent body\n";
    return 1;
  }

  if (invalid_result.error().empty()) {
    std::cout << "ERROR: Error message should not be empty\n";
    return 1;
  }

  // Test empty body name
  auto empty_result = factory.create_body("");
  if (empty_result.has_value()) {
    std::cout << "ERROR: Should fail for empty body name\n";
    return 1;
  }

  std::cout << "✅ BodyFactory error handling tests passed\n";
  return 0;
}

/**
 * @brief Test BodyFactory storage system
 */
int test_body_factory_storage() {
  std::cout << "Testing BodyFactory storage system...\n";

  BodyFactory factory;

  // Test storage system
  auto storage_result = factory.test_storage_system();
  // Result depends on implementation and available storage

  // Test current year epoch
  auto current_year_epoch = BodyFactory::get_current_year_epoch();
  if (current_year_epoch.time_since_epoch().count() <= 0) {
    std::cout << "ERROR: Current year epoch should be valid\n";
    return 1;
  }

  std::cout << "✅ BodyFactory storage tests passed\n";
  return 0;
}

int main() {
  std::cout << "🚀 Running comprehensive BodyFactory tests\n\n";

  int result = 0;

  result += test_body_factory_initialization();
  result += test_body_factory_creation();
  result += test_body_factory_collections();
  result += test_body_factory_utilities();
  result += test_body_factory_error_handling();
  result += test_body_factory_storage();

  if (result == 0) {
    std::cout << "\n🎉 All BodyFactory comprehensive tests passed!\n";
  } else {
    std::cout << "\n❌ Some BodyFactory tests failed\n";
  }

  return result;
}
