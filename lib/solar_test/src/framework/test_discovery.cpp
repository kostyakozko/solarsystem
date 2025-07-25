#include "solar_test/framework/test_discovery.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace SolarSystem::Testing {

TestDiscovery& TestDiscovery::instance() {
  static TestDiscovery instance;
  return instance;
}

void TestDiscovery::register_test_factory(const std::string& name,
                                          std::function<std::unique_ptr<TestCase>()> factory,
                                          const std::vector<std::string>& tags) {
  TestRegistration registration;
  registration.name = name;
  registration.factory = std::move(factory);
  registration.tags = tags;

  registered_tests_[name] = std::move(registration);
}

std::vector<std::unique_ptr<TestCase>> TestDiscovery::discover_all_tests() const {
  std::vector<std::unique_ptr<TestCase>> tests;
  tests.reserve(registered_tests_.size());

  for (const auto& [name, registration] : registered_tests_) {
    auto test = registration.factory();
    if (test) {
      tests.push_back(std::move(test));
    }
  }

  return tests;
}

std::vector<std::unique_ptr<TestCase>> TestDiscovery::discover_tests(
    const DiscoveryOptions& options) const {
  std::vector<std::unique_ptr<TestCase>> tests;

  for (const auto& [name, registration] : registered_tests_) {
    if (passes_filter(registration, options)) {
      auto test = registration.factory();
      if (test) {
        tests.push_back(std::move(test));
      }
    }
  }

  return tests;
}

std::vector<std::unique_ptr<TestCase>> TestDiscovery::discover_tests_by_pattern(
    const std::string& pattern) const {
  std::vector<std::unique_ptr<TestCase>> tests;

  for (const auto& [name, registration] : registered_tests_) {
    if (matches_pattern(name, pattern)) {
      auto test = registration.factory();
      if (test) {
        tests.push_back(std::move(test));
      }
    }
  }

  return tests;
}

std::vector<std::unique_ptr<TestCase>> TestDiscovery::discover_tests_by_tag(
    const std::string& tag) const {
  std::vector<std::unique_ptr<TestCase>> tests;

  for (const auto& [name, registration] : registered_tests_) {
    if (has_tag(registration.tags, tag)) {
      auto test = registration.factory();
      if (test) {
        tests.push_back(std::move(test));
      }
    }
  }

  return tests;
}

std::vector<std::unique_ptr<TestCase>> TestDiscovery::discover_tests_by_tags(
    const std::vector<std::string>& tags) const {
  std::vector<std::unique_ptr<TestCase>> tests;

  for (const auto& [name, registration] : registered_tests_) {
    if (has_all_tags(registration.tags, tags)) {
      auto test = registration.factory();
      if (test) {
        tests.push_back(std::move(test));
      }
    }
  }

  return tests;
}

std::vector<std::string> TestDiscovery::get_available_test_names() const {
  std::vector<std::string> names;
  names.reserve(registered_tests_.size());

  for (const auto& [name, registration] : registered_tests_) {
    names.push_back(name);
  }

  std::sort(names.begin(), names.end());
  return names;
}

std::vector<std::string> TestDiscovery::get_available_tags() const {
  std::set<std::string> unique_tags;

  for (const auto& [name, registration] : registered_tests_) {
    for (const auto& tag : registration.tags) {
      unique_tags.insert(tag);
    }
  }

  return std::vector<std::string>(unique_tags.begin(), unique_tags.end());
}

std::map<std::string, size_t> TestDiscovery::get_test_count_by_category() const {
  std::map<std::string, size_t> counts;

  // Initialize common categories
  counts["total"] = 0;
  counts["unit"] = 0;
  counts["integration"] = 0;
  counts["benchmark"] = 0;
  counts["slow"] = 0;
  counts["fast"] = 0;

  for (const auto& [name, registration] : registered_tests_) {
    counts["total"]++;

    // Count by tags
    for (const auto& tag : registration.tags) {
      counts[tag]++;
    }

    // Default categorization if no specific tags
    if (registration.tags.empty()) {
      counts["unit"]++;
    }
  }

  return counts;
}

bool TestDiscovery::has_test(const std::string& name) const {
  return registered_tests_.find(name) != registered_tests_.end();
}

std::unique_ptr<TestCase::TestInfo> TestDiscovery::get_test_info(const std::string& name) const {
  auto it = registered_tests_.find(name);
  if (it == registered_tests_.end()) {
    return nullptr;
  }

  // Create a temporary test instance to get its info
  auto test = it->second.factory();
  if (!test) {
    return nullptr;
  }

  return std::make_unique<TestCase::TestInfo>(test->info());
}

void TestDiscovery::clear_registry() { registered_tests_.clear(); }

