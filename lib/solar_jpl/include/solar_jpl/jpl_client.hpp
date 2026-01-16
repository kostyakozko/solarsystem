/**
 * @file jpl_client.hpp
 * @brief Modern C++20 JPL HORIZONS API Client
 *
 * Provides a modern, type-safe interface to NASA JPL HORIZONS system with:
 * - RAII-based resource management
 * - Async/await patterns for concurrent data fetching
 * - Type-safe configuration and error handling
 * - Integration with Phase 0.3 body management
 * - Structured caching with validation
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "solar_core/bodies/body_mappings.hpp"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_jpl/export.hpp"

// Forward declarations
namespace SolarSystem::JPL {
class CacheManager;
class DataValidator;
}  // namespace SolarSystem::JPL

namespace SolarSystem::JPL {

/**
 * @brief JPL HORIZONS API error types
 */
enum class JPLError {
  NetworkError,
  ParseError,
  InvalidBody,
  InvalidDate,
  RateLimited,
  ServerError,
  CacheError,
  ValidationError
};

/**
 * @brief JPL API result type (C++20 compatible)
 */
template <typename T>
using JPLResult = std::variant<T, JPLError>;

/**
 * @brief Helper to check if result is successful
 */
template <typename T>
[[nodiscard]] bool is_success(const JPLResult<T>& result) {
  return std::holds_alternative<T>(result);
}

/**
 * @brief Helper to get value from successful result
 */
template <typename T>
[[nodiscard]] const T& get_value(const JPLResult<T>& result) {
  return std::get<T>(result);
}

/**
 * @brief Helper to get error from failed result
 */
template <typename T>
[[nodiscard]] JPLError get_error(const JPLResult<T>& result) {
  return std::get<JPLError>(result);
}

/**
 * @brief JPL API result type for void operations
 */
using JPLVoidResult = std::optional<JPLError>;

/**
 * @brief Helper to create successful void result
 */
[[nodiscard]] inline JPLVoidResult success() { return std::nullopt; }

/**
 * @brief Helper to create error void result
 */
[[nodiscard]] inline JPLVoidResult error(JPLError err) { return err; }

/**
 * @brief Helper to check if void result is successful
 */
[[nodiscard]] inline bool is_success(const JPLVoidResult& result) { return !result.has_value(); }

/**
 * @brief Ephemeris data for a single celestial body
 */
struct SOLAR_JPL_API EphemerisData {
  std::string body_name;
  int jpl_id;
  std::chrono::system_clock::time_point epoch;
  SolarSystem::Math::Vector3d position;  // km
  SolarSystem::Math::Vector3d velocity;  // km/s
  long double mass;                      // kg

  /**
   * @brief Convert to modern CelestialBody
   */
  [[nodiscard]] SolarSystem::Bodies::CelestialBody to_celestial_body() const;
};

/**
 * @brief Configuration for JPL client
 */
struct SOLAR_JPL_API JPLClientConfig {
  std::string api_endpoint = "https://ssd.jpl.nasa.gov/api/horizons.api";
  std::chrono::seconds request_timeout = std::chrono::seconds(30);
  std::chrono::milliseconds request_delay = std::chrono::milliseconds(200);
  size_t max_concurrent_requests = 3;
  size_t max_retries = 3;
  std::chrono::milliseconds retry_delay = std::chrono::milliseconds(500);

  // Enhanced network resilience configuration
  bool enable_exponential_backoff = true;
  double backoff_multiplier = 2.0;
  std::chrono::milliseconds max_backoff_delay = std::chrono::milliseconds(30000);  // 30 seconds
  bool enable_circuit_breaker = true;
  size_t circuit_breaker_failure_threshold = 5;
  std::chrono::minutes circuit_breaker_timeout = std::chrono::minutes(5);
  bool enable_connection_pooling = true;
  size_t connection_pool_size = 5;
  std::chrono::seconds connection_keep_alive = std::chrono::seconds(300);  // 5 minutes

  // Fallback configuration
  std::vector<std::string> fallback_endpoints;
  bool enable_offline_mode = true;
  bool prefer_cache_on_network_failure = true;

  // Cache configuration
  std::filesystem::path cache_directory = "./cache";
  bool enable_binary_cache = true;
  bool enable_json_cache = true;
  std::chrono::hours cache_validity = std::chrono::hours(24 * 30);  // 30 days

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] bool is_valid(std::string* error = nullptr) const;
};

/**
 * @brief Circuit breaker states for network resilience
 */
