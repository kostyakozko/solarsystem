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
#include <nlohmann/json.hpp>
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
  nlohmann::json j;

  j["validation_timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
      validation_timestamp.time_since_epoch()).count();
  j["validation_level"] = static_cast<int>(validation_level);
  j["validation_duration_ms"] = validation_duration.count();
  j["bodies_validated"] = bodies_validated;
  j["files_validated"] = files_validated;
  j["total_issues"] = total_issues;
  j["critical_issues"] = critical_issues;
  j["error_issues"] = error_issues;
  j["warning_issues"] = warning_issues;
  j["info_issues"] = info_issues;
  j["validation_passed"] = validation_passed;
  j["confidence_score"] = confidence_score;

  j["quality_metrics"] = {
      {"overall_quality_score", quality_metrics.overall_quality_score},
      {"completeness_ratio", quality_metrics.completeness_ratio},
      {"accuracy_score", quality_metrics.accuracy_score},
      {"consistency_score", quality_metrics.consistency_score}};

  return j.dump(2);
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

/**
 * @brief Validate cache integrity
 */
JPLResult<ValidationReport> DataValidator::validate_cache_integrity(
    const std::filesystem::path& cache_directory) {
  update_statistics("validate_cache_integrity");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Comprehensive;

  std::vector<ValidationIssue> issues;

  // Check if cache directory exists
  if (!std::filesystem::exists(cache_directory)) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Critical,
      "Cache directory does not exist: " + cache_directory.string(),
      "",
      -1,
      "Create cache directory or regenerate cache"
    ));
    report.issues = std::move(issues);
    report.total_issues = report.issues.size();
    report.critical_issues = report.issues.size();
    report.validation_passed = false;
    return report;
  }

  // Check for binary cache file
  auto binary_cache = cache_directory / "ephemeris_cache.bin";
  if (std::filesystem::exists(binary_cache)) {
    auto binary_result = validate_binary_format(binary_cache);
    if (is_success(binary_result)) {
      const auto& binary_report = get_value(binary_result);
      issues.insert(issues.end(), binary_report.issues.begin(), binary_report.issues.end());
      report.files_validated++;
    }
  } else {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Warning,
      "Binary cache file not found: " + binary_cache.string(),
      "",
      -1,
      "Generate binary cache for improved performance"
    ));
  }

  // Check for JSON cache file
  auto json_cache = cache_directory / "ephemeris_data.json";
  if (std::filesystem::exists(json_cache)) {
    auto json_result = validate_json_format(json_cache);
    if (is_success(json_result)) {
      const auto& json_report = get_value(json_result);
      issues.insert(issues.end(), json_report.issues.begin(), json_report.issues.end());
      report.files_validated++;
    }
  } else {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Info,
      "JSON cache file not found: " + json_cache.string(),
      "",
      -1,
      "JSON cache is optional but useful for debugging"
    ));
  }

  // Cross-format validation if both files exist
  if (config_.enable_cross_format_validation &&
      std::filesystem::exists(binary_cache) &&
      std::filesystem::exists(json_cache)) {
    auto cross_result = validate_cross_format_consistency(binary_cache, json_cache);
    if (is_success(cross_result)) {
      const auto& cross_report = get_value(cross_result);
      issues.insert(issues.end(), cross_report.issues.begin(), cross_report.issues.end());
    }
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
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
 * @brief Generate comprehensive quality report
 */
JPLResult<ValidationReport> DataValidator::generate_quality_report(
    const std::vector<EphemerisData>& data_collection) {
  update_statistics("generate_quality_report");

  // Use comprehensive validation
  auto validation_result = validate_ephemeris_collection(data_collection);
  if (!is_success(validation_result)) {
    return get_error(validation_result);
  }

  auto report = get_value(validation_result);
  report.validation_level = ValidationLevel::Comprehensive;

  // Add statistical validation if enabled
  if (config_.enable_statistical_validation) {
    auto stats_result = validate_statistical_properties(data_collection);
    if (is_success(stats_result)) {
      const auto& stats_report = get_value(stats_result);
      report.issues.insert(report.issues.end(),
                          stats_report.issues.begin(),
                          stats_report.issues.end());
    }
  }

  // Add temporal validation if enabled
  if (config_.enable_temporal_validation) {
    auto temporal_result = validate_temporal_consistency(data_collection);
    if (is_success(temporal_result)) {
      const auto& temporal_report = get_value(temporal_result);
      report.issues.insert(report.issues.end(),
                          temporal_report.issues.begin(),
                          temporal_report.issues.end());
    }
  }

  // Recalculate issue counts
  report.total_issues = report.issues.size();
  report.critical_issues = 0;
  report.error_issues = 0;
  report.warning_issues = 0;
  report.info_issues = 0;

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
    }
  }

  report.validation_passed = (report.critical_issues == 0 && report.error_issues == 0) &&
                            report.quality_metrics.meets_quality_threshold(config_.overall_quality_threshold);

  return report;
}