bool TestDiscovery::matches_pattern(const std::string& test_name,
                                    const std::string& pattern) const {
  try {
    // Try regex matching first
    std::regex regex_pattern(pattern, std::regex_constants::icase);
    return std::regex_search(test_name, regex_pattern);
  } catch (const std::regex_error&) {
    // Fall back to simple wildcard matching
    if (pattern.find('*') != std::string::npos) {
      // Simple wildcard support: convert * to .*
      std::string regex_pattern = pattern;
      // Replace * with .* for regex
      size_t pos = 0;
      while ((pos = regex_pattern.find('*', pos)) != std::string::npos) {
        regex_pattern.replace(pos, 1, ".*");
        pos += 2;
      }

      try {
        std::regex regex(regex_pattern, std::regex_constants::icase);
        return std::regex_match(test_name, regex);
      } catch (const std::regex_error&) {
        // Fall back to substring matching
        return test_name.find(pattern) != std::string::npos;
      }
    } else {
      // Simple substring matching
      std::string lower_test_name = test_name;
      std::string lower_pattern = pattern;
      std::transform(lower_test_name.begin(), lower_test_name.end(), lower_test_name.begin(),
                     ::tolower);
      std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), ::tolower);
      return lower_test_name.find(lower_pattern) != std::string::npos;
    }
  }
}

bool TestDiscovery::matches_any_pattern(const std::string& test_name,
                                        const std::vector<std::string>& patterns) const {
  if (patterns.empty()) {
    return true;  // No patterns means match all
  }

  for (const auto& pattern : patterns) {
    if (matches_pattern(test_name, pattern)) {
      return true;
    }
  }

  return false;
}

bool TestDiscovery::has_tag(const std::vector<std::string>& test_tags,
                            const std::string& tag) const {
  return std::find(test_tags.begin(), test_tags.end(), tag) != test_tags.end();
}

bool TestDiscovery::has_all_tags(const std::vector<std::string>& test_tags,
                                 const std::vector<std::string>& required_tags) const {
  if (required_tags.empty()) {
    return true;  // No required tags means match all
  }

  for (const auto& required_tag : required_tags) {
    if (!has_tag(test_tags, required_tag)) {
      return false;
    }
  }

  return true;
}

bool TestDiscovery::has_any_excluded_tag(const std::vector<std::string>& test_tags,
                                         const std::vector<std::string>& excluded_tags) const {
  if (excluded_tags.empty()) {
    return false;  // No excluded tags means none are excluded
  }

  for (const auto& excluded_tag : excluded_tags) {
    if (has_tag(test_tags, excluded_tag)) {
      return true;
    }
  }

  return false;
}

bool TestDiscovery::passes_filter(const TestRegistration& registration,
                                  const DiscoveryOptions& options) const {
  // Check include patterns
  if (!matches_any_pattern(registration.name, options.include_patterns)) {
    return false;
  }

  // Check exclude patterns
  if (!options.exclude_patterns.empty() &&
      matches_any_pattern(registration.name, options.exclude_patterns)) {
    return false;
  }

  // Check required tags
  if (!has_all_tags(registration.tags, options.required_tags)) {
    return false;
  }

  // Check excluded tags
  if (has_any_excluded_tag(registration.tags, options.excluded_tags)) {
    return false;
  }

  // Check benchmark filter
  if (!options.include_benchmarks && has_tag(registration.tags, "benchmark")) {
    return false;
  }

  // Check slow test filter
  if (!options.include_slow_tests && has_tag(registration.tags, "slow")) {
    return false;
  }

  return true;
}

// TestScanner implementation
TestScanner::ScanResult TestScanner::scan_directory(const std::string& directory, bool recursive) {
  ScanResult result;

  try {
    std::filesystem::path dir_path(directory);

    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
      result.scan_errors.push_back("Directory does not exist or is not a directory: " + directory);
      return result;
    }

    if (recursive) {
      for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
          const auto& path = entry.path();
          const auto extension = path.extension().string();

          // Look for C++ source files
          if (extension == ".cpp" || extension == ".cc" || extension == ".cxx" ||
              extension == ".hpp" || extension == ".h" || extension == ".hxx") {
            auto file_result = scan_file(path.string());

            // Merge results
            result.found_tests.insert(result.found_tests.end(), file_result.found_tests.begin(),
                                      file_result.found_tests.end());
            result.found_benchmarks.insert(result.found_benchmarks.end(),
                                           file_result.found_benchmarks.begin(),
                                           file_result.found_benchmarks.end());

            for (const auto& [test_name, tags] : file_result.test_tags) {
              result.test_tags[test_name] = tags;
            }

            result.scan_errors.insert(result.scan_errors.end(), file_result.scan_errors.begin(),
                                      file_result.scan_errors.end());
          }
        }
      }
    } else {
      for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
          const auto& path = entry.path();
          const auto extension = path.extension().string();

          // Look for C++ source files
          if (extension == ".cpp" || extension == ".cc" || extension == ".cxx" ||
              extension == ".hpp" || extension == ".h" || extension == ".hxx") {
            auto file_result = scan_file(path.string());

            // Merge results
            result.found_tests.insert(result.found_tests.end(), file_result.found_tests.begin(),
                                      file_result.found_tests.end());
            result.found_benchmarks.insert(result.found_benchmarks.end(),
                                           file_result.found_benchmarks.begin(),
                                           file_result.found_benchmarks.end());

            for (const auto& [test_name, tags] : file_result.test_tags) {
              result.test_tags[test_name] = tags;
            }

            result.scan_errors.insert(result.scan_errors.end(), file_result.scan_errors.begin(),
                                      file_result.scan_errors.end());
          }
        }
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    result.scan_errors.push_back("Filesystem error: " + std::string(e.what()));
  } catch (const std::exception& e) {
    result.scan_errors.push_back("Error scanning directory: " + std::string(e.what()));
  }

  return result;
}

