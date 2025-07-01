# Solar System Fetch Application
# Dedicated JPL HORIZONS data fetching and management utility

# Legacy version (original implementation)
add_executable(solar_system_fetch
    fetch.cpp
)

# Modern version (C++20 with fluent APIs and modern patterns)
add_executable(solar_system_fetch_modern
    fetch_modern.cpp
)

# Link required libraries for legacy version
target_link_libraries(solar_system_fetch PRIVATE
    solar_jpl       # JPL data integration
    solar_utils     # Command-line utilities
    Threads::Threads
)

# Link required libraries for modern version
target_link_libraries(solar_system_fetch_modern PRIVATE
    solar_core      # Modern builders and patterns
    solar_jpl       # JPL data integration  
    solar_utils     # Configuration and logging
    Threads::Threads
)

# Optimization flags for legacy version
target_compile_options(solar_system_fetch PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Optimization flags for modern version
target_compile_options(solar_system_fetch_modern PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system_fetch PRIVATE -pg)
    target_link_options(solar_system_fetch PRIVATE -pg)
    target_compile_options(solar_system_fetch_modern PRIVATE -pg)
    target_link_options(solar_system_fetch_modern PRIVATE -pg)
endif()

# Set executable properties for legacy version
set_target_properties(solar_system_fetch PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Set executable properties for modern version
set_target_properties(solar_system_fetch_modern PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)
