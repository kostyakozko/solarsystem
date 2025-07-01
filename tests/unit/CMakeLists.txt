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

add_executable(test_modern_body_factory test_modern_body_factory.cpp)
target_link_libraries(test_modern_body_factory test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_ModernBodyFactory COMMAND test_modern_body_factory)
set_tests_properties(UnitTest_ModernBodyFactory PROPERTIES LABELS "unit;modern;bodies;factory" TIMEOUT ${TEST_TIMEOUT})

# Legacy tests (to be migrated gradually)
add_executable(test_solar_core test_solar_core.cpp)
target_link_libraries(test_solar_core test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_SolarCore COMMAND test_solar_core)
set_tests_properties(UnitTest_SolarCore PROPERTIES LABELS "unit;core" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_simulation_only test_simulation.cpp)
target_link_libraries(test_simulation_only test_utils solar_core solar_utils)
add_test(NAME UnitTest_Simulation COMMAND test_simulation_only)
set_tests_properties(UnitTest_Simulation PROPERTIES LABELS "unit;simulation")

add_executable(test_model_only test_model.cpp)
target_link_libraries(test_model_only test_utils solar_core solar_utils)
add_test(NAME UnitTest_Model COMMAND test_model_only)
set_tests_properties(UnitTest_Model PROPERTIES LABELS "unit;model")

add_executable(test_constants test_constants.cpp)
target_link_libraries(test_constants test_utils solar_core solar_utils)
add_test(NAME UnitTest_Constants COMMAND test_constants)
set_tests_properties(UnitTest_Constants PROPERTIES LABELS "unit;constants")

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

add_executable(test_types test_types.cpp)
target_link_libraries(test_types test_utils solar_core solar_utils)
add_test(NAME UnitTest_Types COMMAND test_types)
set_tests_properties(UnitTest_Types PROPERTIES LABELS "unit;types")

# Performance-sensitive unit tests
add_executable(test_performance_critical test_performance_critical.cpp)
target_link_libraries(test_performance_critical test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_Performance COMMAND test_performance_critical)
set_tests_properties(UnitTest_Performance PROPERTIES LABELS "unit;performance" TIMEOUT 60)

message(STATUS "Unit tests configured:")
message(STATUS "  - Solar Core Library Tests")
message(STATUS "  - Solar JPL Library Tests") 
message(STATUS "  - Solar Utils Library Tests")
message(STATUS "  - Individual Component Tests")
message(STATUS "  - Performance Critical Tests")
