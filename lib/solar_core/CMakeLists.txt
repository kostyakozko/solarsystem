# Solar Core Library - High-Performance Physics Engine
# Static library only for maximum performance and LTO benefits

add_library(solar_core STATIC
    model.cpp
    simulation.cpp
    constants.cpp
)

# Public headers for other libraries and applications
target_include_directories(solar_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(solar_core PUBLIC
    solar_utils     # Needs args.h for simulation parameters
)

# Aggressive optimization for core physics calculations
target_compile_options(solar_core PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_core PRIVATE -pg)
    target_link_options(solar_core PRIVATE -pg)
endif()

# Set library properties
set_target_properties(solar_core PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
