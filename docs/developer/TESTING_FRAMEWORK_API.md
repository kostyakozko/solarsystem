# Solar System Testing Framework API Documentation

## Overview

The Solar System Testing Framework provides a comprehensive, modern testing infrastructure for the Solar System Suite. It includes unit testing, integration testing, performance benchmarking, mocking capabilities, and CI/CD integration.

## Quick Start

```cpp
#include <solar_test/solar_test.hpp>

// Simple test case
SOLAR_TEST_CASE(BasicMathTest, "Test basic mathematical operations") {
  assert_equals(4, 2 + 2, "Addition should work correctly");
  assert_true(5 > 3, "Comparison should work");
}

// Benchmark test case
SOLAR_BENCHMARK_CASE(CachePerformanceTest, "Benchmark cache loading performance") {
  auto start = std::chrono::high_resolution_clock::now();

  // Your performance-critical code here
  load_ephemeris_cache();

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = sturation_cast<std::chrono::milliseconds>(end - start);

  assert_less_than(duration.count(), 100, "Cache loading should be under 100ms");
}

// Main function
SOLAR_TEST_MAIN()
```

## Core Classes

### TestCase

Base class for all test cases. Provides assertion methods and test lifecycle management.

#### Constructor

```cpp
explicit TestCase(TestInfo info);
```

**Parameters:**
- `info`: Test metadata including name, description, tags, timeout, and benchmark flag

#### Lifecycle Methods

```cpp
virtual void setup();    // Called before run()
virtual void run() = 0;  // Test implementation (pure virtual)
virtual void teardown(); // Called after run()
```

#### Assertion Methods

##### Basic Assertions

```cpp
void assert_true(bool condition, const std::string& message = "");
void assert_false(bool condition, const std::string& message = "");

template<typename T>
void assert_equals(const T& expected, const T& actual, const std::string& message = "");

template<typename T>
void assert_not_equals(const T& expected, const T& actual, const std::string& message = "");
```

**Example:**
```cpp
assert_true(is_valid_date("2024-01-01"), "Date should be valid");
assert_equals(42, calculate_answer(), "Answer should be 42");
assert_not_equals(0, get_planet_count(), "Should have planets");
```

##### Exception Assertions

```cpp
void assert_throws(const std::function<void()>& func, const std::string& message = "");
void assert_no_throw(const std::function<void()>& func, const std::string& message = "");
```

**Example:**
```cpp
assert_throws([]() {
  divide_by_zero();
}, "Division by zero should throw");

assert_no_throw([]() {
  load_valid_config();
}, "Valid config should not throw");
```

##### String Assertions

```cpp
void assert_contains(const std::string& haystack, const std::string& needle,
                     const std::string& message = "");
void assert_starts_with(const std::string& str, const std::string& prefix,
                        const std::string& message = "");
void assert_ends_with(const std::string& str, const std::string& suffix,
                      const std::string& message = "");
```

**Example:**
```cpp
assert_contains(error_message, "JPL API", "Error should mention JPL API");
assert_starts_with(file_path, "/tmp/", "Temp files should be in /tmp/");
assert_ends_with(cache_file, ".bin", "Cache files should have .bin extension");
```

##### Numeric Assertions

```cpp
template<typename T>
void assert_near(const T& expected, const T& actual, const T& tolerance,
                 const std::string& message = "");

template<typename T>
void assert_greater_than(const T& actual, const T& threshold,
                         const std::string& message = "");

template<typename T>
void assert_less_than(const T& actual, const T& threshold,
                      const std::string& message = "");
```

**Example:**
```cpp
assert_near(3.14159, calculated_pi, 0.001, "Pi calculation should be accurate");
assert_greater_than(performance_score, 100.0, "Performance should exceed threshold");
assert_less_than(memory_usage, 1024*1024, "Memory usage should be under 1MB");
```

##### Performance Assertions

```cpp
void assert_execution_time_less_than(const std::function<void()>& func,
                                     std::chrono::milliseconds max_time);
void assert_memory_usage_less_than(const std::function<void()>& func, size_t max_bytes);
```

**Example:**
```cpp
assert_execution_time_less_than([]() {
  simulate_solar_system(1000);
}, std::chrono::milliseconds(500));

assert_memory_usage_less_than([]() {
  load_large_dataset();
}, 10 * 1024 * 1024); // 10MB limit
```

#### Test Control Methods

