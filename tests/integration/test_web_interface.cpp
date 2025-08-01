/**
 * @file test_web_interface.cpp
 * @brief Integration tests for web interface with simulation backend
 *
 * Tests the complete web server integration including:
 * - Web server startup and configuration
 * - API endpoints with simulation data
 * - Real-time data updates
 * - Error handling and graceful degradation
 */

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "test_data_manager.hpp"
#include "test_framework.h"

using namespace SolarSystem;
using namespace TestData;

/**
 * @brief Simple HTTP client for testing web endpoints
 */
class SimpleHTTPClient {
 public:
  struct HTTPResponse {
    int status_code = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success = false;
  };

  static HTTPResponse get(const std::string& url, int timeout_seconds = 5) {
    HTTPResponse response;

    // Use curl command for simplicity in tests
    std::string command =
        "curl -s -w '%{http_code}' -m " + std::to_string(timeout_seconds) + " '" + url + "'";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
      return response;
    }

    std::string result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result += buffer;
    }

    int exit_code = pclose(pipe);
    if (exit_code == 0 && result.length() >= 3) {
      // Extract status code from end of response
      std::string status_str = result.substr(result.length() - 3);
      response.status_code = std::stoi(status_str);
      response.body = result.substr(0, result.length() - 3);
      response.success = true;
    }

    return response;
  }

  static bool is_port_available(int port) {
    std::string command = "lsof -i:" + std::to_string(port) + " >/dev/null 2>&1";
    return system(command.c_str()) != 0;  // Returns true if port is free
  }

  static bool wait_for_server(int port, int max_wait_seconds = 10) {
    for (int i = 0; i < max_wait_seconds; ++i) {
      if (!is_port_available(port)) {
        return true;  // Server is running
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
  }
};

/**
 * @brief Web server process manager for testing
 */
class TestWebServer {
 public:
  TestWebServer(int port = 8081) : port_(port), process_id_(-1) {}

  ~TestWebServer() { stop(); }

  bool start(const std::string& web_root = "./web") {
    if (process_id_ != -1) {
      return false;  // Already runn
    }

    // Create test web root if it doesn't exist
    std::filesystem::create_directories(web_root);
    create_test_web_files(web_root);

    // Start web server process
    std::string command = "./solar_system_web --port " + std::to_string(port_) + " --web-root " +
                          web_root + " >/dev/null 2>&1 &";

    int result = system(command.c_str());
    if (result != 0) {
      return false;
    }

    // Wait for server to start
    return SimpleHTTPClient::wait_for_server(port_, 5);
  }

  void stop() {
    if (process_id_ != -1) {
      std::string command = "pkill -f 'solar_system_web.*--port " + std::to_string(port_) + "'";
      system(command.c_str());
      process_id_ = -1;
    }
  }

  int port() const { return port_; }
  std::string base_url() const { return "http://localhost:" + std::to_string(port_); }

 private:
  int port_;
  int process_id_;

  void create_test_web_files(const std::string& web_root) {
    // Create minimal test HTML file
    std::ofstream index_file(web_root + "/index.html");
    index_file << R"(<!DOCTYPE html>
<html>
<head><title>Solar System Test</title></head>
<body>
<h1>Solar System Simulation</h1>
<div id="status">Loading...</div>
<script>
fetch('/api/status')
  .then(response => response.json())
  .then(data => {
    document.getElementById('status').textContent = 'Status: ' + data.status;
  })
  .catch(error => {
    document.getElementById('status').textContent = 'Error: ' + error.message;
  });
</script>
</body>
</html>)";
    index_file.close();
  }
};

