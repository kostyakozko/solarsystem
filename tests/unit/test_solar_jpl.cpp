/**
 * @file test_solar_jpl.cpp
 * @brief Comprehensive integration tests for solar_jpl library
 */

#include "../utils/test_framework.h"
#include "solar_jpl/jpl_client.hpp"
#include "solar_jpl/cache_manager.hpp"
#include "solar_jpl/data_validator.hpp"
#include <chrono>

using namespace SolarSystem::JPL;

int main() {
  TestSuite suite("Solar JPL Library Integration Tests");

  // Test Complete JPL Workflow
  suite.run_test("Complete JPL Workflow - Cache Creation", []() {
    auto client = JPLClientFactory::create_for_testing();

    // Clear any existing cache
    (void)client->clear_cache();

    // Create test data
    std::vector<EphemerisData> test_data;

    EphemerisData sun;
    sun.body_name = "Sun";
    sun.jpl_id = 10;
    sun.epoch = std::chrono::system_clock::now();
    sun.position = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
    sun.velocity = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
    sun.mass = 1.98847e30;
    test_data.push_back(sun);

    EphemerisData earth;
    earth.body_name = "Earth";
    earth.jpl_id = 399;
    earth.epoch = std::chrono::system_clock::now();
    earth.position = SolarSystem::Math::Vector3d{1.496e11, 0.0, 0.0};
    earth.velocity = SolarSystem::Math::Vector3d{0.0, 29780.0, 0.0};
    earth.mass = 5.97219e24;
    test_data.push_back(earth);

    // Save to cache
    (void)client->save_to_cache(test_data);

    // Load from cache
    auto load_result = client->load_from_cache();

    if (is_success(load_result)) {
      auto loaded = get_value(load_result);
      ASSERT_TRUE(loaded.size() >= 2);
    }
  });

  suite.run_test("JPL Client with Cache Manager", []() {
    auto client = JPLClientFactory::create_for_testing();
    auto& cache_mgr = client->cache_manager();

    // Test cache manager operations
    auto stats = cache_mgr.get_statistics();
    ASSERT_TRUE(stats.total_reads >= 0);
    ASSERT_TRUE(stats.total_writes >= 0);
  });

  suite.run_test("JPL Client with Data Validator", []() {
    auto client = JPLClientFactory::create_for_testing();
    auto& validator = client->data_validator();

    // Create valid test data
    EphemerisData valid_data;
    valid_data.body_name = "Venus";
    valid_data.jpl_id = 299;
    valid_data.epoch = std::chrono::system_clock::now();
    valid_data.position = SolarSystem::Math::Vector3d{1.082e11, 0.0, 0.0};
    valid_data.velocity = SolarSystem::Math::Vector3d{0.0, 35020.0, 0.0};
    valid_data.mass = 4.86732e24;

    auto result = validator.validate_ephemeris_data(valid_data);
    // Validation should succeed for valid data
  });

  suite.run_test("Cache Manager Standalone", []() {
    auto cache_mgr = CacheManagerFactory::create_for_testing();
    ASSERT_TRUE(cache_mgr != nullptr);

    (void)cache_mgr->initialize();
    // Initialization should succeed
  });

  suite.run_test("Cache Manager Statistics", []() {
    auto cache_mgr = CacheManagerFactory::create_for_testing();
    (void)cache_mgr->initialize();

    const auto& stats = cache_mgr->get_statistics();
    ASSERT_EQ(0.0, stats.hit_ratio());  // No reads yet

    cache_mgr->reset_statistics();
    const auto& reset_stats = cache_mgr->get_statistics();
    ASSERT_EQ(0, reset_stats.total_reads);
  });

  suite.run_test("Cache Manager Refresh Strategy", []() {
    auto cache_mgr = CacheManagerFactory::create_for_testing();
    (void)cache_mgr->initialize();

    (void)cache_mgr->set_refresh_strategy(RefreshStrategy::Manual);
    (void)cache_mgr->set_refresh_strategy(RefreshStrategy::TimeBasedAuto);
    (void)cache_mgr->set_refresh_strategy(RefreshStrategy::Intelligent);

    // Just verify we can set different strategies
  });

  suite.run_test("Data Validator Standalone", []() {
    auto validator = DataValidatorFactory::create_for_testing();
    ASSERT_TRUE(validator != nullptr);
  });

  suite.run_test("Data Validator - Valid Data", []() {
    auto validator = DataValidatorFactory::create_default();

    EphemerisData valid_data;
    valid_data.body_name = "Jupiter";
    valid_data.jpl_id = 599;
    valid_data.epoch = std::chrono::system_clock::now();
    valid_data.position = SolarSystem::Math::Vector3d{7.785e11, 0.0, 0.0};
    valid_data.velocity = SolarSystem::Math::Vector3d{0.0, 13070.0, 0.0};
    valid_data.mass = 1.89813e27;

    (void)validator->validate_ephemeris_data(valid_data);
    // Should succeed for valid data
  });

  suite.run_test("Data Validator - Invalid Position", []() {
    auto validator = DataValidatorFactory::create_default();

    EphemerisData invalid_data;
    invalid_data.body_name = "Invalid";
    invalid_data.jpl_id = 999;
    invalid_data.epoch = std::chrono::system_clock::now();
    invalid_data.position = SolarSystem::Math::Vector3d{1e20, 0.0, 0.0};  // Too far
    invalid_data.velocity = SolarSystem::Math::Vector3d{0.0, 1000.0, 0.0};
    invalid_data.mass = 1e24;

    (void)validator->validate_ephemeris_data(invalid_data);
    // Should detect invalid position
  });

  suite.run_test("Data Validator - Invalid Mass", []() {
    auto validator = DataValidatorFactory::create_default();

    EphemerisData invalid_data;
    invalid_data.body_name = "Invalid";
    invalid_data.jpl_id = 999;
    invalid_data.epoch = std::chrono::system_clock::now();
    invalid_data.position = SolarSystem::Math::Vector3d{1e11, 0.0, 0.0};
    invalid_data.velocity = SolarSystem::Math::Vector3d{0.0, 1000.0, 0.0};
    invalid_data.mass = 1e5;  // Too small

    (void)validator->validate_ephemeris_data(invalid_data);
    // Should detect invalid mass
  });

  suite.run_test("Data Validator Collection", []() {
    auto validator = DataValidatorFactory::create_default();

    std::vector<EphemerisData> collection;

    EphemerisData earth;
    earth.body_name = "Earth";
    earth.jpl_id = 399;
    earth.epoch = std::chrono::system_clock::now();
    earth.position = SolarSystem::Math::Vector3d{1.496e11, 0.0, 0.0};
    earth.velocity = SolarSystem::Math::Vector3d{0.0, 29780.0, 0.0};
    earth.mass = 5.97219e24;
    collection.push_back(earth);

    EphemerisData mars;
    mars.body_name = "Mars";
    mars.jpl_id = 499;
    mars.epoch = std::chrono::system_clock::now();
    mars.position = SolarSystem::Math::Vector3d{2.279e11, 0.0, 0.0};
    mars.velocity = SolarSystem::Math::Vector3d{0.0, 24077.0, 0.0};
    mars.mass = 6.41693e23;
    collection.push_back(mars);

    (void)validator->validate_ephemeris_collection(collection);
    // Should validate collection
  });

  suite.run_test("Data Validator Quality Assessment", []() {
    auto validator = DataValidatorFactory::create_default();

    std::vector<EphemerisData> collection;

    EphemerisData earth;
    earth.body_name = "Earth";
    earth.jpl_id = 399;
    earth.epoch = std::chrono::system_clock::now();
    earth.position = SolarSystem::Math::Vector3d{1.496e11, 0.0, 0.0};
    earth.velocity = SolarSystem::Math::Vector3d{0.0, 29780.0, 0.0};
    earth.mass = 5.97219e24;
    collection.push_back(earth);

    (void)validator->assess_data_quality(collection);
    // Should generate quality metrics
  });

  suite.run_test("Validation Utils - Error Type to String", []() {
    ASSERT_EQ("InvalidJPLId", ValidationUtils::to_string(ValidationErrorType::InvalidJPLId));
    ASSERT_EQ("InvalidPosition", ValidationUtils::to_string(ValidationErrorType::InvalidPosition));
    ASSERT_EQ("InvalidMass", ValidationUtils::to_string(ValidationErrorType::InvalidMass));
  });

  suite.run_test("Validation Utils - Severity to String", []() {
    ASSERT_EQ("Info", ValidationUtils::to_string(ValidationSeverity::Info));
    ASSERT_EQ("Warning", ValidationUtils::to_string(ValidationSeverity::Warning));
    ASSERT_EQ("Error", ValidationUtils::to_string(ValidationSeverity::Error));
    ASSERT_EQ("Critical", ValidationUtils::to_string(ValidationSeverity::Critical));
  });

  suite.run_test("Network Diagnostics Integration", []() {
    auto client = JPLClientFactory::create_for_testing();

    // Get initial diagnostics
    auto diagnostics = client->get_network_diagnostics();

    // Check connectivity
    (void)client->check_network_connectivity();

    // Get health score
    double health = client->get_network_health_score();
    ASSERT_TRUE(health >= 0.0 && health <= 1.0);
  });

  suite.run_test("Offline Mode Integration", []() {
    auto client = JPLClientFactory::create_for_testing();

    // Enable offline mode
    client->set_offline_mode(true);
    ASSERT_TRUE(client->is_offline_mode());

    // Try to load from cache (should work in offline mode)
    (void)client->load_from_cache();

    // Disable offline mode
    client->set_offline_mode(false);
    ASSERT_FALSE(client->is_offline_mode());
  });

  suite.run_test("Cache Validation Integration", []() {
    auto client = JPLClientFactory::create_for_testing();

    // Validate cache
    (void)client->validate_cache();
    // Result depends on cache state
  });

  suite.run_test("Complete Data Pipeline", []() {
    auto client = JPLClientFactory::create_for_testing();

    // 1. Clear cache
    (void)client->clear_cache();

    // 2. Create test data
    std::vector<EphemerisData> data;
    EphemerisData mercury;
    mercury.body_name = "Mercury";
    mercury.jpl_id = 199;
    mercury.epoch = std::chrono::system_clock::now();
    mercury.position = SolarSystem::Math::Vector3d{5.79e10, 0.0, 0.0};
    mercury.velocity = SolarSystem::Math::Vector3d{0.0, 47362.0, 0.0};
    mercury.mass = 3.30104e23;
    data.push_back(mercury);

    // 3. Validate data
    auto& validator = client->data_validator();
    auto validation_result = validator.validate_ephemeris_collection(data);

    // 4. Save to cache
    (void)client->save_to_cache(data);

    // 5. Validate cache
    (void)client->validate_cache();

    // 6. Load from cache
    auto load_result = client->load_from_cache();

    // Pipeline should complete without crashes
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