/**
 * @brief Validate statistical properties
 */
JPLResult<ValidationReport> DataValidator::validate_statistical_properties(
    const std::vector<EphemerisData>& data_collection) {
  update_statistics("validate_statistical_properties");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Standard;
  report.bodies_validated = data_collection.size();

  std::vector<ValidationIssue> issues;

  if (data_collection.size() < 3) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Warning,
      "Insufficient data for statistical validation (need at least 3 bodies)",
      "",
      -1,
      "Add more bodies to enable statistical validation"
    ));
  } else {
    // Validate position distribution
    auto pos_result = validate_position_distribution(data_collection, issues);
    (void)pos_result;

    // Validate velocity distribution
    auto vel_result = validate_velocity_distribution(data_collection, issues);
    (void)vel_result;

    // Validate mass distribution
    auto mass_result = validate_mass_distribution(data_collection, issues);
    (void)mass_result;
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
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
 * @brief Validate JSON format
 */
JPLResult<ValidationReport> DataValidator::validate_json_format(
    const std::filesystem::path& json_file) {
  update_statistics("validate_json_format");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Standard;
  report.files_validated = 1;

  std::vector<ValidationIssue> issues;

  // Check file existence
  if (!std::filesystem::exists(json_file)) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Critical,
      "JSON cache file does not exist: " + json_file.string(),
      "",
      -1,
      "Create or regenerate JSON cache file"
    ));
  } else {
    // Check file size
    auto file_size = std::filesystem::file_size(json_file);
    if (file_size == 0) {
      issues.push_back(create_validation_issue(
        ValidationErrorType::CorruptedData,
        ValidationSeverity::Critical,
        "JSON cache file is empty: " + json_file.string(),
        "",
        -1,
        "Regenerate JSON cache file"
      ));
    } else {
      // Read and validate JSON structure
      std::ifstream file(json_file);
      if (file.is_open()) {
        std::string json_content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        auto json_result = validate_json_structure(json_content, issues);
        (void)json_result;
      } else {
        issues.push_back(create_validation_issue(
          ValidationErrorType::InvalidJSONFormat,
          ValidationSeverity::Error,
          "Cannot open JSON cache file for reading: " + json_file.string(),
          "",
          -1,
          "Check file permissions"
        ));
      }
    }
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
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
 * @brief Validate format conversion
 */
JPLResult<ValidationReport> DataValidator::validate_format_conversion(
    const std::filesystem::path& source_file,
    const std::filesystem::path& target_file) {
  update_statistics("validate_format_conversion");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Standard;
  report.files_validated = 2;

  std::vector<ValidationIssue> issues;

  // Check source file
  if (!std::filesystem::exists(source_file)) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Critical,
      "Source file does not exist: " + source_file.string(),
      "",
      -1,
      "Provide valid source file"
    ));
  }

  // Check target file
  if (!std::filesystem::exists(target_file)) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Critical,
      "Target file does not exist: " + target_file.string(),
      "",
      -1,
      "Perform format conversion first"
    ));
  }

  // If both files exist, validate consistency
  if (std::filesystem::exists(source_file) && std::filesystem::exists(target_file)) {
    auto consistency_result = validate_cross_format_consistency(source_file, target_file);
    if (is_success(consistency_result)) {
      const auto& consistency_report = get_value(consistency_result);
      issues.insert(issues.end(), consistency_report.issues.begin(), consistency_report.issues.end());
    }
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
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
 * @brief Validate cross-format consistency
 */
JPLResult<ValidationReport> DataValidator::validate_cross_format_consistency(
    const std::filesystem::path& binary_file,
    const std::filesystem::path& json_file) {
  update_statistics("validate_cross_format_consistency");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Comprehensive;
  report.files_validated = 2;

  std::vector<ValidationIssue> issues;

  // Check file existence
  if (!std::filesystem::exists(binary_file)) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Error,
      "Binary file does not exist: " + binary_file.string(),
      "",
      -1,
      "Generate binary cache file"
    ));
  }

  if (!std::filesystem::exists(json_file)) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Error,
      "JSON file does not exist: " + json_file.string(),
      "",
      -1,
      "Generate JSON cache file"
    ));
  }

  // If both files exist, compare timestamps
  if (std::filesystem::exists(binary_file) && std::filesystem::exists(json_file)) {
    auto binary_time = std::filesystem::last_write_time(binary_file);
    auto json_time = std::filesystem::last_write_time(json_file);

    auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
      binary_time - json_time);

    if (std::abs(time_diff.count()) > 60) {  // More than 1 minute difference
      issues.push_back(create_validation_issue(
        ValidationErrorType::TimestampInconsistency,
        ValidationSeverity::Warning,
        "Binary and JSON cache files have different timestamps (diff: " +
          std::to_string(time_diff.count()) + " seconds)",
        "",
        -1,
        "Regenerate both cache files to ensure consistency"
      ));
    }

    // Compare file sizes (rough check)
    auto binary_size = std::filesystem::file_size(binary_file);
    auto json_size = std::filesystem::file_size(json_file);

    // Binary should be smaller than JSON (compressed)
    if (binary_size > json_size) {
      issues.push_back(create_validation_issue(
        ValidationErrorType::SuspiciousValues,
        ValidationSeverity::Warning,
        "Binary cache is larger than JSON cache (binary: " +
          std::to_string(binary_size) + ", json: " + std::to_string(json_size) + ")",
        "",
        -1,
        "Verify cache generation process"
      ));
    }
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
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
 * @brief Validate metadata consistency
 */
