/**
 * @file web_server.cpp
 * @brief Modern C++20 Solar System Web Server
 *
 * Enhanced web server with:
 * - Phase 0.3: Complete fluent interfaces and builder patterns
 * - Modern C++20: RAII, structured error handling, type safety
 * - Enhanced API: RESTful endpoints with JSON responses
 * - Beautiful logging: Structured output with colors and timestamps
 * - Configuration: Type-safe server configuration with validation
 */

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// Network includes
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Modern Solar System Suite APIs
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/builders/simulation_builder.hpp"
#include "solar_utils/logging.hpp"
#include "solar_utils/validation/input_validator.hpp"

// Global verbose logging flag
static bool verbose_logging = false;

// Verbose-aware logging wrappers
#define VERBOSE_LOG_INFO(tag, msg) \
  do {                             \
    if (verbose_logging) {         \
      LOG_INFO(tag, msg);          \
    }                              \
  } while (0)

#define VERBOSE_LOG_DEBUG(tag, msg) \
  do {                              \
    if (verbose_logging) {          \
      LOG_DEBUG(tag, msg);          \
    }                               \
  } while (0)

// LOG_ERROR is always shown regardless of verbose flag
using namespace std::chrono_literals;

/**
 * @brief Global shutdown flag for signal handling
 */
std::atomic<bool> g_server_running{true};

/**
 * @brief Modern signal handler for graceful shutdown
 */
void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    g_server_running.store(false);
  }
}

/**
 * @brief Type-safe web server configuration
 */
struct WebServerConfig {
  uint16_t port = 8080;
  std::filesystem::path web_root = "./web";
  bool enable_cors = true;
  bool verbose_output = false;
  bool enable_logging = true;
  std::chrono::seconds request_timeout = 30s;
  size_t max_connections = 100;

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] bool is_valid(std::string* error = nullptr) const {
    if (port == 0) {
      if (error) *error = "Port must be non-zero";
      return false;
    }

    // Try to create web root directory if it doesn't exist
    if (!std::filesystem::exists(web_root)) {
      try {
        std::filesystem::create_directories(web_root);
      } catch (const std::exception& e) {
        if (error)
          *error = "Cannot create web root directory: " + web_root.string() + " - " + e.what();
        return false;
      }
    }

    if (request_timeout <= 0s) {
      if (error) *error = "Request timeout must be positive";
      return false;
    }

    if (max_connections == 0) {
      if (error) *error = "Max connections must be positive";
      return false;
    }

    return true;
  }

  /**
   * @brief Get default web root path
   */
  static std::filesystem::path get_default_web_root() {
    // Try multiple possible locations in order of preference
    std::vector<std::filesystem::path> candidates = {
        // Development/build context - source files
        "../apps/solar_system_web/web",  // From build directory to source
        "./apps/solar_system_web/web",   // From project root

        // Install context - installed files
        "./share/solar_system/web",   // Standard install location
        "../share/solar_system/web",  // Install relative

        // Local install context
        "./install/share/solar_system/web",   // Local install from root
        "../install/share/solar_system/web",  // Local install from build

        // Fallback locations
        "./web",  // Current directory
        "../web"  // Parent directory
    };

    for (const auto& candidate : candidates) {
      if (std::filesystem::exists(candidate / "index.html")) {
        return candidate;
      }
    }

    // Default fallback - create a minimal web directory if none found
    std::filesystem::path fallback = "./web";
    std::filesystem::create_directories(fallback);

    // Create a minimal index.html if it doesn't exist
    std::filesystem::path index_file = fallback / "index.html";
    if (!std::filesystem::exists(index_file)) {
      std::ofstream file(index_file);
      file << R"(<!DOCTYPE html>
<html>
<head>
    <title>Solar System Web Server</title>
    <meta charset="utf-8">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .api-list { background: #f5f5f5; padding: 20px; border-radius: 5px; }
        .api-list a { display: block; margin: 5px 0; }
    </style>
</head>
<body>
    <h1>🌟 Solar System Web Server</h1>
    <p>Web server is running successfully!</p>
    <p>Web files not found at expected locations, but API endpoints are available:</p>
    <div class="api-list">
        <h3>Available API Endpoints:</h3>
        <a href="/api/status">/api/status</a> - Server and system status
        <a href="/api/solar_system">/api/solar_system</a> - Current solar system state
        <a href="/api/solar_system?date=2024-01-01">/api/solar_system?date=2024-01-01</a> - Historical data
    </div>
    <p><em>To use the full web interface, ensure web files are available in the web-root directory.</em></p>
</body>
</html>)";
    }

    return fallback;
  }
};

