#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <variant>

#include "test_data_manager.hpp"

namespace solar_test {

/**
 * @brief Comprehensive validation result with detailed error reporting
 */
struct ValidationResult {
  bool is_valid = false;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::string validation_type;
  std::chrono::system_clock::time_point validated_at;

  void add_error(const std::string& error) {
    errors.push_back(error);
    is_valid = false;
  }

  void add_warning(const std::string& warning) { warnings.push_back(warning); }

  [[nodiscard]] bool has_errors() const { return !errors.empty(); }
  [[nodiscard]] bool has_warnings() const { return !warnings.empty(); }
  [[nodiscard]] std::string summary() const {
    std::ostringstream oss;
    oss << validation_type << " validation: " << (is_valid ? "PASSED" : "FAILED") << " ("
        << errors.size() << " errors, " << warnings.size() << " warnings)";
    return oss.str();
  }
};

/**
 * @brief Test data versioning and migration support
 */
struct DataVersion {
  int major = 1;
  int minor = 0;
  int patch = 0;
  std::string description;
  std::chrono::system_clock::time_point created_at;

  [[nodiscard]] std::string to_string() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
  }

  [[nodiscard]] bool is_compatible_with(const DataVersion& other) const {
    return major == other.major && minor >= other.minor;
  }
};

/**
 * @brief Test data mutation strategies for robustness testing
 */
enum class MutationStrategy {
  CORRUPT_HEADER,
  CORRUPT_BODY,
  TRUNCATE_DATA,
  ADD_INVALID_FIELDS,
  REMOVE_REQUIRED_FIELDS,
  MODIFY_CHECKSUMS,
  INJECT_MALFORMED_JSON,
  SIMULATE_NETWORK_ERRORS,
  INTRODUCE_ENCODING_ISSUES,
  CREATE_PARTIAL_WRITES
};

/**
 * @brief Enhanced test data set with versioning and validation
 */
struct EnhancedTestDataSet : public TestDataSet {
  DataVersion version;
  ValidationResult validation_result;
  std::unordered_map<std::string, std::string> checksums;
  std::vector<MutationStrategy> applied_mutations;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_validated_at;

  [[nodiscard]] bool is_current_version() const;
  [[nodiscard]] bool needs_migration() const;
  void update_checksum(const std::string& file, const std::string& content);
  [[nodiscard]] bool verify_integrity() const;
};

/**
 * @brief Test environment isolation with comprehensive cleanup
 */
class IsolatedTestEnvironment {
 public:
  explicit IsolatedTestEnvironment(const std::string& test_name);
  ~IsolatedTestEnvironment();

  // Non-copyable but movable
  IsolatedTestEnvironment(const IsolatedTestEnvironment&) = delete;
  IsolatedTestEnvironment& operator=(const IsolatedTestEnvironment&) = delete;
  IsolatedTestEnvironment(IsolatedTestEnvironment&&) noexcept;
  IsolatedTestEnvironment& operator=(IsolatedTestEnvironment&&) noexcept;

  // Environment management
  void setup_clean_environment();
  void restore_original_environment();
  void cleanup_all_resources();

  // Resource tracking
  void register_temp_file(const std::string& path);
  void register_temp_directory(const std::string& path);
  void register_process(int pid);
  void register_network_port(int port);

  // Environment variables
  void set_env_var(const std::string& name, const std::string& value);
  void unset_env_var(const std::string& name);
  [[nodiscard]] std::optional<std::string> get_env_var(const std::string& name) const;

  // Working directory management
  void change_working_directory(const std::string& path);
  [[nodiscard]] std::string get_working_directory() const;

  // Resource limits
  void set_memory_limit(size_t bytes);
  void set_time_limit(std::chrono::seconds timeout);
  void set_file_descriptor_limit(int max_fds);

  [[nodiscard]] std::string get_test_name() const { return test_name_; }
  [[nodiscard]] bool is_clean() const { return is_clean_; }

 private:
  std::string test_name_;
  bool is_clean_ = false;
  std::string original_working_dir_;
  std::unordered_map<std::string, std::string> original_env_vars_;
  std::unordered_map<std::string, std::string> modified_env_vars_;
  std::vector<std::string> temp_files_;
  std::vector<std::string> temp_directories_;
  std::vector<int> spawned_processes_;
  std::vector<int> allocated_ports_;