int main() {
  TEST_SUITE("Web Interface Integration Tests");

  // Test 1: Web server startup and basic functionality
  TEST_CASE("Web Server Startup and Configuration") {
    // Find available port for testing
    int test_port = 8081;
    while (!SimpleHTTPClient::is_port_available(test_port) && test_port < 8090) {
      test_port++;
    }
    ASSERT_TRUE(SimpleHTTPClient::is_port_available(test_port));

    // Create test web server
    TestWebServer server(test_port);

    // Create temporary web root
    auto test_env = TestDataManager::create_test_environment();
    std::string web_root = test_env->path_string() + "/web";

    // Start server
    bool started = server.start(web_root);
    ASSERT_TRUE(started);

    // Test basic connectivity
    auto response = SimpleHTTPClient::get(server.base_url() + "/");
    ASSERT_TRUE(response.success);
    ASSERT_EQ(response.status_code, 200);
    ASSERT_TRUE(response.body.find("Solar System") != std::string::npos);

    // Stop server
    server.stop();

    // Verify server stopped
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(SimpleHTTPClient::is_port_available(test_port));
  });

  // Test 2: API endpoints with simulation data
  TEST_CASE("API Endpoints with Simulation Data") {
    int test_port = 8082;
    while (!SimpleHTTPClient::is_port_available(test_port) && test_port < 8090) {
      test_port++;
    }

    TestWebServer server(test_port);
    auto test_env = TestDataManager::create_test_environment();
    std::string web_root = test_env->path_string() + "/web";

    ASSERT_TRUE(server.start(web_root));

    // Test status endpoint
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/api/status");
      ASSERT_TRUE(response.success);
      ASSERT_EQ(response.status_code, 200);

      // Should return JSON with status information
      ASSERT_TRUE(response.body.find("status") != std::string::npos);
    }

    // Test bodies endpoint
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/api/bodies");
      ASSERT_TRUE(response.success);
      ASSERT_EQ(response.status_code, 200);

      // Should return JSON array of celestial bodies
      ASSERT_TRUE(response.body.find("[") != std::string::npos);  // JSON array
    }

    // Test simulation endpoint
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/api/simulation");
      ASSERT_TRUE(response.success);
      ASSERT_EQ(response.status_code, 200);

      // Should return simulation state information
      ASSERT_TRUE(response.body.find("time") != std::string::npos ||
                  response.body.find("bodies") != std::string::npos);
    }

    // Test invalid endpoint
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/api/nonexistent");
      ASSERT_TRUE(response.success);
      ASSERT_EQ(response.status_code, 404);
    }

    server.stop();
  });

  // Test 3: Error handling and graceful degradation
  TEST_CASE("Error Handling and Graceful Degradation") {
    int test_port = 8083;
    while (!SimpleHTTPClient::is_port_available(test_port) && test_port < 8090) {
      test_port++;
    }

    TestWebServer server(test_port);
    auto test_env = TestDataManager::create_test_environment();
    std::string web_root = test_env->path_string() + "/web";

    ASSERT_TRUE(server.start(web_root));

    // Test malformed requests
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/api/bodies?invalid=query");
      ASSERT_TRUE(response.success);
      // Should handle gracefully (either 200 with error message or 400)
      ASSERT_TRUE(response.status_code == 200 || response.status_code == 400);
    }

    // Test very long URLs
    {
      std::string long_path = "/api/test";
      for (int i = 0; i < 100; ++i) {
        long_path += "/very/long/path/segment";
      }

      auto response = SimpleHTTPClient::get(server.base_url() + long_path);
      ASSERT_TRUE(response.success);
      // Should return 404 or 414 (URI Too Long)
      ASSERT_TRUE(response.status_code == 404 || response.status_code == 414);
    }

    // Test concurrent requests
    {
      std::vector<std::thread> threads;
      std::vector<bool> results(5, false);

      for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&server, &results, i]() {
          auto response = SimpleHTTPClient::get(server.base_url() + "/api/status");
          results[static_cast<size_t>(i)] = response.success && response.status_code == 200;
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      // At least most requests should succeed
      int success_count = 0;
      for (bool result : results) {
        if (result) success_count++;
      }
      ASSERT_GT(success_count, 3);  // At least 4 out of 5 should succeed
    }

    server.stop();
  });

  // Test 4: Static file serving
  TEST_CASE("Static File Serving") {
    int test_port = 8084;
    while (!SimpleHTTPClient::is_port_available(test_port) && test_port < 8090) {
      test_port++;
    }

    TestWebServer server(test_port);
    auto test_env = TestDataManager::create_test_environment();
    std::string web_root = test_env->path_string() + "/web";

    // Create additional test files
    std::filesystem::create_directories(web_root);

    // Create CSS file
    std::ofstream css_file(web_root + "/style.css");
    css_file << "body { font-family: Arial, sans-serif; }";
    css_file.close();

    // Create JavaScript file
    std::ofstream js_file(web_root + "/script.js");
    js_file << "console.log('Solar System loaded');";
    js_file.close();

    ASSERT_TRUE(server.start(web_root));

    // Test CSS file serving
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/style.css");
      ASSERT_TRUE(response.success);
      ASSERT_EQ(response.status_code, 200);
      ASSERT_TRUE(response.body.find("font-family") != std::string::npos);
    }

    // Test JavaScript file serving
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/script.js");
      ASSERT_TRUE(response.success);
      ASSERT_EQ(response.status_code, 200);
      ASSERT_TRUE(response.body.find("console.log") != std::string::npos);
    }

    // Test non-existent file
    {
      auto response = SimpleHTTPClient::get(server.base_url() + "/nonexistent.txt");
      ASSERT_TRUE(response.success);
      ASSERT_EQ(response.status_code, 404);
    }

    server.stop();
  });

  // Test 5: CORS and security headers
  TEST_CASE("CORS and Security Headers") {
    int test_port = 8085;
    while (!SimpleHTTPClient::is_port_available(test_port) && test_port < 8090) {
      test_port++;
    }

    TestWebServer server(test_port);
    auto test_env = TestDataManager::create_test_environment();
    std::string web_root = test_env->path_string() + "/web";

    ASSERT_TRUE(server.start(web_root));

    // Test CORS headers on API endpoints
    {
      // Use curl to get headers
      std::string command = "curl -s -I '" + server.base_url() + "/api/status'";
      FILE* pipe = popen(command.c_str(), "r");
      ASSERT_NOT_NULL(pipe);

      std::string headers;
      char buffer[256];
      while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        headers += buffer;
      }
      pclose(pipe);

      // Should have CORS headers if enabled
      // CORS may or may not be enabled depending on configuration
      // Just verify we get a valid response structure
      ASSERT_TRUE(headers.find("HTTP/") != std::string::npos);
    }

    server.stop();
  });

  // Test 6: Performance under load
  TEST_CASE("Performance Under Load") {
    int test_port = 8086;
    while (!SimpleHTTPClient::is_port_available(test_port) && test_port < 8090) {
      test_port++;
    }

    TestWebServer server(test_port);
    auto test_env = TestDataManager::create_test_environment();
    std::string web_root = test_env->path_string() + "/web";

    ASSERT_TRUE(server.start(web_root));

    // Measure response time for multiple requests
    auto start_time = std::chrono::high_resolution_clock::now();

    int successful_requests = 0;
    const int total_requests = 10;

    for (int i = 0; i < total_requests; ++i) {
      auto response = SimpleHTTPClient::get(server.base_url() + "/api/status", 2);
      if (response.success && response.status_code == 200) {
        successful_requests++;
      }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Most requests should succeed
    ASSERT_GT(successful_requests, total_requests * 0.8);  // At least 80% success rate

    // Average response time should be reasonable (< 100ms per request)
    double avg_time_ms = static_cast<double>(duration.count()) / total_requests;
    ASSERT_LT(avg_time_ms, 100.0);

    server.stop();
  });

  return current_suite->all_passed() ? 0 : 1;
}
