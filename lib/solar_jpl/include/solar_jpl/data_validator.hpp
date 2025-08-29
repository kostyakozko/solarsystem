/**
 * @file data_validator.hpp
 * @brief Comprehensive Data Validation System for JPL HORIZONS Data
 *
 * Provides comprehensive data validation with:
 * - Data integrity checking and validation
 * - Data quality assessment and reporting
 * - Data format validation and conversion
 * - Data consistency checking across sources
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "jpl_client.hpp"

// Forward declaration to avoid circular dependency
namespace SolarSystem::JPL {
enum class ValidationLevel;
}

namespace SolarSystem::JPL {

/**
 * @brief Data validation error types
 */
enum class ValidationErrorType {
  // Data integrity errors
  InvalidJPLId,
  InvalidPosition,
  InvalidVelocity,
  InvalidMass,
  InvalidBodyName,
  InvalidEpoch,

  // Data quality errors
  OutOfRangeValues,
  SuspiciousValues,
  InconsistentData,
  MissingData,
  DuplicateData,

  // Format validation errors
  InvalidBinaryFormat,
  InvalidJSONFormat,
  CorruptedData,
  IncompatibleVersion,

  // Consistency errors
  CrossFormatMismatch,
  MetadataMismatch,
  ChecksumMismatch,
  TimestampInconsistency
};

/**
 * @brief Validation severity levels
 */
enum class ValidationSeverity {
  Info,      // Informational, no action needed
  Warning,   // Potential issue, should be reviewed
  Error,     // Definite problem, needs correction
  Critical   // Critical issue, data unusable
};

/**
 * @brief Data quality metrics
 */
struct DataQualityMetrics {
  // Completeness metrics
  size_t total_bodies_expected = 0;
  size_t total_bodies_found = 0;
  double completeness_ratio = 0.0;

  // Accuracy metrics
  size_t bodies_with_valid_positions = 0;
  size_t bodies_with_valid_velocities = 0;
  size_t bodies_with_valid_masses = 0;
  double accuracy_score = 0.0;

  // Consistency metrics
  size_t consistent_cross_format_bodies = 0;
  size_t total_cross_format_comparisons = 0;
  double consistency_score = 0.0;

  // Freshness metrics
  std::chrono::system_clock::time_point data_epoch;
  std::chrono::hours data_age{0};
  bool is_current = false;

  // Overall quality score (0.0 to 1.0)
  double overall_quality_score = 0.0;

  /**
   * @brief Calculate overall quality score
   */
  void calculate_overall_score();

  /**
   * @brief Check if data meets quality thresholds
   */
  [[nodiscard]] bool meets_quality_threshold(double threshold = 0.8) const;
};

/**
 * @brief Validation issue details
 */
struct ValidationIssue {
  ValidationErrorType error_type;
  ValidationSeverity severity;
  std::string description;
  std::string affected_body;
  int affected_jpl_id = -1;
  std::string suggested_action;
  std::chrono::system_clock::time_point detected_at;

  // Additional context
  std::unordered_map<std::string, std::string> context_data;
};

/**
 * @brief Comprehensive validation report
 */
struct ValidationReport {
  std::chrono::system_clock::time_point validation_timestamp;
  ValidationLevel validation_level;

  // Summary statistics
  size_t total_issues = 0;
  size_t critical_issues = 0;
  size_t error_issues = 0;
  size_t warning_issues = 0;
  size_t info_issues = 0;

  // Detailed issues
  std::vector<ValidationIssue> issues;

  // Quality metrics
  DataQualityMetrics quality_metrics;

  // Validation performance
  std::chrono::milliseconds validation_duration{0};
  size_t bodies_validated = 0;
  size_t files_validated = 0;

  // Overall validation result
  bool validation_passed = false;
  double confidence_score = 0.0;

  /**
   * @brief Check if validation passed with no critical issues
   */
  [[nodiscard]] bool is_valid() const;

  /**
   * @brief Get issues by severity
   */
  [[nodiscard]] std::vector<ValidationIssue> get_issues_by_severity(ValidationSeverity severity) const;