  void cleanup_temp_files();
  void cleanup_temp_directories();
  void cleanup_processes();
  void cleanup_network_resources();
  void restore_environment_variables();
  void restore_working_directory();
};

/**
 * @brief Test data generator with mutation capabilities
 */
class TestDataGenerator {
 public:
  // JPL response generation
  [[nodiscard]] static std::string generate_valid_jpl_response(const std::string& body_name);
  [[nodiscard]] static std::string generate_error_jpl_response(const std::string& error_type);
  [[nodiscard]] static std::string generate_timeout_jpl_response();
  [[nodiscard]] static std::string generate_malformed_jpl_response();

  // Ephemeris data generation
  [[nodiscard]] static std::string generate_ephemeris_json(const std::string& time_range);
  [[nodiscard]] static std::string generate_ephemeris_binary(const std::string& time_range);
  [[nodiscard]] static std::string generate_corrupted_ephemeris(MutationStrategy strategy);

  // Cache file generation
  [[nodiscard]] static std::string generate_valid_cache_file(const std::string& format);
  [[nodiscard]] static std::string generate_corrupted_cache_file(MutationStrategy strategy);
  [[nodiscard]] static std::string generate_partial_cache_file();

  // Data mutation
  [[nodiscard]] static std::string mutate_data(const std::string& original_data,
                                               MutationStrategy strategy);
  [[nodiscard]] static std::vector<std::string> generate_mutation_variants(
      const std::string& original_data, const std::vector<MutationStrategy>& strategies);

  // Realistic data generation
  [[nodiscard]] static EnhancedTestDataSet generate_realistic_solar_system_data();
  [[nodiscard]] static EnhancedTestDataSet generate_historical_data_set(const std::string& epoch);
  [[nodiscard]] static EnhancedTestDataSet generate_stress_test_data_set(size_t data_size);

 private:
  static std::mt19937 random_generator_;
  static std::uniform_real_distribution<double> real_dist_;
  static std::uniform_int_distribution<int> int_dist_;

  [[nodiscard]] static std::string generate_random_string(size_t length);
  [[nodiscard]] static double generate_realistic_orbital_element();
  [[nodiscard]] static std::string generate_body_data_json(const std::string& body_name);
};

/**
 * @brief Comprehensive test data validator
 */
class TestDataValidator {
 public:
  // JPL response validation
  [[nodiscard]] static ValidationResult validate_jpl_response_comprehensive(
      const std::string& response);
  [[nodiscard]] static ValidationResult validate_jpl_response_format(const std::string& response);
  [[nodiscard]] static ValidationResult validate_jpl_response_content(const std::string& response);

  // Ephemeris data validation
  [[nodiscard]] static ValidationResult validate_ephemeris_data_comprehensive(
      const std::string& data);
  [[nodiscard]] static ValidationResult validate_ephemeris_json_format(
      const std::string& json_data);
  [[nodiscard]] static ValidationResult validate_ephemeris_binary_format(
      const std::string& binary_data);

  // Cache file validation
  [[nodiscard]] static ValidationResult validate_cache_file_comprehensive(
      const std::string& cache_path);
  [[nodiscard]] static ValidationResult validate_cache_integrity_comprehensive(
      const std::string& cache_path);
  [[nodiscard]] static ValidationResult validate_cache_format(const std::string& cache_content);

  // Data consistency validation
  [[nodiscard]] static ValidationResult validate_data_consistency(
      const EnhancedTestDataSet& dataset);
  [[nodiscard]] static ValidationResult validate_version_compatibility(
      const DataVersion& required, const DataVersion& available);

  // Recovery mechanisms
  [[nodiscard]] static std::optional<std::string> attempt_data_recovery(
      const std::string& corrupted_data, const std::string& data_type);
  [[nodiscard]] static std::vector<std::string> suggest_recovery_actions(
      const ValidationResult& validation_result);

 private:
  static const std::regex jpl_response_pattern_;
  static const std::regex ephemeris_header_pattern_;
  static const std::unordered_map<std::string, std::function<bool(const std::string&)>> validators_;

