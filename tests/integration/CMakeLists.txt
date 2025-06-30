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

# Add integration tests to CTest
add_test(NAME IntegrationTest_ApplicationWorkflows COMMAND test_application_workflows)
add_test(NAME IntegrationTest_DataPipeline COMMAND test_data_pipeline)
add_test(NAME IntegrationTest_WebInterface COMMAND test_web_interface)
add_test(NAME IntegrationTest_EndToEnd COMMAND test_end_to_end)

# Set test properties
set_tests_properties(IntegrationTest_ApplicationWorkflows PROPERTIES
    LABELS "integration;workflows"
    TIMEOUT 120
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
message(STATUS "  - Test data directory: ${CMAKE_CURRENT_BINARY_DIR}/test_data")
