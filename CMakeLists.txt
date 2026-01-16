cmake_minimum_required(VERSION 3.15)

# Configure compiler selection BEFORE project() declaration
include(cmake/CompilerSelection.cmake)
configure_solar_system_compiler()

project(SolarSystemSuite VERSION 4.0.0 LANGUAGES CXX)

# Project versioning for shared libraries
set(PROJECT_VERSION_MAJOR 4)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

# Modern C++20 standard for Phase 0 architecture refactoring
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set installation directories
if(NOT CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif()

# RPATH configuration for shared libraries
if(BUILD_SHARED_LIBS)
    # Set RPATH for installed binaries
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    # For development builds
    set(CMAKE_BUILD_RPATH "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    # Platform-specific RPATH settings
    if(APPLE)
        # macOS: Use @rpath for dynamic library loading
        set(CMAKE_MACOSX_RPATH TRUE)
        set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    elseif(UNIX)
        # Linux: Use $ORIGIN for relative RPATH
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
        # Enable RUNPATH instead of RPATH for better security
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-new-dtags")
    elseif(WIN32)
        # Windows: DLLs must be in PATH or same directory as executable
        # Copy DLLs to bin directory during build
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    endif()

    message(STATUS "Shared library build enabled")
    message(STATUS "  Install RPATH: ${CMAKE_INSTALL_RPATH}")
    message(STATUS "  Build RPATH: ${CMAKE_BUILD_RPATH}")
    if(WIN32)
        message(STATUS "  Windows DLL output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()
endif()

# Include compiler warnings configuration
include(cmake/CompilerWarnings.cmake)

# Include export header generation
include(cmake/GenerateExportHeader.cmake)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Build system modernization options
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)
option(USE_SYSTEM_GTEST "Use system-installed Google Test instead of FetchContent" OFF)
option(USE_SYSTEM_JSON "Use system-installed nlohmann/json instead of FetchContent" OFF)
option(ENABLE_SYMBOL_VISIBILITY "Enable symbol visibility control for shared libraries" ON)

# Compiler warnings configuration - enabled by default
option(WARNINGS_AS_ERRORS "Treat compilation warnings as errors (linking warnings ignored)" ON)

# Enable LTO for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Workaround for GCC 13+ false positive warnings (affects both Debug and Release with -Werror)
# These occur in standard library and Google Test code
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
        message(STATUS "Applying GCC 13+ warning workarounds")
        add_compile_options(-Wno-null-dereference -Wno-float-conversion -Wno-conversion)
        add_link_options(-Wno-null-dereference -Wno-float-conversion -Wno-conversion)
    endif()
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
    # Note: -ffast-math removed as it causes issues with JSON parsing (infinity handling)
    # Using -fno-math-errno and -fno-trapping-math for safe optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -flto -fno-math-errno -fno-trapping-math -funroll-loops")

    # Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

    # Profiling support (matching current -pg flag)
    if(ENABLE_PROFILING)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    endif()
endif()

# Configure warnings as errors BEFORE adding subdirectories
configure_solar_system_warnings()

# Find required packages
find_package(Threads REQUIRED)

# Find OpenSSL (required for security features)
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
    message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
else()
    message(FATAL_ERROR "OpenSSL is required for security features. Install with:\n"
                        "  macOS: brew install openssl@3\n"
                        "  Ubuntu: sudo apt-get install libssl-dev\n"
                        "  Fedora: sudo dnf install openssl-devel")
endif()

# Find libcurl (required for HTTP operations)
find_package(CURL REQUIRED)
if(CURL_FOUND)
    message(STATUS "libcurl found: ${CURL_VERSION_STRING}")
    message(STATUS "libcurl include: ${CURL_INCLUDE_DIRS}")
    message(STATUS "libcurl libraries: ${CURL_LIBRARIES}")
else()
    message(FATAL_ERROR "libcurl is required for HTTP operations. Install with:\n"
                        "  macOS: brew install curl\n"
                        "  Ubuntu: sudo apt-get install libcurl4-openssl-dev\n"
                        "  Fedora: sudo dnf install libcurl-devel")
endif()

# Find zlib (required for compression)
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    message(STATUS "zlib found: ${ZLIB_VERSION_STRING}")
    message(STATUS "zlib include: ${ZLIB_INCLUDE_DIRS}")
    message(STATUS "zlib libraries: ${ZLIB_LIBRARIES}")
else()
    message(FATAL_ERROR "zlib is required for compression. Install with:\n"
                        "  macOS: brew install zlib\n"
                        "  Ubuntu: sudo apt-get install zlib1g-dev\n"
                        "  Fedora: sudo dnf install zlib-devel")
endif()

# Check for curl executable (required for JPL data fetching)
find_program(CURL_EXECUTABLE curl REQUIRED)
if(NOT CURL_EXECUTABLE)
    message(FATAL_ERROR "curl executable is required for JPL HORIZONS data fetching")
endif()

# Find Boost (required for msgpack headers - endian detection)
find_package(Boost REQUIRED)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Boost is required for msgpack serialization. Install with:\n"
                        "  macOS: brew install boost\n"
                        "  Ubuntu: sudo apt-get install libboost-dev\n"
                        "  Fedora: sudo dnf install boost-devel")
endif()

# ============================================================================
# nlohmann/json Integration
# ============================================================================
# Supports both system-installed nlohmann/json and FetchContent download
# Use -DUSE_SYSTEM_JSON=ON to use system installation

if(USE_SYSTEM_JSON)
  # Try to find system-installed nlohmann/json
  find_package(nlohmann_json QUIET)
  if(nlohmann_json_FOUND)
    message(STATUS "Using system-installed nlohmann/json")
    set(NLOHMANN_JSON_AVAILABLE TRUE)
  else()
    message(WARNING "System nlohmann/json not found, falling back to FetchContent")
    set(USE_SYSTEM_JSON OFF)
  endif()
endif()

if(NOT USE_SYSTEM_JSON)
  # Download nlohmann/json via FetchContent
  include(FetchContent)

  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
  )

  # Configure nlohmann/json options
  set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
  set(JSON_Install ON CACHE BOOL "" FORCE)  # Enable installation
  set(JSON_MultipleHeaders OFF CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(nlohmann_json)

  message(STATUS "Using FetchContent nlohmann/json v3.11.3")
  set(NLOHMANN_JSON_AVAILABLE TRUE)
endif()

if(NLOHMANN_JSON_AVAILABLE)
  message(STATUS "nlohmann/json: Available for JSON operations")
else()
  message(WARNING "nlohmann/json: Not available - JSON operations may be limited")
endif()

# ============================================================================
# msgpack-c Integration
# ============================================================================
# Supports both system-installed msgpack-c and FetchContent download
# Use -DUSE_SYSTEM_MSGPACK=ON to use system installation
# msgpack-c is used for binary message serialization in communication protocols

option(USE_SYSTEM_MSGPACK "Use system-installed msgpack-c instead of FetchContent" OFF)

if(USE_SYSTEM_MSGPACK)
  # Try to find system-installed msgpack-c
  find_package(msgpack-cxx QUIET)
  if(msgpack-cxx_FOUND)
    message(STATUS "Using system-installed msgpack-c")
    set(MSGPACK_AVAILABLE TRUE)
  else()
    message(WARNING "System msgpack-c not found, falling back to FetchContent")
    set(USE_SYSTEM_MSGPACK OFF)
  endif()
endif()

if(NOT USE_SYSTEM_MSGPACK)
  # Download msgpack-c via FetchContent
  FetchContent_Declare(
    msgpack-cxx
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG        cpp-7.0.0
    GIT_SHALLOW    TRUE
  )

  # Configure msgpack-c options
  set(MSGPACK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(MSGPACK_CXX20 ON CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(msgpack-cxx)

  message(STATUS "Using FetchContent msgpack-c cpp-7.0.0")
  set(MSGPACK_AVAILABLE TRUE)
endif()

if(MSGPACK_AVAILABLE)
  message(STATUS "msgpack-c: Available for binary serialization")
else()
  message(WARNING "msgpack-c: Not available - binary serialization may be limited")
endif()

# Add subdirectories for modular build
add_subdirectory(lib)

# Generate export headers for shared library builds
if(BUILD_SHARED_LIBS AND ENABLE_SYMBOL_VISIBILITY)
    generate_library_export_header(solar_core)
    generate_library_export_header(solar_jpl)
    generate_library_export_header(solar_utils)
    message(STATUS "Export headers generated for shared library build")
endif()

# For shared libraries, create a combined library to avoid circular dependencies
if(BUILD_SHARED_LIBS)
    # Create solar_system_combined that includes both solar_core and solar_jpl
    add_library(solar_system_combined INTERFACE)
    target_link_libraries(solar_system_combined INTERFACE solar_jpl solar_core)

    message(STATUS "Shared library build: Applications will link solar_system_combined")
endif()

add_subdirectory(apps)
add_subdirectory(examples)

# Testing framework (optional)
option(ENABLE_TESTING "Enable comprehensive testing framework" OFF)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Testing framework enabled")
endif()

# Installation targets (conditional on target existence)
if(TARGET solar_system)
    install(TARGETS solar_system RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(TARGET solar_system_fetch)
    install(TARGETS solar_system_fetch RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(TARGET solar_system_launcher)
    install(TARGETS solar_system_launcher RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    # Install launcher as main entry point in root directory
    install(TARGETS solar_system_launcher
            RUNTIME DESTINATION "."
            RENAME "solar_system_launcher")
endif()

if(TARGET solar_system_realtime)
    install(TARGETS solar_system_realtime RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(TARGET solar_system_web)
    install(TARGETS solar_system_web RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

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

# Create symlinks for executables in build root for testing
# This target runs after all applications are built
add_custom_target(create-symlinks ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        apps/solar_system/solar_system solar_system
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        apps/solar_system_web/solar_system_web solar_system_web
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        apps/solar_system_launcher/solar_system_launcher solar_system_launcher
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        apps/solar_system_fetch/solar_system_fetch solar_system_fetch
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        apps/solar_system_realtime/solar_system_realtime solar_system_realtime
    DEPENDS applications
    COMMENT "Creating symlinks for executables in build root"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom install target with post-processing
add_custom_target(install-suite
    COMMAND ${CMAKE_COMMAND} --build . --target install
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/post_install.cmake"
    COMMENT "Installing Solar System Suite with post-processing"
    VERBATIM
)

# =============================================================================
# CMake Package Configuration
# =============================================================================
# This enables other projects to use find_package(SolarSystem) to find and
# link against the Solar System Suite libraries.

include(CMakePackageConfigHelpers)

# Generate the config file from template
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SolarSystemConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SolarSystem"
)

# Generate the version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SolarSystem"
)

# Export targets for installation
install(EXPORT SolarSystemTargets
    FILE SolarSystemTargets.cmake
    NAMESPACE SolarSystem::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SolarSystem"
)

# Print build information
display_compiler_info()
message(STATUS "Solar System Suite Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Warnings as errors: ${WARNINGS_AS_ERRORS}")
message(STATUS "  LTO enabled: ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "  Optimization flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
message(STATUS "  Build system modernization:")
message(STATUS "    Shared libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "    System Google Test: ${USE_SYSTEM_GTEST}")
message(STATUS "    System nlohmann/json: ${USE_SYSTEM_JSON}")
message(STATUS "    Symbol visibility: ${ENABLE_SYMBOL_VISIBILITY}")
message(STATUS "  Libraries: solar_core, solar_jpl, solar_utils")
message(STATUS "  Applications: solar_system, solar_system_fetch, solar_system_launcher, solar_system_realtime, solar_system_web")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Install layout:")
message(STATUS "    Launcher: ${CMAKE_INSTALL_PREFIX}/solar_system_launcher")
message(STATUS "    Binaries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/")
message(STATUS "    Libraries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/")
message(STATUS "    Data: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/")
