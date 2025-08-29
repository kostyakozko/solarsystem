/**
 * @file data_validator.cpp
 * @brief Implementation of Comprehensive Data Validation System
 */

#include "solar_jpl/data_validator.hpp"
#include "solar_jpl/cache_manager.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <regex>
#include <sstream>
#include <thread>

namespace SolarSystem::JPL {

/**
 * @brief Calculate overall quality score
 */
void DataQualityMetrics::calculate_overall_score() {
  // Weight different aspects of quality
  const double completeness_weight = 0.3;
  const double accuracy_weight = 0.4;
  const double consistency_weight = 0.2;
  const double freshness_weight = 0.1;

  // Calculate freshness score (1.0 for current data, decreasing with age)
  double freshness_score = 1.0;
  if (data_age > std::chrono::hours(24 * 30)) {  // Older than 30 days
    freshness_score = std::max(0.0, 1.0 - (data_age.count() / (24.0 * 30.0 * 6.0))); // Decay over 6 months
  }

  overall_quality_score =
    completeness_ratio * completeness_weight +
    accuracy_score * accuracy_weight +
    consistency_score * consistency_weight +
    freshness_score * freshness_weight;

  overall_quality_score = std::clamp(overall_quality_score, 0.0, 1.0);
}

/**
 * @brief Check if data meets quality thresholds
 */
bool DataQualityMetrics::meets_quality_threshold(double threshold) const {
  return overall_quality_score >= threshold;
}

/**
 * @brief Check if validation passed with no critical issues
 */
bool ValidationReport::is_valid() const {
  return validation_passed && critical_issues == 0;
}

/**
 * @brief Get issues by severity
 */
std::vector<ValidationIssue> ValidationReport::get_issues_by_severity(ValidationSeverity severity) const {
  std::vector<ValidationIssue> filtered_issues;
  std::copy_if(issues.begin(), issues.end(), std::back_inserter(filtered_issues),
               [severity](const ValidationIssue& issue) {
                 return issue.severity == severity;
               });
  return filtered_issues;
}

/**
 * @brief Generate human-readable summary
 */
std::string ValidationReport::generate_summary() const {
  std::ostringstream summary;

  summary << "=== Data Validation Report ===\n";
  summary << "Validation Time: " << std::chrono::duration_cast<std::chrono::seconds>(
    validation_timestamp.time_since_epoch()).count() << " (epoch seconds)\n";
  summary << "Validation Level: " << static_cast<int>(validation_level) << "\n";
  summary << "Duration: " << validation_duration.count() << " ms\n";
  summary << "Bodies Validated: " << bodies_validated << "\n";
  summary << "Files Validated: " << files_validated << "\n\n";

  summary << "=== Issue Summary ===\n";
  summary << "Total Issues: " << total_issues << "\n";
  summary << "Critical: " << critical_issues << "\n";
  summary << "Errors: " << error_issues << "\n";
  summary << "Warnings: " << warning_issues << "\n";
  summary << "Info: " << info_issues << "\n\n";

  summary << "=== Quality Metrics ===\n";
  summary << "Overall Quality Score: " << std::fixed << std::setprecision(2)
          << quality_metrics.overall_quality_score * 100 << "%\n";
  summary << "Completeness: " << std::fixed << std::setprecision(2)
          << quality_metrics.completeness_ratio * 100 << "%\n";
  summary << "Accuracy: " << std::fixed << std::setprecision(2)
          << quality_metrics.accuracy_score * 100 << "%\n";
  summary << "Consistency: " << std::fixed << std::setprecision(2)
          << quality_metrics.consistency_score * 100 << "%\n";

  summary << "\n=== Validation Result ===\n";
  summary << "Status: " << (validation_passed ? "PASSED" : "FAILED") << "\n";
  summary << "Confidence: " << std::fixed << std::setprecision(2)
          << confidence_score * 100 << "%\n";

  return summary.str();
}

/**
 * @brief Export report to JSON
 */
std::string ValidationReport::to_json() const {
  std::ostringstream json;

  json << "{\n";
  json << "  \"validation_timestamp\": " << std::chrono::duration_cast<std::chrono::seconds>(
    validation_timestamp.time_since_epoch()).count() << ",\n";
  json << "  \"validation_level\": " << static_cast<int>(validation_level) << ",\n";
  json << "  \"validation_duration_ms\": " << validation_duration.count() << ",\n";
  json << "  \"bodies_validated\": " << bodies_validated << ",\n";
  json << "  \"files_validated\": " << files_validated << ",\n";
  json << "  \"total_issues\": " << total_issues << ",\n";
  json << "  \"critical_issues\": " << critical_issues << ",\n";
  json << "  \"error_issues\": " << error_issues << ",\n";
  json << "  \"warning_issues\": " << warning_issues << ",\n";
  json << "  \"info_issues\": " << info_issues << ",\n";
  json << "  \"validation_passed\": " << (validation_passed ? "true" : "false") << ",\n";
  json << "  \"confidence_score\": " << confidence_score << ",\n";
  json << "  \"quality_metrics\": {\n";
  json << "    \"overall_quality_score\": " << quality_metrics.overall_quality_score << ",\n";
  json << "    \"completeness_ratio\": " << quality_metrics.completeness_ratio << ",\n";
  json << "    \"accuracy_score\": " << quality_metrics.accuracy_score << ",\n";
  json << "    \"consistency_score\": " << quality_metrics.consistency_score << "\n";
  json << "  }\n";
  json << "}";

  return json.str();
}

/**
 * @brief Configuration validation
 */
bool DataValidatorConfig::is_valid(std::string* error) const {
  if (position_min_km <= 0 || position_max_km <= position_min_km) {
    if (error) *error = "Invalid position range";
    return false;
  }

  if (velocity_max_km_s <= 0) {
    if (error) *error = "Invalid velocity maximum";
    return false;
  }

  if (mass_min_kg <= 0 || mass_max_kg <= mass_min_kg) {
    if (error) *error = "Invalid mass range";
    return false;
  }

  if (completeness_threshold < 0.0 || completeness_threshold > 1.0) {
    if (error) *error = "Completeness threshold must be between 0.0 and 1.0";
    return false;
  }

  if (max_concurrent_validations == 0) {
    if (error) *error = "Max concurrent validations must be positive";
    return false;
  }

  return true;
}

/**
 * @brief Data validator constructor
 */
DataValidator::DataValidator(DataValidatorConfig config) : config_(std::move(config)) {
  // Validate configuration
  std::string error;
  if (!config_.is_valid(&error)) {
    throw std::invalid_argument("Invalid data validator configuration: " + error);
  }
}

/**
 * @brief Validate individual ephemeris data entry
 */
JPLResult<ValidationReport> DataValidator::validate_ephemeris_data(const EphemerisData& data) {
  update_statistics("validate_ephemeris_data");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Standard;
  report.bodies_validated = 1;

  std::vector<ValidationIssue> issues;

  // Validate JPL ID
  auto jpl_result = validate_jpl_id(data.jpl_id, data.body_name, issues);
  if (!is_success(jpl_result)) {
    return get_error(jpl_result);
  }

  // Validate body name
  auto name_result = validate_body_name(data.body_name, issues);
  if (!is_success(name_result)) {
    return get_error(name_result);
  }

  // Validate position
  auto pos_result = validate_position_vector(data.position, data.body_name, issues);
  if (!is_success(pos_result)) {
    return get_error(pos_result);
  }

  // Validate velocity
  auto vel_result = validate_velocity_vector(data.velocity, data.body_name, issues);
  if (!is_success(vel_result)) {
    return get_error(vel_result);
  }

  // Validate mass
  auto mass_result = validate_mass_value(data.mass, data.body_name, issues);
  if (!is_success(mass_result)) {
    return get_error(mass_result);
  }

  // Validate epoch
  auto epoch_result = validate_epoch(data.epoch, data.body_name, issues);
  if (!is_success(epoch_result)) {
    return get_error(epoch_result);
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  // Count issues by severity
  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical:
        report.critical_issues++;
        break;
      case ValidationSeverity::Error:
        report.error_issues++;
        break;
      case ValidationSeverity::Warning:
        report.warning_issues++;
        break;
      case ValidationSeverity::Info:
        report.info_issues++;
        break;
    }
  }