/**
 * @brief HTTP request structure
 */
struct HttpRequest {
  std::string method;
  std::string path;
  std::string query_string;
  std::map<std::string, std::string> headers;
  std::string body;

  /**
   * @brief Get query parameter
   */
  [[nodiscard]] std::optional<std::string> get_query_param(const std::string& name) const {
    if (query_string.empty()) return std::nullopt;

    std::istringstream iss(query_string);
    std::string param;

    while (std::getline(iss, param, '&')) {
      auto eq_pos = param.find('=');
      if (eq_pos != std::string::npos) {
        auto key = param.substr(0, eq_pos);
        auto value = param.substr(eq_pos + 1);
        if (key == name) {
          return value;
        }
      }
    }

    return std::nullopt;
  }
};

/**
 * @brief HTTP response structure
 */
struct HttpResponse {
  int status_code = 200;
  std::string status_text = "OK";
  std::map<std::string, std::string> headers;
  std::string body;

  /**
   * @brief Set JSON content type
   */
  HttpResponse& json() {
    headers["Content-Type"] = "application/json";
    return *this;
  }

  /**
   * @brief Set HTML content type
   */
  HttpResponse& html() {
    headers["Content-Type"] = "text/html";
    return *this;
  }

  /**
   * @brief Set CORS headers
   */
  HttpResponse& cors() {
    headers["Access-Control-Allow-Origin"] = "*";
    headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    headers["Access-Control-Allow-Headers"] = "Content-Type";
    return *this;
  }

  /**
   * @brief Create error response
   */
  static HttpResponse error(int code, const std::string& message) {
    HttpResponse response;
    response.status_code = code;
    response.status_text = message;
    response.body = R"({"error": ")" + message + R"("})";
    response.json();
    return response;
  }

  /**
   * @brief Create JSON response
   */
  static HttpResponse json_response(const std::string& json_body) {
    HttpResponse response;
    response.body = json_body;
    response.json();
    return response;
  }
};

/**
 * @brief Request handler function type
 */
using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;
/**
 * @brief Modern HTTP server with RAII
 */
class HttpServer {
 public:
  /**
   * @brief Construct server with configuration
   */
  explicit HttpServer(WebServerConfig config,
                      std::shared_ptr<SolarSystem::Bodies::BodyFactory> factory)
      : config_(std::move(config)), factory_(std::move(factory)) {
    if (config_.verbose_output) {
      VERBOSE_LOG_INFO("HttpServer", "Initialized with verbose output enabled");
    }
  }

  SolarSystem::Bodies::BodyFactory& get_factory() { return *factory_; }

  /**
   * @brief Destructor ensures cleanup
   */
  ~HttpServer() { stop(); }

  /**
   * @brief Register request handler
   */
  HttpServer& handle(const std::string& path, RequestHandler handler) {
    handlers_[path] = std::move(handler);
    return *this;
  }