JPLResult<ValidationReport> DataValidator::validate_metadata_consistency(
    const CacheMetadata& metadata,
    const std::vector<EphemerisData>& actual_data) {
  update_statistics("validate_metadata_consistency");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Standard;
  report.bodies_validated = actual_data.size();

  std::vector<ValidationIssue> issues;

  // Check body count consistency
  if (metadata.body_count != actual_data.size()) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MetadataMismatch,
      ValidationSeverity::Error,
      "Metadata body count (" + std::to_string(metadata.body_count) +
        ") does not match actual data count (" + std::to_string(actual_data.size()) + ")",
      "",
      -1,
      "Regenerate cache with correct metadata"
    ));
  }

  // Check timestamp freshness
  auto now = std::chrono::system_clock::now();
  auto age = now - metadata.created_at;
  if (age > std::chrono::hours(24 * 30)) {  // Older than 30 days
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidEpoch,
      ValidationSeverity::Info,
      "Cache is more than 30 days old",
      "",
      -1,
      "Consider refreshing cache data"
    ));
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
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
 * @brief Validate temporal consistency
 */
JPLResult<ValidationReport> DataValidator::validate_temporal_consistency(
    const std::vector<EphemerisData>& data_collection) {
  update_statistics("validate_temporal_consistency");

  auto start_time = std::chrono::steady_clock::now();

  ValidationReport report;
  report.validation_timestamp = std::chrono::system_clock::now();
  report.validation_level = ValidationLevel::Standard;
  report.bodies_validated = data_collection.size();

  std::vector<ValidationIssue> issues;

  if (data_collection.empty()) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::MissingData,
      ValidationSeverity::Error,
      "No data provided for temporal validation",
      "",
      -1,
      "Provide ephemeris data"
    ));
  } else {
    // Check that all epochs are reasonably close
    auto first_epoch = data_collection[0].epoch;

    for (size_t i = 1; i < data_collection.size(); ++i) {
      auto time_diff = std::chrono::duration_cast<std::chrono::hours>(
        data_collection[i].epoch - first_epoch);

      if (std::abs(time_diff.count()) > 24) {  // More than 24 hours difference
        issues.push_back(create_validation_issue(
          ValidationErrorType::TimestampInconsistency,
          ValidationSeverity::Warning,
          "Body " + data_collection[i].body_name + " has epoch " +
            std::to_string(time_diff.count()) + " hours different from first body",
          data_collection[i].body_name,
          data_collection[i].jpl_id,
          "Ensure all ephemeris data is from the same time point"
        ));
      }
    }

    // Check for future epochs
    auto now = std::chrono::system_clock::now();
    for (const auto& data : data_collection) {
      if (data.epoch > now) {
        issues.push_back(create_validation_issue(
          ValidationErrorType::InvalidEpoch,
          ValidationSeverity::Warning,
          "Body " + data.body_name + " has future epoch",
          data.body_name,
          data.jpl_id,
          "Verify epoch timestamp"
        ));
      }
    }
  }

  // Compile report
  report.issues = std::move(issues);
  report.total_issues = report.issues.size();

  for (const auto& issue : report.issues) {
    switch (issue.severity) {
      case ValidationSeverity::Critical: report.critical_issues++; break;
      case ValidationSeverity::Error: report.error_issues++; break;
      case ValidationSeverity::Warning: report.warning_issues++; break;
      case ValidationSeverity::Info: report.info_issues++; break;
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
 * @brief Update configuration
 */
JPLVoidResult DataValidator::update_config(const DataValidatorConfig& new_config) {
  std::string error;
  if (!new_config.is_valid(&error)) {
    return JPLError::ValidationError;
  }

  config_ = new_config;
  return success();
}

// Private validation method implementations
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

/**
 * @brief Validate position distribution
 */
JPLResult<bool> DataValidator::validate_position_distribution(
    const std::vector<EphemerisData>& data_collection,
    std::vector<ValidationIssue>& issues) const {

  if (data_collection.size() < 3) {
    return true;  // Not enough data for statistical analysis
  }

  // Extract position magnitudes
  std::vector<double> position_magnitudes;
  position_magnitudes.reserve(data_collection.size());

  for (const auto& data : data_collection) {
    position_magnitudes.push_back(data.position.magnitude());
  }

  // Find statistical outliers
  auto outliers = ValidationUtils::find_statistical_outliers(position_magnitudes, 3.0);

  for (auto idx : outliers) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::SuspiciousValues,
      ValidationSeverity::Warning,
      "Body " + data_collection[idx].body_name + " has outlier position magnitude: " +
        std::to_string(position_magnitudes[idx]) + " km",
      data_collection[idx].body_name,
      data_collection[idx].jpl_id,
      "Verify position data for this body"
    ));
  }

  return true;
}

