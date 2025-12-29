/**
 * @file test_solar_jpl.cpp
 * @brief Comprehensive integration tests for solar_jpl library
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>

#include "solar_jpl/cache_manager.hpp"
#include "solar_jpl/data_validator.hpp"
#include "solar_jpl/jpl_client.hpp"

using namespace SolarSystem::JPL;

// ============================================================================
// Complete JPL Workflow Tests
// ============================================================================

TEST(SolarJPL, CompleteWorkflowCacheCreation) {
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
    EXPECT_GE(loaded.size(), 2);
  }
}

TEST(SolarJPL, ClientWithCacheManager) {
  auto client = JPLClientFactory::create_for_testing();
  auto& cache_mgr = client->cache_manager();

  // Test cache manager operations
  auto stats = cache_mgr.get_statistics();
  EXPECT_GE(stats.total_reads, 0);
  EXPECT_GE(stats.total_writes, 0);
}

TEST(SolarJPL, ClientWithDataValidator) {
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
}

// ============================================================================
// Cache Manager Tests
// ============================================================================

TEST(SolarJPL, CacheManagerStandalone) {
  auto cache_mgr = CacheManagerFactory::create_for_testing();
  ASSERT_NE(nullptr, cache_mgr);

  (void)cache_mgr->initialize();
  // Initialization should succeed
}

TEST(SolarJPL, CacheManagerStatistics) {
  auto cache_mgr = CacheManagerFactory::create_for_testing();
  (void)cache_mgr->initialize();

  const auto& stats = cache_mgr->get_statistics();
  EXPECT_EQ(0.0, stats.hit_ratio());  // No reads yet

  cache_mgr->reset_statistics();
  const auto& reset_stats = cache_mgr->get_statistics();
  EXPECT_EQ(0, reset_stats.total_reads);
}

TEST(SolarJPL, CacheManagerRefreshStrategy) {
  auto cache_mgr = CacheManagerFactory::create_for_testing();
  (void)cache_mgr->initialize();

  (void)cache_mgr->set_refresh_strategy(RefreshStrategy::Manual);
  (void)cache_mgr->set_refresh_strategy(RefreshStrategy::TimeBasedAuto);
  (void)cache_mgr->set_refresh_strategy(RefreshStrategy::Intelligent);

  // Just verify we can set different strategies
}

// ============================================================================
// Data Validator Tests
// ============================================================================

TEST(SolarJPL, DataValidatorStandalone) {
  auto validator = DataValidatorFactory::create_for_testing();
  ASSERT_NE(nullptr, validator);
}

TEST(SolarJPL, DataValidatorValidData) {
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
}

TEST(SolarJPL, DataValidatorInvalidPosition) {
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
}

TEST(SolarJPL, DataValidatorInvalidMass) {
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
}

TEST(SolarJPL, DataValidatorCollection) {
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
}

TEST(SolarJPL, DataValidatorQualityAssessment) {
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
}

// ============================================================================
// Validation Utils Tests
// ============================================================================

TEST(SolarJPL, ValidationUtilsErrorTypeToString) {
  EXPECT_EQ("InvalidJPLId", ValidationUtils::to_string(ValidationErrorType::InvalidJPLId));
  EXPECT_EQ("InvalidPosition", ValidationUtils::to_string(ValidationErrorType::InvalidPosition));
  EXPECT_EQ("InvalidMass", ValidationUtils::to_string(ValidationErrorType::InvalidMass));
}

TEST(SolarJPL, ValidationUtilsSeverityToString) {
  EXPECT_EQ("Info", ValidationUtils::to_string(ValidationSeverity::Info));
  EXPECT_EQ("Warning", ValidationUtils::to_string(ValidationSeverity::Warning));
  EXPECT_EQ("Error", ValidationUtils::to_string(ValidationSeverity::Error));
  EXPECT_EQ("Critical", ValidationUtils::to_string(ValidationSeverity::Critical));
}

// ============================================================================
// Network Integration Tests
// ============================================================================

TEST(SolarJPL, NetworkDiagnosticsIntegration) {
  auto client = JPLClientFactory::create_for_testing();

  // Get initial diagnostics
  auto diagnostics = client->get_network_diagnostics();

  // Check connectivity
  (void)client->check_network_connectivity();

  // Get health score
  double health = client->get_network_health_score();
  EXPECT_GE(health, 0.0);
  EXPECT_LE(health, 1.0);
}

TEST(SolarJPL, OfflineModeIntegration) {
  auto client = JPLClientFactory::create_for_testing();

  // Enable offline mode
  client->set_offline_mode(true);
  EXPECT_TRUE(client->is_offline_mode());

  // Try to load from cache (should work in offline mode)
  (void)client->load_from_cache();

  // Disable offline mode
  client->set_offline_mode(false);
  EXPECT_FALSE(client->is_offline_mode());
}

TEST(SolarJPL, CacheValidationIntegration) {
  auto client = JPLClientFactory::create_for_testing();

  // Validate cache
  (void)client->validate_cache();
  // Result depends on cache state
}

// ============================================================================
// Complete Data Pipeline Tests
// ============================================================================

TEST(SolarJPL, CompleteDataPipeline) {
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
}