TestScanner::ScanResult TestScanner::scan_file(const std::string& file_path) {
  ScanResult result;

  try {
    std::ifstream file(file_path);
    if (!file.is_open()) {
      result.scan_errors.push_back("Could not open file: " + file_path);
      return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Extract test names
    auto test_names = extract_test_names_from_content(content);

    for (const auto& test_name : test_names) {
      // Check if it's a benchmark
      if (content.find("SOLAR_BENCHMARK_CASE") != std::string::npos &&
          content.find(test_name) != std::string::npos) {
        result.found_benchmarks.push_back(test_name);
      } else {
        result.found_tests.push_back(test_name);
      }

      // Extract tags for this test
      auto tags = extract_tags_from_content(content, test_name);
      if (!tags.empty()) {
        result.test_tags[test_name] = tags;
      }
    }

  } catch (const std::exception& e) {
    result.scan_errors.push_back("Error scanning file " + file_path + ": " + e.what());
  }

  return result;
}

std::vector<std::string> TestScanner::extract_test_names_from_content(const std::string& content) {
  std::vector<std::string> test_names;

  // Regex patterns for different test case macros
  std::vector<std::regex> patterns = {
      std::regex(R"(SOLAR_TEST_CASE\s*\(\s*(\w+)\s*,)"),
      std::regex(R"(SOLAR_TEST_CASE_AUTO\s*\(\s*(\w+)\s*,)"),
      std::regex(R"(SOLAR_BENCHMARK_CASE\s*\(\s*(\w+)\s*,)"),
      std::regex(R"(SOLAR_BENCHMARK_CASE_AUTO\s*\(\s*(\w+)\s*,)"),
      std::regex(R"(class\s+(\w+)\s*:\s*public\s+.*TestCase)"),
  };

  for (const auto& pattern : patterns) {
    std::sregex_iterator iter(content.begin(), content.end(), pattern);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
      const std::smatch& match = *iter;
      if (match.size() > 1) {
        test_names.push_back(match[1].str());
      }
    }
  }

  // Remove duplicates
  std::sort(test_names.begin(), test_names.end());
  test_names.erase(std::unique(test_names.begin(), test_names.end()), test_names.end());

  return test_names;
}

std::vector<std::string> TestScanner::extract_tags_from_content(const std::string& content,
                                                                const std::string& test_name) {
  std::vector<std::string> tags;

  // Look for SOLAR_REGISTER_TEST_WITH_TAGS macro
  std::regex tag_pattern(R"(SOLAR_REGISTER_TEST_WITH_TAGS\s*\(\s*)" + test_name +
                         R"(\s*,\s*([^)]+)\))");
  std::smatch match;

  if (std::regex_search(content, match, tag_pattern)) {
    if (match.size() > 1) {
      std::string tags_str = match[1].str();

      // Parse comma-separated tags
      std::regex tag_item_pattern("\"([^\"]+)\"");
      std::sregex_iterator tag_iter(tags_str.begin(), tags_str.end(), tag_item_pattern);
      std::sregex_iterator tag_end;

      for (; tag_iter != tag_end; ++tag_iter) {
        const std::smatch& tag_match = *tag_iter;
        if (tag_match.size() > 1) {
          tags.push_back(tag_match[1].str());
        }
      }
    }
  }

  // Also look for inline tags in AUTO macros
  std::regex auto_pattern(R"(SOLAR_TEST_CASE_AUTO\s*\(\s*)" + test_name +
                          R"(\s*,\s*[^,]+\s*,\s*([^)]+)\))");
  if (std::regex_search(content, match, auto_pattern)) {
    if (match.size() > 1) {
      std::string tags_str = match[1].str();

      // Parse comma-separated tags
      std::regex tag_item_pattern("\"([^\"]+)\"");
      std::sregex_iterator tag_iter2(tags_str.begin(), tags_str.end(), tag_item_pattern);
      std::sregex_iterator tag_end2;

      for (; tag_iter2 != tag_end2; ++tag_iter2) {
        const std::smatch& tag_match = *tag_iter2;
        if (tag_match.size() > 1) {
          tags.push_back(tag_match[1].str());
        }
      }
    }
  }

  return tags;
}

}  // namespace SolarSystem::Testing