enum class CircuitBreakerState {
  Closed,   // Normal operation
  Open,     // Failing, requests blocked
  HalfOpen  // Testing if service recovered
};

/**
 * @brief Network connectivity status
 */
enum class NetworkConnectivityStatus {
  Connected,     // Full network connectivity
  Limited,       // Limited connectivity (some endpoints reachable)
  Disconnected,  // No network connectivity
  Unknown        // Connectivity status unknown
};

/**
 * @brief Network diagnostics information
 */
struct NetworkDiagnostics {
  NetworkConnectivityStatus connectivity_status = NetworkConnectivityStatus::Unknown;
  std::chrono::system_clock::time_point last_check_time;
  std::chrono::milliseconds average_response_time{0};
  std::chrono::milliseconds min_response_time{std::chrono::milliseconds::max()};
  std::chrono::milliseconds max_response_time{0};
  size_t successful_requests = 0;
  size_t failed_requests = 0;
  size_t timeout_requests = 0;
  double packet_loss_rate = 0.0;
  std::vector<std::string> reachable_endpoints;
  std::vector<std::string> unreachable_endpoints;
  std::string last_error_message;

  /**
   * @brief Calculate success rate
   */
  [[nodiscard]] double success_rate() const {
    auto total = successful_requests + failed_requests;
    return total > 0 ? static_cast<double>(successful_requests) / static_cast<double>(total) : 0.0;
  }

  /**
   * @brief Check if network is healthy
   */
  [[nodiscard]] bool is_healthy() const {
    return connectivity_status == NetworkConnectivityStatus::Connected && success_rate() > 0.8 &&
           packet_loss_rate < 0.2;
  }
};

/**
 * @brief Circuit breaker for network failure management
 */
struct SOLAR_JPL_API CircuitBreaker {
  CircuitBreakerState state = CircuitBreakerState::Closed;
  size_t failure_count = 0;
  std::chrono::system_clock::time_point last_failure_time;
  std::chrono::system_clock::time_point next_attempt_time;

  bool should_allow_request(const JPLClientConfig& config) const;
  void record_success();
  void record_failure(const JPLClientConfig& config);
};

/**
 * @brief Connection pool entry for reusing connections
 */
struct ConnectionPoolEntry {
  std::string endpoint;
  std::chrono::system_clock::time_point last_used;
  bool is_available = true;
  size_t active_requests = 0;
};

/**
 * @brief Cache metadata
 */
struct SOLAR_JPL_API CacheMetadata {
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point epoch;
  std::string source;
  size_t body_count;
  uint64_t checksum;

  /**
   * @brief Check if cache is still valid
   */
  [[nodiscard]] bool is_valid(std::chrono::hours max_age) const;
};

/**
 * @brief Modern JPL HORIZONS API client
 */
class SOLAR_JPL_API JPLClient {
 public:
  /**
   * @brief Construct JPL client with configuration
   */
  explicit JPLClient(JPLClientConfig config = {});

  /**
   * @brief Destructor ensures cleanup
   */
  ~JPLClient();

  // Non-copyable and non-movable (due to mutex)
  JPLClient(const JPLClient&) = delete;
  JPLClient& operator=(const JPLClient&) = delete;
  JPLClient(JPLClient&&) = delete;
  JPLClient& operator=(JPLClient&&) = delete;

  /**
   * @brief Fetch ephemeris data for a single body
   */
  [[nodiscard]] std::future<JPLResult<EphemerisData>> fetch_body_async(
      int jpl_id, std::chrono::system_clock::time_point epoch);

  /**
   * @brief Fetch ephemeris data for multiple bodies
   */
  [[nodiscard]] std::future<JPLResult<std::vector<EphemerisData>>> fetch_bodies_async(
      const std::vector<int>& jpl_ids, std::chrono::system_clock::time_point epoch);

  /**
   * @brief Fetch ephemeris data for all known bodies
   */
  [[nodiscard]] std::future<JPLResult<std::vector<EphemerisData>>> fetch_all_bodies_async(
      std::chrono::system_clock::time_point epoch);

  /**
   * @brief Load ephemeris data from cache
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> load_from_cache();

  /**
   * @brief Save ephemeris data to cache
   */
  [[nodiscard]] JPLVoidResult save_to_cache(const std::vector<EphemerisData>& data);

  /**
   * @brief Check if current year data is available
   */
  [[nodiscard]] bool has_current_year_data() const;

  /**
   * @brief Get cache metadata
   */
  [[nodiscard]] std::optional<CacheMetadata> get_cache_metadata() const;

  /**
   * @brief Validate cache integrity
   */
  [[nodiscard]] JPLResult<bool> validate_cache() const;

