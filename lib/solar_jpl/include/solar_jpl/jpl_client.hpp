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
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "solar_core/bodies/body_mappings.hpp"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/vector3.hpp"

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
struct EphemerisData {
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
struct JPLClientConfig {
  std::string api_endpoint = "https://ssd.jpl.nasa.gov/api/horizons.api";
  std::chrono::seconds request_timeout = std::chrono::seconds(30);
  std::chrono::milliseconds request_delay = std::chrono::milliseconds(200);
  size_t max_concurrent_requests = 3;
  size_t max_retries = 3;
  std::chrono::milliseconds retry_delay = std::chrono::milliseconds(500);

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
 * @brief Cache metadata
 */
struct CacheMetadata {
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
class JPLClient {
 public:
  /**
   * @brief Construct JPL client with configuration
   */
  explicit JPLClient(JPLClientConfig config = {});

  /**
   * @brief Destructor ensures cleanup
   */
  ~JPLClient();

  // Non-copyable but movable
  JPLClient(const JPLClient&) = delete;
  JPLClient& operator=(const JPLClient&) = delete;
  JPLClient(JPLClient&&) = default;
  JPLClient& operator=(JPLClient&&) = default;

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

  /**
   * @brief Get configuration
   */
  [[nodiscard]] const JPLClientConfig& config() const noexcept { return config_; }

 private:
  JPLClientConfig config_;

  // Internal implementation details
  struct Impl;
  std::unique_ptr<Impl> impl_;

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
};

/**
 * @brief Factory for creating JPL clients
 */
class JPLClientFactory {
 public:
  /**
   * @brief Create default JPL client
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
};

/**
 * @brief Utility functions for JPL data management
 */
namespace Utils {

/**
 * @brief Convert system_clock time point to JPL date string
 */
[[nodiscard]] std::string to_jpl_date_string(std::chrono::system_clock::time_point tp);

/**
 * @brief Parse JPL date string to system_clock time point
 */
[[nodiscard]] std::optional<std::chrono::system_clock::time_point> from_jpl_date_string(
    std::string_view date_str);

/**
 * @brief Get current year as time point (January 1st)
 */
[[nodiscard]] std::chrono::system_clock::time_point get_current_year_epoch();

/**
 * @brief Get JPL ID for body name
 */
[[nodiscard]] std::optional<int> get_jpl_id_for_body(std::string_view body_name);

/**
 * @brief Get all known JPL IDs
 */
[[nodiscard]] std::vector<int> get_all_jpl_ids();

/**
 * @brief Convert JPL error to string
 */
[[nodiscard]] std::string to_string(JPLError error);

}  // namespace Utils

}  // namespace SolarSystem::JPL