  /**
   * @brief Generate human-readable summary
   */
  [[nodiscard]] std::string generate_summary() const;

  /**
   * @brief Export report to JSON
   */
  [[nodiscard]] std::string to_json() const;
};

/**
 * @brief Data validation configuration
 */
struct DataValidatorConfig {
  // Validation thresholds
  double position_min_km = 1e3;           // 1,000 km minimum distance
  double position_max_km = 1e12;          // 1 trillion km maximum distance
  double velocity_max_km_s = 1e6;         // 1 million km/s maximum velocity
  double mass_min_kg = 1e10;              // 10 billion kg minimum mass
  double mass_max_kg = 1e35;              // 10^35 kg maximum mass

  // Quality thresholds
  double completeness_threshold = 0.95;    // 95% completeness required
  double accuracy_threshold = 0.90;       // 90% accuracy required
  double consistency_threshold = 0.95;     // 95% consistency required
  double overall_quality_threshold = 0.85; // 85% overall quality required

  // Validation behavior
  bool enable_cross_format_validation = true;
  bool enable_metadata_validation = true;
  bool enable_checksum_validation = true;
  bool enable_statistical_validation = true;
  bool enable_temporal_validation = true;

  // Performance settings
  size_t max_concurrent_validations = 4;
  std::chrono::seconds validation_timeout = std::chrono::seconds(300); // 5 minutes
  bool enable_detailed_logging = true;

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] bool is_valid(std::string* error = nullptr) const;
};

/**
 * @brief Comprehensive Data Validator
 */
class DataValidator {
public:
  /**
   * @brief Construct data validator with configuration
   */
  explicit DataValidator(DataValidatorConfig config = {});

  /**
   * @brief Destructor
   */
  ~DataValidator() = default;

  // Non-copyable and non-movable
  DataValidator(const DataValidator&) = delete;
  DataValidator& operator=(const DataValidator&) = delete;
  DataValidator(DataValidator&&) = delete;
  DataValidator& operator=(DataValidator&&) = delete;

  // Data Integrity Validation

  /**
   * @brief Validate individual ephemeris data entry
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_ephemeris_data(
    const EphemerisData& data
  );

  /**
   * @brief Validate collection of ephemeris data
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_ephemeris_collection(
    const std::vector<EphemerisData>& data_collection
  );

  /**
   * @brief Validate cache data integrity
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_cache_integrity(
    const std::filesystem::path& cache_directory
  );

  // Data Quality Assessment

  /**
   * @brief Assess data quality and generate metrics
   */
  [[nodiscard]] JPLResult<DataQualityMetrics> assess_data_quality(
    const std::vector<EphemerisData>& data_collection
  );

  /**
   * @brief Generate comprehensive quality report
   */
  [[nodiscard]] JPLResult<ValidationReport> generate_quality_report(
    const std::vector<EphemerisData>& data_collection
  );

  /**
   * @brief Validate data against expected statistical distributions
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_statistical_properties(
    const std::vector<EphemerisData>& data_collection
  );

  // Format Validation

  /**
   * @brief Validate binary cache format
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_binary_format(
    const std::filesystem::path& binary_file
  );

  /**
   * @brief Validate JSON cache format
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_json_format(
    const std::filesystem::path& json_file
  );

  /**
   * @brief Validate and convert between formats
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_format_conversion(
    const std::filesystem::path& source_file,
    const std::filesystem::path& target_file
  );

  // Consistency Validation

  /**
   * @brief Validate consistency between binary and JSON formats
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_cross_format_consistency(
    const std::filesystem::path& binary_file,
    const std::filesystem::path& json_file
  );

  /**
   * @brief Validate metadata consistency
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_metadata_consistency(
    const CacheMetadata& metadata,
    const std::vector<EphemerisData>& actual_data
  );

  /**
   * @brief Validate temporal consistency
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_temporal_consistency(
    const std::vector<EphemerisData>& data_collection
  );

  // Configuration and Utilities

  /**
   * @brief Get current configuration
   */
  [[nodiscard]] const DataValidatorConfig& config() const { return config_; }

