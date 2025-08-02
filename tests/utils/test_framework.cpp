/**
 * @file test_framework.cpp
 * @brief Implementation of lightweight testing framework
 */

#include "test_framework.h"

#include <sys/stat.h>

#include <fstream>
#include <iomanip>
#include <random>

// Global test suite instance
TestSuite* current_suite = nullptr;

TestSuite::TestSuite(const std::string& name) : suite_name(name), total_tests(0), passed_tests(0) {
  std::cout << "\n=== Running Test Suite: " << suite_name << " ===" << std::endl;

  // Create isolated test environment
  TestUtils::TestEnvironmentIsolation::EnvironmentConfig config;
  config.test_name = suite_name;
  test_env_ = TestUtils::TestEnvironmentIsolation::create_environment(config);

  // Configure diagnostic logger
  TestUtils::TestDiagnosticLogger::Config log_config;
  log_config.log_file_prefix = "test_" + suite_name;
  TestUtils::TestDiagnosticLogger::instance().configure(log_config);
}

TestSuite::~TestSuite() { print_summary(); }

void TestSuite::run_test(const std::string& test_name, std::function<void()> test_func) {
  total_tests++;

  // Start diagnostic logging and performance monitoring
  TestUtils::TestDiagnosticLogger::instance().test_started(test_name, suite_name);
  TestUtils::TestPerformanceMonitor::start_monitoring(test_name);

  auto start = std::chrono::high_resolution_clock::now();

  try {
    test_func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    results.push_back({test_name, true, "PASSED", static_cast<double>(duration.count()) / 1000.0});
    passed_tests++;

    std::cout << "✓ " << test_name << " (" << std::fixed << std::setprecision(2)
              << static_cast<double>(duration.count()) / 1000.0 << " ms)" << std::endl;

    // Log successful completion
    TestUtils::TestDiagnosticLogger::instance().test_completed(test_name, true);

  } catch (const std::exception& e) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    results.push_back({test_name, false, e.what(), static_cast<double>(duration.count()) / 1000.0});

    std::cout << "✗ " << test_name << " (" << std::fixed << std::setprecision(2)
              << static_cast<double>(duration.count()) / 1000.0 << " ms)" << std::endl;
    std::cout << "  Error: " << e.what() << std::endl;

    // Log failure with detailed diagnostics
    TestUtils::TestDiagnosticLogger::instance().test_completed(test_name, false, e.what());

    // Generate failure analysis
    auto failure_analysis =
        TestUtils::TestDiagnosticLogger::instance().generate_failure_analysis(test_name);
    if (!failure_analysis.empty()) {
      std::cout << "  Analysis: " << failure_analysis.substr(0, 200) << "..." << std::endl;
    }
  }

  // Stop performance monitoring
  auto perf_metrics = TestUtils::TestPerformanceMonitor::stop_monitoring(test_name);
}

void TestSuite::print_summary() {
  std::cout << "\n=== Test Suite Summary: " << suite_name << " ===" << std::endl;
  std::cout << "Total tests: " << total_tests << std::endl;
  std::cout << "Passed: " << passed_tests << std::endl;
  std::cout << "Failed: " << (total_tests - passed_tests) << std::endl;

  if (total_tests > 0) {
    double success_rate = static_cast<double>(passed_tests) / total_tests * 100.0;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%"
              << std::endl;
  }

  // Calculate total duration
  double total_duration = 0.0;
  for (const auto& result : results) {
    total_duration += result.duration_ms;
  }
  std::cout << "Total duration: " << std::fixed << std::setprecision(2) << total_duration << " ms"
            << std::endl;

  if (total_tests - passed_tests > 0) {
    std::cout << "\nFailed tests:" << std::endl;
    for (const auto& result : results) {
      if (!result.passed) {
        std::cout << "  - " << result.name << ": " << result.message << std::endl;
      }
    }
  }

  std::cout << std::string(50, '=') << std::endl;
}

bool TestSuite::all_passed() const { return passed_tests == total_tests; }

int TestSuite::get_failed_count() const { return total_tests - passed_tests; }

TestUtils::ScopedPortAllocation TestSuite::allocate_port() {
  if (test_env_) {
    return test_env_->allocate_port();
  }
  return TestUtils::ScopedPortAllocation(suite_name);
}

std::string TestSuite::create_temp_file(const std::string& content) {
  if (test_env_) {
    return test_env_->create_temp_file(content);
  }
  return "";
}

std::string TestSuite::create_temp_directory() {
  if (test_env_) {
    return test_env_->create_temp_directory();
  }
  return "";
}

// TestUtils implementation
namespace TestUtils {

bool nearly_equal(double a, double b, double rel_tolerance) {
  if (std::abs(a - b) < 1e-15) return true;  // Handle exact zeros

  double abs_a = std::abs(a);
  double abs_b = std::abs(b);
  double largest = (abs_a > abs_b) ? abs_a : abs_b;

  return std::abs(a - b) <= rel_tolerance * largest;
}

bool vectors_equal(const std::vector<double>& a, const std::vector<double>& b, double tolerance) {
  if (a.size() != b.size()) return false;

  for (size_t i = 0; i < a.size(); ++i) {
    if (std::abs(a[i] - b[i]) > tolerance) {
      return false;
    }
  }
  return true;
}

std::string format_double(double value, int precision) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << value;
  return oss.str();
}

std::string format_vector(const std::vector<double>& vec) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << format_double(vec[i], 3);
  }
  oss << "]";
  return oss.str();
}

std::vector<double> generate_test_positions(int count, double range) {
  std::vector<double> positions;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(-range, range);

  for (int i = 0; i < count; ++i) {
    positions.push_back(dis(gen));
  }
  return positions;
}

std::vector<double> generate_test_velocities(int count, double range) {
  std::vector<double> velocities;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(-range, range);

  for (int i = 0; i < count; ++i) {
    velocities.push_back(dis(gen));
  }
  return velocities;
}

bool file_exists(const std::string& filename) {
  std::ifstream file(filename);
  return file.good();
}

bool create_test_directory(const std::string& dirname) {
  return mkdir(dirname.c_str(), 0755) == 0 || errno == EEXIST;
}

void cleanup_test_files(const std::vector<std::string>& filenames) {
  for (const auto& filename : filenames) {
    std::remove(filename.c_str());
  }
}

}  // namespace TestUtils