/**
 * @brief Validate velocity distribution
 */
JPLResult<bool> DataValidator::validate_velocity_distribution(
    const std::vector<EphemerisData>& data_collection,
    std::vector<ValidationIssue>& issues) const {

  if (data_collection.size() < 3) {
    return true;  // Not enough data for statistical analysis
  }

  // Extract velocity magnitudes
  std::vector<double> velocity_magnitudes;
  velocity_magnitudes.reserve(data_collection.size());

  for (const auto& data : data_collection) {
    velocity_magnitudes.push_back(data.velocity.magnitude());
  }

  // Find statistical outliers
  auto outliers = ValidationUtils::find_statistical_outliers(velocity_magnitudes, 3.0);

  for (auto idx : outliers) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::SuspiciousValues,
      ValidationSeverity::Warning,
      "Body " + data_collection[idx].body_name + " has outlier velocity magnitude: " +
        std::to_string(velocity_magnitudes[idx]) + " km/s",
      data_collection[idx].body_name,
      data_collection[idx].jpl_id,
      "Verify velocity data for this body"
  ));
  }

  return true;
}

/**
 * @brief Validate mass distribution
 */
JPLResult<bool> DataValidator::validate_mass_distribution(
    const std::vector<EphemerisData>& data_collection,
    std::vector<ValidationIssue>& issues) const {

  if (data_collection.size() < 3) {
    return true;  // Not enough data for statistical analysis
  }

  // Extract masses (convert to double for statistics)
  std::vector<double> masses;
  masses.reserve(data_collection.size());

  for (const auto& data : data_collection) {
    masses.push_back(static_cast<double>(data.mass));
  }

  // Find statistical outliers (using log scale for masses due to large range)
  std::vector<double> log_masses;
  log_masses.reserve(masses.size());
  for (double mass : masses) {
    if (mass > 0) {
      log_masses.push_back(std::log10(mass));
    }
  }

  auto outliers = ValidationUtils::find_statistical_outliers(log_masses, 3.0);

  for (auto idx : outliers) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::SuspiciousValues,
      ValidationSeverity::Info,
      "Body " + data_collection[idx].body_name + " has outlier mass: " +
        std::to_string(masses[idx]) + " kg",
      data_collection[idx].body_name,
      data_collection[idx].jpl_id,
      "Verify mass data for this body (may be normal for extreme bodies)"
    ));
  }

  return true;
}

