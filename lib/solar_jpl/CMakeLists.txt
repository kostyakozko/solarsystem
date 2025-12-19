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

# Include directories
target_include_directories(solar_jpl PUBLIC
    include

)

# Link dependencies
target_link_libraries(solar_jpl PUBLIC
    solar_core      # For celestial body types
    Threads::Threads  # For async operations
)

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
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
    target_compile_definitions(solar_jpl PRIVATE SOLAR_JPL_EXPORTS)

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
endif()

# Install library
install(TARGETS solar_jpl
    EXPORT SolarSystemTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
