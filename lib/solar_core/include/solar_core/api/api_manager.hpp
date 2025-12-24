/**
 * @file api_manager.hpp
 * @brief Comprehensive API management system
 *
 * Provides API management with:
 * - RESTful API with versioning
 * - API documentation generation
 * - Rate limiting and throttling
 * - API monitoring and analytics
 * - Request/response validation
 */

#pragma once

#include "solar_core/export.hpp"
#include "solar_core/performance/request_handler.hpp"
#include "solar_core/security/rate_limiter.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::API {

/**
 * @brief API version
 */
struct APIVersion {
  int major = 1;
  int minor = 0;
  int patch = 0;

  [[nodiscard]] SOLAR_CORE_API std::string to_string() const;
  [[nodiscard]] static SOLAR_CORE_API std::optional<APIVersion> parse(const std::string& str);
};

/**
 * @brief API endpoint definition
 */
struct APIEndpoint {
  std::string path;
  Performance::HttpMethod method;
  APIVersion version;
  std::string description;
  std::vector<std::string> parameters;
  std::vector<std::string> required_permissions;
  bool requires_auth = false;
  bool rate_limited = true;
  Performance::RequestHandler handler;
};

/**
 * @brief API request metadata
 */
struct APIRequestMetadata {
  std::string endpoint_path;
  Performance::HttpMethod method;
  APIVersion version;
  std::chrono::system_clock::time_point timestamp;
  std::string client_ip;
  std::string user_id;
  std::chrono::milliseconds response_time{0};
  int status_code = 0;
  size_t request_size = 0;
  size_t response_size = 0;
};

/**
 * @brief API statistics
 */
struct APIStatistics {
  size_t total_requests = 0;
  size_t successful_requests = 0;
  size_t failed_requests = 0;
  size_t rate_limited_requests = 0;
  std::chrono::milliseconds avg_response_time{0};
  std::map<std::string, size_t> endpoint_usage;
  std::map<int, size_t> status_code_distribution;
};

/**
 * @brief API manager
 */
class SOLAR_CORE_API APIManager {
 public:
  /**
   * @brief Construct API manager
   */
  APIManager();

  /**
   * @brief Destructor
   */
  ~APIManager();

  // Non-copyable, movable
  APIManager(const APIManager&) = delete;
  APIManager& operator=(const APIManager&) = delete;
  APIManager(APIManager&&) noexcept;
  APIManager& operator=(APIManager&&) noexcept;

  /**
   * @brief Register API endpoint
   */
  void register_endpoint(const APIEndpoint& endpoint);

  /**
   * @brief Handle API request
   */
  [[nodiscard]] Performance::HttpResponse handle_request(
      const Performance::HttpRequest& request);

  /**
   * @brief Get API documentation
   */
  [[nodiscard]] std::string get_documentation(const std::string& format = "json") const;

  /**
   * @brief Get API statistics
   */
  [[nodiscard]] APIStatistics get_statistics() const;

  /**
   * @brief Get endpoint usage
   */
  [[nodiscard]] std::map<std::string, size_t> get_endpoint_usage() const;

  /**
   * @brief Set rate limiter
   */
  void set_rate_limiter(std::shared_ptr<Security::RateLimiter> limiter);

  /**
   * @brief Enable/disable API monitoring
   */
  void set_monitoring_enabled(bool enabled);

  /**
   * @brief Get all registered endpoints
   */
  [[nodiscard]] std::vector<APIEndpoint> get_endpoints() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief API documentation generator
 */
class APIDocumentationGenerator {
 public:
  /**
   * @brief Generate OpenAPI/Swagger documentation
   */
  [[nodiscard]] static std::string generate_openapi(
      const std::vector<APIEndpoint>& endpoints,
      const std::string& title = "Solar System API",
      const APIVersion& version = {1, 0, 0});

  /**
   * @brief Generate Markdown documentation
   */
  [[nodiscard]] static std::string generate_markdown(
      const std::vector<APIEndpoint>& endpoints);

  /**
   * @brief Generate HTML documentation
   */
  [[nodiscard]] static std::string generate_html(
      const std::vector<APIEndpoint>& endpoints);
};

/**
 * @brief API validator
 */
class APIValidator {
 public:
  /**
   * @brief Validate request against endpoint definition
   */
  [[nodiscard]] static bool validate_request(
      const Performance::HttpRequest& request,
      const APIEndpoint& endpoint,
      std::string* error = nullptr);

  /**
   * @brief Validate API version
   */
  [[nodiscard]] static bool validate_version(
      const APIVersion& requested,
      const APIVersion& supported);

  /**
   * @brief Validate required parameters
   */
  [[nodiscard]] static bool validate_parameters(
      const std::map<std::string, std::string>& params,
      const std::vector<std::string>& required);
};

}  // namespace SolarSystem::API
