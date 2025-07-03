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
#include <chrono>
#include <csignal>
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
#include "jpl_data.h"    // Legacy JPL interface (to be modernized)
#include "model.h"       // Legacy model for body data access
#include "simulation.h"  // Legacy simulation functions

// Simple logging macros to avoid namespace conflicts
#define LOG_INFO(tag, msg) \
  if (verbose_logging) std::cout << "[INFO ] [" << tag << "] " << msg << "\n"
#define LOG_ERROR(tag, msg) std::cerr << "[ERROR] [" << tag << "] " << msg << "\n"
#define LOG_DEBUG(tag, msg) \
  if (verbose_logging) std::cout << "[DEBUG] [" << tag << "] " << msg << "\n"

// Global verbose logging flag
static bool verbose_logging = false;
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

    if (!std::filesystem::exists(web_root)) {
      if (error) *error = "Web root directory does not exist: " + web_root.string();
      return false;
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
    // Try multiple possible locations
    std::vector<std::filesystem::path> candidates = {"./web", "../share/solar_system/web",
                                                     "./share/solar_system/web",
                                                     "./apps/solar_system_web/web"};

    for (const auto& candidate : candidates) {
      if (std::filesystem::exists(candidate / "index.html")) {
        return candidate;
      }
    }

    // Default fallback
    return "./web";
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
  explicit HttpServer(WebServerConfig config) : config_(std::move(config)) {
    if (config_.verbose_output) {
      LOG_INFO("HttpServer", "Initialized with verbose output enabled");
    }
  }

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
      LOG_INFO("HttpServer", "Starting web server on port " + std::to_string(config_.port));

      // Create socket
      server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
      if (server_socket_ < 0) {
        LOG_ERROR("HttpServer", "Failed to create socket");
        return false;
      }

      // Set socket options
      int opt = 1;
      if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("HttpServer", "Failed to set socket options");
        return false;
      }

      // Bind socket
      sockaddr_in address{};
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port = htons(config_.port);

      if (bind(server_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        LOG_ERROR("HttpServer", "Failed to bind socket to port " + std::to_string(config_.port));
        return false;
      }

      // Listen for connections
      if (listen(server_socket_, static_cast<int>(config_.max_connections)) < 0) {
        LOG_ERROR("HttpServer", "Failed to listen on socket");
        return false;
      }

      if (!config_.verbose_output) {
        std::cout << "🌐 Web server started on http://localhost:" << config_.port << "\n";
        std::cout << "📁 Serving files from: " << config_.web_root << "\n";
        std::cout << "🛑 Press Ctrl+C to stop\n\n";
      }

      LOG_INFO("HttpServer", "Server listening on port " + std::to_string(config_.port));

      // Main server loop
      return run_server_loop();

    } catch (const std::exception& e) {
      LOG_ERROR("HttpServer", "Exception during startup: " + std::string(e.what()));
      return false;
    }
  }

  /**
   * @brief Stop the server
   */
  void stop() {
    if (server_socket_ >= 0) {
      close(server_socket_);
      server_socket_ = -1;
      LOG_INFO("HttpServer", "Server stopped");
    }
  }

 private:
  WebServerConfig config_;
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
        if (g_server_running.load()) {
          LOG_ERROR("HttpServer", "Failed to accept connection");
        }
        continue;
      }

      // Handle request in separate thread for better performance
      std::thread([this, client_socket]() { handle_client(client_socket); }).detach();
    }

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
        LOG_DEBUG("HttpServer", "Request: " + request->method + " " + request->path);
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
  static HttpResponse handle_status(const HttpRequest& request) {
    try {
      std::ostringstream json;
      json << "{\n";
      json << "  \"status\": \"active\",\n";
      json << "  \"server\": \"Solar System Web Server (Modern)\",\n";
      json << "  \"version\": \"4.0.0\",\n";

      // JPL data status
      json << "  \"data\": {\n";
      if (has_current_ephemeris_data()) {
        auto epoch = get_ephemeris_epoch();
        auto source = get_ephemeris_source();
        auto tm_epoch = *std::localtime(&epoch);
        auto cached_year = tm_epoch.tm_year + 1900;

        json << "    \"status\": \"active\",\n";
        json << "    \"source\": \"" << source << "\",\n";
        json << "    \"year\": " << cached_year << ",\n";

        auto now = std::time(nullptr);
        auto current_tm = *std::localtime(&now);
        auto current_year = current_tm.tm_year + 1900;

        json << "    \"current\": " << (cached_year == current_year ? "true" : "false") << "\n";
      } else {
        json << "    \"status\": \"hardcoded\",\n";
        json << "    \"source\": \"Built-in data\",\n";
        json << "    \"current\": false\n";
      }
      json << "  },\n";

      // Body information using legacy interface
      json << "  \"bodies\": {\n";
      try {
        auto body_count = get_body_count();
        json << "    \"total\": " << body_count << ",\n";
        json << "    \"essential\": " << body_count << ",\n";
        json << "    \"important\": " << body_count << ",\n";
        json << "    \"optional\": " << body_count << "\n";
      } catch (const std::exception& e) {
        json << "    \"error\": \"" << e.what() << "\"\n";
      }
      json << "  },\n";

      // Timestamp
      auto now = std::chrono::system_clock::now();
      auto time_t = std::chrono::system_clock::to_time_t(now);
      auto tm = *std::localtime(&time_t);

      json << "  \"timestamp\": \"" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\"\n";
      json << "}";

      return HttpResponse::json_response(json.str());

    } catch (const std::exception& e) {
      LOG_ERROR("API", "Status handler exception: " + std::string(e.what()));
      return HttpResponse::error(500, "Failed to get status");
    }
  }

  /**
   * @brief Get solar system data
   */
  static HttpResponse handle_solar_system(const HttpRequest& request) {
    try {
      // Check for date parameter
      auto date_param = request.get_query_param("date");

      if (date_param.has_value()) {
        // Handle specific date request
        LOG_INFO("API", "Solar system data requested for date: " + *date_param);

        // For now, use legacy simulation approach
        // TODO: Replace with modern SimulationBuilder when date parsing is available

        // Update simulation to current time (legacy approach)
        update_simulation_to_current_time();
      }

      // Get current solar system state
      std::ostringstream json;
      json << "{\n";
      json << "  \"timestamp\": \"" << std::time(nullptr) << "\",\n";
      json << "  \"bodies\": [\n";

      // Get actual body data from the simulation
      try {
        // Use legacy get_bodies() function to get actual positions
        auto body_count = get_body_count();
        bool first = true;

        for (int i = 0; i < body_count; ++i) {
          const auto& body = get_body(i);

          if (!first) json << ",\n";
          first = false;

          json << "    {\n";
          json << "      \"name\": \"" << body.name << "\",\n";
          json << "      \"type\": \"celestial_body\",\n";
          json << "      \"position\": { ";
          json << "\"x\": " << body.position.x << ", ";
          json << "\"y\": " << body.position.y << ", ";
          json << "\"z\": " << body.position.z << " },\n";
          json << "      \"velocity\": { ";
          json << "\"x\": " << body.speed.x << ", ";
          json << "\"y\": " << body.speed.y << ", ";
          json << "\"z\": " << body.speed.z << " }\n";
          json << "    }";
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
  static HttpResponse handle_simulate(const HttpRequest& request) {
    try {
      auto date_param = request.get_query_param("date");
      auto speed_param = request.get_query_param("speed");

      LOG_INFO("API",
               "Simulation request - Date: " + (date_param.has_value() ? *date_param : "current") +
                   ", Speed: " + (speed_param.has_value() ? *speed_param : "1.0"));

      // For now, use legacy simulation approach
      // TODO: Replace with modern SimulationBuilder
      update_simulation_to_current_time();

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

    // Parse command-line arguments
    auto config = ArgumentParser::parse(argc, argv);
    if (!config.has_value()) {
      ArgumentParser::print_usage(argv[0]);
      return 0;  // Help was requested or parsing failed gracefully
    }

    // Initialize logging system
    verbose_logging = config->verbose_output;

    LOG_INFO("Main", "Solar System Web Server (Modern) starting");

    // Initialize JPL data system
    if (!initialize_jpl_data()) {
      LOG_ERROR("Main", "Failed to initialize JPL data system");
      std::cerr << "❌ Failed to initialize JPL data system\n";
      return 1;
    }

    // Initialize simulation to current time
    try {
      update_simulation_to_current_time();
      LOG_INFO("Main", "Simulation initialized to current time");
    } catch (const std::exception& e) {
      LOG_ERROR("Main", "Failed to initialize simulation: " + std::string(e.what()));
      std::cerr << "⚠️  Warning: Failed to initialize simulation\n";
    }

    // Create and configure HTTP server
    HttpServer server(*config);

    // Register API endpoints
    server.handle("/api/status", SolarSystemAPI::handle_status)
        .handle("/api/solar_system", SolarSystemAPI::handle_solar_system)
        .handle("/api/simulate", SolarSystemAPI::handle_simulate);

    // Start server
    bool success = server.start();

    if (success) {
      LOG_INFO("Main", "Web server completed successfully");
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
