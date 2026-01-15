# Modern C++ Solar System Examples
# Demonstrates the new architecture and classes

# Simple core demo (no JPL dependencies for static builds, but needs it for shared)
add_executable(simple_demo simple_demo.cpp)
if(BUILD_SHARED_LIBS)
    # For shared library builds, solar_core depends on solar_jpl
    target_link_libraries(simple_demo
        solar_core
        solar_jpl
        solar_utils
        Threads::Threads
    )
else()
    target_link_libraries(simple_demo
        solar_core
        solar_utils
        Threads::Threads
    )
endif()

# Modern demo application (with JPL integration - currently has linking issues)
add_executable(modern_demo modern_demo.cpp)
target_link_libraries(modern_demo
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Set properties
set_target_properties(simple_demo modern_demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Install the examples
install(TARGETS simple_demo modern_demo
    RUNTIME DESTINATION bin
    COMPONENT examples
)
