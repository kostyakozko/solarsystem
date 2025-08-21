# Solar Test Library - Comprehensive Testing Framework
cmake_minimum_required(VERSION 3.15)

# Library configuration
add_library(solar_test STATIC
    src/framework/test_runner.cpp
    src/framework/test_case.cpp
    src/framework/assertions.cpp
    src/framework/test_result.cpp
    src/framework/test_discovery.cpp
    src/framework/test_data_manager.cpp
    src/framework/enhanced_test_data_manager.cpp
    src/reporters/test_reporter.cpp
    src/reporters/console_reporter.cpp
    src/reporters/xml_reporter.cpp
    src/reporters/json_reporter.cpp
    src/reporters/tap_reporter.cpp
    src/reporters/coverage_reporter.cpp
    src/reporters/enhanced_test_reporter.cpp
    src/utils/robust_file_handler.cpp
    src/formatters/output_formatter.cpp
    src/formatters/streaming_output_manager.cpp
    src/formatters/output_validator.cpp
    src/mocks/jpl_mock.cpp
    src/mocks/cache_mock.cpp
    src/mocks/time_mock.cpp
    src/mocks/network_mock.cpp
    src/mocks/mock_service_registry.cpp
    src/benchmarks/benchmark.cpp
    src/benchmarks/performance_monitor.cpp
    src/benchmarks/regression_detector.cpp
    src/benchmarks/ci_integration.cpp
    src/solar_test_main.cpp
)

# Include directories
target_include_directories(solar_test
    PUBLIC
        include
    PRIVATE
        src
)

# C++20 standard
target_compile_features(solar_test PUBLIC cxx_std_20)

# Link dependencies
target_link_libraries(solar_test
    PUBLIC
        solar_core
        solar_utils
        solar_jpl
    PRIVATE
        Threads::Threads
)

# Installation
install(TARGETS solar_test
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)
