#include "solar_test/framework/test_case.hpp"

#include <chrono>
#include <iostream>

#include "solar_test/framework/assertions.hpp"

namespace SolarSystem::Testing {

TestCase::TestCase(TestInfo info) : info_(std::move(info)) {
  result_.test_name = info_.name;
  result_.status = TestResult::Status::Failed;
}

TestResult TestCase::execute() {
  auto start_time = std::chrono::high_resolution_clock::now();

  try {
    // Setup phase
    setup();

    if (test_skipped_) {
      result_.status = TestResult::Status::Skipped;
      return result_;
    }

    // Measure memory usage before test
    measure_memory_usage();
    size_t memory_before = result_.memory_usage_bytes;

    // Run the actual test
    run();

    // Measure memory usage after test
    measure_memory_usage();
    result_.memory_usage_bytes -= memory_before;

    // Teardown phase
    teardown();

    // If we get here without exceptions, the test passed
    result_.status = TestResult::Status::Passed;

  } catch (const AssertionFailure& e) {
    result_.status = TestResult::Status::Failed;
    result_.error_message = e.what();
    // Don't duplicate the assertion failure in the list since it's already in error_message
    // record_assertion_failure(e.what());

    try {
      teardown();
    } catch (...) {
      // Ignore teardown exceptions when test already failed
    }

  } catch (const std::exception& e) {
    result_.status = TestResult::Status::Error;
    result_.error_message = std::string("Unexpected exception: ") + e.what();

    try {
      teardown();
    } catch (...) {
      // Ignore teardown exceptions when test already failed
    }

  } catch (...) {
    result_.status = TestResult::Status::Error;
    result_.error_message = "Unknown exception occurred";

    try {
      teardown();
    } catch (...) {
      // Ignore teardown exceptions when test already failed
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  result_.execution_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Check for timeout
  if (result_.execution_time > info_.timeout) {
    result_.status = TestResult::Status::Timeout;
    result_.error_message =
        "Test execution exceeded timeout of " + std::to_string(info_.timeout.count()) + "ms";
  }

  return result_;
}

void TestCase::assert_true(bool condition, const std::string& message) {
  Assertions::assert_true(condition, message);
}

void TestCase::assert_false(bool condition, const std::string& message) {
  Assertions::assert_false(condition, message);
}

void TestCase::assert_throws(const std::function<void()>& func, const std::string& message) {
  bool exception_thrown = false;
  try {
    func();
  } catch (...) {
    exception_thrown = true;
  }

  if (!exception_thrown) {
    throw AssertionFailure("Expected exception but none was thrown" +
                           (message.empty() ? "" : " - " + message));
  }
}

void TestCase::assert_no_throw(const std::function<void()>& func, const std::string& message) {
  Assertions::assert_no_throw(func, message);
}

void TestCase::assert_execution_time_less_than(const std::function<void()>& func,
                                               std::chrono::milliseconds max_time) {
  auto start = std::chrono::high_resolution_clock::now();
  func();
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  if (duration > max_time) {
    std::ostringstream oss;
    oss << "Execution time " << duration.count() << "ms exceeded maximum " << max_time.count()
        << "ms";
    throw AssertionFailure(oss.str());
  }
}

void TestCase::assert_memory_usage_less_than(const std::function<void()>& func, size_t max_bytes) {
  size_t memory_before = result_.memory_usage_bytes;
  func();
  measure_memory_usage();
  size_t memory_used = result_.memory_usage_bytes - memory_before;

  if (memory_used > max_bytes) {
    std::ostringstream oss;
    oss << "Memory usage " << memory_used << " bytes exceeded maximum " << max_bytes << " bytes";
    throw AssertionFailure(oss.str());
  }
}

void TestCase::skip_test(const std::string& reason) {
  test_skipped_ = true;
  result_.error_message = reason;
}

void TestCase::add_metadata(const std::string& key, const std::string& value) {
  result_.metadata[key] = value;
}

void TestCase::record_assertion_failure(const std::string& message) {
  result_.assertion_failures.push_back(message);
}

void TestCase::measure_memory_usage() {
  // Simple memory usage measurement
  // In a real implementation, this would use platform-specific APIs
  // For now, we'll use a placeholder
  result_.memory_usage_bytes = 0;  // TODO: Implement actual memory measurement
}

}  // namespace SolarSystem::Testing
