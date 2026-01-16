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
#include <nlohmann/json.hpp>
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
        // On Linux, EAGAIN and EWOULDBLOCK are typically the same value
        // Use a single check that handles both cases
        if (errno == EAGAIN || errno == EINTR) {
          // No pending connections or interrupted, sleep briefly and continue
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
   * @brief Get MIME type for file extension
   */
  [[nodiscard]] static std::string get_mime_type(const std::string& extension) {
    static const std::map<std::string, std::string> mime_types = {
        // Text formats
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".xml", "text/xml"},
        {".csv", "text/csv"},

        // JavaScript
        {".js", "application/javascript"},
        {".mjs", "application/javascript"},
        {".json", "application/json"},

        // Images
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".webp", "image/webp"},

        // Fonts
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".eot", "application/vnd.ms-fontobject"},

        // Media
        {".mp3", "audio/mpeg"},
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".ogg", "audio/ogg"},
        {".wav", "audio/wav"},

        // Archives
        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".gz", "application/gzip"},

        // Documents
        {".pdf", "application/pdf"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},

        // Other
        {".wasm", "application/wasm"},
        {".bin", "application/octet-stream"}};

    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
      return it->second;
    }
    return "application/octet-stream";  // Default fallback
  }

  /**
   * @brief Serve static files from web root with caching and security
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

      // Enhanced security checks
      if (clean_path.find("..") != std::string::npos) {
        LOG_ERROR("HttpServer", "Directory traversal attempt: " + clean_path);
        return HttpResponse::error(403, "Forbidden");
      }

      // Prevent access to hidden files
      if (clean_path.starts_with(".") || clean_path.find("/.") != std::string::npos) {
        LOG_ERROR("HttpServer", "Hidden file access attempt: " + clean_path);
        return HttpResponse::error(403, "Forbidden");
      }

      file_path /= clean_path;
    }

    // Verify the resolved path is still within web root (canonical path check)
    try {
      auto canonical_file = std::filesystem::canonical(file_path);
      auto canonical_root = std::filesystem::canonical(config_.web_root);

      // Check if file is within web root
      auto rel_path = std::filesystem::relative(canonical_file, canonical_root);
      if (rel_path.string().starts_with("..")) {
        LOG_ERROR("HttpServer", "Path escape attempt: " + path);
        return HttpResponse::error(403, "Forbidden");
      }
    } catch (const std::filesystem::filesystem_error&) {
      // File doesn't exist or can't be accessed
      VERBOSE_LOG_DEBUG("HttpServer", "File not found: " + file_path.string());
      return HttpResponse::error(404, "Not Found");
    }

    if (!std::filesystem::exists(file_path)) {
      return HttpResponse::error(404, "Not Found");
    }

    // Don't serve directories
    if (std::filesystem::is_directory(file_path)) {
      // Try index.html in directory
      auto index_path = file_path / "index.html";
      if (std::filesystem::exists(index_path)) {
        file_path = index_path;
      } else {
        return HttpResponse::error(403, "Forbidden");
      }
    }

    // Check file size (prevent serving huge files)
    auto file_size = std::filesystem::file_size(file_path);
    if (file_size > 100 * 1024 * 1024) {  // 100 MB limit
      LOG_ERROR("HttpServer", "File too large: " + file_path.string());
      return HttpResponse::error(413, "Payload Too Large");
    }

    // Read file
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
      LOG_ERROR("HttpServer", "Failed to open file: " + file_path.string());
      return HttpResponse::error(500, "Failed to read file");
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    HttpResponse response;
    response.body = content;

    // Set content type based on file extension
    auto extension = file_path.extension().string();
    response.headers["Content-Type"] = get_mime_type(extension);

    // Add caching headers for static assets
    if (extension == ".js" || extension == ".css" || extension == ".png" || extension == ".jpg" ||
        extension == ".jpeg" || extension == ".gif" || extension == ".svg" ||
        extension == ".woff" || extension == ".woff2" || extension == ".ttf") {
      // Cache static assets for 1 hour
      response.headers["Cache-Control"] = "public, max-age=3600";
    } else if (extension == ".html") {
      // Don't cache HTML files (or cache briefly)
      response.headers["Cache-Control"] = "no-cache, must-revalidate";
    }

    // Add ETag for cache validation
    auto last_write_time = std::filesystem::last_write_time(file_path);
    auto time_since_epoch = last_write_time.time_since_epoch().count();
    response.headers["ETag"] = "\"" + std::to_string(static_cast<long long>(time_since_epoch)) +
                               "-" + std::to_string(static_cast<unsigned long long>(file_size)) +
                               "\"";

    // Add security headers
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["X-Frame-Options"] = "SAMEORIGIN";

    if (config_.enable_cors) {
      response.cors();
    }

    VERBOSE_LOG_DEBUG("HttpServer", "Served file: " + file_path.string() + " (" +
                                        std::to_string(file_size) + " bytes)");

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
      nlohmann::json j;
      j["status"] = "active";
      j["server"] = "Solar System Web Server (Modern)";
      j["version"] = "4.0.0";

      // JPL data status
      nlohmann::json data_json;
      if (factory.has_current_ephemeris_data()) {
        auto epoch = factory.current_epoch();
        auto source = factory.current_source();
        std::chrono::year_month_day ymd = std::chrono::floor<std::chrono::days>(epoch);
        int cached_year = static_cast<int>(ymd.year());

        data_json["status"] = "active";
        data_json["source"] = source;
        data_json["year"] = cached_year;

        auto curr_epoch = factory.get_current_year_epoch();
        std::chrono::year_month_day curr_ymd = std::chrono::floor<std::chrono::days>(curr_epoch);
        int current_year = static_cast<int>(curr_ymd.year());

        data_json["current"] = (cached_year == current_year);
      } else {
        data_json["status"] = "hardcoded";
        data_json["source"] = "Built-in data";
        data_json["current"] = false;
      }
      j["data"] = data_json;

      // Body information using modern BodyFactory
      nlohmann::json bodies_json;
      try {
        SolarSystem::Bodies::BodyFactory local_factory;
        auto available_bodies = local_factory.get_available_bodies();
        bodies_json["total"] = available_bodies.size();
        bodies_json["essential"] = available_bodies.size();
        bodies_json["important"] = available_bodies.size();
        bodies_json["optional"] = available_bodies.size();
      } catch (const std::exception& e) {
        bodies_json["error"] = e.what();
      }
      j["bodies"] = bodies_json;

      // Timestamp
      auto now = std::chrono::system_clock::now();
      auto current_time_t = std::chrono::system_clock::to_time_t(now);
      auto tm = *std::localtime(&current_time_t);
      std::ostringstream timestamp_ss;
      timestamp_ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
      j["timestamp"] = timestamp_ss.str();

      return HttpResponse::json_response(j.dump(2));

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
      nlohmann::json j;
      j["status"] = "healthy";
      j["server"] = "Solar System Web Server";
      j["timestamp"] = std::time(nullptr);

      return HttpResponse::json_response(j.dump(2));

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Health check exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Health check failed");
    }
  }

  /**
   * @brief Get celestial bodies data with filtering and pagination
   */
  static HttpResponse handle_bodies(const HttpRequest& request,
                                    SolarSystem::Bodies::BodyFactory& factory) {
    try {
      // Parse query parameters for filtering
      auto type_filter = request.get_query_param("type");
      auto name_filter = request.get_query_param("name");
      auto limit_param = request.get_query_param("limit");
      auto offset_param = request.get_query_param("offset");

      // Parse pagination parameters
      size_t limit = 100;  // Default limit
      size_t offset = 0;   // Default offset

      if (limit_param.has_value()) {
        try {
          limit = std::stoull(*limit_param);
          if (limit > 1000) limit = 1000;  // Cap at 1000
        } catch (...) {
          return HttpResponse::error(400, "Invalid limit parameter");
        }
      }

      if (offset_param.has_value()) {
        try {
          offset = std::stoull(*offset_param);
        } catch (...) {
          return HttpResponse::error(400, "Invalid offset parameter");
        }
      }

      VERBOSE_LOG_INFO(
          "API", "Bodies request - Type: " + (type_filter.has_value() ? *type_filter : "all") +
                     ", Name: " + (name_filter.has_value() ? *name_filter : "all") +
                     ", Limit: " + std::to_string(limit) + ", Offset: " + std::to_string(offset));

      // Get available bodies from factory
      auto available_bodies = factory.get_available_bodies();

      // Build JSON response with nlohmann/json
      nlohmann::json j;
      j["total"] = available_bodies.size();
      j["limit"] = limit;
      j["offset"] = offset;
      j["bodies"] = nlohmann::json::array();

      size_t count = 0;
      size_t index = 0;

      for (const auto& body_name : available_bodies) {
        // Apply offset
        if (index < offset) {
          index++;
          continue;
        }

        // Apply limit
        if (count >= limit) {
          break;
        }

        // Apply name filter
        if (name_filter.has_value()) {
          std::string lower_name = body_name;
          std::string lower_filter = *name_filter;
          std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
          std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(), ::tolower);
          if (lower_name.find(lower_filter) == std::string::npos) {
            index++;
            continue;
          }
        }

        // Determine body type based on name (without creating the body - fast!)
        std::string body_type = "unknown";
        if (body_name == "Sun") {
          body_type = "star";
        } else if (body_name == "Mercury" || body_name == "Venus" || body_name == "Earth" ||
                   body_name == "Mars" || body_name == "Jupiter" || body_name == "Saturn" ||
                   body_name == "Uranus" || body_name == "Neptune") {
          body_type = "planet";
        } else if (body_name == "Moon") {
          body_type = "moon";
        } else if (body_name == "Pluto" || body_name == "Ceres" || body_name == "Eris") {
          body_type = "dwarf_planet";
        } else {
          body_type = "celestial_body";
        }

        // Apply type filter
        if (type_filter.has_value() && body_type != *type_filter) {
          index++;
          continue;
        }

        // Return metadata only (name and type) for fast response
        // Full body data with position/velocity available via /api/solar_system
        nlohmann::json body_json;
        body_json["name"] = body_name;
        body_json["type"] = body_type;
        j["bodies"].push_back(body_json);

        count++;
        index++;
      }

      return HttpResponse::json_response(j.dump(2));

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Bodies handler exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Failed to get bodies data");
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
          VERBOSE_LOG_INFO("API", "Solar system data requested for validated date: " +
                                      validation_result.normalized_value);
          // Modern BodyFactory provides current data automatically
          // No explicit simulation updates needed for current data
        } else {
          LOG_ERROR("API",
                    "Invalid date in solar system request: " + validation_result.error_message);
          // Return structured validation error using nlohmann/json
          nlohmann::json error_j;
          error_j["error"] = "Invalid date parameter";
          error_j["message"] = validation_result.error_message;
          error_j["expected_formats"] = validation_result.expected_formats;
          return HttpResponse::error(400, error_j.dump(2));
        }
      }

      // Get current solar system state using nlohmann/json
      nlohmann::json j;
      j["timestamp"] = std::to_string(std::time(nullptr));
      j["bodies"] = nlohmann::json::array();

      // Get actual body data from the modern BodyFactory
      try {
        SolarSystem::Bodies::BodyFactory local_factory;
        auto result = local_factory.create_solar_system();

        if (result) {
          const auto& bodies = result.value();

          for (const auto& body : bodies) {
            nlohmann::json body_json;
            body_json["name"] = body.name();
            body_json["type"] = "celestial_body";
            body_json["position"] = {
                {"x", body.position().x()}, {"y", body.position().y()}, {"z", body.position().z()}};
            body_json["velocity"] = {
                {"x", body.velocity().x()}, {"y", body.velocity().y()}, {"z", body.velocity().z()}};
            j["bodies"].push_back(body_json);
          }
        } else {
          LOG_ERROR("API", "Failed to create solar system: " + result.error());
        }
      } catch (const std::exception& e) {
        LOG_ERROR("API", "Failed to get body data: " + std::string(e.what()));

        // Fallback: return error body
        nlohmann::json error_body;
        error_body["name"] = "Error";
        error_body["type"] = "error";
        error_body["position"] = {{"x", 0}, {"y", 0}, {"z", 0}};
        error_body["velocity"] = {{"x", 0}, {"y", 0}, {"z", 0}};
        j["bodies"].push_back(error_body);
      }

      return HttpResponse::json_response(j.dump(2));

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Solar system handler exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Failed to get solar system data");
    }
  }

  /**
   * @brief Handle simulation request with state management
   */
  static HttpResponse handle_simulate(const HttpRequest& request,
                                      SolarSystem::Bodies::BodyFactory&) {
    try {
      auto date_param = request.get_query_param("date");
      auto speed_param = request.get_query_param("speed");
      auto steps_param = request.get_query_param("steps");
      auto timestep_param = request.get_query_param("timestep");

      VERBOSE_LOG_INFO("API", "Simulation request - Date: " +
                                  (date_param.has_value() ? *date_param : "current") +
                                  ", Speed: " + (speed_param.has_value() ? *speed_param : "1.0"));

      // Parse simulation parameters
      double speed = 1.0;
      if (speed_param.has_value()) {
        try {
          speed = std::stod(*speed_param);
          if (speed <= 0.0) speed = 1.0;
        } catch (...) {
          return HttpResponse::error(400, "Invalid speed parameter");
        }
      }

      size_t steps = 100;  // Default steps
      if (steps_param.has_value()) {
        try {
          steps = std::stoull(*steps_param);
          if (steps > 10000) steps = 10000;  // Cap at 10000
        } catch (...) {
          return HttpResponse::error(400, "Invalid steps parameter");
        }
      }

      double timestep = 3600.0;  // Default 1 hour
      if (timestep_param.has_value()) {
        try {
          timestep = std::stod(*timestep_param);
          if (timestep <= 0.0) timestep = 3600.0;
        } catch (...) {
          return HttpResponse::error(400, "Invalid timestep parameter");
        }
      }

      // Use modern SimulationBuilder for time travel
      using namespace SolarSystem::Core::Builders;

      // Create body selector for web interface (essential bodies for optimal web performance)
      BodySelector selector;
      selector.body_set(SolarSystem::Bodies::BodyFactory::DefaultBodySet::ESSENTIAL);

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
            VERBOSE_LOG_INFO(
                "API", "Time travel to validated date: " + validation_result.normalized_value);
          } else {
            LOG_ERROR("API",
                      "Failed to parse validated date: " + validation_result.normalized_value);
          }
        } else {
          LOG_ERROR("API", "Invalid date format: " + validation_result.error_message);
          // Return error response with validation details using nlohmann/json
          nlohmann::json error_j;
          error_j["error"] = "Invalid date format";
          error_j["message"] = validation_result.error_message;
          error_j["expected_formats"] = validation_result.expected_formats;
          if (!validation_result.suggestions.empty()) {
            error_j["suggestions"] = validation_result.suggestions;
          }
          error_j["provided_value"] = *date_param;
          return HttpResponse::error(400, error_j.dump(2));
        }
      }

      // Create and configure simulation
      SimulationBuilder sim_builder;
      std::string error_message;
      // cppcheck-suppress accessMoved
      auto simulation = sim_builder.with_bodies(std::move(bodies))
                            .with_timestep(timestep * speed)
                            .with_max_iterations(steps)
                            .build(&error_message);

      if (!simulation) {
        LOG_ERROR("API", "Failed to build simulation: " + error_message);
        return HttpResponse::error(500, "Simulation configuration failed: " + error_message);
      }

      // Run simulation steps
      size_t completed_steps = 0;
      for (size_t i = 0; i < steps; ++i) {
        auto step_result = simulation->step();
        if (step_result.has_value()) {
          completed_steps++;
        } else {
          LOG_ERROR("API", "Simulation step failed: " + step_result.error());
          break;
        }
      }

      // Get final state
      const auto& final_bodies = simulation->get_bodies();

      // Build response with simulation results using nlohmann/json
      nlohmann::json j;
      j["status"] = "success";
      j["message"] = "Simulation completed";

      j["configuration"] = {{"date", date_param.has_value() ? *date_param : "current"},
                            {"speed", speed},
                            {"timestep", timestep},
                            {"requested_steps", steps},
                            {"completed_steps", completed_steps}};

      nlohmann::json results;
      results["body_count"] = final_bodies.size();
      results["simulation_time"] = static_cast<double>(completed_steps) * timestep * speed;
      results["bodies"] = nlohmann::json::array();

      for (const auto& body : final_bodies) {
        nlohmann::json body_json;
        body_json["name"] = body.name();
        body_json["position"] = {
            {"x", body.position().x()}, {"y", body.position().y()}, {"z", body.position().z()}};
        body_json["velocity"] = {
            {"x", body.velocity().x()}, {"y", body.velocity().y()}, {"z", body.velocity().z()}};
        results["bodies"].push_back(body_json);
      }

      j["results"] = results;

      return HttpResponse::json_response(j.dump(2));

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Simulate handler exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Simulation failed: " + std::string(e.what()));
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
    std::cout << "  GET  /api/health           # Health check\n";
    std::cout << "  GET  /api/status           # Server and system status\n";
    std::cout << "  GET  /api/bodies           # List all celestial bodies\n";
    std::cout << "  GET  /api/bodies?type=planet&limit=10  # Filter and paginate bodies\n";
    std::cout << "  GET  /api/solar_system     # Current solar system state\n";
    std::cout << "  GET  /api/solar_system?date=YYYY-MM-DD  # Historical data\n";
    std::cout << "  GET  /api/simulation       # Run simulation with parameters\n";
    std::cout << "  GET  /api/simulation?date=YYYY-MM-DD&steps=100  # Time travel simulation\n\n";

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
        .handle("/api/bodies",
                [factory](const HttpRequest& req) {
                  return SolarSystemAPI::handle_bodies(req, *factory);
                })
        .handle("/api/solar_system",
                [factory](const HttpRequest& req) {
                  return SolarSystemAPI::handle_solar_system(req, *factory);
                })
        .handle("/api/simulation",
                [factory](const HttpRequest& req) {
                  return SolarSystemAPI::handle_simulate(req, *factory);
                })
        .handle("/api/simulate", [factory](const HttpRequest& req) {
          return SolarSystemAPI::handle_simulate(req, *factory);
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
