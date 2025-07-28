#include <algorithm>
#include <cctype>
#include <iostream>
#include <regex>
#include <string>

bool matches_pattern_test(const std::string& test_name, const std::string& pattern) {
  try {
    // First, try to use the pattern as a regex directly
    std::regex regex_pattern(pattern, std::regex_constants::icase);
    bool result = std::regex_search(test_name, regex_pattern);
    std::cout << "regex_search('" << test_name << "', '" << pattern << "') = " << result
              << std::endl;
    return result;
  } catch (const std::regex_error& e) {
    std::cout << "Regex error for '" << pattern << "': " << e.what() << std::endl;

    // If regex fails, check if it's a simple wildcard pattern (only contains * and alphanumeric)
    bool is_simple_wildcard = true;
    for (char c : pattern) {
      if (c != '*' && !std::isalnum(c) && c != '_') {
        is_simple_wildcard = false;
        break;
      }
    }

    std::cout << "Is simple wildcard: " << is_simple_wildcard << std::endl;

    if (is_simple_wildcard && pattern.find('*') != std::string::npos) {
      // Simple wildcard pattern: convert * to .* and escape other special chars
      std::string escaped_pattern;
      for (char c : pattern) {
        if (c == '*') {
          escaped_pattern += ".*";
        } else {
          escaped_pattern += c;
        }
      }

      std::cout << "Wildcard pattern '" << pattern << "' converted to regex: '" << escaped_pattern
                << "'" << std::endl;

      try {
        std::regex regex(escaped_pattern, std::regex_constants::icase);
        bool result = std::regex_match(test_name, regex);
        std::cout << "regex_match('" << test_name << "', '" << escaped_pattern << "') = " << result
                  << std::endl;
        return result;
      } catch (const std::regex_error& e2) {
        std::cout << "Regex error for wildcard: " << e2.what() << std::endl;
        return false;
      }
    } else {
      // Fall back to simple substring matching
      std::string lower_test_name = test_name;
      std::string lower_pattern = pattern;
      std::transform(lower_test_name.begin(), lower_test_name.end(), lower_test_name.begin(),
                     ::tolower);
      std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), ::tolower);
      bool result = lower_test_name.find(lower_pattern) != std::string::npos;
      std::cout << "substring_match('" << test_name << "', '" << pattern << "') = " << result
                << std::endl;
      return result;
    }
  }
}

int main() {
  std::cout << "Testing new pattern matching:" << std::endl;

  // Test the patterns that are failing
  std::cout << "\n1. Testing 'UniqueTest.*' pattern:" << std::endl;
  matches_pattern_test("UniqueTestBodyFactory", "UniqueTest.*");
  matches_pattern_test("UniqueTestSimulationEngine", "UniqueTest.*");

  std::cout << "\n2. Testing '*UniqueTestBodyFactory*' pattern:" << std::endl;
  matches_pattern_test("UniqueTestBodyFactory", "*UniqueTestBodyFactory*");

  return 0;
}
