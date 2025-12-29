/**
 * @file test_data_validator.cpp
 * @brief Unit tests for comprehensive data validation system
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>

#include "solar_jpl/data_validator.hpp"
#include "solar_jpl/cache_manager.hpp"
#include "solar_core/math/vector3.hpp"

using namespace SolarSystem::JPL;
using namespace SolarSystem::Math;
TEST(DataValidatorTests, Configuration_Validation) {
    DataValidatorConfig config;

    // Valid configuration
    std::string error;
    ASSERT_TRUE(config.is_valid(&error));

    // Invalid position range
    config.position_min_km = -1.0;
    ASSERT_FALSE(config.is_valid(&error));
    ASSERT_FALSE(error.empty());

    // Reset to valid
    config.position_min_km = 1e3;
    ASSERT_TRUE(config.is_valid(&error));

    // Invalid velocity maximum
    config.velocity_max_km_s = -1.0;
    ASSERT_FALSE(config.is_valid(&error));

    // Reset to valid
    config.velocity_max_km_s = 1e6;
    ASSERT_TRUE(config.is_valid(&error));
}
TEST(DataValidatorTests, Individual_Data_Validation) {
    auto validator = DataValidatorFactory::create_default();

    // Create sample ephemeris data
    EphemerisData sample_data;
    sample_data.body_name = "Earth";
    sample_data.jpl_id = 399;
    sample_data.epoch = std::chrono::system_clock::now();
    sample_data.position = Vector3d(1.496e8, 0.0, 0.0);  // 1 AU in km
    sample_data.velocity = Vector3d(0.0, 29.78, 0.0);    // Earth's orbital velocity in km/s
    sample_data.mass = 5.972e24;                          // Earth's mass in kg

    // Valid data should pass
    auto result = validator->validate_ephemeris_data(sample_data);
    ASSERT_TRUE(is_success(result));

    const auto& report = get_value(result);
    ASSERT_TRUE(report.validation_passed);
    ASSERT_EQ(report.critical_issues, 0);
    ASSERT_EQ(report.error_issues, 0);
    ASSERT_EQ(report.bodies_validated, 1);

    // Invalid JPL ID
    EphemerisData invalid_data = sample_data;
    invalid_data.jpl_id = -1;

    result = validator->validate_ephemeris_data(invalid_data);
    ASSERT_TRUE(is_success(result));

    const auto& invalid_report = get_value(result);
    ASSERT_FALSE(invalid_report.validation_passed);
    ASSERT_GT(invalid_report.error_issues, 0);
}
TEST(DataValidatorTests, Data_Quality_Assessment) {
    auto validator = DataValidatorFactory::create_default();

    std::vector<EphemerisData> collection;

    // Create a collection with various quality levels
    for (int i = 0; i < 5; ++i) {
      EphemerisData data;
      data.body_name = "TestBody" + std::to_string(i);
      data.jpl_id = 300 + i;
      data.epoch = std::chrono::system_clock::now();
      data.position = Vector3d(1e8 + i * 1e7, 0.0, 0.0);
      data.velocity = Vector3d(0.0, 20.0 + i, 0.0);
      data.mass = 1e24 + i * 1e23;
      collection.push_back(data);
    }

    auto result = validator->assess_data_quality(collection);
    ASSERT_TRUE(is_success(result));

    const auto& metrics = get_value(result);
    ASSERT_EQ(metrics.total_bodies_found, 5);
    ASSERT_GT(metrics.completeness_ratio, 0.0);
    ASSERT_GT(metrics.accuracy_score, 0.0);
    ASSERT_LE(metrics.overall_quality_score, 1.0);
}
TEST(DataValidatorTests, Binary_Format_Validation) {
    auto validator = DataValidatorFactory::create_default();

    // Create test directory
    auto test_dir = std::filesystem::temp_directory_path() / "data_validator_test";
    std::filesystem::create_directories(test_dir);

    // Test non-existent file
    auto non_existent_path = test_dir / "non_existent.bin";
    auto result = validator->validate_binary_format(non_existent_path);
    ASSERT_TRUE(is_success(result));

    const auto& report = get_value(result);
    ASSERT_FALSE(report.validation_passed);
    ASSERT_GT(report.critical_issues, 0);

    // Test empty file
    auto empty_path = test_dir / "empty.bin";
    std::ofstream empty_file(empty_path, std::ios::binary);
    empty_file.close();

    result = validator->validate_binary_format(empty_path);
    ASSERT_TRUE(is_success(result));

    const auto& empty_report = get_value(result);
    ASSERT_FALSE(empty_report.validation_passed);
    ASSERT_GT(empty_report.critical_issues, 0);

    // Clean up test directory
    if (std::filesystem::exists(test_dir)) {
      std::filesystem::remove_all(test_dir);
    }
}
TEST(DataValidatorTests, Validation_Report_Functionality) {
    ValidationReport report;
    report.validation_timestamp = std::chrono::system_clock::now();
    report.validation_level = ValidationLevel::Standard;
    report.bodies_validated = 5;
    report.total_issues = 3;
    report.critical_issues = 1;
    report.error_issues = 1;
    report.warning_issues = 1;
    report.info_issues = 0;
    report.validation_passed = false;
    report.confidence_score = 0.7;

    // Test validation status
    ASSERT_FALSE(report.is_valid());

    // Test summary generation
    std::string summary = report.generate_summary();
    ASSERT_FALSE(summary.empty());
    ASSERT_NE(summary.find("FAILED"), std::string::npos);
    ASSERT_NE(summary.find("Critical: 1"), std::string::npos);

    // Test JSON export
    std::string json = report.to_json();
    ASSERT_FALSE(json.empty());
    ASSERT_NE(json.find("\"validation_passed\": false"), std::string::npos);
    ASSERT_NE(json.find("\"critical_issues\": 1"), std::string::npos);

    // Test with passing report
    report.critical_issues = 0;
    report.error_issues = 0;
    report.validation_passed = true;
    ASSERT_TRUE(report.is_valid());
}
TEST(DataValidatorTests, Validation_Utilities) {
    // Test error type to string conversion
    ASSERT_EQ(ValidationUtils::to_string(ValidationErrorType::InvalidJPLId), "InvalidJPLId");
    ASSERT_EQ(ValidationUtils::to_string(ValidationErrorType::InvalidPosition), "InvalidPosition");
    ASSERT_EQ(ValidationUtils::to_string(ValidationErrorType::CorruptedData), "CorruptedData");

    // Test severity to string conversion
    ASSERT_EQ(ValidationUtils::to_string(ValidationSeverity::Critical), "Critical");
    ASSERT_EQ(ValidationUtils::to_string(ValidationSeverity::Error), "Error");
    ASSERT_EQ(ValidationUtils::to_string(ValidationSeverity::Warning), "Warning");
    ASSERT_EQ(ValidationUtils::to_string(ValidationSeverity::Info), "Info");

    // Test astronomical value validation
    ASSERT_TRUE(ValidationUtils::is_reasonable_astronomical_value(1.496e8, "position_km"));  // 1 AU
    ASSERT_FALSE(ValidationUtils::is_reasonable_astronomical_value(100.0, "position_km"));   // Too small
    ASSERT_FALSE(ValidationUtils::is_reasonable_astronomical_value(1e15, "position_km"));    // Too large

    ASSERT_TRUE(ValidationUtils::is_reasonable_astronomical_value(30.0, "velocity_km_s"));   // Earth orbital velocity
    ASSERT_FALSE(ValidationUtils::is_reasonable_astronomical_value(2e6, "velocity_km_s"));   // Too large

    ASSERT_TRUE(ValidationUtils::is_reasonable_astronomical_value(5.972e24, "mass_kg"));     // Earth mass
    ASSERT_FALSE(ValidationUtils::is_reasonable_astronomical_value(1e5, "mass_kg"));         // Too small
}
TEST(DataValidatorTests, Factory_Methods) {
    // Test default factory
    auto default_validator = DataValidatorFactory::create_default();
    ASSERT_NE(default_validator, nullptr);

    // Test custom configuration factory
    DataValidatorConfig custom_config;
    custom_config.position_max_km = 1e10;
    custom_config.enable_detailed_logging = false;

    auto custom_validator = DataValidatorFactory::create(custom_config);
    ASSERT_NE(custom_validator, nullptr);
    ASSERT_EQ(custom_validator->config().position_max_km, 1e10);

    // Test testing factory
    auto test_validator = DataValidatorFactory::create_for_testing();
    ASSERT_NE(test_validator, nullptr);
    ASSERT_EQ(test_validator->config().validation_timeout, std::chrono::seconds(10));
}
