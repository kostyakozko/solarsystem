/**
 * @file test_data_manager.hpp
 * @brief Test data management and validation utilities
 *
 * Provides comprehensive test data loading, validation, and management
 * capabilities for the Solar System Suite testing framework.
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/math/vector3.hpp"

namespace TestData {

/**
 * @brief Test data set metadata and content
 */
struct TestDataSet {
  std::string name;
  std::string description;
  std::string version;
  std::string created_date;
  std::string source;
  std::map<std::string, std::string> files;
  std::map<std::string, std::string> metadata;
  std::string checksum;
};

/**
 * @brief Temporary directory for isolatt environments
 */
class TemporaryDirectory {
 public:
  explicit TemporaryDirectory(const std::string& prefix = "solar_test_");
  ~TemporaryDirectory();

  // Non-copyable, movable
  TemporaryDirectory(const TemporaryDirectory&) = delete;
  TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
  TemporaryDirectory(TemporaryDirectory&&) = default;
  TemporaryDirectory& operator=(TemporaryDirectory&&) = default;

  [[nodiscard]] std::filesystem::path path() const { return temp_path_; }
  [[nodiscard]] std::string path_string() const { return temp_path_.string(); }

  // File and directory operations
  void create_file(const std::string& name, const std::string& content);
  void create_subdirectory(const std::string& name);
  void copy_file(const std::filesystem::path& source, const std::string& dest_name);

  // Validation
  [[nodiscard]] bool exists() const;
  [[nodiscard]] bool is_empty() const;
  [[nodiscard]] size_t file_count() const;

 private:
  std::filesystem::path temp_path_;
  bool cleanup_on_destroy_ = true;
};

/**
 * @brief Temporary cache environment for testing
 */
class TemporaryCache {
 public:
  explicit TemporaryCache(const std::string& cache_type = "ephemeris");
  ~TemporaryCache() = default;

  // Non-copyable, movable
  TemporaryCache(const TemporaryCache&) = delete;
  TemporaryCache& operator=(const TemporaryCache&) = delete;
  TemporaryCache(TemporaryCache&&) = default;
  TemporaryCache& operator=(TemporaryCache&&) = default;

  // Cache operations
  void populate_with_valid_data();
  void populate_with_corrupted_data();
  void simulate_partial_corruption();
  void simulate_version_mismatch();
  void simulate_checksum_failure();

  // Access
  [[nodiscard]] std::filesystem::path cache_path() const;
  [[nodiscard]] std::string cache_path_string() const;
  [[nodiscard]] bool cache_exists() const;
  [[nodiscard]] size_t cache_size() const;

 private:
  std::unique_ptr<TemporaryDirectory> temp_dir_;
  std::string cache_file_;
  std::string cache_type_;
};

/**
 * @brief Main test data management class
 */
class TestDataManager {
 public:
  // Test data loading
  [[nodiscard]] static std::optional<TestDataSet> load_jpl_responses(const std::string& scenario);
  [[nodiscard]] static std::optional<TestDataSet> load_ephemeris_data(
      const std::string& time_period);
  [[nodiscard]] static std::optional<TestDataSet> load_cache_samples(const std::string& cache_type);
  [[nodiscard]] static std::optional<TestDataSet> load_validation_data(const std::string& category);

  // Specific data loaders
  [[nodiscard]] static std::string load_jpl_response_file(const std::string& body_name,
                                                          const std::string& epoch = "j2000");
  [[nodiscard]] static std::string load_ephemeris_json(const std::string& time_period);
  [[nodiscard]] static std::vector<uint8_t> load_binary_cache(const std::string& cache_name);

  // Test environment creation
  [[nodiscard]] static std::unique_ptr<TemporaryDirectory> create_test_environment();
  [[nodiscard]] static std::unique_ptr<TemporaryCache> create_test_cache(
      const std::string& type = "ephemeris");

  // Data validation
  [[nodiscard]] static bool validate_jpl_response(const std::string& response);
  [[nodiscard]] static bool validate_ephemeris_data(const std::string& data);
  [[nodiscard]] static bool validate_cache_integrity(const std::filesystem::path& cache_path);
  [[nodiscard]] static bool validate_json_format(const std::string& json_data);
  [[nodiscard]] static bool validate_binary_format(const std::vector<uint8_t>& binary_data);

  // Checksum and integrity
  [[nodiscard]] static std::string calculate_checksum(const std::string& data);
  [[nodiscard]] static std::string calculate_file_checksum(const std::filesystem::path& file_path);
  [[nodiscard]] static bool verify_checksum(const std::string& data,
                                            const std::string& expected_checksum);

  // Data paths
  [[nodiscard]] static std::filesystem::path get_test_data_root();
  [[nodiscard]] static std::filesystem::path get_jpl_responses_path();
  [[nodiscard]] static std::filesystem::path get_ephemeris_data_path();
  [[nodiscard]] static std::filesystem::path get_cache_samples_path();
  [[nodiscard]] static std::filesystem::path get_validation_data_path();

  // Cleanup operations
  static void cleanup_test_data();
  static void cleanup_temporary_files();
  static void register_cleanup_handler();

