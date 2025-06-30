/**
 * @file test_application_workflows.cpp
 * @brief Integration tests for application workflows
 *
 * Tests the interaction between different applications and their
 * ability to work together as a complete system.
 */

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>

#include "test_data.h"
#include "test_framework.h"

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
                output.find("Test completed") != std::string::npos);
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
    std::string command =
        "timeout 10s ./apps/solar_system_realtime/solar_system_realtime --no-continuous";
    std::string output = execute_command(command);

    // Should provide real-time data snapshot
    ASSERT_TRUE(output.find("Solar System") != std::string::npos ||
                output.find("Position") != std::string::npos ||
                output.find("Real-time") != std::string::npos);
  });

  // Test web server startup
  TEST_CASE("Web Server Startup") {
    // Start web server in background
    std::string start_command =
        "./apps/solar_system_web/solar_system_web --port 8081 --web-root share/solar_system/web &";
    system(start_command.c_str());

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Test if server is responding
    std::string test_command =
        "curl -s -o /dev/null -w \"%{http_code}\" http://localhost:8081/api/status";
    std::string response = execute_command(test_command);

    // Should get HTTP 200 response
    ASSERT_TRUE(response.find("200") != std::string::npos);

    // Clean up - kill the web server
    system("pkill -f solar_system_web");
  });

  // Test data consistency across applications
  TEST_CASE("Data Consistency") {
    // Run simulation and capture output
    std::string sim_command = "./apps/solar_system/solar_system --date 2025-06-01 --verbose";
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
    std::string invalid_date_command = "./apps/solar_system/solar_system --date invalid-date";
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
    std::vector<std::string> expected_files = {
        "./apps/solar_system_launcher/solar_system_launcher", "./bin/solar_system",
        "./bin/solar_system_fetch", "./bin/solar_system_realtime", "./bin/solar_system_web"};

    for (const auto& file : expected_files) {
      ASSERT_TRUE(file_exists(file));
    }

    // Check for documentation
    ASSERT_TRUE(file_exists("./README.md"));
    ASSERT_TRUE(file_exists("./share/solar_system/web/index.html") ||
                file_exists("./docs/README.md"));
  });

  return current_suite->all_passed() ? 0 : 1;
}
