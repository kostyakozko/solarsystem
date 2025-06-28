# Solar System Fetch Application
# Dedicated JPL HORIZONS data fetching and management utility

add_executable(solar_system_fetch
    fetch.cpp
)

# Link required libraries
target_link_libraries(solar_system_fetch PRIVATE
    solar_jpl       # JPL data integration
    solar_utils     # Command-line utilities
    Threads::Threads
)

# Optimization flags (less aggressive than core simulation)
target_compile_options(solar_system_fetch PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system_fetch PRIVATE -pg)
    target_link_options(solar_system_fetch PRIVATE -pg)
endif()

# Set executable properties
set_target_properties(solar_system_fetch PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
