# Modern Solar JPL Library
# C++20 JPL HORIZONS API client with async support and type safety
# Supports both static and shared library builds

# Create modern JPL library
add_library(solar_jpl
    src/jpl_client.cpp
    src/cache_manager.cpp
    src/data_validator.cpp
    src/cache_recovery.cpp
)

# Set modern C++20 properties
set_target_properties(solar_jpl PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

# Include directories - use generator expressions for proper export
target_include_directories(solar_jpl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(solar_jpl PUBLIC
    solar_core      # For celestial body types
    Threads::Threads  # For async operations
)

# Link nlohmann/json (required for JSON operations)
if(NLOHMANN_JSON_AVAILABLE)
    target_link_libraries(solar_jpl PRIVATE nlohmann_json::nlohmann_json)
    message(STATUS "solar_jpl: nlohmann/json linked for JSON operations")
else()
    message(FATAL_ERROR "solar_jpl: nlohmann/json is required but not available")
endif()

# Link zlib (required for cache compression)
find_package(ZLIB REQUIRED)
target_link_libraries(solar_jpl PRIVATE ZLIB::ZLIB)

# Modern C++20 optimization flags
target_compile_options(solar_jpl PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto>
    $<$<CONFIG:Debug>:-g -O0>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_jpl PRIVATE -pg)
    target_link_options(solar_jpl PRIVATE -pg)
endif()

# Create alias for consistent naming
add_library(SolarSystem::JPL::Modern ALIAS solar_jpl)

# Test executable for modern JPL client
add_executable(test_modern_jpl
    test_modern_jpl.cpp
)

target_link_libraries(test_modern_jpl PRIVATE
    solar_jpl
)

set_target_properties(test_modern_jpl PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Install headers
install(DIRECTORY include/solar_jpl
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Shared library specific properties
if(BUILD_SHARED_LIBS)
    set_target_properties(solar_jpl PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    target_compile_definitions(solar_jpl PRIVATE SOLAR_JPL_EXPORTS)

    # Configure symbol visibility
    if(ENABLE_SYMBOL_VISIBILITY)
        set_target_properties(solar_jpl PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN YES
        )
        message(STATUS "solar_jpl: Symbol visibility set to hidden")
    else()
        set_target_properties(solar_jpl PROPERTIES
            CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN NO
        )
    endif()

    # Platform-specific settings
    if(APPLE)
        # macOS: Set install_name for dylib
        set_target_properties(solar_jpl PROPERTIES
            INSTALL_NAME_DIR "@rpath"
            BUILD_WITH_INSTALL_NAME_DIR TRUE
        )
    elseif(UNIX)
        # Linux: Set SONAME for .so files
        set_target_properties(solar_jpl PROPERTIES
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    elseif(WIN32)
        # Windows: Set output directories for DLLs and import libraries
        set_target_properties(solar_jpl PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    endif()
else()
    # Static library: define STATIC_DEFINE for export headers
    target_compile_definitions(solar_jpl PUBLIC SOLAR_JPL_STATIC_DEFINE)
endif()

# Install library
install(TARGETS solar_jpl
    EXPORT SolarSystemTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)


# Generate and install pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/solar_jpl.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/solar_jpl.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/solar_jpl.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
