# Solar System Testing Framework Templates

This document provides ready-to-use templates for common testing scenarios in the Solar System Suite. These templates follow the established standards and conventions and can be adapted for specific use cases.

## Table of Contents

1. [Unit Test Templates](#unit-test-templates)
2. [Integration Test Templates](#integration-test-templates)
3. [Performance Test Templates](#performance-test-templates)
4. [Mock Usage Templates](#mock-usage-templates)
5. [Test Fixture Templates](#test-fixture-templates)
6. [Error Handling Templates](#error-handling-templates)
7. [CMake Integration Templates](#cmake-integration-templates)

## Unit Test Templates

### Basic Unit Test Template

```cpp
// tests/unit/test_component_name.cpp
/**
 * @file test_component_name.cpp
 * @brief Unit tests for ComponentName class
 *
 * Tests cover:
 * - Constructor initialization
 * - Core functionality methods
 * - Error handling and edge cases
 * - Performance characteristics
 */

#include <solar_test/solar_test.hpp>
#include <solar_core/component_name.hpp>

// Basic test case template
SOLAR_TEST_CASE(ComponentName_Method_Scenario_ExpectedBehavior,
                "ComponentName.method() should behave correctly under normal conditions") {
  // Arrange - Set up test data and dependies
  ComponentName component;
  TestData input_data = create_test_data();
  ExpectedResult expected = calculate_expected_result(input_data);

  // Act - Execute the behavior being tested
  auto actual_result = component.method(input_data);

  // Assert - Verify the expected outcome
  assert_equals(expected.value, actual_result.value, "Result value should match expected");
  assert_true(actual_result.is_valid(), "Result should be valid");
  assert_equals(expected.status, actual_result.status, "Status should match expected");
}

// Constructor test template
SOLAR_TEST_CASE(ComponentName_Constructor_ValidParameters_InitializesCorrectly,
                "Constructor should initialize all properties correctly with valid parameters") {
  // Arrange
  std::string name = "TestComponent";
  double value = 42.0;
  bool flag = true;

  // Act
  ComponentName component(name, value, flag);

  // Assert
  assert_equals(name, component.name(), "Name should be set correctly");
  assert_equals(value, component.value(), "Value should be set correctly");
  assert_equals(flag, component.flag(), "Flag should be set correctly");
  assert_true(component.is_initialized(), "Component should be initialized");
}

// Error handling test template
SOLAR_TEST_CASE(ComponentName_Method_InvalidInput_ThrowsException,
                "Method should throw appropriate exception for invalid input") {
  // Arrange
  ComponentName component;
  InvalidInput invalid_input = create_invalid_input();

  // Act & Assert
  assert_throws([&]() {
    component.method(invalid_input);
  }, "Should throw exception for invalid input");
}

// Main function for standalone test executable
SOLAR_TEST_MAIN()
```

### Class with Dependencies Template

```cpp
// tests/unit/test_component_with_dependencies.cpp
#include <solar_test/solar_test.hpp>
#include <solar_core/component_with_dependencies.hpp>
#include <solar_test/mocks/dependency_mock.hpp>

SOLAR_TEST_CASE(ComponentWithDependencies_Method_DependencySuccess_ReturnsExpectedResult,
                "Component should work correctly when dependency succeeds") {
  // Arrange - Set up mocks and dependencies
  auto dependency_mock = std::make_shared<DependencyMock>();
  dependency_mock->set_return_value("expected_response");
  dependency_mock->set_success(true);

  ComponentWithDependencies component(dependency_mock);
  TestInput input = create_test_input();

  // Act
  auto result = component.process(input);

  // Assert - Verify result and mock interactions
  assert_true(result.success(), "Processing should succeed");
  assert_equals("expected_response", result.data(), "Should return expected data");

  // Verify mock interactions
  assert_equals(1, dependency_mock->call_count(), "Should call dependency once");
  assert_equals(input.key(), dependency_mock->last_call_parameter(),
                "Should pass correct parameter to dependency");
}

SOLAR_TEST_CASE(ComponentWithDependencies_Method_DependencyFailure_HandlesGracefully,
                "Component should handle dependency failure gracefully") {
  // Arrange
  auto dependency_mock = std::make_shared<DependencyMock>();
  dependency_mock->set_failure(true);
  dependency_mock->set_error_message("Dependency failed");

  ComponentWithDependencies component(dependency_mock);
  TestInput input = create_test_input();

  // Act
  auto result = component.process(input);

  // Assert
  assert_false(result.success(), "Processing should fail");
  assert_contains(result.error_message(), "Dependency failed",
                  "Error message should mention dependency failure");

  // Verify fallback behavior
  assert_not_equals("", result.fallback_data(), "Should provide fallback data");
}
```

## Integration Test Templates

### End-to-End Workflow Template

```cpp
// tests/integration/test_complete_workflow.cpp
/**
 * @file test_complete_workflow.cpp
 * @brief Integration tests for complete system workflows
 *
 * Tests the complete data flow from input to output through all system components.
 */

#include <solar_test/solar_test.hpp>
#include <solar_jpl/jpl_client.hpp>
#include <solar_jpl/body_factory.hpp>
#include <solar_core/simulation_builder.hpp>
#include <solar_test/framework/test_data_manager.hpp>

SOLAR_TEST_CASE(CompleteWorkflow_JPLToSimulation_ProducesValidResults,
                "Complete workflow from JPL data to simulation should work correctly") {
  // Arrange - Set up complete system
  JPLClient jpl_client;
  BodyFactory body_factory(jpl_client);
  SimulationBuilder simulation_builder;

  std::vector<std::string> body_names = {"Sun", "Earth", "Mars"};

  // Act - Execute complete workflow
  std::vector<std::unique_ptr<CelestialBody>> bodies;

  for (const auto& name : body_names) {
    try {
      auto body = body_factory.create_body(name);
      assert_not_equals(nullptr, body, "Body should be created: " + name);
      bodies.push_back(std::move(body));
    } catch (const std::exception& e) {
      // Skip test if JPL API is unavailable
      skip_test("JPL API unavailable: " + std::string(e.what()));
      return;
    }
  }

  auto simulation = simulation_builder
    .with_bodies(std::move(bodies))
    .with_time_step(3600.0)  // 1 hour
    .with_duration(24 * 30)  // 30 days
    .build();

  auto result = simulation->run();

  // Assert - Verify end-to-end results
  assert_true(result.success(), "Simulation should complete successfully");
  assert_greater_than(result.step_count(), 0, "Should have simulation steps");
  assert_equals(body_names.size(), result.final_state().body_count(),
                "Should have all requested bodies");

  // Verify physical constraints
  auto final_state = result.final_state();
  for (const auto& name : body_names) {
    auto body = final_state.get_body(name);
    assert_greater_than(body.mass(), 0.0, name + " should have positive mass");
    assert_greater_than(body.radius(), 0.0, name + " should have positive radius");
  }
}
```

### Service Integration Template

```cpp
// tests/integration/test_service_integration.cpp
#include <solar_test/solar_test.hpp>
#include <solar_web/web_server.hpp>
#include <solar_core/simulation_engine.hpp>

class WebServiceIntegrationTest : public SolarSystem::Testing::TestCase {
public:
  WebServiceIntegrationTest() : TestCase({
    "WebServiceIntegrationTest",
    "Test web service integration with simulation engine",
    {"integration", "web", "slow"}
  }) {}

  void setup() override {
    // Start services
    simulation_engine_ = std::make_unique<SimulationEngine>();
    simulation_engine_->initialize();

    web_server_ = std::make_unique<WebServer>(test_port_, simulation_engine_.get());
    web_server_->start();

    // Wait for services to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    http_client_ = std::make_unique<HTTPClient>();
  }

  void run() override {
    test_simulation_api_endpoint();
    test_body_data_endpoint();
    test_time_travel_functionality();
    test_error_handling();
  }

  void teardown() override {
    if (web_server_) {
      web_server_->stop();
    }
    simulation_engine_.reset();
    http_client_.reset();
  }

private:
  static constexpr int test_port_ = 8080;
  std::unique_ptr<SimulationEngine> simulation_engine_;
  std::unique_ptr<WebServer> web_server_;
  std::unique_ptr<HTTPClient> http_client_;

  void test_simulation_api_endpoint() {
    auto response = http_client_->get(base_url() + "/api/simulation");

    assert_equals(200, response.status_code, "Simulation API should return 200");
    assert_contains(response.body, "bodies", "Response should contain bodies data");

    auto json_data = parse_json(response.body);
    assert_true(json_data.contains("timestamp"), "Should contain timestamp");
    assert_true(json_data["bodies"].is_array(), "Bodies should be an array");
  }

  void test_body_data_endpoint() {
    auto response = http_client_->get(base_url() + "/api/bodies/Earth");

    assert_equals(200, response.status_code, "Body endpoint should return 200");

    auto json_data = parse_json(response.body);
    assert_equals("Earth", json_data["name"], "Should return Earth data");
    assert_true(json_data.contains("position"), "Should contain position");
    assert_true(json_data.contains("velocity"), "Should contain velocity");
    assert_true(json_data.contains("mass"), "Should contain mass");
  }

  void test_time_travel_functionality() {
    // Set specific date
    auto request_body = R"({"date": "2024-06-01T00:00:00Z"})";
    auto response = http_client_->post(base_url() + "/api/time-travel", request_body);

    assert_equals(200, response.status_code, "Time travel should succeed");

    // Verify simulation updated
    auto sim_response = http_client_->get(base_url() + "/api/simulation");
    auto sim_data = parse_json(sim_response.body);

    assert_contains(sim_data["timestamp"], "2024-06-01",
                    "Simulation should be updated to requested date");
  }

  void test_error_handling() {
    // Test invalid body name
    auto response = http_client_->get(base_url() + "/api/bodies/InvalidPlanet");
    assert_equals(404, response.status_code, "Invalid body should return 404");

    // Test invalid time travel date
    auto invalid_request = R"({"date": "invalid-date"})";
    response = http_client_->post(base_url() + "/api/time-travel", invalid_request);
    assert_equals(400, response.status_code, "Invalid date should return 400");
  }

  std::string base_url() const {
    return "http://localhost:" + std::to_string(test_port_);
  }
};

REGISTER_TEST(WebServiceIntegrationTest);
```

## Performance Test Templates

### Basic Benchmark Template

```cpp
// tests/benchmarks/benchmark_component_performance.cpp
/**
 * @file benchmark_component_performance.cpp
 * @brief Performance benchmarks for ComponentName
 *
 * Benchmarks cover:
 * - Core operation performance
 * - Memory usage characteristics
 * - Scalability with different data sizes
 * - Regression detection
 */

#include <solar_test/solar_test.hpp>
#include <solar_test/benchmarks/benchmark.hpp>
#include <solar_core/component_name.hpp>

SOLAR_BENCHMARK_CASE(ComponentName_CoreOperation_MeetsPerformanceRequirements,
                     "Core operation should meet performance requirements") {
  // Arrange - Set up test environment
  ComponentName component;
  auto test_data = create_performance_test_data(1000);  // Specify data size
  component.initialize(test_data);

  // Warm-up - Initialize caches and JIT compilation
  component.core_operation(test_data.sample());

  // Benchmark - Measure performance
  Benchmark benchmark("ComponentName_CoreOperation");
  benchmark.set_time_threshold(std::chrono::milliseconds(10));  // 10ms requirement
  benchmark.set_memory_threshold(50 * 1024 * 1024);  // 50MB limit
  benchmark.set_operations_per_second_threshold(100);  // 100 ops/sec minimum

  auto result = benchmark.measure([&]() {
    component.core_operation(test_data.sample());
  }, 1000);  // 1000 iterations for statistical accuracy

  // Assert - Verify performance requirements
  assert_less_than(result.mean_time.count(), 10000000,  // 10ms in nanoseconds
                   "Mean execution time should be under 10ms");
  assert_less_than(result.percentile_95.count(), 20000000,  // 20ms in nanoseconds
                   "95th percentile should be under 20ms");
  assert_greater_than(result.operations_per_second, 100,
                      "Should achieve at least 100 operations per second");

  // Log performance metrics
  add_metadata("mean_time_ns", std::to_string(result.mean_time.count()));
  add_metadata("p95_time_ns", std::to_string(result.percentile_95.count()));
  add_metadata("ops_per_second", std::to_string(result.operations_per_second));
  add_metadata("memory_usage_mb", std::to_string(result.memory_usage_bytes / 1024 / 1024));
}
```

### Scalability Benchmark Template

```cpp
SOLAR_BENCHMARK_CASE(ComponentName_Scalability_HandlesLargeDatasets,
                     "Component should scale well with increasing data size") {
  std::vector<int> data_sizes = {100, 500, 1000, 5000, 10000};
  std::vector<double> execution_times;
  std::vector<size_t> memory_usage;

  for (int size : data_sizes) {
    // Create test data of specified size
    ComponentName component;
    auto test_data = create_performance_test_data(size);
    component.initialize(test_data);

    // Benchmark with current data size
    Benchmark benchmark("ComponentName_Scalability_" + std::to_string(size));
    auto result = benchmark.measure([&]() {
      component.process_all_data();
    }, 10);  // Fewer iterations for large datasets

    execution_times.push_back(result.mean_time.count());
    memory_usage.push_back(result.memory_usage_bytes);

    // Log individual results
    add_metadata("time_" + std::to_string(size), std::to_string(result.mean_time.count()));
    add_metadata("memory_" + std::to_string(size), std::to_string(result.memory_usage_bytes));
  }

  // Analyze scalability characteristics
  // For O(n) algorithm, 10x data should take ~10x time
  double time_ratio_100_to_1000 = execution_times[2] / execution_times[0];  // 1000 vs 100
  double expected_linear_ratio = 10.0;

  // Allow some deviation from perfect linear scaling
  assert_less_than(time_ratio_100_to_1000, expected_linear_ratio * 2.0,
                   "Scaling should not be worse than 2x linear");
  assert_greater_than(time_ratio_100_to_1000, expected_linear_ratio * 0.5,
                      "Scaling should not be better than 0.5x linear (sanity check)");

  // Memory usage should scale reasonably
  double memory_ratio = static_cast<double>(memory_usage[2]) / memory_usage[0];
  assert_less_than(memory_ratio, 15.0, "Memory usage should not scale worse than 1.5x linear");
}
```

### Regression Detection Template

```cpp
SOLAR_BENCHMARK_CASE(ComponentName_PerformanceRegression_DetectsSlowdowns,
                     "Performance regression detection for core operations") {
  // Load baseline performance data
  auto baseline = load_performance_baseline("component_baseline.json");

  // Run current benchmark
  ComponentName component;
  auto test_data = create_standard_test_data();
  component.initialize(test_data);

  Benchmark benchmark("ComponentName_Regression");
  auto current_result = benchmark.measure([&]() {
    component.core_operation(test_data.sample());
  }, 1000);

  // Compare against baseline
  double performance_ratio = static_cast<double>(current_result.mean_time.count()) /
                            baseline.mean_time_ns;

  // Allow 10% performance degradation
  assert_less_than(performance_ratio, 1.1,
                   "Performance should not degrade by more than 10%");

  // Update baseline if performance improved significantly (>5%)
  if (performance_ratio < 0.95) {
    save_performance_baseline("component_baseline.json", current_result);
    add_metadata("baseline_updated", "true");
    add_metadata("improvement_percent",
                 std::to_string((1.0 - performance_ratio) * 100));
  }

  // Log comparison results
  add_metadata("baseline_time_ns", std::to_string(baseline.mean_time_ns));
  add_metadata("current_time_ns", std::to_string(current_result.mean_time.count()));
  add_metadata("performance_ratio", std::to_string(performance_ratio));
}
```

## Mock Usage Templates

### Basic Mock Template

```cpp
// tests/unit/test_component_with_mock.cpp
#include <solar_test/solar_test.hpp>
#include <solar_test/mocks/external_service_mock.hpp>
#include <solar_core/component_using_service.hpp>

SOLAR_TEST_CASE(ComponentUsingService_Method_ServiceSuccess_ReturnsExpectedResult,
                "Component should work correctly when external service succeeds") {
  // Arrange - Configure mock
  auto service_mock = std::make_shared<ExternalServiceMock>();
  service_mock->set_response("test_key", "expected_response");
  service_mock->set_delay(std::chrono::milliseconds(10));  // Realistic delay
  service_mock->set_success_rate(1.0);  // Always succeed

  ComponentUsingService component(service_mock);
  TestRequest request = create_test_request("test_key");

  // Act
  auto response = component.process_request(request);

  // Assert - Verify results
  assert_true(response.success(), "Request should succeed");
  assert_equals("expected_response", response.data(), "Should return expected data");
  assert_greater_than(response.processing_time().count(), 10000000,  // 10ms in ns
                      "Should include service delay time");

  // Verify mock interactions
  assert_equals(1, service_mock->call_count(), "Should call service once");
  assert_equals("test_key", service_mock->last_request_key(),
                "Should pass correct key to service");
  assert_true(service_mock->was_method_called("get_data"),
              "Should call get_data method");
}

SOLAR_TEST_CASE(ComponentUsingService_Method_ServiceFailure_HandlesGracefully,
                "Component should handle service failure gracefully") {
  // Arrange - Configure mock for failure
  auto service_mock = std::make_shared<ExternalServiceMock>();
  service_mock->set_failure_mode(true);
  service_mock->set_error_code(500);
  service_mock->set_error_message("Service temporarily unavailable");

  ComponentUsingService component(service_mock);
  TestRequest request = create_test_request("test_key");

  // Act
  auto response = component.process_request(request);

  // Assert - Verify error handling
  assert_false(response.success(), "Request should fail");
  assert_equals(500, response.error_code(), "Should propagate error code");
  assert_contains(response.error_message(), "Service temporarily unavailable",
                  "Should include service error message");

  // Verify fallback behavior
  assert_not_equals("", response.fallback_data(), "Should provide fallback data");
  assert_true(response.used_fallback(), "Should indicate fallback was used");
}
```

### Advanced Mock Template with Multiple Dependencies

```cpp
class MultiDependencyTest : public SolarSystem::Testing::TestCase {
public:
  MultiDependencyTest() : TestCase({
    "MultiDependencyTest",
    "Test component with multiple mocked dependencies",
    {"unit", "mock"}
  }) {}

  void setup() override {
    // Create fresh mocks for each test
    jpl_mock_ = std::make_shared<JPLMock>();
    cache_mock_ = std::make_shared<CacheMock>();
    time_mock_ = std::make_shared<TimeMock>();

    // Set up default mock behaviors
    jpl_mock_->set_default_delay(std::chrono::milliseconds(100));
    cache_mock_->set_cache_exists(true);
    time_mock_->set_current_time(test_start_time_);

    // Create component with mocked dependencies
    component_ = std::make_unique<ComponentWithMultipleDependencies>(
      jpl_mock_, cache_mock_, time_mock_);
  }

  void run() override {
    test_cache_hit_scenario();
    test_cache_miss_scenario();
    test_jpl_failure_with_cache_fallback();
    test_time_dependent_behavior();
  }

  void teardown() override {
    // Clean up
    component_.reset();
    jpl_mock_->reset_call_history();
    cache_mock_->reset_state();
    time_mock_->reset_time();
  }

private:
  std::shared_ptr<JPLMock> jpl_mock_;
  std::shared_ptr<CacheMock> cache_mock_;
  std::shared_ptr<TimeMock> time_mock_;
  std::unique_ptr<ComponentWithMultipleDependencies> component_;

  std::chrono::system_clock::time_point test_start_time_ =
    std::chrono::system_clock::from_time_t(1640995200);  // 2022-01-01

  void test_cache_hit_scenario() {
    // Configure mocks for cache hit
    cache_mock_->set_cache_valid(true);
    cache_mock_->set_cache_data("cached_earth_data");

    auto result = component_->get_body_data("Earth");

    // Verify cache was used, JPL was not called
    assert_true(result.success(), "Should succeed with cached data");
    assert_equals("cached_earth_data", result.data(), "Should return cached data");
    assert_true(cache_mock_->was_cache_read(), "Should read from cache");
    assert_equals(0, jpl_mock_->call_count(), "Should not call JPL API");
  }

  void test_cache_miss_scenario() {
    // Configure mocks for cache miss
    cache_mock_->set_cache_exists(false);
    jpl_mock_->set_response_for_body("Mars", "fresh_mars_data");

    auto result = component_->get_body_data("Mars");

    // Verify JPL was called, cache was updated
    assert_true(result.success(), "Should succeed with fresh data");
    assert_equals("fresh_mars_data", result.data(), "Should return fresh data");
    assert_equals(1, jpl_mock_->call_count(), "Should call JPL API once");
    assert_true(cache_mock_->was_cache_written(), "Should write to cache");
  }

  void test_jpl_failure_with_cache_fallback() {
    // Configure mocks for JPL failure with stale cache
    cache_mock_->set_cache_exists(true);
    cache_mock_->set_cache_valid(false);  // Stale cache
    cache_mock_->set_cache_data("stale_jupiter_data");
    jpl_mock_->simulate_network_failure();

    auto result = component_->get_body_data("Jupiter");

    // Verify fallback to stale cache
    assert_true(result.success(), "Should succeed with fallback");
    assert_equals("stale_jupiter_data", result.data(), "Should return stale data");
    assert_true(result.used_fallback(), "Should indicate fallback was used");
    assert_equals(1, jpl_mock_->call_count(), "Should attempt JPL call");
    assert_true(cache_mock_->was_cache_read(), "Should read stale cache");
  }

  void test_time_dependent_behavior() {
    // Configure time-dependent test
    cache_mock_->set_cache_exists(true);
    cache_mock_->set_cache_timestamp(test_start_time_ - std::chrono::hours(25));  // 25 hours old

    // Advance time to make cache stale
    time_mock_->advance_time(std::chrono::hours(1));

    jpl_mock_->set_response_for_body("Venus", "fresh_venus_data");

    auto result = component_->get_body_data("Venus");

    // Verify time-based cache invalidation
    assert_true(result.success(), "Should succeed");
    assert_equals("fresh_venus_data", result.data(), "Should fetch fresh data");
    assert_equals(1, jpl_mock_->call_count(), "Should call JPL due to stale cache");
  }
};

REGISTER_TEST(MultiDependencyTest);
```

## Test Fixture Templates

### Basic Test Fixture Template

```cpp
// tests/unit/test_simulation_fixture.cpp
#include <solar_test/solar_test.hpp>
#include <solar_core/simulation.hpp>

class SimulationTestFixture : public SolarSystem::Testing::TestCase {
public:
  SimulationTestFixture() : TestCase({
    "SimulationTestFixture",
    "Base fixture for simulation tests",
    {"unit", "simulation"}
  }) {}

protected:
  void setup() override {
    // Create standard test simulation
    simulation_ = std::make_unique<Simulation>();

    // Add standard celestial bodies
    simulation_->add_body(create_test_sun());
    simulation_->add_body(create_test_earth());
    simulation_->add_body(create_test_mars());

    // Set standard configuration
    simulation_->set_time_step(3600.0);  // 1 hour
    simulation_->set_integration_method(IntegrationMethod::RungeKutta4);

    // Record initial state
    initial_energy_ = simulation_->calculate_total_energy();
    initial_momentum_ = simulation_->calculate_total_momentum();
  }

  void teardown() override {
    simulation_.reset();
  }

  // Helper methods for derived tests
  void advance_simulation_days(int days) {
    for (int day = 0; day < days; ++day) {
      for (int hour = 0; hour < 24; ++hour) {
        simulation_->advance_time_step();
      }
    }
  }

  void advance_simulation_hours(int hours) {
    for (int hour = 0; hour < hours; ++hour) {
      simulation_->advance_time_step();
    }
  }

  double get_earth_sun_distance() const {
    auto earth_pos = simulation_->get_body("Earth").position();
    auto sun_pos = simulation_->get_body("Sun").position();
    return (earth_pos - sun_pos).magnitude();
  }

  double get_mars_sun_distance() const {
    auto mars_pos = simulation_->get_body("Mars").position();
    auto sun_pos = simulation_->get_body("Sun").position();
    return (mars_pos - sun_pos).magnitude();
  }

  void verify_energy_conservation(double tolerance = 1e-6) const {
    double current_energy = simulation_->calculate_total_energy();
    double energy_change = std::abs(current_energy - initial_energy_);
    double relative_change = energy_change / std::abs(initial_energy_);

    assert_less_than(relative_change, tolerance,
                     "Energy should be conserved within tolerance");
  }

  void verify_momentum_conservation(double tolerance = 1e-6) const {
    auto current_momentum = simulation_->calculate_total_momentum();
    auto momentum_change = (current_momentum - initial_momentum_).magnitude();
    double relative_change = momentum_change / initial_momentum_.magnitude();

    assert_less_than(relative_change, tolerance,
                     "Momentum should be conserved within tolerance");
  }

  // Test data creation helpers
  std::unique_ptr<CelestialBody> create_test_sun() const {
    return std::make_unique<CelestialBody>(
      "Sun", 1.989e30, 6.96e8,  // Mass, radius
      Vector3(0, 0, 0),         // Position at origin
      Vector3(0, 0, 0)          // Stationary
    );
  }

  std::unique_ptr<CelestialBody> create_test_earth() const {
    return std::make_unique<CelestialBody>(
      "Earth", 5.972e24, 6.371e6,     // Mass, radius
      Vector3(1.496e11, 0, 0),        // 1 AU from Sun
      Vector3(0, 29780, 0)            // Orbital velocity
    );
  }

  std::unique_ptr<CelestialBody> create_test_mars() const {
    return std::make_unique<CelestialBody>(
      "Mars", 6.417e23, 3.390e6,      // Mass, radius
      Vector3(2.279e11, 0, 0),        // 1.52 AU from Sun
      Vector3(0, 24077, 0)            // Orbital velocity
    );
  }

protected:
  std::unique_ptr<Simulation> simulation_;
  double initial_energy_;
  Vector3 initial_momentum_;
};

// Derived test class using the fixture
class OrbitalMechanicsTest : public SimulationTestFixture {
public:
  OrbitalMechanicsTest() : SimulationTestFixture() {
    info_.name = "OrbitalMechanicsTest";
    info_.description = "Test orbital mechanics using simulation fixture";
    info_.tags.push_back("orbital");
  }

  void run() override {
    test_earth_orbital_period();
    test_mars_orbital_period();
    test_energy_conservation();
    test_momentum_conservation();
  }

private:
  void test_earth_orbital_period() {
    double initial_distance = get_earth_sun_distance();

    // Advance one Earth year
    advance_simulation_days(365);

    double final_distance = get_earth_sun_distance();

    // Earth should complete approximately one orbit
    assert_near(initial_distance, final_distance, 1e10,
                "Earth should return to approximately same distance after one year");
  }

  void test_mars_orbital_period() {
    auto initial_mars_pos = simulation_->get_body("Mars").position();

    // Advance one Mars year (687 days)
    advance_simulation_days(687);

    auto final_mars_pos = simulation_->get_body("Mars").position();
    double position_difference = (final_mars_pos - initial_mars_pos).magnitude();

    assert_less_than(position_difference, 1e11,
                     "Mars should return to approximately same position after one Mars year");
  }

  void test_energy_conservation() {
    advance_simulation_days(30);
    verify_energy_conservation(1e-6);
  }

  void test_momentum_conservation() {
    advance_simulation_days(30);
    verify_momentum_conservation(1e-6);
  }
};

REGISTER_TEST(OrbitalMechanicsTest);
```

## Error Handling Templates

### Exception Testing Template

```cpp
// tests/unit/test_error_handling.cpp
#include <solar_test/solar_test.hpp>
#include <solar_core/component_with_errors.hpp>

// Test specific exception types
SOLAR_TEST_CASE(ComponentWithErrors_Method_InvalidInput_ThrowsSpecificException,
                "Method should throw specific exception type for invalid input") {
  // Arrange
  ComponentWithErrors component;
  InvalidInput invalid_input = create_invalid_input();

  // Act & Assert
  try {
    component.risky_method(invalid_input);
    assert_true(false, "Should have thrown InvalidInputException");
  } catch (const InvalidInputException& e) {
    // Verify exception details
    assert_contains(e.what(), "invalid input", "Exception message should mention invalid input");
    assert_equals(ErrorCode::INVALID_INPUT, e.error_code(), "Should have correct error code");
    assert_not_equals("", e.details(), "Should provide error details");
  } catch (const std::exception& e) {
    assert_true(false, "Should have thrown InvalidInputException, not: " + std::string(e.what()));
  } catch (...) {
    assert_true(false, "Should have thrown InvalidInputException, not unknown exception");
  }
}

// Test error recovery
SOLAR_TEST_CASE(ComponentWithErrors_Method_RecoverableError_RetriesAndSucceeds,
                "Method should retry on recoverable errors and eventually succeed") {
  // Arrange
  auto mock_service = std::make_shared<UnreliableServiceMock>();
  mock_service->set_failure_count(2);  // Fail first 2 attempts
  mock_service->set_success_response("success_data");

  ComponentWithErrors component(mock_service);
  TestInput input = create_test_input();

  // Act
  auto result = component.method_with_retry(input);

  // Assert
  assert_true(result.success(), "Should eventually succeed after retries");
  assert_equals("success_data", result.data(), "Should return success data");
  assert_equals(3, mock_service->call_count(), "Should have made 3 attempts");
  assert_equals(2, result.retry_count(), "Should report correct retry count");
}

// Test error boundaries
SOLAR_TEST_CASE(ComponentWithErrors_Method_FatalError_FailsFast,
                "Method should fail fast on fatal errors without retries") {
  // Arrange
  auto mock_service = std::make_shared<UnreliableServiceMock>();
  mock_service->set_fatal_error(true);
  mock_service->set_error_message("Fatal system error");

  ComponentWithErrors component(mock_service);
  TestInput input = create_test_input();

  // Act
  auto result = component.method_with_retry(input);

  // Assert
  assert_false(result.success(), "Should fail on fatal error");
  assert_contains(result.error_message(), "Fatal system error",
                  "Should propagate fatal error message");
  assert_equals(1, mock_service->call_count(), "Should not retry on fatal error");
  assert_equals(0, result.retry_count(), "Should not report retries for fatal error");
}
```

### Resource Management Template

```cpp
// Test resource cleanup
SOLAR_TEST_CASE(ComponentWithResources_Destructor_CleansUpResources,
                "Component destructor should clean up all allocated resources") {
  // Arrange
  auto resource_tracker = std::make_shared<ResourceTracker>();

  {
    // Create component in limited scope
    ComponentWithResources component(resource_tracker);
    component.allocate_resources();

    // Verify resources are allocated
    assert_greater_than(resource_tracker->allocated_count(), 0,
                        "Should have allocated resources");
  }  // Component goes out of scope here

  // Assert - Resources should be cleaned up
  assert_equals(0, resource_tracker->allocated_count(),
                "All resources should be cleaned up after destruction");
  assert_equals(0, resource_tracker->leaked_count(),
                "Should have no resource leaks");
}

// Test exception safety
SOLAR_TEST_CASE(ComponentWithResources_Method_ExceptionDuringOperation_CleansUpResources,
                "Method should clean up resources even when exception is thrown") {
  // Arrange
  auto resource_tracker = std::make_shared<ResourceTracker>();
  ComponentWithResources component(resource_tracker);

  auto failing_operation = []() {
    throw std::runtime_error("Simulated failure");
  };

  // Act & Assert
  assert_throws([&]() {
    component.operation_with_resources(failing_operation);
  }, "Should throw exception from failing operation");

  // Verify cleanup occurred despite exception
  assert_equals(0, resource_tracker->allocated_count(),
                "Resources should be cleaned up despite exception");
  assert_equals(0, resource_tracker->leaked_count(),
                "Should have no resource leaks after exception");
}
```

## CMake Integration Templates

### Basic Test CMakeLists.txt Template

```cmake
# tests/unit/CMakeLists.txt

# Find required packages
find_package(Threads REQUIRED)

# Define test sources
set(UNIT_TEST_SOURCES
    test_celestial_body.cpp
    test_body_factory.cpp
    test_simulation_engine.cpp
    test_cache_manager.cpp
    test_jpl_client.cpp
)

# Create test executables
foreach(TEST_SOURCE ${UNIT_TEST_SOURCES})
    # Extract test name from filename
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # Create executable
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # Link libraries
    target_link_libraries(${TEST_NAME}
        PRIVATE
            solar_test
            solar_core
            solar_jpl
            solar_utils
            Threads::Threads
    )

    # Set properties
    set_target_properties(${TEST_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/unit
    )

    # Add to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    # Set test properties
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "unit;fast"
        TIMEOUT 60
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()

# Create combined test executable (optional)
add_executable(unit_tests_combined ${UNIT_TEST_SOURCES})
target_link_libraries(unit_tests_combined
    PRIVATE
        solar_test
        solar_core
        solar_jpl
        solar_utils
        Threads::Threads
)

add_test(NAME UnitTestsCombined COMMAND unit_tests_combined)
set_tests_properties(UnitTestsCombined PROPERTIES
    LABELS "unit;combined"
    TIMEOUT 300
)
```

### Integration Test CMakeLists.txt Template

```cmake
# tests/integration/CMakeLists.txt

# Integration tests may need additional dependencies
find_package(CURL REQUIRED)

set(INTEGRATION_TEST_SOURCES
    test_jpl_integration.cpp
    test_cache_pipeline.cpp
    test_web_interface.cpp
    test_end_to_end.cpp
)

foreach(TEST_SOURCE ${INTEGRATION_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SOURCE})

    target_link_libraries(${TEST_NAME}
        PRIVATE
            solar_test
            solar_core
            solar_jpl
            solar_utils
            solar_web
            CURL::libcurl
            Threads::Threads
    )

    set_target_properties(${TEST_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/integration
    )

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "integration;slow"
        TIMEOUT 300
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()
```

### Benchmark CMakeLists.txt Template

```cmake
# tests/benchmarks/CMakeLists.txt

set(BENCHMARK_SOURCES
    benchmark_simulation_performance.cpp
    benchmark_cache_performance.cpp
    benchmark_jpl_parsing.cpp
    benchmark_regression_detection.cpp
)

foreach(BENCHMARK_SOURCE ${BENCHMARK_SOURCES})
    get_filename_component(BENCHMARK_NAME ${BENCHMARK_SOURCE} NAME_WE)

    add_executable(${BENCHMARK_NAME} ${BENCHMARK_SOURCE})

    target_link_libraries(${BENCHMARK_NAME}
        PRIVATE
            solar_test
            solar_core
            solar_jpl
            solar_utils
            Threads::Threads
    )

    # Use Release build for benchmarks
    target_compile_definitions(${BENCHMARK_NAME} PRIVATE NDEBUG)
    target_compile_options(${BENCHMARK_NAME} PRIVATE -O3 -march=native)

    set_target_properties(${BENCHMARK_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/benchmarks
    )

    add_test(NAME ${BENCHMARK_NAME} COMMAND ${BENCHMARK_NAME})

    set_tests_properties(${BENCHMARK_NAME} PROPERTIES
        LABELS "benchmark;performance"
        TIMEOUT 600
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()

# Create benchmark results directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/benchmark_results)
```

### Root Test CMakeLists.txt Template

```cmake
# tests/CMakeLists.txt

# Enable testing
enable_testing()

# Set test output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests)

# Add test subdirectories
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(benchmarks)
add_subdirectory(utils)

# Create custom test targets
add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} -L "unit" --output-on-failure
    DEPENDS ${UNIT_TEST_TARGETS}
    COMMENT "Running unit tests"
)

add_custom_target(test-integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L "integration" --output-on-failure
    DEPENDS ${INTEGRATION_TEST_TARGETS}
    COMMENT "Running integration tests"
)

add_custom_target(test-benchmarks
    COMMAND ${CMAKE_CTEST_COMMAND} -L "benchmark" --output-on-failure
    DEPENDS ${BENCHMARK_TARGETS}
    COMMENT "Running performance benchmarks"
)

add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${ALL_TEST_TARGETS}
    COMMENT "Running all tests"
)

# Test data setup
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/test_config.json.in
    ${CMAKE_BINARY_DIR}/tests/data/test_config.json
    @ONLY
)

# Copy test data
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/
     DESTINATION ${CMAKE_BINARY_DIR}/tests/data/
     PATTERN "*.in" EXCLUDE
)
```

These templates provide a solid foundation for creating consistent, well-structured tests across the Solar System Suite project. They can be adapted and extended based on specific testing needs while maintaining the established standards and conventions.
