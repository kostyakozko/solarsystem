#include <iostream>
#include <regex>
#include <string>

bool matches_pattern_test(const std::string& test_name, const std::string& pattern) {
  try {
    // Check if pattern contains wildcards
    if (pattern.find('*') != std::string::npos) {
      // Simple wildcard support: convert * to .*
      std::string regex_pattern = pattern;
      // Escape special regex characters except *
      std::string escaped_pattern;
      for (char c : regex_pattern) {
        if (c == '*') {
          escaped_pattern += ".*";
        } else if (c == '.' || c == '^' || c == '$' || c == '+' || c == '?' || c == '(' ||
                   c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '|' ||
                   c == '\\') {
          escaped_pattern += '\\';
          escaped_pattern += c;
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
      } catch (const std::regex_error& e) {
        std::cout << "Regex error: " << e.what() << std::endl;
        return false;
      }
    } else {
      // Try regex matching for non-wildcard patterns
      std::regex regex_pattern(pattern, std::regex_constants::icase);
      bool result = std::regex_search(test_name, regex_pattern);
      std::cout << "regex_search('" << test_name << "', '" << pattern << "') = " << result
                << std::endl;
      return result;
    }
  } catch (const std::regex_error& e) {
    std::cout << "Regex error: " << e.what() << std::endl;
    return false;
  }
}

int main() {
  std::cout << "Testing pattern matching:" << std::endl;

  // Test the patterns that are failing
  std::cout << "\n1. Testing 'UniqueTest.*' pattern:" << std::endl;
  matches_pattern_test("UniqueTestBodyFactory", "UniqueTest.*");
  matches_pattern_test("UniqueTestSimulationEngine", "UniqueTest.*");

  std::cout << "\n2. Testing '*UniqueTestBodyFactory*' pattern:" << std::endl;
  matches_pattern_test("UniqueTestBodyFactory", "*UniqueTestBodyFactory*");

  std::cout << "\n3. Testing simple substring:" << std::endl;
  std::string test_name = "UniqueTestBodyFactory";
  std::string pattern = "UniqueTest";
  bool contains = test_name.find(pattern) != std::string::npos;
  std::cout << "'" << test_name << "' contains '" << pattern << "': " << contains << std::endl;

  return 0;
}
