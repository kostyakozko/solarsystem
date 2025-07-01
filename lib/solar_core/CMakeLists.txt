# Solar Core Library - High-Performance Physics Engine with Modern C++20 Architecture
# Static library only for maximum performance and LTO benefits

add_library(solar_core STATIC
    # Legacy C-style files (to be migrated)
    model.cpp
    simulation.cpp
    constants.cpp
    
    # Modern C++20 classes
    src/math/vector3.cpp
    src/bodies/celestial_body.cpp
    src/bodies/body_collection.cpp
    src/data/body_definitions.cpp
)

# Public headers for other libraries and applications
target_include_directories(solar_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
