cmake_minimum_required(VERSION 3.15)
project(SolarSystemSuite VERSION 2.0.0 LANGUAGES CXX)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Release optimizations (matching current Makefile)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops")
    
    # Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    
    # Profiling support (matching current -pg flag)
    if(ENABLE_PROFILING)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    endif()
endif()

# Find required packages
find_package(Threads REQUIRED)

# Check for curl (required for JPL data fetching)
find_program(CURL_EXECUTABLE curl REQUIRED)
if(NOT CURL_EXECUTABLE)
    message(FATAL_ERROR "curl is required for JPL HORIZONS data fetching")
endif()

# Source files for the main executable
set(SOLAR_SYSTEM_SOURCES
    solar_system.cpp
    model.cpp
    constants.cpp
    args.cpp
    simulation.cpp
    jpl_data.cpp
    jpl_bodies.cpp
)

# Create the main executable
add_executable(solar_system ${SOLAR_SYSTEM_SOURCES})

# Link threading library
target_link_libraries(solar_system PRIVATE Threads::Threads)

# Include current directory for headers
target_include_directories(solar_system PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Installation
install(TARGETS solar_system
        RUNTIME DESTINATION bin)

# Custom target for formatting (equivalent to format-code.sh)
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_CXX_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
        VERBATIM
    )
endif()

# Custom target for cleaning cache files
add_custom_target(clean-cache
    COMMAND ${CMAKE_COMMAND} -E remove -f ephemeris_cache.bin ephemeris_data.json
    COMMENT "Cleaning ephemeris cache files"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Print build information
message(STATUS "Solar System Suite Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "  Optimization flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
