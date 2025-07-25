#include "solar_test/framework/test_runner.hpp"

#include <algorithm>
#include <future>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <thread>

namespace SolarSystem::Testing {

TestRunner::TestRunner(Configuration config) : config_(std::move(config)) {}

void TestRunner::register_test(std::unique_ptr<TestCase> test_case) {
  registered_tests_.push_back(std::move(test_case));
}

void TestRunner::register_test_suite(const std::string& suite_name,
                                     std::vector<std::unique_ptr<TestCase>> test_cases) {
  test_suites_[suite_name] = std::move(test_cases);
}

TestSuiteResult TestRunner::run_all_tests() {
  std::vector<TestCase*> all_tests;

  // Add registered tests
  for (const auto& test : registered_tests_) {
    all_tests.push_back(test.get());
  }

  // Add tests from suites
  for (const auto& [suite_name, tests] : test_suites_) {
    for (const auto& test : tests) {
      all_tests.push_back(test.get());
    }
  }

  return execute_tests(all_tests);
}

TestSuiteResult TestRunner::run_tests_with_tag(const std::string& tag) {
  std::vector<TestCase*> filtered_tests;

  for (const auto& test : registered_tests_) {
    if (has_tag(*test, tag)) {
      filtered_tests.push_back(test.get());
    }
  }

  for (const auto& [suite_name, tests] : test_suites_) {
    for (const auto& test : tests) {
      if (has_tag(*test, tag)) {
        filtered_tests.push_back(test.get());
      }
    }
  }

  return execute_tests(filtered_tests);
}

TestSuiteResult TestRunner::run_specific_test(const std::string& test_name) {
  std::vector<TestCase*> matching_tests;

  for (const auto& test : registered_tests_) {
    if (test->info().name == test_name) {
      matching_tests.push_back(test.get());
      break;
    }
  }

  if (matching_tests.empty()) {
    for (const auto& [suite_name, tests] : test_suites_) {
      for (const auto& test : tests) {
        if (test->info().name == test_name) {
          matching_tests.push_back(test.get());
          break;
        }
      }
      if (!matching_tests.empty()) break;
    }
  }

  return execute_tests(matching_tests);
}

TestSuiteResult TestRunner::run_tests_matching_pattern(const std::string& pattern) {
  std::vector<TestCase*> matching_tests = filter_tests({pattern}, {});
  return execute_tests(matching_tests);
}

void TestRunner::set_progress_callback(std::function<void(const std::string&, double)> callback) {
  progress_callback_ = std::move(callback);
}

void TestRunner::set_test_started_callback(std::function<void(const std::string&)> callback) {
  test_started_callback_ = std::move(callback);
}

void TestRunner::set_test_completed_callback(std::function<void(const TestResult&)> callback) {
  test_completed_callback_ = std::move(callback);
}

size_t TestRunner::total_test_count() const {
  size_t count = registered_tests_.size();
  for (const auto& [suite_name, tests] : test_suites_) {
    count += tests.size();
  }
  return count;
}

std::vector<std::string> TestRunner::available_tags() const {
  std::set<std::string> unique_tags;

  for (const auto& test : registered_tests_) {
    for (const auto& tag : test->info().tags) {
      unique_tags.insert(tag);
    }
  }

  for (const auto& [suite_name, tests] : test_suites_) {
    for (const auto& test : tests) {
      for (const auto& tag : test->info().tags) {
        unique_tags.insert(tag);
      }
    }
  }

  return std::vector<std::string>(unique_tags.begin(), unique_tags.end());
}

std::vector<std::string> TestRunner::available_test_names() const {
  std::vector<std::string> names;

  for (const auto& test : registered_tests_) {
    names.push_back(test->info().name);
  }

  for (const auto& [suite_name, tests] : test_suites_) {
    for (const auto& test : tests) {
      names.push_back(test->info().name);
    }
  }

  return names;
}

std::vector<TestCase*> TestRunner::filter_tests(const std::vector<std::string>& patterns,
                                                const std::vector<std::string>& tags) const {
  std::vector<TestCase*> filtered_tests;

  auto check_test = [&](TestCase* test) {
    // Check patterns
    if (!patterns.empty()) {
      bool matches_pattern = false;
      for (const auto& pattern : patterns) {
        if (this->matches_pattern(test->info().name, pattern)) {
          matches_pattern = true;
          break;
        }
      }
      if (!matches_pattern) return false;
    }

    // Check tags
    if (!tags.empty()) {
      bool has_required_tag = false;
      for (const auto& tag : tags) {
        if (has_tag(*test, tag)) {
          has_required_tag = true;
          break;
        }
      }
      if (!has_required_tag) return false;
    }

    return true;
  };

  for (const auto& test : registered_tests_) {
    if (check_test(test.get())) {
      filtered_tests.push_back(test.get());
    }
  }

  for (const auto& [suite_name, tests] : test_suites_) {
    for (const auto& test : tests) {
      if (check_test(test.get())) {
        filtered_tests.push_back(test.get());
      }
    }
  }

  return filtered_tests;
}

TestSuiteResult TestRunner::execute_tests(const std::vector<TestCase*>& tests) {
  if (config_.parallel_execution && tests.size() > 1) {
    return execute_tests_parallel(tests);
  } else {
    return execute_tests_sequential(tests);
  }
}

TestSuiteResult TestRunner::execute_tests_sequential(const std::vector<TestCase*>& tests) {
  TestSuiteResult suite_result;
  suite_result.suite_name = "Sequential Test Execution";

  notify_progress("Starting test execution", 0.0);

  for (size_t i = 0; i < tests.size(); ++i) {
    TestCase* test = tests[i];

    notify_test_started(test->info().name);

    TestResult result = test->execute();
    suite_result.add_result(result);

    notify_test_completed(result);

    double progress = static_cast<double>(i + 1) / static_cast<double>(tests.size()) * 100.0;
    notify_progress("Test " + std::to_string(i + 1) + "/" + std::to_string(tests.size()), progress);
  }

  notify_progress("Test execution completed", 100.0);
  return suite_result;
}

TestSuiteResult TestRunner::execute_tests_parallel(const std::vector<TestCase*>& tests) {
  TestSuiteResult suite_result;
  suite_result.suite_name = "Parallel Test Execution";

  notify_progress("Starting parallel test execution", 0.0);

  const size_t num_threads = std::min(config_.max_threads, tests.size());
  const size_t tests_per_thread = tests.size() / num_threads;
  const size_t remaining_tests = tests.size() % num_threads;

  std::vector<std::future<std::vector<TestResult>>> futures;

  size_t test_index = 0;
  for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
    size_t thread_test_count = tests_per_thread + (thread_id < remaining_tests ? 1 : 0);

    std::vector<TestCase*> thread_tests(tests.begin() + test_index,
                                        tests.begin() + test_index + thread_test_count);

    futures.push_back(std::async(std::launch::async, [this, thread_tests]() {
      std::vector<TestResult> thread_results;
      for (TestCase* test : thread_tests) {
        notify_test_started(test->info().name);
        TestResult result = test->execute();
        thread_results.push_back(result);
        notify_test_completed(result);
      }
      return thread_results;
    }));

    test_index += thread_test_count;
  }

