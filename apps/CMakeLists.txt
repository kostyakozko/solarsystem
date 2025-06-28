# Solar System Suite Applications

# Add main simulation application
add_subdirectory(solar_system)

# Future applications will be added here:
# add_subdirectory(solar_system_fetch)
# add_subdirectory(solar_system_launcher)
# add_subdirectory(solar_system_realtime)
# add_subdirectory(solar_system_web)

# Create convenience target for all applications
add_custom_target(applications
    DEPENDS solar_system
    COMMENT "Building all Solar System applications"
)
