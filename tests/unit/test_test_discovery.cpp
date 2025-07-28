/**
 * @file test_test_discovery.cpp
 * @brief Unit tests for the test discovery mechanism
 */

#include <fstream>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_discovery.hpp"
#include "test_framework.h"

using namespace SolarSystem::Testing;

// Mock test cases for testing discovery
class MockUnitTest : public TestCase {
 public:
  MockUnitTest()
      : TestCase({"MockUnitTest",
                  "A mock unit test",
                  {"unit", "fast"},
                  std::chrono::seconds(10),
                  false}) {}
  void run() override {
    // Mock test implementation
  }
};

class MockIntegrationTest : public TestCase {
 public:
  MockIntegrationTest()
      : TestCase({"MockIntegrationTest",
                  "A mock integration test",
                  {"integration", "slow"},
                  std::chrono::minutes(1),
                  false}) {}
  void run() override {
    // Mock test implementation
  }
};

class MockBenchmarkTest : public TestCase {
 public:
  MockBenchmarkTest()
      : TestCase({"MockBenchmarkTest",
                  "A mock benchmark test",
                  {"benchmark", "performance"},
                  std::chrono::minutes(5),
                  true}) {}
  void run() override {
    // Mock test implementation
  }
};

int main() {
  TEST_SUITE("Test Discovery Mechanism Tests");

  TEST_CASE("Test registration and discovery") {
    // Clear any existing registrations
    TestDiscovery::instance().clear_registry();

    // Register mock test factories
    TestDiscovery::instance().register_test_factory(
        "MockUnitTest", []() { return std::make_unique<MockUnitTest>(); }, {"unit", "fast"});

    TestDiscovery::instance().register_test_factory(
        "MockIntegrationTest", []() { return std::make_unique<MockIntegrationTest>(); },
        {"integration", "slow"});

    TestDiscovery::instance().register_test_factory(
        "MockBenchmarkTest", []() { return std::make_unique<MockBenchmarkTest>(); },
        {"benchmark", "performance"});

    // Test discovery of all tests
    auto all_tests = TestDiscovery::instance().discover_all_tests();
    ASSERT_EQ(all_tests.size(), 3);

    // Test discovery by tag
    auto unit_tests = TestDiscovery::instance().discover_tests_by_tag("unit");
    ASSERT_EQ(unit_tests.size(), 1);
    ASSERT_EQ(unit_tests[0]->info().name, "MockUnitTest");

    auto integration_tests = TestDiscovery::instance().discover_tests_by_tag("integration");
    ASSERT_EQ(integration_tests.size(), 1);
    ASSERT_EQ(integration_tests[0]->info().name, "MockIntegrationTest");

    auto benchmark_tests = TestDiscovery::instance().discover_tests_by_tag("benchmark");
    ASSERT_EQ(benchmark_tests.size(), 1);
    ASSERT_EQ(benchmark_tests[0]->info().name, "MockBenchmarkTest");

    // Test discovery by multiple tags
    auto slow_tests = TestDiscovery::instance().discover_tests_by_tag("slow");
    ASSERT_EQ(slow_tests.size(), 1);

    auto fast_tests = TestDiscovery::instance().discover_tests_by_tag("fast");
    ASSERT_EQ(fast_tests.size(), 1);
  });

  TEST_CASE("Pattern matching") {
    // Clear any existing registrations
    TestDiscovery::instance().clear_registry();

    // Create test classes with unique names to avoid conflicts
    class UniqueTestBodyFactory : public TestCase {
     public:
      UniqueTestBodyFactory()
          : TestCase({"UniqueTestBodyFactory",
                      "Test body factory",
                      {"unit"},
                      std::chrono::seconds(10),
                      false}) {}
      void run() override {}
    };

    class UniqueTestSimulationEngine : public TestCase {
     public:
      UniqueTestSimulationEngine()
          : TestCase({"UniqueTestSimulationEngine",
                      "Test simulation engine",
                      {"unit"},
                      std::chrono::seconds(10),
                      false}) {}
      void run() override {}
    };

    class UniqueBenchmarkCachePerformance : public TestCase {
     public:
      UniqueBenchmarkCachePerformance()
          : TestCase({"UniqueBenchmarkCachePerformance",
                      "Benchmark cache performance",
                      {"benchmark"},
                      std::chrono::seconds(10),
                      true}) {}
      void run() override {}
    };

    // Register test with specific name pattern
    TestDiscovery::instance().register_test_factory(
        "UniqueTestBodyFactory", []() { return std::make_unique<UniqueTestBodyFactory>(); },
        {"unit"});

    TestDiscovery::instance().register_test_factory(
        "UniqueTestSimulationEngine",
        []() { return std::make_unique<UniqueTestSimulationEngine>(); }, {"unit"});

    TestDiscovery::instance().register_test_factory(
        "UniqueBenchmarkCachePerformance",
        []() { return std::make_unique<UniqueBenchmarkCachePerformance>(); }, {"benchmark"});

    // Test pattern matching with unique patterns
    auto test_pattern_tests = TestDiscovery::instance().discover_tests_by_pattern("UniqueTest.*");
    ASSERT_EQ(test_pattern_tests.size(), 2);

    auto benchmark_pattern_tests =
        TestDiscovery::instance().discover_tests_by_pattern("UniqueBenchmark.*");
    ASSERT_EQ(benchmark_pattern_tests.size(), 1);

    // Test wildcard pattern
    auto factory_tests =
        TestDiscovery::instance().discover_tests_by_pattern("*UniqueTestBodyFactory*");
    ASSERT_EQ(factory_tests.size(), 1);
    ASSERT_EQ(factory_tests[0]->info().name, "UniqueTestBodyFactory");
  });

  TEST_CASE("Advanced filtering with DiscoveryOptions") {
    // Clear any existing registrations
    TestDiscovery::instance().clear_registry();

    // Create test classes with the expected names
    class FastUnitTest : public TestCase {
     public:
      FastUnitTest()
          : TestCase({"FastUnitTest",
                      "Fast unit test",
                      {"unit", "fast"},
                      std::chrono::seconds(10),
                      false}) {}
      void run() override {}
    };

    class SlowIntegrationTest : public TestCase {
     public:
      SlowIntegrationTest()
          : TestCase({"SlowIntegrationTest",
                      "Slow integration test",
                      {"integration", "slow"},
                      std::chrono::seconds(10),
                      false}) {}
      void run() override {}
    };

    class PerformanceBenchmark : public TestCase {
     public:
      PerformanceBenchmark()
          : TestCase({"PerformanceBenchmark",
                      "Performance benchmark",
                      {"benchmark", "performance", "slow"},
                      std::chrono::seconds(10),
                      true}) {}
      void run() override {}
    };

    // Register various tests
    TestDiscovery::instance().register_test_factory(
        "FastUnitTest", []() { return std::make_unique<FastUnitTest>(); }, {"unit", "fast"});

    TestDiscovery::instance().register_test_factory(
        "SlowIntegrationTest", []() { return std::make_unique<SlowIntegrationTest>(); },
        {"integration", "slow"});

    TestDiscovery::instance().register_test_factory(
        "PerformanceBenchmark", []() { return std::make_unique<PerformanceBenchmark>(); },
        {"benchmark", "performance", "slow"});

    // Test filtering with options
    TestDiscovery::DiscoveryOptions options;

    // Include only fast tests
    options.required_tags = {"fast"};
    auto fast_tests = TestDiscovery::instance().discover_tests(options);
    ASSERT_EQ(fast_tests.size(), 1);
    ASSERT_EQ(fast_tests[0]->info().name, "FastUnitTest");

    // Exclude slow tests
    options = TestDiscovery::DiscoveryOptions{};
    options.excluded_tags = {"slow"};
    auto non_slow_tests = TestDiscovery::instance().discover_tests(options);
    ASSERT_EQ(non_slow_tests.size(), 1);
    ASSERT_EQ(non_slow_tests[0]->info().name, "FastUnitTest");

    // Exclude benchmarks
    options = TestDiscovery::DiscoveryOptions{};
    options.include_benchmarks = false;
    auto non_benchmark_tests = TestDiscovery::instance().discover_tests(options);
    ASSERT_EQ(non_benchmark_tests.size(), 2);

    // Include pattern
    options = TestDiscovery::DiscoveryOptions{};
    options.include_patterns = {"Fast*", "Performance*"};
    auto pattern_tests = TestDiscovery::instance().discover_tests(options);
    ASSERT_EQ(pattern_tests.size(), 2);

    // Exclude pattern
    options = TestDiscovery::DiscoveryOptions{};
    options.exclude_patterns = {"*Benchmark*"};
    auto non_benchmark_pattern_tests = TestDiscovery::instance().discover_tests(options);
    ASSERT_EQ(non_benchmark_pattern_tests.size(), 2);
  });

  TEST_CASE("Test information retrieval") {
    // Clear any existing registrations
    TestDiscovery::instance().clear_registry();

    // Register a test
    TestDiscovery::instance().register_test_factory(
        "InfoTest", []() { return std::make_unique<MockUnitTest>(); }, {"unit", "info"});

    // Test name retrieval
    auto test_names = TestDiscovery::instance().get_available_test_names();
    ASSERT_EQ(test_names.size(), 1);
    ASSERT_EQ(test_names[0], "InfoTest");

    // Test tag retrieval
    auto available_tags = TestDiscovery::instance().get_available_tags();
    ASSERT_EQ(available_tags.size(), 2);
    ASSERT_TRUE(std::find(available_tags.begin(), available_tags.end(), "unit") !=
                available_tags.end());
    ASSERT_TRUE(std::find(available_tags.begin(), available_tags.end(), "info") !=
                available_tags.end());

    // Test existence check
    ASSERT_TRUE(TestDiscovery::instance().has_test("InfoTest"));
    ASSERT_FALSE(TestDiscovery::instance().has_test("NonExistentTest"));

    // Test info retrieval
    auto test_info = TestDiscovery::instance().get_test_info("InfoTest");
    ASSERT_NOT_NULL(test_info.get());
    ASSERT_EQ(test_info->name, "MockUnitTest");  // This is the actual test class name

    // Test count by category
    auto counts = TestDiscovery::instance().get_test_count_by_category();
    ASSERT_EQ(counts["total"], 1);
    ASSERT_EQ(counts["unit"], 1);
    ASSERT_EQ(counts["info"], 1);
  });

  TEST_CASE("Test scanner functionality") {
    // Create a temporary test file to scan
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

    // Test file scanning
    auto scan_result = TestScanner::scan_file(temp_file);

    // Debug output if test fails
    if (scan_result.found_tests.size() == 0 || scan_result.found_benchmarks.size() == 0) {
      std::cout << "Scanner debug info:" << std::endl;
      std::cout << "  Found tests: " << scan_result.found_tests.size() << std::endl;
      std::cout << "  Found benchmarks: " << scan_result.found_benchmarks.size() << std::endl;
      std::cout << "  Scan errors: " << scan_result.scan_errors.size() << std::endl;
      for (const auto& error : scan_result.scan_errors) {
        std::cout << "    Error: " << error << std::endl;
      }
    }

    // Verify found tests
    ASSERT_GT(scan_result.found_tests.size(), 0);
    ASSERT_GT(scan_result.found_benchmarks.size(), 0);

    // Check for specific test names
    bool found_sample_test = false;
    bool found_custom_test = false;
    for (const auto& test_name : scan_result.found_tests) {
      if (test_name == "SampleTest") found_sample_test = true;
      if (test_name == "CustomTest") found_custom_test = true;
    }
    ASSERT_TRUE(found_sample_test);
    ASSERT_TRUE(found_custom_test);

    // Check for benchmark
    bool found_sample_benchmark = false;
    for (const auto& benchmark_name : scan_result.found_benchmarks) {
      if (benchmark_name == "SampleBenchmark") found_sample_benchmark = true;
    }
    ASSERT_TRUE(found_sample_benchmark);

    // Verify tags were extracted
    ASSERT_GT(scan_result.test_tags.size(), 0);

    // Clean up
    std::remove(temp_file.c_str());
  });

  return current_suite->all_passed() ? 0 : 1;
}
