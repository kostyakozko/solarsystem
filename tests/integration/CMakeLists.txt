# Integration Tests for Solar System Suite Applications

# Integration test executables
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

# Link libraries
target_link_libraries(test_application_workflows
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_data_pipeline
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_web_interface
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_end_to_end
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for enhanced integration tests
target_link_libraries(test_enhanced_components_integration
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_performance_regression_integration
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

target_link_libraries(test_cross_component_validation
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for advanced config integration tests
target_link_libraries(test_advanced_config_integration
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for comprehensive integration test
target_link_libraries(test_comprehensive_integration
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for component interface communication tests
target_link_libraries(test_component_interface_communication
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Link libraries for realistic environment simulation tests
target_link_libraries(test_realistic_environment_simulation
    test_utils
    solar_core
    solar_jpl
    solar_utils
    Threads::Threads
)

# Add integration tests to CTest
add_test(NAME IntegrationTest_ApplicationWorkflows
         COMMAND tests/integration/test_application_workflows
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME IntegrationTest_DataPipeline COMMAND test_data_pipeline)
add_test(NAME IntegrationTest_WebInterface COMMAND test_web_interface)
add_test(NAME IntegrationTest_EndToEnd COMMAND test_end_to_end)

# Add enhanced integration tests to CTest
add_test(NAME IntegrationTest_EnhancedComponents COMMAND test_enhanced_components_integration)
add_test(NAME IntegrationTest_PerformanceRegression COMMAND test_performance_regression_integration)
add_test(NAME IntegrationTest_CrossComponentValidation COMMAND test_cross_component_validation)

# Add advanced config integration tests to CTest
add_test(NAME IntegrationTest_AdvancedConfigIntegration COMMAND test_advanced_config_integration)

# Add comprehensive integration test to CTest (Task 30)
add_test(NAME IntegrationTest_Comprehensive COMMAND test_comprehensive_integration)

# Add component interface communication tests to CTest (Task 6)
add_test(NAME IntegrationTest_ComponentInterfaceCommunication
         COMMAND test_component_interface_communication
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Add realistic environment simulation tests to CTest (Task 7)
add_test(NAME IntegrationTest_RealisticEnvironmentSimulation
         COMMAND test_realistic_environment_simulation
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Set working directory for all integration tests
set_tests_properties(IntegrationTest_DataPipeline PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(IntegrationTest_WebInterface PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(IntegrationTest_EndToEnd PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Set test properties
set_tests_properties(IntegrationTest_ApplicationWorkflows PROPERTIES
    LABELS "integration;workflows"
    TIMEOUT 30
)

set_tests_properties(IntegrationTest_DataPipeline PROPERTIES
    LABELS "integration;data"
    TIMEOUT 180
)

set_tests_properties(IntegrationTest_WebInterface PROPERTIES
    LABELS "integration;web"
    TIMEOUT 60
)

set_tests_properties(IntegrationTest_EndToEnd PROPERTIES
    LABELS "integration;e2e"
    TIMEOUT 300
)

# Set properties for enhanced integration tests
set_tests_properties(IntegrationTest_EnhancedComponents PROPERTIES
    LABELS "integration;enhanced;task21"
    TIMEOUT 300
)

set_tests_properties(IntegrationTest_PerformanceRegression PROPERTIES
    LABELS "integration;performance;task21"
    TIMEOUT 300
)

set_tests_properties(IntegrationTest_CrossComponentValidation PROPERTIES
    LABELS "integration;validation;task21"
    TIMEOUT 300
)

# Set properties for advanced config integration tests
set_tests_properties(IntegrationTest_AdvancedConfigIntegration PROPERTIES
    LABELS "integration;advanced;config;task7"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Set properties for comprehensive integration test (Task 30)
set_tests_properties(IntegrationTest_Comprehensive PROPERTIES
    LABELS "integration;comprehensive;task30;phase11"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Set properties for component interface communication tests (Task 6)
set_tests_properties(IntegrationTest_ComponentInterfaceCommunication PROPERTIES
    LABELS "integration;component;interface;communication;task6;phase2"
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Set properties for realistic environment simulation tests (Task 7)
set_tests_properties(IntegrationTest_RealisticEnvironmentSimulation PROPERTIES
    LABELS "integration;environment;simulation;realistic;task7;phase2"
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
message(STATUS "  - Test data directory: ${CMAKE_CURRENT_BINARY_DIR}/test_data")