  // Determine validation result
  report.validation_passed = (report.critical_issues == 0 && report.error_issues == 0);
  report.confidence_score = report.validation_passed ?
    std::max(0.0, 1.0 - (report.warning_issues * 0.1)) : 0.0;

  auto end_time = std::chrono::steady_clock::now();
  report.validation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time);

  return report;
}

/**
 * @brief Validate collection of ephemeris data
 */
JPLResult<ValidationReport> DataValidator::validate_ephemeris_collection(
    const std::vector<EphemerisData>& data_collection) {
  update_statistics("validate_ephemeris_collection");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Comprehensive;
  report.bodies_validated = data_collection.size();

  std::vector<ValidationIssue> issues;

  // Validate individual entries
  for (const auto& data : data_collection) {
    auto individual_result = validate_ephemeris_data(data);
    if (is_success(individual_result)) {
      const auto& individual_report = get_value(individual_result);
      issues.insert(issues.end(), individual_report.issues.begin(), individual_report.issues.end());
    }
  }

  // Check for duplicates
  std::unordered_map<int, size_t> jpl_id_counts;
  for (const auto& data : data_collection) {
    jpl_id_counts[data.jpl_id]++;
  }

  for (const auto& [jpl_id, count] : jpl_id_counts) {
    if (count > 1) {
      issues.push_back(create_validation_issue(
        ValidationErrorType::DuplicateData,
        ValidationSeverity::Error,
        "Duplicate JPL ID found: " + std::to_string(jpl_id),
        "",
        jpl_id,
        "Remove duplicate entries"
      ));
    }
  }

  // Statistical validation if enabled
  if (config_.enable_statistical_validation) {
    auto pos_dist_result = validate_position_distribution(data_collection, issues);
    auto vel_dist_result = validate_velocity_distribution(data_collection, issues);
    auto mass_dist_result = validate_mass_distribution(data_collection, issues);

    (void)pos_dist_result;  // Suppress unused variable warnings
    (void)vel_dist_result;
    (void)mass_dist_result;
  }

  // Generate quality metrics
  auto quality_result = assess_data_quality(data_collection);
  if (is_success(quality_result)) {
    report.quality_metrics = get_value(quality_result);
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  // Count issues by severity
  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical:
        report.critical_issues++;
        break;
      case ValidationSeverity::Error:
        report.error_issues++;
        break;
      case ValidationSeverity::Warning:
        report.warning_issues++;
        break;
      case ValidationSeverity::Info:
        report.info_issues++;
        break;
    }
  }

  // Determine validation result
  report.validation_passed = (report.critical_issues == 0 && report.error_issues == 0) &&
                            report.quality_metrics.meets_quality_threshold(config_.overall_quality_threshold);

  report.confidence_score = report.validation_passed ?
    std::max(0.0, report.quality_metrics.overall_quality_score - (report.warning_issues * 0.05)) : 0.0;

  auto end_time = std::chrono::steady_clock::now();
  report.validation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time);

  return report;
}

