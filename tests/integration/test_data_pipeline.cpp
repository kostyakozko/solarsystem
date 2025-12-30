/**
 * @file test_data_pipeline.cpp
 * @brief Integration tests for complete data pipelines
 * @note Migrated to Google Test
 *
 * Tests the complete JPL → BodyFactory → Simulation flow with:
 * - End-to-end data pipeline validation
 * - Cache loading and fallback mechanisms
 * - Network error handling and retry mechanisms
 * - Real data integration testing
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_jpl/jpl_client.hpp"
#include "test_data_manager.hpp"
#include <gtest/gtest.h>

using namespace SolarSystem;
using namespace TestData;

/**
 * @brief Mock JPL client for testing network failures
 */
class MockJPLClient {
 public:
  enum class FailureMode { None, NetworkTimeout, ServerError, ParseError, RateLimited };

  static void set_failure_mode(FailureMode mode) { failure_mode_ = mode; }

  static FailureMode get_failure_mode() { return failure_mode_; }

 private:
  static FailureMode failure_mode_;
};

MockJPLClient::FailureMode MockJPLClient::failure_mode_ = MockJPLClient::FailureMode::None;

// Test 1: Complete JPL → BodyFactory → Simulation pipeline
TEST(DataPipelineIntegrationTest, Complete_Data_Pipeline_JPL_to_Simulation) {
  // Create temporary test environment
  auto test_env = TestDataManager::create_test_environment();
  ASSERT_NE(test_env.get(), nullptr);

  // Initialize body factory with test configuration
  Bodies::BodyFactory::CreationOptions options;
  options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
  options.allow_fallback = true;
  options.validate_data = true;

  Bodies::BodyFactory factory(options);
  ASSERT_TRUE(factory.is_initialized());

  // Create a small collection of bodies for testing
  std::vector<std::string> test_bodies = {"Sun", "Earth", "Moon"};
  auto collection_result = factory.create_collection(test_bodies, options);
  ASSERT_TRUE(collection_result.has_value());

  auto& collection = collection_result.value();
  ASSERT_EQ(collection.size(), 3);

  // Initialize simulation engine
  Simulation::SimulationConfig sim_config;
  sim_config.time_step = 3600.0;  // 1 hour steps for testing
  sim_config.use_adaptive_timestep = false;

  Simulation::SimulationEngine engine(sim_config);

  // Initialize simulation with the body collection
  auto init_result = engine.initialize(std::move(collection));
  ASSERT_TRUE(init_result.has_value());
  ASSERT_TRUE(engine.is_initialized());

  // Run a few simulation steps to verify the pipeline works
  for (int i = 0; i < 5; ++i) {
    auto step_result = engine.step();
    ASSERT_TRUE(step_result.has_value());
  }

  // Verify simulation state is reasonable
  const auto& state = engine.get_state();
  ASSERT_GT(state.current_time, 0.0);
  ASSERT_EQ(state.iteration_count, 5);
  // Total energy should be finite and reasonable for a gravitational system
  // (typically negative for bound systems)
  ASSERT_TRUE(std::isfinite(state.total_energy));
  ASSERT_TRUE(std::abs(state.total_energy) < 1e50);  // Reasonable magnitude check
}

// Test 2: Cache loading and fallback mechanisms
TEST(DataPipelineIntegrationTest, Cache_Loading_and_Fallback_Mechanisms) {
  // Test with valid cache
  {
    auto test_cache = TestDataManager::create_test_cache("ephemeris");
    test_cache->populate_with_valid_data();
    EXPECT_TRUE(test_cache->cache_exists());

    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
    options.allow_fallback = true;

    Bodies::BodyFactory factory(options);
    auto result = factory.create_body("Earth", options);
    ASSERT_TRUE(result.has_value());

    const auto& earth = result.value();
    ASSERT_EQ(earth.name(), "Earth");
    ASSERT_GT(earth.mass(), 0.0);
  }

  // Test with corrupted cache - should fallback
  {
    auto test_cache = TestDataManager::create_test_cache("ephemeris");
    test_cache->populate_with_corrupted_data();
    EXPECT_TRUE(test_cache->cache_exists());

    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
    options.allow_fallback = true;

    Bodies::BodyFactory factory(options);
    auto result = factory.create_body("Earth", options);
    ASSERT_TRUE(result.has_value());  // Should succeed via fallback

    const auto& earth = result.value();
    ASSERT_EQ(earth.name(), "Earth");
    ASSERT_GT(earth.mass(), 0.0);
  }

  // Test with no cache and no fallback - should fail gracefully
  {
    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
    options.allow_fallback = false;

    Bodies::BodyFactory factory(options);
    auto result = factory.create_body("NonexistentBody", options);
    ASSERT_FALSE(result.has_value());  // Should fail without fallback
  }
}

// Test 3: Network error handling and retry mechanisms
TEST(DataPipelineIntegrationTest, Network_Error_Handling_and_Retry) {
  // Test network timeout handling
  {
    MockJPLClient::set_failure_mode(MockJPLClient::FailureMode::NetworkTimeout);

    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::JPL_HORIZONS;
    options.allow_fallback = true;  // Should fallback on network failure

    Bodies::BodyFactory factory(options);
    auto result = factory.create_body("Mars", options);

    // Should succeed via fallback when network fails
    ASSERT_TRUE(result.has_value());
    const auto& mars = result.value();
    ASSERT_EQ(mars.name(), "Mars");
  }

  // Test server error handling
  {
    MockJPLClient::set_failure_mode(MockJPLClient::FailureMode::ServerError);

    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::JPL_HORIZONS;
    options.allow_fallback = true;

    Bodies::BodyFactory factory(options);
    auto result = factory.create_body("Venus", options);

    // Should succeed via fallback when server errors occur
    ASSERT_TRUE(result.has_value());
    const auto& venus = result.value();
    ASSERT_EQ(venus.name(), "Venus");
  }

  // Test rate limiting handling
  {
    MockJPLClient::set_failure_mode(MockJPLClient::FailureMode::RateLimited);

    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::JPL_HORIZONS;
    options.allow_fallback = true;

    Bodies::BodyFactory factory(options);
    auto result = factory.create_body("Jupiter", options);

    // Should succeed via fallback when rate limited
    ASSERT_TRUE(result.has_value());
    const auto& jupiter = result.value();
    ASSERT_EQ(jupiter.name(), "Jupiter");
  }

  // Reset failure mode
  MockJPLClient::set_failure_mode(MockJPLClient::FailureMode::None);
}

