/**
 * @file test_cache_mock.cpp
 * @brief Unit tests for cache mock functionality
 */

#include <chrono>
#include <filesystem>
#include <thread>

#include "solar_test/framework/test_discovery.hpp"
#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;
using namespace SolarSystem::Testing::Mocks;
using namespace SolarSystem::JPL;

/**
 * @brief Test basic cache mock construction and configuration
 */
class CacheMockBasicTest : public TestCase {
 public:
  CacheMockBasicTest()
      : TestCase({"CacheMockBasicTest",
                  "Test basic cache mock construction and configuration",
                  {"unit", "mock", "cache"}}) {}

  void run() override {
    // Create default cache mock
    auto cache_mock = CacheMockFactory::create_default();
    assert_true(cache_mock != nullptr, "Cache mock should be created successfully");

    // Check default configuration
    const auto& config = cache_mock->config();
    assert_true(config.simulate_file_operations, "File operations should be simulated by default");
    assert_equals(0.0, config.disk_full_rate, "Disk full rate should be 0.0 by default");
    assert_equals(0.0, config.corruption_rate, "Corruption rate should be 0.0 by default");
    assert_true(config.enable_call_history, "Call history should be enabled by default");
  }
};

/**
 * @brief Test cache existence simulation
 */
class CacheMockExistenceTest : public TestCase {
 public:
  CacheMockExistenceTest()
      : TestCase({"CacheMockExistenceTest",
                  "Test cache existence simulation",
                  {"unit", "mock", "cache"}}) {}

  void run() override {
    auto cache_mock = CacheMockFactory::create_default();

    // Initially cache should exist (default behavior)
    assert_true(cache_mock->mock_file_exists(cache_mock->binary_cache_path()),
                "Cache should exist by default");

    // Set cache to not exist
    cache_mock->set_cache_exists(false);
    assert_false(cache_mock->mock_file_exists(cache_mock->binary_cache_path()),
                 "Cache should not exist after setting to false");

    // Set cache to exist again
    cache_mock->set_cache_exists(true);
    assert_true(cache_mock->mock_file_exists(cache_mock->binary_cache_path()),
                "Cache should exist after setting to true");
  }
};

/**
 * @brief Test cache validity simulation
 */
class CacheMockValidityTest : public TestCase {
 public:
  CacheMockValidityTest()
      : TestCase({"CacheMockValidityTest",
                  "Test cache validity simulation",
                  {"unit", "mock", "cache"}}) {}

  void run() override {
    auto cache_mock = CacheMockFactory::create_default();

    // Populate with valid data
    cache_mock->populate_with_valid_data();

    auto validation_result = cache_mock->mock_validate_cache();
    assert_true(is_success(validation_result), "Validation should succeed");
    assert_true(get_value(validation_result), "Cache should be valid");

    // Set cache as invalid
    cache_mock->set_cache_valid(false);
    validation_result = cache_mock->mock_validate_cache();
    assert_true(is_success(validation_result), "Validation call should succeed");
    assert_false(get_value(validation_result), "Cache should be invalid");
  }
};

/**
 * @brief Test cache corruption simulation
 */
class CacheMockCorruptionTest : public TestCase {
 public:
  CacheMockCorruptionTest()
      : TestCase({"CacheMockCorruptionTest",
                  "Test cache corruption simulation",
                  {"unit", "mock", "cache"}}) {}

  void run() override {
    auto cache_mock = CacheMockFactory::create_default();

    // Populate with valid data first
    cache_mock->populate_with_valid_data();

    // Verify data loads successfully
    auto load_result = cache_mock->mock_load_from_cache();
    assert_true(is_success(load_result), "Initial load should succeed");

    // Simulate corruption
    cache_mock->simulate_cache_corruption();

    // Now loading should fail
    load_result = cache_mock->mock_load_from_cache();
    assert_false(is_success(load_result), "Load should fail after corruption");
    assert_true(get_error(load_result) == JPLError::ValidationError, "Should get validation error");

    // Validation should also fail
    auto validation_result = cache_mock->mock_validate_cache();
    assert_true(is_success(validation_result), "Validation call should succeed");
    assert_false(get_value(validation_result), "Cache should be invalid after corruption");
  }
};

/**
 * @brief Test disk full simulation
 */
class CacheMockDiskFullTest : public TestCase {
 public:
  CacheMockDiskFullTest()
      : TestCase(
            {"CacheMockDiskFullTest", "Test disk full simulation", {"unit", "mock", "cache"}}) {}

  void run() override {
    auto cache_mock = CacheMockFactory::create_default();

    // Create test data
    std::vector<EphemerisData> test_data;
    EphemerisData test_body;
    test_body.jpl_id = 10;
    test_body.body_name = "Sun";
    test_body.epoch = std::chrono::system_clock::now();
    test_body.position = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
    test_body.velocity = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
    test_body.mass = 1.98847e30;
    test_data.push_back(test_body);

    // Simulate disk full
    cache_mock->simulate_disk_full();

    // Saving should fail
    auto save_result = cache_mock->mock_save_to_cache(test_data);
    assert_false(is_success(save_result), "Save should fail when disk is full");
    assert_true(save_result.has_value(), "Should have error value");
    assert_true(save_result.value() == JPLError::CacheError, "Should get cache error");
  }
};