/**
 * @brief Validate binary header
 */
JPLResult<bool> DataValidator::validate_binary_header(
    std::ifstream& file,
    std::vector<ValidationIssue>& issues) const {

  // Read magic number (first 4 bytes)
  uint32_t magic;
  file.read(reinterpret_cast<char*>(&magic), sizeof(magic));

  if (!file.good()) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidBinaryFormat,
      ValidationSeverity::Error,
      "Cannot read binary header magic number",
      "",
      -1,
      "File may be corrupted or truncated"
    ));
    return false;
  }

  // Expected magic number: "EPHE" = 0x45504845
  const uint32_t expected_magic = 0x45504845;
  if (magic != expected_magic) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidBinaryFormat,
      ValidationSeverity::Critical,
      "Invalid binary header magic number: 0x" +
        std::to_string(magic) + " (expected 0x" + std::to_string(expected_magic) + ")",
      "",
      -1,
      "File is not a valid ephemeris cache or is corrupted"
    ));
    return false;
  }

  // Read version (next 4 bytes)
  uint32_t version;
  file.read(reinterpret_cast<char*>(&version), sizeof(version));

  if (!file.good()) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidBinaryFormat,
      ValidationSeverity::Error,
      "Cannot read binary header version",
      "",
      -1,
      "File may be corrupted or truncated"
    ));
    return false;
  }

  // Check version compatibility (major version should be 1)
  uint16_t major_version = (version >> 16) & 0xFFFF;
  if (major_version != 1) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::IncompatibleVersion,
      ValidationSeverity::Warning,
      "Binary cache version " + std::to_string(major_version) + " may not be compatible",
      "",
      -1,
      "Consider regenerating cache with current version"
    ));
  }

  return true;
}

/**
 * @brief Validate JSON structure
 */
