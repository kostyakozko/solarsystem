# Unit Tests for Solar System Suite Libraries

# Modern C++20 class tests
add_executable(test_modern_vector3 test_modern_vector3.cpp)
target_link_libraries(test_modern_vector3 test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ModernVector3 COMMAND test_modern_vector3)
set_tests_properties(UnitTest_ModernVector3 PROPERTIES LABELS "unit;modern;math;vector3" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_modern_celestial_body test_modern_celestial_body.cpp)
target_link_libraries(test_modern_celestial_body test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ModernCelestialBody COMMAND test_modern_celestial_body)
set_tests_properties(UnitTest_ModernCelestialBody PROPERTIES LABELS "unit;modern;bodies;celestial" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_modern_body_collection test_modern_body_collection.cpp)
target_link_libraries(test_modern_body_collection test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ModernBodyCollection COMMAND test_modern_body_collection)
set_tests_properties(UnitTest_ModernBodyCollection PROPERTIES LABELS "unit;modern;bodies;collection" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_modern_simulation_engine test_modern_simulation_engine.cpp)
target_link_libraries(test_modern_simulation_engine test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ModernSimulationEngine COMMAND test_modern_simulation_engine)
set_tests_properties(UnitTest_ModernSimulationEngine PROPERTIES LABELS "unit;modern;simulation;engine" TIMEOUT ${TEST_TIMEOUT})

# NEW: Modern BodyFactory with JPL Integration (now working!)
add_executable(test_body_factory test_body_factory.cpp)
target_link_libraries(test_body_factory test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_BodyFactory COMMAND test_body_factory)
set_tests_properties(UnitTest_BodyFactory PROPERTIES LABELS "unit;modern;bodies;factory;jpl" TIMEOUT ${TEST_TIMEOUT})

# NEW: SimulationBuilder with Body Filtering
add_executable(test_simulation_builder test_simulation_builder.cpp)
target_link_libraries(test_simulation_builder test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_SimulationBuilder COMMAND test_simulation_builder)
set_tests_properties(UnitTest_SimulationBuilder PROPERTIES LABELS "unit;modern;builders;filtering" TIMEOUT ${TEST_TIMEOUT})

# Re-enabled: Modern body factory test (JPL linking issues fixed)
add_executable(test_modern_body_factory test_modern_body_factory.cpp)
target_link_libraries(test_modern_body_factory test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_ModernBodyFactory COMMAND test_modern_body_factory)
set_tests_properties(UnitTest_ModernBodyFactory PROPERTIES LABELS "unit;modern;bodies;factory" TIMEOUT ${TEST_TIMEOUT})

# Existing tests that have source files
add_executable(test_jpl_data_only test_jpl_data.cpp)
target_link_libraries(test_jpl_data_only test_utils solar_jpl solar_core solar_utils)
add_test(NAME UnitTest_JPLData COMMAND test_jpl_data_only)
set_tests_properties(UnitTest_JPLData PROPERTIES LABELS "unit;jpl;data")

add_executable(test_jpl_bodies test_jpl_bodies.cpp)
target_link_libraries(test_jpl_bodies test_utils solar_jpl solar_core solar_utils)
add_test(NAME UnitTest_JPLBodies COMMAND test_jpl_bodies)
set_tests_properties(UnitTest_JPLBodies PROPERTIES LABELS "unit;jpl;bodies")

add_executable(test_solar_utils test_solar_utils.cpp)
target_link_libraries(test_solar_utils test_utils solar_utils Threads::Threads)
add_test(NAME UnitTest_SolarUtils COMMAND test_solar_utils)
set_tests_properties(UnitTest_SolarUtils PROPERTIES LABELS "unit;utils" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_args test_args.cpp)
target_link_libraries(test_args test_utils solar_utils)
add_test(NAME UnitTest_Args COMMAND test_args)
set_tests_properties(UnitTest_Args PROPERTIES LABELS "unit;args")

add_executable(test_solar_jpl test_solar_jpl.cpp)
target_link_libraries(test_solar_jpl test_utils solar_jpl solar_core solar_utils)
add_test(NAME UnitTest_SolarJPL COMMAND test_solar_jpl)
set_tests_properties(UnitTest_SolarJPL PROPERTIES LABELS "unit;jpl")

# NEW: Test Discovery Mechanism Tests
add_executable(test_test_discovery test_test_discovery.cpp)
target_link_libraries(test_test_discovery test_utils solar_test solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_TestDiscovery COMMAND test_test_discovery)
set_tests_properties(UnitTest_TestDiscovery PROPERTIES LABELS "unit;testing;discovery" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_discovery_example test_discovery_example.cpp)
target_link_libraries(test_discovery_example test_utils solar_test solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_DiscoveryExample COMMAND test_discovery_example)
set_tests_properties(UnitTest_DiscoveryExample PROPERTIES LABELS "unit;testing;example" TIMEOUT ${TEST_TIMEOUT})

message(STATUS "Unit tests configured:")
message(STATUS "  - Solar Core Library Tests")
message(STATUS "  - Solar JPL Library Tests")
message(STATUS "  - Solar Utils Library Tests")
message(STATUS "  - Individual Component Tests")
message(STATUS "  - NEW: BodyFactory JPL Integration Tests")
message(STATUS "  - NEW: SimulationBuilder Filtering Tests")

# Install test executables to install/bin/tests/ directory
install(TARGETS
    test_modern_vector3
    test_modern_celestial_body
    test_modern_body_collection
    test_modern_simulation_engine
    test_body_factory
    test_simulation_builder
    test_modern_body_factory
    test_jpl_data_only
    test_jpl_bodies
    test_solar_utils
    test_args
    test_solar_jpl
    test_test_discovery
    test_discovery_example
    RUNTIME DESTINATION bin/tests
    COMPONENT tests
)
