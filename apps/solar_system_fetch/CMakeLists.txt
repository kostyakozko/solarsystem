# Solar System Fetch Application
# Modern C++20 JPL HORIZONS data fetching and management utility

add_executable(solar_system_fetch
    fetch.cpp
)

# Link required libraries
target_link_libraries(solar_system_fetch PRIVATE
    solar_core      # Modern builders and patterns
    solar_jpl       # JPL data integration  
    solar_utils     # Configuration and logging
    Threads::Threads
)

# Modern C++20 optimization flags
target_compile_options(solar_system_fetch PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system_fetch PRIVATE -pg)
    target_link_options(solar_system_fetch PRIVATE -pg)
endif()

# Set modern C++20 properties
set_target_properties(solar_system_fetch PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)
