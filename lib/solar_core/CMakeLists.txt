# Solar Core Library - High-Performance Physics Engine with Modern C++20 Architecture
# Static library only for maximum performance and LTO benefits

add_library(solar_core STATIC
    # Modern C++20 classes - Pure modern architecture
    src/math/vector3.cpp
    src/bodies/celestial_body.cpp
    src/bodies/body_collection.cpp
    src/bodies/body_factory.cpp
    src/data/body_definitions.cpp
    src/simulation/simulation_engine.cpp
    src/simulation/checkpoint.cpp

    # Modern API design patterns (Phase 0.3)
    src/builders/simulation_builder.cpp

    # Output formatting system (Phase 3 - Task 9)
    src/output/output_formatter.cpp
)

# Public headers for other libraries and applications
target_include_directories(solar_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find zlib for checkpoint compression
find_package(ZLIB REQUIRED)

# Link dependencies
target_link_libraries(solar_core PUBLIC
    solar_utils     # Needs args.h for simulation parameters
    solar_jpl       # Needs JPL data integration for BodyFactory
    Threads::Threads
    ZLIB::ZLIB      # For checkpoint compression
)

# Aggressive optimization for core physics calculations
target_compile_options(solar_core PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops>
    $<$<CONFIG:Debug>:-g -O0>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_core PRIVATE -pg)
    target_link_options(solar_core PRIVATE -pg)
endif()

# Set library properties
set_target_properties(solar_core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
