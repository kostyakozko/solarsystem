/**
 * @file test_advanced_config.cpp
 * @brief Unit tests for advanced configuration management system
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "solar_utils/advanced_config.hpp"

using namespace SolarSystem::Utils::Advanced;
using namespace SolarSystem::Utils;

// Test helper functions
std::unique_ptr<AdvancedConfigManager> create_manager() {
  return std::make_unique<AdvancedConfigManager>();
}

std::filesystem::path create_test_directory() {
  auto test_dir = std::filesystem::temp_directory_path() / "solar_system_test";
  std::filesystem::create_directories(test_dir);
  return test_dir;
}

void cleanup_test_directory(const std::filesystem::path& test_dir) {
  if (std::filesystem::exists(test_dir)) {
    std::filesystem::remove_all(test_dir);
  }
}
TEST(AdvancedConfigurationManagementTests, Parameter_Validation) {
  auto manager = create_manager();

  // Test valid integer parameter
  auto result = manager->validate_parameter("simulation.max_iterations", "1000000");
  ASSERT_TRUE(result.is_valid);

  // Test invalid integer parameter
  result = manager->validate_parameter("simulation.max_iterations", "invalid");
  ASSERT_FALSE(result.is_valid);

  // Test out of range parameter
  result = manager->validate_parameter("simulation.max_iterations", "-1");
  ASSERT_FALSE(result.is_valid);

  // Test valid double parameter
  result = manager->validate_parameter("simulation.timestep", "3600.0");
  ASSERT_TRUE(result.is_valid);

  // Test invalid double parameter
  result = manager->validate_parameter("simulation.timestep", "not_a_number");
  ASSERT_FALSE(result.is_valid);

  // Test valid string parameter with allowed values
  result = manager->validate_parameter("simulation.output_format", "json");
  ASSERT_TRUE(result.is_valid);

  // Test invalid string parameter
  result = manager->validate_parameter("simulation.output_format", "invalid_format");
  ASSERT_FALSE(result.is_valid);
}

TEST(AdvancedConfigurationManagementTests, Configuration_Validation) {
  auto manager = create_manager();
  Config::AppConfig config = Config::get_default();

  // Test valid configuration
  auto result = manager->validate_configuration(config);
  ASSERT_TRUE(result.is_valid);

  // Test invalid configuration - negative timestep
  config.simulation.timestep = -1.0;
  result = manager->validate_configuration(config);
  ASSERT_FALSE(result.is_valid);
  ASSERT_FALSE(result.errors.empty());

  // Test configuration with warnings
  config = Config::get_default();
  config.simulation.timestep = 0.1;  // Very small timestep should generate warning
  result = manager->validate_configuration(config);
  ASSERT_TRUE(result.is_valid);  // Still valid but should have warnings
  ASSERT_FALSE(result.warnings.empty());
}

TEST(AdvancedConfigurationManagementTests, Conflict_Detection) {
  auto manager = create_manager();
  Config::AppConfig config = Config::get_default();

  // Test configuration without conflicts
  auto conflicts = manager->detect_conflicts(config);
  ASSERT_TRUE(conflicts.empty());
}

TEST(AdvancedConfigurationManagementTests, Template_Application) {
  auto manager = create_manager();

  // Test applying development template
  auto result = manager->apply_template("development");
  ASSERT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    ASSERT_TRUE(config.debug_mode);
    ASSERT_TRUE(config.logging.min_level == ConfigLogLevel::DEBUG);
    ASSERT_TRUE(config.logging.colored_output);
  }

  // Test applying production template
  result = manager->apply_template("production");
  ASSERT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    ASSERT_FALSE(config.debug_mode);
    ASSERT_TRUE(config.logging.min_level == ConfigLogLevel::INFO);
    ASSERT_FALSE(config.logging.colored_output);
  }

  // Test applying non-existent template
  result = manager->apply_template("non_existent");
  ASSERT_FALSE(result.has_value());
}

TEST(AdvancedConfigurationManagementTests, Basic_Functionality) {
  auto manager = create_manager();

  // Test that manager can be created
  ASSERT_NE(nullptr, manager);

  // Test getting available templates
  auto templates = manager->get_available_templates();
  // Should have at least some built-in templates

  // Test getting available presets
  auto presets = manager->get_available_presets();
  // Should have at least some built-in presets

  // Test parameter definitions
  auto definitions = manager->get_parameter_definitions();
  // Should have parameter definitions
}
