/**
 * Solar System Web Server
 *
 * Lightweight HTTP server providing REST API and WebSocket support
 * for browser-based solar system visualization and control.
 * Built using only standard libraries - no external dependencies.
 */

#include <errno.h>   // For errno
#include <fcntl.h>   // For fcntl()
#include <libgen.h>  // For dirname()
#include <limits.h>  // For PATH_MAX
#include <signal.h>
#include <unistd.h>  // For readlink()

#include <chrono>
#include <cstring>  // For strerror()
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
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

// Function to get the default web root path
std::string get_default_web_root() {
  // Try the standard installation layout: ../share/solar_system/web
  std::string web_root = "../share/solar_system/web";

  std::cout << "🔍 Trying web root: " << web_root << std::endl;

  std::ifstream test_file(web_root + "/index.html");
  if (test_file.good()) {
    std::cout << "✅ Found web files at: " << web_root << std::endl;
    return web_root;
  }

  // Try alternative: ./share/solar_system/web (if we're in install root)
  web_root = "./share/solar_system/web";
  std::cout << "🔍 Trying alternative web root: " << web_root << std::endl;

  std::ifstream test_file2(web_root + "/index.html");
  if (test_file2.good()) {
    std::cout << "✅ Found web files at: " << web_root << std::endl;
    return web_root;
  }

  // Fallback to current directory
  std::cout << "⚠️ Could not find web files, falling back to ./web" << std::endl;
  return "./web";
}

// Store current simulation state
struct SimulationState {
  time_t current_simulation_time;
  bool is_initialized;
  std::string last_simulated_date;
};

SimulationState current_sim_state = {0, false, ""};

// Initialize simulation state with current JPL epoch
void initialize_simulation_state() {
  if (!current_sim_state.is_initialized) {
    current_sim_state.current_simulation_time = get_ephemeris_epoch();
    current_sim_state.is_initialized = true;
    current_sim_state.last_simulated_date = "";  // Will be set after first simulation
    std::cout << "🔧 Initialized simulation state with JPL epoch: "
              << current_sim_state.current_simulation_time << std::endl;
  }
}

// Thread safety for simulation requests
std::mutex simulation_mutex;

// Store original JPL data state
struct SystemState {
  planet bodies[BODY_COUNT];
  time_t epoch;
  bool saved;
};

SystemState original_state = {{}, 0, false};

// Save current system state (assumes mutex is already held by caller)
void save_system_state() {
  if (!original_state.saved) {
    std::cout << "💾 Copying system state..." << std::endl;
    for (int i = 0; i < get_body_count(); i++) {
      original_state.bodies[i] = get_body(i);
    }
    original_state.epoch = get_ephemeris_epoch();
    original_state.saved = true;
    std::cout << "✅ System state copied successfully" << std::endl;
  } else {
    std::cout << "ℹ️ System state already saved, skipping" << std::endl;
  }
}

// Restore original system state (assumes mutex is already held by caller)
void restore_system_state() {
  if (original_state.saved) {
    std::cout << "🔄 Restoring system state..." << std::endl;
    for (int i = 0; i < get_body_count(); i++) {
      SolarSystem[i] = original_state.bodies[i];
    }
    std::cout << "✅ System state restored successfully" << std::endl;
    // Note: We can't easily restore the epoch, but that's OK for our use case
    // The simulation will work from the restored positions
  } else {
    std::cout << "⚠️ No saved state to restore" << std::endl;
  }
}
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

  WebServerConfig()
      : port(8080), web_root(get_default_web_root()), enable_cors(true), verbose(false) {}
};

// HTTP Response structure
struct HttpResponse {
  int status_code;
  std::string status_text;
  std::map<std::string, std::string> headers;
  std::string body;

