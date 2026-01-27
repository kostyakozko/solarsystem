# Solar Core Library - High-Performance Physics Engine with Modern C++20 Architecture
# Supports both static and shared library builds

add_library(solar_core
    # Modern C++20 classes - Pure modern architecture
    src/math/vector3.cpp
    src/bodies/celestial_body.cpp
    src/bodies/body_collection.cpp
    src/bodies/body_factory.cpp
    src/data/body_definitions.cpp
    src/simulation/simulation_engine.cpp
    src/simulation/checkpoint.cpp

    # Modern API design patterns (Phase 0.3)
    src/builders/simulation_builder.cpp

    # Output formatting system (Phase 3 - Task 9)
    src/output/output_formatter.cpp

    # Live data streaming system (Phase 4 - Task 10)
    src/streaming/data_stream.cpp
    src/streaming/realtime_stream.cpp
    src/streaming/stream_filter.cpp
    src/streaming/stream_aggregator.cpp
    src/streaming/quality_monitor.cpp

    # Multiple visualization modes (Phase 4 - Task 11)
    src/visualization/visualization_modes.cpp
    src/visualization/visualization_mode_manager.cpp

    # Robust connection management (Phase 4 - Task 12)
    src/connection/connection_manager.cpp
    src/connection/streaming_connection.cpp

    # Comprehensive security hardening (Phase 5 - Task 13)
    src/security/security_manager.cpp
    src/security/input_sanitizer.cpp
    src/security/session_manager.cpp
    src/security/rate_limiter.cpp
    src/security/security_monitor.cpp
    src/security/security_headers.cpp

    # Performance optimization (Phase 5 - Task 14)
    src/performance/request_handler.cpp
    src/performance/response_cache.cpp
    src/performance/compression.cpp
    src/performance/connection_pool.cpp
    src/performance/load_balancer.cpp

    # Comprehensive API management (Phase 5 - Task 15)
    src/api/api_manager.cpp

    # Unified configuration system (Phase 6 - Task 16)
    src/config/config_manager.cpp

    # Standardized communication protocols (Phase 7 - Task 18)
    src/communication/message.cpp
    src/communication/protocol.cpp

    # Data sharing and synchronization (Phase 7 - Task 19)
    src/data/shared_data_manager.cpp

    # Workflow coordination system (Phase 7 - Task 20)
    src/workflow/workflow_coordinator.cpp

    # Performance monitoring system (Phase 8 - Task 22)
    src/performance/performance_monitor.cpp
    src/performance/notification.cpp

    # Diagnostic and troubleshooting system (Phase 8 - Task 23)
    src/diagnostics/diagnostic_system.cpp

    # File system and network security (Phase 9 - Task 25)
    src/security/file_security.cpp
    src/security/network_security.cpp

    # Data protection and backup systems (Phase 9 - Task 26)
    src/backup/backup_manager.cpp
    src/backup/data_protection.cpp

    # Comprehensive help systems (Phase 10 - Task 27)
    src/help/help_system.cpp

    # Comprehensive error messaging (Phase 10 - Task 29)
    src/error/error_messaging.cpp

    # User-friendly interfaces (Phase 10 - Task 28)
    src/ui/user_interface.cpp
    src/ui/progress_indicator.cpp
    src/ui/status_display.cpp
    src/ui/accessibility.cpp
    src/ui/cli_interface.cpp
)

# Public headers for other libraries and applications - use generator expressions for proper export
target_include_directories(solar_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Find zlib for checkpoint compression
find_package(ZLIB REQUIRED)

# Find Cairo for PNG/PDF export (optional)
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CAIRO cairo)
    if(CAIRO_FOUND)
        message(STATUS "Cairo found - PNG/PDF export will be enabled")
        target_compile_definitions(solar_core PRIVATE HAVE_CAIRO)
        target_include_directories(solar_core PRIVATE ${CAIRO_INCLUDE_DIRS})
    else()
        message(STATUS "Cairo not found - PNG/PDF export will be disabled")
        message(STATUS "To enable PNG/PDF export, install cairo development packages:")
        message(STATUS "  Ubuntu/Debian: sudo apt-get install libcairo2-dev libpng-dev")
        message(STATUS "  Fedora/RHEL: sudo dnf install cairo-devel libpng-devel")
        message(STATUS "  macOS: brew install cairo libpng")
    endif()
endif()

# Link dependencies
target_link_libraries(solar_core PUBLIC
    solar_utils     # Needs args.h for simulation parameters
    Threads::Threads
    ZLIB::ZLIB      # For checkpoint compression and backup compression
    OpenSSL::SSL    # For encryption and secure operations
    OpenSSL::Crypto # For hashing and cryptographic functions
    CURL::libcurl   # For HTTP operations in network security
)