```cpp
void skip_test(const std::string& reason);
void add_metadata(const std::string& key, const std::string& value);
```

**Example:**
```cpp
if (!network_available()) {
  skip_test("Network not available for integration test");
}

add_metadata("test_data_version", "1.2.3");
add_metadata("jpl_api_endpoint", "https://ssd.jpl.nasa.gov/api/horizons.api");
```

### TestRunner

Main class for discovering, organizing, and executing tests.

#### Configuration

```cpp
struct Configuration {
  std::vector<std::string> test_patterns;     // Test name patterns to run
  std::vector<std::string> tags;              // Test tags to filter by
  bool parallel_execution = true;             // Enable parallel execution
  size_t max_threads = std::thread::hardware_concurrency();
  std::chrono::milliseconds timeout = std::chrono::minutes(5);
  bool generate_coverage = false;             // Generate code coverage
  std::string output_format = "console";      // Output format
  std::string output_file;                    // Output file path
  bool verbose = false;                       // Verbose output
  bool quiet = false;                         // Quiet output

  // CI/CD specific options
  bool ci_mode = false;                       // Enable CI optimizations
  bool fail_fast = false;                     // Stop on first failure
  double max_failure_rate = 1.0;             // Stop if failure rate exceeds
  bool cleanup_on_exit = true;                // Clean up resources
  std::string ci_system = "";                 // CI system identifier
  std::string artifact_directory = "";       // CI artifacts directory
  bool containerized = false;                 // Container environment
  std::chrono::milliseconds ci_timeout = std::chrono::minutes(30);
  size_t max_memory_mb = 0;                   // Memory limit (0 = no limit)
};
```

#### Constructor

```cpp
explicit TestRunner(Configuration config);
```

#### Test Registration

```cpp
void register_test(std::unique_ptr<TestCase> test_case);
void register_test_suite(const std::string& suite_name,
                         std::vector<std::unique_ptr<TestCase>> test_cases);
```

#### Test Execution

```cpp
TestSuiteResult run_all_tests();
TestSuiteResult run_tests_with_tag(const std::string& tag);
TestSuiteResult run_specific_test(const std::string& test_name);
TestSuiteResult run_tests_matching_pattern(const std::string& pattern);
```

**Example:**
```cpp
TestRunner::Configuration config;
config.parallel_execution = true;
config.max_threads = 4;
config.tags = {"unit", "fast"};

TestRunner runner(config);

// Register tests
runner.register_test(std::make_unique<BasicMathTest>());
runner.register_test(std::make_unique<CachePerformanceTest>());

// Run tests
auto result = runner.run_tests_with_tag("unit");
if (result.all_passed()) {
  std::cout << "All tests passed!" << std::endl;
}
```

#### Reporter Management

```cpp
void add_reporter(std::unique_ptr<TestReporter> reporter);
void clear_reporters();
```

**Example:**
```cpp
// Add console reporter
runner.add_reporter(std::make_unique<ConsoleReporter>(true)); // verbose

// Add XML reporter for CI
runner.add_reporter(std::make_unique<XmlReporter>("test_results.xml"));

// Add JSON reporter for analysis
runner.add_reporter(std::make_unique<JsonReporter>("test_results.json"));
```

## Mock Framework

### JPLMock

Mock implementation for JPL HORIZONS API testing.

```cpp
class JPLMock {
public:
  struct MockConfiguration {
    bool simulate_network_delays = false;
    std::chrono::milliseconds response_delay = std::chrono::milliseconds(100);
    double failure_rate = 0.0;  // 0.0 = never fail, 1.0 = always fail
    std::string mock_data_path = "tests/data/jpl_responses/";
  };

  explicit JPLMock(MockConfiguration config = {});

  // Mock responses
  void set_response_for_body(const std::string& body_name, const std::string& response);
  void set_error_response(int http_code, const std::string& error_message);
  void simulate_network_failure();
  void simulate_timeout();

  // Verification
  size_t call_count() const;
  std::vector<std::string> requested_bodies() const;
  void reset_call_history();
};
```

