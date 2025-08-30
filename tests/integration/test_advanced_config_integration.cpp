/**
 * @file test_advanced_config_integration.cpp
 * @brief Integration tests for advanced configuration management with applications
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "solar_utils/advanced_config.hpp"
#include "solar_utils/config.hpp"

using namespace SolarSystem::Utils::Advanced;
using namespace SolarSystem::Utils;

class AdvancedConfigIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    manager_ = std::make_unique<AdvancedConfigManager>();

    // Create temporary directory for test files
    test_dir_ = std::filesystem::temp_directory_path() / "solar_system_integration_test";
    std::filesystem::create_directories(test_dir_);

    // Create test configuration files
    create_test_config_files();
  }

  void TearDown() override {
    // Clean up test directory
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  void create_test_config_files() {
    // Create a valid configuration file
    auto valid_config = test_dir_ / "valid_config.ini";
    seam file(valid_config);
    file << "# Solar System Suite Configuration\n";
    file << "[simulation]\n";
    file << "timestep = 3600.0\n";
    file << "max_iterations = 1000000\n";
    file << "enable_progress = true\n";
    file << "verbose_output = false\n";
    file << "output_format = \"json\"\n";
    file << "\n[data]\n";
    file << "jpl_api_url = \"https://ssd.jpl.nasa.gov/api/horizons.api\"\n";
    file << "cache_directory = \"./cache\"\n";
    file << "cache_max_age_days = 30\n";
    file << "allow_fallback_data = true\n";
    file << "\n[logging]\n";
    file << "min_level = 1\n";
    file << "log_file = \"solar_system.log\"\n";
    file << "colored_output = true\n";
    file << "\n[web]\n";
    file << "port = 8080\n";
    file << "host = \"localhost\"\n";
    file << "enable_cors = true\n";
    file.close();

    // Create an invalid configuration file
    auto invalid_config = test_dir_ / "invalid_config.ini";
    std::ofstream invalid_file(invalid_config);
    invalid_file << "[simulation]\n";
    invalid_file << "timestep = -1.0\n";  // Invalid negative timestep
    invalid_file << "max_iterations = 0\n";  // Invalid zero iterations
    invalid_file << "output_format = \"invalid_format\"\n";  // Invalid format
    invalid_file << "\n[web]\n";
    invalid_file << "port = 0\n";  // Invalid port
    invalid_file.close();

    // Create a configuration with conflicts
    auto conflict_config = test_dir_ / "conflict_config.ini";
    std::ofstream conflict_file(conflict_config);
    conflict_file << "[simulation]\n";
    conflict_file << "verbose_output = true\n";
    conflict_file << "quiet_mode = true\n";  // Conflicts with verbose
    conflict_file.close();
  }

  std::unique_ptr<AdvancedConfigManager> manager_;
  std::filesystem::path test_dir_;
};

// Test loading valid configuration file
TEST_F(AdvancedConfigIntegrationTest, LoadValidConfiguration) {
  auto config_file = test_dir_ / "valid_config.ini";

  auto result = manager_->load_configuration(config_file.string());
  EXPECT_TRUE(result.has_value()) << "Failed to load valid configuration";

  if (result.has_value()) {
    const auto& config = result.value();

    // Verify simulation settings
    EXPECT_EQ(config.simulation.timestep, 3600.0);
    EXPECT_EQ(config.simulation.max_iterations, 1000000u);
    EXPECT_TRUE(config.simulation.enable_progress);
    EXPECT_FALSE(config.simulation.verbose_output);
    EXPECT_EQ(config.simulation.output_format, "json");

    // Verify data settings
    EXPECT_EQ(config.data.jpl_api_url, "https://ssd.jpl.nasa.gov/api/horizons.api");
    EXPECT_EQ(config.data.cache_directory, "./cache");
    EXPECT_EQ(config.data.cache_max_age_days, 30u);
    EXPECT_TRUE(config.data.allow_fallback_data);

    // Verify web settings
    EXPECT_EQ(config.web.port, 8080);
    EXPECT_EQ(config.web.host, "localhost");
    EXPECT_TRUE(config.web.enable_cors);
  }
}

// Test loading invalid configuration file
TEST_F(AdvancedConfigIntegrationTest, LoadInvalidConfiguration) {
  auto config_file = test_dir_ / "invalid_config.ini";

  auto result = manager_->load_configuration(config_file.string());
  EXPECT_FALSE(result.has_value()) << "Should fail to load invalid configuration";

  if (!result.has_value()) {
    const auto& error = result.error();
    EXPECT_FALSE(error.empty());

    // Check that error message contains validation details
    EXPECT_NE(error.find("validation failed"), std::string::npos);
  }
}

// Test configuration with CLI argument override
TEST_F(AdvancedConfigIntegrationTest, ConfigurationWithCLIOverride) {
  auto config_file = test_dir_ / "valid_config.ini";

  // CLI arguments that override file settings
  std::vector<std::string> cli_args = {
    "--timestep=1800.0",
    "--port=9090",
    "--verbose=true"
  };

  auto result = manager_->load_configuration(config_file.string(), cli_args);
  EXPECT_TRUE(result.has_value()) << "Failed to load configuration with CLI override";

  if (result.has_value()) {
    const auto& config = result.value();

    // CLI arguments should override file settings
    EXPECT_EQ(config.simulation.timestep, 1800.0);  // Overridden by CLI
    EXPECT_EQ(config.web.port, 9090);  // Overridden by CLI

    // Non-overridden settings should remain from file
    EXPECT_EQ(config.simulation.max_iterations, 1000000u);  // From file
    EXPECT_EQ(config.data.cache_directory, "./cache");  // From file
  }
}

// Test template-based configuration
TEST_F(AdvancedConfigIntegrationTest, TemplateBasedConfiguration) {
  // Test development template
  auto result = manager_->load_configuration(std::nullopt, {}, "", "development");
  EXPECT_TRUE(result.has_value()) << "Failed to load development template";

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_TRUE(config.debug_mode);
    EXPECT_EQ(config.logging.min_level, Logger::Level::DEBUG);
    EXPECT_TRUE(config.logging.colored_output);
    EXPECT_TRUE(config.simulation.enable_progress);
    EXPECT_TRUE(config.simulation.verbose_output);
  }

  // Test production template
  result = manager_->load_configuration(std::nullopt, {}, "", "production");
  EXPECT_TRUE(result.has_value()) << "Failed to load production template";

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_FALSE(config.debug_mode);
    EXPECT_EQ(config.logging.min_level, Logger::Level::INFO);
    EXPECT_FALSE(config.logging.colored_output);
    EXPECT_FALSE(config.simulation.enable_progress);
    EXPECT_FALSE(config.simulation.verbose_output);
  }

  // Test high performance template
  result = manager_->load_configuration(std::nullopt, {}, "", "high_performance");
  EXPECT_TRUE(result.has_value()) << "Failed to load high performance template";

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_TRUE(config.simulation.enable_simd);
    EXPECT_TRUE(config.simulation.enable_lto);
    EXPECT_EQ(config.simulation.thread_count, 0u);  // Auto-detect
    EXPECT_EQ(config.logging.min_level, Logger::Level::WARN);
  }
}

// Test preset-based configuration
TEST_F(AdvancedConfigIntegrationTest, PresetBasedConfiguration) {
  // Test fast simulation preset
  auto result = manager_->load_configuration(std::nullopt, {}, "fast_simulation");
  EXPECT_TRUE(result.has_value()) << "Failed to load fast simulation preset";

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 7200.0);  // 2 hours
    EXPECT_EQ(config.simulation.max_iterations, 100000u);
    EXPECT_EQ(config.simulation.convergence_threshold, 1e-6);
  }

  // Test high accuracy preset
  result = manager_->load_configuration(std::nullopt, {}, "high_accuracy");
  EXPECT_TRUE(result.has_value()) << "Failed to load high accuracy preset";

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.simulation.timestep, 900.0);  // 15 minutes
    EXPECT_EQ(config.simulation.max_iterations, 10000000u);
    EXPECT_EQ(config.simulation.convergence_threshold, 1e-15);
  }

  // Test web server preset
  result = manager_->load_configuration(std::nullopt, {}, "web_server");
  EXPECT_TRUE(result.has_value()) << "Failed to load web server preset";

  if (result.has_value()) {
    const auto& config = result.value();
    EXPECT_EQ(config.web.port, 8080);
    EXPECT_TRUE(config.web.enable_cors);
    EXPECT_TRUE(config.web.enable_compression);
    EXPECT_EQ(config.web.max_connections, 100u);
    EXPECT_EQ(config.logging.min_level, Logger::Level::INFO);
  }
}

// Test configuration builder integration
TEST_F(AdvancedConfigIntegrationTest, ConfigurationBuilderIntegration) {
  // Test building configuration for simulation application
  ConfigurationBuilder builder;
  auto result = builder
    .from_template("high_performance")
    .simulation_timestep(1800.0)  // 30 minutes
    .simulation_max_iterations(2000000)
    .simulation_output_format("csv")
    .data_cache_directory("./simulation_cache")
    .logging_level(Logger::Level::INFO)
    .build_with_manager(*manager_);

  EXPECT_TRUE(result.has_value()) << "Failed to build simulation configuration";

  if (result.has_value()) {
    const auto& config = result.value();

    // Check overridden values
    EXPECT_EQ(config.simulation.timestep, 1800.0);
    EXPECT_EQ(config.simulation.max_iterations, 2000000u);
    EXPECT_EQ(config.simulation.output_format, "csv");
    EXPECT_EQ(config.data.cache_directory, "./simulation_cache");
    EXPECT_EQ(config.logging.min_level, Logger::Level::INFO);

    // Check inherited template values
    EXPECT_TRUE(config.simulation.enable_simd);
    EXPECT_TRUE(config.simulation.enable_lto);
  }

  // Test building configuration for web server application
  result = builder
    .from_preset("web_server")
    .web_port(9000)
    .web_host("0.0.0.0")
    .logging_level(Logger::Level::WARN)
    .build_with_manager(*manager_);

  EXPECT_TRUE(result.has_value()) << "Failed to build web server configuration";

  if (result.has_value()) {
    const auto& config = result.value();

    // Check overridden values
    EXPECT_EQ(config.web.port, 9000);
    EXPECT_EQ(config.web.host, "0.0.0.0");
    EXPECT_EQ(config.logging.min_level, Logger::Level::WARN);

    // Check inherited preset values
    EXPECT_TRUE(config.web.enable_cors);
    EXPECT_TRUE(config.web.enable_compression);
  }
}

// Test configuration validation in real scenarios
TEST_F(AdvancedConfigIntegrationTest, RealScenarioValidation) {
  // Test configuration for long-running simulation
  ConfigurationBuilder builder;
  auto result = builder
    .simulation_timestep(60.0)  // 1 minute - very small
    .simulation_max_iterations(50000000)  // Very large
    .build_with_manager(*manager_);

  EXPECT_TRUE(result.has_value()) << "Configuration should be valid but may have warnings";

  // Check validation result for warnings
  if (result.has_value()) {
    auto validation_result = manager_->validate_configuration(result.value());
    EXPECT_TRUE(validation_result.is_valid);
    EXPECT_FALSE(validation_result.warnings.empty());  // Should have performance warnings
  }

  // Test configuration for web deployment
  result = builder
    .from_preset("web_server")
    .web_port(80)  // Standard HTTP port
    .web_host("0.0.0.0")  // Listen on all interfaces
    .build_with_manager(*manager_);

  EXPECT_TRUE(result.has_value()) << "Web deployment configuration should be valid";

  if (result.has_value()) {
    auto validation_result = manager_->validate_configuration(result.value());
    EXPECT_TRUE(validation_result.is_valid);
    // May have security warnings about port 80 and 0.0.0.0
  }
}

// Test configuration migration scenarios
TEST_F(AdvancedConfigIntegrationTest, ConfigurationMigration) {
  // Create old-style configuration
  Config::AppConfig old_config = Config::get_default();

  // Test migration from version 3.0 to 4.0
  auto result = manager_->migrate_configuration(old_config, "3.0", "4.0");
  EXPECT_TRUE(result.has_value()) << "Configuration migration should succeed";

  if (result.has_value()) {
    // Migrated configuration should be valid
    auto validation_result = manager_->validate_configuration(result.value());
    EXPECT_TRUE(validation_result.is_valid);
  }
}

// Test error recovery and suggestions
TEST_F(AdvancedConfigIntegrationTest, ErrorRecoveryAndSuggestions) {
  // Test configuration with recoverable errors
  ConfigurationBuilder builder;
  auto result = builder
    .simulation_output_format("XML")  // Invalid format, should suggest alternatives
    .build_with_manager(*manager_);

  EXPECT_FALSE(result.has_value()) << "Invalid configuration should fail";

  if (!result.has_value()) {
    const auto& error = result.error();
    EXPECT_FALSE(error.empty());

    // Error should contain suggestions
    EXPECT_NE(error.find("json"), std::string::npos);  // Should suggest valid formats
  }

  // Test parameter validation with suggestions
  auto param_result = manager_->validate_parameter("simulation.output_format", "xml");
  EXPECT_FALSE(param_result.is_valid);
  EXPECT_FALSE(param_result.suggestions.empty());
}

// Test preset and template management
TEST_F(AdvancedConfigIntegrationTest, PresetAndTemplateManagement) {
  // Create custom configuration
  Config::AppConfig custom_config = Config::get_default();
  custom_config.simulation.timestep = 2700.0;  // 45 minutes
  custom_config.simulation.max_iterations = 750000;
  custom_config.simulation.output_format = "csv";

  // Save as custom preset
  auto save_result = manager_->save_preset("custom_test", custom_config,
                                          "Custom test configuration", "testing");
  EXPECT_TRUE(save_result.has_value()) << "Should be able to save custom preset";

  // Load the custom preset
  auto load_result = manager_->load_preset("custom_test");
  EXPECT_TRUE(load_result.has_value()) << "Should be able to load custom preset";

  if (load_result.has_value()) {
    const auto& loaded_config = load_result.value();
    EXPECT_EQ(loaded_config.simulation.timestep, 2700.0);
    EXPECT_EQ(loaded_config.simulation.max_iterations, 750000u);
    EXPECT_EQ(loaded_config.simulation.output_format, "csv");
  }

  // Verify preset appears in available presets
  auto available_presets = manager_->get_available_presets();
  bool found_custom = false;
  for (const auto& preset : available_presets) {
    if (preset.name == "custom_test") {
      found_custom = true;
      EXPECT_EQ(preset.description, "Custom test configuration");
      EXPECT_EQ(preset.use_case, "testing");
      EXPECT_FALSE(preset.is_system_preset);
      break;
    }
  }
  EXPECT_TRUE(found_custom) << "Custom preset should appear in available presets";
}

// Test comprehensive configuration loading with all features
TEST_F(AdvancedConfigIntegrationTest, ComprehensiveConfigurationLoading) {
  auto config_file = test_dir_ / "valid_config.ini";

  // Test loading with file, CLI args, preset, and template
  std::vector<std::string> cli_args = {
    "--timestep=900.0",  // Override file and preset
    "--verbose=true"
  };

  // This should apply template, then preset, then file, then CLI (in priority order)
  auto result = manager_->load_configuration(config_file.string(), cli_args,
                                           "fast_simulation", "development");
  EXPECT_TRUE(result.has_value()) << "Comprehensive configuration loading should succeed";

  if (result.has_value()) {
    const auto& config = result.value();

    // CLI should have highest priority
    EXPECT_EQ(config.simulation.timestep, 900.0);  // From CLI

    // File should override preset and template
    EXPECT_EQ(config.simulation.max_iterations, 1000000u);  // From file

    // Template should provide base values
    EXPECT_TRUE(config.debug_mode);  // From development template

    // Validate the final configuration
    auto validation_result = manager_->validate_configuration(config);
    EXPECT_TRUE(validation_result.is_valid);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
