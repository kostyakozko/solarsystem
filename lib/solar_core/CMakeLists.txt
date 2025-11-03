# Solar Core Library - High-Performance Physics Engine with Modern C++20 Architecture
# Static library only for maximum performance and LTO benefits

add_library(solar_core STATIC
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
)

# Public headers for other libraries and applications
target_include_directories(solar_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find zlib for checkpoint compression
find_package(ZLIB REQUIRED)

# Link dependencies
target_link_libraries(solar_core PUBLIC
    solar_utils     # Needs args.h for simulation parameters
    solar_jpl       # Needs JPL data integration for BodyFactory
    Threads::Threads
    ZLIB::ZLIB      # For checkpoint compression and backup compression
    OpenSSL::SSL    # For encryption and secure operations
    OpenSSL::Crypto # For hashing and cryptographic functions
    CURL::libcurl   # For HTTP operations in network security
)

# Aggressive optimization for core physics calculations
target_compile_options(solar_core PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0>
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