  explicit HttpResponse(int code = 200, const std::string& text = "OK")
      : status_code(code), status_text(text) {
    headers["Content-Type"] = "text/html";
    headers["Server"] = "SolarSystemSuite/2.1.0";
    // Permissive CSP for local development - allows all JavaScript execution
    headers["Content-Security-Policy"] =
        "default-src 'self' 'unsafe-inline' 'unsafe-eval'; script-src 'self' 'unsafe-inline' "
        "'unsafe-eval'; style-src 'self' 'unsafe-inline';";
    // CORS headers for API requests
    headers["Access-Control-Allow-Origin"] = "*";
    headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    headers["Access-Control-Allow-Headers"] = "Content-Type";
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
std::string generate_solar_system_json(const std::string& date_param = "",
                                       bool is_manual_request = false, bool verbose = false) {
  if (verbose) {
    std::cout << "🔧 Starting JSON generation for date: "
              << (date_param.empty() ? "current" : date_param) << std::endl;
  }

  std::ostringstream json;
  json << "{\n";
  json << "  \"timestamp\": " << time(NULL) << ",\n";

  // If date parameter is provided, run simulation to that date
  if (!date_param.empty()) {
    std::cout << "📅 Parsing date parameter: " << date_param << std::endl;

    // Parse the date and run simulation
    struct tm tm = {};
    if (sscanf(date_param.c_str(), "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
      std::cout << "✅ Date parsed successfully: " << tm.tm_year << "-" << tm.tm_mon << "-"
                << tm.tm_mday << std::endl;

      tm.tm_year -= 1900;  // tm_year is years since 1900
      tm.tm_mon -= 1;      // tm_mon is 0-based
      tm.tm_hour = 12;     // Noon
      time_t target_date = mktime(&tm);

      std::cout << "🎯 Target date timestamp: " << target_date << std::endl;

      // Thread-safe simulation with timeout protection
      {
        std::cout << "🔒 Attempting to acquire simulation lock (non-blocking)..." << std::endl;

        // Try to acquire lock with timeout instead of blocking indefinitely
        std::unique_lock<std::mutex> lock(simulation_mutex, std::defer_lock);

        // Initialize simulation state if needed
        initialize_simulation_state();

        if (!lock.try_lock()) {
          std::cout << "⚠️ Simulation already in progress, returning cached data" << std::endl;
          json << "  \"error\": \"Simulation busy, using cached data\",\n";
          json << "  \"simulation_mode\": false,\n";
        } else {
          std::cout << "✅ Simulation lock acquired" << std::endl;

          try {
            std::cout << "💾 Saving system state..." << std::endl;
            // Save original state if not already saved
            save_system_state();
            std::cout << "✅ System state saved" << std::endl;

            // DON'T automatically restore - let smart logic decide
            // std::cout << "🔄 Restoring system state..." << std::endl;
            // restore_system_state();
            // std::cout << "✅ System state restored" << std::endl;

            // Run simulation to target date with smart starting point selection
            time_t jpl_epoch = get_ephemeris_epoch();

            // Initialize simulation state if not done yet
            if (!current_sim_state.is_initialized) {
              current_sim_state.current_simulation_time = jpl_epoch;
              current_sim_state.is_initialized = true;
              std::cout << "🎯 Initialized simulation state to JPL epoch: " << jpl_epoch
                        << std::endl;
            }

            // Smart starting point: use cached data if it's closer to target than JPL epoch
            time_t smart_start_time;
            time_t jpl_to_target = abs(target_date - jpl_epoch);
            time_t cached_to_target = abs(target_date - current_sim_state.current_simulation_time);

            std::cout << "🔍 CACHE DEBUG:" << std::endl;
            std::cout << "    last_simulated_date: '" << current_sim_state.last_simulated_date
                      << "'" << std::endl;
            std::cout << "    current_simulation_time: "
                      << current_sim_state.current_simulation_time << std::endl;
            std::cout << "    is_initialized: " << current_sim_state.is_initialized << std::endl;
            std::cout << "    jpl_epoch: " << jpl_epoch << std::endl;
            std::cout << "    target_date: " << target_date << std::endl;
            std::cout << "    jpl_to_target: " << jpl_to_target << " seconds ("
                      << (jpl_to_target / 86400) << " days)" << std::endl;
            std::cout << "    cached_to_target: " << cached_to_target << " seconds ("
                      << (cached_to_target / 86400) << " days)" << std::endl;

            if (current_sim_state.last_simulated_date.empty()) {
              // No cached data, must start from JPL epoch
              smart_start_time = jpl_epoch;
              std::cout << "📍 No cached data, starting from JPL epoch" << std::endl;
            } else if (cached_to_target < jpl_to_target) {
              // Cached data is closer to target - DON'T RESTORE, keep current state
              smart_start_time = current_sim_state.current_simulation_time;
              std::cout << "📍 Using cached simulation state (closer to target) - NO RESTORE"
                        << std::endl;
              std::cout << "    Cached date: " << current_sim_state.last_simulated_date
                        << ", distance to target: " << (cached_to_target / 86400) << " days"
                        << std::endl;
            } else {
              // JPL epoch is closer to target - restore to JPL state
              smart_start_time = jpl_epoch;
              std::cout << "📍 Using JPL epoch as starting point (closer to target)" << std::endl;
              std::cout << "    JPL distance: " << (jpl_to_target / 86400)
                        << " days, Cached distance: " << (cached_to_target / 86400) << " days"
                        << std::endl;
              std::cout << "🔄 Restoring to JPL state..." << std::endl;
              restore_system_state();
              std::cout << "✅ Restored to JPL state" << std::endl;
            }

            time_t time_diff = abs(target_date - smart_start_time);

            if (verbose) {
              std::cout << "📊 Smart start: " << smart_start_time << ", Target: " << target_date
                        << ", Diff: " << time_diff << " seconds (" << (time_diff / 86400)
                        << " days)" << std::endl;
            }

            if (is_manual_request) {
              // Manual requests: Use JPL data fetching for any date
              if (verbose) {
                std::cout << "📡 Manual request: Fetching JPL data for date: " << date_param
                          << std::endl;
              }

              // Try to fetch JPL data for the specific date
              bool jpl_success = fetch_jpl_data_for_date(date_param.c_str());

              if (jpl_success) {
                if (verbose) {
                  std::cout << "✅ JPL data fetched successfully for " << date_param << std::endl;
                }
                current_sim_state.current_simulation_time = target_date;
                current_sim_state.last_simulated_date = date_param;
                json << "  \"simulated_date\": \"" << date_param << "\",\n";
                json << "  \"simulation_mode\": true,\n";
                json << "  \"data_source\": \"JPL_HORIZONS\",\n";
              } else {
                if (verbose) {
                  std::cout << "❌ Failed to fetch JPL data for " << date_param << std::endl;
                }
                json << "  \"error\": \"Failed to fetch JPL data for requested date\",\n";
                json << "  \"simulation_mode\": false,\n";
                json << "  \"current_sim_date\": \"" << current_sim_state.last_simulated_date
                     << "\",\n";
              }
            } else {
              // Automatic requests: Use simulation with reasonable limits
              const time_t max_auto_simulation_days = 30;  // 30 days max for automatic requests
              const time_t max_auto_simulation_seconds = max_auto_simulation_days * 24 * 3600;

              if (time_diff > max_auto_simulation_seconds) {
                if (verbose) {
                  std::cout << "⚠️ Automatic request range too large (" << (time_diff / 86400)
                            << " days), exceeds 30 day limit. Using cached data." << std::endl;
                }

                json << "  \"error\": \"Time jump too large for automatic simulation (>"
                     << (time_diff / 86400) << " days)\",\n";
                json << "  \"simulation_mode\": false,\n";
                json << "  \"current_sim_date\": \"" << current_sim_state.last_simulated_date
                     << "\",\n";
                json << "  \"suggested_action\": \"Use 'Start Time Travel' button for large "
                        "jumps\",\n";
              } else {
                if (verbose) {
                  std::cout << "🚀 Automatic request: Running simulation for "
                            << (time_diff / 86400) << " days..." << std::endl;
                }

                // Run the simulation
                bool success = false;
                if (target_date > smart_start_time) {
                  if (verbose) {
                    std::cout << "⏩ Running forward simulation..." << std::endl;
                  }
                  success = run_web_forward_simulation(smart_start_time, target_date);
                } else {
                  if (verbose) {
                    std::cout << "⏪ Running backward simulation..." << std::endl;
                  }
                  success = run_web_backward_simulation(smart_start_time, target_date);
                }

                if (success) {
                  if (verbose) {
                    std::cout << "✅ Simulation completed successfully" << std::endl;
                  }
                  current_sim_state.current_simulation_time = target_date;
                  current_sim_state.last_simulated_date = date_param;
                  json << "  \"simulated_date\": \"" << date_param << "\",\n";
                  json << "  \"simulation_mode\": true,\n";
                  json << "  \"data_source\": \"SIMULATION\",\n";
                } else {
                  if (verbose) {
                    std::cout << "❌ Simulation failed" << std::endl;
                  }
                  json << "  \"error\": \"Simulation failed or timed out\",\n";
                  json << "  \"simulation_mode\": false,\n";
                }
              }
            }
          } catch (const std::exception& e) {
            std::cout << "❌ Exception during simulation: " << e.what() << std::endl;
            json << "  \"error\": \"Simulation failed: " << e.what() << "\",\n";
            json << "  \"simulation_mode\": false,\n";
          } catch (...) {
            std::cout << "❌ Unknown exception during simulation" << std::endl;
            json << "  \"error\": \"Unknown simulation error\",\n";
            json << "  \"simulation_mode\": false,\n";
          }
          std::cout << "🔓 Releasing simulation lock..." << std::endl;
        }
      }
      std::cout << "✅ Simulation section completed" << std::endl;
    } else {
      std::cout << "❌ Failed to parse date: " << date_param << std::endl;
      json << "  \"error\": \"Invalid date format\",\n";
      json << "  \"simulation_mode\": false,\n";
    }
  } else {
    json << "  \"simulation_mode\": false,\n";
  }

  std::cout << "📊 Generating bodies data..." << std::endl;
  json << "  \"bodies\": [\n";

  // Thread-safe body data access with detailed logging
  {
    std::lock_guard<std::mutex> lock(simulation_mutex);
    int body_count = get_body_count();
    std::cout << "🌍 Total bodies available: " << body_count << std::endl;

    for (int i = 0; i < body_count; i++) {
      const planet& body = get_body(i);

      // Log first few bodies for debugging
      if (i < 3) {
        std::cout << "  🪐 Body " << i << " (" << body.name << "): pos=(" << body.position.x << ", "
                  << body.position.y << ", " << body.position.z << ")" << std::endl;
      }

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

      if (i < body_count - 1) {
        json << ",";
      }
      json << "\n";
    }
  }

  json << "  ]\n";
  json << "}\n";

  std::cout << "✅ JSON generation completed" << std::endl;
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
    const struct tm* tm_now = localtime(&now);
    const struct tm* tm_epoch = localtime(&epoch);
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

  if (request.path == "/api/solar_system" ||
      (request.path.length() >= 18 && request.path.substr(0, 18) == "/api/solar_system?")) {
    response.headers["Content-Type"] = "application/json";

    // Extract date parameter if present
    std::string date_param = "";
    bool is_manual_request = false;  // Flag for manual time travel requests
    size_t date_pos = request.path.find("date=");
    if (date_pos != std::string::npos) {
      size_t start = date_pos + 5;  // Skip "date="
      size_t end = request.path.find("&", start);
      if (end == std::string::npos) end = request.path.length();
      date_param = request.path.substr(start, end - start);
      if (config.verbose) {
        std::cout << "📅 Processing simulation request for date: " << date_param << std::endl;
      }

      // Check for manual request flag
      if (request.path.find("manual=true") != std::string::npos) {
        is_manual_request = true;
        if (config.verbose) {
          std::cout << "🎯 Manual time travel request detected - will use JPL data" << std::endl;
        }
      }
    } else {
      if (config.verbose) {
        std::cout << "📊 Processing current solar system data request" << std::endl;
      }
    }

    try {
      if (config.verbose) {
        std::cout << "🔄 Starting JSON generation..." << std::endl;
      }
      response.body = generate_solar_system_json(date_param, is_manual_request, config.verbose);
      if (config.verbose) {
        std::cout << "✅ Successfully generated response for date: "
                  << (date_param.empty() ? "current" : date_param) << std::endl;
      }
    } catch (const std::exception& e) {
      std::cout << "❌ Error generating response: " << e.what() << std::endl;
      response.status_code = 500;
      response.status_text = "Internal Server Error";
      response.body = "{\"error\": \"Server error during simulation\"}";
    } catch (...) {
      std::cout << "❌ Unknown error generating response" << std::endl;
      response.status_code = 500;
      response.status_text = "Internal Server Error";
      response.body = "{\"error\": \"Unknown server error\"}";
    }

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
  // Add CSP headers for all responses to fix JavaScript execution issues
  response.headers["Content-Security-Policy"] =
      "default-src 'self' 'unsafe-inline' 'unsafe-eval'; script-src 'self' 'unsafe-inline' "
      "'unsafe-eval'; style-src 'self' 'unsafe-inline';";
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

      // Add detailed logging for API requests
      if (request.path.length() >= 17 && request.path.substr(0, 17) == "/api/solar_system") {
        std::cout << "🔍 API REQUEST DETAILS:" << std::endl;
        std::cout << "  📡 Full path: " << request.path << std::endl;
        std::cout << "  🕐 Server time: " << time(NULL) << std::endl;
      }
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
  std::cout << "  -w, --web-root DIR     Web root directory (default: auto-detect "
               "../share/solar_system/web)\n";
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

  // Save initial system state for simulation requests
  {
    std::lock_guard<std::mutex> lock(simulation_mutex);
    save_system_state();
  }

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

  if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) <
      0) {
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
  if (config.verbose) {
    std::cout << "🔄 CORS: " << (config.enable_cors ? "enabled" : "disabled") << "\n";
    std::cout << "📊 Verbose logging: enabled\n";
  }
  std::cout << "🛑 Press Ctrl+C to stop\n\n";

  // Set socket to non-blocking mode for better signal handling
  int flags = fcntl(server_socket, F_GETFL, 0);
  fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

  // Main server loop
  while (server_running) {
    // Use select() to wait for connections with timeout
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 1;  // 1 second timeout
    timeout.tv_usec = 0;

    int select_result = select(server_socket + 1, &read_fds, nullptr, nullptr, &timeout);

    if (select_result < 0) {
      if (errno == EINTR) {
        // Interrupted by signal, check if we should continue
        continue;
      }
      if (server_running) {
        std::cerr << "Select error: " << strerror(errno) << "\n";
      }
      break;
    }

    if (select_result == 0) {
      // Timeout, continue loop to check server_running
      continue;
    }

    // Accept connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket =
        accept(server_socket, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (client_socket < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        // Interrupted by signal or would block, continue
        continue;
      }
      if (server_running) {
        std::cerr << "Failed to accept client connection: " << strerror(errno) << "\n";
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