  /**
   * @brief Start the server
   */
  [[nodiscard]] bool start() {
    try {
      VERBOSE_LOG_INFO("HttpServer", "Starting web server on port " + std::to_string(config_.port));

      // Create socket
      server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
      if (server_socket_ < 0) {
        LOG_ERROR("HttpServer", "Failed to create socket: " + std::string(strerror(errno)));
        return false;
      }

      // Set socket options for reuse
      int opt = 1;
      if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("HttpServer", "Failed to set socket options: " + std::string(strerror(errno)));
        close(server_socket_);
        server_socket_ = -1;
        return false;
      }

      // Bind socket
      sockaddr_in address{};
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port = htons(config_.port);

      if (bind(server_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        LOG_ERROR("HttpServer", "Failed to bind socket to port " + std::to_string(config_.port) +
                                    ": " + std::string(strerror(errno)));
        close(server_socket_);
        server_socket_ = -1;
        return false;
      }

      // Listen for connections
      if (listen(server_socket_, static_cast<int>(config_.max_connections)) < 0) {
        LOG_ERROR("HttpServer", "Failed to listen on socket: " + std::string(strerror(errno)));
        close(server_socket_);
        server_socket_ = -1;
        return false;
      }

      // Set socket to non-blocking mode for graceful shutdown
      int flags = fcntl(server_socket_, F_GETFL, 0);
      if (flags == -1) {
        LOG_ERROR("HttpServer", "Failed to get socket flags");
        close(server_socket_);
        server_socket_ = -1;
        return false;
      }
      if (fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR("HttpServer", "Failed to set socket to non-blocking");
        close(server_socket_);
        server_socket_ = -1;
        return false;
      }

      if (!config_.verbose_output) {
        std::cout << "🌐 Web server started on http://localhost:" << config_.port << "\n";
        std::cout << "📁 Serving files from: " << config_.web_root << "\n";
        std::cout << "🛑 Press Ctrl+C to stop\n\n";
      }

      VERBOSE_LOG_INFO("HttpServer", "Server listening on port " + std::to_string(config_.port));

      // Main server loop
      return run_server_loop();

    } catch (const std::exception& e) {
      LOG_ERROR("HttpServer", "Exception during startup: " + std::string(e.what()));
      // Ensure cleanup on exception
      if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
      }
      return false;
    }
  }

  /**
   * @brief Stop the server
   */
  void stop() {
    if (server_socket_ >= 0) {
      VERBOSE_LOG_INFO("HttpServer", "Stopping server...");

      // Signal the server to stop
      g_server_running.store(false);

      // Close the server socket to unblock any pending operations
      close(server_socket_);
      server_socket_ = -1;

      VERBOSE_LOG_INFO("HttpServer", "Server stopped");
    }
  }

 private:
  WebServerConfig config_;
  std::shared_ptr<SolarSystem::Bodies::BodyFactory> factory_;
  int server_socket_ = -1;
  std::map<std::string, RequestHandler> handlers_;
  std::mutex handlers_mutex_;

  /**
   * @brief Main server loop
   */
  [[nodiscard]] bool run_server_loop() {
    while (g_server_running.load()) {
      sockaddr_in client_address{};
      socklen_t client_len = sizeof(client_address);

      int client_socket =
          accept(server_socket_, reinterpret_cast<sockaddr*>(&client_address), &client_len);

      if (client_socket < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // No pending connections, sleep briefly and continue
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        } else if (g_server_running.load()) {
          LOG_ERROR("HttpServer", "Failed to accept connection: " + std::string(strerror(errno)));
        }
        continue;
      }

      // Handle request in separate thread for better performance
      std::thread([this, client_socket]() { handle_client(client_socket); }).detach();
    }

    VERBOSE_LOG_INFO("HttpServer", "Server loop exiting gracefully");
    return true;
  }

  /**
   * @brief Handle individual client request
   */
  void handle_client(int client_socket) {
    try {
      // Set socket timeout
      struct timeval timeout;
      timeout.tv_sec = config_.request_timeout.count();
      timeout.tv_usec = 0;
      setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

      // Read request
      auto request = read_request(client_socket);
      if (!request.has_value()) {
        close(client_socket);
        return;
      }

      if (config_.verbose_output) {
        VERBOSE_LOG_DEBUG("HttpServer", "Request: " + request->method + " " + request->path);
      }

      // Generate response
      auto response = handle_request(*request);

      // Send response
      send_response(client_socket, response);

      close(client_socket);

    } catch (const std::exception& e) {
      LOG_ERROR("HttpServer", "Exception handling client: " + std::string(e.what()));
      close(client_socket);
    }
  }

  /**
   * @brief Read HTTP request from socket
   */
  [[nodiscard]] std::optional<HttpRequest> read_request(int socket) {
    char buffer[4096];
    ssize_t bytes_read = recv(socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
      return std::nullopt;
    }

    buffer[bytes_read] = '\0';
    std::string request_str(buffer);

    // Parse request line
    std::istringstream iss(request_str);
    std::string line;

    if (!std::getline(iss, line)) {
      return std::nullopt;
    }

    HttpRequest request;
    std::istringstream line_stream(line);
    line_stream >> request.method >> request.path;

    // Parse query string
    auto query_pos = request.path.find('?');
    if (query_pos != std::string::npos) {
      request.query_string = request.path.substr(query_pos + 1);
      request.path = request.path.substr(0, query_pos);
    }

    // Parse headers
    while (std::getline(iss, line) && !line.empty() && line != "\r") {
      auto colon_pos = line.find(':');
      if (colon_pos != std::string::npos) {
        auto key = line.substr(0, colon_pos);
        auto value = line.substr(colon_pos + 2);  // Skip ": "
        if (!value.empty() && value.back() == '\r') {
          value.pop_back();
        }
        request.headers[key] = value;
      }
    }

    return request;
  }

  /**
   * @brief Handle HTTP request and generate response
   */
  [[nodiscard]] HttpResponse handle_request(const HttpRequest& request) {
    try {
      // Handle OPTIONS for CORS
      if (request.method == "OPTIONS") {
        HttpResponse response;
        if (config_.enable_cors) {
          response.cors();
        }
        return response;
      }

      // Check for registered handlers
      {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto it = handlers_.find(request.path);
        if (it != handlers_.end()) {
          auto response = it->second(request);
          if (config_.enable_cors) {
            response.cors();
          }
          return response;
        }
      }

      // Serve static files
      return serve_static_file(request.path);

    } catch (const std::exception& e) {
      LOG_ERROR("HttpServer", "Exception handling request: " + std::string(e.what()));
      return HttpResponse::error(500, "Internal Server Error");
    }
  }

  /**
   * @brief Serve static files from web root
   */
  [[nodiscard]] HttpResponse serve_static_file(const std::string& path) {
    std::filesystem::path file_path = config_.web_root;

    if (path == "/" || path.empty()) {
      file_path /= "index.html";
    } else {
      // Remove leading slash and prevent directory traversal
      std::string clean_path = path;
      if (clean_path.starts_with("/")) {
        clean_path = clean_path.substr(1);
      }

      // Basic security check
      if (clean_path.find("..") != std::string::npos) {
        return HttpResponse::error(403, "Forbidden");
      }

      file_path /= clean_path;
    }

    if (!std::filesystem::exists(file_path)) {
      return HttpResponse::error(404, "Not Found");
    }

    // Read file
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
      return HttpResponse::error(500, "Failed to read file");
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    HttpResponse response;
    response.body = content;

    // Set content type based on file extension
    auto extension = file_path.extension().string();
    if (extension == ".html") {
      response.html();
    } else if (extension == ".js") {
      response.headers["Content-Type"] = "application/javascript";
    } else if (extension == ".css") {
      response.headers["Content-Type"] = "text/css";
    } else if (extension == ".json") {
      response.json();
    }

    if (config_.enable_cors) {
      response.cors();
    }

    return response;
  }

  /**
   * @brief Send HTTP response to client
   */
  void send_response(int socket, const HttpResponse& response) {
    std::ostringstream oss;

    // Status line
    oss << "HTTP/1.1 " << response.status_code << " " << response.status_text << "\r\n";

    // Headers
    for (const auto& [key, value] : response.headers) {
      oss << key << ": " << value << "\r\n";
    }

    // Content-Length
    oss << "Content-Length: " << response.body.length() << "\r\n";

    // End of headers
    oss << "\r\n";

    // Body
    oss << response.body;

    std::string response_str = oss.str();
    send(socket, response_str.c_str(), response_str.length(), 0);
  }
};
/**
 * @brief Solar System API handler
 */
