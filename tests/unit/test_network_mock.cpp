/**
 * @file test_network_mock.cpp
 * @brief Unit tests for NetworkMock functionality
 */

#include <chrono>
#include <thread>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;
using namespace SolarSystem::Testing::Mocks;

/**
 * @brief Test basic network mock functionality
 */
class NetworkMockBasicTest : public TestCase {
 public:
  NetworkMockBasicTest()
      : TestCase({"NetworkMockBasicTest",
                  "Test basic network mock operations",
                  {"unit", "network_mock"},
                  std::chrono::seconds(30),
                  false,
                  false,
                  ""}) {}

  void run() override {
    NetworkMockConfig config;
    config.simulate_real_delays = false;  // Don't actually delay

    NetworkMock network_mock(config);

    // Test default HTTP GET
    auto response = network_mock.mock_http_get("http://example.com/test");
    assert_equals(200, response.status_code, "Default response should be 200 OK");
    assert_equals(std::string("OK"), response.status_message,
                  "Default status message should be OK");
    assert_false(response.body.empty(), "Response body should not be empty");

    // Test operation tracking
    assert_equals(static_cast<size_t>(1), network_mock.operation_count(),
                  "Should have recorded one operation");
    assert_equals(static_cast<size_t>(1),
                  network_mock.operation_count(MockNetworkOperationType::HttpGet),
                  "Should have one GET operation");

    // Test URL tracking
    assert_true(network_mock.was_url_requested("http://example.com/test"),
                "URL should be marked as requested");
    assert_false(network_mock.was_url_requested("http://other.com"),
                 "Other URL should not be requested");
  }
};

/**
 * @brief Test network condition simulation
 */
class NetworkConditionTest : public TestCase {
 public:
  NetworkConditionTest()
      : TestCase({"NetworkConditionTest",
                  "Test network condition simulation",
                  {"unit", "network_mock"},
                  std::chrono::seconds(30),
                  false,
                  false,
                  ""}) {}

  void run() override {
    NetworkMock network_mock;

    // Test slow network simulation
    network_mock.simulate_slow_network(56000);  // 56k modem speed
    auto condition = network_mock.current_condition();
    assert_equals(static_cast<size_t>(56000), condition.bandwidth_bps,
                  "Bandwidth should be set to 56k");
    assert_true(condition.simulate_bandwidth_limit, "Bandwidth limiting should be enabled");

    // Test high latency simulation
    network_mock.simulate_high_latency(std::chrono::milliseconds(500));
    condition = network_mock.current_condition();
    assert_equals(500, static_cast<int>(condition.base_latency.count()),
                  "Base latency should be 500ms");

    // Test unstable network simulation
    network_mock.simulate_unstable_network(0.1);  // 10% packet loss
    condition = network_mock.current_condition();
    assert_equals(0.1, condition.packet_loss_rate, "Packet loss rate should be 10%");
    assert_true(condition.simulate_packet_loss, "Packet loss simulation should be enabled");

    // Test mobile network simulation
    network_mock.simulate_mobile_network();
    condition = network_mock.current_condition();
    assert_equals(std::string("mobile"), condition.connection_type,
                  "Connection type should be mobile");

    // Test reset to normal conditions
    network_mock.reset_to_normal_conditions();
    condition = network_mock.current_condition();
    assert_equals(std::string("broadband"), condition.connection_type, "Should reset to broadband");
  }
};

/**
 * @brief Test custom response configuration
 */
class NetworkResponseTest : public TestCase {
 public:
  NetworkResponseTest()
      : TestCase({"NetworkResponseTest",
                  "Test custom response configuration",
                  {"unit", "network_mock"},
                  std::chrono::seconds(30),
                  false,
                  false,
                  ""}) {}

  void run() override {
    NetworkMock network_mock;

    // Set custom response for specific URL
    MockHttpResponse custom_response;
    custom_response.status_code = 404;
    custom_response.status_message = "Not Found";
    custom_response.body = "Custom 404 response";
    custom_response.headers["Content-Type"] = "text/plain";

    network_mock.set_response_for_url("http://example.com/notfound", custom_response);

    // Test custom response
    auto response = network_mock.mock_http_get("http://example.com/notfound");
    assert_equals(404, response.status_code, "Should return custom 404 status");
    assert_equals(std::string("Not Found"), response.status_message,
                  "Should return custom status message");
    assert_equals(std::string("Custom 404 response"), response.body, "Should return custom body");
    assert_equals(std::string("text/plain"), response.headers.at("Content-Type"),
                  "Should return custom headers");

    // Test default response for other URLs
    auto default_response = network_mock.mock_http_get("http://example.com/other");
    assert_equals(200, default_response.status_code, "Other URLs should return default response");
  }
};

/**
 * @brief Test HTTP method support
 */
