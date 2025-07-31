#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace solar_test {

/**
 * @brief Manages test data sets for various testing scenarios
 */
struct TestDataSet {
  std::string name;
  std::string description;
  std::map<std::string, std::string> files;
  std::map<std::string, std::string> metadata;
};

/**
 * @brief RAII class for creating isolated temporary directories for tests
 */
class TemporaryDirectory {
 public:
  explicit TemporaryDirectory(const std::string& prefix = "solar_test_");
  ~TemporaryDirectory();

  // Non-copyable but movable
  TemporaryDirectory(const TemporaryDirectory&) = delete;
  TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
  TemporaryDirectory(TemporaryDirectory&&) noexcept;
  TemporaryDirectory& operator=(TemporaryDirectory&&) noexcept;

  [[nodiscard]] std::string path() const;
  void create_file(const std::string& name, const std::string& content);
  void create_subdirectory(const std::string& name);
  [[nodiscard]] bool exists() const;

 private:
  std::filesystem::path temp_path_;
  bool cleanup_on_destroy_ = true;
};

/**
 * @brief RAII class for creating temporary cache environments for testing
 */
class TemporaryCache {
 public:
  explicit TemporaryCache(const std::string& cache_type = "ephemeris");
  ~TemporaryCache();

  // Non-copyable but movable
  TemporaryCache(const TemporaryCache&) = delete;
  TemporaryCache& operator=(const TemporaryCache&) = delete;
  TemporaryCache(TemporaryCache&&) noexcept;
  TemporaryCache& operator=(TemporaryCache&&) noexcept;

  void populate_with_valid_data();
  void populate_with_corrupted_data();
  void simulate_partial_corruption();
  [[nodiscard]] std::string cache_path() const;
  [[nodiscard]] std::string cache_file() const;

 private:
  std::unique_ptr<TemporaryDirectory> temp_dir_;
  std::string cache_file_;
  std::string cache_type_;
};

/**
 * @brief Base class for test fixtures with setup/teardown lifecycle
 */
class TestFixture {
 public:
  TestFixture() = default;
  virtual ~TestFixture() = default;

  // Non-copyable but movable
  TestFixture(const TestFixture&) = delete;
  TestFixture& operator=(const TestFixture&) = delete;
  TestFixture(TestFixture&&) = default;
  TestFixture& operator=(TestFixture&&) = default;

  virtual void setup() {}
  virtual void teardown() {}

 protected:
  std::vector<std::unique_ptr<TemporaryDirectory>> temp_directories_;
  std::vector<std::unique_ptr<TemporaryCache>> temp_caches_;

  // Helper methods for derived classes
  TemporaryDirectory* create_temp_directory(const std::string& prefix = "test_");
  TemporaryCache* create_temp_cache(const std::string& cache_type = "ephemeris");
};

/**
 * @brief Manages test data loading, validation, and cleanup
 */
class TestDataManager {
 public:
  // Test data loading
  [[nodiscard]] static TestDataSet load_jpl_responses(const std::string& scenario);
  [[nodiscard]] static TestDataSet load_ephemeris_data(const std::string& time_period);
  [[nodiscard]] static TestDataSet load_cache_samples(const std::string& cache_type);

  // Temporary test environments
  [[nodiscard]] static std::unique_ptr<TemporaryDirectory> create_test_environment();
  [[nodiscard]] static std::unique_ptr<TemporaryCache> create_test_cache();

  // Data validation
  [[nodiscard]] static bool validate_jpl_response(const std::string& response);
  [[nodiscard]] static bool validate_ephemeris_data(const std::string& data);
  [[nodiscard]] static bool validate_cache_integrity(const std::string& cache_path);

  // Test database creation
  static void create_test_database(const std::string& output_path);
  [[nodiscard]] static TestDataSet load_realistic_solar_system_data();

  // Automatic cleanup (requirement 7.5)
  static void cleanup_test_data();
  static void register_cleanup_handler();

 private:
  static std::vector<std::string> active_temp_directories_;
  static bool cleanup_registered_;

  // Friend class to allow TemporaryDirectory access to static members
  friend class TemporaryDirectory;
};

// Helper functions for creating test data (implementation details)
std::string load_file_or_fallback(const std::string& file_path, const std::string& fallback);
std::string create_valid_ephemeris_cache();
std::string create_valid_jpl_cache();
std::string create_json_cache_data();
std::string create_valid_cache_header();
std::string load_sample_jpl_response(const std::string& body);
std::string create_timeout_response();
std::string create_error_response(const std::string& message);
std::string create_server_error_response();
std::string create_ephemeris_binary_data();
std::string create_ephemeris_json_data();
std::string create_historical_ephemeris_data();
std::string create_valid_cache_binary();
std::string create_valid_cache_json();
std::string create_corrupted_header_cache();
std::string create_corrupted_body_cache();
std::string create_realistic_body_data(const std::string& body);
std::string create_orbital_elements_data();
std::string create_physical_parameters_data();

}  // namespace solar_test
