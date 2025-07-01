# Legacy solar_system application
add_executable(solar_system
    solar_system.cpp
)

# Modern solar_system application (demonstrates new architecture)
add_executable(solar_system_modern
    solar_system_modern.cpp
)

# Fixed modern solar_system application (matches legacy behavior exactly)
add_executable(solar_system_modern_fixed
    solar_system_modern_fixed.cpp
)

# Link all required libraries (static for maximum performance)
target_link_libraries(solar_system PRIVATE
    solar_core      # Physics engine
    solar_jpl       # JPL data integration
    solar_utils     # Command-line utilities
    Threads::Threads
)

target_link_libraries(solar_system_modern PRIVATE
    solar_core      # Modern physics engine with SimulationEngine
    solar_jpl       # JPL data integration
    solar_utils     # Command-line utilities
    Threads::Threads
)

target_link_libraries(solar_system_modern_fixed PRIVATE
    solar_core      # Modern physics engine with SimulationEngine
    solar_jpl       # JPL data integration
    solar_utils     # Command-line utilities
    Threads::Threads
)

# Aggressive optimization for main executables
target_compile_options(solar_system PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

target_compile_options(solar_system_modern PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

target_compile_options(solar_system_modern_fixed PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable LTO for maximum performance
set_target_properties(solar_system PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system PRIVATE -pg)
    target_link_options(solar_system PRIVATE -pg)
endif()
