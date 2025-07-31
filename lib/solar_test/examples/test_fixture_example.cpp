#include <iostream>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_data_manager.hpp"

using namespace SolarSystem::Testing;
using namespace solar_test;

/**
 * @brief Example test fixture demonstrating test environment management
 */
class ExampleTestFixture : public TestFixture {
 public:
  void setup() override {
    // Create temporary directories for test isolation
    temp_data_dir_ = create_temp_directory("test_data_");
    temp_cache_dir_ = create_temp_directory("test_cache_");

    // Create temporary cache for testing
    ephemeris_cache_ = create_temp_cache("ephemeris");
    jpl_cache_ = create_temp_cache("jpl");

    // Populate with test data
    ephemeris_cache_->populate_with_valid_data();
    jpl_cache_->populate_with_valid_data();

    std::cout << "Test fixture setup complete\n";
    std::cout << "Data directory: " << temp_data_dir_->path() << "\n";
    std::cout << "Cache directory: " << temp_cache_dir_->path() << "\n";
  }

  void teardown() override {
    // Automatic cleanup handled by RAII
    std::cout << "Test fixture teardown complete\n";
  }

 protected:
  TemporaryDirectory* temp_data_dir_ = nullptr;
  TemporaryDirectory* temp_cache_dir_ = nullptr;
  TemporaryCache* ephemeris_cache_ = nullptr;
  TemporaryCache* jpl_cache_ = nullptr;
};

/**
 * @brief Example test case using the test fixture
 */
class CacheIntegrityTest : public TestCase, public ExampleTestFixture {
 public:
  CacheIntegrityTest()
      : TestCase({.name = "cache_integrity_test",
                  .description = "Test cache integrity validation",
                  .tags = {"cache", "integration"},
                  .timeout = std::chrono::seconds(30)}) {}

  void setup() override { ExampleTestFixture::setup(); }

  void run() override {
    // Test valid cache
    std::string cache_path = ephemeris_cache_->cache_file();
    assert_true(TestDataManager::validate_cache_integrity(cache_path),
                "Valid cache should pass integrity check");

    // Test corrupted cache
    TemporaryCache corrupted_cache("corrupted");
    corrupted_cache.populate_with_corrupted_data();
    assert_false(TestDataManager::validate_cache_integrity(corrupted_cache.cache_file()),
                 "Corrupted cache should fail integrity check");

    // Test partial corruption
    TemporaryCache partial_cache("partial");
    partial_cache.simulate_partial_corruption();
    assert_false(TestDataManager::validate_cache_integrity(partial_cache.cache_file()),
                 "Partially corrupted cache should fail integrity check");
  }

  void teardown() override { ExampleTestFixture::teardown(); }
};

/**
 * @brief Example test demonstrating test data management
 */
class TestDataManagementExample : public TestCase {
 public:
  TestDataManagementExample()
      : TestCase({.name = "test_data_management_example",
                  .description = "Demonstrate test data management capabilities",
                  .tags = {"data", "example"},
                  .timeout = std::chrono::seconds(60)}) {}

  void run() override {
    // Load JPL response test data
    auto jpl_data = TestDataManager::load_jpl_responses("planets");
    assert_false(jpl_data.files.empty(), "JPL data should contain files");
    assert_true(jpl_data.files.count("mercury.json") > 0, "Should contain Mercury data");

    // Load ephemeris test data
    auto ephemeris_data = TestDataManager::load_ephemeris_data("2024");
    assert_false(ephemeris_data.files.empty(), "Ephemeris data should contain files");

    // Load cache samples
    auto cache_samples = TestDataManager::load_cache_samples("valid");
    assert_false(cache_samples.files.empty(), "Cache samples should contain files");

    // Create test environment
    auto test_env = TestDataManager::create_test_environment();
    assert_true(test_env->exists(), "Test environment should exist");

    // Create files in test environment
    test_env->create_file("test_file.txt", "test content");
    test_env->create_subdirectory("subdir");

    // Validate test data
    std::string sample_jpl = TestDataManager::load_jpl_responses("planets").files["mercury.json"];
    assert_true(TestDataManager::validate_jpl_response(sample_jpl),
                "Sample JPL response should be valid");
  }
};

/**
 * @brief Example test demonstrating realistic solar system data
 */
class RealisticDataTest : public TestCase {
 public:
  RealisticDataTest()
      : TestCase({.name = "realistic_data_test",
                  .description = "Test with realistic solar system data",
                  .tags = {"realistic", "comprehensive"},
                  .timeout = std::chrono::minutes(2)}) {}

  void run() override {
    // Load realistic solar system data
    auto solar_data = TestDataManager::load_realistic_solar_system_data();

    // Verify we have data for all expected bodies
    assert_true(solar_data.files.size() >= 27, "Should have data for at least 27 bodies");
    assert_true(solar_data.files.count("Earth_ephemeris.json") > 0, "Should contain Earth data");
    assert_true(solar_data.files.count("orbital_elements.json") > 0,
                "Should contain orbital elements");
    assert_true(solar_data.files.count("physical_parameters.json") > 0,
                "Should contain physical parameters");

    // Validate metadata
    assert_equals(std::string("realistic_solar_system"), solar_data.name,
                  "Dataset name should match");
    assert_equals(std::string("comprehensive"), solar_data.metadata.at("data_type"),
                  "Data type should be comprehensive");

    // Create test database
    auto temp_dir = TestDataManager::create_test_environment();
    std::string db_path = temp_dir->path() + "/test_database";
    TestDataManager::create_test_database(db_path);

    // Verify database was created
    assert_true(std::filesystem::exists(db_path), "Test database directory should exist");
    assert_true(std::filesystem::exists(db_path + "/metadata.json"), "Metadata file should exist");
  }
};

// Example usage demonstration
int main() {
  std::cout << "=== Test Environment Management Examples ===\n\n";

  // Example 1: Basic test fixture usage
  std::cout << "1. Testing with ExampleTestFixture:\n";
  CacheIntegrityTest cache_test;
  cache_test.setup();
  try {
    cache_test.run();
    std::cout << "✓ Cache integrity test passed\n";
  } catch (const std::exception& e) {
    std::cout << "✗ Cache integrity test failed: " << e.what() << "\n";
  }
  cache_test.teardown();

  std::cout << "\n2. Testing data management:\n";
  TestDataManagementExample data_test;
  try {
    data_test.run();
    std::cout << "✓ Data management test passed\n";
  } catch (const std::exception& e) {
    std::cout << "✗ Data management test failed: " << e.what() << "\n";
  }

  std::cout << "\n3. Testing with realistic data:\n";
  RealisticDataTest realistic_test;
  try {
    realistic_test.run();
    std::cout << "✓ Realistic data test passed\n";
  } catch (const std::exception& e) {
    std::cout << "✗ Realistic data test failed: " << e.what() << "\n";
  }

  std::cout << "\n=== Cleanup ===\n";
  TestDataManager::cleanup_test_data();
  std::cout << "All temporary test data cleaned up\n";

  return 0;
}