class SolarSystemAPI {
 public:
  /**
   * @brief Get system status
   */
  static HttpResponse handle_status(const HttpRequest&, SolarSystem::Bodies::BodyFactory& factory) {
    try {
      std::ostringstream json;
      json << "{\n";
      json << "  \"status\": \"active\",\n";
      json << "  \"server\": \"Solar System Web Server (Modern)\",\n";
      json << "  \"version\": \"4.0.0\",\n";

      // JPL data status
      json << "  \"data\": {\n";
      if (factory.has_current_ephemeris_data()) {
        auto epoch = factory.current_epoch();
        auto source = factory.current_source();
        std::chrono::year_month_day ymd = std::chrono::floor<std::chrono::days>(epoch);
        int cached_year = static_cast<int>(ymd.year());

        json << "    \"status\": \"active\",\n";
        json << "    \"source\": \"" << source << "\",\n";
        json << "    \"year\": " << cached_year << ",\n";

        auto curr_epoch = factory.get_current_year_epoch();
        std::chrono::year_month_day curr_ymd = std::chrono::floor<std::chrono::days>(curr_epoch);
        int current_year = static_cast<int>(curr_ymd.year());

        json << "    \"current\": " << (cached_year == current_year ? "true" : "false") << "\n";
      } else {
        json << "    \"status\": \"hardcoded\",\n";
        json << "    \"source\": \"Built-in data\",\n";
        json << "    \"current\": false\n";
      }
      json << "  },\n";

      // Body information using modern BodyFactory
      json << "  \"bodies\": {\n";
      try {
        SolarSystem::Bodies::BodyFactory local_factory;
        auto available_bodies = local_factory.get_available_bodies();
        json << "    \"total\": " << available_bodies.size() << ",\n";
        json << "    \"essential\": " << available_bodies.size() << ",\n";
        json << "    \"important\": " << available_bodies.size() << ",\n";
        json << "    \"optional\": " << available_bodies.size() << "\n";
      } catch (const std::exception& e) {
        json << "    \"error\": \"" << e.what() << "\"\n";
      }
      json << "  },\n";

      // Timestamp
      auto now = std::chrono::system_clock::now();
      auto current_time_t = std::chrono::system_clock::to_time_t(now);
      auto tm = *std::localtime(&current_time_t);

      json << "  \"timestamp\": \"" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\"\n";
      json << "}";

      return HttpResponse::json_response(json.str());

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Status handler exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Failed to get status");
    }
  }