# Add Boost include directories for msgpack (header-only, needed for endian detection)
if(Boost_FOUND)
    target_include_directories(solar_core PRIVATE ${Boost_INCLUDE_DIRS})
    message(STATUS "solar_core: Boost headers linked for msgpack")
endif()

# Link msgpack-cxx if available (for binary message serialization)
if(MSGPACK_AVAILABLE)
    target_link_libraries(solar_core PRIVATE msgpack-cxx)
    target_compile_definitions(solar_core PRIVATE SOLAR_CORE_HAS_MSGPACK=1)
    # Mark msgpack headers as SYSTEM to suppress warnings from third-party code
    get_target_property(MSGPACK_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
    if(MSGPACK_INCLUDE_DIRS)
        target_include_directories(solar_core SYSTEM PRIVATE ${MSGPACK_INCLUDE_DIRS})
    endif()
    message(STATUS "solar_core: msgpack-cxx linked for binary serialization")
endif()

# Link nlohmann/json (required for JSON operations)
if(NLOHMANN_JSON_AVAILABLE)
    target_link_libraries(solar_core PRIVATE nlohmann_json::nlohmann_json)
    message(STATUS "solar_core: nlohmann/json linked for JSON operations")
else()
    message(FATAL_ERROR "solar_core: nlohmann/json is required but not available")
endif()

# solar_jpl dependency handled differently for static vs shared
if(NOT BUILD_SHARED_LIBS)
    # Static: normal linking
    target_link_libraries(solar_core PUBLIC solar_jpl)
else()
    # Shared: only headers, actual linking deferred to applications
    # This breaks the circular dependency
    target_include_directories(solar_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/solar_jpl/include>
    )
    # Allow undefined symbols - they'll be resolved when applications link both libraries
    if(APPLE)
        target_link_options(solar_core PRIVATE "-Wl,-undefined,dynamic_lookup")
    elseif(UNIX)
        target_link_options(solar_core PRIVATE "-Wl,--allow-shlib-undefined")
    endif()
endif()



# Link Cairo if available
if(CAIRO_FOUND)
    target_link_directories(solar_core PUBLIC ${CAIRO_LIBRARY_DIRS})
    target_link_libraries(solar_core PUBLIC ${CAIRO_LIBRARIES})
endif()

# Aggressive optimization for core physics calculations
# Note: -ffast-math removed as it causes issues with JSON parsing (infinity handling)
target_compile_options(solar_core PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -fno-math-errno -fno-trapping-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0>
)

# Disable old-style-cast and double-promotion warnings for message.cpp due to MessagePack library
set_source_files_properties(src/communication/message.cpp PROPERTIES
    COMPILE_FLAGS "-Wno-old-style-cast -Wno-double-promotion"
)

# Disable old-style-cast warnings for security files due to OpenSSL headers
set_source_files_properties(
    src/security/security_manager.cpp
    src/security/session_manager.cpp
    src/security/network_security.cpp
    src/backup/data_protection.cpp
    src/backup/backup_manager.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast"
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_core PRIVATE -pg)
    target_link_options(solar_core PRIVATE -pg)
endif()

# Set library properties
set_target_properties(solar_core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Shared library specific properties
if(BUILD_SHARED_LIBS)
    set_target_properties(solar_core PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    target_compile_definitions(solar_core PRIVATE SOLAR_CORE_EXPORTS)

    # Configure symbol visibility
    if(ENABLE_SYMBOL_VISIBILITY)
        set_target_properties(solar_core PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN YES
        )
        message(STATUS "solar_core: Symbol visibility set to hidden")
    else()
        set_target_properties(solar_core PROPERTIES
            CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN NO
        )
    endif()

    # Platform-specific settings
    if(APPLE)
        # macOS: Set install_name for dylib
        set_target_properties(solar_core PROPERTIES
            INSTALL_NAME_DIR "@rpath"
            BUILD_WITH_INSTALL_NAME_DIR TRUE
        )
    elseif(UNIX)
        # Linux: Set SONAME for .so files
        set_target_properties(solar_core PROPERTIES
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    elseif(WIN32)
        # Windows: Set output directories for DLLs and import libraries
        set_target_properties(solar_core PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    endif()
endif()

# Install library
install(TARGETS solar_core
    EXPORT SolarSystemTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY include/solar_core
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)


# Generate and install pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/solar_core.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/solar_core.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/solar_core.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
