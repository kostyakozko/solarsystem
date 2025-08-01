#include <chrono>
#include <iostream>
#include <memory>
#include <set>
#include <thread>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_runner.hpp"

using namespace SolarSystem::Testing;

// Test case that simulates work
class MockTestCase : public TestCase {
 public:
  MockTestCase(const std::string& name, std::chrono::milliseconds duration,
               const std::vector<std::string>& tags = {})
      : TestCase(
            TestInfo{name, "Mock test case", tags, std::chrono::seconds(10), false, false, ""}),
        duration_(duration) {}

  void run() override {
    std::this_thread::sleep_for(duration_);
    // Test passes by default
  }

 private:
  std::chrono::milliseconds duration_;
};

// Test case that requires specific resources
class ResourceTestCase : public TestCase {
 public:
  ResourceTestCase(const std::string& name, const std::vector<std::string>& tags)
      : TestCase(TestInfo{name, "Resource test case", tags, std::chrono::seconds(5), false, false,
                          ""}) {}

  void run() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Test passes by default
  }
};

// Test basic parallel execution functionality
class BasicParallelExecutionTest : public TestCase {
 public:
  BasicParallelExecutionTest()
      : TestCase(TestInfo{"BasicParallelExecutionTest",
                          "Test basic parallel execution",
                          {"unit", "parallel"},
                          std::chrono::seconds(10),
                          false,
                          false,
                          ""}) {}
  void run() override {
    TestRunner::Configuration config;
    config.parallel_execution = true;
    config.max_threads = 4;
    config.verbose = false;
    config.quiet = true;
    TestRunner runner(config);

    // Create several test cases with different durations
    auto test1 = std::make_unique<MockTestCase>("test1", std::chrono::milliseconds(100));
    auto test2 = std::make_unique<MockTestCase>("test2", std::chrono::milliseconds(100));
    auto test3 = std::make_unique<MockTestCase>("test3", std::chrono::milliseconds(100));
    auto test4 = std::make_unique<MockTestCase>("test4", std::chrono::milliseconds(100));

    runner.register_test(std::move(test1));
    runner.register_test(std::move(test2));
    runner.register_test(std::move(test3));
    runner.register_test(std::move(test4));

    auto start_time = std::chrono::steady_clock::now();
    TestSuiteResult result = runner.run_all_tests();
    auto end_time = std::chrono::steady_clock::now();

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // With parallel execution, total time should be less than sum of individual times
    // (4 * 100ms = 400ms, but with 4 threads it should be closer to 100ms)
    assert_true(total_time.count() < 300, "Parallel execution should be faster than sequential");
    assert_equals(result.test_results.size(), static_cast<size_t>(4), "Should have 4 test results");
    assert_equals(result.passed_count, static_cast<size_t>(4), "All tests should pass");
    assert_equals(result.failed_count, static_cast<size_t>(0), "No tests should fail");
  }
};

// Test resource coordination
class ResourceCoordinationTest : public TestCase {
 public:
  ResourceCoordinationTest()
      : TestCase(TestInfo{"ResourceCoordinationTest",
                          "Test resource coordination",
                          {"unit", "parallel", "resources"},
                          std::chrono::seconds(10),
                          false,
                          false,
                          ""}) {}

  void run() override {
    TestRunner::Configuration config;
    config.parallel_execution = true;
    config.max_threads = 4;
    config.verbose = false;
    config.quiet = true;
    TestRunner runner(config);

    // Create tests that require the same resource (should run sequentially)
    auto test1 = std::make_unique<ResourceTestCase>("filesystem_test1",
                                                    std::vector<std::string>{"filesystem"});
    auto test2 = std::make_unique<ResourceTestCase>("filesystem_test2",
                                                    std::vector<std::string>{"filesystem"});
    auto test3 =
        std::make_unique<ResourceTestCase>("network_test1", std::vector<std::string>{"network"});
    auto test4 =
        std::make_unique<ResourceTestCase>("network_test2", std::vector<std::string>{"network"});

    runner.register_test(std::move(test1));
    runner.register_test(std::move(test2));
    runner.register_test(std::move(test3));
    runner.register_test(std::move(test4));

    TestSuiteResult result = runner.run_all_tests();

    // All tests should complete successfully
    assert_equals(result.test_results.size(), static_cast<size_t>(4), "Should have 4 test results");
    assert_equals(result.passed_count, static_cast<size_t>(4), "All tests should pass");
    assert_equals(result.failed_count, static_cast<size_t>(0), "No tests should fail");
  }
};

// Test thread-safe result collection
class ThreadSafeResultCollectionTest : public TestCase {
 public:
  ThreadSafeResultCollectionTest()
      : TestCase(TestInfo{"ThreadSafeResultCollectionTest",
                          "Test thread-safe result collection",
                          {"unit", "parallel", "threadsafe"},
                          std::chrono::seconds(15),
                          false,
                          false,
                          ""}) {}