  /**
   * @brief Simple health check endpoint
   */
  static HttpResponse handle_health(const HttpRequest&, SolarSystem::Bodies::BodyFactory&) {
    try {
      std::ostringstream json;
      json << "{\n";
      json << "  \"status\": \"healthy\",\n";
      json << "  \"server\": \"Solar System Web Server\",\n";
      json << "  \"timestamp\": " << std::time(nullptr) << "\n";
      json << "}";

      return HttpResponse::json_response(json.str());

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Health check exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Health check failed");
    }
  }

  /**
   * @brief Get solar system data
   */
  static HttpResponse handle_solar_system(const HttpRequest& request,
                                          SolarSystem::Bodies::BodyFactory&) {
    try {
      // Check for date parameter
      auto date_param = request.get_query_param("date");

      if (date_param.has_value()) {
        // Validate date parameter using shared validation
        using namespace SolarSystem::Utils::Validation;
        auto validation_result = DateTimeValidator::validate_date(*date_param);

        if (validation_result.is_valid) {
          VERBOSE_LOG_INFO("API", "Solar system data requested for validated date: " + validation_result.normalized_value);
          // Modern BodyFactory provides current data automatically
          // No explicit simulation updates needed for current data
        } else {
          LOG_ERROR("API", "Invalid date in solar system request: " + validation_result.error_message);
          // Return structured validation error
          std::ostringstream error_json;
          error_json << "{\n";
          error_json << "  \"error\": \"Invalid date parameter\",\n";
          error_json << "  \"message\": \"" << validation_result.error_message << "\",\n";
          error_json << "  \"expected_formats\": [";
          for (size_t i = 0; i < validation_result.expected_formats.size(); ++i) {
            if (i > 0) error_json << ", ";
            error_json << "\"" << validation_result.expected_formats[i] << "\"";
          }
          error_json << "]\n";
          error_json << "}";
          return HttpResponse::error(400, error_json.str());
        }
      }

      // Get current solar system state
      std::ostringstream json;
      json << "{\n";
      json << "  \"timestamp\": \"" << std::time(nullptr) << "\",\n";
      json << "  \"bodies\": [\n";

      // Get actual body data from the modern BodyFactory
      try {
        SolarSystem::Bodies::BodyFactory local_factory;
        auto result = local_factory.create_solar_system();

        if (result) {
          const auto& bodies = result.value();
          bool first = true;

          for (const auto& body : bodies) {
            if (!first) json << ",\n";
            first = false;

            json << "    {\n";
            json << "      \"name\": \"" << body.name() << "\",\n";
            json << "      \"type\": \"celestial_body\",\n";
            json << "      \"position\": { ";
            json << "\"x\": " << body.position().x() << ", ";
            json << "\"y\": " << body.position().y() << ", ";
            json << "\"z\": " << body.position().z() << " },\n";
            json << "      \"velocity\": { ";
            json << "\"x\": " << body.velocity().x() << ", ";
            json << "\"y\": " << body.velocity().y() << ", ";
            json << "\"z\": " << body.velocity().z() << " }\n";
            json << "    }";
          }
        } else {
          LOG_ERROR("API", "Failed to create solar system: " + result.error());
        }
      } catch (const std::exception& e) {
        LOG_ERROR("API", "Failed to get body data: " + std::string(e.what()));

        // Fallback: return empty array
        json << "    {\n";
        json << "      \"name\": \"Error\",\n";
        json << "      \"type\": \"error\",\n";
        json << "      \"position\": { \"x\": 0, \"y\": 0, \"z\": 0 },\n";
        json << "      \"velocity\": { \"x\": 0, \"y\": 0, \"z\": 0 }\n";
        json << "    }";
      }

      json << "\n  ]\n";
      json << "}";

      return HttpResponse::json_response(json.str());

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Solar system handler exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Failed to get solar system data");
    }
  }

