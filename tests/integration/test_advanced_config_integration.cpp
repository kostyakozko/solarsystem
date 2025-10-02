/**
 * @file test_advanced_config_integration.cpp
 * @brief Integration tests for advanced configuration management with applications
 */

#include "../utils/test_framework.h"
#include <filesystem>
#include <fstream>
#include <sstream>

#include "solar_utils/advanced_config.hpp"
#include "solar_utils/config.hpp"

using namespace SolarSystem::Utils::Advanced;
using namespace SolarSystem::Utils;

int main() {
  TestSuite suite("Advanced Configuration Integration Tests");

  suite.run_test("Basic Integration", []() {
    auto manager = std::make_unique<AdvancedConfigManager>();

    // Test that manager can be created and basic functionality works
    ASSERT_TRUE(manager != nullptr);

    // Test loading default configuration
    auto config = Config::get_default();

    // Test that the configuration is valid
    auto validation_result = manager->validate_configuration(config);
    ASSERT_TRUE(validation_result.is_valid);
  });

  suite.run_test("Template Integration", []() {
    auto manager = std::make_unique<AdvancedConfigManager>();

    // Test applying a template
    auto result = manager->apply_template("development");
    if (result.has_value()) {
      // Template applied successfully
      auto validation_result = manager->validate_configuration(result.value());
      ASSERT_TRUE(validation_result.is_valid);
    }
  });

  suite.print_summary();
  return suite.all_passed() ? 0 : 1;
}
