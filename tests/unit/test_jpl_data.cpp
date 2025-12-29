/**
 * @file test_jpl_data.cpp
 * @brief Comprehensive unit tests for JPL data structures and operations
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>

#include "solar_jpl/jpl_client.hpp"

using namespace SolarSystem::JPL;
using namespace SolarSystem::Math;

// ============================================================================
// EphemerisData Tests
// ============================================================================

TEST(EphemerisData, Construction) {
  EphemerisData data;
  data.body_name = "Earth";
  data.jpl_id = 399;
  data.epoch = std::chrono::system_clock::now();
  data.position = Vector3d{1.496e11, 0.0, 0.0};
  data.velocity = Vector3d{0.0, 29780.0, 0.0};
  data.mass = 5.97219e24;

  EXPECT_EQ("Earth", data.body_name);
  EXPECT_EQ(399, data.jpl_id);
  EXPECT_GT(data.mass, 0.0);
}

TEST(EphemerisData, ToCelestialBodyConversion) {
  EphemerisData data;
  data.body_name = "Mars";
  data.jpl_id = 499;
  data.epoch = std::chrono::system_clock::now();
  data.position = Vector3d{2.279e11, 0.0, 0.0};
  data.velocity = Vector3d{0.0, 24077.0, 0.0};
  data.mass = 6.41693e23;

  auto body = data.to_celestial_body();
  EXPECT_EQ("Mars", body.name());
  EXPECT_DOUBLE_EQ(6.41693e23, body.mass());
  EXPECT_GT(body.position().magnitude(), 0.0);
  EXPECT_GT(body.velocity().magnitude(), 0.0);
}

// ============================================================================
// JPLResult Tests
// ============================================================================

TEST(JPLResult, SuccessResult) {
  JPLResult<int> success_result = 42;
  EXPECT_TRUE(is_success(success_result));
  EXPECT_EQ(42, get_value(success_result));
}

TEST(JPLResult, ErrorResult) {
  JPLResult<int> error_result = JPLError::NetworkError;
  EXPECT_FALSE(is_success(error_result));
  auto error = get_error(error_result);
  EXPECT_EQ(JPLError::NetworkError, error);
}

TEST(JPLVoidResult, SuccessVoidResult) {
  JPLVoidResult success_void = success();
  EXPECT_TRUE(is_success(success_void));
}

TEST(JPLVoidResult, ErrorVoidResult) {
  JPLVoidResult error_void = error(JPLError::CacheError);
  EXPECT_FALSE(is_success(error_void));
  EXPECT_EQ(JPLError::CacheError, error_void.value());
}

// ============================================================================
// JPLClientConfig Tests
// ============================================================================

TEST(JPLClientConfig, DefaultValues) {
  JPLClientConfig config;

  EXPECT_EQ("https://ssd.jpl.nasa.gov/api/horizons.api", config.api_endpoint);
  EXPECT_EQ(std::chrono::seconds(30), config.request_timeout);
  EXPECT_EQ(3, config.max_concurrent_requests);
  EXPECT_EQ(3, config.max_retries);
  EXPECT_TRUE(config.enable_exponential_backoff);
  EXPECT_TRUE(config.enable_circuit_breaker);
  EXPECT_TRUE(config.enable_binary_cache);
  EXPECT_TRUE(config.enable_json_cache);
}

TEST(JPLClientConfig, ValidationValidConfig) {
  JPLClientConfig config;
  std::string error;

  EXPECT_TRUE(config.is_valid(&error));
  EXPECT_TRUE(error.empty());
}

TEST(JPLClientConfig, ValidationInvalidTimeout) {
  JPLClientConfig config;
  config.request_timeout = std::chrono::seconds(-1);
  std::string error;

  EXPECT_FALSE(config.is_valid(&error));
  EXPECT_FALSE(error.empty());
}

TEST(JPLClientConfig, ValidationInvalidMaxRetries) {
  JPLClientConfig config;
  config.max_retries = 0;
  std::string error;

  EXPECT_FALSE(config.is_valid(&error));
  EXPECT_FALSE(error.empty());
}

// ============================================================================
// CacheMetadata Tests
// ============================================================================

TEST(CacheMetadata, ValidityCheckFreshCache) {
  CacheMetadata metadata;
  metadata.created_at = std::chrono::system_clock::now();
  metadata.epoch = std::chrono::system_clock::now();
  metadata.source = "JPL HORIZONS";
  metadata.body_count = 27;
  metadata.checksum = 12345;

  EXPECT_TRUE(metadata.is_valid(std::chrono::hours(24 * 30)));
}

TEST(CacheMetadata, ValidityCheckExpiredCache) {
  CacheMetadata metadata;
  metadata.created_at = std::chrono::system_clock::now() - std::chrono::hours(24 * 31);
  metadata.epoch = std::chrono::system_clock::now();
  metadata.source = "JPL HORIZONS";
  metadata.body_count = 27;
  metadata.checksum = 12345;

  EXPECT_FALSE(metadata.is_valid(std::chrono::hours(24 * 30)));
}

// ============================================================================
// CircuitBreaker Tests
// ============================================================================

TEST(CircuitBreaker, InitialState) {
  CircuitBreaker breaker;

  EXPECT_EQ(CircuitBreakerState::Closed, breaker.state);
  EXPECT_EQ(0u, breaker.failure_count);
}

TEST(CircuitBreaker, ShouldAllowRequestWhenClosed) {
  CircuitBreaker breaker;
  JPLClientConfig config;

  EXPECT_TRUE(breaker.should_allow_request(config));
}

TEST(CircuitBreaker, RecordSuccess) {
  CircuitBreaker breaker;
  breaker.failure_count = 3;

  breaker.record_success();

  EXPECT_EQ(0u, breaker.failure_count);
  EXPECT_EQ(CircuitBreakerState::Closed, breaker.state);
}

TEST(CircuitBreaker, RecordFailure) {
  CircuitBreaker breaker;
  JPLClientConfig config;

  breaker.record_failure(config);

  EXPECT_EQ(1u, breaker.failure_count);
}

TEST(CircuitBreaker, OpensAfterThreshold) {
  CircuitBreaker breaker;
  JPLClientConfig config;
  config.circuit_breaker_failure_threshold = 3;

  for (size_t i = 0; i < config.circuit_breaker_failure_threshold; ++i) {
    breaker.record_failure(config);
  }

  EXPECT_EQ(CircuitBreakerState::Open, breaker.state);
}

// ============================================================================
// NetworkDiagnostics Tests
// ============================================================================

TEST(NetworkDiagnostics, SuccessRateCalculation) {
  NetworkDiagnostics diagnostics;
  diagnostics.successful_requests = 80;
  diagnostics.failed_requests = 20;

  EXPECT_DOUBLE_EQ(0.8, diagnostics.success_rate());
}

TEST(NetworkDiagnostics, SuccessRateNoRequests) {
  NetworkDiagnostics diagnostics;

  EXPECT_DOUBLE_EQ(0.0, diagnostics.success_rate());
}

TEST(NetworkDiagnostics, HealthCheckHealthy) {
  NetworkDiagnostics diagnostics;
  diagnostics.connectivity_status = NetworkConnectivityStatus::Connected;
  diagnostics.successful_requests = 90;
  diagnostics.failed_requests = 10;
  diagnostics.packet_loss_rate = 0.05;

  EXPECT_TRUE(diagnostics.is_healthy());
}

TEST(NetworkDiagnostics, HealthCheckUnhealthy) {
  NetworkDiagnostics diagnostics;
  diagnostics.connectivity_status = NetworkConnectivityStatus::Limited;
  diagnostics.successful_requests = 50;
  diagnostics.failed_requests = 50;
  diagnostics.packet_loss_rate = 0.3;

  EXPECT_FALSE(diagnostics.is_healthy());
}

// ============================================================================
// Utility Function Tests
// ============================================================================

TEST(JPLUtils, DateStringConversion) {
  auto now = std::chrono::system_clock::now();
  std::string date_str = Utils::to_jpl_date_string(now);

  EXPECT_FALSE(date_str.empty());
  EXPECT_NE(std::string::npos, date_str.find("-"));
}

TEST(JPLUtils, GetCurrentYearEpoch) {
  auto epoch = Utils::get_current_year_epoch();
  auto now = std::chrono::system_clock::now();

  EXPECT_LE(epoch, now);
}

TEST(JPLUtils, GetJPLIdForBodyValid) {
  auto earth_id = Utils::get_jpl_id_for_body("Earth");
  ASSERT_TRUE(earth_id.has_value());
  EXPECT_EQ(399, earth_id.value());

  auto mars_id = Utils::get_jpl_id_for_body("Mars");
  ASSERT_TRUE(mars_id.has_value());
  EXPECT_EQ(499, mars_id.value());
}

TEST(JPLUtils, GetJPLIdForBodyInvalid) {
  auto invalid_id = Utils::get_jpl_id_for_body("NonExistentPlanet");
  EXPECT_FALSE(invalid_id.has_value());
}

TEST(JPLUtils, GetAllJPLIds) {
  auto all_ids = Utils::get_all_jpl_ids();

  EXPECT_GT(all_ids.size(), 0u);
  EXPECT_GE(all_ids.size(), 8u);
}

TEST(JPLUtils, JPLErrorToString) {
  auto network_str = Utils::to_string(JPLError::NetworkError);
  EXPECT_FALSE(network_str.empty());

  auto parse_str = Utils::to_string(JPLError::ParseError);
  EXPECT_FALSE(parse_str.empty());

  auto body_str = Utils::to_string(JPLError::InvalidBody);
  EXPECT_FALSE(body_str.empty());

  auto cache_str = Utils::to_string(JPLError::CacheError);
  EXPECT_FALSE(cache_str.empty());
}

TEST(JPLUtils, NetworkConnectivityStatusToString) {
  EXPECT_EQ("Connected", Utils::to_string(NetworkConnectivityStatus::Connected));
  EXPECT_EQ("Limited", Utils::to_string(NetworkConnectivityStatus::Limited));
  EXPECT_EQ("Disconnected", Utils::to_string(NetworkConnectivityStatus::Disconnected));
  EXPECT_EQ("Unknown", Utils::to_string(NetworkConnectivityStatus::Unknown));
}
