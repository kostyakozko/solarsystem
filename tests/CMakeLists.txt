# Testing Framework for Solar System Suite
# Comprehensive unit tests, integration tests, and benchmarks

cmake_minimum_required(VERSION 3.15)

# Enable testing
enable_testing()

# Find required packages for testing
find_package(Threads REQUIRED)

# ============================================================================
# Google Test Integration
# ============================================================================
# Supports both system-installed Google Test and FetchContent download
# Use -DUSE_SYSTEM_GTEST=ON to use system installation

if(USE_SYSTEM_GTEST)
  # Try to find system-installed Google Test
  find_package(GTest QUIET)
  if(GTest_FOUND)
    message(STATUS "Using system-installed Google Test")
    set(GTEST_AVAILABLE TRUE)
  else()
    message(WARNING "System Google Test not found, falling back to FetchContent")
    set(USE_SYSTEM_GTEST OFF)
  endif()
endif()

if(NOT USE_SYSTEM_GTEST)
  # Download Google Test via FetchContent
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
  )

  # Prevent Google Test from overriding compiler/linker options
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(googletest)

  # Disable strict warnings for Google Test targets (they have their own warnings)
  if(TARGET gtest)
    target_compile_options(gtest PRIVATE -w)
  endif()
  if(TARGET gtest_main)
    target_compile_options(gtest_main PRIVATE -w)
  endif()
  if(TARGET gmock)
    target_compile_options(gmock PRIVATE -w)
  endif()
  if(TARGET gmock_main)
    target_compile_options(gmock_main PRIVATE -w)
  endif()

  message(STATUS "Using FetchContent Google Test v1.14.0")
  set(GTEST_AVAILABLE TRUE)
endif()

# Enable automatic test discovery for Google Test
include(GoogleTest)

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
    utils/test_data_manager.cpp
    utils/benchmark_utils.cpp
    utils/test_port_manager.cpp
    utils/test_diagnostics.cpp
    utils/performance_regression_system_simple.cpp
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
message(STATUS "  Google Test: ${GTEST_AVAILABLE}")
if(USE_SYSTEM_GTEST)
  message(STATUS "    Source: System installation")
else()
  message(STATUS "    Source: FetchContent (v1.14.0)")
endif()
message(STATUS "  Test timeout: ${TEST_TIMEOUT} seconds")
message(STATUS "  Benchmark iterations: ${BENCHMARK_ITERATIONS}")
message(STATUS "  Coverage tools: ${GCOV_EXECUTABLE} ${LCOV_EXECUTABLE} ${GENHTML_EXECUTABLE}")
message(STATUS "  Test categories: unit, integration, benchmarks")
