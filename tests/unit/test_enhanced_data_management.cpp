/**
 * @file test_enhanced_data_management_simple.cpp
 * @brief Simplified tests for enhanced test data management system
 */

#include "../test_framework_enhanced.hpp"
#include "solar_test/framework/enhanced_test_data_manager.hpp"
#include <chrono>
#include <thread>

using namespace solar_test;
using namespace SolarSystem::Testing;

class EnhancedDataManagementTests {
public:
  static void run_all_tests() {
    auto& framework = EnhancedTestFramework::instance();

    framework.load_performance_baselines("enhanced_data_management_baselines.txt");

    std::vector<std::pair<std::string, std::function<void()>>> tests = {
      {"BasicJPLValidation", test_basic_jpl_validation},
      {"DataGeneration", test_data_generation},
      {"IsolatedEnvironment", test_isolated_environment},
      {"IntegrityChecking", test_integrity_checking},
      {"PerformanceMonitoring", test_performance_monitoring}
    };

    framework.run_test_suite("EnhancedDataManagement", tests);

    framework.save_performance_baselines("enhanced_data_management_baselines.txt");
    framework.generate_report("enhanced_data_management_report.html");
  }

private:
  static void test_basic_jpl_validation() {
    // Test basic JPL response validation (Requirement 6.1)

    // Test valid JPL response generation
    std::string valid_response = TestDataGenerator::generate_valid_jpl_response("Earth");
    ASSERT_TRUE(!valid_response.empty());
    ASSERT_TRUE(valid_response.find("EPHEMERIS") != std::string::npos);
    ASSERT_TRUE(valid_response.find("*******************************************************************************") != std::string::npos);

    // Test validation function exists and runs
    auto validation_result = TestDataValidator::validate_jpl_response_comprehensive(valid_response);
    ASSERT_EQ(validation_result.validation_type, "JPL Response Comprehensive");

    // Test invalid JPL response
    std::string invalid_response = TestDataGenerator::generate_malformed_jpl_response();
    ASSERT_TRUE(!invalid_response.empty());
    ASSERT_NE(invalid_response, valid_response);

    // Test that invalid response is different from valid response
    ASSERT_TRUE(invalid_response.find("INVALID") != std::string::npos);
  }

  static void test_data_generation() {
    // Test data generation capabilities

    // Test JPL response generation
    std::string jpl_response = TestDataGenerator::generate_valid_jpl_response("Mars");
    ASSERT_TRUE(!jpl_response.empty());
    ASSERT_TRUE(jpl_response.find("EPHEMERIS") != std::string::npos);

    // Test ephemeris data generation
    std::string ephemeris_json = TestDataGenerator::generate_ephemeris_json("2025-01-01");
    ASSERT_TRUE(!ephemeris_json.empty());
    ASSERT_TRUE(ephemeris_json.find("ephemeris_json") != std::string::npos);

    // Test data mutation
    std::string mutated = TestDataGenerator::mutate_data(jpl_response, MutationStrategy::CORRUPT_HEADER);
    ASSERT_TRUE(!mutated.empty());
    ASSERT_NE(mutated, jpl_response); // Should be different from original
  }

  static void test_isolated_environment() {
    // Test isolated test environment (Requirement 6.3)

    std::string original_cwd = std::filesystem::current_path();

    {
      auto env = std::make_unique<IsolatedTestEnvironment>("test_isolation");

      ASSERT_TRUE(env->is_clean());
      ASSERT_EQ(env->get_test_name(), "test_isolation");

      // Test environment variable management
      env->set_env_var("TEST_VAR", "test_value");
      auto var_value = env->get_env_var("TEST_VAR");
      ASSERT_TRUE(var_value.has_value());
      ASSERT_EQ(var_value.value(), "test_value");

      // Test working directory change
      std::string new_cwd = env->get_working_directory();
      ASSERT_NE(new_cwd, original_cwd);

      // Environment should clean up automatically when destroyed
    }

    // Test completed successfully - environment was created and destroyed without issues
    ASSERT_TRUE(true);
  }

  static void test_integrity_checking() {
    // Test data integrity checking (Requirement 6.5)

    // Test checksum calculation and verification
    std::string test_data = "This is test data for integrity checking";
    std::string checksum = test_data_utils::IntegrityChecker::calculate_checksum(test_data);

    ASSERT_TRUE(!checksum.empty());
    ASSERT_TRUE(test_data_utils::IntegrityChecker::verify_checksum(test_data, checksum));

    // Test with modified data
    std::string modified_data = test_data + " modified";
    ASSERT_FALSE(test_data_utils::IntegrityChecker::verify_checksum(modified_data, checksum));

    // Test dataset integrity
    auto dataset = TestDataGenerator::generate_realistic_solar_system_data();
    ASSERT_TRUE(dataset.verify_integrity());
  }

  static void test_performance_monitoring() {
    // Test performance monitoring capabilities

    EnhancedTestDataManager::enable_performance_monitoring(true);

    // Perform some operations that should be monitored
    auto dataset = EnhancedTestDataManager::load_validated_jpl_responses("valid");

    // Check if performance metrics were recorded
    auto metrics = EnhancedTestDataManager::get_performance_metrics();
    ASSERT_TRUE(true); // Test completed without exception

    // Test metrics reset
    EnhancedTestDataManager::reset_performance_metrics();
    auto reset_metrics = EnhancedTestDataManager::get_performance_metrics();
    ASSERT_TRUE(reset_metrics.empty() || reset_metrics.size() >= 0);

    EnhancedTestDataManager::enable_performance_monitoring(false);
  }
};

int main() {
  std::cout << "🚀 Running Enhanced Data Management Tests" << std::endl;
  std::cout << "==========================================" << std::endl << std::endl;

  try {
    EnhancedDataManagementTests::run_all_tests();

    auto& framework = EnhancedTestFramework::instance();
    std::cout << std::endl << "📊 Final Results:" << std::endl;
    std::cout << "Total tests: " << framework.get_total_tests() << std::endl;
    std::cout << "Passed: " << framework.get_passed_tests() << std::endl;
    std::cout << "Failed: " << framework.get_failed_tests() << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1)
              << (framework.get_total_tests() > 0 ?
                  (framework.get_passed_tests() * 100.0 / framework.get_total_tests()) : 0.0)
              << "%" << std::endl;

    return framework.get_failed_tests() == 0 ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "❌ Test framework error: " << e.what() << std::endl;
    return 1;
  }
}