  /**
   * @brief Update configuration
   */
  [[nodiscard]] JPLVoidResult update_config(const DataValidatorConfig& new_config);

  /**
   * @brief Get validation statistics
   */
  [[nodiscard]] std::unordered_map<std::string, size_t> get_validation_statistics() const;

  /**
   * @brief Reset validation statistics
   */
  void reset_statistics();

private:
  DataValidatorConfig config_;

  // Validation statistics
  mutable std::unordered_map<std::string, size_t> validation_stats_;

  // Internal validation methods
  [[nodiscard]] ValidationIssue create_validation_issue(
    ValidationErrorType error_type,
    ValidationSeverity severity,
    const std::string& description,
    const std::string& affected_body = "",
    int affected_jpl_id = -1,
    const std::string& suggested_action = ""
  ) const;

  [[nodiscard]] JPLResult<bool> validate_position_vector(
    const SolarSystem::Math::Vector3d& position,
    const std::string& body_name,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_velocity_vector(
    const SolarSystem::Math::Vector3d& velocity,
    const std::string& body_name,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_mass_value(
    long double mass,
    const std::string& body_name,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_jpl_id(
    int jpl_id,
    const std::string& body_name,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_body_name(
    const std::string& body_name,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_epoch(
    const std::chrono::system_clock::time_point& epoch,
    const std::string& body_name,
    std::vector<ValidationIssue>& issues
  ) const;

  // Statistical validation helpers
  [[nodiscard]] JPLResult<bool> validate_position_distribution(
    const std::vector<EphemerisData>& data_collection,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_velocity_distribution(
    const std::vector<EphemerisData>& data_collection,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_mass_distribution(
    const std::vector<EphemerisData>& data_collection,
    std::vector<ValidationIssue>& issues
  ) const;

  // Format validation helpers
  [[nodiscard]] JPLResult<bool> validate_binary_header(
    std::ifstream& file,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<bool> validate_json_structure(
    const std::string& json_content,
    std::vector<ValidationIssue>& issues
  ) const;

  // Consistency validation helpers
  [[nodiscard]] JPLResult<bool> compare_ephemeris_data(
    const EphemerisData& data1,
    const EphemerisData& data2,
    double tolerance,
    std::vector<ValidationIssue>& issues
  ) const;

  [[nodiscard]] JPLResult<uint64_t> calculate_data_checksum(
    const std::vector<EphemerisData>& data_collection
  ) const;

  // Utility methods
  void update_statistics(const std::string& operation) const;
  [[nodiscard]] std::string format_validation_issue(const ValidationIssue& issue) const;
};

/**
 * @brief Data Validator Factory
 */
class DataValidatorFactory {
public:
  /**
   * @brief Create default data validator
   */
  [[nodiscard]] static std::unique_ptr<DataValidator> create_default();

  /**
   * @brief Create data validator with custom configuration
   */
  [[nodiscard]] static std::unique_ptr<DataValidator> create(DataValidatorConfig config);

  /**
   * @brief Create data validator for testing
   */
  [[nodiscard]] static std::unique_ptr<DataValidator> create_for_testing();
};

/**
 * @brief Utility functions for data validation
 */
namespace ValidationUtils {

/**
 * @brief Convert validation error type to string
 */
[[nodiscard]] std::string to_string(ValidationErrorType error_type);

/**
 * @brief Convert validation severity to string
 */
[[nodiscard]] std::string to_string(ValidationSeverity severity);

/**
 * @brief Check if value is within reasonable astronomical range
 */
[[nodiscard]] bool is_reasonable_astronomical_value(
  double value,
  const std::string& value_type
);

/**
 * @brief Calculate statistical outliers in data collection
 */
[[nodiscard]] std::vector<size_t> find_statistical_outliers(
  const std::vector<double>& values,
  double threshold = 3.0  // Standard deviations
);

/**
 * @brief Generate validation report summary
 */
[[nodiscard]] std::string generate_validation_summary(
  const ValidationReport& report
);

}  // namespace ValidationUtils

}  // namespace SolarSystem::JPL
