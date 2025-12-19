# Solar Utils Library - Common Utilities
# Supports both static and shared library builds

add_library(solar_utils
    src/argument_parser.cpp
    src/logging.cpp
    src/validation/input_validator.cpp
    src/resource_manager.cpp
    src/file_resource_manager.cpp
    src/network_resource_manager.cpp
    src/error_handling.cpp
    src/error_recovery.cpp
    src/workflow_orchestration.cpp
    src/status_management.cpp
    src/config.cpp
    src/advanced_config.cpp
)

# Link with solar_core for Expected type
target_link_libraries(solar_utils PUBLIC solar_core)

# Public headers for other libraries and applications
target_include_directories(solar_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Standard optimization flags
target_compile_options(solar_utils PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_utils PRIVATE -pg)
    target_link_options(solar_utils PRIVATE -pg)
endif()

# Set library properties
set_target_properties(solar_utils PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Shared library specific properties
if(BUILD_SHARED_LIBS)
    set_target_properties(solar_utils PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
    target_compile_definitions(solar_utils PRIVATE SOLAR_UTILS_EXPORTS)

    # Platform-specific settings
    if(APPLE)
        # macOS: Set install_name for dylib
        set_target_properties(solar_utils PROPERTIES
            INSTALL_NAME_DIR "@rpath"
            BUILD_WITH_INSTALL_NAME_DIR TRUE
        )
    elseif(UNIX)
        # Linux: Set SONAME for .so files
        set_target_properties(solar_utils PROPERTIES
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    elseif(WIN32)
        # Windows: Set output directories for DLLs and import libraries
        set_target_properties(solar_utils PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    endif()
endif()
