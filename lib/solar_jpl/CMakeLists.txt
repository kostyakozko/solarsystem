# Solar JPL Library - JPL HORIZONS Data Integration
# Static library for now, shared library option can be added later

add_library(solar_jpl STATIC
    jpl_data.cpp
    jpl_bodies.cpp
)

# Public headers for other libraries and applications
target_include_directories(solar_jpl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(solar_jpl PUBLIC
    solar_core      # Needs constants.h and other core headers
    Threads::Threads
)

# Optimization flags (less aggressive than core, but still optimized)
target_compile_options(solar_jpl PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_jpl PRIVATE -pg)
    target_link_options(solar_jpl PRIVATE -pg)
endif()

# Set library properties
set_target_properties(solar_jpl PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