**Example:**
```cpp
JPLMock::MockConfiguration config;
config.simulate_network_delays = true;
config.response_delay = std::chrono::milliseconds(50);

JPLMock jpl_mock(config);

// Set up mock responses
jpl_mock.set_response_for_body("Earth", load_test_data("earth_response.json"));
jpl_mock.set_response_for_body("Mars", load_test_data("mars_response.json"));

// Simulate error conditions
jpl_mock.set_error_response(500, "Internal Server Error");

// Use in test
auto body_factory = BodyFactory(jpl_mock);
auto earth = body_factory.create_body("Earth");

// Verify interactions
assert_equals(1, jpl_mock.call_count(), "Should have called JPL API once");
assert_contains(jpl_mock.requested_bodies(), "Earth", "Should have requested Earth data");
```

### CacheMock

Mock implementation for cache system testing.

```cpp
class CacheMock {
public:
  // Mock cache operations
  void set_cache_exists(bool exists);
  void set_cache_valid(bool valid);
  void set_cache_data(const std::string& data);
  void simulate_cache_corruption();
  void simulate_disk_full();

  // Verification
  bool was_cache_read() const;
  bool was_cache_written() const;
  std::string last_written_data() const;
};
```

**Example:**
```cpp
CacheMock cache_mock;

// Set up cache state
cache_mock.set_cache_exists(true);
cache_mock.set_cache_valid(true);
cache_mock.set_cache_data(load_test_data("valid_cache.bin"));

// Test cache loading
auto cache_manager = CacheManager(cache_mock);
auto data = cache_manager.load_ephemeris_data();

// Verify cache was used
assert_true(cache_mock.was_cache_read(), "Cache should have been read");
assert_false(cache_mock.was_cache_written(), "Cache should not have been written");
```

### NetworkMock

Mock implementation for network operations.

```cpp
class NetworkMock {
public:
  // Network simulation
  void simulate_slow_connection(std::chrono::milliseconds delay);
  void simulate_connection_timeout();
  void simulate_connection_failure();
  void simulate_intermittent_failures(double failure_rate);

  // Response configuration
  void set_response(const std::string& url, const std::string& response);
  void set_http_status(const std::string& url, int status_code);
};
```

### TimeMock

Mock implementation for time-dependent testing.

```cpp
class TimeMock {
public:
  // Time control
  void set_current_time(std::chrono::system_clock::time_point time);
  void advance_time(std::chrono::duration<double> duration);
  void freeze_time();
  void unfreeze_time();

  // Time queries
  std::chrono::system_clock::time_point now() const;
  bool is_frozen() const;
};
```

**Example:**
```cpp
TimeMock time_mock;

// Set specific time for testing
auto test_time = std::chrono::system_clock::from_time_t(1640995200); // 2022-01-01
time_mock.set_current_time(test_time);

// Test time-dependent functionality
auto simulation = Simulation(time_mock);
simulation.advance_to_date("2022-06-01");

// Advance time and test again
time_mock.advance_time(std::chrono::hours(24 * 30)); // 30 days
auto new_positions = simulation.get_current_positions();
```

## Benchmark Framework

### Benchmark

Class for performance testing and regression detection.

```cpp
class Benchmark {
public:
  struct BenchmarkResult {
    std::string name;
    std::chrono::nanoseconds min_time;
    std::chrono::nanoseconds max_time;
    std::chrono::nanoseconds mean_time;
    std::chrono::nanoseconds median_time;
    size_t iterations;
    double operations_per_second;
    size_t memory_usage_bytes;
  };

  explicit Benchmark(const std::string& name);

  // Benchmark execution
  template<typename Func>
  BenchmarkResult measure(Func&& func, size_t iterations = 1000);

  template<typename Func>
  BenchmarkResult measure_with_setup(Func&& setup, Func&& func, size_t iterations = 1000);

  // Performance thresholds
  void set_time_threshold(std::chrono::nanoseconds max_time);
  void set_memory_threshold(size_t max_bytes);
  void set_operations_per_second_threshold(double min_ops_per_sec);
};
```

**Example:**
```cpp
Benchmark cache_benchmark("CacheLoadingBenchmark");
cache_benchmark.set_time_threshold(std::chrono::milliseconds(100));
cache_benchmark.set_memory_threshold(10 * 1024 * 1024); // 10MB

auto result = cache_benchmark.measure([&]() {
  cache_manager.load_ephemeris_data();
}, 1000);

std::cout << "Average time: " << result.mean_time.count() << "ns" << std::endl;
std::cout << "Operations/sec: " << result.operations_per_second << std::endl;
```

## Reporters

### ConsoleReporter

Human-readable console output with colors and progress indicators.