JPLResult<bool> DataValidator::validate_json_structure(
    const std::string& json_content,
    std::vector<ValidationIssue>& issues) const {

  // Basic JSON syntax validation
  if (json_content.empty()) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJSONFormat,
      ValidationSeverity::Critical,
      "JSON content is empty",
      "",
      -1,
      "Regenerate JSON cache file"
    ));
    return false;
  }

  // Check for basic JSON structure
  if (json_content.front() != '{' && json_content.front() != '[') {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJSONFormat,
      ValidationSeverity::Critical,
      "JSON does not start with '{' or '['",
      "",
      -1,
      "File may be corrupted or not valid JSON"
    ));
    return false;
  }

  if (json_content.back() != '}' && json_content.back() != ']') {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJSONFormat,
      ValidationSeverity::Critical,
      "JSON does not end with '}' or ']'",
      "",
      -1,
      "File may be corrupted or truncated"
    ));
    return false;
  }

  // Check for balanced braces
  int brace_count = 0;
  int bracket_count = 0;

  for (char c : json_content) {
    if (c == '{') brace_count++;
    else if (c == '}') brace_count--;
    else if (c == '[') bracket_count++;
    else if (c == ']') bracket_count--;
  }

  if (brace_count != 0) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJSONFormat,
      ValidationSeverity::Error,
      "Unbalanced braces in JSON (difference: " + std::to_string(brace_count) + ")",
      "",
      -1,
      "File may be corrupted or malformed"
    ));
    return false;
  }

  if (bracket_count != 0) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJSONFormat,
      ValidationSeverity::Error,
      "Unbalanced brackets in JSON (difference: " + std::to_string(bracket_count) + ")",
      "",
      -1,
      "File may be corrupted or malformed"
    ));
    return false;
  }

  // Check for required fields (basic check)
  if (json_content.find("\"bodies\"") == std::string::npos &&
      json_content.find("\"ephemeris_data\"") == std::string::npos) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::InvalidJSONFormat,
      ValidationSeverity::Warning,
      "JSON does not contain expected 'bodies' or 'ephemeris_data' field",
      "",
      -1,
      "Verify JSON structure matches expected format"
    ));
  }

  return true;
}

/**
 * @brief Compare two ephemeris data entries
 */
JPLResult<bool> DataValidator::compare_ephemeris_data(
    const EphemerisData& data1,
    const EphemerisData& data2,
    double tolerance,
    std::vector<ValidationIssue>& issues) const {

  bool all_match = true;

  // Compare JPL IDs
  if (data1.jpl_id != data2.jpl_id) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::CrossFormatMismatch,
      ValidationSeverity::Error,
      "JPL ID mismatch: " + std::to_string(data1.jpl_id) + " vs " + std::to_string(data2.jpl_id),
      data1.body_name,
      data1.jpl_id,
      "Verify data sources are consistent"
    ));
    all_match = false;
  }

  // Compare body names
  if (data1.body_name != data2.body_name) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::CrossFormatMismatch,
      ValidationSeverity::Error,
      "Body name mismatch: " + data1.body_name + " vs " + data2.body_name,
      data1.body_name,
      data1.jpl_id,
      "Verify data sources are consistent"
    ));
    all_match = false;
  }

  // Compare positions (with tolerance)
  double pos_diff = (data1.position - data2.position).magnitude();
  if (pos_diff > tolerance) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::CrossFormatMismatch,
      ValidationSeverity::Warning,
      "Position difference exceeds tolerance for " + data1.body_name + ": " +
        std::to_string(pos_diff) + " km",
      data1.body_name,
      data1.jpl_id,
      "Verify data precision and conversion accuracy"
    ));
    all_match = false;
  }

  // Compare velocities (with tolerance)
  double vel_diff = (data1.velocity - data2.velocity).magnitude();
  if (vel_diff > tolerance) {
    issues.push_back(create_validation_issue(
      ValidationErrorType::CrossFormatMismatch,
      ValidationSeverity::Warning,
      "Velocity difference exceeds tolerance for " + data1.body_name + ": " +
        std::to_string(vel_diff) + " km/s",
      data1.body_name,
      data1.jpl_id,
      "Verify data precision and conversion accuracy"
    ));
    all_match = false;
  }

  // Compare masses (with relative tolerance)
  double mass_diff = std::abs(static_cast<double>(data1.mass - data2.mass));
  double mass_avg = (static_cast<double>(data1.mass) + static_cast<double>(data2.mass)) / 2.0;
  double relative_mass_diff = mass_diff / mass_avg;

  if (relative_mass_diff > tolerance / 1000.0) {  // Use smaller tolerance for mass
    issues.push_back(create_validation_issue(
      ValidationErrorType::CrossFormatMismatch,
      ValidationSeverity::Warning,
      "Mass difference exceeds tolerance for " + data1.body_name + ": " +
        std::to_string(relative_mass_diff * 100.0) + "%",
      data1.body_name,
      data1.jpl_id,
      "Verify data precision and conversion accuracy"
    ));
    all_match = false;
  }

  return all_match;
}