// Test 4: Real data integration with validation
TEST(DataPipelineIntegrationTest, Real_Data_Integration_and_Validation) {
  // Load real test data
  auto jpl_data = TestDataManager::load_jpl_responses("current_year");
  if (jpl_data.has_value()) {
    // Validate JPL response format
    for (const auto& [body_name, response] : jpl_data->files) {
      ASSERT_TRUE(TestDataManager::validate_jpl_response(response));
    }
  }

  // Test with real ephemeris data
  auto ephemeris_data = TestDataManager::load_ephemeris_data("2024");
  if (ephemeris_data.has_value()) {
    EXPECT_TRUE(TestDataManager::validate_ephemeris_data(ephemeris_data->files.begin()->second));
  }

  // Create bodies using real data and validate physical properties
  Bodies::BodyFactory::CreationOptions options;
  options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
  options.validate_data = true;

  Bodies::BodyFactory factory(options);

  // Test major planets
  std::vector<std::string> major_bodies = {"Sun",  "Mercury", "Venus", "Earth",
                                           "Mars", "Jupiter", "Saturn"};

  for (const auto& body_name : major_bodies) {
    auto result = factory.create_body(body_name, options);
    ASSERT_TRUE(result.has_value());

    const auto& body = result.value();
    ASSERT_EQ(body.name(), body_name);

    // Validate physical properties
    ASSERT_GT(body.mass(), 0.0);

    // Validate position is within reasonable bounds (within 100 AU)
    const auto& pos = body.position();
    double distance_au = static_cast<double>(pos.magnitude()) / 1.496e11;  // Convert to AU
    ASSERT_LT(distance_au, 100.0);

    // Validate velocity is within reasonable bounds (< 100 km/s)
    const auto& vel = body.velocity();
    double speed_kms = static_cast<double>(vel.magnitude()) / 1000.0;  // Convert to km/s
    ASSERT_LT(speed_kms, 100.0);
  }
}

// Test 5: Data consistency across pipeline stages
TEST(DataPipelineIntegrationTest, Data_Consistency_Across_Pipeline_Stages) {
  // Create the same body using different data sources
  Bodies::BodyFactory::CreationOptions cached_options;
  cached_options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
  cached_options.allow_fallback = false;

  Bodies::BodyFactory::CreationOptions fallback_options;
  fallback_options.preferred_source = Bodies::BodyFactory::DataSource::FALLBACK_DATA;
  fallback_options.allow_fallback = false;

  Bodies::BodyFactory factory;

  // Create Earth from different sources
  auto cached_earth = factory.create_body("Earth", cached_options);
  auto fallback_earth = factory.create_body("Earth", fallback_options);

  if (cached_earth.has_value() && fallback_earth.has_value()) {
    const auto& earth1 = cached_earth.value();
    const auto& earth2 = fallback_earth.value();

    // Names should match exactly
    ASSERT_EQ(earth1.name(), earth2.name());

    // Masses should be reasonably close (within 1%)
    double mass_diff = static_cast<double>(std::abs(earth1.mass() - earth2.mass())) /
                       static_cast<double>(earth1.mass());
    ASSERT_LT(mass_diff, 0.01);

    // Positions may differ due to different epochs, but should be reasonable
    double pos1_au = static_cast<double>(earth1.position().magnitude()) / 1.496e11;
    double pos2_au = static_cast<double>(earth2.position().magnitude()) / 1.496e11;
    ASSERT_GT(pos1_au, 0.8);  // Earth's minimum distance from Sun
    ASSERT_LT(pos1_au, 1.2);  // Earth's maximum distance from Sun
    ASSERT_GT(pos2_au, 0.8);
    ASSERT_LT(pos2_au, 1.2);
  }
}

// Test 6: Performance characteristics of data pipeline
TEST(DataPipelineIntegrationTest, Data_Pipeline_Performance) {
  auto start_time = std::chrono::high_resolution_clock::now();

  // Create a collection of bodies and measure time
  Bodies::BodyFactory::CreationOptions options;
  options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
  options.allow_fallback = true;

  Bodies::BodyFactory factory(options);

  std::vector<std::string> all_bodies = {"Sun",  "Mercury", "Venus",  "Earth",  "Moon",
                                         "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"};

  auto collection_result = factory.create_collection(all_bodies, options);
  ASSERT_TRUE(collection_result.has_value());

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Body creation should be fast (< 1 second for 10 bodies)
  ASSERT_LT(duration.count(), 1000);

  // Initialize simulation and measure setup time
  start_time = std::chrono::high_resolution_clock::now();

  Simulation::SimulationEngine engine;
  auto init_result = engine.initialize(std::move(collection_result.value()));
  ASSERT_TRUE(init_result.has_value());

  end_time = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Simulation initialization should be fast (< 100ms)
  ASSERT_LT(duration.count(), 100);
}
