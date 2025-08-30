/**
 * @file test_advanced_config.cpp
 * @brief Unit tests for advanced configuration management system
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "solar_utils/advanced_config.hpp"

using namespace SolarSystem::Utils::Advanced;
using namespace SolarSystem::Utils;

class AdvancedConfigTest : public ::testing::Test {
protected:
  void SetUp() override {
    manager_ = std::make_unique<AdvancedConfigManager>();

    // Create temporary directory for test files
    test_dir_ = std::filesystem::temp_directory_path() / "solar_system_test";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override {
    // Clean up test directory
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  std::unique_ptr<AdvancedConfigManager> manager_;
  std::filesystem::path test_dir_;
};

// Test parameter validation
TEST_F(AdvancedConfigTest, ParameterValidation) {
  // Test valid integer parameter
  auto result = manager_->validate_parameter("simulation.max_iterations", "1000000");
  EXPECT_TRUE(result.is_valid);

  // Test invalid integer parameter
  result = manager_->validate_parameter("simulation.max_iterations", "invalid");
  EXPECT_FALSE(result.is_valid);

  // Test out of range parameter
  result = manager_->validate_parameter("simulation.max_iterations", "-1");
  EXPECT_FALSE(result.is_valid);

  // Test valid double parameter
  result = manager_->validate_parameter("simulation.timestep", "3600.0");
  EXPECT_TRUE(result.is_valid);

  // Test invalid double parameter
  result = manager_->validate_parameter("simulation.timestep", "not_a_number");
  EXPECT_FALSE(result.is_valid);

  // Test valid string parameter with allowed values
  result = manager_->validate_parameter("simulation.output_format", "json");
  EXPECT_TRUE(result.is_valid);

  // Test invalid string parameter
  result = manager_->validate_parameter("simulation.output_format", "invalid_format");
  EXPECT_FALSE(result.is_valid);
}

// Test configuration validation
TEST_F(AdvancedConfigTest, ConfigurationValidation) {
  Config::AppConfig config = Config::get_default();

  // Test valid configuration
  auto result = manager_->validate_configuration(config);
  EXPECT_TRUE(result.is_valid);

  // Test invalid configuration - negative timestep
  config.simulation.timestep = -1.0;
  result = manager_->validate_configuration(config);
  EXPECT_FALSE(result.is_valid);
  EXPECT_FALSE(result.errors.empty());

  // Test configuration with warnings
  config = Config::get_default();
  config.simulation.timestep = 0.1;  // Very small timestep should generate warning
  result = manager_->validate_configuration(config);
  EXPECT_TRUE(result.is_valid);  // Still valid but should have warnings
  EXPECT_FALSE(result.warnings.empty());
}

// Test conflict detection
TEST_F(AdvancedConfigTest, ConflictDetection) {
  Config::AppConfig config = Config::get_default();

  // Test configuration without conflicts
  auto conflicts = manager_->detect_conflicts(config);
  EXPECT_TRUE(conflicts.empty());

  // Note: Since we don't have actual conflicting parameters in the default config,
  // this test would need to be enhanced with specific conflicting parameters
  // For now, west that the method works without crashing
}

// Test template application
TEST_F(AdvancedConfigTest, TemplateApplication) {
  // Test applying development template
  auto result = manager_->apply_template("development");
  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_TRUE(config.debug_mode);
    EXPECT_EQ(config.logging.min_level, Logger::Level::DEBUG);
    EXPECT_TRUE(config.logging.colored_output);
  }

  // Test applying production template
  result = manager_->apply_template("production");
  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_FALSE(config.debug_mode);
    EXPECT_EQ(config.logging.min_level, Logger::Level::INFO);
    EXPECT_FALSE(config.logging.colored_output);
  }

  // Test applying non-existent template
  result = manager_->apply_template("non_existent");
  EXPECT_FALSE(result.has_value());
}

// Test preset loading
TEST_F(AdvancedConfigTest, PresetLoading) {
  // Test loading fast simulation preset
  auto result = manager_->load_preset("fast_simulation");
  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 7200.0);  // 2 hours
    EXPECT_EQ(config.simulation.max_iterations, 100000u);
  }

  // Test loading high accuracy preset
  result = manager_->load_preset("high_accuracy");
  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 900.0);  // 15 minutes
    EXPECT_EQ(config.simulation.max_iterations, 10000000u);
  }

  // Test loading non-existent preset
  result = manager_->load_preset("non_existent");
  EXPECT_FALSE(result.has_value());
}

// Test preset saving
TEST_F(AdvancedConfigTest, PresetSaving) {
  Config::AppConfig config = Config::get_default();
  config.simulation.timestep = 1800.0;  // 30 minutes
  config.simulation.max_iterations = 500000;

  // Save custom preset
  auto result = manager_->save_preset("test_preset", config, "Test preset", "testing");
  EXPECT_TRUE(result.has_value());

  // Load the saved preset
  auto load_result = manager_->load_preset("test_preset");
  EXPECT_TRUE(load_result.has_value());

  if (load_result.has_value()) {
    const auto& loaded_config = load_result.value();
    EXPECT_EQ(loaded_config.simulation.timestep, 1800.0);
    EXPECT_EQ(loaded_config.simulation.max_iterations, 500000u);
  }
}

// Test configuration builder
TEST_F(AdvancedConfigTest, ConfigurationBuilder) {
  // Test basic builder usage
  ConfigurationBuilder builder;
  auto result = builder
    .simulation_timestep(1800.0)
    .simulation_max_iterations(500000)
    .simulation_output_format("json")
    .data_cache_directory("./test_cache")
    .web_port(9090)
    .build();

  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 1800.0);
    EXPECT_EQ(config.simulation.max_iterations, 500000u);
    EXPECT_EQ(config.simulation.output_format, "json");
    EXPECT_EQ(config.data.cache_directory, "./test_cache");
    EXPECT_EQ(config.web.port, 9090);
  }

  // Test builder with template
  result = builder
    .from_template("development")
    .simulation_timestep(3600.0)  // Override template value
    .build();

  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 3600.0);  // Should use overridden value
    EXPECT_TRUE(config.debug_mode);  // Should inherit from template
  }

  // Test builder with invalid values
  result = builder
    .simulation_timestep(-1.0)  // Invalid timestep
    .build();

  EXPECT_FALSE(result.has_value());  // Should fail validation
}

// Test configuration migration
TEST_F(AdvancedConfigTest, ConfigurationMigration) {
  // Create old configuration with deprecated parameters
  Config::AppConfig old_config = Config::get_default();

  // Test migration (this is a simplified test since we don't have actual deprecated parameters)
  auto result = manager_->migrate_configuration(old_config, "3.0", "4.0");
  EXPECT_TRUE(result.has_value());

  // The migrated configuration should be valid
  if (result.has_value()) {
    auto validation_result = manager_->validate_configuration(result.value());
    EXPECT_TRUE(validation_result.is_valid);
  }
}

// Test comprehensive configuration loading
TEST_F(AdvancedConfigTest, ComprehensiveConfigurationLoading) {
  // Create a test configuration file
  auto config_file = test_dir_ / "test_config.ini";
  std::ofstream file(config_file);
  file << "[simulation]\n";
  file << "timestep = 1800.0\n";
  file << "max_iterations = 500000\n";
  file << "output_format = \"json\"\n";
  file << "\n[data]\n";
  file << "cache_directory = \"./test_cache\"\n";
  file << "\n[web]\n";
  file << "port = 9090\n";
  file.close();

  // Test loading configuration with file
  std::vector<std::string> cli_args = {"--verbose=true"};
  auto result = manager_->load_configuration(config_file.string(), cli_args);

  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 1800.0);
    EXPECT_EQ(config.simulation.max_iterations, 500000u);
    EXPECT_EQ(config.simulation.output_format, "json");
    EXPECT_EQ(config.data.cache_directory, "./test_cache");
    EXPECT_EQ(config.web.port, 9090);
  }

  // Test loading with preset
  result = manager_->load_configuration(std::nullopt, {}, "fast_simulation");
  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 7200.0);  // From preset
  }

  // Test loading with template
  result = manager_->load_configuration(std::nullopt, {}, "", "development");
  EXPECT_TRUE(result.has_value());

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_TRUE(config.debug_mode);  // From template
  }
}

// Test available templates and presets
TEST_F(AdvancedConfigTest, AvailableTemplatesAndPresets) {
  // Test getting available templates
  auto templates = manager_->get_available_templates();
  EXPECT_FALSE(templates.empty());

  // Check for expected templates
  bool found_development = false;
  bool found_production = false;
  for (const auto& template_def : templates) {
    if (template_def.name == "development") found_development = true;
    if (template_def.name == "production") found_production = true;
  }
  EXPECT_TRUE(found_development);
  EXPECT_TRUE(found_production);

  // Test getting available presets
  auto presets = manager_->get_available_presets();
  EXPECT_FALSE(presets.empty());

  // Check for expected presets
  bool found_fast = false;
  bool found_accurate = false;
  for (const auto& preset_def : presets) {
    if (preset_def.name == "fast_simulation") found_fast = true;
    if (preset_def.name == "high_accuracy") found_accurate = true;
  }
  EXPECT_TRUE(found_fast);
  EXPECT_TRUE(found_accurate);
}

// Test parameter definitions
TEST_F(AdvancedConfigTest, ParameterDefinitions) {
  auto definitions = manager_->get_parameter_definitions();
  EXPECT_FALSE(definitions.empty());

  // Check for expected parameter definitions
  bool found_timestep = false;
  bool found_max_iterations = false;
  for (const auto& param_def : definitions) {
    if (param_def.name == "simulation.timestep") {
      found_timestep = true;
      EXPECT_EQ(param_def.type, "double");
      EXPECT_FALSE(param_def.required);
    }
    if (param_def.name == "simulation.max_iterations") {
      found_max_iterations = true;
      EXPECT_EQ(param_def.type, "int");
      EXPECT_FALSE(param_def.required);
    }
  }
  EXPECT_TRUE(found_timestep);
  EXPECT_TRUE(found_max_iterations);
}

// Test documentation generation
TEST_F(AdvancedConfigTest, DocumentationGeneration) {
  auto documentation = manager_->generate_documentation();
  EXPECT_FALSE(documentation.empty());

  // Check that documentation contains expected sections
  EXPECT_NE(documentation.find("Configuration Parameters"), std::string::npos);
  EXPECT_NE(documentation.find("Configuration Templates"), std::string::npos);
  EXPECT_NE(documentation.find("Configuration Presets"), std::string::npos);

  // Check that specific parameters are documented
  EXPECT_NE(documentation.find("simulation.timestep"), std::string::npos);
  EXPECT_NE(documentation.find("simulation.max_iterations"), std::string::npos);
}

// Test configuration validator
TEST_F(AdvancedConfigTest, ConfigurationValidator) {
  Config::AppConfig config = Config::get_default();

  // Test valid simulation config
  auto result = ConfigurationValidator::validate_simulation_config(config.simulation);
  EXPECT_TRUE(result.is_valid);

  // Test invalid simulation config
  Config::SimulationConfig invalid_sim_config = config.simulation;
  invalid_sim_config.timestep = -1.0;
  result = ConfigurationValidator::validate_simulation_config(invalid_sim_config);
  EXPECT_FALSE(result.is_valid);
  EXPECT_FALSE(result.errors.empty());

  // Test data config validation
  result = ConfigurationValidator::validate_data_config(config.data);
  EXPECT_TRUE(result.is_valid);

  // Test logging config validation
  result = ConfigurationValidator::validate_logging_config(config.logging);
  EXPECT_TRUE(result.is_valid);

  // Test web config validation
  result = ConfigurationValidator::validate_web_config(config.web);
  EXPECT_TRUE(result.is_valid);
}

// Test conflict resolution
TEST_F(AdvancedConfigTest, ConflictResolution) {
  Config::AppConfig config = Config::get_default();

  // Test resolving conflicts (simplified test since we don't have actual conflicts)
  auto result = manager_->resolve_conflicts(config);
  EXPECT_TRUE(result.has_value());

  // Test with resolution preferences
  std::vector<std::string> preferences = {"simulation.verbose_output"};
  result = manager_->resolve_conflicts(config, preferences);
  EXPECT_TRUE(result.has_value());
}

// Test error handling
TEST_F(AdvancedConfigTest, ErrorHandling) {
  // Test loading non-existent configuration file
  auto result = manager_->load_configuration("/non/existent/file.ini");
  // Should still succeed with default configuration
  EXPECT_TRUE(result.has_value());

  // Test applying non-existent template
  result = manager_->apply_template("non_existent_template");
  EXPECT_FALSE(result.has_value());

  // Test loading non-existent preset
  auto preset_result = manager_->load_preset("non_existent_preset");
  EXPECT_FALSE(preset_result.has_value());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
