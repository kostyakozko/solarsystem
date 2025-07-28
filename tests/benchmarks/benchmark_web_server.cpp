/**
 * @file benchmark_web_server.cpp
 * @brief Performance benchmarks for web server
 */

#include <filesystem>

#include "benchmark_utils.h"
#include "test_framework.h"

int main() {
  Benchmark::BenchmarkSuite suite("Web Server Performance");

  // HTTP request processing benchmark
  suite.run_benchmark(
      "HTTPRequestProcessingBenchmark",
      []() {
        // Simulate HTTP request processing
        std::string request = "GET /api/simulation HTTP/1.1\r\nHost: localhost\r\n\r\n";
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
          sum += positions[i];
        }
        (void)sum;
      },
      1000);

  suite.print_summary();

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/web_server_benchmark.csv", "csv");

  return 0;
}