  void run() override {
    TestRunner::Configuration config;
    config.parallel_execution = true;
    config.max_threads = 4;
    config.verbose = false;
    config.quiet = true;
    TestRunner runner(config);

    // Create many small tests to stress test result collection
    for (int i = 0; i < 20; ++i) {
      auto test = std::make_unique<MockTestCase>("test_" + std::to_string(i),
                                                 std::chrono::milliseconds(10));
      runner.register_test(std::move(test));
    }

    TestSuiteResult result = runner.run_all_tests();

    // All tests should be collected properly
    assert_equals(result.test_results.size(), static_cast<size_t>(20),
                  "Should have 20 test results");
    assert_equals(result.passed_count, static_cast<size_t>(20), "All tests should pass");
    assert_equals(result.failed_count, static_cast<size_t>(0), "No tests should fail");

    // Verify all test names are unique and present
    std::set<std::string> test_names;
    for (const auto& test_result : result.test_results) {
      test_names.insert(test_result.test_name);
    }
    assert_equals(test_names.size(), static_cast<size_t>(20), "All test names should be unique");
  }
};

// Test sequential fallback
class SequentialFallbackTest : public TestCase {
 public:
  SequentialFallbackTest()
      : TestCase(TestInfo{"SequentialFallbackTest",
                          "Test sequential fallback",
                          {"unit", "parallel", "sequential"},
                          std::chrono::seconds(10),
                          false,
                          false,
                          ""}) {}

  void run() override {
    // Test with single thread (should fall back to sequential execution)
    TestRunner::Configuration config;
    config.parallel_execution = true;
    config.max_threads = 1;
    config.verbose = false;
    config.quiet = true;

    TestRunner sequential_runner(config);

    auto test1 = std::make_unique<MockTestCase>("seq_test1", std::chrono::milliseconds(50));
    auto test2 = std::make_unique<MockTestCase>("seq_test2", std::chrono::milliseconds(50));

    sequential_runner.register_test(std::move(test1));
    sequential_runner.register_test(std::move(test2));

    auto start_time = std::chrono::steady_clock::now();
    TestSuiteResult result = sequential_runner.run_all_tests();
    auto end_time = std::chrono::steady_clock::now();

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // With single thread, execution should be sequential (closer to 100ms)
    assert_true(total_time.count() > 80, "Sequential execution should take longer");
    assert_equals(result.test_results.size(), static_cast<size_t>(2), "Should have 2 test results");
    assert_equals(result.passed_count, static_cast<size_t>(2), "All tests should pass");
  }
};

// Test the resource manager directly
class ResourceManagerTest : public TestCase {
 public:
  ResourceManagerTest()
      : TestCase(TestInfo{"ResourceManagerTest",
                          "Test resource manager",
                          {"unit", "resources"},
                          std::chrono::seconds(5),
                          false,
                          false,
                          ""}) {}

  void run() override {
    TestResourceManager manager;

    // Acquire a resource
    assert_true(manager.acquire_resource(TestResourceManager::ResourceType::FileSystem, "test1"),
                "Should be able to acquire resource");

    // Try to acquire the same resource from another test (should fail with short timeout)
    assert_false(manager.acquire_resource(TestResourceManager::ResourceType::FileSystem, "test2",
                                          std::chrono::milliseconds(10)),
                 "Should not be able to acquire already owned resource");

    // Release the resource
    manager.release_resource(TestResourceManager::ResourceType::FileSystem, "test1");

    // Now the second test should be able to acquire it
    assert_true(manager.acquire_resource(TestResourceManager::ResourceType::FileSystem, "test2"),
                "Should be able to acquire released resource");

    manager.release_resource(TestResourceManager::ResourceType::FileSystem, "test2");
  }
};

// Test resource conflict detection
class ResourceConflictDetectionTest : public TestCase {
 public:
  ResourceConflictDetectionTest()
      : TestCase(TestInfo{"ResourceConflictDetectionTest",
                          "Test resource conflict detection",
                          {"unit", "resources", "conflicts"},
                          std::chrono::seconds(5),
                          false,
                          false,
                          ""}) {}

  void run() override {
    TestResourceManager manager;

    // Acquire a resource
    manager.acquire_resource(TestResourceManager::ResourceType::Network, "test1");

    // Check for conflicts
    std::vector<TestResourceManager::ResourceType> required = {
        TestResourceManager::ResourceType::Network};
    assert_true(manager.has_resource_conflict(required, "test2"),
                "Should detect resource conflict");
    assert_false(manager.has_resource_conflict(required, "test1"),
                 "Same test should not have conflict");

    // Check conflicting tests
    auto conflicting = manager.get_conflicting_tests(required);
    assert_equals(conflicting.size(), static_cast<size_t>(1), "Should have one conflicting test");
    assert_equals(conflicting[0], std::string("test1"), "Conflicting test should be test1");

    manager.release_resource(TestResourceManager::ResourceType::Network, "test1");
  }
};

