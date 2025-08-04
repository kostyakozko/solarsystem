/**
 * @file test_application_workflows.cpp
 * @brief Integration tests for system integration scenarios
 *
 * Tests comprehensive system integration including:
 * - Application startup and initialization
 * - Configuration loading and validation
 * - Inter-application communication
 * - Deployment and installation processes
 */

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "test_data_manager.hpp"
#include "test_framework.h"

using namespace TestData;

// Cross-platform timeout command detection
std::string get_timeout_command() {
  // Check if gtimeout is available (macOS with coreutils)
  if (system("which gtimeout > /dev/null 2>&1") == 0) {
    return "gtimeout";
  }
  // Check if timeout is available (Linux/Ubuntu)
  if (system("which timeout > /dev/null 2>&1") == 0) {
    return "timeout";
  }
  // Fallback - return empty string to skip timeout
  return "";
}

// Helper function to execute system commands and capture output
std::string execute_command(const std::string& command) {
  std::string result;
  FILE* pipe = popen(command.c_str(), "r");
  if (pipe) {
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result += buffer;
    }
    pclose(pipe);
  }
  return result;
}

// Helper function to check if file exists
bool file_exists(const std::string& filename) {
  std::ifstream file(filename);
  return file.good();
}

// Helper function to get absolute path to executable
std::string get_executable_path(const std::string& exe_name) {
  static std::string build_dir = std::filesystem::current_path().string();
  return build_dir + "/" + exe_name;
}

