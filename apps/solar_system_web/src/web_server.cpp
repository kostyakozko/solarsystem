/**
 * Solar System Web Server
 *
 * Lightweight HTTP server providing REST API and WebSocket support
 * for browser-based solar system visualization and control.
 * Built using only standard libraries - no external dependencies.
 */

#include <signal.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Network includes
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Include our modular libraries
#include "args.h"
#include "jpl_data.h"
#include "model.h"
#include "simulation.h"

// Global server control
volatile bool server_running = true;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    std::cout << "\nReceived shutdown signal. Stopping web server...\n";
    server_running = false;
  }
}

// Web server configuration
struct WebServerConfig {
  int port;
  std::string web_root;
  bool enable_cors;
  bool verbose;

  WebServerConfig() : port(8080), web_root("./web"), enable_cors(true), verbose(false) {}
};

// HTTP Response structure
struct HttpResponse {
  int status_code;
  std::string status_text;
  std::map<std::string, std::string> headers;
  std::string body;

  HttpResponse(int code = 200, const std::string& text = "OK")
      : status_code(code), status_text(text) {
    headers["Content-Type"] = "text/html";
    headers["Server"] = "SolarSystemSuite/2.1.0";
  }
};

// HTTP Request structure
struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
  std::map<std::string, std::string> headers;
  std::string body;
};

// Parse HTTP request
HttpRequest parse_http_request(const std::string& request_data) {
  HttpRequest request;
  std::istringstream stream(request_data);
  std::string line;

  // Parse request line
  if (std::getline(stream, line)) {
    std::istringstream request_line(line);
    request_line >> request.method >> request.path >> request.version;
  }

  // Parse headers
  while (std::getline(stream, line) && line != "\r" && !line.empty()) {
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
      std::string key = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 2);  // Skip ": "
      // Remove \r if present
      if (!value.empty() && value.back() == '\r') {
        value.pop_back();
      }
      request.headers[key] = value;
    }
  }

  // Parse body (if any)
  std::string body_line;
  while (std::getline(stream, body_line)) {
    request.body += body_line + "\n";
  }

  return request;
}

// Generate HTTP response string
std::string generate_http_response(const HttpResponse& response) {
  std::ostringstream stream;

  // Status line
  stream << "HTTP/1.1 " << response.status_code << " " << response.status_text << "\r\n";

  // Headers
  for (const auto& header : response.headers) {
    stream << header.first << ": " << header.second << "\r\n";
  }

  // Content-Length
  stream << "Content-Length: " << response.body.length() << "\r\n";

  // End of headers
  stream << "\r\n";

  // Body
  stream << response.body;

  return stream.str();
}

// Read file content
std::string read_file(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return "";
  }

  std::ostringstream content;
  content << file.rdbuf();
  return content.str();
}

// Get MIME type based on file extension
std::string get_mime_type(const std::string& filepath) {
  size_t dot_pos = filepath.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return "text/plain";
  }

  std::string extension = filepath.substr(dot_pos + 1);

  if (extension == "html" || extension == "htm") return "text/html";
  if (extension == "css") return "text/css";
  if (extension == "js") return "application/javascript";
  if (extension == "json") return "application/json";
  if (extension == "png") return "image/png";
  if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
  if (extension == "gif") return "image/gif";
  if (extension == "svg") return "image/svg+xml";

  return "text/plain";
}

// Generate JSON response for solar system data with optional date parameter
std::string generate_solar_system_json(const std::string& date_param = "") {
  std::ostringstream json;
  json << "{\n";
  json << "  \"timestamp\": " << time(NULL) << ",\n";

  // If date parameter is provided, simulate to that date
  if (!date_param.empty()) {
    // Parse the date and simulate to that time
    // For now, we'll use the current positions but this is where
    // we would integrate with the C++ simulation engine
    json << "  \"simulated_date\": \"" << date_param << "\",\n";
  }

  json << "  \"bodies\": [\n";

  for (int i = 0; i < get_body_count(); i++) {
    const planet& body = get_body(i);

    json << "    {\n";
    json << "      \"name\": \"" << body.name << "\",\n";
    json << "      \"mass\": " << body.mass << ",\n";
    json << "      \"position\": {\n";
    json << "        \"x\": " << body.position.x << ",\n";
    json << "        \"y\": " << body.position.y << ",\n";
    json << "        \"z\": " << body.position.z << "\n";
    json << "      },\n";
    json << "      \"velocity\": {\n";
    json << "        \"x\": " << body.speed.x << ",\n";
    json << "        \"y\": " << body.speed.y << ",\n";
    json << "        \"z\": " << body.speed.z << "\n";
    json << "      }\n";
    json << "    }";

    if (i < get_body_count() - 1) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ]\n";
  json << "}\n";

  return json.str();
}