  // Collect results from all threads
  for (auto& future : futures) {
    std::vector<TestResult> thread_results = future.get();
    for (const auto& result : thread_results) {
      suite_result.add_result(result);
    }
  }

  notify_progress("Parallel test execution completed", 100.0);
  return suite_result;
}

bool TestRunner::matches_pattern(const std::string& test_name, const std::string& pattern) const {
  try {
    std::regex regex_pattern(pattern);
    return std::regex_match(test_name, regex_pattern);
  } catch (const std::regex_error&) {
    // If regex fails, fall back to simple wildcard matching
    return test_name.find(pattern) != std::string::npos;
  }
}

bool TestRunner::has_tag(const TestCase& test_case, const std::string& tag) const {
  const auto& tags = test_case.info().tags;
  return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

void TestRunner::notify_progress(const std::string& message, double percentage) {
  if (progress_callback_) {
    progress_callback_(message, percentage);
  }
}

void TestRunner::notify_test_started(const std::string& test_name) {
  if (test_started_callback_) {
    test_started_callback_(test_name);
  }
}

void TestRunner::notify_test_completed(const TestResult& result) {
  if (test_completed_callback_) {
    test_completed_callback_(result);
  }
}

// TestRegistry implementation
TestRegistry& TestRegistry::instance() {
  static TestRegistry instance;
  return instance;
}

void TestRegistry::register_test(std::unique_ptr<TestCase> test_case) {
  std::string name = test_case->info().name;
  // Store the test case in a shared_ptr to avoid move capture issues
  auto shared_test = std::shared_ptr<TestCase>(test_case.release());
  test_factories_[name] = [shared_test]() -> std::unique_ptr<TestCase> {
    // This is a one-time use factory, so we can't reuse the same test
    // For now, return nullptr to indicate this pattern needs rethinking
    return nullptr;
  };
}

void TestRegistry::register_test_factory(const std::string& name,
                                         std::function<std::unique_ptr<TestCase>()> factory) {
  test_factories_[name] = std::move(factory);
}

std::vector<std::unique_ptr<TestCase>> TestRegistry::create_all_tests() const {
  std::vector<std::unique_ptr<TestCase>> tests;
  for (const auto& [name, factory] : test_factories_) {
    tests.push_back(factory());
  }
  return tests;
}

std::unique_ptr<TestCase> TestRegistry::create_test(const std::string& name) const {
  auto it = test_factories_.find(name);
  if (it != test_factories_.end()) {
    return it->second();
  }
  return nullptr;
}

}  // namespace SolarSystem::Testing