/**
 * @brief Calculate checksum for data collection
 */
JPLResult<uint64_t> DataValidator::calculate_data_checksum(
    const std::vector<EphemerisData>& data_collection) const {

  // Simple checksum using FNV-1a hash algorithm
  uint64_t hash = 14695981039346656037ULL;  // FNV offset basis
  const uint64_t fnv_prime = 1099511628211ULL;

  for (const auto& data : data_collection) {
    // Hash JPL ID
    hash ^= static_cast<uint64_t>(data.jpl_id);
    hash *= fnv_prime;

    // Hash position components (convert to double for hashing)
    long double pos_x = data.position.x();
    long double pos_y = data.position.y();
    long double pos_z = data.position.z();

    auto pos_x_bytes = reinterpret_cast<const uint8_t*>(&pos_x);
    for (size_t i = 0; i < sizeof(long double); ++i) {
      hash ^= pos_x_bytes[i];
      hash *= fnv_prime;
    }

    auto pos_y_bytes = reinterpret_cast<const uint8_t*>(&pos_y);
    for (size_t i = 0; i < sizeof(long double); ++i) {
      hash ^= pos_y_bytes[i];
      hash *= fnv_prime;
    }

    auto pos_z_bytes = reinterpret_cast<const uint8_t*>(&pos_z);
    for (size_t i = 0; i <sizeof(long double); ++i) {
      hash ^= pos_z_bytes[i];
      hash *= fnv_prime;
    }

    // Hash velocity components
    long double vel_x = data.velocity.x();
    long double vel_y = data.velocity.y();
    long double vel_z = data.velocity.z();

    auto vel_x_bytes = reinterpret_cast<const uint8_t*>(&vel_x);
    for (size_t i = 0; i < sizeof(long double); ++i) {
 hash ^= vel_x_bytes[i];
      hash*= fnv_prime;
    }

    auto vel_y_bytes = reinterpret_cast<const uint8_t*>(&vel_y);
    for (size_t i = 0; i < sizeof(long double); ++i) {
      hash ^= vel_y_bytes[i];
      hash *= fnv_prime;
    }

    auto vel_z_bytes = reinterpret_cast<const uint8_t*>(&vel_z);
    for (size_t i = 0; i < sizeof(long double); ++i) {
      hash ^= vel_z_bytes[i];
      hash *= fnv_prime;
    }

    // Hash mass
    auto mass_bytes = reinterpret_cast<const uint8_t*>(&data.mass);
    for (size_t i = 0; i < sizeof(long double); ++i) {
      hash ^= mass_bytes[i];
      hash *= fnv_prime;
    }
  }

  return hash;
}

/**
 * @brief Format validation issue for display
 */
std::string DataValidator::format_validation_issue(const ValidationIssue& issue) const {
  std::ostringstream formatted;

  formatted << "[" << ValidationUtils::to_string(issue.severity) << "] ";
  formatted << ValidationUtils::to_string(issue.error_type) << ": ";
  formatted << issue.description;

  if (!issue.affected_body.empty()) {
    formatted << " (Body: " << issue.affected_body;
    if (issue.affected_jpl_id >= 0) {
      formatted << ", JPL ID: " << issue.affected_jpl_id;
    }
    formatted << ")";
  }

  if (!issue.suggested_action.empty()) {
    formatted << "\n  Suggested Action: " << issue.suggested_action;
  }

  return formatted.str();
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
