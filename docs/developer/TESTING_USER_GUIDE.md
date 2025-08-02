# Solar System Testing Framework User Guide

## Table of Contents

1. [Getting Started](#getting-started)
2. [Writing Your First Test](#writing-your-first-test)
3. [Test Types and Categories](#test-types-and-categories)
4. [Using Assertions](#using-assertions)
5. [Working with Mocks](#working-with-mocks)
6. [Performance Testing](#performance-testing)
7. [Test Data Management](#test-data-management)
8. [Running Tests](#running-tests)
9. [CI/CD Integration](#cicd-integration)
10. [Best Practices](#best-practices)
11. [Troubleshooting](#troubleshooting)

## Getting Started

### Prerequisites

- C++20 compatible compiler (GCC 7+, Clang 5+, MSVC 2019+)
- CMake 3.15+
- Solar System Suite libraries built with testing enabled

### Building with Testing Support

```bash
# Configure build westing enabled
cmake -B build -DENABLE_TESTING=ON
cmake --build build -j$(nproc)

# Verify test framework is available
cd build
ctest --show-only
```

### Project Structure for Tests

```
your_project/
├── tests/
│   ├── unit/                    # Unit tests
│   │   ├── test_body_factory.cpp
│   │   ├── test_simulation.cpp
│   │   └── test_cache_system.cpp
│   ├── integration/             # Integration tests
│   │   ├── test_jpl_integration.cpp
│   │   ├── test_web_interface.cpp
│   │   └── test_end_to_end.cpp
│   ├── benchmarks/              # Performance tests
│   │   ├── benchmark_cache.cpp
│   │   ├── benchmark_simulation.cpp
│   │   └── benchmark_jpl_parsing.cpp
│   └── data/                    # Test data
│       ├── jpl_responses/
│       ├── cache_samples/
│       └── ephemeris_data/
└── CMakeLists.txt
```

## Writing Your First Test

### Simple Unit Test

Create a file `tests/unit/test_basic_math.cpp`:

```cpp
#include <solar_test/solar_test.hpp>

// Simple test case
SOLAR_TEST_CASE(BasicAdditionTest, "Test that addition works correctly") {
  // Arrange
  int a = 2;
  int b = 3;

  // Act
  int result = a + b;

  // Assert
  assert_equals(5, result, "2 + 3 should equal 5");
}

// Test with setup and teardown
class DatabaseTest : public SolarSystem::Testing::TestCase {
public:
  DatabaseTest() : TestCase({"DatabaseTest", "Test database operations", {"unit", "database"}}) {}

  void setup() override {
    // Initialize test database
    test_db = std::make_unique<TestDatabase>();
    test_db->create_tables();
  }

  void run() override {
    // Test database operations
    test_db->insert_record("test_key", "test_value");
    auto value = test_db->get_record("test_key");
    assert_equals("test_value", value, "Should retrieve inserted value");
  }

  void teardown() override {
    // Clean up test database
    test_db->drop_tables();
    test_db.reset();
  }

private:
  std::unique_ptr<TestDatabase> test_db;
};

// Register the test
REGISTER_TEST(DatabaseTest);

// Main function for standalone test executable
SOLAR_TEST_MAIN()
```

### CMake Integration

Add to your `tests/unit/CMakeLists.txt`:

```cmake
# Create test executable
add_executable(test_basic_math test_basic_math.cpp)
target_link_libraries(test_basic_math
  PRIVATE
    solar_test
    solar_core
    solar_utils
)

# Register with CTest
add_test(NAME BasicMathTest COMMAND test_basic_math)
set_tests_properties(BasicMathTest PROPERTIES
  LABELS "unit;fast"
  TIMEOUT 30
)
```

## Test Types and Categories

### Unit Tests

Test individual components in isolation:

```cpp
SOLAR_TEST_CASE(BodyFactoryTest, "Test celestial body creation") {
  // Use mocks for external dependencies
  auto jpl_mock = std::make_shared<JPLMock>();
  jpl_mock->set_response_for_body("Earth", load_test_data("earth_response.json"));

  BodyFactory factory(jpl_mock);
  auto earth = factory.create_body("Earth");

  assert_not_equals(nullptr, earth, "Earth should be created");
  assert_equals("Earth", earth->name(), "Name should be correct");
  assert_greater_than(earth->mass(), 0.0, "Mass should be positive");
}
```

### Integration Tests

Test complete workflows:

```cpp
SOLAR_TEST_CASE(JPLIntegrationTest, "Test complete JPL data pipeline") {
  // This test uses real JPL API (or comprehensive mocks)
  JPLClient client;
  BodyFactory factory(client);
  SimulationBuilder builder;

  // Test complete pipeline
  auto bodies = factory.create_solar_system();
  auto simulation = builder.with_bodies(bodies)
                           .with_time_range("2024-01-01", "2024-12-31")
                           .build();

  auto result = simulation.run();

  assert_true(result.success(), "Simulation should complete successfully");
  assert_greater_than(result.step_count(), 0, "Should have simulation steps");
}
```

### Performance Tests

Test performance characteristics:

```cpp
SOLAR_BENCHMARK_CASE(CacheLoadingBenchmark, "Benchmark cache loading performance") {
  // Setup
  auto cache_manager = CacheManager();
  cache_manager.populate_cache_with_test_data();

  // Benchmark the operation
  assert_execution_time_less_than([&]() {
    auto data = cache_manager.load_ephemeris_data();
    assert_greater_than(data.size(), 0, "Should load data");
  }, std::chrono::milliseconds(100));

  // Memory usage test
  assert_memory_usage_less_than([&]() {
    auto large_dataset = cache_manager.load_full_solar_system();
  }, 50 * 1024 * 1024); // 50MB limit
}
```

### Expected Failure Tests

Test error conditions:

```cpp
SOLAR_EXPECTED_FAILURE_TEST(NetworkFailureTest,
                            "Test behavior when network fails",
                            "Network failures should be handled gracefully") {
  auto network_mock = std::make_shared<NetworkMock>();
  network_mock->simulate_connection_failure();

  JPLClient client(network_mock);

  // This should fail, but gracefully
  assert_throws([&]() {
    client.fetch_body_data("Mars");
  }, "Should throw network exception");
}
```

## Using Assertions

### Basic Assertions

```cpp
// Boolean assertions
assert_true(condition, "Condition should be true");
assert_false(!condition, "Condition should be false");

// Equality assertions
assert_equals(expected, actual, "Values should be equal");
assert_not_equals(unexpected, actual, "Values should not be equal");

// Null pointer assertions
assert_equals(nullptr, ptr, "Pointer should be null");
assert_not_equals(nullptr, ptr, "Pointer should not be null");
```

### String Assertions

```cpp
std::string response = get_jpl_response();

assert_contains(response, "EPHEMERIS", "Response should contain ephemeris data");
assert_starts_with(response, "$$SOE", "Response should start with start marker");
assert_ends_with(response, "$$EOE", "Response should end with end marker");
```

### Numeric Assertions

```cpp
double calculated_pi = calculate_pi();
assert_near(3.14159, calculated_pi, 0.001, "Pi should be accurate to 3 decimal places");

int performance_score = run_benchmark();
assert_greater_than(performance_score, 100, "Performance should exceed baseline");
assert_less_than(performance_score, 1000, "Performance should be reasonable");
```

### Exception Assertions

```cpp
// Test that exceptions are thrown
assert_throws([&]() {
  divide_by_zero();
}, "Division by zero should throw exception");

// Test that exceptions are NOT thrown
assert_no_throw([&]() {
  load_valid_configuration();
}, "Valid config should not throw");

// Test specific exception types
try {
  invalid_operation();
  assert_true(false, "Should have thrown exception");
} catch (const std::invalid_argument& e) {
  assert_contains(e.what(), "invalid", "Exception message should mention 'invalid'");
} catch (...) {
  assert_true(false, "Should have thrown std::invalid_argument");
}
```

### Performance Assertions

```cpp
// Time-based assertions
assert_execution_time_less_than([&]() {
  expensive_operation();
}, std::chrono::milliseconds(500));

// Memory-based assertions
assert_memory_usage_less_than([&]() {
  load_large_dataset();
}, 100 * 1024 * 1024); // 100MB limit
```

## Working with Mocks

### JPL API Mocking

```cpp
SOLAR_TEST_CASE(JPLMockTest, "Test JPL API mocking") {
  // Configure mock
  JPLMock::MockConfiguration config;
  config.simulate_network_delays = true;
  config.response_delay = std::chrono::milliseconds(50);
  config.failure_rate = 0.1; // 10% failure rate

  auto jpl_mock = std::make_shared<JPLMock>(config);

  // Set up responses
  jpl_mock->set_response_for_body("Earth", R"({
    "name": "Earth",
    "mass": 5.972e24,
    "radius": 6371000,
    "ephemeris": [...]
  })");

  // Use mock in test
  BodyFactory factory(jpl_mock);
  auto earth = factory.create_body("Earth");

  // Verify mock interactions
  assert_equals(1, jpl_mock->call_count(), "Should have made one API call");
  assert_contains(jpl_mock->requested_bodies(), "Earth", "Should have requested Earth");
}
```

### Cache System Mocking

```cpp
SOLAR_TEST_CASE(CacheMockTest, "Test cache system with mocks") {
  auto cache_mock = std::make_shared<CacheMock>();

  // Test cache hit scenario
  cache_mock->set_cache_exists(true);
  cache_mock->set_cache_valid(true);
  cache_mock->set_cache_data(load_test_data("valid_cache.bin"));

  CacheManager manager(cache_mock);
  auto data = manager.load_ephemeris_data();

  assert_true(cache_mock->was_cache_read(), "Cache should have been read");
  assert_false(cache_mock->was_cache_written(), "Cache should not have been written");

  // Test cache miss scenario
  cache_mock->set_cache_exists(false);
  data = manager.load_ephemeris_data();

  assert_true(cache_mock->was_cache_written(), "Cache should have been written");
}
```

### Network Mocking

```cpp
SOLAR_TEST_CASE(NetworkMockTest, "Test network conditions") {
  auto network_mock = std::make_shared<NetworkMock>();

  // Test slow network
  network_mock->simulate_slow_connection(std::chrono::seconds(2));

  auto start = std::chrono::high_resolution_clock::now();
  auto client = HTTPClient(network_mock);
  auto response = client.get("https://example.com/api");
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  assert_greater_than(duration.count(), 1, "Should have taken at least 2 seconds");

  // Test network failure
  network_mock->simulate_connection_failure();
  assert_throws([&]() {
    client.get("https://example.com/api");
  }, "Should throw network exception");
}
```

### Time Mocking

```cpp
SOLAR_TEST_CASE(TimeMockTest, "Test time-dependent functionality") {
  auto time_mock = std::make_shared<TimeMock>();

  // Set specific time
  auto test_time = std::chrono::system_clock::from_time_t(1640995200); // 2022-01-01
  time_mock->set_current_time(test_time);

  TimeBasedSimulation simulation(time_mock);
  auto initial_state = simulation.get_current_state();

  // Advance time
  time_mock->advance_time(std::chrono::hours(24)); // 1 day
  simulation.update();
  auto new_state = simulation.get_current_state();

  assert_not_equals(initial_state, new_state, "State should change over time");
}
```

## Performance Testing

### Basic Benchmarking

```cpp
SOLAR_BENCHMARK_CASE(SimulationPerformance, "Benchmark simulation performance") {
  Simulation simulation;
  simulation.initialize_solar_system();

  // Measure single step performance
  Benchmark step_benchmark("SimulationStep");
  auto result = step_benchmark.measure([&]() {
    simulation.advance_time_step();
  }, 10000); // 10,000 iterations

  // Verify performance requirements
  assert_less_than(result.mean_time.count(), 1000000, "Step should take < 1ms");
  assert_greater_than(result.operations_per_second, 1000, "Should achieve > 1000 ops/sec");

  std::cout << "Average step time: " << result.mean_time.count() << "ns" << std::endl;
  std::cout << "Operations per second: " << result.operations_per_second << std::endl;
}
```

### Memory Performance Testing

```cpp
SOLAR_BENCHMARK_CASE(MemoryUsageTest, "Test memory usage patterns") {
  // Test memory usage of large operations
  assert_memory_usage_less_than([&]() {
    auto large_simulation = create_large_solar_system(1000); // 1000 bodies
    large_simulation.run_for_steps(100);
  }, 500 * 1024 * 1024); // 500MB limit

  // Test for memory leaks
  size_t initial_memory = get_current_memory_usage();

  for (int i = 0; i < 1000; ++i) {
    auto simulation = create_simulation();
    simulation.run_single_step();
    // simulation should be destroyed here
  }

  size_t final_memory = get_current_memory_usage();
  size_t memory_growth = final_memory - initial_memory;

  assert_less_than(memory_growth, 10 * 1024 * 1024, "Memory growth should be < 10MB");
}
```

### Regression Testing

```cpp
SOLAR_BENCHMARK_CASE(PerformanceRegression, "Detect performance regressions") {
  // Load baseline performance data
  auto baseline = load_performance_baseline("cache_loading_baseline.json");

  // Run current performance test
  Benchmark benchmark("CacheLoading");
  auto current_result = benchmark.measure([&]() {
    load_ephemeris_cache();
  }, 1000);

  // Compare against baseline
  double performance_ratio = static_cast<double>(current_result.mean_time.count()) /
                            baseline.mean_time_ns;

  assert_less_than(performance_ratio, 1.1, "Performance should not degrade by > 10%");

  // Save current results as new baseline if better
  if (performance_ratio < 0.95) {
    save_performance_baseline("cache_loading_baseline.json", current_result);
  }
}
```

## Test Data Management

### Using Test Data Sets

```cpp
SOLAR_TEST_CASE(TestDataExample, "Example of using test data") {
  // Load predefined test data
  auto jpl_data = TestDataManager::load_jpl_responses("solar_system_2024");
  auto ephemeris_data = TestDataManager::load_ephemeris_data("2020_2025");

  // Validate test data
  assert_true(TestDataManager::validate_jpl_response(jpl_data.files["earth.json"]),
              "Earth JPL response should be valid");

  // Use test data in simulation
  auto simulation = create_simulation_with_data(ephemeris_data);
  auto result = simulation.run();

  assert_true(result.success(), "Simulation with test data should succeed");
}
```

### Creating Temporary Test Environments

```cpp
SOLAR_TEST_CASE(TemporaryEnvironmentTest, "Test with temporary environment") {
  // Create temporary directory
  auto temp_dir = TestDataManager::create_test_environment();

  // Create test files
  temp_dir->create_file("config.json", R"({
    "simulation_steps": 1000,
    "time_step": 3600,
    "output_format": "json"
  })");

  temp_dir->create_subdirectory("cache");
  temp_dir->create_file("cache/ephemeris.bin", generate_test_cache_data());

  // Use temporary environment
  auto config = load_configuration(temp_dir->path() + "/config.json");
  assert_equals(1000, config.simulation_steps, "Config should be loaded correctly");

  // Automatic cleanup when temp_dir goes out of scope
}
```

### Creating Test Caches

```cpp
SOLAR_TEST_CASE(TestCacheExample, "Example of using test cache") {
  // Create temporary cache with valid data
  auto temp_cache = TestDataManager::create_test_cache();
  temp_cache->populate_with_valid_data();

  CacheManager manager(temp_cache->cache_path());
  auto data = manager.load_ephemeris_data();

  assert_greater_than(data.size(), 0, "Should load cache data");

  // Test with corrupted cache
  temp_cache->simulate_partial_corruption();

  assert_throws([&]() {
    manager.load_ephemeris_data();
  }, "Corrupted cache should throw exception");
}
```

## Running Tests

### Command Line Usage

```bash
# Run all tests
cd build
ctest

# Run specific test categories
ctest -L "unit"                    # Unit tests only
ctest -L "integration"             # Integration tests only
ctest -L "benchmark"               # Performance tests only
ctest -L "unit;fast"               # Unit tests tagged as fast

# Run specific tests
ctest -R "BodyFactory"             # Tests matching pattern
ctest -R "Cache.*Test"             # Tests matching regex

# Verbose output
ctest --verbose
ctest --output-on-failure

# Parallel execution
ctest -j 4                         # Use 4 parallel jobs

# Timeout control
ctest --timeout 300                # 5 minute timeout
```

### Using Test Runner Directly

```bash
# Run test executable directly
./tests/unit/test_body_factory

# With custom options
./tests/unit/test_body_factory --verbose --tags unit,fast

# Generate reports
./tests/unit/test_body_factory --output-format xml --output-file results.xml
```

### Configuration Files

Create `test_config.json`:

```json
{
  "parallel_execution": true,
  "max_threads": 4,
  "timeout_seconds": 300,
  "output_format": "console",
  "verbose": false,
  "tags": ["unit", "integration"],
  "test_patterns": ["*Test", "*Benchmark"],
  "ci_mode": false,
  "generate_coverage": false
}
```

Use with:

```bash
./test_runner --config test_config.json
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Test Suite

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Configure CMake
      run: cmake -B build -DENABLE_TESTING=ON

    - name: Build
      run: cmake --build build -j$(nproc)

    - name: Run Unit Tests
      run: |
        cd build
        ctest -L "unit" --output-on-failure --timeout 60

    - name: Run Integration Tests
      run: |
        cd build
        ctest -L "integration" --output-on-failure --timeout 180

    - name: Run Performance Tests
      run: |
        cd build
        ctest -L "benchmark" --output-on-failure --timeout 300

    - name: Generate Test Report
      if: always()
      run: |
        cd build
        ctest --output-junit test_results.xml

    - name: Upload Test Results
      if: always()
      uses: actions/upload-artifact@v3
      with:
        name: test-results
        path: build/test_results.xml
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                sh 'cmake -B build -DENABLE_TESTING=ON'
                sh 'cmake --build build -j$(nproc)'
            }
        }

        stage('Test') {
            parallel {
                stage('Unit Tests') {
                    steps {
                        sh 'cd build && ctest -L "unit" --output-on-failure'
                    }
                }
                stage('Integration Tests') {
                    steps {
                        sh 'cd build && ctest -L "integration" --output-on-failure'
                    }
                }
                stage('Performance Tests') {
                    steps {
                        sh 'cd build && ctest -L "benchmark" --output-on-failure'
                    }
                }
            }
        }
    }

    post {
        always {
            publishTestResults testResultsPattern: 'build/test_results.xml'
        }
    }
}
```

### Docker Integration

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . /app
WORKDIR /app

# Build with testing
RUN cmake -B build -DENABLE_TESTING=ON
RUN cmake --build build -j$(nproc)

# Run tests
CMD ["ctest", "--test-dir", "build", "--output-on-failure"]
```

## Best Practices

### Test Organization

1. **One test per behavior**: Each test should verify one specific behavior
2. **Descriptive names**: Test names should clearly describe what is being tested
3. **Arrange-Act-Assert**: Structure tests with clear setup, execution, and verification phases
4. **Independent tests**: Tests should not depend on each other's state

```cpp
// Good: Specific, descriptive test
SOLAR_TEST_CASE(BodyFactory_CreateEarth_ReturnsValidEarthObject,
                "BodyFactory should create valid Earth object with correct properties") {
  // Arrange
  auto jpl_mock = std::make_shared<JPLMock>();
  jpl_mock->set_response_for_body("Earth", load_earth_test_data());
  BodyFactory factory(jpl_mock);

  // Act
  auto earth = factory.create_body("Earth");

  // Assert
  assert_not_equals(nullptr, earth, "Earth object should be created");
  assert_equals("Earth", earth->name(), "Name should be 'Earth'");
  assert_near(5.972e24, earth->mass(), 1e20, "Mass should be approximately correct");
}

// Bad: Vague, tests multiple things
SOLAR_TEST_CASE(TestBodyFactory, "Test body factory") {
  BodyFactory factory;
  auto earth = factory.create_body("Earth");
  auto mars = factory.create_body("Mars");
  assert_not_equals(nullptr, earth);
  assert_not_equals(nullptr, mars);
  // What exactly are we testing?
}
```

### Mock Usage

1. **Use mocks for external dependencies**: Network, file system, time, etc.
2. **Verify interactions**: Check that mocks were called correctly
3. **Reset mocks between tests**: Ensure clean state for each test

```cpp
class JPLIntegrationTest : public SolarSystem::Testing::TestCase {
public:
  JPLIntegrationTest() : TestCase({"JPLIntegrationTest", "Test JPL integration"}) {}

  void setup() override {
    jpl_mock_ = std::make_shared<JPLMock>();
    // Reset mock state
    jpl_mock_->reset_call_history();
  }

  void run() override {
    // Configure mock
    jpl_mock_->set_response_for_body("Mars", mars_test_data_);

    // Test
    BodyFactory factory(jpl_mock_);
    auto mars = factory.create_body("Mars");

    // Verify
    assert_equals(1, jpl_mock_->call_count(), "Should call JPL API once");
    assert_contains(jpl_mock_->requested_bodies(), "Mars", "Should request Mars data");
  }

private:
  std::shared_ptr<JPLMock> jpl_mock_;
  std::string mars_test_data_ = load_test_data("mars_response.json");
};
```

### Performance Testing

1. **Establish baselines**: Save performance baselines for regression detection
2. **Use realistic data**: Test with data similar to production
3. **Account for variance**: Use statistical measures, not single measurements
4. **Set reasonable thresholds**: Allow for normal performance variation

```cpp
SOLAR_BENCHMARK_CASE(CachePerformanceRegression, "Detect cache performance regressions") {
  // Load baseline
  auto baseline = load_performance_baseline("cache_baseline.json");

  // Run benchmark multiple times for statistical accuracy
  Benchmark benchmark("CacheLoading");
  std::vector<std::chrono::nanoseconds> measurements;

  for (int i = 0; i < 100; ++i) {
    auto result = benchmark.measure([&]() {
      load_ephemeris_cache();
    }, 10);
    measurements.push_back(result.mean_time);
  }

  // Calculate statistics
  auto mean_time = calculate_mean(measurements);
  auto std_dev = calculate_std_dev(measurements);

  // Allow for 10% performance degradation + 2 standard deviations
  auto threshold = baseline.mean_time * 1.1 + 2 * std_dev;

  assert_less_than(mean_time.count(), threshold.count(),
                   "Performance should not degrade significantly");
}
```

### Error Handling

1. **Test error conditions**: Verify that errors are handled correctly
2. **Use specific assertions**: Check for specific exception types and messages
3. **Test recovery**: Verify that the system can recover from errors

```cpp
SOLAR_TEST_CASE(JPLClient_NetworkFailure_HandlesGracefully,
                "JPL client should handle network failures gracefully") {
  auto network_mock = std::make_shared<NetworkMock>();
  network_mock->simulate_connection_failure();

  JPLClient client(network_mock);

  // Should throw specific exception
  try {
    client.fetch_body_data("Earth");
    assert_true(false, "Should have thrown NetworkException");
  } catch (const NetworkException& e) {
    assert_contains(e.what(), "connection failed",
                    "Exception should mention connection failure");
  } catch (...) {
    assert_true(false, "Should have thrown NetworkException, not other exception");
  }

  // Should be able to recover
  network_mock->restore_connection();
  network_mock->set_response("Earth", valid_earth_response_);

  assert_no_throw([&]() {
    auto data = client.fetch_body_data("Earth");
  }, "Should recover after network restoration");
}
```

## Troubleshooting

### Common Issues

#### Test Discovery Problems

**Problem**: Tests are not being discovered by CTest

**Solution**:
```bash
# Check if tests are registered
ctest --show-only

# Verify CMake configuration
cmake -B build -DENABLE_TESTING=ON
cmake --build build

# Check test labels
ctest --print-labels
```

#### Mock Setup Issues

**Problem**: Mocks are not working as expected

**Solution**:
```cpp
// Ensure mock is properly configured
auto jpl_mock = std::make_shared<JPLMock>();
jpl_mock->set_response_for_body("Earth", earth_data);

// Verify mock is being used
BodyFactory factory(jpl_mock);  // Pass mock to constructor
auto earth = factory.create_body("Earth");

// Check mock interactions
assert_greater_than(jpl_mock->call_count(), 0, "Mock should have been called");
```

#### Performance Test Failures

**Problem**: Performance tests are failing inconsistently

**Solution**:
```cpp
// Use multiple measurements for stability
Benchmark benchmark("Operation");
std::vector<double> measurements;

for (int i = 0; i < 10; ++i) {
  auto result = benchmark.measure(operation, 100);
  measurements.push_back(result.mean_time.count());
}

// Use statistical measures
auto median_time = calculate_median(measurements);
auto percentile_95 = calculate_percentile(measurements, 0.95);

// Set reasonable thresholds
assert_less_than(percentile_95, threshold, "95th percentile should be under threshold");
```

#### Memory Leak Detection

**Problem**: Tests are reporting memory leaks

**Solution**:
```cpp
// Use RAII and smart pointers
auto resource = std::make_unique<ExpensiveResource>();

// Ensure proper cleanup in teardown
void teardown() override {
  cleanup_global_state();
  reset_singletons();
  clear_caches();
}

// Use memory tracking
size_t initial_memory = get_memory_usage();
{
  // Test code that might leak
  run_test_operation();
}
size_t final_memory = get_memory_usage();
assert_equals(initial_memory, final_memory, "Memory should not leak");
```

#### CI/CD Integration Issues

**Problem**: Tests pass locally but fail in CI

**Solution**:
```bash
# Use same environment as CI
docker run -it ubuntu:22.04 bash

# Install same dependencies as CI
apt-get update && apt-get install -y build-essential cmake

# Run tests with same configuration
cmake -B build -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest --output-on-failure
```

### Debugging Test Failures

#### Verbose Output

```bash
# Run with maximum verbosity
ctest --verbose --output-on-failure

# Run specific failing test
ctest -R "FailingTest" --verbose
```

#### Debug Builds

```bash
# Build in debug mode
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
cmake --build build-debug

# Run with debugger
gdb ./build-debug/tests/unit/failing_test
```

#### Logging and Diagnostics

```cpp
SOLAR_TEST_CASE(DiagnosticTest, "Test with diagnostic output") {
  // Add diagnostic output
  std::cout << "Starting test with configuration: " << config.to_string() << std::endl;

  auto result = perform_operation();

  // Log intermediate results
  std::cout << "Intermediate result: " << result.intermediate_value << std::endl;

  // Add metadata for debugging
  add_metadata("operation_time", std::to_string(result.execution_time));
  add_metadata("memory_usage", std::to_string(result.memory_used));

  assert_equals(expected_value, result.final_value, "Final value should match expected");
}
```

### Getting Help

1. **Check the API documentation**: Refer to the comprehensive API docs
2. **Look at examples**: Study the example tests in `lib/solar_test/examples/`
3. **Review existing tests**: Look at tests in `tests/unit/` and `tests/integration/`
4. **Check CI logs**: Review GitHub Actions logs for CI-specific issues
5. **Use verbose output**: Enable verbose logging to understand what's happening

Remember: Good tests are an investment in code quality and maintainability. Take time to write clear, comprehensive tests that will help you catch bugs early and maintain confidence in your code.
