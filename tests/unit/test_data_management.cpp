/**
 * @file test_data_management.cpp
 * @brief Unit tests for test data management system
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>
#include "solar_test/framework/test_data_manager.hpp"

using namespace solar_test;
TEST(TestDataManagementTests, TemporaryDirectory_Creation) {
    auto temp_dir = std::make_unique<TemporaryDirectory>("test_prefix_");

    EXPECT_TRUE(temp_dir->exists());

    // Create a test file
    temp_dir->create_file("test.txt", "test content");

    // Create a subdirectory
    temp_dir->create_subdirectory("subdir");
    auto subdir_path = std::filesystem::path(temp_dir->path()) / "subdir";
    ASSERT_TRUE(std::filesystem::exists(subdir_path));
}

TEST(TestDataManagementTests, TemporaryCache_Operations) {
    auto temp_cache = std::make_unique<TemporaryCache>("json");

    // Populate with valid data
    temp_cache->populate_with_valid_data();
    ASSERT_FALSE(temp_cache->cache_path().empty());

    // Test corrupted data
    temp_cache->populate_with_corrupted_data();
    ASSERT_FALSE(temp_cache->cache_path().empty());
}

TEST(TestDataManagementTests, JPL_Response_Loading) {
    auto jpl_data = TestDataManager::load_jpl_responses("valid");

    ASSERT_FALSE(jpl_data.files.empty());
    ASSERT_EQ(jpl_data.name, "jpl_responses_valid");

    // Validate at least one response
    for (const auto& [filename, content] : jpl_data.files) {
      ASSERT_TRUE(TestDataManager::validate_jpl_response(content));
    }
}

TEST(TestDataManagementTests, Ephemeris_Data_Loading) {
    auto ephemeris_data = TestDataManager::load_ephemeris_data("j2000_epoch");

    ASSERT_FALSE(ephemeris_data.files.empty());
    ASSERT_EQ(ephemeris_data.name, "ephemeris_j2000_epoch");

    // Validate JSON format
    for (const auto& [filename, content] : ephemeris_data.files) {
      ASSERT_TRUE(TestDataManager::validate_ephemeris_data(content));
    }
}

TEST(TestDataManagementTests, Cache_Sample_Loading) {
    auto cache_data = TestDataManager::load_cache_samples("valid");

    ASSERT_FALSE(cache_data.files.empty());
    ASSERT_EQ(cache_data.name, "cache_samples_valid");
}

TEST(TestDataManagementTests, Test_Environment_Management) {
    // Test test environment creation
    auto test_env = TestDataManager::create_test_environment();
    EXPECT_TRUE(test_env->exists());

    // Test test cache creation
    auto test_cache = TestDataManager::create_test_cache();
    ASSERT_FALSE(test_cache->cache_path().empty());

    test_cache->populate_with_valid_data();
    ASSERT_FALSE(test_cache->cache_path().empty());
}

TEST(TestDataManagementTests, Cache_Integrity_Validation) {
    auto temp_cache = std::make_unique<TemporaryCache>("ephemeris");
    temp_cache->populate_with_valid_data();

    // Test cache integrity validation
    EXPECT_TRUE(TestDataManager::validate_cache_integrity(temp_cache->cache_file()));
}

TEST(TestDataManagementTests, Realistic_Solar_System_Data) {
    auto solar_system_data = TestDataManager::load_realistic_solar_system_data();

    ASSERT_FALSE(solar_system_data.files.empty());
    ASSERT_FALSE(solar_system_data.name.empty());
    ASSERT_FALSE(solar_system_data.description.empty());

    // Cleanup after test
    TestDataManager::cleanup_test_data();
}