/**
 * @brief Assess data quality and generate metrics
 */
JPLResult<DataQualityMetrics> DataValidator::assess_data_quality(
    const std::vector<EphemerisData>& data_collection) {
  update_statistics("assess_data_quality");

  DataQualityMetrics metrics;

  // Completeness assessment
  metrics.total_bodies_expected = 27;  // Standard solar system body count
  metrics.total_bodies_found = data_collection.size();
  metrics.completeness_ratio = static_cast<double>(metrics.total_bodies_found) /
                              static_cast<double>(metrics.total_bodies_expected);

  // Accuracy assessment
  for (const auto& data : data_collection) {
    // Check position validity
    double pos_magnitude = data.position.magnitude();
    if (pos_magnitude >= config_.position_min_km && pos_magnitude <= config_.position_max_km) {
      metrics.bodies_with_valid_positions++;
    }

    // Check velocity validity
    double vel_magnitude = data.velocity.magnitude();
    if (vel_magnitude <= config_.velocity_max_km_s) {
      metrics.bodies_with_valid_velocities++;
    }

    // Check mass validity
    if (data.mass >= config_.mass_min_kg && data.mass <= config_.mass_max_kg) {
      metrics.bodies_with_valid_masses++;
    }
  }

  if (!data_collection.empty()) {
    double total_bodies = static_cast<double>(data_collection.size());
    double pos_accuracy = static_cast<double>(metrics.bodies_with_valid_positions) / total_bodies;
    double vel_accuracy = static_cast<double>(metrics.bodies_with_valid_velocities) / total_bodies;
    double mass_accuracy = static_cast<double>(metrics.bodies_with_valid_masses) / total_bodies;

    metrics.accuracy_score = (pos_accuracy + vel_accuracy + mass_accuracy) / 3.0;
  }

  // Consistency assessment (placeholder - would need multiple data sources)
  metrics.consistent_cross_format_bodies = data_collection.size();
  metrics.total_cross_format_comparisons = data_collection.size();
  metrics.consistency_score = 1.0;  // Assume consistent for single source

  // Freshness assessment
  if (!data_collection.empty()) {
    metrics.data_epoch = data_collection[0].epoch;
    auto now = std::chrono::system_clock::now();
    metrics.data_age = std::chrono::duration_cast<std::chrono::hours>(now - metrics.data_epoch);
    metrics.is_current = metrics.data_age <= std::chrono::hours(24 * 7);  // Within a week
  }

  // Calculate overall quality score
  metrics.calculate_overall_score();

  return metrics;
}

