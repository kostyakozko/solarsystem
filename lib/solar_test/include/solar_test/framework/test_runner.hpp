#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../reporters/test_reporter.hpp"
#include "test_case.hpp"
#include "test_discovery.hpp"
#include "test_result.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Resource coordination for parallel test execution
 */
class TestResourceManager {
 public:
  enum class ResourceType {
    FileSystem,   // Tests that modify files
    Network,      // Tests that use network resources
    Database,     // Tests that use database connections
    Environment,  // Tests that modify environment variables
    GlobalState,  // Tests that modify global application state
    Cache,        // Tests that use cache systems
    JPLApi,       // Tests that use JPL API (mock or real)
    WebServer     // Tests that start web servers
  };

  TestResourceManager();
  ~TestResourceManager();

  // Resource acquisition and release
  bool acquire_resource(ResourceType resource, const std::string& test_name,
                        std::chrono::milliseconds timeout = std::chrono::seconds(30));
  void release_resource(ResourceType resource, const std::string& test_name);
  void release_all_resources(const std::string& test_name);

  // Resource conflict detection
  [[nodiscard]] bool has_resource_conflict(const std::vector<ResourceType>& required_resources,
                                           const std::string& test_name) const;
  [[nodiscard]] std::vector<std::string> get_conflicting_tests(
      const std::vector<ResourceType>& required_resources) const;

  // Statistics and monitoring
  [[nodiscard]] size_t active_resource_count() const;
  [[nodiscard]] std::map<ResourceType, std::string> get_resource_owners() const;

 private:
  mutable std::mutex resource_mutex_;
  std::map<ResourceType, std::string> resource_owners_;  // Resource -> Test name
  std::condition_variable resource_condition_;
};

/**
 * @brief Test dependency management for execution order
 */
class TestDependencyManager {
 public:
  struct TestDependency {
    std::string test_name;
    std::vector<std::string> depends_on;  // Tests that must complete first
    std::vector<TestResourceManager::ResourceType> required_resources;
    int priority = 0;  // Higher priority tests run first
  };

  TestDependencyManager();

  // Dependency registration
  void add_dependency(const std::string& test_name, const std::string& depends_on);
  void add_dependencies(const std::string& test_name, const std::vector<std::string>& depends_on);
  void set_test_resources(const std::string& test_name,
                          const std::vector<TestResourceManager::ResourceType>& resources);
  void set_test_priority(const std::string& test_name, int priority);

  // Dependency resolution
  [[nodiscard]] std::vector<std::vector<std::string>> resolve_execution_order(
      const std::vector<std::string>& test_names) const;
  [[nodiscard]] bool has_circular_dependency(const std::vector<std::string>& test_names) const;
  [[nodiscard]] std::vector<std::string> get_ready_tests(
      const std::vector<std::string>& all_tests,
      const std::set<std::string>& completed_tests) const;

  // Validation
  [[nodiscard]] bool validate_dependencies(const std::vector<std::string>& test_names) const;
  [[nodiscard]] std::vector<std::string> find_missing_dependencies(
      const std::vector<std::string>& test_names) const;

 private:
  std::map<std::string, TestDependency> dependencies_;

  friend class ParallelTestExecutor;

  // Helper methods for topological sorting
  void topological_sort_util(const std::string& test_name, std::set<std::string>& visited,
                             std::set<std::string>& recursion_stack,
                             std::vector<std::string>& result,
                             const std::vector<std::string>& all_tests) const;
};

/**
 * @brief Thread pool for parallel test execution
 */
class TestThreadPool {
 public:
  explicit TestThreadPool(size_t num_threads);
  ~TestThreadPool();

  // Task submission
  template <typename F, typename... Args>
  auto submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> result = task->get_future();

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (stop_) {
        throw std::runtime_error("Cannot submit task to stopped thread pool");
      }

      tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return result;
  }

  // Pool management
  void shutdown();
  void wait_for_completion();
  [[nodiscard]] size_t active_thread_count() const;
  [[nodiscard]] size_t queued_task_count() const;

 private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_;
  std::atomic<size_t> active_tasks_;
};

