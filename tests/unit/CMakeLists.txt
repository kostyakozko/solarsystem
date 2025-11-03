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

# Enhanced Body Collection Tests (Task 5)
add_executable(test_enhanced_body_collection test_enhanced_body_collection.cpp)
target_link_libraries(test_enhanced_body_collection test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_EnhancedBodyCollection COMMAND test_enhanced_body_collection)
set_tests_properties(UnitTest_EnhancedBodyCollection PROPERTIES LABELS "unit;enhanced;bodies;collection;task5" TIMEOUT ${TEST_TIMEOUT})

# Intelligent Fallback Strategies Tests (Task 6)
add_executable(test_intelligent_fallback test_intelligent_fallback.cpp)
target_link_libraries(test_intelligent_fallback test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_IntelligentFallback COMMAND test_intelligent_fallback)
set_tests_properties(UnitTest_IntelligentFallback PROPERTIES LABELS "unit;intelligent;fallback;factory;task6" TIMEOUT ${TEST_TIMEOUT})

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

# Connection management test (Task 12)
add_executable(test_connection_management test_connection_management.cpp)
target_link_libraries(test_connection_management test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ConnectionManagement COMMAND test_connection_management)
set_tests_properties(UnitTest_ConnectionManagement PROPERTIES LABELS "unit;connection;management;task12" TIMEOUT ${TEST_TIMEOUT})

# Security system test (Task 13)
add_executable(test_security test_security.cpp)
target_link_libraries(test_security test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_Security COMMAND test_security)
set_tests_properties(UnitTest_Security PROPERTIES LABELS "unit;security;hardening;task13" TIMEOUT ${TEST_TIMEOUT})

# API management test (Task 15)
add_executable(test_api_management test_api_management.cpp)
target_link_libraries(test_api_management test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_APIManagement COMMAND test_api_management)
set_tests_properties(UnitTest_APIManagement PROPERTIES LABELS "unit;api;management;task15" TIMEOUT ${TEST_TIMEOUT})

# Configuration manager test (Tasks 16 & 17)
add_executable(test_config_manager test_config_manager.cpp)
target_link_libraries(test_config_manager test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ConfigManager COMMAND test_config_manager)
set_tests_properties(UnitTest_ConfigManager PROPERTIES LABELS "unit;config;manager;task16;task17" TIMEOUT ${TEST_TIMEOUT})

# Communication protocols test (Task 18)
add_executable(test_communication test_communication.cpp)
target_link_libraries(test_communication test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_Communication COMMAND test_communication)
set_tests_properties(UnitTest_Communication PROPERTIES LABELS "unit;communication;protocols;task18" TIMEOUT ${TEST_TIMEOUT})

# Shared data manager and synchronization test (Task 19)
add_executable(test_shared_data_manager test_shared_data_manager.cpp)
target_link_libraries(test_shared_data_manager test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_SharedDataManager COMMAND test_shared_data_manager)
set_tests_properties(UnitTest_SharedDataManager PROPERTIES LABELS "unit;data;sharing;synchronization;task19" TIMEOUT ${TEST_TIMEOUT})

# Workflow coordination system test (Task 20)
add_executable(test_workflow_coordinator test_workflow_coordinator.cpp)
target_link_libraries(test_workflow_coordinator test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_WorkflowCoordinator COMMAND test_workflow_coordinator)
set_tests_properties(UnitTest_WorkflowCoordinator PROPERTIES LABELS "unit;workflow;coordination;transaction;task20" TIMEOUT ${TEST_TIMEOUT})

# Comprehensive logging system test (Task 21)
add_executable(test_logging test_logging.cpp)
target_link_libraries(test_logging test_utils solar_utils Threads::Threads)
add_test(NAME UnitTest_Logging COMMAND test_logging)
set_tests_properties(UnitTest_Logging PROPERTIES LABELS "unit;logging;monitoring;diagnostics;task21" TIMEOUT ${TEST_TIMEOUT})

