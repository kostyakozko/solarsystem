#pragma once

/**
 * @file solar_test.hpp
 * @brief Main header for Solar System Testing Framework
 *
 * This header provides a comprehensive testing framework for the Solar System Suite,
 * including unit testing, integration testing, performance benchmarking, and mocking capabilities.
 */

// Core testing framework
#include "framework/assertions.hpp"
#include "framework/test_case.hpp"
#include "framework/test_result.hpp"
#include "framework/test_runner.hpp"

// Reporting framework
#include "reporters/console_reporter.hpp"
#include "reporters/coverage_reporter.hpp"
#include "reporters/json_reporter.hpp"
#include "reporters/tap_reporter.hpp"
#include "reporters/test_reporter.hpp"
#include "reporters/xml_reporter.hpp"

// Mock framework
#include "mocks/cache_mock.hpp"
#include "mocks/jpl_mock.hpp"
#include "mocks/mock_service_registry.hpp"
#include "mocks/network_mock.hpp"
#include "mocks/time_mock.hpp"

// Benchmark framework
#include "benchmarks/benchmark.hpp"
#include "benchmarks/performance_monitor.hpp"
#include "benchmarks/regression_detector.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Main entry point for running tests
 *
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return Exit code (0 for success, non-zero for failure)
 */
int run_tests(int argc, char* argv[]);

/**
 * @brief Create a test runner with default configuration
 *
 * @return Configured test runner
 */
std::unique_ptr<TestRunner> create_default_test_runner();

/**
 * @brief Parse command line arguments for test configuration
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return Test runner configuration
 */
TestRunner::Configuration parse_test_arguments(int argc, char* argv[]);

}  // namespace SolarSystem::Testing

// Convenience macros for common testing patterns
#define SOLAR_TEST_MAIN() \
  int main(int argc, char* argv[]) { return SolarSystem::Testing::run_tests(argc, argv); }