/**
 * @brief Thread-safe result collector for parallel execution
 */
class ThreadSafeResultCollector {
 public:
  ThreadSafeResultCollector();

  // Result collection
  void add_result(const TestResult& result);
  void add_results(const std::vector<TestResult>& results);

  // Progress tracking
  void set_total_tests(size_t total);
  void increment_completed_tests();
  [[nodiscard]] double get_progress() const;
  [[nodiscard]] size_t get_completed_count() const;
  [[nodiscard]] size_t get_total_count() const;

  // Result retrieval
  [[nodiscard]] std::vector<TestResult> get_all_results() const;
  [[nodiscard]] TestSuiteResult build_suite_result(const std::string& suite_name) const;

  // Statistics
  [[nodiscard]] size_t passed_count() const;
  [[nodiscard]] size_t failed_count() const;
  [[nodiscard]] size_t skipped_count() const;

 private:
  mutable std::mutex results_mutex_;
  std::vector<TestResult> results_;
  std::atomic<size_t> total_tests_;
  std::atomic<size_t> completed_tests_;
};

/**
 * @brief Enhanced parallel test executor
 */
class ParallelTestExecutor {
 public:
  struct Configuration {
    size_t max_threads = std::thread::hardware_concurrency();
    std::chrono::milliseconds resource_timeout = std::chrono::seconds(30);
    bool enable_dependency_resolution = true;
    bool enable_resource_coordination = true;
    bool fail_fast = false;         // Stop execution on first failure
    double max_failure_rate = 1.0;  // Stop if failure rate exceeds this (0.0-1.0)
  };

  explicit ParallelTestExecutor(Configuration config);
  ~ParallelTestExecutor();

  // Test execution
  [[nodiscard]] TestSuiteResult execute_tests(
      const std::vector<TestCase*>& tests,
      const std::function<void(const std::string&, double)>& progress_callback = nullptr,
      const std::function<void(const TestResult&)>& result_callback = nullptr);

  // Configuration
  void set_configuration(const Configuration& config) { config_ = config; }
  [[nodiscard]] const Configuration& configuration() const { return config_; }

  // Resource and dependency management
  TestResourceManager& resource_manager() { return resource_manager_; }
  TestDependencyManager& dependency_manager() { return dependency_manager_; }

 private:
  Configuration config_;
  TestResourceManager resource_manager_;
  TestDependencyManager dependency_manager_;
  std::unique_ptr<TestThreadPool> thread_pool_;

  // Execution methods
  [[nodiscard]] TestResult execute_single_test_with_coordination(
      TestCase* test, ThreadSafeResultCollector& collector,
      const std::function<void(const TestResult&)>& result_callback);

  void setup_test_dependencies(const std::vector<TestCase*>& tests);
  [[nodiscard]] bool should_stop_execution(const ThreadSafeResultCollector& collector) const;
};

/**
 * @brief Main test runner for executing test suites
 */

/**
 * @brief CI/CD resource cleanup manager
 */
class CIResourceCleanup {
 public:
  CIResourceCleanup();
  ~CIResourceCleanup();

  // Resource tracking
  void register_temp_directory(const std::string& path);
  void register_temp_file(const std::string& path);
  void register_process(int pid);
  void register_network_port(int port);

  // Cleanup operations
  void cleanup_all();
  void cleanup_temp_files();
  void cleanup_processes();
  void cleanup_network_resources();

  // CI-specific cleanup
  void cleanup_for_github_actions();
  void cleanup_for_jenkins();
  void cleanup_for_docker();

  // Emergency cleanup (called on signals)
  static void emergency_cleanup();

 private:
  std::vector<std::string> temp_directories_;
  std::vector<std::string> temp_files_;
  std::vector<int> processes_;
  std::vector<int> network_ports_;
  std::mutex cleanup_mutex_;

  static CIResourceCleanup* instance_;
};

