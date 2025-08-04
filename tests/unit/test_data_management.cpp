/**
 * @file test_data_management.cpp
 * @brief Unit tests for test data management system
 */

#include "../utils/test_framework.h"
#include "solar_test/framework/test_data_manager.hpp"

using namespace solar_test;

int main() {
  TestSuite suite("Test Data Management Tests");

  suite.run_test("TemporaryDirectory Creation", []() {
    auto temp_dir = std::make_unique<TemporaryDirectory>("test_prefix_");

    ASSERT_TRUE(temp_dir->exists());

    // Create a test file
    temp_dir->create_file("test.txt", "test content");

    // Create a subdirectory
    temp_dir->create_subdirectory("subdir");
    auto subdir_path = std::filesystem::path(temp_dir->path()) / "subdir";
    ASSERT_TRUE(std::filesystem::exists(subdir_path));
  });

  suite.run_test("TemporaryCache Operations", []() {
    auto temp_cache = std::make_unique<TemporaryCache>("json");

    // Populate with valid data
    temp_cache->populate_with_valid_data();
    ASSERT_FALSE(temp_cache->cache_path().empty());

    // Test corrupted data
    temp_cache->populate_with_corrupted_data();
    ASSERT_FALSE(temp_cache->cache_path().empty());
  });

  suite.run_test("JPL Response Loading", []() {
    auto jpl_data = TestDataManager::load_jpl_responses("valid");

    ASSERT_FALSE(jpl_data.files.empty());
    ASSERT_EQ(jpl_data.name, "jpl_responses_valid");

    // Validate at least one response
    for (const auto& [filename, content] : jpl_data.files) {
      ASSERT_TRUE(TestDataManager::validate_jpl_response(content));
    }
  });

  suite.run_test("Ephemeris Data Loading", []() {
    auto ephemeris_data = TestDataManager::load_ephemeris_data("j2000_epoch");

    ASSERT_FALSE(ephemeris_data.files.empty());
    ASSERT_EQ(ephemeris_data.name, "ephemeris_j2000_epoch");

    // Validate JSON format
    for (const auto& [filename, content] : ephemeris_data.files) {
      ASSERT_TRUE(TestDataManager::validate_ephemeris_data(content));
    }
  });

  suite.run_test("Cache Sample Loading", []() {
    auto cache_data = TestDataManager::load_cache_samples("valid");

    ASSERT_FALSE(cache_data.files.empty());
    ASSERT_EQ(cache_data.name, "cache_samples_valid");
  });

  suite.run_test("Test Environment Management", []() {
    // Test test environment creation
    auto test_env = TestDataManager::create_test_environment();
    ASSERT_TRUE(test_env->exists());

    // Test test cache creation
    auto test_cache = TestDataManager::create_test_cache();
    ASSERT_FALSE(test_cache->cache_path().empty());

    test_cache->populate_with_valid_data();
    ASSERT_FALSE(test_cache->cache_path().empty());
  });

  suite.run_test("Cache Integrity Validation", []() {
    auto temp_cache = std::make_unique<TemporaryCache>("ephemeris");
    temp_cache->populate_with_valid_data();

    // Test cache integrity validation
    ASSERT_TRUE(TestDataManager::validate_cache_integrity(temp_cache->cache_file()));
  });

  suite.run_test("Realistic Solar System Data", []() {
    auto solar_system_data = TestDataManager::load_realistic_solar_system_data();

    ASSERT_FALSE(solar_system_data.files.empty());
    ASSERT_FALSE(solar_system_data.name.empty());
    ASSERT_FALSE(solar_system_data.description.empty());
  });

  // Cleanup after all tests
  TestDataManager::cleanup_test_data();

  suite.print_summary();
  return suite.all_passed() ? 0 : 1;
}