# Performance monitoring system test (Task 22)
add_executable(test_performance_monitor test_performance_monitor.cpp)
target_link_libraries(test_performance_monitor test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_PerformanceMonitor COMMAND test_performance_monitor)
set_tests_properties(UnitTest_PerformanceMonitor PROPERTIES LABELS "unit;performance;monitoring;metrics;task22" TIMEOUT ${TEST_TIMEOUT})

# Diagnostic and troubleshooting system test (Task 23)
add_executable(test_diagnostic_system test_diagnostic_system.cpp)
target_link_libraries(test_diagnostic_system test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_DiagnosticSystem COMMAND test_diagnostic_system)
set_tests_properties(UnitTest_DiagnosticSystem PROPERTIES LABELS "unit;diagnostics;troubleshooting;health;task23" TIMEOUT ${TEST_TIMEOUT})

# File system and network security test (Task 25)
add_executable(test_file_network_security test_file_network_security.cpp)
target_link_libraries(test_file_network_security test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_FileNetworkSecurity COMMAND test_file_network_security)
set_tests_properties(UnitTest_FileNetworkSecurity PROPERTIES LABELS "unit;security;filesystem;network;task25" TIMEOUT ${TEST_TIMEOUT})

# Backup and data protection test (Task 26)
add_executable(test_backup_data_protection test_backup_data_protection.cpp)
target_link_libraries(test_backup_data_protection test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_BackupDataProtection COMMAND test_backup_data_protection)
set_tests_properties(UnitTest_BackupDataProtection PROPERTIES LABELS "unit;backup;protection;recovery;task26" TIMEOUT ${TEST_TIMEOUT})

