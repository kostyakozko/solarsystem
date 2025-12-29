# Solar System Main Application
# High-performance N-body simulation with modern C++ architecture

# Main solar_system application (modern C++ version - PRODUCTION)
add_executable(solar_system
    solar_system.cpp
)

# Link all required libraries (static for maximum performance)
target_link_libraries(solar_system PRIVATE
    solar_core      # Modern physics engine with SimulationEngine
    solar_jpl       # JPL data integration
    solar_utils     # Command-line utilities
    Threads::Threads
)

# Aggressive optimization for all executables
# Note: -ffast-math removed as it causes issues with JSON parsing (infinity handling)
target_compile_options(solar_system PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -fno-math-errno -fno-trapping-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0>
)

# Enable LTO for maximum performance
set_target_properties(solar_system PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system PRIVATE -pg)
    target_link_options(solar_system PRIVATE -pg)
endif()