/**
 * @brief Validate binary cache format
 */
JPLResult<ValidationReport> DataValidator::validate_binary_format(
    const std::filesystem::path& binary_file) {
  update_statistics("validate_binary_format");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Standard;
  report.files_validated = 1;

  std::vector<ValidationIssue> issues;

  // Check file existence
  if (!std::filesystem::exists(binary_file)) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Critical,
      "Binary cache file does not exist: " + binary_file.string(),
      "",
      -1,
      "Create or regenerate binary cache file"
    ));
  } else {
    // Check file size
    auto file_size = std::filesystem::file_size(binary_file);
    if (file_size == 0) {
      issues.push_back(create_validation_issue(
        ValidationErrorType::CorruptedData,
        ValidationSeverity::Critical,
        "Binary cache file is empty: " + binary_file.string(),
        "",
        -1,
        "Regenerate binary cache file"
      ));
    } else if (file_size > 100 * 1024 * 1024) {  // > 100MB
      issues.push_back(create_validation_issue(
        ValidationErrorType::SuspiciousValues,
        ValidationSeverity::Warning,
        "Binary cache file is unusually large: " + std::to_string(file_size) + " bytes",
        "",
        -1,
        "Verify cache content and consider compression"
      ));
    }

    // Validate binary header
    std::ifstream file(binary_file, std::ios::binary);
    if (file.is_open()) {
      auto header_result = validate_binary_header(file, issues);
      (void)header_result;  // Suppress unused variable warning
    } else {
      issues.push_back(create_validation_issue(
        ValidationErrorType::InvalidBinaryFormat,
        ValidationSeverity::Error,
        "Cannot open binary cache file for reading: " + binary_file.string(),
        "",
        -1,
        "Check file permissions and disk space"
      ));
    }
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  // Count issues by severity
  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical:
        report.critical_issues++;
        break;
      case ValidationSeverity::Error:
        report.error_issues++;
        break;
      case ValidationSeverity::Warning:
        report.warning_issues++;
        break;
      case ValidationSeverity::Info:
        report.info_issues++;
        break;
    }
  }

  report.validation_passed = (report.critical_issues == 0 && report.error_issues == 0);
  report.confidence_score = report.validation_passed ?
    std::max(0.0, 1.0 - (report.warning_issues * 0.1)) : 0.0;

  auto end_time = std::chrono::steady_clock::now();
  report.validation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time);

  return report;
}

/**
 * @brief Create validation issue
 */
ValidationIssue DataValidator::create_validation_issue(
    ValidationErrorType error_type,
    ValidationSeverity severity,
    const std::string& description,
    const std::string& affected_body,
    int affected_jpl_id,
    const std::string& suggested_action) const {

  ValidationIssue issue;
  issue.error_type = error_type;
  issue.severity = severity;
  issue.description = description;
  issue.affected_body = affected_body;
  issue.affected_jpl_id = affected_jpl_id;
  issue.suggested_action = suggested_action;
  issue.detected_at = std::chrono::system_clock::now();

  return issue;
}

