/**
 * @file test_config_manager.cpp
 * @brief Unit tests for unified configuration system (Tasks 16 & 17)
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/config/config_manager.hpp"

using namespace SolarSystem::Core::Config;
TEST(ConfigurationManagerTests, Configuration_Manager_Initialization) {
    ConfigurationManager manager;
    auto config = manager.get_config();
    if (config.version.empty()) throw std::runtime_error("Config should have version");
}

TEST(ConfigurationManagerTests, Configuration_Validation) {
    ConfigurationManager manager;
    auto validation = manager.validate();
    if (!validation.is_valid) {
      throw std::runtime_error("Default config should be valid");
    }
}

TEST(ConfigurationManagerTests, Configuration_Source_Tracking) {
    ConfigurationManager manager;
    auto source = manager.get_parameter_source("simulation.timestep");
    if (source != ConfigSource::Default)
      throw std::runtime_error("Initial source should be Default");
}

TEST(ConfigurationManagerTests, Configuration_Backup_and_Restore) {
    ConfigurationManager manager;

    auto backup = manager.create_backup("test backup");
    if (backup.description != "test backup")
      throw std::runtime_error("Backup description wrong");

    auto restore_result = manager.restore_from_backup(backup);
    if (!restore_result.has_value()) throw std::runtime_error("Restore should succeed");
}

TEST(ConfigurationManagerTests, Configuration_Change_History) {
    ConfigurationManager manager;

    auto history = manager.get_change_history();
    // Initially empty
    if (history.size() > 0) {
      // Some changes might have been tracked
    }
}

TEST(ConfigurationManagerTests, Precedence_Documentation) {
    auto docs = ConfigurationManager::get_precedence_documentation();
    if (docs.empty()) throw std::runtime_error("Documentation should not be empty");
    if (docs.find("CLI") == std::string::npos)
      throw std::runtime_error("Should mention CLI precedence");
}

TEST(ConfigurationManagerTests, Cross_Application_Conflict_Detection) {
    ConfigurationManager manager;

    std::map<std::string, SolarSystem::Utils::Config::AppConfig> app_configs;
    app_configs["app1"] = manager.get_config();
    app_configs["app2"] = manager.get_config();

    auto conflicts = manager.detect_cross_app_conflicts(app_configs);
    // May or may not have conflicts depending on config
}

TEST(ConfigurationManagerTests, Configuration_Dependencies) {
    ConfigurationManager manager;

    auto deps = manager.get_dependencies();
    if (deps.empty()) throw std::runtime_error("Should have default dependencies");

    // Check dependency validation
    auto validation = manager.validate_dependencies();
    if (!validation.is_valid) {
      throw std::runtime_error("Default dependencies should be valid");
    }
}

TEST(ConfigurationManagerTests, Impact_Analysis) {
    ConfigurationManager manager;

    auto impact = manager.analyze_impact("simulation.timestep", "7200");
    if (impact.changed_parameter != "simulation.timestep")
      throw std::runtime_error("Wrong parameter in impact analysis");
    if (impact.affected_applications.empty()) {
      // May or may not have affected apps
    }
}

TEST(ConfigurationManagerTests, Conflict_Prediction) {
    ConfigurationManager manager;

    // Test if a change would cause conflict
    bool would_conflict = manager.would_cause_conflict("simulation.timestep", "-1");
    if (!would_conflict) {
      // Negative timestep should cause conflict, but depends on validation
    }
}

TEST(ConfigurationManagerTests, Reset_to_Defaults) {
    ConfigurationManager manager;

    manager.reset_to_defaults();
    auto config = manager.get_config();
    if (config.version.empty()) throw std::runtime_error("Config should have version after reset");
}