 private:
  static std::vector<std::unique_ptr<TemporaryDirectory>> temp_directories_;
  static std::vector<std::unique_ptr<TemporaryCache>> temp_caches_;
  static bool cleanup_registered_;

  // Internal helpers
  [[nodiscard]] static std::optional<std::string> read_file_content(
      const std::filesystem::path& file_path);
  [[nodiscard]] static std::optional<std::vector<uint8_t>> read_binary_file(
      const std::filesystem::path& file_path);
  [[nodiscard]] static bool write_file_content(const std::filesystem::path& file_path,
                                               const std::string& content);
  [[nodiscard]] static TestDataSet parse_metadata(const std::string& json_content);
};

/**
 * @brief Data validation utilities
 */
class DataValidator {
 public:
  // Physical validation
  [[nodiscard]] static bool validate_mass_positive(double mass);
  [[nodiscard]] static bool validate_position_bounds(const SolarSystem::Math::Vector3d& position,
                                                     double max_distance_au = 100.0);
  [[nodiscard]] static bool validate_velocity_bounds(const SolarSystem::Math::Vector3d& velocity,
                                                     double max_velocity_kms = 1000.0);

  // Orbital mechanics validation
  [[nodiscard]] static bool validate_orbital_energy(double kinetic_energy, double potential_energy,
                                                    double expected_total_energy,
                                                    double tolerance = 1e-9);
  [[nodiscard]] static bool validate_angular_momentum(
      const SolarSystem::Math::Vector3d& position, const SolarSystem::Math::Vector3d& velocity,
      const SolarSystem::Math::Vector3d& expected_momentum, double tolerance = 1e-9);

  // Data format validation
  [[nodiscard]] static bool validate_json_structure(
      const std::string& json_data, const std::vector<std::string>& required_fields);
  [[nodiscard]] static bool validate_binary_header(const std::vector<uint8_t>& binary_data,
                                                   const std::string& expected_magic);
  [[nodiscard]] static bool validate_csv_format(const std::string& csv_data,
                                                size_t expected_columns);

  // Statistical validation
  [[nodiscard]] static double calculate_relative_error(double computed, double reference);
  [[nodiscard]] static double calculate_position_error(
      const SolarSystem::Math::Vector3d& computed, const SolarSystem::Math::Vector3d& reference);
  [[nodiscard]] static double calculate_velocity_error(
      const SolarSystem::Math::Vector3d& computed, const SolarSystem::Math::Vector3d& reference);

  // Tolerance checking
  [[nodiscard]] static bool within_tolerance(double value, double reference, double tolerance);
  [[nodiscard]] static bool within_relative_tolerance(double value, double reference,
                                                      double relative_tolerance);
};

/**
 * @brief JPL data specific validation
 */
class JPLDataValidator {
 public:
  // JPL response validation
  [[nodiscard]] static bool validate_jpl_response_format(const std::string& response);
  [[nodiscard]] static bool validate_jpl_header(const std::string& response);
  [[nodiscard]] static bool validate_jpl_ephemeris_section(const std::string& response);
  [[nodiscard]] static bool validate_jpl_physical_data(const std::string& response);

  // Data extraction and validation
  [[nodiscard]] static std::optional<std::string> extract_body_name(const std::string& response);
  [[nodiscard]] static std::optional<int> extract_body_id(const std::string& response);
  [[nodiscard]] static std::optional<std::vector<double>> extract_ephemeris_coordinates(
      const std::string& response);

  // Mock data generation
  [[nodiscard]] static std::string generate_mock_jpl_response(const std::string& body_name,
                                                              int body_id, const std::string& date);
  [[nodiscard]] static std::string generate_error_response(int http_code,
                                                           const std::string& error_message);
  [[nodiscard]] static std::string generate_malformed_response(const std::string& corruption_type);

  // Comparison with reference data
  [[nodiscard]] static double compare_with_reference(const std::string& computed_response,
                                                     const std::string& reference_file);
  [[nodiscard]] static bool validate_against_physical_constants(const std::string& response);
};

/**
 * @brief Performance test data utilities
 */
class PerformanceTestData {
 public:
  // Benchmark data generation
  [[nodiscard]] static std::vector<TestDataSet> generate_scalability_test_data(
      const std::vector<size_t>& body_counts);
  [[nodiscard]] static TestDataSet generate_large_system_data(size_t body_count);
  [[nodiscard]] static TestDataSet generate_long_duration_data(double duration_days,
                                                               double time_step_seconds);

  // Performance reference data
  [[nodiscard]] static std::map<std::string, double> load_performance_baselines();
  [[nodiscard]] static bool validate_performance_regression(const std::string& test_name,
                                                            double measured_time,
                                                            double baseline_time,
                                                            double tolerance = 0.1);

  // Memory usage data
  [[nodiscard]] static size_t estimate_memory_usage(size_t body_count, double duration_days,
                                                    double time_step_seconds);
  [[nodiscard]] static bool validate_memory_bounds(size_t measured_memory, size_t expected_memory,
                                                   double tolerance = 0.2);
};

}  // namespace TestData