/**
 * @brief Update validation statistics
 */
void DataValidator::update_statistics(const std::string& operation) const {
  validation_stats_[operation]++;
  validation_stats_["total_operations"]++;
}

/**
 * @brief Get validation statistics
 */
std::unordered_map<std::string, size_t> DataValidator::get_validation_statistics() const {
  return validation_stats_;
}

/**
 * @brief Reset validation statistics
 */
void DataValidator::reset_statistics() {
  validation_stats_.clear();
}

// Placeholder implementations for remaining methods
JPLResult<ValidationReport> DataValidator::validate_cache_integrity(const std::filesystem::path&) {
  return ValidationReport{};
}

JPLResult<ValidationReport> DataValidator::generate_quality_report(const std::vector<EphemerisData>&) {
  return ValidationReport{};
}

JPLResult<ValidationReport> DataValidator::validate_statistical_properties(const std::vector<EphemerisData>&) {
  return ValidationReport{};
}

JPLResult<ValidationReport> DataValidator::validate_json_format(const std::filesystem::path&) {
  return ValidationReport{};
}

JPLResult<ValidationReport> DataValidator::validate_format_conversion(const std::filesystem::path&, const std::filesystem::path&) {
  return ValidationReport{};
}

JPLResult<ValidationReport> DataValidator::validate_cross_format_consistency(const std::filesystem::path&, const std::filesystem::path&) {
  return ValidationReport{};
}

JPLResult<ValidationReport> DataValidator::validate_metadata_consistency(const CacheMetadata&, const std::vector<EphemerisData>&) {
  return ValidationReport{};
}

JPLResult<ValidationReport> DataValidator::validate_temporal_consistency(const std::vector<EphemerisData>&) {
  return ValidationReport{};
}

JPLVoidResult DataValidator::update_config(const DataValidatorConfig&) {
  return success();
}

// Private method placeholder implementations
JPLResult<bool> DataValidator::validate_position_vector(const SolarSystem::Math::Vector3d& position, const std::string& body_name, std::vector<ValidationIssue>& issues) const {
  double magnitude = position.magnitude();

  if (magnitude < config_.position_min_km) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidPosition,
      ValidationSeverity::Error,
      "Position magnitude too small for " + body_name + ": " + std::to_string(magnitude) + " km",
      body_name,
      -1,
      "Verify position data source and units"
    ));
    return false;
  }

  if (magnitude > config_.position_max_km) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidPosition,
      ValidationSeverity::Error,
      "Position magnitude too large for " + body_name + ": " + std::to_string(magnitude) + " km",
      body_name,
      -1,
      "Verify position data source and units"
    ));
    return false;
  }

  // Check for NaN or infinite values
  if (!std::isfinite(position.x()) || !std::isfinite(position.y()) || !std::isfinite(position.z())) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidPosition,
      ValidationSeverity::Critical,
      "Position contains non-finite values for " + body_name,
      body_name,
      -1,
      "Check data source for corruption"
    ));
    return false;
  }

  return true;
}

JPLResult<bool> DataValidator::validate_velocity_vector(const SolarSystem::Math::Vector3d& velocity, const std::string& body_name, std::vector<ValidationIssue>& issues) const {
  double magnitude = velocity.magnitude();

  if (magnitude > config_.velocity_max_km_s) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidVelocity,
      ValidationSeverity::Error,
      "Velocity magnitude too large for " + body_name + ": " + std::to_string(magnitude) + " km/s",
      body_name,
      -1,
      "Verify velocity data source and units"
    ));
    return false;
  }

  // Check for NaN or infinite values
  if (!std::isfinite(velocity.x()) || !std::isfinite(velocity.y()) || !std::isfinite(velocity.z())) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidVelocity,
      ValidationSeverity::Critical,
      "Velocity contains non-finite values for " + body_name,
      body_name,
      -1,
      "Check data source for corruption"
    ));
    return false;
  }

  return true;
}

