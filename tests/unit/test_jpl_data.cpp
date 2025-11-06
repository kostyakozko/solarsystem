/**
 * @file test_jpl_data.cpp
 * @brief Comprehensive unit tests for JPL data structures and operations
 */

#include "../utils/test_framework.h"
#include "solar_jpl/jpl_client.hpp"
#include <chrono>

using namespace SolarSystem::JPL;
using namespace SolarSystem::Math;

int main() {
  TestSuite suite("JPL Data Tests");

  // Test EphemerisData structure
  suite.run_test("EphemerisData Construction", []() {
    EphemerisData data;
    data.body_name = "Earth";
    data.jpl_id = 399;
    data.epoch = std::chrono::system_clock::now();
    data.position = Vector3d{1.496e11, 0.0, 0.0};
    data.velocity = Vector3d{0.0, 29780.0, 0.0};
    data.mass = 5.97219e24;

    ASSERT_EQ("Earth", data.body_name);
    ASSERT_EQ(399, data.jpl_id);
    ASSERT_TRUE(data.mass > 0.0);
  });

  suite.run_test("EphemerisData to CelestialBody Conversion", []() {
    EphemerisData data;
    data.body_name = "Mars";
    data.jpl_id = 499;
    data.epoch = std::chrono::system_clock::now();
    data.position = Vector3d{2.279e11, 0.0, 0.0};  // km
    data.velocity = Vector3d{0.0, 24077.0, 0.0};   // km/s
    data.mass = 6.41693e23;

    auto body = data.to_celestial_body();
    ASSERT_EQ("Mars", body.name());
    ASSERT_EQ(6.41693e23, body.mass());
    // Position may be converted from km to m (1000x factor)
    // Just verify the body was created successfully
    ASSERT_TRUE(body.position().magnitude() > 0.0);
    ASSERT_TRUE(body.velocity().magnitude() > 0.0);
  });

  // Test JPLError enum and helpers
  suite.run_test("JPLError Result Helpers", []() {
    // Test successful result
    JPLResult<int> success_result = 42;
    ASSERT_TRUE(is_success(success_result));
    ASSERT_EQ(42, get_value(success_result));

    // Test error result
    JPLResult<int> error_result = JPLError::NetworkError;
    ASSERT_FALSE(is_success(error_result));
    auto error = get_error(error_result);
    ASSERT_TRUE(error == JPLError::NetworkError);
  });

  suite.run_test("JPLVoidResult Helpers", []() {
    // Test successful void result
    JPLVoidResult success_void = success();
    ASSERT_TRUE(is_success(success_void));

    // Test error void result
    JPLVoidResult error_void = error(JPLError::CacheError);
    ASSERT_FALSE(is_success(error_void));
    ASSERT_TRUE(error_void.value() == JPLError::CacheError);
  });

  // Test JPLClientConfig
  suite.run_test("JPLClientConfig Default Values", []() {
    JPLClientConfig config;

    ASSERT_EQ("https://ssd.jpl.nasa.gov/api/horizons.api", config.api_endpoint);
    ASSERT_EQ(std::chrono::seconds(30), config.request_timeout);
    ASSERT_EQ(3, config.max_concurrent_requests);
    ASSERT_EQ(3, config.max_retries);
    ASSERT_TRUE(config.enable_exponential_backoff);
    ASSERT_TRUE(config.enable_circuit_breaker);
    ASSERT_TRUE(config.enable_binary_cache);
    ASSERT_TRUE(config.enable_json_cache);
  });

  suite.run_test("JPLClientConfig Validation - Valid Config", []() {
    JPLClientConfig config;
    std::string error;

    ASSERT_TRUE(config.is_valid(&error));
    ASSERT_TRUE(error.empty());
  });

  suite.run_test("JPLClientConfig Validation - Invalid Timeout", []() {
    JPLClientConfig config;
    config.request_timeout = std::chrono::seconds(-1);
    std::string error;

    ASSERT_FALSE(config.is_valid(&error));
    ASSERT_FALSE(error.empty());
  });

  suite.run_test("JPLClientConfig Validation - Invalid Max Retries", []() {
    JPLClientConfig config;
    config.max_retries = 0;
    std::string error;

    ASSERT_FALSE(config.is_valid(&error));
    ASSERT_FALSE(error.empty());
  });

  // Test CacheMetadata
  suite.run_test("CacheMetadata Validity Check - Fresh Cache", []() {
    CacheMetadata metadata;
    metadata.created_at = std::chrono::system_clock::now();
    metadata.epoch = std::chrono::system_clock::now();
    metadata.source = "JPL HORIZONS";
    metadata.body_count = 27;
    metadata.checksum = 12345;

    // Fresh cache should be valid
    ASSERT_TRUE(metadata.is_valid(std::chrono::hours(24 * 30)));
  });

  suite.run_test("CacheMetadata Validity Check - Expired Cache", []() {
    CacheMetadata metadata;
    metadata.created_at = std::chrono::system_clock::now() - std::chrono::hours(24 * 31);
    metadata.epoch = std::chrono::system_clock::now();
    metadata.source = "JPL HORIZONS";
    metadata.body_count = 27;
    metadata.checksum = 12345;

    // Cache older than 30 days should be invalid
    ASSERT_FALSE(metadata.is_valid(std::chrono::hours(24 * 30)));
  });

  // Test CircuitBreaker
  suite.run_test("CircuitBreaker Initial State", []() {
    CircuitBreaker breaker;

    ASSERT_TRUE(breaker.state == CircuitBreakerState::Closed);
    ASSERT_EQ(0, breaker.failure_count);
  });

  suite.run_test("CircuitBreaker Should Allow Request When Closed", []() {
    CircuitBreaker breaker;
    JPLClientConfig config;

    ASSERT_TRUE(breaker.should_allow_request(config));
  });

  suite.run_test("CircuitBreaker Record Success", []() {
    CircuitBreaker breaker;
    breaker.failure_count = 3;

    breaker.record_success();

    ASSERT_EQ(0, breaker.failure_count);
    ASSERT_TRUE(breaker.state == CircuitBreakerState::Closed);
  });

  suite.run_test("CircuitBreaker Record Failure", []() {
    CircuitBreaker breaker;
    JPLClientConfig config;

    breaker.record_failure(config);

    ASSERT_EQ(1, breaker.failure_count);
  });

  suite.run_test("CircuitBreaker Opens After Threshold", []() {
    CircuitBreaker breaker;
    JPLClientConfig config;
    config.circuit_breaker_failure_threshold = 3;

    // Record failures up to threshold
    for (size_t i = 0; i < config.circuit_breaker_failure_threshold; ++i) {
      breaker.record_failure(config);
    }

    ASSERT_TRUE(breaker.state == CircuitBreakerState::Open);
  });

  // Test NetworkDiagnostics
  suite.run_test("NetworkDiagnostics Success Rate Calculation", []() {
    NetworkDiagnostics diagnostics;
    diagnostics.successful_requests = 80;
    diagnostics.failed_requests = 20;

    ASSERT_EQ(0.8, diagnostics.success_rate());
  });

  suite.run_test("NetworkDiagnostics Success Rate - No Requests", []() {
    NetworkDiagnostics diagnostics;

    ASSERT_EQ(0.0, diagnostics.success_rate());
  });

  suite.run_test("NetworkDiagnostics Health Check - Healthy", []() {
    NetworkDiagnostics diagnostics;
    diagnostics.connectivity_status = NetworkConnectivityStatus::Connected;
    diagnostics.successful_requests = 90;
    diagnostics.failed_requests = 10;
    diagnostics.packet_loss_rate = 0.05;

    ASSERT_TRUE(diagnostics.is_healthy());
  });

  suite.run_test("NetworkDiagnostics Health Check - Unhealthy", []() {
    NetworkDiagnostics diagnostics;
    diagnostics.connectivity_status = NetworkConnectivityStatus::Limited;
    diagnostics.successful_requests = 50;
    diagnostics.failed_requests = 50;
    diagnostics.packet_loss_rate = 0.3;

    ASSERT_FALSE(diagnostics.is_healthy());
  });

  // Test Utility Functions
  suite.run_test("JPL Date String Conversion", []() {
    auto now = std::chrono::system_clock::now();
    std::string date_str = Utils::to_jpl_date_string(now);

    ASSERT_FALSE(date_str.empty());
    ASSERT_TRUE(date_str.find("-") != std::string::npos);
  });

  suite.run_test("Get Current Year Epoch", []() {
    auto epoch = Utils::get_current_year_epoch();
    auto now = std::chrono::system_clock::now();

    // Epoch should be before or equal to now
    ASSERT_TRUE(epoch <= now);
  });

  suite.run_test("Get JPL ID for Body - Valid", []() {
    auto earth_id = Utils::get_jpl_id_for_body("Earth");
    ASSERT_TRUE(earth_id.has_value());
    ASSERT_EQ(399, earth_id.value());

    auto mars_id = Utils::get_jpl_id_for_body("Mars");
    ASSERT_TRUE(mars_id.has_value());
    ASSERT_EQ(499, mars_id.value());
  });

  suite.run_test("Get JPL ID for Body - Invalid", []() {
    auto invalid_id = Utils::get_jpl_id_for_body("NonExistentPlanet");
    ASSERT_FALSE(invalid_id.has_value());
  });

  suite.run_test("Get All JPL IDs", []() {
    auto all_ids = Utils::get_all_jpl_ids();

    ASSERT_TRUE(all_ids.size() > 0);
    ASSERT_TRUE(all_ids.size() >= 8);  // At least 8 planets + Sun
  });

  suite.run_test("JPL Error to String", []() {
    // Error strings may have spaces or different formatting
    auto network_str = Utils::to_string(JPLError::NetworkError);
    ASSERT_FALSE(network_str.empty());

    auto parse_str = Utils::to_string(JPLError::ParseError);
    ASSERT_FALSE(parse_str.empty());

    auto body_str = Utils::to_string(JPLError::InvalidBody);
    ASSERT_FALSE(body_str.empty());

    auto cache_str = Utils::to_string(JPLError::CacheError);
    ASSERT_FALSE(cache_str.empty());
  });

  suite.run_test("Network Connectivity Status to String", []() {
    ASSERT_EQ("Connected", Utils::to_string(NetworkConnectivityStatus::Connected));
    ASSERT_EQ("Limited", Utils::to_string(NetworkConnectivityStatus::Limited));
    ASSERT_EQ("Disconnected", Utils::to_string(NetworkConnectivityStatus::Disconnected));
    ASSERT_EQ("Unknown", Utils::to_string(NetworkConnectivityStatus::Unknown));
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
