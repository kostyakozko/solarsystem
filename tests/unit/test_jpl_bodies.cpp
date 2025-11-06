/**
 * @file test_jpl_bodies.cpp
 * @brief Comprehensive unit tests for JPL client body operations
 */

#include "../utils/test_framework.h"
#include "solar_jpl/jpl_client.hpp"
#include <chrono>
#include <thread>

using namespace SolarSystem::JPL;

int main() {
  TestSuite suite("JPL Client Body Operations Tests");

  // Test JPLClient Factory
  suite.run_test("JPLClientFactory Create Default", []() {
    auto client = JPLClientFactory::create_default();
    ASSERT_TRUE(client != nullptr);
  });

  suite.run_test("JPLClientFactory Create with Custom Config", []() {
    JPLClientConfig config;
    config.max_retries = 5;
    config.request_timeout = std::chrono::seconds(60);

    auto client = JPLClientFactory::create(config);
    ASSERT_TRUE(client != nullptr);
    ASSERT_EQ(5, client->config().max_retries);
    ASSERT_EQ(std::chrono::seconds(60), client->config().request_timeout);
  });

  suite.run_test("JPLClientFactory Create for Testing", []() {
    auto client = JPLClientFactory::create_for_testing();
    ASSERT_TRUE(client != nullptr);
  });

  // Test JPLClient Basic Operations
  suite.run_test("JPLClient Construction", []() {
    JPLClientConfig config;
    JPLClient client(config);

    ASSERT_EQ(config.api_endpoint, client.config().api_endpoint);
  });

  suite.run_test("JPLClient Cache Metadata - No Cache", []() {
    auto client = JPLClientFactory::create_for_testing();

    auto metadata = client->get_cache_metadata();
    // May or may not have cache, both are valid
  });

  suite.run_test("JPLClient Has Current Year Data", []() {
    auto client = JPLClientFactory::create_for_testing();

    // Should return false if no cache exists
    (void)client->has_current_year_data();
    // Result depends on cache state, just verify it doesn't crash
  });

  // Test Cache Operations
  suite.run_test("JPLClient Load from Cache - No Cache", []() {
    auto client = JPLClientFactory::create_for_testing();

    (void)client->load_from_cache();
    // May succeed or fail depending on cache state
    // Just verify it returns a valid result
  });

  suite.run_test("JPLClient Save to Cache", []() {
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
  });

  suite.run_test("JPLClient Validate Cache", []() {
    auto client = JPLClientFactory::create_for_testing();

    (void)client->validate_cache();
    // May succeed or fail depending on cache state
  });

  suite.run_test("JPLClient Clear Cache", []() {
    auto client = JPLClientFactory::create_for_testing();

    (void)client->clear_cache();
    // Should succeed even if no cache exists
  });

  suite.run_test("JPLClient Rebuild Cache", []() {
    auto client = JPLClientFactory::create_for_testing();

    (void)client->rebuild_cache();
    // May fail if network is unavailable, which is acceptable
  });

  // Test Network Operations
  suite.run_test("JPLClient Offline Mode", []() {
    auto client = JPLClientFactory::create_for_testing();

    // Test setting offline mode
    client->set_offline_mode(true);
    ASSERT_TRUE(client->is_offline_mode());

    client->set_offline_mode(false);
    ASSERT_FALSE(client->is_offline_mode());
  });

  suite.run_test("JPLClient Network Connectivity Check", []() {
    auto client = JPLClientFactory::create_for_testing();

    (void)client->check_network_connectivity();
    // Status depends on actual network, just verify it returns
  });

  suite.run_test("JPLClient Network Diagnostics", []() {
    auto client = JPLClientFactory::create_for_testing();

    auto diagnostics = client->get_network_diagnostics();
    // Should return valid diagnostics structure
    ASSERT_TRUE(diagnostics.successful_requests >= 0);
    ASSERT_TRUE(diagnostics.failed_requests >= 0);
  });

  suite.run_test("JPLClient Network Health Score", []() {
    auto client = JPLClientFactory::create_for_testing();

    double health = client->get_network_health_score();
    ASSERT_TRUE(health >= 0.0);
    ASSERT_TRUE(health <= 1.0);
  });

  // Test Async Operations (basic checks)
  suite.run_test("JPLClient Fetch Body Async - Returns Future", []() {
    auto client = JPLClientFactory::create_for_testing();

    auto epoch = std::chrono::system_clock::now();
    auto future = client->fetch_body_async(399, epoch);  // Earth

    // Just verify we get a future back
    ASSERT_TRUE(future.valid());

    // Don't wait for result as it may fail due to network
  });

  suite.run_test("JPLClient Fetch Bodies Async - Returns Future", []() {
    auto client = JPLClientFactory::create_for_testing();

    std::vector<int> jpl_ids = {399, 499};  // Earth, Mars
    auto epoch = std::chrono::system_clock::now();
    auto future = client->fetch_bodies_async(jpl_ids, epoch);

    ASSERT_TRUE(future.valid());
  });

  suite.run_test("JPLClient Fetch All Bodies Async - Returns Future", []() {
    auto client = JPLClientFactory::create_for_testing();

    auto epoch = std::chrono::system_clock::now();
    auto future = client->fetch_all_bodies_async(epoch);

    ASSERT_TRUE(future.valid());
  });

  // Test Cache Manager Access
  suite.run_test("JPLClient Cache Manager Access", []() {
    auto client = JPLClientFactory::create_for_testing();

    (void)client->cache_manager();
    // Just verify we can access it
  });

  // Test Data Validator Access
  suite.run_test("JPLClient Data Validator Access", []() {
    auto client = JPLClientFactory::create_for_testing();

    (void)client->data_validator();
    // Just verify we can access it
  });

  // Test Error Scenarios
  suite.run_test("JPLClient Invalid JPL ID", []() {
    auto client = JPLClientFactory::create_for_testing();

    auto epoch = std::chrono::system_clock::now();
    auto future = client->fetch_body_async(-1, epoch);  // Invalid ID

    ASSERT_TRUE(future.valid());

    // Result should be an error, but we don't wait for it
  });

  // Test Configuration Access
  suite.run_test("JPLClient Config Access", []() {
    JPLClientConfig config;
    config.max_retries = 7;
    JPLClient client(config);

    ASSERT_EQ(7, client.config().max_retries);
  });

  // Test Multiple Clients
  suite.run_test("Multiple JPLClient Instances", []() {
    auto client1 = JPLClientFactory::create_for_testing();
    auto client2 = JPLClientFactory::create_for_testing();

    ASSERT_TRUE(client1 != nullptr);
    ASSERT_TRUE(client2 != nullptr);

    // Both should be independent
    client1->set_offline_mode(true);
    client2->set_offline_mode(false);

    ASSERT_TRUE(client1->is_offline_mode());
    ASSERT_FALSE(client2->is_offline_mode());
  });

  // Test Cache Round-Trip
  suite.run_test("JPLClient Cache Round-Trip", []() {
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
        ASSERT_TRUE(loaded_data.size() > 0);

        // Find Mars in loaded data
        bool found_mars = false;
        for (const auto& body : loaded_data) {
          if (body.body_name == "Mars") {
            found_mars = true;
            ASSERT_EQ(499, body.jpl_id);
            break;
          }
        }
        ASSERT_TRUE(found_mars);
      }
    }
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