```cpp
class ConsoleReporter : public TestReporter {
public:
  explicit ConsoleReporter(bool verbose = false, bool use_colors = true);
};
```

### XmlReporter

JUnit-compatible XML output for CI systems.

```cpp
class XmlReporter : public TestReporter {
public:
  explicit XmlReporter(const std::string& output_file);
};
```

### JsonReporter

Structured JSON output for programmatic analysis.

```cpp
class JsonReporter : public TestReporter {
public:
  explicit JsonReporter(const std::string& output_file);
};
```

### TapReporter

Test Anything Protocol (TAP) output.

```cpp
class TapReporter : public TestReporter {
public:
  explicit TapReporter(const std::string& output_file = "");
};
```

## Test Data Management

### TestDataManager

Utilities for managing test data and temporary environments.

```cpp
class TestDataManager {
public:
  // Test data loading
  static TestDataSet load_jpl_responses(const std::string& scenario);
  static TestDataSet load_ephemeris_data(const std::string& time_period);
  static TestDataSet load_cache_samples(const std::string& cache_type);

  // Temporary environments
  static std::unique_ptr<TemporaryDirectory> create_test_environment();
  static std::unique_ptr<TemporaryCache> create_test_cache();

  // Data validation
  static bool validate_jpl_response(const std::string& response);
  static bool validate_ephemeris_data(const std::string& data);
  static bool validate_cache_integrity(const std::string& cache_path);

  // Cleanup
  static void cleanup_test_data();
  static void register_cleanup_handler();
};
```

**Example:**
```cpp
// Load test data
auto jpl_data = TestDataManager::load_jpl_responses("solar_system_2024");
auto ephemeris_data = TestDataManager::load_ephemeris_data("2020_2025");

// Create temporary environment
auto temp_dir = TestDataManager::create_test_environment();
temp_dir->create_file("config.json", test_config);

// Create temporary cache
auto temp_cache = TestDataManager::create_test_cache();
temp_cache->populate_with_valid_data();

// Automatic cleanup when objects go out of scope
```

## CI/CD Integration

### Command Line Usage

```bash
# Run all tests
./test_runner

# Run specific test categories
./test_runner --tags unit,fast
./test_runner --tags integration
./test_runner --tags benchmark

# Run tests matching pattern
./test_runner --pattern "*Cache*"
./test_runner --pattern "JPL*Test"

# CI mode with artifacts
./test_runner --ci-mode --artifact-dir ./test_results --output-format xml

# Parallel execution control
./test_runner --threads 4 --timeout 300

# Verbose output
./test_runner --verbose --output-format console
```

### GitHub Actions Integration

```yaml
- name: Run Unit Tests
  run: |
    cd build
    ctest -L "unit" --output-on-failure --timeout 60

- name: Run Integration Tests
  run: |
    cd build
    ctest -L "integration" --output-on-failure --timeout 180

- name: Run Performance Benchmarks
  run: |
    cd build
    ctest -L "benchmark" --output-on-failure --timeout 300
```

### Exit Codes

- `0`: All tests passed
- `1`: One or more tests failed
- `2`: Test discovery failed
- `3`: Configuration error
- `4`: Timeout exceeded
- `5`: Resource error (memory, disk, etc.)

## Error Handling

The framework uses structured error handling with specific error types:

```cpp
enum class TestError {
  TestDiscoveryFailed,
  TestExecutionTimeout,
  AssertionFailed,
  MockSetupFailed,
  TestDataNotFound,
  ReporterError,
  BenchmarkThresholdExceeded
};
```

All framework methods that can fail return `Expected<T, TestError>` or throw exceptions with detailed error messages.

## Thread Safety

The testing framework is designed to be thread-safe:

- Test execution can be parallelized safely
- Reporters use internal synchronization
- Mock objects are thread-safe for read operations
- Resource coordination prevents conflicts between parallel tests

## Memory Management

The framework follows RAII principles:

- Automatic cleanup of temporary resources
- Smart pointers for memory management
- Automatic test data cleanup
- Resource leak detection in debug builds

## Performance Considerations

- Parallel test execution scales with available CPU cores
- Memory usage is monitored and limited
- Benchmark results include memory usage statistics
- Performance regression detection with configurable thresholds
- Container environment optimization

## Extensibility

The framework is designed to be extensible:

- Custom test reporters can be implemented
- Additional mock implementations can be added
- Custom assertion methods can be defined
- Test data providers can be extended
- CI system integrations can be added