JPLResult<bool> DataValidator::validate_mass_value(long double mass, const std::string& body_name, std::vector<ValidationIssue>& issues) const {
  if (mass <= 0) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidMass,
      ValidationSeverity::Error,
      "Mass must be positive for " + body_name + ": " + std::to_string(static_cast<double>(mass)),
      body_name,
      -1,
      "Verify mass data source"
    ));
    return false;
  }

  if (mass < config_.mass_min_kg || mass > config_.mass_max_kg) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::OutOfRangeValues,
      ValidationSeverity::Warning,
      "Mass outside expected range for " + body_name + ": " + std::to_string(static_cast<double>(mass)) + " kg",
      body_name,
      -1,
      "Verify mass is in correct units (kg)"
    ));
  }

  if (!std::isfinite(static_cast<double>(mass))) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidMass,
      ValidationSeverity::Critical,
      "Mass contains non-finite value for " + body_name,
      body_name,
      -1,
      "Check data source for corruption"
    ));
    return false;
  }

  return true;
}

JPLResult<bool> DataValidator::validate_jpl_id(int jpl_id, const std::string& body_name, std::vector<ValidationIssue>& issues) const {
  if (jpl_id <= 0) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJPLId,
      ValidationSeverity::Error,
      "JPL ID must be positive for " + body_name + ": " + std::to_string(jpl_id),
      body_name,
      jpl_id,
      "Verify JPL ID mapping"
    ));
    return false;
  }

  if (jpl_id > 10000) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJPLId,
      ValidationSeverity::Warning,
      "JPL ID unusually high for " + body_name + ": " + std::to_string(jpl_id),
      body_name,
      jpl_id,
      "Verify JPL ID is correct"
    ));
  }

  return true;
}

JPLResult<bool> DataValidator::validate_body_name(const std::string& body_name, std::vector<ValidationIssue>& issues) const {
  if (body_name.empty()) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidBodyName,
      ValidationSeverity::Error,
      "Body name cannot be empty",
      body_name,
      -1,
      "Provide valid body name"
    ));
    return false;
  }

  if (body_name.length() > 100) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidBodyName,
      ValidationSeverity::Warning,
      "Body name unusually long: " + body_name,
      body_name,
      -1,
      "Verify body name is correct"
    ));
  }

  return true;
}

JPLResult<bool> DataValidator::validate_epoch(const std::chrono::system_clock::time_point& epoch, const std::string& body_name, std::vector<ValidationIssue>& issues) const {
  auto now = std::chrono::system_clock::now();

  // Check for future epochs (suspicious)
  if (epoch > now) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidEpoch,
      ValidationSeverity::Warning,
      "Epoch is in the future for " + body_name,
      body_name,
      -1,
      "Verify epoch timestamp"
    ));
  }

  // Check for very old epochs (potentially stale data)
  auto age = now - epoch;
  if (age > std::chrono::hours(24 * 365)) {  // Older than 1 year
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidEpoch,
      ValidationSeverity::Info,
      "Epoch is more than 1 year old for " + body_name,
      body_name,
      -1,
      "Consider updating ephemeris data"
    ));
  }

  return true;
}

// Additional placeholder implementations for statistical validation
JPLResult<bool> DataValidator::validate_position_distribution(const std::vector<EphemerisData>&, std::vector<ValidationIssue>&) const {
  return true;
}

JPLResult<bool> DataValidator::validate_velocity_distribution(const std::vector<EphemerisData>&, std::vector<ValidationIssue>&) const {
  return true;
}

JPLResult<bool> DataValidator::validate_mass_distribution(const std::vector<EphemerisData>&, std::vector<ValidationIssue>&) const {
  return true;
}

JPLResult<bool> DataValidator::validate_binary_header(std::ifstream&, std::vector<ValidationIssue>&) const {
  return true;
}

JPLResult<bool> DataValidator::validate_json_structure(const std::string&, std::vector<ValidationIssue>&) const {
  return true;
}

JPLResult<bool> DataValidator::compare_ephemeris_data(const EphemerisData&, const EphemerisData&, double, std::vector<ValidationIssue>&) const {
  return true;
}

JPLResult<uint64_t> DataValidator::calculate_data_checksum(const std::vector<EphemerisData>&) const {
  return static_cast<uint64_t>(0);
}

