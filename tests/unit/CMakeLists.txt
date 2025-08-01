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

# NEW: JPL Mock System Tests
add_executable(test_jpl_mock test_jpl_mock.cpp)
target_link_libraries(test_jpl_mock test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_JPLMock COMMAND test_jpl_mock)
set_tests_properties(UnitTest_JPLMock PROPERTIES LABELS "unit;testing;mock;jpl" TIMEOUT ${TEST_TIMEOUT})

# NEW: Cache Mock System Tests
add_executable(test_cache_mock test_cache_mock.cpp)
target_link_libraries(test_cache_mock test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_CacheMock COMMAND test_cache_mock)
set_tests_properties(UnitTest_CacheMock PROPERTIES LABELS "unit;testing;mock;cache" TIMEOUT ${TEST_TIMEOUT})

# NEW: Time Mock System Tests
add_executable(test_time_mock test_time_mock.cpp)
target_link_libraries(test_time_mock test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_TimeMock COMMAND test_time_mock)
set_tests_properties(UnitTest_TimeMock PROPERTIES LABELS "unit;testing;mock;time" TIMEOUT ${TEST_TIMEOUT})

# NEW: Network Mock System Tests
add_executable(test_network_mock test_network_mock.cpp)
target_link_libraries(test_network_mock test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_NetworkMock COMMAND test_network_mock)
set_tests_properties(UnitTest_NetworkMock PROPERTIES LABELS "unit;testing;mock;network" TIMEOUT ${TEST_TIMEOUT})

# NEW: Mock Service Registry Tests
add_executable(test_mock_service_registry test_mock_service_registry.cpp)
target_link_libraries(test_mock_service_registry test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_MockServiceRegistry COMMAND test_mock_service_registry)
set_tests_properties(UnitTest_MockServiceRegistry PROPERTIES LABELS "unit;testing;mock;registry" TIMEOUT ${TEST_TIMEOUT})

# NEW: Test Data Management System Tests
add_executable(test_data_management test_data_management.cpp)
target_link_libraries(test_data_management test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_DataManagement COMMAND test_data_management)
set_tests_properties(UnitTest_DataManagement PROPERTIES LABELS "unit;testing;data;management" TIMEOUT ${TEST_TIMEOUT})

# NEW: Parallel Execution Tests
add_executable(test_parallel_execution test_parallel_execution.cpp)
target_link_libraries(test_parallel_execution test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ParallelExecution COMMAND test_parallel_execution)
set_tests_properties(UnitTest_ParallelExecution PROPERTIES LABELS "unit;testing;parallel;execution" TIMEOUT ${TEST_TIMEOUT})

# NEW: Comprehensive Core Library Component Tests
add_executable(test_body_factory_comprehensive test_body_factory_comprehensive.cpp)
target_link_libraries(test_body_factory_comprehensive test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_BodyFactoryComprehensive COMMAND test_body_factory_comprehensive)
set_tests_properties(UnitTest_BodyFactoryComprehensive PROPERTIES LABELS "unit;core;bodies;factory;comprehensive" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_simulation_builder_comprehensive test_simulation_builder_comprehensive.cpp)
target_link_libraries(test_simulation_builder_comprehensive test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_SimulationBuilderComprehensive COMMAND test_simulation_builder_comprehensive)
set_tests_properties(UnitTest_SimulationBuilderComprehensive PROPERTIES LABELS "unit;core;builders;comprehensive" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_cache_systems_comprehensive test_cache_systems_comprehensive.cpp)
target_link_libraries(test_cache_systems_comprehensive test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_CacheSystemsComprehensive COMMAND test_cache_systems_comprehensive)
set_tests_properties(UnitTest_CacheSystemsComprehensive PROPERTIES LABELS "unit;cache;systems;comprehensive" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_error_handling_comprehensive test_error_handling_comprehensive.cpp)
target_link_libraries(test_error_handling_comprehensive test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ErrorHandlingComprehensive COMMAND test_error_handling_comprehensive)
set_tests_properties(UnitTest_ErrorHandlingComprehensive PROPERTIES LABELS "unit;error;handling;comprehensive" TIMEOUT ${TEST_TIMEOUT})

add_executable(test_application_components test_application_components.cpp)
target_link_libraries(test_application_components test_utils solar_test solar_jpl solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ApplicationComponents COMMAND test_application_components)
set_tests_properties(UnitTest_ApplicationComponents PROPERTIES LABELS "unit;applications;components;comprehensive" TIMEOUT ${TEST_TIMEOUT})

message(STATUS "Unit tests configured:")
message(STATUS "  - Solar Core Library Tests")
message(STATUS "  - Solar JPL Library Tests")
message(STATUS "  - Solar Utils Library Tests")
message(STATUS "  - Individual Component Tests")
message(STATUS "  - NEW: BodyFactory JPL Integration Tests")
message(STATUS "  - NEW: SimulationBuilder Filtering Tests")
message(STATUS "  - NEW: Time Mock System Tests")
message(STATUS "  - NEW: Network Mock System Tests")
message(STATUS "  - NEW: Mock Service Registry Tests")
message(STATUS "  - NEW: Test Data Management System Tests")
message(STATUS "  - NEW: Parallel Execution Tests")
message(STATUS "  - NEW: Comprehensive Core Library Component Tests")
message(STATUS "    * BodyFactory with mocked JPL client")
message(STATUS "    * SimulationBuilder with filtering scenarios")
message(STATUS "    * Cache systems with mock file operations")
message(STATUS "    * Error handling and fallback mechanisms")
message(STATUS "    * Application components with argument parsing")
message(STATUS "    * Web server endpoints with mocked simulation data")
message(STATUS "    * Real-time monitoring with time simulation")
message(STATUS "    * Launcher workflow coordination")

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
    test_jpl_mock
    test_cache_mock
    test_time_mock
    test_network_mock
    test_mock_service_registry
    test_data_management
    test_parallel_execution
    test_body_factory_comprehensive
    test_simulation_builder_comprehensive
    test_cache_systems_comprehensive
    test_error_handling_comprehensive
    test_application_components
    RUNTIME DESTINATION bin/tests
    COMPONENT tests
)
