# Solar System Analyzer Application
# Data analysis tool for celestial body data

add_executable(solar_system_analyzer
    analyzer.cpp
)

target_link_libraries(solar_system_analyzer
    PRIVATE
        solar_analysis
        solar_core
        solar_jpl
        solar_utils
)

set_target_properties(solar_system_analyzer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Install
install(TARGETS solar_system_analyzer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
