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

# Enable LTO for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Installation configuration
# Check for environment variable first, then CMake option, then default
if(DEFINED ENV{SOLAR_SYSTEM_INSTALL_DIR})
    set(CMAKE_INSTALL_PREFIX "$ENV{SOLAR_SYSTEM_INSTALL_DIR}" CACHE PATH "Install directory" FORCE)
    message(STATUS "Using install directory from environment: ${CMAKE_INSTALL_PREFIX}")
elseif(SOLAR_SYSTEM_INSTALL_DIR)
    set(CMAKE_INSTALL_PREFIX "${SOLAR_SYSTEM_INSTALL_DIR}" CACHE PATH "Install directory" FORCE)
    message(STATUS "Using install directory from CMake option: ${CMAKE_INSTALL_PREFIX}")
else()
    # Default installation directory
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Install directory" FORCE)
        message(STATUS "Using default install directory: ${CMAKE_INSTALL_PREFIX}")
    endif()
endif()

# Installation directories
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_INSTALL_DATADIR "share/solar_system")

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Release optimizations (matching current performance)
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

# Add subdirectories for modular build
add_subdirectory(lib)
add_subdirectory(apps)

# Testing framework (optional)
option(ENABLE_TESTING "Enable comprehensive testing framework" OFF)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Testing framework enabled")
endif()

# Installation targets
install(TARGETS solar_system solar_system_fetch solar_system_launcher solar_system_realtime solar_system_web
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install launcher as main entry point in root directory
install(TARGETS solar_system_launcher
        RUNTIME DESTINATION "."
        RENAME "solar_system_launcher")

# Install documentation
install(FILES README.md
        DESTINATION ${CMAKE_INSTALL_DATADIR})

# Create a post-install script to set up the launcher
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/post_install.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/post_install.cmake"
    @ONLY
)

# Custom target for formatting (equivalent to format-code.sh)
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/**/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/**/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/**/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/**/*.h
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

# Convenience targets
add_custom_target(all-libraries DEPENDS libraries)
add_custom_target(all-applications DEPENDS applications)

# Custom install target with post-processing
add_custom_target(install-suite
    COMMAND ${CMAKE_COMMAND} --build . --target install
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/post_install.cmake"
    COMMENT "Installing Solar System Suite with post-processing"
    VERBATIM
)

# Print build information
message(STATUS "Solar System Suite Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  LTO enabled: ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "  Optimization flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
message(STATUS "  Libraries: solar_core, solar_jpl, solar_utils")
message(STATUS "  Applications: solar_system, solar_system_fetch, solar_system_launcher, solar_system_realtime, solar_system_web")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Install layout:")
message(STATUS "    Launcher: ${CMAKE_INSTALL_PREFIX}/solar_system_launcher")
message(STATUS "    Binaries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/")
message(STATUS "    Libraries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/")
message(STATUS "    Data: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/")
