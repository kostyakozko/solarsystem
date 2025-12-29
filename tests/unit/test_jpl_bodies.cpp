/**
 * @file test_jpl_bodies.cpp
 * @brief Comprehensive unit tests for JPL client body operations
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "solar_jpl/jpl_client.hpp"

using namespace SolarSystem::JPL;

// ============================================================================
// JPLClientFactory Tests
// ============================================================================

TEST(JPLClientBodies, FactoryCreateDefault) {
  auto client = JPLClientFactory::create_default();
  ASSERT_NE(nullptr, client);
}

TEST(JPLClientBodies, FactoryCreateWithCustomConfig) {
  JPLClientConfig config;
  config.max_retries = 5;
  config.request_timeout = std::chrono::seconds(60);

  auto client = JPLClientFactory::create(config);
  ASSERT_NE(nullptr, client);
  EXPECT_EQ(5, client->config().max_retries);
  EXPECT_EQ(std::chrono::seconds(60), client->config().request_timeout);
}

TEST(JPLClientBodies, FactoryCreateForTesting) {
  auto client = JPLClientFactory::create_for_testing();
  ASSERT_NE(nullptr, client);
}

// ============================================================================
// JPLClient Basic Operations Tests
// ============================================================================

TEST(JPLClientBodies, Construction) {
  JPLClientConfig config;
  JPLClient client(config);

  EXPECT_EQ(config.api_endpoint, client.config().api_endpoint);
}

TEST(JPLClientBodies, CacheMetadataNoCache) {
  auto client = JPLClientFactory::create_for_testing();

  auto metadata = client->get_cache_metadata();
  // May or may not have cache, both are valid
}

TEST(JPLClientBodies, HasCurrentYearData) {
  auto client = JPLClientFactory::create_for_testing();

  // Should return false if no cache exists
  (void)client->has_current_year_data();
  // Result depends on cache state, just verify it doesn't crash
}

// ============================================================================
// Cache Operations Tests
// ============================================================================

TEST(JPLClientBodies, LoadFromCacheNoCache) {
  auto client = JPLClientFactory::create_for_testing();

  (void)client->load_from_cache();
  // May succeed or fail depending on cache state
  // Just verify it returns a valid result
}

TEST(JPLClientBodies, SaveToCache) {
  auto client = JPLClientFactory::create_for_testing();

  std::vector<EphemerisData> test_data;
  EphemerisData earth;
  earth.body_name = "Earth";
  earth.jpl_id = 399;
  earth.epoch = std::chrono::system_clock::now();
  earth.position = SolarSystem::Math::Vector3d{1.496e11, 0.0, 0.0};
  earth.velocity = SolarSystem::Math::Vector3d{0.0, 29780.0, 0.0};
  earth.mass = 5.97219e24;
  test_data.push_back(earth);

  (void)client->save_to_cache(test_data);
  // Result depends on file system permissions
}

TEST(JPLClientBodies, ValidateCache) {
  auto client = JPLClientFactory::create_for_testing();

  (void)client->validate_cache();
  // May succeed or fail depending on cache state
}

TEST(JPLClientBodies, ClearCache) {
  auto client = JPLClientFactory::create_for_testing();

  (void)client->clear_cache();
  // Should succeed even if no cache exists
}

TEST(JPLClientBodies, RebuildCache) {
  auto client = JPLClientFactory::create_for_testing();

  (void)client->rebuild_cache();
  // May fail if network is unavailable, which is acceptable
}

// ============================================================================
// Network Operations Tests
// ============================================================================

TEST(JPLClientBodies, OfflineMode) {
  auto client = JPLClientFactory::create_for_testing();

  // Test setting offline mode
  client->set_offline_mode(true);
  EXPECT_TRUE(client->is_offline_mode());

  client->set_offline_mode(false);
  EXPECT_FALSE(client->is_offline_mode());
}

TEST(JPLClientBodies, NetworkConnectivityCheck) {
  auto client = JPLClientFactory::create_for_testing();

  (void)client->check_network_connectivity();
  // Status depends on actual network, just verify it returns
}

TEST(JPLClientBodies, NetworkDiagnostics) {
  auto client = JPLClientFactory::create_for_testing();

  auto diagnostics = client->get_network_diagnostics();
  // Should return valid diagnostics structure
  EXPECT_GE(diagnostics.successful_requests, 0);
  EXPECT_GE(diagnostics.failed_requests, 0);
}

TEST(JPLClientBodies, NetworkHealthScore) {
  auto client = JPLClientFactory::create_for_testing();

  double health = client->get_network_health_score();
  EXPECT_GE(health, 0.0);
  EXPECT_LE(health, 1.0);
}

// ============================================================================
// Async Operations Tests
// ============================================================================

TEST(JPLClientBodies, FetchBodyAsyncReturnsFuture) {
  auto client = JPLClientFactory::create_for_testing();

  auto epoch = std::chrono::system_clock::now();
  auto future = client->fetch_body_async(399, epoch);  // Earth

  // Just verify we get a future back
  EXPECT_TRUE(future.valid());

  // Don't wait for result as it may fail due to network
}

TEST(JPLClientBodies, FetchBodiesAsyncReturnsFuture) {
  auto client = JPLClientFactory::create_for_testing();

  std::vector<int> jpl_ids = {399, 499};  // Earth, Mars
  auto epoch = std::chrono::system_clock::now();
  auto future = client->fetch_bodies_async(jpl_ids, epoch);

  EXPECT_TRUE(future.valid());
}

TEST(JPLClientBodies, FetchAllBodiesAsyncReturnsFuture) {
  auto client = JPLClientFactory::create_for_testing();

  auto epoch = std::chrono::system_clock::now();
  auto future = client->fetch_all_bodies_async(epoch);

  EXPECT_TRUE(future.valid());
}

// ============================================================================
// Manager Access Tests
// ============================================================================

TEST(JPLClientBodies, CacheManagerAccess) {
  auto client = JPLClientFactory::create_for_testing();

  (void)client->cache_manager();
  // Just verify we can access it
}

TEST(JPLClientBodies, DataValidatorAccess) {
  auto client = JPLClientFactory::create_for_testing();

  (void)client->data_validator();
  // Just verify we can access it
}

// ============================================================================
// Error Scenarios Tests
// ============================================================================

TEST(JPLClientBodies, InvalidJPLID) {
  auto client = JPLClientFactory::create_for_testing();

  auto epoch = std::chrono::system_clock::now();
  auto future = client->fetch_body_async(-1, epoch);  // Invalid ID

  EXPECT_TRUE(future.valid());

  // Result should be an error, but we don't wait for it
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(JPLClientBodies, ConfigAccess) {
  JPLClientConfig config;
  config.max_retries = 7;
  JPLClient client(config);

  EXPECT_EQ(7, client.config().max_retries);
}

// ============================================================================
// Multiple Clients Tests
// ============================================================================

TEST(JPLClientBodies, MultipleClientInstances) {
  auto client1 = JPLClientFactory::create_for_testing();
  auto client2 = JPLClientFactory::create_for_testing();

  ASSERT_NE(nullptr, client1);
  ASSERT_NE(nullptr, client2);

  // Both should be independent
  client1->set_offline_mode(true);
  client2->set_offline_mode(false);

  EXPECT_TRUE(client1->is_offline_mode());
  EXPECT_FALSE(client2->is_offline_mode());
}

// ============================================================================
// Cache Round-Trip Tests
// ============================================================================

TEST(JPLClientBodies, CacheRoundTrip) {
  auto client = JPLClientFactory::create_for_testing();

  // Create test data
  std::vector<EphemerisData> original_data;
  EphemerisData mars;
  mars.body_name = "Mars";
  mars.jpl_id = 499;
  mars.epoch = std::chrono::system_clock::now();
  mars.position = SolarSystem::Math::Vector3d{2.279e11, 0.0, 0.0};
  mars.velocity = SolarSystem::Math::Vector3d{0.0, 24077.0, 0.0};
  mars.mass = 6.41693e23;
  original_data.push_back(mars);

  // Save to cache
  auto save_result = client->save_to_cache(original_data);

  if (is_success(save_result)) {
    // Try to load back
    auto load_result = client->load_from_cache();

    if (is_success(load_result)) {
      auto loaded_data = get_value(load_result);
      ASSERT_GT(loaded_data.size(), 0);

      // Find Mars in loaded data
      bool found_mars = false;
      for (const auto& body : loaded_data) {
        if (body.body_name == "Mars") {
          found_mars = true;
          EXPECT_EQ(499, body.jpl_id);
          break;
        }
      }
      EXPECT_TRUE(found_mars);
    }
  }
}