# Help system test (Task 27)
add_executable(test_help_system test_help_system.cpp)
target_link_libraries(test_help_system test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_HelpSystem COMMAND test_help_system)
set_tests_properties(UnitTest_HelpSystem PROPERTIES LABELS "unit;help;documentation;tutorials;task27" TIMEOUT ${TEST_TIMEOUT})

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
set_tests_properties(UnitTest_DataManagement PROPERTIES
    LABELS "unit;testing;data;management"
    TIMEOUT ${TEST_TIMEOUT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

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

# NEW: Port Management System Tests
add_executable(test_port_management test_port_management.cpp)
target_link_libraries(test_port_management test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_PortManagement COMMAND test_port_management)
set_tests_properties(UnitTest_PortManagement PROPERTIES LABELS "unit;testing;port;management" TIMEOUT ${TEST_TIMEOUT})

# NEW: Concurrent Execution Tests
add_executable(test_concurrent_execution test_concurrent_execution.cpp)
target_link_libraries(test_concurrent_execution test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_ConcurrentExecution COMMAND test_concurrent_execution)
set_tests_properties(UnitTest_ConcurrentExecution PROPERTIES LABELS "unit;testing;concurrent;execution" TIMEOUT ${TEST_TIMEOUT})

# NEW: Enhanced Test Framework with Memory and Performance Monitoring (Task 10)
add_executable(test_simulation_builder_enhanced test_simulation_builder_enhanced.cpp)
target_link_libraries(test_simulation_builder_enhanced test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_SimulationBuilderEnhanced COMMAND test_simulation_builder_enhanced)
set_tests_properties(UnitTest_SimulationBuilderEnhanced PROPERTIES LABELS "unit;enhanced;testing;memory;performance;task10" TIMEOUT 60)

add_executable(test_date_time_comprehensive test_date_time_comprehensive.cpp)
target_link_libraries(test_date_time_comprehensive test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_DateTimeComprehensive COMMAND test_date_time_comprehensive)
set_tests_properties(UnitTest_DateTimeComprehensive PROPERTIES LABELS "unit;enhanced;datetime;comprehensive;task10" TIMEOUT 60)

# NEW: Comprehensive Performance Monitoring (Task 11)
add_executable(test_performance_comprehensive test_performance_comprehensive.cpp)
target_link_libraries(test_performance_comprehensive test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_PerformanceComprehensive COMMAND test_performance_comprehensive)
set_tests_properties(UnitTest_PerformanceComprehensive PROPERTIES LABELS "unit;performance;monitoring;comprehensive;task11" TIMEOUT 120)

# NEW: Enhanced Test Data Management (Task 12)
add_executable(test_enhanced_data_management test_enhanced_data_management.cpp)
target_link_libraries(test_enhanced_data_management test_utils solar_test solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_EnhancedDataManagement COMMAND test_enhanced_data_management)
set_tests_properties(UnitTest_EnhancedDataManagement PROPERTIES LABELS "unit;enhanced;data;management;comprehensive;task12" TIMEOUT 120)

# Input validation tests (Task 13)
add_executable(test_input_validation_comprehensive test_input_validation_comprehensive.cpp)
target_link_libraries(test_input_validation_comprehensive test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_InputValidationComprehensive COMMAND test_input_validation_comprehensive)
set_tests_properties(UnitTest_InputValidationComprehensive PROPERTIES LABELS "unit;input;validation;comprehensive;task13" TIMEOUT 120)

# Data validation tests (Task 6 - Application Enhancements)
add_executable(test_data_validator test_data_validator.cpp)
target_link_libraries(test_data_validator test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_DataValidator COMMAND test_data_validator)
set_tests_properties(UnitTest_DataValidator PROPERTIES LABELS "unit;data;validation;comprehensive;task6" TIMEOUT 120)

# Performance Regression Testing System (Task 22)
add_executable(test_performance_regression_system_simple test_performance_regression_system_simple.cpp)
target_link_libraries(test_performance_regression_system_simple test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_PerformanceRegressionSystemSimple COMMAND test_performance_regression_system_simple)
set_tests_properties(UnitTest_PerformanceRegressionSystemSimple PROPERTIES LABELS "unit;performance;regression;testing;task22" TIMEOUT 120)

# Advanced Configuration Management System (Task 7)
add_executable(test_advanced_config test_advanced_config.cpp)
target_link_libraries(test_advanced_config test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_AdvancedConfig COMMAND test_advanced_config)
set_tests_properties(UnitTest_AdvancedConfig PROPERTIES LABELS "unit;advanced;config;management;task7" TIMEOUT 120)

# Simulation Checkpointing and Resume (Task 8)
add_executable(test_checkpoint test_checkpoint.cpp)
target_link_libraries(test_checkpoint test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_Checkpoint COMMAND test_checkpoint)
set_tests_properties(UnitTest_Checkpoint PROPERTIES LABELS "unit;checkpoint;simulation;resume;task8" TIMEOUT 120)

# Comprehensive Output Formatting (Task 9)
add_executable(test_output_formatter test_output_formatter.cpp)
target_link_libraries(test_output_formatter test_utils solar_core solar_jpl solar_utils Threads::Threads)
add_test(NAME UnitTest_OutputFormatter COMMAND test_output_formatter)
set_tests_properties(UnitTest_OutputFormatter PROPERTIES LABELS "unit;output;formatting;comprehensive;task9" TIMEOUT 120)

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
    test_port_management
    test_concurrent_execution
    test_simulation_builder_enhanced
    test_date_time_comprehensive
    test_performance_comprehensive
    test_enhanced_data_management
    test_data_validator
    test_checkpoint
    test_output_formatter
    test_connection_management
    test_security
    test_api_management
    test_config_manager
    test_communication
    test_shared_data_manager
    test_workflow_coordinator
    test_logging
    test_performance_monitor
    test_diagnostic_system
    test_file_network_security
    test_backup_data_protection
    test_help_system
    RUNTIME DESTINATION bin/tests
    COMPONENT tests
)

