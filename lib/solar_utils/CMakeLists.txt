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

# No dependencies on other solar libraries - utils is the base layer
# Link external dependencies needed by utils
target_link_libraries(solar_utils PUBLIC
    CURL::libcurl   # For network operations
    OpenSSL::SSL    # For security operations
    OpenSSL::Crypto # For cryptographic operations
)

# Link nlohmann/json if available
if(NLOHMANN_JSON_AVAILABLE)
    target_link_libraries(solar_utils PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(solar_utils PRIVATE SOLAR_UTILS_HAS_NLOHMANN_JSON=1)
    message(STATUS "solar_utils: nlohmann/json linked for JSON operations")
endif()

# Public headers for other libraries and applications - use generator expressions for proper export
target_include_directories(solar_utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
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
    )
    target_compile_definitions(solar_utils PRIVATE SOLAR_UTILS_EXPORTS)

    # Configure symbol visibility
    if(ENABLE_SYMBOL_VISIBILITY)
        set_target_properties(solar_utils PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN YES
        )
        message(STATUS "solar_utils: Symbol visibility set to hidden")
    else()
        set_target_properties(solar_utils PROPERTIES
            CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN NO
        )
    endif()

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

# Install library
install(TARGETS solar_utils
    EXPORT SolarSystemTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY include/solar_utils
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)


# Generate and install pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/solar_utils.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/solar_utils.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/solar_utils.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
