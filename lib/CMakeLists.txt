# Solar System Suite Libraries
# Static libraries for maximum performance and LTO benefits

# Add all library subdirectories
add_subdirectory(solar_jpl)
add_subdirectory(solar_core)
add_subdirectory(solar_utils)

# Create convenience target for all libraries
add_custom_target(libraries
    DEPENDS solar_core solar_jpl solar_utils
    COMMENT "Building all Solar System libraries"
)
