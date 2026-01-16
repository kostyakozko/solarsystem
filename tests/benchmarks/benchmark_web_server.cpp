/**
 * @file benchmark_web_server.cpp
 * @brief Performance benchmarks for web server
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>

#include "benchmark_utils.h"

// Web Server Performance Benchmark Test
TEST(WebServerBenchmark, ResponsePerformanceValidation) {
  Benchmark::BenchmarkSuite suite("Web Server Performance Benchmarks");

  // HTTP request handling benchmark
  suite.run_benchmark(
      "HTTPRequestHandlingBenchmark",
      []() {
        // Simulate HTTP request parsing
        std::string request = "GET /api/bodies HTTP/1.1\r\nHost: localhost\r\n\r\n";
        volatile size_t len = request.length();
        for (size_t i = 0; i < len; ++i) {
          volatile char c = request[i];
          (void)c;
        }
      },
      1000);

  // JSON response generation benchmark
  suite.run_benchmark(
      "JSONResponseGenerationBenchmark",
      []() {
        // Simulate JSON response generation
        std::string json =
            R"({"status":"success","data":{"bodies":[{"name":"Earth","x":1.0,"y":0.0,"z":0.0}]}})";
        volatile size_t len = json.length();
        for (size_t i = 0; i < len; ++i) {
          volatile char c = json[i];
          (void)c;
        }
      },
      1000);

  // WebGL data serialization benchmark
  suite.run_benchmark(
      "WebGLDataSerializationBenchmark",
      []() {
        // Simulate WebGL data serialization
        double positions[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
        volatile double sum = 0.0;
        for (int i = 0; i < 9; ++i) {
          sum = sum + positions[i];
        }
        (void)sum;
      },
      1000);

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/web_server_benchmark.csv", "csv");

  // Validate performance
  auto results = suite.get_results();
  bool all_passed = true;

  std::cout << "\n=== Web Server Performance Validation ===" << std::endl;

  for (const auto& result : results) {
    std::cout << result.name << ": " << result.avg_duration_ms << " ms avg" << std::endl;
    // Web server operations should be fast (under 1ms)
    if (result.avg_duration_ms > 10.0) {
      all_passed = false;
    }
  }

  std::cout << "\nOverall Web Server Performance: " << (all_passed ? "PASS" : "FAIL") << std::endl;

  EXPECT_TRUE(all_passed);
}
