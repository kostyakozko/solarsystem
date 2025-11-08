# Performance Benchmarks for Solar System Suite

# Benchmark executables (only include existing files)
add_executable(benchmark_jpl_data benchmark_jpl_data.cpp)
target_link_libraries(benchmark_jpl_data test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(benchmark_web_server benchmark_web_server.cpp)
target_link_libraries(benchmark_web_server test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(benchmark_comprehensive benchmark_comprehensive.cpp)
target_link_libraries(benchmark_comprehensive test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(benchmark_core_performance benchmark_core_performance.cpp)
target_link_libraries(benchmark_core_performance test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(regression_detector regression_detector.cpp)
target_link_libraries(regression_detector test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)

add_executable(scalability_tests scalability_tests.cpp)
target_link_libraries(scalability_tests test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)

# Performance Measurement Framework Tests (Task 8)
add_executable(test_performance_measurement_framework test_performance_measurement_framework.cpp)
target_link_libraries(test_performance_measurement_framework test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)

# Add benchmark tests to CTest
add_test(NAME Benchmark_JPLData COMMAND benchmark_jpl_data)
add_test(NAME Benchmark_WebServer COMMAND benchmark_web_server)
add_test(NAME Benchmark_Comprehensive COMMAND benchmark_comprehensive)
add_test(NAME Benchmark_CorePerformance COMMAND benchmark_core_performance)
add_test(NAME Benchmark_RegressionDetector COMMAND regression_detector)
add_test(NAME Benchmark_ScalabilityTests COMMAND scalability_tests)
add_test(NAME Benchmark_PerformanceMeasurementFramework COMMAND test_performance_measurement_framework)

# Set benchmark properties
set_tests_properties(Benchmark_JPLData PROPERTIES
    LABELS "benchmark;data"
    TIMEOUT 180
)

set_tests_properties(Benchmark_WebServer PROPERTIES
    LABELS "benchmark;web"
    TIMEOUT 120
)

set_tests_properties(Benchmark_Comprehensive PROPERTIES
    LABELS "benchmark;comprehensive"
    TIMEOUT 600
)

set_tests_properties(Benchmark_CorePerformance PROPERTIES
    LABELS "benchmark;core;performance"
    TIMEOUT 300
)

set_tests_properties(Benchmark_RegressionDetector PROPERTIES
    LABELS "benchmark;regression"
    TIMEOUT 300
)

set_tests_properties(Benchmark_ScalabilityTests PROPERTIES
    LABELS "benchmark;scalability"
    TIMEOUT 300
)

set_tests_properties(Benchmark_PerformanceMeasurementFramework PROPERTIES
    LABELS "benchmark;performance;measurement;framework;task8;phase3"
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
message(STATUS "  - Results directory: ${CMAKE_CURRENT_BINARY_DIR}/benchmark_results")
