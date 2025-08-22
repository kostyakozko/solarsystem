# Solar Utils Library - Common Utilities
# Static library for now, shared library option can be added later

add_library(solar_utils STATIC
    src/argument_parser.cpp
    src/logging.cpp
    src/validation/input_validator.cpp
    src/resource_manager.cpp
    src/file_resource_manager.cpp
    src/network_resource_manager.cpp
    src/error_handling.cpp
    src/error_recovery.cpp
    # src/config.cpp  # Temporarily disabled due to Expected usage issues
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
