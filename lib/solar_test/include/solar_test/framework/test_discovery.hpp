#pragma once

#include <functional>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "test_case.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Test discovery system for automatic test registration and filtering
 */
class TestDiscovery {
 public:
  struct DiscoveryOptions {
    std::vector<std::string> include_patterns;
    std::vector<std::string> exclude_patterns;
    std::vector<std::string> required_tags;
    std::vector<std::string> excluded_tags;
    bool include_benchmarks = true;
    bool include_slow_tests = true;
    std::string test_directory = "";
  };

  /**
   * @brief Get the singleton instance of TestDiscovery
   */
  static TestDiscovery& instance();

  /**
   * @brief Register a test factory function
   * @param name Test name
   * @param factory Function that creates the test instance
   * @param tags Tags associated with the test
   */
  void register_test_factory(const std::string& name,
                             std::function<std::unique_ptr<TestCase>()> factory,
                             const std::vector<std::string>& tags = {});

  /**
   * @brief Discover all registered tests
   * @return Vector of all available test instances
   */
  [[nodiscard]] std::vector<std::unique_ptr<TestCase>> discover_all_tests() const;

  /**
   * @brief Discover tests matching specific criteria
   * @param options Discovery options for filtering
   * @return Vector of matching test instances
   */
  [[nodiscard]] std::vector<std::unique_ptr<TestCase>> discover_tests(
      const DiscoveryOptions& options) const;

  /**
   * @brief Discover tests by name pattern
   * @param pattern Regex pattern to match test names
   * @return Vector of matching test instances
   */
  [[nodiscard]] std::vector<std::unique_ptr<TestCase>> discover_tests_by_pattern(
      const std::string& pattern) const;

  /**
   * @brief Discover tests by tag
   * @param tag Tag to filter tests by
   * @return Vector of matching test instances
   */
  [[nodiscard]] std::vector<std::unique_ptr<TestCase>> discover_tests_by_tag(
      const std::string& tag) const;

  /**
   * @brief Discover tests by multiple tags (AND logic)
   * @param tags Vector of tags that tests must have
   * @return Vector of matching test instances
   */
  [[nodiscard]] std::vector<std::unique_ptr<TestCase>> discover_tests_by_tags(
      const std::vector<std::string>& tags) const;

  /**
   * @brief Get all available test names
   * @return Vector of test names
   */
  [[nodiscard]] std::vector<std::string> get_available_test_names() const;

  /**
   * @brief Get all available tags
   * @return Vector of unique tags
   */
  [[nodiscard]] std::vector<std::string> get_available_tags() const;

  /**
   * @brief Get test count by category
   * @return Map of category to count
   */
  [[nodiscard]] std::map<std::string, size_t> get_test_count_by_category() const;

  /**
   * @brief Check if a test exists
   * @param name Test name to check
   * @return True if test exists
   */
  [[nodiscard]] bool has_test(const std::string& name) const;

  /**
   * @brief Get test information without creating instance
   * @param name Test name
   * @return Test info or nullptr if not found
   */
  [[nodiscard]] std::unique_ptr<TestCase::TestInfo> get_test_info(const std::string& name) const;

  /**
   * @brief Clear all registered tests (mainly for testing)
   */
  void clear_registry();

 private:
  struct TestRegistration {
    std::string name;
    std::function<std::unique_ptr<TestCase>()> factory;
    std::vector<std::string> tags;
  };

  TestDiscovery() = default;
  ~TestDiscovery() = default;
  TestDiscovery(const TestDiscovery&) = delete;
  TestDiscovery& operator=(const TestDiscovery&) = delete;

  std::map<std::string, TestRegistration> registered_tests_;

  // Helper methods
  [[nodiscard]] bool matches_pattern(const std::string& test_name,
                                     const std::string& pattern) const;
  [[nodiscard]] bool matches_any_pattern(const std::string& test_name,
                                         const std::vector<std::string>& patterns) const;
  [[nodiscard]] bool has_tag(const std::vector<std::string>& test_tags,
                             const std::string& tag) const;
  [[nodiscard]] bool has_all_tags(const std::vector<std::string>& test_tags,
                                  const std::vector<std::string>& required_tags) const;
  [[nodiscard]] bool has_any_excluded_tag(const std::vector<std::string>& test_tags,
                                          const std::vector<std::string>& excluded_tags) const;
  [[nodiscard]] bool passes_filter(const TestRegistration& registration,
                                   const DiscoveryOptions& options) const;
};

/**
 * @brief Helper class for automatic test registration
 */
template <typename TestCaseType>
class TestRegistrar {
 public:
  explicit TestRegistrar(const std::string& name, const std::vector<std::string>& tags = {}) {
    TestDiscovery::instance().register_test_factory(
        name, []() { return std::make_unique<TestCaseType>(); }, tags);
  }
};

/**
 * @brief Test scanner for finding tests in source files (future enhancement)
 */
class TestScanner {
 public:
  struct ScanResult {
    std::vector<std::string> found_tests;
    std::vector<std::string> found_benchmarks;
    std::map<std::string, std::vector<std::string>> test_tags;
    std::vector<std::string> scan_errors;
  };

  /**
   * @brief Scan directory for test files
   * @param directory Directory to scan
   * @param recursive Whether to scan recursively
   * @return Scan results
   */
  [[nodiscard]] static ScanResult scan_directory(const std::string& directory,
                                                 bool recursive = true);

  /**
   * @brief Scan single file for tests
   * @param file_path Path to file to scan
   * @return Scan results
   */
  [[nodiscard]] static ScanResult scan_file(const std::string& file_path);

 private:
  static std::vector<std::string> extract_test_names_from_content(const std::string& content);
  static std::vector<std::string> extract_tags_from_content(const std::string& content,
                                                            const std::string& test_name);
};

}  // namespace SolarSystem::Testing

// Macros for automatic test registration
#define SOLAR_REGISTER_TEST(TestCaseType)                                                 \
  static SolarSystem::Testing::TestRegistrar<TestCaseType> test_registrar_##TestCaseType( \
      #TestCaseType)

#define SOLAR_REGISTER_TEST_WITH_TAGS(TestCaseType, ...)                                  \
  static SolarSystem::Testing::TestRegistrar<TestCaseType> test_registrar_##TestCaseType( \
      #TestCaseType, {__VA_ARGS__})

// Enhanced test case macros with automatic registration
#define SOLAR_TEST_CASE_AUTO(name, description, ...)                                              \
  class name : public SolarSystem::Testing::TestCase {                                            \
   public:                                                                                        \
    name()                                                                                        \
        : TestCase(                                                                               \
              {#name, description, {__VA_ARGS__}, std::chrono::seconds(30), false, false, ""}) {} \
    void run() override;                                                                          \
  };                                                                                              \
  SOLAR_REGISTER_TEST_WITH_TAGS(name, __VA_ARGS__);                                               \
  void name::run()

#define SOLAR_BENCHMARK_CASE_AUTO(name, description, ...)        \
  class name : public SolarSystem::Testing::TestCase {           \
   public:                                                       \
    name()                                                       \
        : TestCase({#name,                                       \
                    description,                                 \
                    {"benchmark", __VA_ARGS__},                  \
                    std::chrono::minutes(5),                     \
                    true,                                        \
                    false,                                       \
                    ""}) {}                                      \
    void run() override;                                         \
  };                                                             \
  SOLAR_REGISTER_TEST_WITH_TAGS(name, "benchmark", __VA_ARGS__); \
  void name::run()
