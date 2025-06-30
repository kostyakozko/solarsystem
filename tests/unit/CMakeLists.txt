# Unit Tests for Solar System Suite Libraries

# Test executables for each library
add_executable(test_solar_core
    test_solar_core.cpp
    test_simulation.cpp
    test_model.cpp
    test_constants.cpp
)

add_executable(test_solar_jpl
    test_solar_jpl.cpp
    test_jpl_data.cpp
    test_jpl_bodies.cpp
)

add_executable(test_solar_utils
    test_solar_utils.cpp
    test_args.cpp
    test_types.cpp
)

# Link libraries
target_link_libraries(test_solar_core
    test_utils
    solar_core
    solar_utils
    Threads::Threads
)

target_link_libraries(test_solar_jpl
    test_utils
    solar_jpl
    solar_core
    solar_utils
    Threads::Threads
)

target_link_libraries(test_solar_utils
    test_utils
    solar_utils
    Threads::Threads
)

# Add tests to CTest
add_test(NAME UnitTest_SolarCore COMMAND test_solar_core)
add_test(NAME UnitTest_SolarJPL COMMAND test_solar_jpl)
add_test(NAME UnitTest_SolarUtils COMMAND test_solar_utils)

# Set test properties
set_tests_properties(UnitTest_SolarCore PROPERTIES
    LABELS "unit;core"
    TIMEOUT ${TEST_TIMEOUT}
)

set_tests_properties(UnitTest_SolarJPL PROPERTIES
    LABELS "unit;jpl"
    TIMEOUT ${TEST_TIMEOUT}
)

set_tests_properties(UnitTest_SolarUtils PROPERTIES
    LABELS "unit;utils"
    TIMEOUT ${TEST_TIMEOUT}
)

# Individual test components
add_executable(test_simulation_only test_simulation.cpp)
target_link_libraries(test_simulation_only test_utils solar_core solar_utils)
add_test(NAME UnitTest_Simulation COMMAND test_simulation_only)
set_tests_properties(UnitTest_Simulation PROPERTIES LABELS "unit;simulation")

add_executable(test_model_only test_model.cpp)
target_link_libraries(test_model_only test_utils solar_core solar_utils)
add_test(NAME UnitTest_Model COMMAND test_model_only)
set_tests_properties(UnitTest_Model PROPERTIES LABELS "unit;model")

add_executable(test_jpl_data_only test_jpl_data.cpp)
target_link_libraries(test_jpl_data_only test_utils solar_jpl solar_core solar_utils)
add_test(NAME UnitTest_JPLData COMMAND test_jpl_data_only)
set_tests_properties(UnitTest_JPLData PROPERTIES LABELS "unit;jpl;data")

# Performance-sensitive unit tests
add_executable(test_performance_critical
    test_performance_critical.cpp
)
target_link_libraries(test_performance_critical
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)
add_test(NAME UnitTest_Performance COMMAND test_performance_critical)
set_tests_properties(UnitTest_Performance PROPERTIES 
    LABELS "unit;performance"
    TIMEOUT 60
)

message(STATUS "Unit tests configured:")
message(STATUS "  - Solar Core Library Tests")
message(STATUS "  - Solar JPL Library Tests") 
message(STATUS "  - Solar Utils Library Tests")
message(STATUS "  - Individual Component Tests")
message(STATUS "  - Performance Critical Tests")