std::string DataValidator::format_validation_issue(const ValidationIssue&) const {
  return "";
}

/**
 * @brief Factory implementations
 */
std::unique_ptr<DataValidator> DataValidatorFactory::create_default() {
  return std::make_unique<DataValidator>();
}

std::unique_ptr<DataValidator> DataValidatorFactory::create(DataValidatorConfig config) {
  return std::make_unique<DataValidator>(std::move(config));
}

std::unique_ptr<DataValidator> DataValidatorFactory::create_for_testing() {
  DataValidatorConfig config;
  config.enable_detailed_logging = false;
  config.validation_timeout = std::chrono::seconds(10);
  return std::make_unique<DataValidator>(std::move(config));
}

/**
 * @brief Utility function implementations
 */
namespace ValidationUtils {

std::string to_string(ValidationErrorType error_type) {
  switch (error_type) {
    case ValidationErrorType::InvalidJPLId: return "InvalidJPLId";
    case ValidationErrorType::InvalidPosition: return "InvalidPosition";
    case ValidationErrorType::InvalidVelocity: return "InvalidVelocity";
    case ValidationErrorType::InvalidMass: return "InvalidMass";
    case ValidationErrorType::InvalidBodyName: return "InvalidBodyName";
    case ValidationErrorType::InvalidEpoch: return "InvalidEpoch";
    case ValidationErrorType::OutOfRangeValues: return "OutOfRangeValues";
    case ValidationErrorType::SuspiciousValues: return "SuspiciousValues";
    case ValidationErrorType::InconsistentData: return "InconsistentData";
    case ValidationErrorType::MissingData: return "MissingData";
    case ValidationErrorType::DuplicateData: return "DuplicateData";
    case ValidationErrorType::InvalidBinaryFormat: return "InvalidBinaryFormat";
    case ValidationErrorType::InvalidJSONFormat: return "InvalidJSONFormat";
    case ValidationErrorType::CorruptedData: return "CorruptedData";
    case ValidationErrorType::IncompatibleVersion: return "IncompatibleVersion";
    case ValidationErrorType::CrossFormatMismatch: return "CrossFormatMismatch";
    case ValidationErrorType::MetadataMismatch: return "MetadataMismatch";
    case ValidationErrorType::ChecksumMismatch: return "ChecksumMismatch";
    case ValidationErrorType::TimestampInconsistency: return "TimestampInconsistency";
    default: return "Unknown";
  }
}

std::string to_string(ValidationSeverity severity) {
  switch (severity) {
    case ValidationSeverity::Info: return "Info";
    case ValidationSeverity::Warning: return "Warning";
    case ValidationSeverity::Error: return "Error";
    case ValidationSeverity::Critical: return "Critical";
    default: return "Unknown";
  }
}

bool is_reasonable_astronomical_value(double value, const std::string& value_type) {
  if (value_type == "position_km") {
    return value >= 1e3 && value <= 1e12;  // 1,000 km to 1 trillion km
  } else if (value_type == "velocity_km_s") {
    return value >= 0 && value <= 1e6;     // 0 to 1 million km/s
  } else if (value_type == "mass_kg") {
    return value >= 1e10 && value <= 1e35; // 10 billion kg to 10^35 kg
  }
  return false;
}

std::vector<size_t> find_statistical_outliers(const std::vector<double>& values, double threshold) {
  std::vector<size_t> outliers;

  if (values.size() < 3) {
    return outliers;  // Need at least 3 values for meaningful statistics
  }

  // Calculate mean and standard deviation
  double sum = std::accumulate(values.begin(), values.end(), 0.0);
  double mean = sum / values.size();

  double sq_sum = std::inner_product(values.begin(), values.end(), values.begin(), 0.0);
  double stdev = std::sqrt(sq_sum / values.size() - mean * mean);

  // Find outliers
  for (size_t i = 0; i < values.size(); ++i) {
    double z_score = std::abs(values[i] - mean) / stdev;
    if (z_score > threshold) {
      outliers.push_back(i);
    }
  }

  return outliers;
}

std::string generate_validation_summary(const ValidationReport& report) {
  return report.generate_summary();
}

}  // namespace ValidationUtils

}  // namespace SolarSystem::JPL
