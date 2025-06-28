# Solar System Web Application
# HTTP server with REST API and WebGL visualization

add_executable(solar_system_web
    src/web_server.cpp
)

# Link required libraries
target_link_libraries(solar_system_web PRIVATE
    solar_core      # For simulation and model functions
    solar_jpl       # For JPL data integration
    solar_utils     # For argument parsing utilities
    Threads::Threads
)

# Optimization flags (moderate optimization for web server responsiveness)
target_compile_options(solar_system_web PRIVATE
    $<$<CONFIG:Release>:-O2 -march=native -mtune=native>
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
)

# Enable profiling if requested
if(ENABLE_PROFILING)
    target_compile_options(solar_system_web PRIVATE -pg)
    target_link_options(solar_system_web PRIVATE -pg)
endif()

# Set executable properties
set_target_properties(solar_system_web PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Install web files
install(DIRECTORY web/
        DESTINATION share/solar_system/web
        FILES_MATCHING 
        PATTERN "*.html"
        PATTERN "*.js"
        PATTERN "*.css")
