#include <chrono>
#include <iostream>
#include <thread>

#include "solar_test/framework/test_case.hpp"
#include "solar_test/framework/test_runner.hpp"

using namespace SolarSystem::Testing;

// Test case that simulates work
class WorkTest1 : public TestCase {
 public:
  WorkTest1()
      : TestCase(
            {"WorkTest1", "A test that does work", {"parallel"}, std::chrono::seconds(5), false}) {}

  void run() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert_true(true, "Work completed");
  }
};

class WorkTest2 : public TestCase {
 public:
  WorkTest2()
      : TestCase({"WorkTest2",
                  "Another test that does work",
                  {"parallel"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert_true(true, "Work completed");
  }
};

class WorkTest3 : public TestCase {
 public:
  WorkTest3()
      : TestCase({"WorkTest3",
                  "Third test that does work",
                  {"parallel"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert_true(true, "Work completed");
  }
};

class WorkTest4 : public TestCase {
 public:
  WorkTest4()
      : TestCase({"WorkTest4",
                  "Fourth test that does work",
                  {"parallel"},
                  std::chrono::seconds(5),
                  false}) {}

  void run() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert_true(true, "Work completed");
  }
};

int main() {
  std::cout << "=== Parallel Test Execution Demo ===" << std::endl;

  // Create test runner with parallel execution enabled
  TestRunner::Configuration config;
  config.parallel_execution = true;
  config.max_threads = 2;
  config.verbose = true;
  config.quiet = false;

  TestRunner runner(config);

  // Register test cases
  runner.register_test(std::make_unique<WorkTest1>());
  runner.register_test(std::make_unique<WorkTest2>());
  runner.register_test(std::make_unique<WorkTest3>());
  runner.register_test(std::make_unique<WorkTest4>());

  // Set up callbacks
  runner.set_progress_callback([](const std::string& message, double percentage) {
    std::cout << "[PROGRESS] " << message << " (" << percentage << "%)" << std::endl;
  });

  runner.set_test_started_callback([](const std::string& test_name) {
    std::cout << "[STARTED] " << test_name << " (Thread: " << std::this_thread::get_id() << ")"
              << std::endl;
  });

  runner.set_test_completed_callback([](const TestResult& result) {
    std::cout << "[COMPLETED] " << result.to_string() << " (Thread: " << std::this_thread::get_id()
              << ")" << std::endl;
  });

  // Measure execution time
  auto start_time = std::chrono::high_resolution_clock::now();

  // Run all tests in parallel
  TestSuiteResult suite_result = runner.run_all_tests();

  auto end_time = std::chrono::high_resolution_clock::now();
  auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Print summary
  std::cout << "\n=== Parallel Execution Summary ===" << std::endl;
  std::cout << suite_result.summary() << std::endl;
  std::cout << "Actual wall clock time: " << total_time.count() << "ms" << std::endl;
  std::cout << "Expected sequential time: ~200ms (4 tests × 50ms each)" << std::endl;
  std::cout << "Parallel efficiency: " << (200.0 / total_time.count()) << "x speedup" << std::endl;

  return suite_result.all_passed() ? 0 : 1;
}
