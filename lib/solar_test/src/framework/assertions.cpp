#include "solar_test/framework/assertions.hpp"

#include <sstream>

namespace SolarSystem::Testing {

void Assertions::assert_true(bool condition, const std::string& message) {
  if (!condition) {
    throw AssertionFailure(format_message("Expected true but got false", message));
  }
}

void Assertions::assert_false(bool condition, const std::string& message) {
  if (condition) {
    throw AssertionFailure(format_message("Expected false but got true", message));
  }
}

void Assertions::assert_contains(const std::string& haystack, const std::string& needle,
                                 const std::string& message) {
  if (haystack.find(needle) == std::string::npos) {
    std::ostringstream oss;
    oss << "Assertion failed: expected '" << haystack << "' to contain '" << needle << "'";
    throw AssertionFailure(format_message(oss.str(), message));
  }
}

void Assertions::assert_starts_with(const std::string& str, const std::string& prefix,
                                    const std::string& message) {
  if (str.length() < prefix.length() || str.substr(0, prefix.length()) != prefix) {
    std::ostringstream oss;
    oss << "Assertion failed: expected '" << str << "' to start with '" << prefix << "'";
    throw AssertionFailure(format_message(oss.str(), message));
  }
}

void Assertions::assert_ends_with(const std::string& str, const std::string& suffix,
                                  const std::string& message) {
  if (str.length() < suffix.length() || str.substr(str.length() - suffix.length()) != suffix) {
    std::ostringstream oss;
    oss << "Assertion failed: expected '" << str << "' to end with '" << suffix << "'";
    throw AssertionFailure(format_message(oss.str(), message));
  }
}

void Assertions::assert_no_throw(const std::function<void()>& func, const std::string& message) {
  try {
    func();
  } catch (const std::exception& e) {
    std::ostringstream oss;
    oss << "Assertion failed: expected no exception but got: " << e.what();
    throw AssertionFailure(format_message(oss.str(), message));
  } catch (...) {
    throw AssertionFailure(format_message(
        "Assertion failed: expected no exception but got unknown exception", message));
  }
}

std::string Assertions::format_message(const std::string& assertion, const std::string& message) {
  if (message.empty()) {
    return assertion;
  }
  return assertion + " - " + message;
}

}  // namespace SolarSystem::Testing
