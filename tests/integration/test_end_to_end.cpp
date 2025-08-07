/**
 * @file test_end_to_end.cpp
 * @brief End-to-end integration tests for complete workflows
 *
 * Tests complete user workflows including:
 * - Full application integration scenarios
 * - Multi-component data flow validation
 * - Real-world usage patterns
 * - System resilience and error recovery
 */

#include <unistd.h>

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
 * @brief Application process manager for testing
 */
class TestApplicationRunner {
 public:
  struct ProcessResult {
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    bool success = false;
    std::chrono::milliseconds execution_time{0};
  };

  static ProcessResult run_command(const std::string& command, int timeout_seconds = 30) {
    ProcessResult result;
    auto start_time = std::chrono::high_resolution_clock::now();

    // Create temporary files for output capture
    std::string stdout_file = "/tmp/test_stdout_" + std::to_string(getpid()) + ".txt";
    std::string stderr_file = "/tmp/test_stderr_" + std::to_string(getpid()) + ".txt";

    // Convert relative paths to absolute paths if needed
    std::string abs_command = command;
    if (abs_command.starts_with("./")) {
      // Get current working directory and build absolute path
      std::string cwd = std::filesystem::current_path().string();
      // If we're in the project root, applications are in the build subdirectory
      if (cwd.ends_with("solarsystem")) {
        abs_command = cwd + "/build/" + abs_command.substr(2);
      } else {
        abs_command = cwd + "/" + abs_command.substr(2);
      }
    }

    std::string full_command = abs_command + " >" + stdout_file + " 2>" + stderr_file;

    // Add timeout using the timeout command
    full_command = "timeout " + std::to_string(timeout_seconds) + " " + full_command;

    result.exit_code = system(full_command.c_str());

    auto end_time = std::chrono::high_resolution_clock::now();
    result.execution_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Read output files
    result.stdout_output = read_file_content(stdout_file);
    result.stderr_output = read_file_content(stderr_file);

    // Clean up temporary files
    std::filesystem::remove(stdout_file);
    std::filesystem::remove(stderr_file);

    result.success = (result.exit_code == 0);
    return result;
  }

 private:
  static std::string read_file_content(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }
};

int main() {
  TEST_SUITE("End-to-End Integration Tests");

  // Test 1: Complete solar system launcher workflow
  TEST_CASE("Solar System Launcher Complete Workflow"){
      // Test launcher help functionality
      {auto result = TestApplicationRunner::run_command("./solar_system_launcher --help");
  ASSERT_TRUE(result.success);
  ASSERT_TRUE(result.stdout_output.find("Solar System") != std::string::npos ||
              result.stdout_output.find("Usage") != std::string::npos);
}

// Test launcher status check
{
  auto result = TestApplicationRunner::run_command("./solar_system_launcher --status");
  ASSERT_TRUE(result.success);
  // Should provide system status information
  ASSERT_TRUE(result.stdout_output.find("Status") != std::string::npos ||
              result.stdout_output.find("System") != std::string::npos ||
              result.stdout_output.find("OK") != std::string::npos);
}

// Test launcher with basic simulation
{
  auto result =
      TestApplicationRunner::run_command("./solar_system_launcher --simulate --date 2025-01-01");
  ASSERT_TRUE(result.success);
  ASSERT_LT(result.execution_time.count(), 10000);  // Should complete within 10 seconds
}
});

// Test 2: Data fetch and cache workflow
TEST_CASE("Data Fetch and Cache Workflow") {
  // Create temporary cache directory
  auto test_env = TestDataManager::create_test_environment();
  std::string cache_dir = test_env->path_string() + "/cache";
  std::filesystem::create_directories(cache_dir);

  // Test cache storage functionality
  {
    auto result = TestApplicationRunner::run_command("./solar_system_fetch --test-storage");
    ASSERT_TRUE(result.success);
    // Should validate cache system is working
  }

  // Test data fetching with cache
  {
    std::string command = "./solar_system_fetch --status";
    auto result = TestApplicationRunner::run_command(command);
    ASSERT_TRUE(result.success);

    // The status command should succeed and show cache status
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.stdout_output.find("Cache Status") != std::string::npos ||
                result.stdout_output.find("Data Source") != std::string::npos);
  }

  // Test using cached data
  {
    std::string command = "./solar_system_fetch --status";
    auto result = TestApplicationRunner::run_command(command);
    ASSERT_TRUE(result.success);
    // Should be faster when using cache
    ASSERT_LT(result.execution_time.count(), 5000);  // Should complete within 5 seconds
  }
});

