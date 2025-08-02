# Solar System Testing Framework Examples

This document provides comprehensive examples of different types of tests using the Solar System Testing Framework.

## Table of Contents

1. [Unit Test Examples](#unit-test-examples)
2. [Integration Test Examples](#integration-test-examples)
3. [Performance Test Examples](#performance-test-examples)
4. [Mock Usage Examples](#mock-usage-examples)
5. [Advanced Testing Patterns](#advanced-testing-patterns)
6. [CI/CD Integration Examples](#cicd-integration-examples)

## Unit Test Examples

### Basic Component Testing

```cpp
// tests/unit/test_celestial_body.cpp
#include <solar_test/solar_test.hpp>
#include <solar_core/celestial_body.hpp>

SOLAR_TEST_CASE(CelestialBody_Constructor_InitializesCorrectly,
                "CelestialBody constructor should initialize all properties correctly") {
  // Arrange
  std::string name = "Earth";
  double mass = 5.972e24;  // kg
  double radius = 6.371e6; // meters
  Vector3 position(1.496e11, 0, 0); // 1 AU from origin
  Vector3 velocity(0, 29780, 0);    // Earth's orbital velocity

  // Act
  CelestialBody earth(name, mass, radius, position, velocity);

  // Assert
  assert_equals(name, earth.name(), "Name should be set correctly");
  assert_equals(mass, earth.mass(), "Mass should be set correctly");
  assert_equals(radius, earth.radius(), "Radius should be set correctly");
  assert_equals(position, earth.position(), "Position should be set correctly");
  assert_equals(velocity, earth.velocity(), "Velocity should be set correctly");
}

SOLAR_TEST_CASE(CelestialBody_CalculateGravitationalForce_ReturnsCorrectForce,
                "Should calculate gravitational force correctly between two bodies") {
  // Arrange
  CelestialBody earth("Earth", 5.972e24, 6.371e6, Vector3(0, 0, 0), Vector3(0, 0, 0));
  CelestialBody moon("Moon", 7.342e22, 1.737e6, Vector3(3.844e8, 0, 0), Vector3(0, 0, 0));

  // Act
  Vector3 force = earth.calculate_gravitational_force(moon);

  // Assert
  double expected_magnitude = 1.982e20; // Newton's law of gravitation
  assert_near(expected_magnitude, force.magnitude(), 1e18,
              "Gravitational force magnitude should be approximately correct");

  // Force should point toward the moon (positive x direction)
  assert_greater_than(force.x(), 0, "Force should point toward moon");
  assert_near(0.0, force.y(), 1e10, "Y component should be near zero");
  assert_near(0.0, force.z(), 1e10, "Z component should be near zero");
}

SOLAR_TEST_CASE(CelestialBody_UpdatePosition_IntegratesCorrectly,
                "Should update position using velocity integration") {
  // Arrange
  Vector3 initial_position(1000, 2000, 3000);
  Vector3 velocity(10, 20, 30); // m/s
  CelestialBody body("TestBody", 1000, 100, initial_position, velocity);
  double time_step = 1.0; // 1 second

  // Act
  body.update_position(time_step);

  // Assert
  Vector3 expected_position = initial_position + velocity * time_step;
  assert_equals(expected_position, body.position(),
                "Position should be updated using velocity integration");
}
```

### Error Handling Tests

```cpp
// tests/unit/test_body_factory_errors.cpp
#include <solar_test/solar_test.hpp>
#include <solar_jpl/body_factory.hpp>
#include <solar_test/mocks/jpl_mock.hpp>

SOLAR_TEST_CASE(BodyFactory_InvalidBodyName_ThrowsException,
                "BodyFactory should throw exception for invalid body names") {
  // Arrange
  auto jpl_mock = std::make_shared<JPLMock>();
  jpl_mock->set_error_response(404, "Body not found");
  BodyFactory factory(jpl_mock);

  // Act & Assert
  assert_throws([&]() {
    factory.create_body("InvalidPlanet");
  }, "Should throw exception for invalid body name");
}

SOLAR_TEST_CASE(BodyFactory_NetworkTimeout_HandlesGracefully,
                "BodyFactory should handle network timeouts gracefully") {
  // Arrange
  auto jpl_mock = std::make_shared<JPLMock>();
  jpl_mock->simulate_timeout();
  BodyFactory factory(jpl_mock);

  // Act & Assert
  try {
    factory.create_body("Mars");
    assert_true(false, "Should have thrown timeout exception");
  } catch (const NetworkTimeoutException& e) {
    assert_contains(e.what(), "timeout", "Exception should mention timeout");
  } catch (...) {
    assert_true(false, "Should have thrown NetworkTimeoutException");
  }
}

SOLAR_TEST_CASE(BodyFactory_MalformedJPLResponse_HandlesGracefully,
                "BodyFactory should handle malformed JPL responses") {
  // Arrange
  auto jpl_mock = std::make_shared<JPLMock>();
  jpl_mock->set_response_for_body("Mars", "invalid json data {{{");
  BodyFactory factory(jpl_mock);

  // Act & Assert
  assert_throws([&]() {
    factory.create_body("Mars");
  }, "Should throw exception for malformed JSON");
}
```

### Boundary Value Testing

```cpp
// tests/unit/test_simulation_boundaries.cpp
#include <solar_test/solar_test.hpp>
#include <solar_core/simulation.hpp>

SOLAR_TEST_CASE(Simulation_ZeroTimeStep_HandlesCorrectly,
                "Simulation should handle zero time step correctly") {
  // Arrange
  Simulation simulation;
  simulation.add_body(create_test_earth());

  // Act & Assert
  assert_no_throw([&]() {
    simulation.advance_time_step(0.0);
  }, "Zero time step should not throw exception");

  // Position should not change
  auto initial_position = simulation.get_body("Earth").position();
  simulation.advance_time_step(0.0);
  auto final_position = simulation.get_body("Earth").position();

  assert_equals(initial_position, final_position,
                "Position should not change with zero time step");
}

SOLAR_TEST_CASE(Simulation_VeryLargeTimeStep_RemainsStable,
                "Simulation should remain stable with large time steps") {
  // Arrange
  Simulation simulation;
  simulation.add_body(create_test_earth());
  simulation.add_body(create_test_sun());

  double large_time_step = 86400.0; // 1 day in seconds

  // Act
  Vector3 initial_position = simulation.get_body("Earth").position();

  for (int i = 0; i < 365; ++i) { // 1 year
    simulation.advance_time_step(large_time_step);
  }

  Vector3 final_position = simulation.get_body("Earth").position();

  // Assert
  // Earth should complete approximately one orbit
  double distance_from_sun = (final_position - simulation.get_body("Sun").position()).magnitude();
  double expected_orbital_radius = 1.496e11; // 1 AU

  assert_near(expected_orbital_radius, distance_from_sun, 1e10,
              "Earth should maintain approximately circular orbit");
}
```

## Integration Test Examples

### End-to-End Data Pipeline

```cpp
// tests/integration/test_jpl_to_simulation_pipeline.cpp
#include <solar_test/solar_test.hpp>
#include <solar_jpl/jpl_client.hpp>
#include <solar_jpl/body_factory.hpp>
#include <solar_core/simulation_builder.hpp>

SOLAR_TEST_CASE(JPLToSimulationPipeline_CompleteWorkflow_ProducesValidResults,
                "Complete pipeline from JPL data to simulation should work correctly") {
  // Arrange
  JPLClient jpl_client;
  BodyFactory body_factory(jpl_client);
  SimulationBuilder builder;

  // Act - Create solar system from JPL data
  std::vector<std::string> body_names = {"Sun", "Earth", "Mars", "Jupiter"};
  std::vector<std::unique_ptr<CelestialBody>> bodies;

  for (const auto& name : body_names) {
    try {
      auto body = body_factory.create_body(name);
      assert_not_equals(nullptr, body, "Body should be created: " + name);
      bodies.push_back(std::move(body));
    } catch (const std::exception& e) {
      // If JPL API is unavailable, skip this test
      skip_test("JPL API unavailable: " + std::string(e.what()));
      return;
    }
  }

  // Build simulation
  auto simulation = builder.with_bodies(std::move(bodies))
                           .with_time_step(3600.0) // 1 hour
                           .with_integration_method(IntegrationMethod::RungeKutta4)
                           .build();

  // Run simulation for 30 days
  for (int day = 0; day < 30; ++day) {
    for (int hour = 0; hour < 24; ++hour) {
      simulation->advance_time_step();
    }
  }

  // Assert - Verify simulation results
  auto earth = simulation->get_body("Earth");
  auto mars = simulation->get_body("Mars");
  auto jupiter = simulation->get_body("Jupiter");

  // Earth should be roughly 1 AU from Sun
  auto sun_position = simulation->get_body("Sun").position();
  double earth_sun_distance = (earth.position() - sun_position).magnitude();
  assert_near(1.496e11, earth_sun_distance, 1e10,
              "Earth should be approximately 1 AU from Sun");

  // Mars should be farther than Earth
  double mars_sun_distance = (mars.position() - sun_position).magnitude();
  assert_greater_than(mars_sun_distance, earth_sun_distance,
                      "Mars should be farther from Sun than Earth");

  // Jupiter should be farther than Mars
  double jupiter_sun_distance = (jupiter.position() - sun_position).magnitude();
  assert_greater_than(jupiter_sun_distance, mars_sun_distance,
                      "Jupiter should be farther from Sun than Mars");
}
```

### Cache System Integration

```cpp
// tests/integration/test_cache_integration.cpp
#include <solar_test/solar_test.hpp>
#include <solar_jpl/cache_manager.hpp>
#include <solar_test/framework/test_data_manager.hpp>

SOLAR_TEST_CASE(CacheSystem_FullWorkflow_WorksCorrectly,
                "Complete cache workflow should work from empty to populated") {
  // Arrange
  auto temp_dir = TestDataManager::create_test_environment();
  std::string cache_path = temp_dir->path() + "/ephemeris_cache.bin";

  CacheManager cache_manager(cache_path);

  // Act & Assert - Initially no cache
  assert_false(cache_manager.cache_exists(), "Cache should not exist initially");

  // Create and populate cache
  EphemerisData test_data = create_test_ephemeris_data();
  cache_manager.save_ephemeris_data(test_data);

  assert_true(cache_manager.cache_exists(), "Cache should exist after saving");
  assert_true(cache_manager.is_cache_valid(), "Cache should be valid after saving");

  // Load from cache
  auto loaded_data = cache_manager.load_ephemeris_data();

  assert_equals(test_data.body_count(), loaded_data.body_count(),
                "Loaded data should have same body count");
  assert_equals(test_data.time_range(), loaded_data.time_range(),
                "Loaded data should have same time range");

  // Verify cache performance
  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 100; ++i) {
    auto data = cache_manager.load_ephemeris_data();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Cache loading should be very fast (< 10ms for 100 loads)
  assert_less_than(duration.count(), 10, "Cache loading should be very fast");
}

SOLAR_TEST_CASE(CacheSystem_CorruptionRecovery_HandlesGracefully,
                "Cache system shoecover from corruption gracefully") {
  // Arrange
  auto temp_dir = TestDataManager::create_test_environment();
  std::string cache_path = temp_dir->path() + "/corrupted_cache.bin";

  // Create corrupted cache file
  std::ofstream corrupted_file(cache_path, std::ios::binary);
  corrupted_file << "corrupted data that is not valid cache format";
  corrupted_file.close();

  CacheManager cache_manager(cache_path);

  // Act & Assert
  assert_true(cache_manager.cache_exists(), "Corrupted cache file should exist");
  assert_false(cache_manager.is_cache_valid(), "Corrupted cache should not be valid");

  // Should handle corruption gracefully
  assert_throws([&]() {
    cache_manager.load_ephemeris_data();
  }, "Should throw exception for corrupted cache");

  // Should be able to recreate cache
  EphemerisData new_data = create_test_ephemeris_data();
  assert_no_throw([&]() {
    cache_manager.save_ephemeris_data(new_data);
  }, "Should be able to recreate cache after corruption");

  assert_true(cache_manager.is_cache_valid(), "Recreated cache should be valid");
}
```

### Web Interface Integration

```cpp
// tests/integration/test_web_interface.cpp
#include <solar_test/solar_test.hpp>
#include <solar_web/web_server.hpp>
#include <solar_test/mocks/network_mock.hpp>

class WebIntegrationTest : public SolarSystem::Testing::TestCase {
public:
  WebIntegrationTest() : TestCase({"WebIntegrationTest",
                                   "Test web interface integration",
                                   {"integration", "web"}}) {}

  void setup() override {
    // Start web server on test port
    web_server_ = std::make_unique<WebServer>(test_port_);
    web_server_->start();

    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  void run() override {
    // Test API endpoints
    test_simulation_endpoint();
    test_body_data_endpoint();
    test_time_travel_endpoint();
    test_static_file_serving();
  }

  void teardown() override {
    if (web_server_) {
      web_server_->stop();
    }
  }

private:
  std::unique_ptr<WebServer> web_server_;
  int test_port_ = 8080;

  void test_simulation_endpoint() {
    HTTPClient client;
    auto response = client.get("http://localhost:" + std::to_string(test_port_) + "/api/simulation");

    assert_equals(200, response.status_code, "Simulation endpoint should return 200");
    assert_contains(response.body, "bodies", "Response should contain bodies data");

    // Verify JSON structure
    auto json_data = parse_json(response.body);
    assert_true(json_data.contains("bodies"), "JSON should contain bodies array");
    assert_true(json_data.contains("timestamp"), "JSON should contain timestamp");
  }

  void test_body_data_endpoint() {
    HTTPClient client;
    auto response = client.get("http://localhost:" + std::to_string(test_port_) + "/api/bodies/Earth");

    assert_equals(200, response.status_code, "Body data endpoint should return 200");

    auto json_data = parse_json(response.body);
    assert_equals("Earth", json_data["name"], "Should return Earth data");
    assert_true(json_data.contains("position"), "Should contain position data");
    assert_true(json_data.contains("velocity"), "Should contain velocity data");
  }

  void test_time_travel_endpoint() {
    HTTPClient client;

    // Test time travel to specific date
    auto response = client.post("http://localhost:" + std::to_string(test_port_) + "/api/time-travel",
                               R"({"date": "2024-06-01T00:00:00Z"})");

    assert_equals(200, response.status_code, "Time travel should succeed");

    // Verify simulation updated
    auto sim_response = client.get("http://localhost:" + std::to_string(test_port_) + "/api/simulation");
    auto sim_data = parse_json(sim_response.body);

    assert_contains(sim_data["timestamp"], "2024-06-01",
                    "Simulation should be updated to requested date");
  }

  void test_static_file_serving() {
    HTTPClient client;
    auto response = client.get("http://localhost:" + std::to_string(test_port_) + "/");

    assert_equals(200, response.status_code, "Index page should be served");
    assert_contains(response.body, "<html>", "Should serve HTML content");
    assert_contains(response.body, "Solar System", "Should contain page title");
  }
};

REGISTER_TEST(WebIntegrationTest);
```

## Performance Test Examples

### Simulation Performance Benchmarks

```cpp
// tests/benchmarks/benchmark_simulation_performance.cpp
#include <solar_test/solar_test.hpp>
#include <solar_test/benchmarks/benchmark.hpp>
#include <solar_core/simulation.hpp>

SOLAR_BENCHMARK_CASE(SimulationStep_Performance_MeetsRequirements,
                     "Simulation step should complete within microsecond requirements") {
  // Arrange
  Simulation simulation;
  simulation.add_body(create_test_sun());
  simulation.add_body(create_test_earth());
  simulation.add_body(create_test_mars());
  simulation.add_body(create_test_jupiter());

  // Benchmark single step performance
  Benchmark step_benchmark("SimulationStep");
  step_benchmark.set_time_threshold(std::chrono::microseconds(1000)); // 1ms threshold

  auto result = step_benchmark.measure([&]() {
    simulation.advance_time_step(3600.0); // 1 hour step
  }, 10000); // 10,000 iterations for statistical accuracy

  // Assert performance requirements
  assert_less_than(result.mean_time.count(), 1000000, // 1ms in nanoseconds
                   "Mean step time should be under 1ms");
  assert_less_than(result.percentile_95.count(), 2000000, // 2ms in nanoseconds
                   "95th percentile should be under 2ms");
  assert_greater_than(result.operations_per_second, 1000,
                      "Should achieve at least 1000 steps per second");

  // Log performance metrics
  add_metadata("mean_time_ns", std::to_string(result.mean_time.count()));
  add_metadata("ops_per_second", std::to_string(result.operations_per_second));
  add_metadata("memory_usage_mb", std::to_string(result.memory_usage_bytes / 1024 / 1024));
}

SOLAR_BENCHMARK_CASE(SimulationScalability_LargeBodiesCount_PerformsWell,
                     "Simulation should scale well with large numbers of bodies") {
  std::vector<int> body_counts = {10, 50, 100, 500, 1000};
  std::vector<double> performance_results;

  for (int body_count : body_counts) {
    // Create simulation with specified number of bodies
    Simulation simulation;
    for (int i = 0; i < body_count; ++i) {
      simulation.add_body(create_random_test_body("Body" + std::to_string(i)));
    }

    // Benchmark performance
    Benchmark scalability_benchmark("Scalability_" + std::to_string(body_count));
    auto result = scalability_benchmark.measure([&]() {
      simulation.advance_time_step(3600.0);
    }, 100); // Fewer iterations for large simulations

    performance_results.push_back(result.mean_time.count());

    // Log results
    add_metadata("bodies_" + std::to_string(body_count) + "_time_ns",
                 std::to_string(result.mean_time.count()));
  }

  // Analyze scalability
  // Performance should scale roughly as O(n²) for n-body simulation
  double ratio_10_to_100 = performance_results[2] / performance_results[0]; // 100 vs 10 bodies
  double expected_ratio = (100.0 * 100.0) / (10.0 * 10.0); // O(n²) scaling

  // Allow for some deviation from perfect O(n²) scaling
  assert_less_than(ratio_10_to_100, expected_ratio * 1.5,
                   "Scaling should not be worse than 1.5x O(n²)");
  assert_greater_than(ratio_10_to_100, expected_ratio * 0.5,
                      "Scaling should not be better than 0.5x O(n²) (sanity check)");
}
```

### Cache Performance Benchmarks

```cpp
// tests/benchmarks/benchmark_cache_performance.cpp
#include <solar_test/solar_test.hpp>
#include <solar_test/benchmarks/benchmark.hpp>
#include <solar_jpl/cache_manager.hpp>

SOLAR_BENCHMARK_CASE(CacheLoading_Performance_Meets1000xImprovement,
                     "Cache loading should be 1000x faster than JPL API") {
  // Arrange
  auto temp_dir = TestDataManager::create_test_environment();
  std::string cache_path = temp_dir->path() + "/performance_cache.bin";

  // Create large test dataset
  EphemerisData large_dataset = create_large_test_dataset(1000); // 1000 bodies

  CacheManager cache_manager(cache_path);
  cache_manager.save_ephemeris_data(large_dataset);

  // Benchmark cache loading
  Benchmark cache_benchmark("CacheLoading");
  auto cache_result = cache_benchmark.measure([&]() {
    auto data = cache_manager.load_ephemeris_data();
    assert_equals(1000, data.body_count(), "Should load all bodies");
  }, 1000);

  // Simulate JPL API loading time (much slower)
  Benchmark jpl_benchmark("JPLLoading");
  auto jpl_result = jpl_benchmark.measure([&]() {
    // Simulate JPL API delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto data = create_large_test_dataset(1000);
  }, 10); // Fewer iterations due to slowness

  // Assert 1000x improvement
  double improvement_ratio = static_cast<double>(jpl_result.mean_time.count()) /
                            cache_result.mean_time.count();

  assert_greater_than(improvement_ratio, 1000.0,
                      "Cache should be at least 1000x faster than JPL API");

  // Additional performance requirements
  assert_less_than(cache_result.mean_time.count(), 1000000, // 1ms
                   "Cache loading should be under 1ms");

  // Log performance metrics
  add_metadata("cache_time_ns", std::to_string(cache_result.mean_time.count()));
  add_metadata("jpl_time_ns", std::to_string(jpl_result.mean_time.count()));
  add_metadata("improvement_ratio", std::to_string(improvement_ratio));
}

SOLAR_BENCHMARK_CASE(CacheMemoryUsage_LargeDatasets_RemainsEfficient,
                     "Cache should use memory efficiently for large datasets") {
  // Test memory usage with various dataset sizes
  std::vector<int> dataset_sizes = {100, 500, 1000, 5000};

  for (int size : dataset_sizes) {
    auto temp_dir = TestDataManager::create_test_environment();
    std::string cache_path = temp_dir->path() + "/memory_test_" + std::to_string(size) + ".bin";

    // Measure memory usage during cache operations
    assert_memory_usage_less_than([&]() {
      EphemerisData dataset = create_large_test_dataset(size);
      CacheManager cache_manager(cache_path);

      // Save and load multiple times
      for (int i = 0; i < 10; ++i) {
        cache_manager.save_ephemeris_data(dataset);
        auto loaded_data = cache_manager.load_ephemeris_data();
      }
    }, size * 1024 * 10); // 10KB per body maximum

    add_metadata("memory_usage_" + std::to_string(size) + "_bodies",
                 std::to_string(get_current_memory_usage()));
  }
}
```

### Regression Detection

```cpp
// tests/benchmarks/benchmark_regression_detection.cpp
#include <solar_test/solar_test.hpp>
#include <solar_test/benchmarks/regression_detector.hpp>

SOLAR_BENCHMARK_CASE(PerformanceRegression_Detection_WorksCorrectly,
                     "Performance regression detection should identify degradations") {
  // Load historical performance baselines
  RegressionDetector detector("performance_baselines.json");

  // Run current performance tests
  std::map<std::string, BenchmarkResult> current_results;

  // Cache loading benchmark
  {
    Benchmark cache_benchmark("CacheLoading");
    auto result = cache_benchmark.measure([&]() {
      load_test_cache();
    }, 1000);
    current_results["CacheLoading"] = result;
  }

  // Simulation step benchmark
  {
    Benchmark sim_benchmark("SimulationStep");
    auto result = sim_benchmark.measure([&]() {
      run_single_simulation_step();
    }, 10000);
    current_results["SimulationStep"] = result;
  }

  // JPL parsing benchmark
  {
    Benchmark jpl_benchmark("JPLParsing");
    auto result = jpl_benchmark.measure([&]() {
      parse_jpl_response(sample_jpl_response_);
    }, 1000);
    current_results["JPLParsing"] = result;
  }

  // Detect regressions
  auto regression_report = detector.detect_regressions(current_results);

  // Assert no significant regressions
  for (const auto& regression : regression_report.regressions) {
    assert_less_than(regression.degradation_percentage, 10.0,
                     "Performance degradation should be less than 10% for " + regression.benchmark_name);
  }

  // Update baselines if performance improved
  for (const auto& improvement : regression_report.improvements) {
    if (improvement.improvement_percentage > 5.0) {
      detector.update_baseline(improvement.benchmark_name, current_results[improvement.benchmark_name]);
      add_metadata("updated_baseline_" + improvement.benchmark_name, "true");
    }
  }

  // Save regression report
  detector.save_report(regression_report, "regression_report.json");
}
```

## Mock Usage Examples

### Comprehensive JPL Mock Usage

```cpp
// tests/unit/test_jpl_mock_comprehensive.cpp
#include <solar_test/solar_test.hpp>
#include <solar_test/mocks/jpl_mock.hpp>

SOLAR_TEST_CASE(JPLMock_ComprehensiveUsage_WorksCorrectly,
                "JPL mock should support all common usage patterns") {
  // Arrange
  JPLMock::MockConfiguration config;
  config.simulate_network_delays = true;
  config.response_delay = std::chrono::milliseconds(50);
  config.failure_rate = 0.1; // 10% failure rate
  config.mock_data_path = "tests/data/jpl_responses/";

  auto jpl_mock = std::make_shared<JPLMock>(config);

  // Set up various response scenarios
  jpl_mock->set_response_for_body("Earth", load_test_data("earth_response.json"));
  jpl_mock->set_response_for_body("Mars", load_test_data("mars_response.json"));
  jpl_mock->set_error_response(500, "Internal Server Error");

  // Test successful requests
  BodyFactory factory(jpl_mock);

  auto earth = factory.create_body("Earth");
  assert_not_equals(nullptr, earth, "Earth should be created");
  assert_equals("Earth", earth->name(), "Name should be correct");

  auto mars = factory.create_body("Mars");
  assert_not_equals(nullptr, mars, "Mars should be created");
  assert_equals("Mars", mars->name(), "Name should be correct");

  // Verify mock interactions
  assert_equals(2, jpl_mock->call_count(), "Should have made 2 API calls");

  auto requested_bodies = jpl_mock->requested_bodies();
  assert_contains(requested_bodies, "Earth", "Should have requested Earth");
  assert_contains(requested_bodies, "Mars", "Should have requested Mars");

  // Test error scenarios
  jpl_mock->simulate_network_failure();
  assert_throws([&]() {
    factory.create_body("Jupiter");
  }, "Should throw exception for network failure");

  // Test timeout scenarios
  jpl_mock->simulate_timeout();
  assert_throws([&]() {
    factory.create_body("Saturn");
  }, "Should throw exception for timeout");

  // Reset and test again
  jpl_mock->reset_call_history();
  assert_equals(0, jpl_mock->call_count(), "Call count should be reset");
}
```

### Service Registry Mock Pattern

```cpp
// tests/unit/test_service_registry_mock.cpp
#include <solar_test/solar_test.hpp>
#include <solar_test/mocks/mock_service_registry.hpp>

SOLAR_TEST_CASE(ServiceRegistryMock_DependencyInjection_WorksCorrectly,
                "Mock service registry should enable dependency injection testing") {
  // Arrange
  auto service_registry = std::make_shared<MockServiceRegistry>();

  // Register mock services
  auto jpl_mock = std::make_shared<JPLMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  auto time_mock = std::make_shared<TimeMock>();

  service_registry->register_service<IJPLClient>(jpl_mock);
  service_registry->register_service<ICacheManager>(cache_mock);
  service_registry->register_service<ITimeProvider>(time_mock);

  // Configure mocks
  jpl_mock->set_response_for_body("Earth", earth_test_data_);
  cache_mock->set_cache_exists(false); // Force JPL API usage
  time_mock->set_current_time(test_time_);

  // Act - Use service registry in application code
  Application app(service_registry);
  auto simulation_result = app.create_simulation({"Earth"});

  // Assert
  assert_true(simulation_result.success(), "Simulation creation should succeed");

  // Verify service interactions
  assert_equals(1, jpl_mock->call_count(), "Should have called JPL API");
  assert_true(cache_mock->was_cache_read(), "Should have checked cache");
  assert_equals(test_time_, time_mock->now(), "Should have used mock time");
}
```

## Advanced Testing Patterns

### Test Fixtures and Data Builders

```cpp
// tests/unit/test_simulation_fixtures.cpp
#include <solar_test/solar_test.hpp>

class SimulationTestFixture : public SolarSystem::Testing::TestCase {
public:
  SimulationTestFixture() : TestCase({"SimulationTestFixture", "Base fixture for simulation tests"}) {}

protected:
  void setup() override {
    // Create standard test simulation
    simulation_ = std::make_unique<Simulation>();

    // Add standard celestial bodies
    simulation_->add_body(create_test_sun());
    simulation_->add_body(create_test_earth());
    simulation_->add_body(create_test_mars());

    // Set standard configuration
    simulation_->set_time_step(3600.0); // 1 hour
    simulation_->set_integration_method(IntegrationMethod::RungeKutta4);
  }

  void teardown() override {
    simulation_.reset();
  }

  // Helper methods for tests
  void advance_simulation_days(int days) {
    for (int day = 0; day < days; ++day) {
      for (int hour = 0; hour < 24; ++hour) {
        simulation_->advance_time_step();
      }
    }
  }

  double get_earth_sun_distance() {
    auto earth_pos = simulation_->get_body("Earth").position();
    auto sun_pos = simulation_->get_body("Sun").position();
    return (earth_pos - sun_pos).magnitude();
  }

  std::unique_ptr<Simulation> simulation_;
};

class OrbitalMechanicsTest : public SimulationTestFixture {
public:
  OrbitalMechanicsTest() : SimulationTestFixture() {
    info_.name = "OrbitalMechanicsTest";
    info_.description = "Test orbital mechanics calculations";
    info_.tags = {"unit", "physics", "orbital"};
  }

  void run() override {
    test_earth_orbital_period();
    test_mars_orbital_period();
    test_energy_conservation();
  }

private:
  void test_earth_orbital_period() {
    double initial_distance = get_earth_sun_distance();

    // Advance one Earth year (365 days)
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

    // Mars should complete approximately one orbit
    assert_less_than(position_difference, 1e11,
                     "Mars should return to approximately same position after one Mars year");
  }

  void test_energy_conservation() {
    double initial_energy = simulation_->calculate_total_energy();

    // Run simulation for 30 days
    advance_simulation_days(30);

    double final_energy = simulation_->calculate_total_energy();
    double energy_change = std::abs(final_energy - initial_energy);
    double relative_change = energy_change / std::abs(initial_energy);

    // Energy should be conserved (within numerical precision)
    assert_less_than(relative_change, 1e-6,
                     "Total energy should be conserved within numerical precision");
  }
};

REGISTER_TEST(OrbitalMechanicsTest);
```

### Parameterized Tests

```cpp
// tests/unit/test_parameterized_body_creation.cpp
#include <solar_test/solar_test.hpp>

struct BodyTestData {
  std::string name;
  double expected_mass;
  double expected_radius;
  std::string jpl_response_file;
};

class ParameterizedBodyTest : public SolarSystem::Testing::TestCase {
public:
  ParameterizedBodyTest() : TestCase({"ParameterizedBodyTest", "Test body creation with various parameters"}) {}

  void run() override {
    std::vector<BodyTestData> test_cases = {
      {"Earth", 5.972e24, 6.371e6, "earth_response.json"},
      {"Mars", 6.417e23, 3.390e6, "mars_response.json"},
      {"Jupiter", 1.898e27, 6.991e7, "jupiter_response.json"},
      {"Moon", 7.342e22, 1.737e6, "moon_response.json"}
    };

    for (const auto& test_case : test_cases) {
      test_body_creation(test_case);
    }
  }

private:
  void test_body_creation(const BodyTestData& test_data) {
    // Arrange
    auto jpl_mock = std::make_shared<JPLMock>();
    jpl_mock->set_response_for_body(test_data.name,
                                   load_test_data(test_data.jpl_response_file));

    BodyFactory factory(jpl_mock);

    // Act
    auto body = factory.create_body(test_data.name);

    // Assert
    assert_not_equals(nullptr, body, "Body should be created: " + test_data.name);
    assert_equals(test_data.name, body->name(), "Name should match: " + test_data.name);
    assert_near(test_data.expected_mass, body->mass(), test_data.expected_mass * 0.01,
                "Mass should be approximately correct for " + test_data.name);
    assert_near(test_data.expected_radius, body->radius(), test_data.expected_radius * 0.01,
                "Radius should be approximately correct for " + test_data.name);
  }
};

REGISTER_TEST(ParameterizedBodyTest);
```

## CI/CD Integration Examples

### GitHub Actions Workflow

```yaml
# .github/workflows/comprehensive-testing.yml
name: Comprehensive Test Suite

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc-9, gcc-10, clang-10, clang-11]
        build-type: [Debug, Release]

    steps:
    - uses: actions/checkout@v3

    - name: Install Dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake libcurl4-openssl-dev

    - name: Setup Compiler
      run: |
        if [[ "${{ matrix.compiler }}" == gcc-* ]]; then
          sudo apt-get install -y ${{ matrix.compiler }}
          echo "CC=gcc-${compiler#gcc-}" >> $GITHUB_ENV
          echo "CXX=g++-${compiler#gcc-}" >> $GITHUB_ENV
        else
          sudo apt-get install -y ${{ matrix.compiler }}
          echo "CC=${compiler}" >> $GITHUB_ENV
          echo "CXX=${compiler}++" >> $GITHUB_ENV
        fi

    - name: Configure CMake
      run: |
        cmake -B build \
          -DCMAKE_BUILD_TYPE=${{ matrix.build-type }} \
          -DENABLE_TESTING=ON \
          -DENABLE_COVERAGE=ON

    - name: Build
      run: cmake --build build -j$(nproc)

    - name: Run Unit Tests
      run: |
        cd build
        ctest -L "unit" --output-on-failure --timeout 60 --parallel $(nproc)

    - name: Generate Coverage Report
      if: matrix.build-type == 'Debug'
      run: |
        cd build
        gcov -r $(find . -name "*.gcno")
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --list coverage.info

    - name: Upload Coverage
      if: matrix.build-type == 'Debug'
      uses: codecov/codecov-action@v3
      with:
        file: build/coverage.info

  integration-tests:
    runs-on: ubuntu-latest
    needs: unit-tests

    steps:
    - uses: actions/checkout@v3

    - name: Build
      run: |
        cmake -B build -DENABLE_TESTING=ON
        cmake --build build -j$(nproc)

    - name: Run Integration Tests
      run: |
        cd build
        ctest -L "integration" --output-on-failure --timeout 300

    - name: Upload Test Results
      if: always()
      uses: actions/upload-artifact@v3
      with:
        name: integration-test-results
        path: build/Testing/

  performance-tests:
    runs-on: ubuntu-latest
    needs: unit-tests

    steps:
    - uses: actions/checkout@v3

    - name: Build
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON
        cmake --build build -j$(nproc)

    - name: Run Performance Tests
      run: |
        cd build
        ctest -L "benchmark" --output-on-failure --timeout 600

    - name: Check Performance Regression
      run: |
        cd build
        python3 ../tests/scripts/compare_performance.py \
          --baseline ../baseline_performance/comprehensive_baseline.csv \
          --current tests/benchmark_results/ \
          --threshold 10

    - name: Upload Performance Results
      uses: actions/upload-artifact@v3
      with:
        name: performance-results
        path: build/tests/benchmark_results/

  docker-tests:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Build Docker Image
      run: |
        docker build -t solar-system-test .

    - name: Run Tests in Container
      run: |
        docker run --rm \
          -v ${{ github.workspace }}/test-results:/app/test-results \
          solar-system-test \
          bash -c "
            cd build &&
            ctest --output-junit /app/test-results/docker-test-results.xml
          "

    - name: Upload Docker Test Results
      uses: actions/upload-artifact@v3
      with:
        name: docker-test-results
        path: test-results/
```

### Jenkins Pipeline Example

```groovy
// Jenkinsfile
pipeline {
    agent any

    parameters {
        choice(
            name: 'TEST_SUITE',
            choices: ['all', 'unit', 'integration', 'performance'],
            description: 'Which test suite to run'
        )
        booleanParam(
            name: 'GENERATE_COVERAGE',
            defaultValue: false,
            description: 'Generate code coverage report'
        )
    }

    environment {
        BUILD_TYPE = 'Release'
        ENABLE_TESTING = 'ON'
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Build') {
            steps {
                sh '''
                    cmake -B build \
                        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
                        -DENABLE_TESTING=${ENABLE_TESTING} \
                        -DENABLE_COVERAGE=${GENERATE_COVERAGE}
                    cmake --build build -j$(nproc)
                '''
            }
        }

        stage('Test') {
            parallel {
                stage('Unit Tests') {
                    when {
                        anyOf {
                            params.TEST_SUITE == 'all'
                            params.TEST_SUITE == 'unit'
                        }
                    }
                    steps {
                        sh '''
                            cd build
                            ctest -L "unit" \
                                --output-on-failure \
                                --output-junit unit-test-results.xml \
                                --timeout 60
                        '''
                    }
                    post {
                        always {
                            publishTestResults testResultsPattern: 'build/unit-test-results.xml'
                        }
                    }
                }

                stage('Integration Tests') {
                    when {
                        anyOf {
                            params.TEST_SUITE == 'all'
                            params.TEST_SUITE == 'integration'
                        }
                    }
                    steps {
                        sh '''
                            cd build
                            ctest -L "integration" \
                                --output-on-failure \
                                --output-junit integration-test-results.xml \
                                --timeout 300
                        '''
                    }
                    post {
                        always {
                            publishTestResults testResultsPattern: 'build/integration-test-results.xml'
                        }
                    }
                }

                stage('Performance Tests') {
                    when {
                        anyOf {
                            params.TEST_SUITE == 'all'
                            params.TEST_SUITE == 'performance'
                        }
                    }
                    steps {
                        sh '''
                            cd build
                            ctest -L "benchmark" \
                                --output-on-failure \
                                --timeout 600

                            # Check for performance regressions
                            python3 ../tests/scripts/compare_performance.py \
                                --baseline ../baseline_performance/comprehensive_baseline.csv \
                                --current tests/benchmark_results/ \
                                --threshold 10 \
                                --output performance-report.json
                        '''
                    }
                    post {
                        always {
                            archiveArtifacts artifacts: 'build/tests/benchmark_results/**/*'
                            archiveArtifacts artifacts: 'build/performance-report.json'
                        }
                    }
                }
            }
        }

        stage('Coverage Report') {
            when {
                params.GENERATE_COVERAGE == true
            }
            steps {
                sh '''
                    cd build
                    gcov -r $(find . -name "*.gcno")
                    lcov --capture --directory . --output-file coverage.info
                    lcov --remove coverage.info '/usr/*' --output-file coverage.info
                    genhtml coverage.info --output-directory coverage-report
                '''
            }
            post {
                always {
                    publishHTML([
                        allowMissing: false,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: 'build/coverage-report',
                        reportFiles: 'index.html',
                        reportName: 'Coverage Report'
                    ])
                }
            }
        }
    }

    post {
        always {
            // Clean up
            sh 'rm -rf build/CMakeFiles build/Testing/Temporary'
        }
        failure {
            emailext (
                subject: "Build Failed: ${env.JOB_NAME} - ${env.BUILD_NUMBER}",
                body: "Build failed. Check console output at ${env.BUILD_URL}",
                to: "${env.CHANGE_AUTHOR_EMAIL}"
            )
        }
    }
}
```

These examples demonstrate comprehensive testing patterns that cover unit testing, integration testing, performance benchmarking, mock usage, and CI/CD integration. They provide a solid foundation for building robust test suites for the Solar System Suite project.