/**
 * @brief Test cache data operations
 */
class CacheMockDataOperationsTest : public TestCase {
 public:
  CacheMockDataOperationsTest()
      : TestCase({"CacheMockDataOperationsTest",
                  "Test cache data loading and saving",
                  {"unit", "mock", "cache"}}) {}

  void run() override {
    auto cache_mock = CacheMockFactory::create_default();

    // Initially no data should be loaded (cache doesn't exist by default in this test)
    cache_mock->set_cache_exists(false);
    auto load_result = cache_mock->mock_load_from_cache();
    assert_false(is_success(load_result), "Load should fail when cache doesn't exist");
    assert_true(get_error(load_result) == JPLError::CacheError, "Should get cache error");

    // Populate with valid data
    cache_mock->populate_with_valid_data();
    load_result = cache_mock->mock_load_from_cache();
    assert_true(is_success(load_result), "Load should succeed with valid data");

    const auto& data = get_value(load_result);
    assert_true(data.size() > 0, "Should have loaded data");

    // Verify data structure
    for (const auto& body_data : data) {
      assert_true(body_data.jpl_id > 0, "JPL ID should be positive");
      assert_false(body_data.body_name.empty(), "Body name should not be empty");
      assert_true(body_data.mass > 0.0, "Mass should be positive");
    }
  }
};

/**
 * @brief Test temporary cache environment
 */
class TemporaryCacheTest : public TestCase {
 public:
  TemporaryCacheTest()
      : TestCase(
            {"TemporaryCacheTest", "Test temporary cache environment", {"unit", "mock", "cache"}}) {
  }

  void run() override {
    // Create temporary cache
    TemporaryCache temp_cache("test");

    // Initially no cache files
    assert_false(temp_cache.cache_files_exist(), "Cache files should not exist initially");

    // Populate with valid data
    temp_cache.populate_with_valid_data();
    assert_true(temp_cache.cache_files_exist(), "Cache files should exist after population");

    // Check file sizes
    auto sizes = temp_cache.cache_file_sizes();
    assert_true(sizes.size() > 0, "Should have file size information");

    // Verify paths
    assert_true(std::filesystem::exists(temp_cache.cache_path()), "Cache path should exist");
    assert_true(std::filesystem::exists(temp_cache.binary_cache_file()),
                "Binary cache file should exist");
    assert_true(std::filesystem::exists(temp_cache.json_cache_file()),
                "JSON cache file should exist");
    assert_true(std::filesystem::exists(temp_cache.metadata_file()), "Metadata file should exist");

    // Test corruption
    temp_cache.simulate_partial_corruption(0.1);
    assert_true(temp_cache.cache_files_exist(), "Files should still exist after corruption");

    // Create JPL client config
    auto client_config = temp_cache.create_client_config();
    assert_true(client_config.cache_directory == temp_cache.cache_path(),
                "Cache directory should match");
    assert_true(client_config.enable_binary_cache, "Binary cache should be enabled");
    assert_true(client_config.enable_json_cache, "JSON cache should be enabled");
  }
};

/**
 * @brief Test cache mock factory methods
 */
class CacheMockFactoryTest : public TestCase {
 public:
  CacheMockFactoryTest()
      : TestCase({"CacheMockFactoryTest",
                  "Test cache mock factory methods",
                  {"unit", "mock", "cache"}}) {}

  void run() override {
    // Test corruption testing factory
    auto corruption_mock = CacheMockFactory::create_for_corruption_testing();
    assert_true(corruption_mock != nullptr, "Corruption mock should be created");
    assert_true(corruption_mock->config().corruption_rate > 0.0,
                "Corruption rate should be positive");

    // Test disk full testing factory
    auto disk_full_mock = CacheMockFactory::create_for_disk_full_testing();
    assert_true(disk_full_mock != nullptr, "Disk full mock should be created");
    assert_true(disk_full_mock->config().disk_full_rate > 0.0, "Disk full rate should be positive");

    // Test performance testing factory
    auto perf_mock = CacheMockFactory::create_for_performance_testing();
    assert_true(perf_mock != nullptr, "Performance mock should be created");
    assert_true(perf_mock->config().simulate_file_operations,
                "File operations should be simulated");

    // Test valid cache factory
    auto valid_mock = CacheMockFactory::create_with_valid_cache();
    assert_true(valid_mock != nullptr, "Valid cache mock should be created");
    assert_true(valid_mock->config().cache_always_valid, "Cache should always be valid");

    // Test invalid cache factory
    auto invalid_mock = CacheMockFactory::create_with_invalid_cache();
    assert_true(invalid_mock != nullptr, "Invalid cache mock should be created");
    assert_true(invalid_mock->config().cache_always_invalid, "Cache should always be invalid");
  }
};

// Register all cache mock tests
REGISTER_TEST(CacheMockBasicTest);
REGISTER_TEST(CacheMockExistenceTest);
REGISTER_TEST(CacheMockValidityTest);
REGISTER_TEST(CacheMockCorruptionTest);
REGISTER_TEST(CacheMockDiskFullTest);
REGISTER_TEST(CacheMockDataOperationsTest);
REGISTER_TEST(TemporaryCacheTest);
REGISTER_TEST(CacheMockFactoryTest);

SOLAR_TEST_MAIN();