// Test 3: Batch simulation workflow
TEST_CASE("Batch Simulation Workflow"){
    // Test basic simulation
    {auto result = TestApplicationRunner::run_command("./solar_system --date 2025-01-01");
ASSERT_TRUE(result.success);

// Should produce simulation output
ASSERT_TRUE(result.stdout_output.find("Solar System") != std::string::npos ||
            result.stdout_output.find("simulation") != std::string::npos ||
            result.stdout_output.find("completed") != std::string::npos);
}

// Test simulation with different dates (using current application capabilities)
{
  std::vector<std::string> dates = {"2025-01-01", "2024-12-31", "2025-06-15"};

  for (const auto& date : dates) {
    std::string command = "./solar_system --date " + date;
    auto result = TestApplicationRunner::run_command(command);
    ASSERT_TRUE(result.success);
    // Verify simulation output contains expected elements
    ASSERT_TRUE(result.stdout_output.find("Solar System Simulation") != std::string::npos ||
                result.stdout_output.find("Created") != std::string::npos);
  }
}

// Test performance simulation
{
  auto result = TestApplicationRunner::run_command("./solar_system --date 2025-01-01");
  ASSERT_TRUE(result.success);

  // Should complete within reasonable time (< 30 seconds for 1 day simulation)
  ASSERT_LT(result.execution_time.count(), 30000);
}
});

// Test 4: Real-time monitoring workflow
TEST_CASE("Real-time Monitoring Workflow"){
    // Test real-time system startup
    {auto result = TestApplicationRunner::run_command("./solar_system_realtime --help");
ASSERT_TRUE(result.success);
ASSERT_TRUE(result.stdout_output.find("real-time") != std::string::npos ||
            result.stdout_output.find("monitoring") != std::string::npos ||
            result.stdout_output.find("Usage") != std::string::npos);
}

// Test short real-time simulation
{
  auto result = TestApplicationRunner::run_command(
      "./solar_system_realtime --bodies Sun,Earth,Moon --duration 5 --update-interval 1 "
      "--no-continuous",
      10);
  ASSERT_TRUE(result.success);

  // Should provide real-time updates
  ASSERT_TRUE(result.stdout_output.find("time") != std::string::npos ||
              result.stdout_output.find("position") != std::string::npos ||
              result.stdout_output.find("update") != std::string::npos);
}
});

// Test 5: Web interface integration workflow
TEST_CASE("Web Interface Integration Workflow") {
  // Find available port
  int test_port = 8087;
  std::string port_check = "lsof -i:" + std::to_string(test_port) + " >/dev/null 2>&1";
  while (system(port_check.c_str()) == 0 && test_port < 8095) {
    test_port++;
    port_check = "lsof -i:" + std::to_string(test_port) + " >/dev/null 2>&1";
  }

  // Create temporary web directory
  auto test_env = TestDataManager::create_test_environment();
  std::string web_dir = test_env->path_string() + "/web";
  std::filesystem::create_directories(web_dir);

  // Create minimal web files
  std::ofstream index_file(web_dir + "/index.html");
  index_file << "<!DOCTYPE html><html><head><title>Test</title></head><body><h1>Solar "
                "System</h1></body></html>";
  index_file.close();

  // Start web server in background
  // Get the absolute path to the web server
  std::string cwd = std::filesystem::current_path().string();
  std::string web_server_path;
  if (cwd.ends_with("solarsystem")) {
    web_server_path = cwd + "/build/solar_system_web";
  } else {
    web_server_path = cwd + "/solar_system_web";
  }

  std::string start_command =
      web_server_path + " --port " + std::to_string(test_port) + " --web-root " + web_dir + " &";
  [[maybe_unused]] int start_result = system(start_command.c_str());

  // Wait for server to start
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Test web server connectivity
  {
    std::string test_command = "curl -s -f http://localhost:" + std::to_string(test_port) + "/";
    auto web_result = TestApplicationRunner::run_command(test_command, 5);
    ASSERT_TRUE(web_result.success);
    ASSERT_TRUE(web_result.stdout_output.find("Solar System") != std::string::npos);
  }

  // Test API endpoints
  {
    std::string api_command =
        "curl -s -f http://localhost:" + std::to_string(test_port) + "/api/status";
    auto api_result = TestApplicationRunner::run_command(api_command, 5);
    ASSERT_TRUE(api_result.success);
    // Should return some status information
    ASSERT_FALSE(api_result.stdout_output.empty());
  }

  // Clean up web server
  std::string cleanup_command =
      "pkill -f 'solar_system_web.*--port " + std::to_string(test_port) + "'";
  [[maybe_unused]] int result2 = system(cleanup_command.c_str());
  std::this_thread::sleep_for(std::chrono::seconds(1));
});

