# Performance Benchmarks for Solar System Suite

# Benchmark executables
add_executable(benchmark_simulation
    benchmark_simulation.cpp
)

add_executable(benchmark_jpl_data
    benchmark_jpl_data.cpp
)

add_executable(benchmark_web_server
    benchmark_web_server.cpp
)

add_executable(benchmark_comprehensive
    benchmark_comprehensive.cpp
)

# Link libraries
target_link_libraries(benchmark_simulation
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(benchmark_jpl_data
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(benchmark_web_server
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(benchmark_comprehensive
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Add benchmark tests to CTest
add_test(NAME Benchmark_Simulation COMMAND benchmark_simulation)
add_test(NAME Benchmark_JPLData COMMAND benchmark_jpl_data)
add_test(NAME Benchmark_WebServer COMMAND benchmark_web_server)
add_test(NAME Benchmark_Comprehensive COMMAND benchmark_comprehensive)

# Set benchmark properties
set_tests_properties(Benchmark_Simulation PROPERTIES
    LABELS "benchmark;simulation"
    TIMEOUT 300
)

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

# Performance regression tests
add_executable(regression_detector
    regression_detector.cpp
)

target_link_libraries(regression_detector
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

add_test(NAME RegressionTest COMMAND regression_detector)
set_tests_properties(RegressionTest PROPERTIES
    LABELS "benchmark;regression"
    TIMEOUT 300
)

# Scalability tests
add_executable(scalability_tests
    scalability_tests.cpp
)

target_link_libraries(scalability_tests
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

add_test(NAME ScalabilityTest COMMAND scalability_tests)
set_tests_properties(ScalabilityTest PROPERTIES
    LABELS "benchmark;scalability"
    TIMEOUT 600
)

# Create benchmark results directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/benchmark_results)

# Custom targets for different benchmark categories
add_custom_target(benchmark-simulation
    COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark.*simulation" --output-on-failure
    COMMENT "Running simulation benchmarks"
    VERBATIM
)

add_custom_target(benchmark-data
    COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark.*data" --output-on-failure
    COMMENT "Running data processing benchmarks"
    VERBATIM
)

add_custom_target(benchmark-web
    COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark.*web" --output-on-failure
    COMMENT "Running web server benchmarks"
    VERBATIM
)

add_custom_target(benchmark-all
    COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark" --output-on-failure
    COMMENT "Running all benchmarks"
    VERBATIM
)

add_custom_target(regression-check
    COMMAND ${CMAKE_CTEST_COMMAND} -L "regression" --output-on-failure
    COMMENT "Running performance regression tests"
    VERBATIM
)

message(STATUS "Performance benchmarks configured:")
message(STATUS "  - Simulation Performance Tests")
message(STATUS "  - JPL Data Processing Tests")
message(STATUS "  - Web Server Performance Tests")
message(STATUS "  - Comprehensive System Tests")
message(STATUS "  - Performance Regression Detection")
message(STATUS "  - Scalability Analysis")
message(STATUS "  - Results directory: ${CMAKE_CURRENT_BINARY_DIR}/benchmark_results")