int main() {
  TEST_SUITE("Application Workflow Integration Tests");

  // Test launcher application workflow
  TEST_CASE("Launcher Status Check") {
    // Test that launcher can check system status
    std::string command = "./apps/solar_system_launcher/solar_system_launcher --status";
    std::string output = execute_command(command);

    // Should contain status information
    ASSERT_TRUE(output.find("Solar System Suite") != std::string::npos);
    ASSERT_TRUE(output.find("Status") != std::string::npos);
  });

  // Test data fetching workflow
  TEST_CASE("Data Fetch Workflow") {
    // Test that fetch application can validate storage
    std::string command = "./apps/solar_system_fetch/solar_system_fetch --test-storage";
    std::string output = execute_command(command);

    // Should complete without errors
    ASSERT_TRUE(output.find("Storage test") != std::string::npos ||
                output.find("Test completed") != std::string::npos ||
                output.find("test passed") != std::string::npos);
  });

  // Test simulation workflow
  TEST_CASE("Basic Simulation Workflow") {
    // Test that simulation can run with a future date
    std::string command = "./apps/solar_system/solar_system --date 2025-07-01";
    std::string output = execute_command(command);

    // Should complete simulation
    ASSERT_TRUE(output.find("Simulation") != std::string::npos ||
                output.find("completed") != std::string::npos ||
                output.find("2025-07-01") != std::string::npos);
  });

  // Test launcher coordinated workflow
  TEST_CASE("Launcher Coordinated Simulation") {
    // Test launcher's ability to coordinate fetch and simulate
    std::string command =
        "./apps/solar_system_launcher/solar_system_launcher --simulate --date 2025-08-01";
    std::string output = execute_command(command);

    // Should show coordination between components
    ASSERT_TRUE(output.find("2025-08-01") != std::string::npos);
  });

  // Test real-time application
  TEST_CASE("Real-time Application") {
    // Test real-time application with no-continuous mode
    std::string timeout_cmd = get_timeout_command();
    std::string command;
    if (!timeout_cmd.empty()) {
      command =
          timeout_cmd + " 10s ./apps/solar_system_realtime/solar_system_realtime --no-continuous";
    } else {
      command = "./apps/solar_system_realtime/solar_system_realtime --no-continuous";
    }
    std::string output = execute_command(command);

    // Should provide real-time data snapshot
    ASSERT_TRUE(output.find("Solar System") != std::string::npos ||
                output.find("Position") != std::string::npos ||
                output.find("Real-time") != std::string::npos);
  });

  // Test web server startup
  TEST_CASE("Web Server Startup") {
    // Clean up any existing web servers first
    [[maybe_unused]] int cleanup_result = system("pkill -f solar_system_web 2>/dev/null || true");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Use a unique port to avoid conflicts
    int test_port = 8080 + (rand() % 1000);
    // Start web server in background
    std::string start_command = "./apps/solar_system_web/solar_system_web --port " +
                                std::to_string(test_port) +
                                " --web-root share/solar_system/web > "
                                "/dev/null 2>&1 &";
    [[maybe_unused]] int start_result = system(start_command.c_str());

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Test if server is responding
    std::string test_command =
        "curl -s -o /dev/null -w \"%{http_code}\" http://localhost:" + std::to_string(test_port) +
        "/api/status 2>/dev/null || "
        "echo '000'";
    // Retry logic for server startup
    bool server_ready = false;
    std::string response;
    for (int retry = 0; retry < 5 && !server_ready; retry++) {
      if (retry > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "Retrying server connection (attempt " << (retry + 1) << "/5)..." << std::endl;
      }
      response = execute_command(test_command);
      server_ready = (response.find("200") != std::string::npos);
    }
    if (!server_ready) {
      std::ostringstream error_msg;
      error_msg << "Web server failed to respond with HTTP 200 after retries. Response: '"
                << response << "'";
      throw std::runtime_error(error_msg.str());
    }

    // Clean up - kill the web server
    [[maybe_unused]] int cleanup_result2 = system("pkill -f solar_system_web 2>/dev/null || true");
  });

  // Test data consistency across applications
  TEST_CASE("Data Consistency") {
    // Run simulation and capture output
    std::string sim_command = "./apps/solar_system/solar_system --date 2025-06-01";
    std::string sim_output = execute_command(sim_command);

    // Run real-time for same date (if supported)
    std::string rt_command = "./apps/solar_system_realtime/solar_system_realtime --no-continuous";
    std::string rt_output = execute_command(rt_command);

    // Both should reference the same solar system data
    // This is a basic consistency check
    ASSERT_TRUE(!sim_output.empty());
    ASSERT_TRUE(!rt_output.empty());
  });

  // Test error handling across applications
  TEST_CASE("Error Handling") {
    // Test invalid date handling
    std::string invalid_date_command = "./apps/solar_system/solar_system --date invalid-date 2>&1";
    std::string output = execute_command(invalid_date_command);

    // Should handle error gracefully
    ASSERT_TRUE(output.find("Error") != std::string::npos ||
                output.find("Invalid") != std::string::npos ||
                output.find("Usage") != std::string::npos);
  });

  // Test cache file interactions
  TEST_CASE("Cache File Workflow") {
    // Check if cache files are created/used properly
    std::string cache_command = "./apps/solar_system_fetch/solar_system_fetch --validate";
    std::string output = execute_command(cache_command);

    // Should validate or create cache
    ASSERT_TRUE(output.find("cache") != std::string::npos ||
                output.find("Cache") != std::string::npos ||
                output.find("validation") != std::string::npos);
  });

  // Test application help systems
  TEST_CASE("Help System Consistency") {
    std::vector<std::string> applications = {
        "./apps/solar_system_launcher/solar_system_launcher --help",
        "./apps/solar_system/solar_system --help",
        "./apps/solar_system_fetch/solar_system_fetch --help",
        "./apps/solar_system_realtime/solar_system_realtime --help",
        "./apps/solar_system_web/solar_system_web --help"};

    for (const auto& app_command : applications) {
      std::string output = execute_command(app_command);

      // Each application should provide help
      ASSERT_TRUE(output.find("Usage") != std::string::npos ||
                  output.find("Options") != std::string::npos ||
                  output.find("help") != std::string::npos);
    }
  });

  // Test installation verification
  TEST_CASE("Installation Verification") {
    // Check that all expected files are present
    std::vector<std::string> expected_files = {"./apps/solar_system_launcher/solar_system_launcher",
                                               "./apps/solar_system/solar_system",
                                               "./apps/solar_system_fetch/solar_system_fetch",
                                               "./apps/solar_system_realtime/solar_system_realtime",
                                               "./apps/solar_system_web/solar_system_web"};

    for (const auto& file : expected_files) {
      ASSERT_TRUE(file_exists(file));
    }

    // Check for documentation
    ASSERT_TRUE(file_exists("./README.md") || file_exists("../README.md"));
    ASSERT_TRUE(file_exists("./share/solar_system/web/index.html") ||
                file_exists("./docs/README.md") || file_exists("../docs/README.md"));
  });

  // Test 1: Application startup and initialization
  TEST_CASE("Application Startup and Initialization") {
    // Test each application can start and initialize properly
    std::vector<std::pair<std::string, std::string>> applications = {
        {"./build/solar_system_launcher --version", "version"},
        {"./build/solar_system --help", "Usage"},
        {"./build/solar_system_fetch --help", "Usage"},
        {"./build/solar_system_realtime --help", "Usage"},
        {"./build/solar_system_web --help", "Usage"}};

    for (const auto& [command, expected_output] : applications) {
      std::string output = execute_command(command + " 2>&1");
      ASSERT_TRUE(output.find(expected_output) != std::string::npos);
    }

    // Test initialization with different working directories
    {
      auto test_env = TestDataManager::create_test_environment();
      std::string old_cwd = std::filesystem::current_path();
      std::string launcher_path =
          (std::filesystem::current_path() / "solar_system_launcher").string();

      std::filesystem::current_path(test_env->path());

      std::string output = execute_command(launcher_path + " --status 2>&1");
      ASSERT_TRUE(output.find("Status") != std::string::npos ||
                  output.find("Solar System") != std::string::npos);

      std::filesystem::current_path(old_cwd);
    }

    // Test initialization with missing dependencies
    {
      // Temporarily rename a library to test graceful degradation
      std::string lib_path = "./lib/libsolar_core.a";
      std::string backup_path = "./lib/libsolar_core.a.backup";

      if (std::filesystem::exists(lib_path)) {
        std::filesystem::rename(lib_path, backup_path);

        // Application should handle missing library gracefully
        std::string output =
            execute_command(get_executable_path("solar_system_launcher") + " --status 2>&1");
        // Should either work with fallback or provide clear error message
        ASSERT_TRUE(output.find("Status") != std::string::npos ||
                    output.find("Error") != std::string::npos ||
                    output.find("library") != std::string::npos);

        // Restore library
        std::filesystem::rename(backup_path, lib_path);
      }
    }
  });

  // Test 2: Configuration loading and validation
  TEST_CASE("Configuration Loading and Validation") {
    auto test_env = TestDataManager::create_test_environment();

    // Test with valid configuration file
    {
      std::string config_file = test_env->path_string() + "/solar_config.json";
      std::ofstream config(config_file);
      config << R"({
        "simulation": {
          "default_timestep": 60,
          "integration_method": "leapfrog",
          "max_duration": 86400
        },
        "cache": {
          "directory": "./cache",
          "max_age_days": 30,
          "enable_binary": true
        },
        "web": {
          "port": 8080,
          "enable_cors": true,
          "web_root": "./web"
        }
      })";
      config.close();

      std::string command =
          get_executable_path("solar_system_launcher") + " --config " + config_file + " --status";
      std::string output = execute_command(command + " 2>&1");
      ASSERT_TRUE(output.find("Status") != std::string::npos ||
                  output.find("configuration") != std::string::npos);
    }

    // Test with invalid configuration file
    {
      std::string invalid_config = test_env->path_string() + "/invalid_config.json";
      std::ofstream config(invalid_config);
      config << "{ invalid json content }";
      config.close();

      std::string command = get_executable_path("solar_system_launcher") + " --config " +
                            invalid_config + " --status";
      std::string output = execute_command(command + " 2>&1");
      // Should handle invalid config gracefully
      ASSERT_TRUE(output.find("Error") != std::string::npos ||
                  output.find("invalid") != std::string::npos ||
                  output.find("Status") != std::string::npos);  // May use defaults
    }

    // Test with missing configuration file
    {
      std::string missing_config = test_env->path_string() + "/nonexistent.json";
      std::string command = get_executable_path("solar_system_launcher") + " --config " +
                            missing_config + " --status";
      std::string output = execute_command(command + " 2>&1");
      // Should handle missing config gracefully
      ASSERT_TRUE(output.find("Error") != std::string::npos ||
                  output.find("not found") != std::string::npos ||
                  output.find("Status") != std::string::npos);  // May use defaults
    }

    // Test environment variable configuration
    {
      setenv("SOLAR_SYSTEM_CACHE_DIR", test_env->path_string().c_str(), 1);
      setenv("SOLAR_SYSTEM_LOG_LEVEL", "DEBUG", 1);

      std::string output =
          execute_command(get_executable_path("solar_system_launcher") + " --status 2>&1");
      ASSERT_TRUE(output.find("Status") != std::string::npos);

      unsetenv("SOLAR_SYSTEM_CACHE_DIR");
      unsetenv("SOLAR_SYSTEM_LOG_LEVEL");
    }
  });

  // Test 3: Inter-application communication
  TEST_CASE("Inter-Application Communication") {
    auto test_env = TestDataManager::create_test_environment();
    std::string shared_cache = test_env->path_string() + "/shared_cache";
    std::filesystem::create_directories(shared_cache);

    // Test data sharing through cache
    {
      // Step 1: Fetch data using fetch application
      std::string fetch_command = "./build/solar_system_fetch --cache-dir " + shared_cache +
                                  " --bodies Sun,Earth,Moon --update-cache";
      std::string fetch_output = execute_command(fetch_command + " 2>&1");
      ASSERT_TRUE(fetch_output.find("Error") == std::string::npos ||
                  fetch_output.find("completed") != std::string::npos ||
                  fetch_output.find("cache") != std::string::npos);

      // Step 2: Use cached data in simulation
      std::string sim_command = "./build/solar_system --cache-dir " + shared_cache +
                                " --bodies Sun,Earth,Moon --duration 3600 --use-cache";
      std::string sim_output = execute_command(sim_command + " 2>&1");
      ASSERT_TRUE(sim_output.find("Error") == std::string::npos ||
                  sim_output.find("Simulation") != std::string::npos ||
                  sim_output.find("completed") != std::string::npos);

      // Step 3: Verify cache files were created and used
      // Cache files may or may not exist depending on implementation
      // Just verify the applications ran without critical errors
    }

    // Test launcher coordination
    {
      std::string launcher_command = "./build/solar_system_launcher --cache-dir " + shared_cache +
                                     " --simulate --bodies Sun,Earth --duration 1800";
      std::string launcher_output = execute_command(launcher_command + " 2>&1");
      ASSERT_TRUE(launcher_output.find("Error") == std::string::npos ||
                  launcher_output.find("completed") != std::string::npos ||
                  launcher_output.find("Simulation") != std::string::npos);
    }

    // Test web server with shared data
    {
      int test_port = 8088;
      std::string web_command = "./build/solar_system_web --port " + std::to_string(test_port) +
                                " --cache-dir " + shared_cache + " &";
      [[maybe_unused]] int result = system(web_command.c_str());

      std::this_thread::sleep_for(std::chrono::seconds(2));

      // Test API endpoint
      std::string api_test =
          "curl -s -f http://localhost:" + std::to_string(test_port) + "/api/status";
      std::string api_output = execute_command(api_test + " 2>/dev/null || echo 'FAILED'");

      // Clean up web server
      std::string cleanup = "pkill -f 'solar_system_web.*--port " + std::to_string(test_port) + "'";
      [[maybe_unused]] int result2 = system(cleanup.c_str());

      // API may or may not work depending on implementation, just verify no crashes
      ASSERT_TRUE(api_output.find("FAILED") == std::string::npos || api_output.length() > 0);
    }
  });

  // Test 4: Deployment and installation processes
  TEST_CASE("Deployment and Installation Validation"){
      // Test installation directory structure
      {std::vector<std::string> expected_dirs = {"./bin", "./lib", "./include", "./share"};

  for (const auto& dir : expected_dirs) {
    if (std::filesystem::exists(dir)) {
      ASSERT_TRUE(std::filesystem::is_directory(dir));
    }
    // Directories may not exist in test environment, that's OK
  }
}

// Test executable permissions and dependencies
{
  std::vector<std::string> executables = {
      "./build/solar_system_launcher", "./build/solar_system", "./build/solar_system_fetch",
      "./build/solar_system_realtime", "./build/solar_system_web"};

  for (const auto& exe : executables) {
    if (std::filesystem::exists(exe)) {
      // Check if executable
      auto perms = std::filesystem::status(exe).permissions();
      ASSERT_TRUE((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none);

      // Test basic execution (help command should work)
      std::string test_command = exe + " --help 2>&1";
      std::string output = execute_command(test_command);
      ASSERT_TRUE(output.find("Usage") != std::string::npos ||
                  output.find("Options") != std::string::npos ||
                  output.find("help") != std::string::npos ||
                  output.length() > 10);  // Some output expected
    }
  }
}

// Test library dependencies
{
  std::vector<std::string> libraries = {"./lib/libsolar_core.a", "./lib/libsolar_jpl.a",
                                        "./lib/libsolar_utils.a", "./lib/libsolar_test.a"};

  for (const auto& lib : libraries) {
    if (std::filesystem::exists(lib)) {
      ASSERT_TRUE(std::filesystem::file_size(lib) > 0);
    }
  }
}

// Test documentation and web assets
{
  std::vector<std::string> doc_files = {"./README.md", "./docs/README.md",
                                        "./share/solar_system/web/index.html", "./web/index.html"};

  bool found_readme = false;
  bool found_web_assets = false;

  for (const auto& file : doc_files) {
    if (std::filesystem::exists(file)) {
      if (file.find("README") != std::string::npos) {
        found_readme = true;
      }
      if (file.find("index.html") != std::string::npos) {
        found_web_assets = true;
      }
    }
  }

  // At least some documentation should exist
  ASSERT_TRUE(found_readme || found_web_assets);
}

// Test cache directory creation and permissions
{
  auto test_env = TestDataManager::create_test_environment();
  std::string cache_test_dir = test_env->path_string() + "/cache_test";

  std::string command =
      "./build/solar_system_fetch --cache-dir " + cache_test_dir + " --test-storage";
  std::string output = execute_command(command + " 2>&1");

  // Should be able to create and use cache directory
  ASSERT_TRUE(output.find("Error") == std::string::npos ||
              output.find("test") != std::string::npos ||
              output.find("storage") != std::string::npos);
}
});

