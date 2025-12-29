# Integration Tests for Solar System Suite Applications
# Migrated to Google Test framework

# ============================================================================
# Integration Test Executables with Google Test
# ============================================================================

add_executable(test_application_workflows
    test_application_workflows.cpp
)

add_executable(test_data_pipeline
    test_data_pipeline.cpp
)

add_executable(test_web_interface
    test_web_interface.cpp
)

add_executable(test_end_to_end
    test_end_to_end.cpp
)

# Enhanced components integration tests (Task 21)
add_executable(test_enhanced_components_integration
    test_enhanced_components_integration.cpp
)

add_executable(test_performance_regression_integration
    test_performance_regression_integration.cpp
)

add_executable(test_cross_component_validation
    test_cross_component_validation.cpp
)

# Advanced Configuration Management Integration Tests (Task 7)
add_executable(test_advanced_config_integration
    test_advanced_config_integration.cpp
)

# Comprehensive Integration Test Suite (Task 30)
add_executable(test_comprehensive_integration
    test_comprehensive_integration_simple.cpp
)

# Component Interface and Communication Tests (Task 6)
add_executable(test_component_interface_communication
    test_component_interface_communication.cpp
)

# Realistic Environment Simulation Tests (Task 7)
add_executable(test_realistic_environment_simulation
    test_realistic_environment_simulation.cpp
)

# Unimplemented Functions Integration Tests (Task 11.1)
add_executable(test_unimplemented_functions_integration
    test_unimplemented_functions_integration.cpp
)

# Workflow Validation Tests (Task 11.2)
add_executable(test_workflow_validation
    test_workflow_validation.cpp
)

# Data Validation Integration Tests (Task 11)
add_executable(test_data_validation_integration
    test_data_validation_integration.cpp
)

# Performance and Security Validation Tests (Task 12)
add_executable(test_performance_security_validation
    test_performance_security_validation.cpp
)

# ============================================================================
# Link Libraries - All integration tests use Google Test
# ============================================================================
# Note: These tests use GTest::gtest (not gtest_main) because they have
# custom main() functions using the legacy TEST_SUITE/TEST_CASE pattern.
# They will be migrated to native Google Test in Phase 4 (tasks 8-10).

