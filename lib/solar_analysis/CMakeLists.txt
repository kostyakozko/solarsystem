# Solar Analysis Library
# Core analysis components for Solar System data analysis

set(SOLAR_ANALYSIS_SOURCES
    src/analysis_engine.cpp
    src/data_processor.cpp
    src/data_models.cpp
    src/orbital_calculator.cpp
    src/statistical_analyzer.cpp
    src/data_exporter.cpp
    src/batch_processor.cpp
)

set(SOLAR_ANALYSIS_HEADERS
    include/solar_analysis/analysis_engine.hpp
    include/solar_analysis/data_processor.hpp
    include/solar_analysis/data_models.hpp
    include/solar_analysis/orbital_calculator.hpp
    include/solar_analysis/statistical_analyzer.hpp
    include/solar_analysis/data_exporter.hpp
    include/solar_analysis/batch_processor.hpp
    include/solar_analysis/export.hpp
)

# Create library (static or shared based on BUILD_SHARED_LIBS)
add_library(solar_analysis ${SOLAR_ANALYSIS_SOURCES} ${SOLAR_ANALYSIS_HEADERS})

target_include_directories(solar_analysis
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(solar_analysis
    PUBLIC
        solar_core
        solar_jpl
        solar_utils
)

# Set library properties
set_target_properties(solar_analysis PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Export configuration for shared/static builds
if(BUILD_SHARED_LIBS)
    target_compile_definitions(solar_analysis PRIVATE SOLAR_ANALYSIS_EXPORTS)
    set_target_properties(solar_analysis PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
else()
    target_compile_definitions(solar_analysis PUBLIC SOLAR_ANALYSIS_STATIC_DEFINE)
endif()

# Install targets
install(TARGETS solar_analysis
    EXPORT SolarSystemTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