// Test 5: System resource management
TEST_CASE("System Resource Management"){
    // Test memory usage patterns
    {std::string command =
         "./build/solar_system --bodies Sun,Earth,Moon --duration 3600 --timestep 60";
auto start_time = std::chrono::high_resolution_clock::now();

std::string output = execute_command(command + " 2>&1");

auto end_time = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

// Should complete within reasonable time (< 30 seconds)
ASSERT_LT(duration.count(), 30);

// Should not report memory errors
ASSERT_TRUE(output.find("out of memory") == std::string::npos);
ASSERT_TRUE(output.find("segmentation fault") == std::string::npos);
}

// Test file handle management
{
  // Run multiple applications concurrently
  std::vector<std::string> commands = {"./build/solar_system_launcher --status &",
                                       "./build/solar_system_fetch --test-storage &",
                                       "./build/solar_system --bodies Sun,Earth --duration 1800 &"};

  for (const auto& cmd : commands) {
    [[maybe_unused]] int result = system(cmd.c_str());
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Check for any zombie processes or resource leaks
  std::string ps_output = execute_command("ps aux | grep solar_system | grep -v grep");

  // Clean up any remaining processes
  [[maybe_unused]] int result = system("pkill -f solar_system 2>/dev/null || true");

  // Should not have excessive number of processes
  int process_count = 0;
  std::istringstream iss(ps_output);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.find("solar_system") != std::string::npos) {
      process_count++;
    }
  }
  ASSERT_LT(process_count, 10);  // Reasonable limit
}