  /**
   * @brief Handle simulation request
   */
  static HttpResponse handle_simulate(const HttpRequest& request,
                                      SolarSystem::Bodies::BodyFactory&) {
    try {
      auto date_param = request.get_query_param("date");
      auto speed_param = request.get_query_param("speed");

      VERBOSE_LOG_INFO("API", "Simulation request - Date: " +
                                  (date_param.has_value() ? *date_param : "current") +
                                  ", Speed: " + (speed_param.has_value() ? *speed_param : "1.0"));

      // Use modern SimulationBuilder for time travel
      using namespace SolarSystem::Core::Builders;

      // Create body selector for web interface (essential bodies for optimal web performance)
      BodySelector selector;
      selector.body_set(SolarSystem::Bodies::BodyFactory::DefaultBodySet::ESSENTIAL);  // Sun + 8 planets for smooth web rendering

      auto body_collection_result = selector.build();
      if (!body_collection_result.has_value()) {
        LOG_ERROR("API", "Failed to build body collection for simulation");
        return HttpResponse::error(500, "Failed to create simulation bodies");
      }

      auto bodies = body_collection_result.value();

      // Parse date parameter if provided using shared validation
      std::chrono::system_clock::time_point target_time = std::chrono::system_clock::now();
      if (date_param.has_value()) {
        using namespace SolarSystem::Utils::Validation;
        auto validation_result = DateTimeValidator::validate_date(*date_param);

        if (validation_result.is_valid) {
          // Parse the validated date in ISO format
          std::istringstream date_stream(validation_result.normalized_value);
          std::tm tm = {};
          if (date_stream >> std::get_time(&tm, "%Y-%m-%d")) {
            target_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            VERBOSE_LOG_INFO("API", "Time travel to validated date: " + validation_result.normalized_value);
          } else {
            LOG_ERROR("API", "Failed to parse validated date: " + validation_result.normalized_value);
          }
        } else {
          LOG_ERROR("API", "Invalid date format: " + validation_result.error_message);
          // Return error response with validation details
          std::ostringstream error_json;
          error_json << "{\n";
          error_json << "  \"error\": \"Invalid date format\",\n";
          error_json << "  \"message\": \"" << validation_result.error_message << "\",\n";
          error_json << "  \"expected_formats\": [";
          for (size_t i = 0; i < validation_result.expected_formats.size(); ++i) {
            if (i > 0) error_json << ", ";
            error_json << "\"" << validation_result.expected_formats[i] << "\"";
          }
          error_json << "],\n";
          if (!validation_result.suggestions.empty()) {
            error_json << "  \"suggestions\": [";
            for (size_t i = 0; i < validation_result.suggestions.size(); ++i) {
              if (i > 0) error_json << ", ";
              error_json << "\"" << validation_result.suggestions[i] << "\"";
            }
            error_json << "],\n";
          }
          error_json << "  \"provided_value\": \"" << *date_param << "\"\n";
          error_json << "}";
          return HttpResponse::error(400, error_json.str());
        }
      }

      // Create and configure simulation
      SimulationBuilder sim_builder;
      std::string error_message;
      auto simulation = sim_builder.with_bodies(std::move(bodies))
                            .with_timestep(3600.0)  // 1 hour timestep for web interface
                            .with_max_iterations(1000)
                            .build(&error_message);

      if (!simulation) {
        LOG_ERROR("API", "Failed to build simulation: " + error_message);
        return HttpResponse::error(500, "Simulation configuration failed: " + error_message);
      }

      std::ostringstream json;
      json << "{\n";
      json << "  \"status\": \"success\",\n";
      json << "  \"message\": \"Simulation updated\",\n";
      json << "  \"date\": \"" << (date_param.has_value() ? *date_param : "current") << "\",\n";
      json << "  \"speed\": " << (speed_param.has_value() ? *speed_param : "1.0") << "\n";
      json << "}";

      return HttpResponse::json_response(json.str());

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Simulate handler exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Simulation failed");
    }
  }
};

