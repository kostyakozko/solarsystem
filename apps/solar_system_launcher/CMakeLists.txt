# Solar System Launcher Application
# Unified command dispatcher and workflow coordinator

add_executable(solar_system_launcher
    launcher.cpp
)

# Link required libraries
target_link_libraries(solar_system_launcher PRIVATE
    solar_jpl       # For status checking and initialization
    solar_utils     # For argument parsing utilities
    Threads::Threads
)

# Optimization flags (moderate optimization for launcher)
target_compile_options(solar_system_launcher PRIVATE
    $<$<CONFIG:Release>:-O2 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system_launcher PRIVATE -pg)
    target_link_options(solar_system_launcher PRIVATE -pg)
endif()

# Set executable properties
set_target_properties(solar_system_launcher PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