// Test the dependency manager
class DependencyManagerTest : public TestCase {
 public:
  DependencyManagerTest()
      : TestCase(TestInfo{"DependencyManagerTest",
                          "Test dependency manager",
                          {"unit", "dependencies"},
                          std::chrono::seconds(5),
                          false,
                          false,
                          ""}) {}

  void run() override {
    TestDependencyManager manager;

    // Set up dependencies: test2 depends on test1, test3 depends on test2
    manager.add_dependency("test2", "test1");
    manager.add_dependency("test3", "test2");

    std::vector<std::string> tests = {"test1", "test2", "test3"};

    // Validate dependencies
    assert_true(manager.validate_dependencies(tests), "Dependencies should be valid");

    // Resolve execution order
    auto execution_groups = manager.resolve_execution_order(tests);

    // Should have at least 1 group, and the dependencies should be respected
    assert_true(execution_groups.size() >= 1, "Should have at least 1 execution group");

    // Verify that dependencies are respected by checking that test1 comes before test2,
    // and test2 comes before test3 in the flattened execution order
    std::vector<std::string> flattened_order;
    for (const auto& group : execution_groups) {
      for (const auto& test_name : group) {
        flattened_order.push_back(test_name);
      }
    }

    // Find positions of each test
    auto pos1 = std::find(flattened_order.begin(), flattened_order.end(), "test1");
    auto pos2 = std::find(flattened_order.begin(), flattened_order.end(), "test2");
    auto pos3 = std::find(flattened_order.begin(), flattened_order.end(), "test3");

    assert_true(pos1 != flattened_order.end(), "test1 should be in execution order");
    assert_true(pos2 != flattened_order.end(), "test2 should be in execution order");
    assert_true(pos3 != flattened_order.end(), "test3 should be in execution order");
    assert_true(pos1 < pos2, "test1 should come before test2");
    assert_true(pos2 < pos3, "test2 should come before test3");
  }
};

// Test circular dependency detection
class CircularDependencyDetectionTest : public TestCase {
 public:
  CircularDependencyDetectionTest()
      : TestCase(TestInfo{"CircularDependencyDetectionTest",
                          "Test circular dependency detection",
                          {"unit", "dependencies", "circular"},
                          std::chrono::seconds(5),
                          false,
                          false,
                          ""}) {}

  void run() override {
    TestDependencyManager manager;

    // Create circular dependency: test1 -> test2 -> test3 -> test1
    manager.add_dependency("test1", "test2");
    manager.add_dependency("test2", "test3");
    manager.add_dependency("test3", "test1");

    std::vector<std::string> tests = {"test1", "test2", "test3"};

    // Should detect circular dependency
    assert_true(manager.has_circular_dependency(tests), "Should detect circular dependency");
    assert_false(manager.validate_dependencies(tests), "Dependencies should be invalid");
  }
};

int main() {
  std::cout << "Running parallel execution tests..." << std::endl;

  // Create and run all tests
  std::vector<std::unique_ptr<TestCase>> tests;
  tests.push_back(std::make_unique<BasicParallelExecutionTest>());
  tests.push_back(std::make_unique<ResourceCoordinationTest>());
  tests.push_back(std::make_unique<ThreadSafeResultCollectionTest>());
  tests.push_back(std::make_unique<SequentialFallbackTest>());
  tests.push_back(std::make_unique<ResourceManagerTest>());
  tests.push_back(std::make_unique<ResourceConflictDetectionTest>());
  tests.push_back(std::make_unique<DependencyManagerTest>());
  tests.push_back(std::make_unique<CircularDependencyDetectionTest>());

  int failed_tests = 0;
  for (auto& test : tests) {
    std::cout << "Running " << test->info().name << "..." << std::endl;
    try {
      TestResult result = test->execute();
      if (result.passed()) {
        std::cout << "  PASSED" << std::endl;
      } else {
        std::cout << "  FAILED: " << result.error_message << std::endl;
        failed_tests++;
      }
    } catch (const std::exception& e) {
      std::cout << "  ERROR: " << e.what() << std::endl;
      failed_tests++;
    }
  }

  std::cout << std::endl;
  if (failed_tests == 0) {
    std::cout << "All parallel execution tests passed!" << std::endl;
    return 0;
  } else {
    std::cout << failed_tests << " tests failed." << std::endl;
    return 1;
  }
}