// Generate system status JSON
std::string generate_status_json() {
  std::ostringstream json;
  json << "{\n";
  json << "  \"timestamp\": " << time(NULL) << ",\n";
  json << "  \"data_status\": {\n";

  if (has_current_ephemeris_data()) {
    time_t epoch = get_ephemeris_epoch();
    const char* source = get_ephemeris_source();

    json << "    \"available\": true,\n";
    json << "    \"source\": \"" << source << "\",\n";
    json << "    \"epoch\": " << epoch << ",\n";

    // Check data currency
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    struct tm* tm_epoch = localtime(&epoch);
    int current_year = tm_now->tm_year + 1900;
    int cached_year = tm_epoch->tm_year + 1900;

    json << "    \"current\": " << (cached_year == current_year ? "true" : "false") << "\n";
  } else {
    json << "    \"available\": false,\n";
    json << "    \"source\": \"hardcoded\",\n";
    json << "    \"current\": false\n";
  }

  json << "  },\n";
  json << "  \"body_count\": " << get_body_count() << "\n";
  json << "}\n";

  return json.str();
}

// Handle HTTP request
HttpResponse handle_request(const HttpRequest& request, const WebServerConfig& config) {
  HttpResponse response;

  // Add CORS headers if enabled
  if (config.enable_cors) {
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    response.headers["Access-Control-Allow-Headers"] = "Content-Type";
  }

  // Handle OPTIONS request (CORS preflight)
  if (request.method == "OPTIONS") {
    response.status_code = 200;
    response.body = "";
    return response;
  }

  // API endpoints
  if (request.path == "/api/status") {
    response.headers["Content-Type"] = "application/json";
    response.body = generate_status_json();
    return response;
  }

  if (request.path == "/api/solar_system" || request.path.find("/api/solar_system?") == 0) {
    response.headers["Content-Type"] = "application/json";

    // Extract date parameter if present
    std::string date_param = "";
    size_t date_pos = request.path.find("date=");
    if (date_pos != std::string::npos) {
      size_t start = date_pos + 5;  // Skip "date="
      size_t end = request.path.find("&", start);
      if (end == std::string::npos) end = request.path.length();
      date_param = request.path.substr(start, end - start);
    }

    response.body = generate_solar_system_json(date_param);
    return response;
  }

  // Static file serving
  std::string filepath = config.web_root + request.path;

  // Default to index.html for root path
  if (request.path == "/") {
    filepath = config.web_root + "/index.html";
  }

  std::string content = read_file(filepath);
  if (content.empty()) {
    response.status_code = 404;
    response.status_text = "Not Found";
    response.body =
        "<html><body><h1>404 Not Found</h1><p>The requested resource was not "
        "found.</p></body></html>";
    return response;
  }

  response.headers["Content-Type"] = get_mime_type(filepath);
  response.body = content;

  return response;
}

// Handle client connection
void handle_client(int client_socket, const WebServerConfig& config) {
  char buffer[4096];
  ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

  if (bytes_received > 0) {
    buffer[bytes_received] = '\0';

    HttpRequest request = parse_http_request(std::string(buffer));

    if (config.verbose) {
      std::cout << "Request: " << request.method << " " << request.path << std::endl;
    }

    HttpResponse response = handle_request(request, config);
    std::string response_str = generate_http_response(response);

    send(client_socket, response_str.c_str(), response_str.length(), 0);
  }

  close(client_socket);
}

