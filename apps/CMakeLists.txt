# Solar System Suite Applications

# Add main simulation application
add_subdirectory(solar_system)

# Add data fetching utility
add_subdirectory(solar_system_fetch)

# Add unified launcher
add_subdirectory(solar_system_launcher)

# Add real-time simulation
add_subdirectory(solar_system_realtime)

# Add web interface
add_subdirectory(solar_system_web)

# Add data analyzer
add_subdirectory(solar_system_analyzer)

# Create convenience target for all applications
add_custom_target(applications
    DEPENDS solar_system solar_system_fetch solar_system_launcher solar_system_realtime solar_system_web solar_system_analyzer
    COMMENT "Building all Solar System applications"
)