class NetworkHttpMethodTest : public TestCase {
 public:
  NetworkHttpMethodTest()
      : TestCase({"NetworkHttpMethodTest",
                  "Test HTTP method support",
                  {"unit", "network_mock"},
                  std::chrono::seconds(30),
                  false,
                  false,
                  ""}) {}

  void run() override {
    NetworkMock network_mock;

    // Test GET
    auto get_response = network_mock.mock_http_get("http://example.com/get");
    assert_equals(200, get_response.status_code, "GET should work");

    // Test POST
    auto post_response = network_mock.mock_http_post("http://example.com/post", "test data");
    assert_equals(200, post_response.status_code, "POST should work");

    // Test PUT
    auto put_response = network_mock.mock_http_put("http://example.com/put", "update data");
    assert_equals(200, put_response.status_code, "PUT should work");

    // Test DELETE
    auto delete_response = network_mock.mock_http_delete("http://example.com/delete");
    assert_equals(200, delete_response.status_code, "DELETE should work");

    // Verify operation counts
    assert_equals(static_cast<size_t>(1),
                  network_mock.operation_count(MockNetworkOperationType::HttpGet),
                  "Should have one GET");
    assert_equals(static_cast<size_t>(1),
                  network_mock.operation_count(MockNetworkOperationType::HttpPost),
                  "Should have one POST");
    assert_equals(static_cast<size_t>(1),
                  network_mock.operation_count(MockNetworkOperationType::HttpPut),
                  "Should have one PUT");
    assert_equals(static_cast<size_t>(1),
                  network_mock.operation_count(MockNetworkOperationType::HttpDelete),
                  "Should have one DELETE");
  }
};

/**
 * @brief Test connection management
 */
class NetworkConnectionTest : public TestCase {
 public:
  NetworkConnectionTest()
      : TestCase({"NetworkConnectionTest",
                  "Test connection management",
                  {"unit", "network_mock"},
                  std::chrono::seconds(30),
                  false,
                  false,
                  ""}) {}

  void run() override {
    NetworkMock network_mock;

    // Test connection establishment
    bool connected = network_mock.mock_connect("example.com", 80);
    assert_true(connected, "Should be able to connect");
    assert_true(network_mock.is_connected("example.com", 80), "Should be marked as connected");
    assert_equals(static_cast<size_t>(1), network_mock.active_connection_count(),
                  "Should have one active connection");

    // Test multiple connections
    bool connected2 = network_mock.mock_connect("other.com", 443);
    assert_true(connected2, "Second connection should succeed");
    assert_equals(static_cast<size_t>(2), network_mock.active_connection_count(),
                  "Should have two active connections");

    // Test disconnection
    network_mock.mock_disconnect("example.com", 80);
    assert_false(network_mock.is_connected("example.com", 80),
                 "Should not be connected after disconnect");
    assert_equals(static_cast<size_t>(1), network_mock.active_connection_count(),
                  "Should have one active connection after disconnect");
  }
};

/**
 * @brief Test network mock factory functionality
 */
class NetworkMockFactoryTest : public TestCase {
 public:
  NetworkMockFactoryTest()
      : TestCase({"NetworkMockFactoryTest",
                  "Test network mock factory methods",
                  {"unit", "network_mock"},
                  std::chrono::seconds(30),
                  false,
                  false,
                  ""}) {}

  void run() override {
    // Test default factory
    auto default_mock = NetworkMockFactory::create_default();
    assert_true(default_mock.get() != nullptr, "Default mock should be created");

    // Test connection failure testing factory
    auto failure_mock = NetworkMockFactory::create_for_connection_failure_testing();
    assert_true(failure_mock.get() != nullptr, "Connection failure mock should be created");

    // Test slow network testing factory
    auto slow_mock = NetworkMockFactory::create_for_slow_network_testing();
    assert_true(slow_mock.get() != nullptr, "Slow network mock should be created");

    // Test timeout testing factory
    auto timeout_mock = NetworkMockFactory::create_for_timeout_testing();
    assert_true(timeout_mock.get() != nullptr, "Timeout mock should be created");

    // Test mobile network factory
    auto mobile_mock = NetworkMockFactory::create_for_mobile_network();
    assert_true(mobile_mock.get() != nullptr, "Mobile network mock should be created");
    auto condition = mobile_mock->current_condition();
    assert_equals(std::string("mobile"), condition.connection_type,
                  "Mobile mock should have mobile connection type");

    // Test perfect conditions factory
    auto perfect_mock = NetworkMockFactory::create_with_perfect_conditions();
    assert_true(perfect_mock.get() != nullptr, "Perfect conditions mock should be created");
  }
};

// Register all tests
SOLAR_REGISTER_TEST_WITH_TAGS(NetworkMockBasicTest, "unit", "network_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(NetworkConditionTest, "unit", "network_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(NetworkResponseTest, "unit", "network_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(NetworkHttpMethodTest, "unit", "network_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(NetworkConnectionTest, "unit", "network_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(NetworkMockFactoryTest, "unit", "network_mock");

SOLAR_TEST_MAIN();
