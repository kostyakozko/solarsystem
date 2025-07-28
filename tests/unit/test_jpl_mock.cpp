/**
 * @file test_jpl_mock.cpp
 * @brief Unit tests for JPL Mock System
 */

#include <chrono>
#include <thread>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;
using namespace SolarSystem::Testing::Mocks;

/**
 * @brief Test basic JPL mock functionality
 */
class JPLMockBasicTest : public TestCase {
 public:
  JPLMockBasicTest()
      : TestCase({"JPLMockBasicTest", "Test basic JPL mock functionality", {"unit", "mock"}}) {}

  void run() override {
    // Create a basic mock
    auto mock = JPLMockFactory::create_default();

    // Test initial state
    assert_equals(static_cast<size_t>(0), mock->call_count(), "Initial call count should be 0");
    assert_true(mock->requested_bodies().empty(), "Initial requested bodies should be empty");

    // Test mock request
    std::string params = "COMMAND='399'&START_TIME='2023-12-18'";
    auto result = mock->mock_request("https://test.api", params);

    assert_true(SolarSystem::JPL::is_success(result), "Mock request should succeed");
    assert_equals(static_cast<size_t>(1), mock->call_count(),
                  "Call count should be 1 after request");
    assert_true(mock->was_body_requested(399), "Earth (399) should have been requested");

    // Test response content
    const auto& response = SolarSystem::JPL::get_value(result);
    assert_true(
        response.find("Earth") != std::string::npos || response.find("399") != std::string::npos,
        "Response should contain Earth or JPL ID 399");
  }
};

/**
 * @brief Test JPL mock response configuration
 */
class JPLMockResponseConfigTest : public TestCase {
 public:
  JPLMockResponseConfigTest()
      : TestCase({"JPLMockResponseConfigTest",
                  "Test JPL mock response configuration",
                  {"unit", "mock"}}) {}

  void run() override {
    auto mock = JPLMockFactory::create_default();

    // Set custom response for Earth
    std::string custom_response = "Custom Earth response data";
    mock->set_response_for_body(399, custom_response);

    // Test custom response
    std::string params = "COMMAND='399'&START_TIME='2023-12-18'";
    auto result = mock->mock_request("https://test.api", params);

    assert_true(SolarSystem::JPL::is_success(result), "Mock request should succeed");
    assert_equals(custom_response, SolarSystem::JPL::get_value(result),
                  "Should return custom response");

    // Test different body gets realistic response
    params = "COMMAND='301'&START_TIME='2023-12-18'";
    result = mock->mock_request("https://test.api", params);

    assert_true(SolarSystem::JPL::is_success(result), "Mock request for Moon should succeed");
    const auto& moon_response = SolarSystem::JPL::get_value(result);
    assert_true(moon_response != custom_response,
                "Moon response should be different from custom Earth response");
  }
};

/**
 * @brief Test JPL mock failure simulation
 */
class JPLMockFailureTest : public TestCase {
 public:
  JPLMockFailureTest()
      : TestCase({"JPLMockFailureTest", "Test JPL mock failure simulation", {"unit", "mock"}}) {}

  void run() override {
    auto mock = JPLMockFactory::create_default();

    // Test network failure simulation
    mock->simulate_network_failure(1);

    std::string params = "COMMAND='399'&START_TIME='2023-12-18'";
    auto result = mock->mock_request("https://test.api", params);

    assert_false(SolarSystem::JPL::is_success(result),
                 "Request should fail due to simulated network failure");
    assert_true(SolarSystem::JPL::get_error(result) == SolarSystem::JPL::JPLError::NetworkError,
                "Should return network error");

    // Next request should succeed
    result = mock->mock_request("https://test.api", params);
    assert_true(SolarSystem::JPL::is_success(result), "Second request should succeed");

    // Test timeout simulation
    mock->simulate_timeout(1);
    result = mock->mock_request("https://test.api", params);
    assert_false(SolarSystem::JPL::is_success(result),
                 "Request should fail due to simulated timeout");

    // Test server error simulation
    mock->simulate_server_error(1);
    result = mock->mock_request("https://test.api", params);
    assert_false(SolarSystem::JPL::is_success(result),
                 "Request should fail due to simulated server error");
  }
};