  [[nodiscard]] static bool validate_json_structure(const std::string& json_data);
  [[nodiscard]] static bool validate_binary_header(const std::string& binary_data);
  [[nodiscard]] static bool validate_checksum(const std::string& data,
                                              const std::string& expected_checksum);
};

/**
 * @brief Enhanced test data manager with comprehensive capabilities
 */
class EnhancedTestDataManager : public TestDataManager {
 public:
  // Enhanced data loading with validation
  [[nodiscard]] static EnhancedTestDataSet load_validated_jpl_responses(
      const std::string& scenario);
  [[nodiscard]] static EnhancedTestDataSet load_validated_ephemeris_data(
      const std::string& time_period);
  [[nodiscard]] static EnhancedTestDataSet load_validated_cache_samples(
      const std::string& cache_type);

  // Data generation and mutation
  [[nodiscard]] static EnhancedTestDataSet generate_test_data_set(
      const std::string& type, const std::vector<MutationStrategy>& mutations = {});
  [[nodiscard]] static std::vector<EnhancedTestDataSet> generate_mutation_test_suite(
      const std::string& base_type);

  // Environment management
  [[nodiscard]] static std::unique_ptr<IsolatedTestEnvironment> create_isolated_environment(
      const std::string& test_name);
  static void cleanup_all_test_environments();

  // Data versioning and migration
  [[nodiscard]] static DataVersion get_current_data_version();
  [[nodiscard]] static bool migrate_data_set(EnhancedTestDataSet& dataset,
                                             const DataVersion& target_version);
  [[nodiscard]] static std::vector<DataVersion> get_available_versions();

  // Comprehensive validation
  [[nodiscard]] static ValidationResult validate_test_environment();
  [[nodiscard]] static ValidationResult validate_all_test_data();
  [[nodiscard]] static std::vector<ValidationResult> run_comprehensive_validation_suite();

  // Error recovery
  [[nodiscard]] static bool attempt_automatic_recovery(const std::string& data_path);
  [[nodiscard]] static std::vector<std::string> generate_recovery_report(
      const std::vector<ValidationResult>& validation_results);

  // Performance and monitoring
  static void enable_performance_monitoring(bool enable = true);
  [[nodiscard]] static std::unordered_map<std::string, double> get_performance_metrics();
  static void reset_performance_metrics();

 private:
  static std::vector<std::unique_ptr<IsolatedTestEnvironment>> active_environments_;
  static bool performance_monitoring_enabled_;
  static std::unordered_map<std::string, double> performance_metrics_;
  static DataVersion current_version_;

  static void register_environment(std::unique_ptr<IsolatedTestEnvironment> env);
  static void unregister_environment(const std::string& test_name);
  static void update_performance_metric(const std::string& metric_name, double value);
};

// Utility functions for test data management
namespace test_data_utils {

/**
 * @brief RAII helper for automatic test environment cleanup
 */
class AutoCleanupGuard {
 public:
  explicit AutoCleanupGuard(std::function<void()> cleanup_func);
  ~AutoCleanupGuard();

  void release();

 private:
  std::function<void()> cleanup_func_;
  bool released_ = false;
};

/**
 * @brief Test data integrity checker
 */
class IntegrityChecker {
 public:
  [[nodiscard]] static std::string calculate_checksum(const std::string& data);
  [[nodiscard]] static bool verify_checksum(const std::string& data, const std::string& expected);
  [[nodiscard]] static std::unordered_map<std::string, std::string> calculate_dataset_checksums(
      const EnhancedTestDataSet& dataset);
  [[nodiscard]] static bool verify_dataset_integrity(const EnhancedTestDataSet& dataset);
};

/**
 * @brief Test data migration utilities
 */
class MigrationHelper {
 public:
  [[nodiscard]] static bool is_migration_needed(const DataVersion& from, const DataVersion& to);
  [[nodiscard]] static std::vector<std::string> get_migration_steps(const DataVersion& from,
                                                                    const DataVersion& to);
  [[nodiscard]] static bool execute_migration(EnhancedTestDataSet& dataset,
                                              const DataVersion& target_version);

 private:
  static const std::unordered_map<std::string, std::function<bool(EnhancedTestDataSet&)>>
      migration_functions_;
};

}  // namespace test_data_utils

}  // namespace solar_test
