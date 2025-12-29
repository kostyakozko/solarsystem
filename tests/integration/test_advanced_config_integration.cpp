/**
 * @file test_advanced_config_integration.cpp
 * @brief Integration tests for advanced configuration management with applications
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "solar_utils/advanced_config.hpp"
#include "solar_utils/config.hpp"

using namespace SolarSystem::Utils::Advanced;
using namespace SolarSystem::Utils;
TEST(AdvancedConfigurationIntegrationTests, Basic_Integration) {
    auto manager = std::make_unique<AdvancedConfigManager>();

    // Test that manager can be created and basic functionality works
    ASSERT_NE(nullptr, manager );

    // Test loading default configuration
    auto config = Config::get_default();

    // Test that the configuration is valid
    auto validation_result = manager->validate_configuration(config);
    ASSERT_TRUE(validation_result.is_valid);
}

TEST(AdvancedConfigurationIntegrationTests, Template_Integration) {
    auto manager = std::make_unique<AdvancedConfigManager>();

    // Test applying a template
    auto result = manager->apply_template("development");
    if (result.has_value()) {
      // Template applied successfully
      auto validation_result = manager->validate_configuration(result.value());
      ASSERT_TRUE(validation_result.is_valid);
    }
}