/**
 * @brief Test JPL mock call verification
 */
class JPLMockCallVerificationTest : public TestCase {
 public:
  JPLMockCallVerificationTest()
      : TestCase(
            {"JPLMockCallVerificationTest", "Test JPL mock call verification", {"unit", "mock"}}) {}

  void run() override {
    auto mock = JPLMockFactory::create_default();

    // Make requests for different bodies
    std::vector<int> test_bodies = {10, 399, 301, 499};  // Sun, Earth, Moon, Mars

    for (int jpl_id : test_bodies) {
      std::string params = "COMMAND='" + std::to_string(jpl_id) + "'&START_TIME='2023-12-18'";
      auto result = mock->mock_request("https://test.api", params);
      assert_true(SolarSystem::JPL::is_success(result),
                  "Request should succeed for JPL ID " + std::to_string(jpl_id));
    }

    // Verify call counts
    assert_equals(test_bodies.size(), mock->call_count(),
                  "Total call count should match number of requests");

    for (int jpl_id : test_bodies) {
      assert_equals(static_cast<size_t>(1), mock->call_count_for_body(jpl_id),
                    "Each body should have been called once");
      assert_true(mock->was_body_requested(jpl_id), "Each body should be marked as requested");
    }

    // Verify requested bodies list
    auto requested = mock->requested_bodies();
    assert_equals(test_bodies.size(), requested.size(), "Requested bodies count should match");

    for (int jpl_id : test_bodies) {
      assert_true(std::find(requested.begin(), requested.end(), jpl_id) != requested.end(),
                  "JPL ID " + std::to_string(jpl_id) + " should be in requested bodies list");
    }

    // Test call history
    const auto& history = mock->call_history();
    assert_equals(test_bodies.size(), history.size(),
                  "Call history size should match number of calls");

    for (size_t i = 0; i < history.size(); ++i) {
      assert_equals(test_bodies[i], history[i].jpl_id, "Call history should preserve order");
      assert_true(history[i].was_successful, "All calls should be marked as successful");
    }

    // Test last call
    auto last_call = mock->last_call();
    assert_true(last_call.has_value(), "Should have last call information");
    assert_equals(test_bodies.back(), last_call->jpl_id, "Last call should be for Mars (499)");
  }
};

/**
 * @brief Test JPL mock network delay simulation
 */
class JPLMockNetworkDelayTest : public TestCase {
 public:
  JPLMockNetworkDelayTest()
      : TestCase({"JPLMockNetworkDelayTest",
                  "Test JPL mock network delay simulation",
                  {"unit", "mock"}}) {}

  void run() override {
    JPLMockConfig config;
    config.simulate_network_delays = true;
    config.min_response_delay = std::chrono::milliseconds(50);
    config.max_response_delay = std::chrono::milliseconds(100);

    auto mock = JPLMockFactory::create(config);

    // Measure request time
    auto start = std::chrono::steady_clock::now();

    std::string params = "COMMAND='399'&START_TIME='2023-12-18'";
    auto result = mock->mock_request("https://test.api", params);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    assert_true(SolarSystem::JPL::is_success(result), "Request should succeed");
    assert_true(duration >= config.min_response_delay,
                "Request should take at least minimum delay time");
    assert_true(duration <= config.max_response_delay + std::chrono::milliseconds(50),
                "Request should not take much longer than maximum delay time");
  }
};

/**
 * @brief Test JPL mock cache operations
 */
class JPLMockCacheTest : public TestCase {
 public:
  JPLMockCacheTest()
      : TestCase({"JPLMockCacheTest", "Test JPL mock cache operations", {"unit", "mock"}}) {}

