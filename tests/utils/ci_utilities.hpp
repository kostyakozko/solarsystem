/**
 * @file ci_utilities.hpp
 * @brief CI/CD integration utilities for test framework
 *
 * Provides:
 * - JUnit XML generation for CI integration
 * - Coverage report generation
 * - Container and CI environment detection
 * - Performance report generation
 */

#ifndef CI_UTILITIES_HPP
#define CI_UTILITIES_HPP

#include <map>
#include <string>
#include <vector>

namespace TestUtils {
namespace CI {

/**
 * @brief Test result for CI reporting
 */
struct TestCaseResult {
  std::string name;
  std::string classname;
  double time_seconds;
  bool passed;
  std::string failure_message;
  std::string failure_type;
  std::string system_out;
  std::string system_err;
};

/**
 * @brief Test suite result for CI reporting
 */
struct TestSuiteResult {
  std::string name;
  int tests;
  int failures;
  int errors;
  int skipped;
  double time_seconds;
  std::vector<TestCaseResult> test_cases;
};

/**
 * @brief CI artifact generator for test reporting
 */
class CIArtifactGenerator {
 public:
  /**
   * @brief Generate JUnit XML report
   */
  static bool generate_junit_xml(const std::vector<TestSuiteResult>& suites,
                                 const std::string& output_file);

  /**
   * @brief Generate coverage report in multiple formats
   */
  static bool generate_coverage_report(const std::string& coverage_data_file,
                                       const std::string& output_dir,
                                       const std::string& format = "html");

  /**
   * @brief Generate performance report with metrics
   */
  static bool generate_performance_report(const std::map<std::string, double>& metrics,
                                          const std::string& output_file);

  /**
   * @brief Aggregate test results from multiple sources
   */
  static TestSuiteResult aggregate_results(const std::vector<TestCaseResult>& test_cases,
                                           const std::string& suite_name);
};

/**
 * @brief Environment detector for CI/container detection
 */
class EnvironmentDetector {
 public:
  /**
   * @brief Detect if running in Docker container
   */
  static bool is_docker_container();

  /**
   * @brief Detect if running in Kubernetes pod
   */
  static bool is_kubernetes_pod();

  /**
   * @brief Detect CI system
   */
  static std::string detect_ci_system();

  /**
   * @brief Check if running in any CI environment
   */
  static bool is_ci_environment();

  /**
   * @brief Detect cloud platform
   */
  static std::string detect_cloud_platform();

  /**
   * @brief Get environment information
   */
  static std::map<std::string, std::string> get_environment_info();
};

/**
 * @brief Memory monitor with enhanced tracking
 */
class EnhancedMemoryMonitor {
 public:
  struct MemoryUsage {
    size_t rss_bytes;        // Resident set size
    size_t virtual_bytes;    // Virtual memory size
    size_t peak_rss_bytes;   // Peak RSS
    size_t available_bytes;  // Available system memory
    double usage_percent;    // Memory usage percentage
  };

  /**
   * @brief Get current memory usage
   */
  static MemoryUsage get_current_usage();

  /**
   * @brief Check if memory is available
   */
  static bool check_memory_available(size_t required_bytes);

  /**
   * @brief Get memory limit (for containers)
   */
  static size_t get_memory_limit();

  /**
   * @brief Detect memory leaks
   */
  static bool detect_memory_leak(size_t baseline_bytes, size_t current_bytes,
                                 double threshold_percent = 10.0);
};

}  // namespace CI
}  // namespace TestUtils

#endif  // CI_UTILITIES_HPP