target_link_libraries(test_application_workflows
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_data_pipeline
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_web_interface
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_end_to_end
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for enhanced integration tests
target_link_libraries(test_enhanced_components_integration
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_performance_regression_integration
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_cross_component_validation
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for advanced config integration tests
target_link_libraries(test_advanced_config_integration
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for comprehensive integration test
target_link_libraries(test_comprehensive_integration
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for component interface communication tests
target_link_libraries(test_component_interface_communication
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for realistic environment simulation tests
target_link_libraries(test_realistic_environment_simulation
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for unimplemented functions integration tests
target_link_libraries(test_unimplemented_functions_integration
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for workflow validation tests
target_link_libraries(test_workflow_validation
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for data validation integration tests
target_link_libraries(test_data_validation_integration
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for performance and security validation tests
target_link_libraries(test_performance_security_validation
    GTest::gtest
    GTest::gmock
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# ============================================================================
# Register Integration Tests with CTest
# ============================================================================
# Note: These tests use add_test instead of gtest_discover_tests because they
# have custom main() functions using the legacy TEST_SUITE/TEST_CASE pattern.
# They will be migrated to native Google Test in Phase 4 (tasks 8-10).

add_test(NAME IntegrationTest_ApplicationWorkflows COMMAND test_application_workflows)
set_tests_properties(IntegrationTest_ApplicationWorkflows PROPERTIES
    LABELS "integration;workflows"
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME IntegrationTest_DataPipeline COMMAND test_data_pipeline)
set_tests_properties(IntegrationTest_DataPipeline PROPERTIES
    LABELS "integration;data"
    TIMEOUT 180
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME IntegrationTest_WebInterface COMMAND test_web_interface)
set_tests_properties(IntegrationTest_WebInterface PROPERTIES
    LABELS "integration;web"
    TIMEOUT 60
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME IntegrationTest_EndToEnd COMMAND test_end_to_end)
set_tests_properties(IntegrationTest_EndToEnd PROPERTIES
    LABELS "integration;e2e"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Enhanced integration tests
add_test(NAME IntegrationTest_EnhancedComponents COMMAND test_enhanced_components_integration)
set_tests_properties(IntegrationTest_EnhancedComponents PROPERTIES
    LABELS "integration;enhanced;task21"
    TIMEOUT 300
)

add_test(NAME IntegrationTest_PerformanceRegression COMMAND test_performance_regression_integration)
set_tests_properties(IntegrationTest_PerformanceRegression PROPERTIES
    LABELS "integration;performance;task21"
    TIMEOUT 300
)

add_test(NAME IntegrationTest_CrossComponentValidation COMMAND test_cross_component_validation)
set_tests_properties(IntegrationTest_CrossComponentValidation PROPERTIES
    LABELS "integration;validation;task21"
    TIMEOUT 300
)

# Advanced config integration tests
add_test(NAME IntegrationTest_AdvancedConfigIntegration COMMAND test_advanced_config_integration)
set_tests_properties(IntegrationTest_AdvancedConfigIntegration PROPERTIES
    LABELS "integration;advanced;config;task7"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Comprehensive integration test (Task 30)
add_test(NAME IntegrationTest_Comprehensive COMMAND test_comprehensive_integration)
set_tests_properties(IntegrationTest_Comprehensive PROPERTIES
    LABELS "integration;comprehensive;task30;phase11"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Component interface communication tests (Task 6)
add_test(NAME IntegrationTest_ComponentInterfaceCommunication COMMAND test_component_interface_communication)
set_tests_properties(IntegrationTest_ComponentInterfaceCommunication PROPERTIES
    LABELS "integration;component;interface;communication;task6;phase2"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Realistic environment simulation tests (Task 7)
add_test(NAME IntegrationTest_RealisticEnvironmentSimulation COMMAND test_realistic_environment_simulation)
set_tests_properties(IntegrationTest_RealisticEnvironmentSimulation PROPERTIES
    LABELS "integration;environment;simulation;realistic;task7;phase2"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Unimplemented functions integration tests (Task 11.1)
add_test(NAME IntegrationTest_UnimplementedFunctions COMMAND test_unimplemented_functions_integration)
set_tests_properties(IntegrationTest_UnimplementedFunctions PROPERTIES
    LABELS "integration;unimplemented;functions;task11.1;phase4"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Workflow validation tests (Task 11.2)
add_test(NAME IntegrationTest_WorkflowValidation COMMAND test_workflow_validation)
set_tests_properties(IntegrationTest_WorkflowValidation PROPERTIES
    LABELS "integration;workflow;validation;task11.2;phase4"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Data validation integration tests (Task 11)
add_test(NAME IntegrationTest_DataValidation COMMAND test_data_validation_integration)
set_tests_properties(IntegrationTest_DataValidation PROPERTIES
    LABELS "integration;data;validation;task11;phase4"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Performance and security validation tests (Task 12)
add_test(NAME IntegrationTest_PerformanceSecurityValidation COMMAND test_performance_security_validation)
set_tests_properties(IntegrationTest_PerformanceSecurityValidation PROPERTIES
    LABELS "integration;performance;security;validation;task12;phase4"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Test data directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# Copy test data files
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/test_ephemeris.json
    ${CMAKE_CURRENT_BINARY_DIR}/test_data/test_ephemeris.json
    COPYONLY
)

message(STATUS "Integration tests configured:")
message(STATUS "  - Application Workflow Tests")
message(STATUS "  - Data Pipeline Tests")
message(STATUS "  - Web Interface Tests")
message(STATUS "  - End-to-End Tests")
message(STATUS "  - Enhanced Components Integration Tests (Task 21)")
message(STATUS "  - Performance Regression Integration Tests (Task 21)")
message(STATUS "  - Cross-Component Validation Tests (Task 21)")
message(STATUS "  - Component Interface and Communication Tests (Task 6)")
message(STATUS "  - Realistic Environment Simulation Tests (Task 7)")
message(STATUS "  - Comprehensive Integration Test Suite (Task 30)")
message(STATUS "  - Unimplemented Functions Integration Tests (Task 11.1)")
message(STATUS "  - Workflow Validation Tests (Task 11.2)")
message(STATUS "  - Data Validation Integration Tests (Task 11)")
message(STATUS "  - Performance and Security Validation Tests (Task 12)")
message(STATUS "  - Test data directory: ${CMAKE_CURRENT_BINARY_DIR}/test_data")
