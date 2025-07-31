#include <gtest/gtest.h>

#include <filesystem>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_data_manager.hpp"

using namespace solar_test;
using namespace SolarSystem::Testing;

/**
 * @brief Test fixture for environment management tests
 */
class EnvironmentManagementTest : public ::testing::Test, public TestFixture {
 protected:
  void SetUp() override { TestFixture::setup(); }

  void TearDown() override { TestFixture::teardown(); }
};

/**
 * @brief Test TemporaryDirectory functionality
 */
TEST_F(EnvironmentManagementTest, TemporaryDirectoryBasics) {
  // Create temporary directory
  auto temp_dir = create_temp_directory("test_env_");

  // Verify directory exists
  EXPECT_TRUE(temp_dir->exists());
  EXPECT_TRUE(std::filesystem::exists(temp_dir->path()));

  // Create file in directory
  temp_dir->create_file("test.txt", "test content");
  std::string file_path = temp_dir->path() + "/test.txt";
  EXPECT_TRUE(std::filesystem::exists(file_path));

  // Create subdirectory
  temp_dir->create_subdirectory("subdir");
  std::string subdir_path = temp_dir->path() + "/subdir";
  EXPECT_TRUE(std::filesystem::exists(subdir_path));
  EXPECT_TRUE(std::filesystem::is_directory(subdir_path));
}

/**
 * @brief Test TemporaryCache functionality
 */
TEST_F(EnvironmentManagementTest, TemporaryCacheBasics) {
  // Create temporary cache
  auto temp_cache = create_temp_cache("ephemeris");

  // Verify cache directory exists
  EXPECT_TRUE(std::filesystem::exists(temp_cache->cache_path()));

  // Populate with valid data
  temp_cache->populate_with_valid_data();
  EXPECT_TRUE(std::filesystem::exists(temp_cache->cache_file()));

  // Validate cache integrity
  EXPECT_TRUE(TestDataManager::validate_cache_integrity(temp_cache->cache_file()));
}

/**
 * @brief Test corrupted cache scenarios
 */
TEST_F(EnvironmentManagementTest, CorruptedCacheHandling) {
  // Test corrupted cache
  auto corrupted_cache = create_temp_cache("corrupted");
  corrupted_cache->populate_with_corrupted_data();
  EXPECT_FALSE(TestDataManager::validate_cache_integrity(corrupted_cache->cache_file()));

  // Test partially corrupted cache
  auto partial_cache = create_temp_cache("partial");
  partial_cache->simulate_partial_corruption();
  EXPECT_FALSE(TestDataManager::validate_cache_integrity(partial_cache->cache_file()));
}

/**
 * @brief Test TestDataManager functionality
 */
TEST_F(EnvironmentManagementTest, TestDataManagerBasics) {
  // Load JPL responses
  auto jpl_data = TestDataManager::load_jpl_responses("planets");
  EXPECT_FALSE(jpl_data.files.empty());
  EXPECT_EQ(jpl_data.name, "jpl_responses_planets");

  // Load ephemeris data
  auto ephemeris_data = TestDataManager::load_ephemeris_data("2024");
  EXPECT_FALSE(ephemeris_data.files.empty());
  EXPECT_EQ(ephemeris_data.name, "ephemeris_2024");

  // Load cache samples
  auto cache_samples = TestDataManager::load_cache_samples("valid");
  EXPECT_FALSE(cache_samples.files.empty());
  EXPECT_EQ(cache_samples.name, "cache_samples_valid");
}

/**
 * @brief Test realistic solar system data
 */
TEST_F(EnvironmentManagementTest, RealisticSolarSystemData) {
  auto solar_data = TestDataManager::load_realistic_solar_system_data();

  // Verify comprehensive data
  EXPECT_GE(solar_data.files.size(), 27);  // At least 27 celestial bodies
  EXPECT_EQ(solar_data.name, "realistic_solar_system");
  EXPECT_EQ(solar_data.metadata.at("data_type"), "comprehensive");

  // Verify specific files exist
  EXPECT_GT(solar_data.files.count("Earth_ephemeris.json"), 0);
  EXPECT_GT(solar_data.files.count("orbital_elements.json"), 0);
  EXPECT_GT(solar_data.files.count("physical_parameters.json"), 0);
}

/**
 * @brief Test test database creation
 */
TEST_F(EnvironmentManagementTest, TestDatabaseCreation) {
  auto temp_dir = create_temp_directory("db_test_");
  std::string db_path = temp_dir->path() + "/test_database";

  // Create test database
  TestDataManager::create_test_database(db_path);

  // Verify database was created
  EXPECT_TRUE(std::filesystem::exists(db_path));
  EXPECT_TRUE(std::filesystem::exists(db_path + "/metadata.json"));

  // Verify some data files exist
  EXPECT_TRUE(std::filesystem::exists(db_path + "/Earth_ephemeris.json"));
  EXPECT_TRUE(std::filesystem::exists(db_path + "/orbital_elements.json"));
}

/**
 * @brief Test automatic cleanup functionality
 */
TEST_F(EnvironmentManagementTest, AutomaticCleanup) {
  std::string temp_path;

  {
    // Create temporary directory in limited scope
    TemporaryDirectory temp_dir("cleanup_test_");
    temp_path = temp_dir.path();

    // Verify directory exists
    EXPECT_TRUE(std::filesystem::exists(temp_path));

    // Create some content
    temp_dir.create_file("test.txt", "content");
    EXPECT_TRUE(std::filesystem::exists(temp_path + "/test.txt"));
  }

  // After destructor, directory should be cleaned up
  EXPECT_FALSE(std::filesystem::exists(temp_path));
}

/**
 * @brief Test data validation functions
 */
TEST_F(EnvironmentManagementTest, DataValidation) {
  // Test JPL response validation
  std::string valid_jpl = "$$SOE\nEphemeris data here\n$$EOE";
  std::string invalid_jpl = "Invalid response";

  EXPECT_TRUE(TestDataManager::validate_jpl_response(valid_jpl));
  EXPECT_FALSE(TestDataManager::validate_jpl_response(invalid_jpl));

  // Test ephemeris data validation
  std::string valid_ephemeris = "EPHE" + std::string(20, 'A');  // Valid header + data
  std::string invalid_ephemeris = "INVALID";

  EXPECT_TRUE(TestDataManager::validate_ephemeris_data(valid_ephemeris));
  EXPECT_FALSE(TestDataManager::validate_ephemeris_data(invalid_ephemeris));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
