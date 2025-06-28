# Solar System Suite Applications

# Add main simulation application
add_subdirectory(solar_system)

# Add data fetching utility
add_subdirectory(solar_system_fetch)

# Add unified launcher
add_subdirectory(solar_system_launcher)

# Future applications will be added here:
# add_subdirectory(solar_system_realtime)
# add_subdirectory(solar_system_web)

# Create convenience target for all applications
add_custom_target(applications
    DEPENDS solar_system solar_system_fetch solar_system_launcher
    COMMENT "Building all Solar System applications"
)