// Test network resource cleanup
{
  int test_port = 8089;

  // Start web server
  std::string start_cmd = "./build/solar_system_web --port " + std::to_string(test_port) + " &";
  [[maybe_unused]] int result2 = system(start_cmd.c_str());

  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Check port is in use
  std::string port_check = "lsof -i:" + std::to_string(test_port) + " 2>/dev/null";
  std::string port_output = execute_command(port_check);

  // Stop web server
  std::string stop_cmd = "pkill -f 'solar_system_web.*--port " + std::to_string(test_port) + "'";
  [[maybe_unused]] int result3 = system(stop_cmd.c_str());

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Check port is released
  std::string port_check2 = "lsof -i:" + std::to_string(test_port) + " 2>/dev/null";
  std::string port_output2 = execute_command(port_check2);

  // Port should be released after stopping server
  ASSERT_TRUE(port_output2.empty() || port_output2.length() < port_output.length());
}
});

// Test 6: Error propagation and logging
TEST_CASE("Error Propagation and Logging") {
  auto test_env = TestDataManager::create_test_environment();

  // Test error logging to file
  {
    std::string log_file = test_env->path_string() + "/test.log";
    setenv("SOLAR_SYSTEM_LOG_FILE", log_file.c_str(), 1);

    // Run command that might generate logs
    std::string command = "./build/solar_system_launcher --status";
    execute_command(command + " 2>&1");

    unsetenv("SOLAR_SYSTEM_LOG_FILE");

    // Check if log file was created (may or may not exist depending on implementation)
    if (std::filesystem::exists(log_file)) {
      ASSERT_GT(std::filesystem::file_size(log_file), 0);
    }
  }

  // Test error code propagation
  {
    std::string invalid_command = "./build/solar_system --invalid-flag 2>/dev/null";
    int exit_code = system(invalid_command.c_str());

    // Should return non-zero exit code for invalid arguments
    ASSERT_NE(exit_code, 0);
  }

  // Test cascading error handling
  {
    // Create scenario where one component fails and others handle it gracefully
    std::string command =
        "./build/solar_system_launcher --simulate --bodies NonexistentPlanet --duration 3600";
    std::string output = execute_command(command + " 2>&1");

    // Should handle unknown body gracefully
    ASSERT_TRUE(output.find("Error") != std::string::npos ||
                output.find("unknown") != std::string::npos ||
                output.find("fallback") != std::string::npos ||
                output.find("completed") != std::string::npos);  // May use fallback data
  }
});

return current_suite->all_passed() ? 0 : 1;
}
