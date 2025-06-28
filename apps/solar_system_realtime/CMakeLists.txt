# Solar System Real-Time Application
# Live simulation with continuous time updates

add_executable(solar_system_realtime
    realtime.cpp
)

# Link required libraries
target_link_libraries(solar_system_realtime PRIVATE
    solar_core      # For simulation and model functions
    solar_jpl       # For JPL data integration
    solar_utils     # For argument parsing utilities
    Threads::Threads
)

# Optimization flags (moderate optimization for real-time responsiveness)
target_compile_options(solar_system_realtime PRIVATE
    $<$<CONFIG:Release>:-O2 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system_realtime PRIVATE -pg)
    target_link_options(solar_system_realtime PRIVATE -pg)
endif()

# Set executable properties
set_target_properties(solar_system_realtime PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