  void run() override {
    auto mock = JPLMockFactory::create_default();

    // Test initial cache state
    auto cache_result = mock->mock_load_from_cache();
    assert_false(SolarSystem::JPL::is_success(cache_result), "Initial cache load should fail");

    auto validation_result = mock->mock_validate_cache();
    assert_true(SolarSystem::JPL::is_success(validation_result), "Cache validation should succeed");
    assert_false(SolarSystem::JPL::get_value(validation_result),
                 "Cache should initially be invalid");

    // Create some test data
    std::vector<SolarSystem::JPL::EphemerisData> test_data;
    SolarSystem::JPL::EphemerisData earth_data;
    earth_data.jpl_id = 399;
    earth_data.body_name = "Earth";
    earth_data.epoch = std::chrono::system_clock::now();
    earth_data.position = SolarSystem::Math::Vector3d{1.0e8, 2.0e8, 3.0e8};
    earth_data.velocity = SolarSystem::Math::Vector3d{10.0, 20.0, 30.0};
    earth_data.mass = 5.97219e24;
    test_data.push_back(earth_data);

    // Test cache save
    auto save_result = mock->mock_save_to_cache(test_data);
    assert_true(SolarSystem::JPL::is_success(save_result), "Cache save should succeed");

    // Test cache load after save
    cache_result = mock->mock_load_from_cache();
    assert_true(SolarSystem::JPL::is_success(cache_result), "Cache load should succeed after save");

    const auto& loaded_data = SolarSystem::JPL::get_value(cache_result);
    assert_equals(static_cast<size_t>(1), loaded_data.size(), "Should load one body");
    assert_equals(399, loaded_data[0].jpl_id, "Should load Earth data");
    assert_equals(std::string("Earth"), loaded_data[0].body_name, "Should preserve body name");

    // Test cache validation after save
    validation_result = mock->mock_validate_cache();
    assert_true(SolarSystem::JPL::is_success(validation_result), "Cache validation should succeed");
    assert_true(SolarSystem::JPL::get_value(validation_result), "Cache should be valid after save");
  }
};

/**
 * @brief Test JPL mock factory methods
 */
class JPLMockFactoryTest : public TestCase {
 public:
  JPLMockFactoryTest()
      : TestCase({"JPLMockFactoryTest", "Test JPL mock factory methods", {"unit", "mock"}}) {}

  void run() override {
    // Test default factory
    auto default_mock = JPLMockFactory::create_default();
    assert_true(default_mock != nullptr, "Default factory should create valid mock");
    assert_equals(static_cast<size_t>(0), default_mock->call_count(),
                  "Default mock should have no calls initially");

    // Test network failure testing factory
    auto failure_mock = JPLMockFactory::create_for_network_failure_testing();
    assert_true(failure_mock != nullptr, "Network failure factory should create valid mock");

    // Test performance testing factory
    auto perf_mock = JPLMockFactory::create_for_performance_testing();
    assert_true(perf_mock != nullptr, "Performance factory should create valid mock");

    // Test realistic responses factory
    auto realistic_mock = JPLMockFactory::create_with_realistic_responses();
    assert_true(realistic_mock != nullptr, "Realistic responses factory should create valid mock");

    // Test custom configuration factory
    JPLMockConfig custom_config;
    custom_config.simulate_network_delays = false;
    custom_config.use_realistic_responses = false;

    auto custom_mock = JPLMockFactory::create(custom_config);
    assert_true(custom_mock != nullptr, "Custom factory should create valid mock");
    assert_false(custom_mock->config().simulate_network_delays, "Custom config should be applied");
  }
};

// Register all tests
SOLAR_REGISTER_TEST_WITH_TAGS(JPLMockBasicTest, "unit", "mock");
SOLAR_REGISTER_TEST_WITH_TAGS(JPLMockResponseConfigTest, "unit", "mock");
SOLAR_REGISTER_TEST_WITH_TAGS(JPLMockFailureTest, "unit", "mock");
SOLAR_REGISTER_TEST_WITH_TAGS(JPLMockCallVerificationTest, "unit", "mock");
SOLAR_REGISTER_TEST_WITH_TAGS(JPLMockNetworkDelayTest, "unit", "mock");
SOLAR_REGISTER_TEST_WITH_TAGS(JPLMockCacheTest, "unit", "mock");
SOLAR_REGISTER_TEST_WITH_TAGS(JPLMockFactoryTest, "unit", "mock");

SOLAR_TEST_MAIN();