  /**
   * @brief Clear all cached data
   */
  [[nodiscard]] JPLVoidResult clear_cache();

  [[nodiscard]] JPLVoidResult rebuild_cache();

  /**
   * @brief Get configuration
   */
  [[nodiscard]] const JPLClientConfig& config() const noexcept { return config_; }

  [[nodiscard]] JPLVoidResult test_storage();

  /**
   * @brief Get cache manager for advanced cache operations
   */
  [[nodiscard]] CacheManager& cache_manager() const;

  /**
   * @brief Get data validator for comprehensive validation
   */
  [[nodiscard]] DataValidator& data_validator() const;

  // Network Connectivity and Diagnostics

  /**
   * @brief Check network connectivity status
   */
  [[nodiscard]] NetworkConnectivityStatus check_network_connectivity();

  /**
   * @brief Get comprehensive network diagnostics
   */
  [[nodiscard]] NetworkDiagnostics get_network_diagnostics() const;

  /**
   * @brief Test connectivity to specific endpoint
   */
  [[nodiscard]] JPLResult<std::chrono::milliseconds> test_endpoint_connectivity(
      const std::string& endpoint);

  /**
   * @brief Run comprehensive network diagnostics
   */
  [[nodiscard]] JPLVoidResult run_network_diagnostics();

  /**
   * @brief Enable/disable offline mode
   */
  void set_offline_mode(bool enabled);

  /**
   * @brief Check if currently in offline mode
   */
  [[nodiscard]] bool is_offline_mode() const;

  /**
   * @brief Get network health score (0.0 to 1.0)
   */
  [[nodiscard]] double get_network_health_score() const;

 private:
  JPLClientConfig config_;

  // Internal implementation details
  struct Impl;
  std::unique_ptr<Impl> impl_;

  // Cache manager for intelligent cache operations
  mutable std::unique_ptr<CacheManager> cache_manager_;

  // Data validator for comprehensive validation
  mutable std::unique_ptr<DataValidator> data_validator_;

  /**
   * @brief Fetch single body data (internal)
   */
  [[nodiscard]] JPLResult<EphemerisData> fetch_body_internal(
      int jpl_id, std::chrono::system_clock::time_point epoch);

  /**
   * @brief Parse JPL HORIZONS response
   */
  [[nodiscard]] JPLResult<EphemerisData> parse_jpl_response(const std::string& response,
                                                            int jpl_id);

  /**
   * @brief Make HTTP request to JPL API
   */
  [[nodiscard]] JPLResult<std::string> make_request(const std::string& url,
                                                    const std::string& params);

  /**
   * @brief Load cache metadata
   */
  [[nodiscard]] std::optional<CacheMetadata> load_cache_metadata() const;

  /**
   * @brief Save cache metadata
   */
  [[nodiscard]] JPLVoidResult save_cache_metadata(const CacheMetadata& metadata);

  /**
   * @brief Enhanced cache validation methods
   */
  [[nodiscard]] JPLResult<bool> validate_cache_metadata() const;
  [[nodiscard]] JPLResult<bool> validate_cache_files() const;
  [[nodiscard]] JPLResult<bool> validate_cache_formats() const;
  [[nodiscard]] JPLResult<bool> validate_binary_cache_format(
      const std::filesystem::path& binary_path) const;
  [[nodiscard]] JPLResult<bool> validate_json_cache_format(
      const std::filesystem::path& json_path) const;
  [[nodiscard]] JPLResult<bool> validate_cache_integrity(const CacheMetadata& metadata) const;
  [[nodiscard]] JPLResult<bool> validate_cache_consistency() const;

  /**
   * @brief Cache data validation helpers
   */
  [[nodiscard]] uint64_t calculate_enhanced_checksum(const std::vector<EphemerisData>& data) const;
  [[nodiscard]] JPLResult<bool> validate_body_data_integrity(const EphemerisData& body_data) const;
  [[nodiscard]] bool compare_body_data(const EphemerisData& body1,
                                       const EphemerisData& body2) const;

  /**
   * @brief Cache loading methods for validation
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> load_binary_cache() const;
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> load_json_cache() const;

  /**
   * @brief JSON parsing helper
   */
  [[nodiscard]] double parse_json_double(const std::string& json, size_t field_pos) const;

