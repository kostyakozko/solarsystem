#include <fstream>
#include <iostream>

#include "lib/solar_test/include/solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;

int main() {
  // Create the same test content as in the test
  std::string test_content = R"(
#include "solar_test/framework/test_case.hpp"

SOLAR_TEST_CASE_AUTO(SampleTest, "A sample test", "unit", "fast") {
  // Test implementation
}

SOLAR_BENCHMARK_CASE_AUTO(SampleBenchmark, "A sample benchmark", "benchmark", "performance") {
  // Benchmark implementation
}

class CustomTest : public SolarSystem::Testing::TestCase {
public:
  CustomTest() : TestCase({"CustomTest", "Custom test", {"custom"}, std::chrono::seconds(30), false}) {}
  void run() override {}
};

SOLAR_REGISTER_TEST_WITH_TAGS(CustomTest, "custom", "manual");
)";

  // Write test content to a temporary file
  std::string temp_file = "/tmp/test_scanner_sample.cpp";
  std::ofstream file(temp_file);
  file << test_content;
  file.close();

  std::cout << "Test content written to: " << temp_file << std::endl;
  std::cout << "Content length: " << test_content.length() << " characters" << std::endl;

  // Test file scanning
  auto scan_result = TestScanner::scan_file(temp_file);

  std::cout << "Scan results:" << std::endl;
  std::cout << "  Found tests: " << scan_result.found_tests.size() << std::endl;
  for (const auto& test : scan_result.found_tests) {
    std::cout << "    - " << test << std::endl;
  }

  std::cout << "  Found benchmarks: " << scan_result.found_benchmarks.size() << std::endl;
  for (const auto& benchmark : scan_result.found_benchmarks) {
    std::cout << "    - " << benchmark << std::endl;
  }

  std::cout << "  Scan errors: " << scan_result.scan_errors.size() << std::endl;
  for (const auto& error : scan_result.scan_errors) {
    std::cout << "    - " << error << std::endl;
  }

  return 0;
}