/**
 * @brief Container environment detection and optimization
 */
class ContainerEnvironment {
 public:
  static bool is_running_in_container();
  static bool is_docker_container();
  static bool is_kubernetes_pod();
  static std::string get_container_runtime();

  // Container-specific optimizations
  static size_t get_optimal_thread_count();
  static size_t get_available_memory_mb();
  static std::chrono::milliseconds get_optimal_timeout();

  // Resource limits
  static bool has_memory_limit();
  static bool has_cpu_limit();
  static size_t get_memory_limit_mb();
  static double get_cpu_limit();
};

/**
 * @brief Main test runner for executing test suites
 */
class TestRunner {
 public:
  struct Configuration {
    std::vector<std::string> test_patterns;
    std::vector<std::string> tags;
    bool parallel_execution = true;
    size_t max_threads = std::thread::hardware_concurrency();
    std::chrono::milliseconds timeout = std::chrono::minutes(5);
    bool generate_coverage = false;
    std::string output_format = "console";
    std::string output_file;
    bool verbose = false;
    bool quiet = false;

    // CI/CD specific options
    bool ci_mode = false;                 // Enable CI-specific optimizations
    bool fail_fast = false;               // Stop on first failure
    double max_failure_rate = 1.0;        // Stop if failure rate exceeds this
    bool cleanup_on_exit = true;          // Clean up resources on exit
    std::string ci_system = "";           // CI system identifier (github, jenkins, etc.)
    std::string artifact_directory = "";  // Directory for CI artifacts
    bool containerized = false;           // Running in container environment
    std::chrono::milliseconds ci_timeout = std::chrono::minutes(30);  // CI-specific timeout
    size_t max_memory_mb = 0;                                         // Memory limit (0 = no limit)
  };

  explicit TestRunner(Configuration config);

  // Test registration
  void register_test(std::unique_ptr<TestCase> test_case);
  void register_test_suite(const std::string& suite_name,
                           std::vector<std::unique_ptr<TestCase>> test_cases);

  // Test discovery and execution
  [[nodiscard]] TestSuiteResult run_all_tests();
  [[nodiscard]] TestSuiteResult run_tests_with_tag(const std::string& tag);
  [[nodiscard]] TestSuiteResult run_specific_test(const std::string& test_name);
  [[nodiscard]] TestSuiteResult run_tests_matching_pattern(const std::string& pattern);

  // Configuration
  void set_configuration(const Configuration& config) { config_ = config; }
  [[nodiscard]] const Configuration& configuration() const { return config_; }

  // Reporter management
  void add_reporter(std::unique_ptr<TestReporter> reporter);
  void clear_reporters();
  [[nodiscard]] size_t reporter_count() const { return reporters_.size(); }

  // Progress callbacks (legacy - prefer using reporters)
  void set_progress_callback(std::function<void(const std::string&, double)> callback);
  void set_test_started_callback(std::function<void(const std::string&)> callback);
  void set_test_completed_callback(std::function<void(const TestResult&)> callback);

  // Statistics
  [[nodiscard]] size_t total_test_count() const;
  [[nodiscard]] std::vector<std::string> available_tags() const;
  [[nodiscard]] std::vector<std::string> available_test_names() const;

  // CI/CD integration methods
  void enable_ci_mode(const std::string& ci_system = "");
  void set_artifact_directory(const std::string& directory);
  void set_containerized_mode(bool containerized);
  [[nodiscard]] int get_ci_exit_code(const TestSuiteResult& result) const;
  void cleanup_ci_resources();
  [[nodiscard]] bool is_ci_timeout_exceeded(std::chrono::steady_clock::time_point start_time) const;
  void generate_ci_artifacts(const TestSuiteResult& result);

  // Resource monitoring for CI
  [[nodiscard]] size_t get_memory_usage_mb() const;
  [[nodiscard]] bool is_memory_limit_exceeded() const;