  /**
   * @brief Cache corruption detection and recovery
   */
  [[nodiscard]] JPLResult<bool> detect_and_recover_cache_corruption();
  [[nodiscard]] JPLResult<bool> attempt_cache_recovery();
  [[nodiscard]] JPLResult<bool> clear_corrupted_cache();
  [[nodiscard]] JPLResult<bool> save_binary_cache(const std::vector<EphemerisData>& data);
  [[nodiscard]] JPLResult<bool> save_json_cache(const std::vector<EphemerisData>& data);

  /**
   * @brief Enhanced network resilience methods
   */
  [[nodiscard]] JPLResult<std::string> make_resilient_request(const std::string& url,
                                                              const std::string& params);
  [[nodiscard]] std::chrono::milliseconds calculate_backoff_delay(size_t attempt) const;
  [[nodiscard]] JPLResult<std::string> try_fallback_endpoints(const std::string& params);
  [[nodiscard]] JPLResult<std::string> execute_request_with_circuit_breaker(
      const std::string& url, const std::string& params);

  /**
   * @brief Connection pool management
   */
  [[nodiscard]] std::optional<ConnectionPoolEntry*> acquire_connection(const std::string& endpoint);
  void release_connection(const std::string& endpoint);
  void cleanup_expired_connections();

  /**
   * @brief Circuit breaker management
   */
  mutable CircuitBreaker circuit_breaker_;
  mutable std::vector<ConnectionPoolEntry> connection_pool_;
  mutable std::mutex network_mutex_;

  /**
   * @brief Network monitoring and diagnostics
   */
  mutable NetworkDiagnostics network_diagnostics_;
  mutable std::mutex diagnostics_mutex_;
  bool offline_mode_enabled_ = false;

  /**
   * @brief Network connectivity monitoring methods
   */
  [[nodiscard]] NetworkConnectivityStatus test_basic_connectivity();
  [[nodiscard]] JPLResult<std::chrono::milliseconds> ping_endpoint(const std::string& endpoint);
  [[nodiscard]] JPLVoidResult update_network_diagnostics(bool success,
                                                         std::chrono::milliseconds response_time,
                                                         const std::string& error = "");
  [[nodiscard]] JPLVoidResult monitor_network_health();
  [[nodiscard]] JPLVoidResult test_all_endpoints();
};

/**
 * @brief Factory for creating JPL clients
 */
class SOLAR_JPL_API JPLClientFactory {
 public:
  /**
   * @brief Create default JPL client with cache relative to executable
   */
  [[nodiscard]] static std::unique_ptr<JPLClient> create_default();

  /**
   * @brief Create JPL client with custom configuration
   */
  [[nodiscard]] static std::unique_ptr<JPLClient> create(JPLClientConfig config);

  /**
   * @brief Create JPL client for testing (with mock endpoints)
   */
  [[nodiscard]] static std::unique_ptr<JPLClient> create_for_testing();

 private:
  /**
   * @brief Get cache directory relative to executable location
   */
  [[nodiscard]] static std::filesystem::path get_executable_relative_cache_path();
};

/**
 * @brief Utility functions for JPL data management
 */
namespace Utils {

/**
 * @brief Convert system_clock time point to JPL date string
 */
[[nodiscard]] SOLAR_JPL_API std::string to_jpl_date_string(
    std::chrono::system_clock::time_point tp);

/**
 * @brief Parse JPL date string to system_clock time point
 */
[[nodiscard]] SOLAR_JPL_API std::optional<std::chrono::system_clock::time_point>
from_jpl_date_string(std::string_view date_str);

/**
 * @brief Get current year as time point (January 1st)
 */
[[nodiscard]] SOLAR_JPL_API std::chrono::system_clock::time_point get_current_year_epoch();

/**
 * @brief Get JPL ID for body name
 */
[[nodiscard]] SOLAR_JPL_API std::optional<int> get_jpl_id_for_body(std::string_view body_name);

/**
 * @brief Get all known JPL IDs
 */
[[nodiscard]] SOLAR_JPL_API std::vector<int> get_all_jpl_ids();

/**
 * @brief Convert JPL error to string
 */
[[nodiscard]] SOLAR_JPL_API std::string to_string(JPLError error);

/**
 * @brief Convert network connectivity status to string
 */
[[nodiscard]] SOLAR_JPL_API std::string to_string(NetworkConnectivityStatus status);

/**
 * @brief Format network diagnostics as human-readable string
 */
[[nodiscard]] SOLAR_JPL_API std::string format_network_diagnostics(
    const NetworkDiagnostics& diagnostics);

/**
 * @brief Check if endpoint URL is valid
 */
[[nodiscard]] SOLAR_JPL_API bool is_valid_endpoint_url(const std::string& url);

}  // namespace Utils

}  // namespace SolarSystem::JPL