// Print usage information
void print_web_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [OPTIONS]\n";
  std::cout << "Solar System Web Server - Browser-based visualization and API\n\n";

  std::cout << "SERVER OPTIONS:\n";
  std::cout << "  -p, --port PORT        Server port (default: 8080)\n";
  std::cout << "  -w, --web-root DIR     Web root directory (default: ./web)\n";
  std::cout << "  --no-cors              Disable CORS headers\n\n";

  std::cout << "OUTPUT OPTIONS:\n";
  std::cout << "  -v, --verbose          Enable verbose output\n";
  std::cout << "  -h, --help             Show this help message\n\n";

  std::cout << "API ENDPOINTS:\n";
  std::cout << "  GET /api/status        System status and data information\n";
  std::cout << "  GET /api/solar_system  Current solar system state (JSON)\n\n";

  std::cout << "EXAMPLES:\n";
  std::cout << "  # Start web server on default port 8080\n";
  std::cout << "  " << program_name << "\n\n";

  std::cout << "  # Custom port and web root\n";
  std::cout << "  " << program_name << " --port 3000 --web-root /path/to/web\n\n";

  std::cout << "  # Verbose mode\n";
  std::cout << "  " << program_name << " --verbose\n\n";

  std::cout << "CONTROLS:\n";
  std::cout << "  Ctrl+C                 Graceful shutdown\n\n";
}

// Parse command line arguments
bool parse_web_args(int argc, char* argv[], WebServerConfig& config) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_web_usage(argv[0]);
      return false;
    } else if (arg == "-p" || arg == "--port") {
      if (i + 1 < argc) {
        try {
          config.port = std::stoi(argv[++i]);
          if (config.port < 1 || config.port > 65535) {
            std::cerr << "Error: Port must be between 1 and 65535\n";
            return false;
          }
        } catch (const std::exception& e) {
          std::cerr << "Error: Invalid port number: " << argv[i] << "\n";
          return false;
        }
      } else {
        std::cerr << "Error: --port requires a value\n";
        return false;
      }
    } else if (arg == "-w" || arg == "--web-root") {
      if (i + 1 < argc) {
        config.web_root = argv[++i];
      } else {
        std::cerr << "Error: --web-root requires a value\n";
        return false;
      }
    } else if (arg == "--no-cors") {
      config.enable_cors = false;
    } else if (arg == "-v" || arg == "--verbose") {
      config.verbose = true;
    } else {
      std::cerr << "Error: Unknown option: " << arg << "\n";
      return false;
    }
  }

  return true;
}

int main(int argc, char* argv[]) {
  WebServerConfig config;

  // Parse arguments
  if (!parse_web_args(argc, argv, config)) {
    return 1;
  }

  // Set up signal handlers
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Initialize JPL data system
  if (!initialize_jpl_data()) {
    std::cerr << "Failed to initialize JPL data system\n";
    return 1;
  }

  // Initialize simulation to current time
  initialize_simulation_to_current_time();

  // Create server socket
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    std::cerr << "Failed to create socket\n";
    return 1;
  }

  // Set socket options
  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "Failed to set socket options\n";
    close(server_socket);
    return 1;
  }

  // Bind socket
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(config.port);

  if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "Failed to bind socket to port " << config.port << "\n";
    close(server_socket);
    return 1;
  }

  // Listen for connections
  if (listen(server_socket, 10) < 0) {
    std::cerr << "Failed to listen on socket\n";
    close(server_socket);
    return 1;
  }

  std::cout << "🌐 Solar System Web Server Started\n";
  std::cout << "📡 Server: http://localhost:" << config.port << "\n";
  std::cout << "📁 Web root: " << config.web_root << "\n";
  std::cout << "🔄 CORS: " << (config.enable_cors ? "enabled" : "disabled") << "\n";
  std::cout << "🛑 Press Ctrl+C to stop\n\n";

  // Main server loop
  while (server_running) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
      if (server_running) {
        std::cerr << "Failed to accept client connection\n";
      }
      continue;
    }

    // Handle client in separate thread
    std::thread client_thread(handle_client, client_socket, config);
    client_thread.detach();
  }

  close(server_socket);
  std::cout << "\n🛑 Web server stopped.\n";

  return 0;
}
