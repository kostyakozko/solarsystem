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
    src/mocks/jpl_mock.cpp
    src/mocks/cache_mock.cpp
    src/mocks/time_mock.cpp
    src/mocks/network_mock.cpp
    src/mocks/mock_service_registry.cpp
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

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(solar_test PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
endif()

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
