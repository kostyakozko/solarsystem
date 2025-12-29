# Performance Benchmarks for Solar System Suite
# Using Google Test framework for benchmark tests
# Note: These tests use GTest::gtest (not gtest_main) because they have
# custom main() functions using the legacy TEST_SUITE/TEST_CASE pattern.
# They will be migrated to native Google Test in Phase 4 (tasks 8-10).

# Benchmark executables (only include existing files)
add_executable(benchmark_jpl_data benchmark_jpl_data.cpp)
target_link_libraries(benchmark_jpl_data GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(benchmark_web_server benchmark_web_server.cpp)
target_link_libraries(benchmark_web_server GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(benchmark_comprehensive benchmark_comprehensive.cpp)
target_link_libraries(benchmark_comprehensive GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(benchmark_core_performance benchmark_core_performance.cpp)
target_link_libraries(benchmark_core_performance GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(regression_detector regression_detector.cpp)
target_link_libraries(regression_detector GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(scalability_tests scalability_tests.cpp)
target_link_libraries(scalability_tests GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

# Performance Measurement Framework Tests (Task 8)
add_executable(test_performance_measurement_framework test_performance_measurement_framework.cpp)
target_link_libraries(test_performance_measurement_framework GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

# Baseline Management System Tests (Task 9)
add_executable(test_baseline_management_system test_baseline_management_system.cpp)
target_link_libraries(test_baseline_management_system GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

# Optimization Guidance System Tests (Task 10)
add_executable(test_optimization_guidance_system test_optimization_guidance_system.cpp)
target_link_libraries(test_optimization_guidance_system GTest::gtest GTest::gmock solar_test solar_core solar_jpl solar_utils Threads::Threads)

# ============================================================================
# Register Benchmark Tests with CTest
# ============================================================================
# Note: These tests use add_test instead of gtest_discover_tests because they
# have custom main() functions using the legacy TEST_SUITE/TEST_CASE pattern.

add_test(NAME Benchmark_JPLData COMMAND benchmark_jpl_data)
set_tests_properties(Benchmark_JPLData PROPERTIES
    LABELS "benchmark;data"
    TIMEOUT 180
)

add_test(NAME Benchmark_WebServer COMMAND benchmark_web_server)
set_tests_properties(Benchmark_WebServer PROPERTIES
    LABELS "benchmark;web"
    TIMEOUT 120
)

add_test(NAME Benchmark_Comprehensive COMMAND benchmark_comprehensive)
set_tests_properties(Benchmark_Comprehensive PROPERTIES
    LABELS "benchmark;comprehensive"
    TIMEOUT 600
)

add_test(NAME Benchmark_CorePerformance COMMAND benchmark_core_performance)
set_tests_properties(Benchmark_CorePerformance PROPERTIES
    LABELS "benchmark;core;performance"
    TIMEOUT 300
)

add_test(NAME Benchmark_RegressionDetector COMMAND regression_detector)
set_tests_properties(Benchmark_RegressionDetector PROPERTIES
    LABELS "benchmark;regression"
    TIMEOUT 300
)

add_test(NAME Benchmark_ScalabilityTests COMMAND scalability_tests)
set_tests_properties(Benchmark_ScalabilityTests PROPERTIES
    LABELS "benchmark;scalability"
    TIMEOUT 300
)

add_test(NAME Benchmark_PerformanceMeasurementFramework COMMAND test_performance_measurement_framework)
set_tests_properties(Benchmark_PerformanceMeasurementFramework PROPERTIES
    LABELS "benchmark;performance;measurement;framework;task8;phase3"
    TIMEOUT 300
)

add_test(NAME Benchmark_BaselineManagementSystem COMMAND test_baseline_management_system)
set_tests_properties(Benchmark_BaselineManagementSystem PROPERTIES
    LABELS "benchmark;baseline;management;regression;task9;phase3"
    TIMEOUT 300
)

add_test(NAME Benchmark_OptimizationGuidanceSystem COMMAND test_optimization_guidance_system)
set_tests_properties(Benchmark_OptimizationGuidanceSystem PROPERTIES
    LABELS "benchmark;optimization;guidance;recommendations;task10;phase3"
    TIMEOUT 300
)

# Create benchmark results directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/benchmark_results)

message(STATUS "Performance benchmarks configured:")
message(STATUS "  - JPL Data Processing Tests")
message(STATUS "  - Web Server Performance Tests")
message(STATUS "  - Comprehensive System Tests")
message(STATUS "  - Core Performance Validation Tests")
message(STATUS "  - Performance Regression Detection")
message(STATUS "  - Scalability Analysis")
message(STATUS "  - Performance Measurement Framework Tests (Task 8)")
message(STATUS "  - Baseline Management System Tests (Task 9)")
message(STATUS "  - Optimization Guidance System Tests (Task 10)")
message(STATUS "  - Results directory: ${CMAKE_CURRENT_BINARY_DIR}/benchmark_results")