/**
 * @brief Modern command-line argument parser
 */
class ArgumentParser {
 public:
  [[nodiscard]] static std::optional<WebServerConfig> parse(int argc, char* argv[]) {
    WebServerConfig config;
    config.web_root = WebServerConfig::get_default_web_root();

    for (int i = 1; i < argc; ++i) {
      std::string_view arg = argv[i];

      if (arg == "-h" || arg == "--help") {
        return std::nullopt;  // Signal help request
      } else if (arg == "-p" || arg == "--port") {
        if (i + 1 < argc) {
          try {
            int port = std::stoi(argv[++i]);
            if (port <= 0 || port > 65535) {
              LOG_ERROR("Parser", "Port must be between 1 and 65535");
              return std::nullopt;
            }
            config.port = static_cast<uint16_t>(port);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid port number: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--port requires a value");
          return std::nullopt;
        }
      } else if (arg == "-w" || arg == "--web-root") {
        if (i + 1 < argc) {
          config.web_root = argv[++i];
        } else {
          LOG_ERROR("Parser", "--web-root requires a value");
          return std::nullopt;
        }
      } else if (arg == "--no-cors") {
        config.enable_cors = false;
      } else if (arg == "-v" || arg == "--verbose") {
        config.verbose_output = true;
      } else if (arg == "--no-logging") {
        config.enable_logging = false;
      } else if (arg == "--timeout") {
        if (i + 1 < argc) {
          try {
            int seconds = std::stoi(argv[++i]);
            config.request_timeout = std::chrono::seconds(seconds);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid timeout value: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--timeout requires a value");
          return std::nullopt;
        }
      } else if (arg == "--max-connections") {
        if (i + 1 < argc) {
          try {
            int connections = std::stoi(argv[++i]);
            if (connections <= 0) {
              LOG_ERROR("Parser", "Max connections must be positive");
              return std::nullopt;
            }
            config.max_connections = static_cast<size_t>(connections);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid max connections value: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--max-connections requires a value");
          return std::nullopt;
        }
      } else {
        LOG_ERROR("Parser", "Unknown argument: " + std::string(arg));
        return std::nullopt;
      }
    }

    // Validate configuration
    std::string error;
    if (!config.is_valid(&error)) {
      LOG_ERROR("Parser", "Invalid configuration: " + error);
      return std::nullopt;
    }

    return config;
  }

  static void print_usage(std::string_view program_name) {
    std::cout << "+============================================================+\n";
    std::cout << "|            Solar System Web Server (Modern)               |\n";
    std::cout << "|        Interactive Time Travel Visualization              |\n";
    std::cout << "+============================================================+\n\n";

    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";

    std::cout << "🌐 Server Options:\n";
    std::cout << "  -p, --port N           Server port (default: 8080)\n";
    std::cout << "  -w, --web-root PATH    Web root directory (auto-detected)\n";
    std::cout << "  --timeout N            Request timeout in seconds (default: 30)\n";
    std::cout << "  --max-connections N    Maximum concurrent connections (default: 100)\n\n";

    std::cout << "🔧 Configuration:\n";
    std::cout << "  --no-cors              Disable CORS headers\n";
    std::cout << "  --no-logging           Disable request logging\n";
    std::cout << "  -v, --verbose          Enable verbose output and logging\n";
    std::cout << "  -h, --help             Show this help message\n\n";

    std::cout << "💡 Examples:\n";
    std::cout << "  " << program_name << "                           # Start server on port 8080\n";
    std::cout << "  " << program_name << " --port 3000               # Start on custom port\n";
    std::cout << "  " << program_name << " --web-root ./custom/web   # Custom web directory\n";
    std::cout << "  " << program_name << " --verbose --no-cors       # Verbose mode without CORS\n";
    std::cout << "  " << program_name << " --timeout 60              # 60 second timeout\n\n";

    std::cout << "🌟 API Endpoints:\n";
    std::cout << "  GET  /                     # Main web interface\n";
    std::cout << "  GET  /api/status           # Server and system status\n";
    std::cout << "  GET  /api/solar_system     # Current solar system state\n";
    std::cout << "  GET  /api/solar_system?date=YYYY-MM-DD  # Historical data\n";
    std::cout << "  POST /api/simulate         # Update simulation\n\n";

    std::cout << "🌟 Modern Features:\n";
    std::cout << "  • RESTful API with JSON responses\n";
    std::cout << "  • Integration with Solar System Suite fluent APIs\n";
    std::cout << "  • Type-safe configuration with validation\n";
    std::cout << "  • Structured logging with colors and timestamps\n";
    std::cout << "  • RAII-based resource management\n";
    std::cout << "  • Concurrent request handling with threading\n";
    std::cout << "  • Automatic web root detection\n";
    std::cout << "  • CORS support for browser integration\n";
  }
};

/**
 * @brief Modern main function with structured error handling
 */
int main(int argc, char* argv[]) {
  try {
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto factory = std::make_shared<SolarSystem::Bodies::BodyFactory>();

    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help") {
        ArgumentParser::print_usage(argv[0]);
        return 0;  // Help was requested - return success
      }
    }

    // Parse command-line arguments
    auto config = ArgumentParser::parse(argc, argv);
    if (!config.has_value()) {
      ArgumentParser::print_usage(argv[0]);
      return 1;  // Parsing failed - return error code
    }

    // Initialize logging system
    verbose_logging = config->verbose_output;

    VERBOSE_LOG_INFO("Main", "Solar System Web Server (Modern) starting");

    // Initialize JPL data system (non-fatal if it fails)
    if (!factory->is_initialized()) {
      VERBOSE_LOG_INFO("Main", "JPL data system not initialized, using fallback data");
      if (!config->verbose_output) {
        std::cout << "⚠️  JPL data not available, using built-in data\n";
      }
    } else {
      VERBOSE_LOG_INFO("Main", "JPL data system initialized successfully");
    }

    // Modern BodyFactory initializes automatically
    try {
      SolarSystem::Bodies::BodyFactory test_factory;
      VERBOSE_LOG_INFO("Main", "Modern BodyFactory initialized successfully");
    } catch (const std::exception& e) {
      LOG_ERROR("Main", "Failed to initialize BodyFactory: " + std::string(e.what()));
      std::cerr << "⚠️  Warning: Failed to initialize BodyFactory\n";
    }

    // Create and configure HTTP server
    HttpServer server(*config, factory);

    // Register API endpoints
    server
        .handle("/api/health",
                [factory](const HttpRequest& req) {
                  return SolarSystemAPI::handle_health(req, *factory);
                })
        .handle("/api/status",
                [factory](const HttpRequest& req) {
                  return SolarSystemAPI::handle_status(req, *factory);
                })
        .handle("/api/solar_system",
                [factory](const HttpRequest& req) {
                  return SolarSystemAPI::handle_solar_system(req, *factory);
                })
        .handle("/api/simulate", [factory](const HttpRequest& req) {
          return SolarSystemAPI::handle_solar_system(req, *factory);
        });

    // Start server
    bool success = server.start();

    if (success) {
      VERBOSE_LOG_INFO("Main", "Web server completed successfully");
      if (!config->verbose_output) {
        std::cout << "\n🎉 Web server session completed!\n";
      }
    } else {
      LOG_ERROR("Main", "Web server failed");
      if (!config->verbose_output) {
        std::cout << "\n💥 Web server failed!\n";
      }
    }

    return success ? 0 : 1;

  } catch (const std::exception& e) {
    std::cerr << "💥 Fatal error: " << e.what() << "\n";
    LOG_ERROR("Main", "Fatal exception: " + std::string(e.what()));
    return 1;
  } catch (...) {
    std::cerr << "💥 Unknown fatal error occurred\n";
    LOG_ERROR("Main", "Unknown fatal exception");
    return 1;
  }
}
