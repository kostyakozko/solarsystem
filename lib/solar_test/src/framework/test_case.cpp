#include "solar_test/framework/test_case.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef __APPLE__
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/resource.h>
#elif defined(_WIN32)
#include <psapi.h>
#include <windows.h>
#else
#include <sys/resource.h>
#endif

#include "solar_jpl/jpl_client.hpp"
#include "solar_test/framework/assertions.hpp"

namespace SolarSystem::Testing {

TestCase::TestCase(TestInfo info) : info_(std::move(info)) {
  result_.test_name = info_.name;
  result_.status = TestResult::Status::Failed;
  result_.was_expected_to_fail = info_.expect_failure;
  result_.expected_failure_reason = info_.expected_failure_reason;
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

  // Handle expected failure logic
  if (info_.expect_failure) {
    if (result_.status == TestResult::Status::Failed ||
        result_.status == TestResult::Status::Error ||
        result_.status == TestResult::Status::Timeout) {
      // Test was expected to fail and it did - this is a success!
      result_.status = TestResult::Status::ExpectedFailure;
      if (!info_.expected_failure_reason.empty()) {
        result_.error_message = "Expected failure: " + info_.expected_failure_reason +
                                " (Original: " + result_.error_message + ")";
      }
    } else if (result_.status == TestResult::Status::Passed) {
      // Test was expected to fail but it passed - this is unexpected!
      result_.status = TestResult::Status::Failed;
      result_.error_message = "Test was expected to fail (" + info_.expected_failure_reason +
                              ") but it passed unexpectedly";
    }
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

void TestCase::assert_contains(const std::string& haystack, const std::string& needle,
                               const std::string& message) {
  Assertions::assert_contains(haystack, needle, message);
}

void TestCase::assert_starts_with(const std::string& str, const std::string& prefix,
                                  const std::string& message) {
  Assertions::assert_starts_with(str, prefix, message);
}

void TestCase::assert_ends_with(const std::string& str, const std::string& suffix,
                                const std::string& message) {
  Assertions::assert_ends_with(str, suffix, message);
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
  // Use platform-specific memory measurement
#ifdef __APPLE__
  struct mach_task_basic_info info;
  mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &info_count) ==
      KERN_SUCCESS) {
    result_.memory_usage_bytes = info.resident_size;
    return;
  }
#elif defined(__linux__)
  std::ifstream status_file("/proc/self/status");
  std::string line;
  while (std::getline(status_file, line)) {
    if (line.substr(0, 6) == "VmRSS:") {
      std::istringstream iss(line);
      std::string label;
      size_t value;
      std::string unit;
      if (iss >> label >> value >> unit) {
        result_.memory_usage_bytes = value * 1024;  // Convert from kB to bytes
        return;
      }
    }
  }
#elif defined(_WIN32)
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    result_.memory_usage_bytes = pmc.WorkingSetSize;
    return;
  }
#else
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
#ifdef __linux__
    result_.memory_usage_bytes = usage.ru_maxrss * 1024;
#else
    result_.memory_usage_bytes = usage.ru_maxrss;
#endif
    return;
  }
#endif

  // Fallback if platform-specific measurement fails
  result_.memory_usage_bytes = 0;
}

// Template method implementations
template <typename T>
void TestCase::assert_equals(const T& expected, const T& actual, const std::string& message) {
  Assertions::assert_equals(expected, actual, message);
}

template <typename T>
void TestCase::assert_not_equals(const T& expected, const T& actual, const std::string& message) {
  Assertions::assert_not_equals(expected, actual, message);
}

template <typename T>
void TestCase::assert_near(const T& expected, const T& actual, const T& tolerance,
                           const std::string& message) {
  Assertions::assert_near(expected, actual, tolerance, message);
}

template <typename T>
void TestCase::assert_greater_than(const T& actual, const T& threshold,
                                   const std::string& message) {
  Assertions::assert_greater_than(actual, threshold, message);
}

template <typename T>
void TestCase::assert_less_than(const T& actual, const T& threshold, const std::string& message) {
  Assertions::assert_less_than(actual, threshold, message);
}

// Explicit instantiations for common types
template void TestCase::assert_equals<int>(const int&, const int&, const std::string&);
template void TestCase::assert_equals<double>(const double&, const double&, const std::string&);
template void TestCase::assert_equals<float>(const float&, const float&, const std::string&);
template void TestCase::assert_equals<std::string>(const std::string&, const std::string&,
                                                   const std::string&);
template void TestCase::assert_equals<size_t>(const size_t&, const size_t&, const std::string&);

template void TestCase::assert_not_equals<int>(const int&, const int&, const std::string&);
template void TestCase::assert_not_equals<double>(const double&, const double&, const std::string&);
template void TestCase::assert_not_equals<float>(const float&, const float&, const std::string&);
template void TestCase::assert_not_equals<std::string>(const std::string&, const std::string&,
                                                       const std::string&);

template void TestCase::assert_near<double>(const double&, const double&, const double&,
                                            const std::string&);
template void TestCase::assert_near<float>(const float&, const float&, const float&,
                                           const std::string&);
template void TestCase::assert_near<int>(const int&, const int&, const int&, const std::string&);

template void TestCase::assert_greater_than<double>(const double&, const double&,
                                                    const std::string&);
template void TestCase::assert_greater_than<float>(const float&, const float&, const std::string&);
template void TestCase::assert_greater_than<int>(const int&, const int&, const std::string&);

template void TestCase::assert_less_than<double>(const double&, const double&, const std::string&);
template void TestCase::assert_less_than<float>(const float&, const float&, const std::string&);
template void TestCase::assert_less_than<int>(const int&, const int&, const std::string&);

}  // namespace SolarSystem::Testing
