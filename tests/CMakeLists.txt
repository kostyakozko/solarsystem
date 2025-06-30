# Testing Framework for Solar System Suite
# Comprehensive unit tests, integration tests, and benchmarks

cmake_minimum_required(VERSION 3.15)

# Enable testing
enable_testing()

# Find required packages for testing
find_package(Threads REQUIRED)

# Test configuration
set(TEST_TIMEOUT 30 CACHE STRING "Test timeout in seconds")
set(BENCHMARK_ITERATIONS 1000 CACHE STRING "Number of benchmark iterations")

# Include directories for tests
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/solar_core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/solar_jpl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/solar_utils)

# Test utilities library
add_library(test_utils STATIC
    utils/test_framework.cpp
    utils/test_data.cpp
    utils/benchmark_utils.cpp
)

target_include_directories(test_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Unit Tests
add_subdirectory(unit)

# Integration Tests
add_subdirectory(integration)

# Performance Benchmarks
add_subdirectory(benchmarks)

# Test data generation
add_subdirectory(data)

# Custom targets for different test categories
add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -L "unit" --output-on-failure
    COMMENT "Running unit tests"
    VERBATIM
)

add_custom_target(test-integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L "integration" --output-on-failure
    COMMENT "Running integration tests"
    VERBATIM
)

add_custom_target(test-benchmarks
    COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark" --output-on-failure
    COMMENT "Running performance benchmarks"
    VERBATIM
)

add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running all tests"
    VERBATIM
)

# Test coverage (if available)
find_program(GCOV_EXECUTABLE gcov)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(GCOV_EXECUTABLE AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    add_custom_target(coverage
        COMMAND ${LCOV_EXECUTABLE} --directory . --capture --output-file coverage.info
        COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' --output-file coverage.info
        COMMAND ${LCOV_EXECUTABLE} --list coverage.info
        COMMAND ${GENHTML_EXECUTABLE} coverage.info --output-directory coverage_html
        COMMENT "Generating test coverage report"
        VERBATIM
    )
endif()

# Continuous Integration helper
add_custom_target(ci-test
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout ${TEST_TIMEOUT}
    COMMENT "Running CI test suite"
    VERBATIM
)

# Test installation verification
add_custom_target(test-install
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_installation.cmake
    COMMENT "Testing installation integrity"
    VERBATIM
)

message(STATUS "Testing Framework Configuration:")
message(STATUS "  Test timeout: ${TEST_TIMEOUT} seconds")
message(STATUS "  Benchmark iterations: ${BENCHMARK_ITERATIONS}")
message(STATUS "  Coverage tools: ${GCOV_EXECUTABLE} ${LCOV_EXECUTABLE} ${GENHTML_EXECUTABLE}")
message(STATUS "  Test categories: unit, integration, benchmarks")