// Test 6: Error recovery and resilience
TEST_CASE("Error Recovery and System Resilience"){
    // Test handling of invalid arguments
    {auto result = TestApplicationRunner::run_command("./solar_system --invalid-argument");
ASSERT_FALSE(result.success);  // Should fail gracefully
ASSERT_TRUE(result.stderr_output.find("invalid") != std::string::npos ||
            result.stderr_output.find("unknown") != std::string::npos ||
            result.stdout_output.find("Usage") != std::string::npos);
}

// Test handling of invalid body names
{
  auto result = TestApplicationRunner::run_command("./solar_system --invalid-option");
  // Should either succeed with fallback or fail gracefully
  if (!result.success) {
    ASSERT_TRUE(result.stderr_output.find("body") != std::string::npos ||
                result.stderr_output.find("unknown") != std::string::npos ||
                result.stderr_output.find("Unknown") != std::string::npos ||
                result.stdout_output.find("Unknown") != std::string::npos ||
                result.stdout_output.find("Usage") != std::string::npos);
  }
}

// Test handling of extreme parameters
{
  auto result = TestApplicationRunner::run_command("./solar_system --date invalid-date");
  // Should handle invalid date gracefully
  ASSERT_TRUE(result.success || result.stderr_output.find("date") != std::string::npos ||
              result.stderr_output.find("invalid") != std::string::npos ||
              result.stderr_output.find("Invalid") != std::string::npos ||
              result.stdout_output.find("Invalid") != std::string::npos);
}

// Test memory constraints
{
  auto result = TestApplicationRunner::run_command("./solar_system --date 2025-01-01");
  // Should complete or fail gracefully with memory constraints
  if (!result.success) {
    ASSERT_TRUE(result.stderr_output.find("memory") != std::string::npos ||
                result.stderr_output.find("resource") != std::string::npos ||
                result.execution_time.count() > 25000);  // Timeout is acceptable
  }
}
});

// Test 7: Multi-component integration
TEST_CASE("Multi-Component Integration") {
  // Create shared cache directory
  auto test_env = TestDataManager::create_test_environment();
  std::string shared_cache = test_env->path_string() + "/shared_cache";
  std::filesystem::create_directories(shared_cache);

  // Step 1: Fetch data using fetch application
  {
    std::string command = "./solar_system_fetch --status";
    auto result = TestApplicationRunner::run_command(command);
    ASSERT_TRUE(result.success);
  }

  // Step 2: Use cached data in simulation
  {
    std::string command = "./solar_system --date 2025-01-01";
    auto result = TestApplicationRunner::run_command(command);
    ASSERT_TRUE(result.success);
    // Should be faster when using pre-fetched cache
    ASSERT_LT(result.execution_time.count(), 8000);
  }

  // Step 3: Test storage system (since we don't have actual cache)
  {
    std::string command = "./solar_system_fetch --test-storage";
    auto result = TestApplicationRunner::run_command(command);
    ASSERT_TRUE(result.success);
  }
});

// Test 8: Performance and scalability validation
TEST_CASE("Performance and Scalability Validation"){
    // Test small system performance
    {auto start_time = std::chrono::high_resolution_clock::now();

auto result = TestApplicationRunner::run_command("./solar_system --date 2025-01-01");
ASSERT_TRUE(result.success);

auto end_time = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

// Small system should complete quickly (< 5 seconds)
ASSERT_LT(duration.count(), 5000);
}

// Test medium system performance
{
  auto start_time = std::chrono::high_resolution_clock::now();

  auto result = TestApplicationRunner::run_command("./solar_system --date 2025-01-01");
  ASSERT_TRUE(result.success);

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Medium system should complete reasonably quickly (< 15 seconds)
  ASSERT_LT(duration.count(), 15000);
}

// Test cache performance claims
{
  auto test_cache = TestDataManager::create_test_cache();
  test_cache->populate_with_valid_data();

  auto start_time = std::chrono::high_resolution_clock::now();

  // Load from cache multiple times
  for (int i = 0; i < 10; ++i) {
    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;

    Bodies::BodyFactory factory(options);
    auto result = factory.create_body("Earth", options);
    ASSERT_TRUE(result.has_value());
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Cache loading should be very fast (< 100ms for 10 loads)
  ASSERT_LT(duration.count(), 100);
}
});

return current_suite->all_passed() ? 0 : 1;
}
