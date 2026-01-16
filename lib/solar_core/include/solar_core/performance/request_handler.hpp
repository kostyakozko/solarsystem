/**
 * @file request_handler.hpp
 * @brief High-performance HTTP request handling
 *
 * Provides efficient request handling with:
 * - Fast routing and dispatching
 * - Thread pool for concurrent requests
 * - Request parsing and validation
 * - Response generation
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Performance {

/**
 * @brief HTTP method enumeration
 */
enum class HttpMethod { GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS };

/**
 * @brief Convert HTTP method to string
 */
[[nodiscard]] std::string to_string(HttpMethod method);

/**
 * @brief Parse HTTP method from string
 */
[[nodiscard]] std::optional<HttpMethod> parse_http_method(const std::string& str);

/**
 * @brief HTTP request
 */
struct HttpRequest {
  HttpMethod method;
  std::string path;
  std::string query_string;
  std::map<std::string, std::string> headers;
  std::map<std::string, std::string> query_params;
  std::string body;
  std::string client_ip;
};

/**
 * @brief HTTP response
 */
struct HttpResponse {
  int status_code = 200;
  std::string status_message = "OK";
  std::map<std::string, std::string> headers;
  std::string body;
  bool should_close_connection = false;
};

/**
 * @brief Request handler function
 */
using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

/**
 * @brief Route definition
 */
struct Route {
  HttpMethod method;
  std::string path;
  RequestHandler handler;
  bool requires_auth = false;
};

/**
 * @brief HTTP router for efficient request routing
 */
class HttpRouter {
 public:
  /**
   * @brief Add route
   */
  void add_route(HttpMethod method, const std::string& path, RequestHandler handler);

  /**
   * @brief Add route with authentication requirement
   */
  void add_route(HttpMethod method, const std::string& path, RequestHandler handler,
                 bool requires_auth);

  /**
   * @brief Find handler for request
   */
  [[nodiscard]] std::optional<RequestHandler> find_handler(HttpMethod method,
                                                           const std::string& path) const;

  /**
   * @brief Check if route requires authentication
   */
  [[nodiscard]] bool requires_auth(HttpMethod method, const std::string& path) const;

  /**
   * @brief Get all routes
   */
  [[nodiscard]] std::vector<Route> get_routes() const;

 private:
  std::vector<Route> routes_;
};

/**
 * @brief Request processor with thread pool
 */
class RequestProcessor {
 public:
  /**
   * @brief Construct with thread pool size
   */
  explicit RequestProcessor(size_t thread_pool_size = 4);

  /**
   * @brief Destructor
   */
  ~RequestProcessor();

  // Non-copyable, movable
  RequestProcessor(const RequestProcessor&) = delete;
  RequestProcessor& operator=(const RequestProcessor&) = delete;
  RequestProcessor(RequestProcessor&&) noexcept;
  RequestProcessor& operator=(RequestProcessor&&) noexcept;

  /**
   * @brief Process request asynchronously
   */
  void process_async(const HttpRequest& request, RequestHandler handler,
                     std::function<void(const HttpResponse&)> callback);

  /**
   * @brief Process request synchronously
   */
  [[nodiscard]] HttpResponse process_sync(const HttpRequest& request, RequestHandler handler);

  /**
   * @brief Get active request count
   */
  [[nodiscard]] size_t get_active_count() const;

  /**
   * @brief Get total processed count
   */
  [[nodiscard]] size_t get_total_processed() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTTP request parser
 */
class HttpRequestParser {
 public:
  /**
   * @brief Parse HTTP request from raw data
   */
  [[nodiscard]] static std::optional<HttpRequest> parse(const std::string& raw_request);

  /**
   * @brief Parse query string
   */
  [[nodiscard]] static std::map<std::string, std::string> parse_query_string(
      const std::string& query);

  /**
   * @brief Parse headers
   */
  [[nodiscard]] static std::map<std::string, std::string> parse_headers(
      const std::string& header_section);
};

/**
 * @brief HTTP response builder
 */
class HttpResponseBuilder {
 public:
  /**
   * @brief Create response builder
   */
  HttpResponseBuilder() = default;

  /**
   * @brief Set status code
   */
  HttpResponseBuilder& status(int code, const std::string& message = "");

  /**
   * @brief Add header
   */
  HttpResponseBuilder& header(const std::string& name, const std::string& value);

  /**
   * @brief Set body
   */
  HttpResponseBuilder& body(const std::string& content);

  /**
   * @brief Set JSON body
   */
  HttpResponseBuilder& json(const std::string& json_content);

  /**
   * @brief Set HTML body
   */
  HttpResponseBuilder& html(const std::string& html_content);

  /**
   * @brief Build response
   */
  [[nodiscard]] HttpResponse build() const;

  /**
   * @brief Serialize response to HTTP format
   */
  [[nodiscard]] static std::string serialize(const HttpResponse& response);

 private:
  HttpResponse response_;
};

}  // namespace SolarSystem::Performance