 private:
  Configuration config_;
  std::vector<std::unique_ptr<TestCase>> registered_tests_;
  std::map<std::string, std::vector<std::unique_ptr<TestCase>>> test_suites_;
  std::vector<std::unique_ptr<TestReporter>> reporters_;

  // Callbacks (legacy)
  std::function<void(const std::string&, double)> progress_callback_;
  std::function<void(const std::string&)> test_started_callback_;
  std::function<void(const TestResult&)> test_completed_callback_;

  // Thread safety for callbacks and reporters
  mutable std::mutex callback_mutex_;

  // Enhanced parallel execution
  std::unique_ptr<ParallelTestExecutor> parallel_executor_;

  // CI/CD integration removed for now to avoid linking issues
  std::chrono::steady_clock::time_point execution_start_time_;

  // Internal execution methods
  [[nodiscard]] std::vector<TestCase*> filter_tests(const std::vector<std::string>& patterns,
                                                    const std::vector<std::string>& tags) const;
  [[nodiscard]] TestSuiteResult execute_tests(const std::vector<TestCase*>& tests);
  [[nodiscard]] TestSuiteResult execute_tests_sequential(const std::vector<TestCase*>& tests);
  [[nodiscard]] TestSuiteResult execute_tests_parallel(const std::vector<TestCase*>& tests);

  // Test execution engine methods
  [[nodiscard]] TestResult execute_single_test_with_timeout(TestCase* test);
  [[nodiscard]] TestResult execute_test_in_isolation(TestCase* test);
  void setup_test_isolation();
  void cleanup_test_isolation();

  // Utility methods
  [[nodiscard]] bool matches_pattern(const std::string& test_name,
                                     const std::string& pattern) const;
  [[nodiscard]] bool has_tag(const TestCase& test_case, const std::string& tag) const;
  void notify_progress(const std::string& message, double percentage);
  void notify_test_started(const std::string& test_name);
  void notify_test_completed(const TestResult& result);
  void notify_suite_started(const std::string& suite_name, size_t total_tests);
  void notify_suite_finished(const TestSuiteResult& result);
  void notify_error(const std::string& error_message);
};

/**
 * @brief CI system integration utilities
 */
class CISystemIntegration {
 public:
  enum class CISystem {
    Unknown,
    GitHubActions,
    Jenkins,
    GitLabCI,
    CircleCI,
    TravisCI,
    AzurePipelines
  };

  static CISystem detect_ci_system();
  static std::string get_ci_system_name(CISystem system);

  // CI-specific configurations
  static void configure_for_github_actions(TestRunner& runner);
  static void configure_for_jenkins(TestRunner& runner);
  static void configure_for_gitlab_ci(TestRunner& runner);

  // Artifact generation
  static void generate_github_actions_artifacts(const TestSuiteResult& result,
                                                const std::string& directory);
  static void generate_jenkins_artifacts(const TestSuiteResult& result,
                                         const std::string& directory);
  static void generate_junit_xml(const TestSuiteResult& result, const std::string& file_path);

  // Exit codes
  static int get_standard_exit_code(const TestSuiteResult& result);
  static int get_ci_specific_exit_code(CISystem system, const TestSuiteResult& result);
};

/**
 * @brief Test registry for automatic test registration
 */
class TestRegistry {
 public:
  static TestRegistry& instance();

  void register_test(std::unique_ptr<TestCase> test_case);
  void register_test_factory(const std::string& name,
                             std::function<std::unique_ptr<TestCase>()> factory);

  [[nodiscard]] std::vector<std::unique_ptr<TestCase>> create_all_tests() const;
  [[nodiscard]] std::unique_ptr<TestCase> create_test(const std::string& name) const;

 private:
  TestRegistry() = default;
  std::map<std::string, std::function<std::unique_ptr<TestCase>()>> test_factories_;
};

// Macro for automatic test registration (using TestDiscovery)
#define REGISTER_TEST(TestCaseType)                                                       \
  static SolarSystem::Testing::TestRegistrar<TestCaseType> test_registrar_##TestCaseType( \
      #TestCaseType)

}  // namespace SolarSystem::Testing
