#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <type_traits>

namespace SolarSystem::Testing {

/**
 * @brief Exception thrown when an assertion fails
 */
class AssertionFailure : public std::exception {
 public:
  explicit AssertionFailure(const std::string& message) : message_(message) {}

  [[nodiscard]] const char* what() const noexcept override { return message_.c_str(); }

 private:
  std::string message_;
};

/**
 * @brief Assertion utilities
 */
class Assertions {
 public:
  // Basic assertions
  static void assert_true(bool condition, const std::string& message = "");
  static void assert_false(bool condition, const std::string& message = "");

  // Equality assertions
  template <typename T>
  static void assert_equals(const T& expected, const T& actual, const std::string& message = "") {
    if (!(expected == actual)) {
      std::ostringstream oss;
      oss << "Assertion failed: expected '" << expected << "' but got '" << actual << "'";
      if (!message.empty()) {
        oss << " - " << message;
      }
      throw AssertionFailure(oss.str());
    }
  }

  template <typename T>
  static void assert_not_equals(const T& expected, const T& actual,
                                const std::string& message = "") {
    if (expected == actual) {
      std::ostringstream oss;
      oss << "Assertion failed: expected not to equal '" << expected << "'";
      if (!message.empty()) {
        oss << " - " << message;
      }
      throw AssertionFailure(oss.str());
    }
  }

  // Numeric assertions
  template <typename T>
  static void assert_near(const T& expected, const T& actual, const T& tolerance,
                          const std::string& message = "") {
    static_assert(std::is_arithmetic_v<T>, "assert_near requires arithmetic types");

    T diff = (expected > actual) ? (expected - actual) : (actual - expected);
    if (diff > tolerance) {
      std::ostringstream oss;
      oss << "Assertion failed: expected '" << expected << "' ± " << tolerance << " but got '"
          << actual << "' (difference: " << diff << ")";
      if (!message.empty()) {
        oss << " - " << message;
      }
      throw AssertionFailure(oss.str());
    }
  }

  template <typename T>
  static void assert_greater_than(const T& actual, const T& threshold,
                                  const std::string& message = "") {
    if (!(actual > threshold)) {
      std::ostringstream oss;
      oss << "Assertion failed: expected '" << actual << "' to be greater than '" << threshold
          << "'";
      if (!message.empty()) {
        oss << " - " << message;
      }
      throw AssertionFailure(oss.str());
    }
  }

  template <typename T>
  static void assert_less_than(const T& actual, const T& threshold,
                               const std::string& message = "") {
    if (!(actual < threshold)) {
      std::ostringstream oss;
      oss << "Assertion failed: expected '" << actual << "' to be less than '" << threshold << "'";
      if (!message.empty()) {
        oss << " - " << message;
      }
      throw AssertionFailure(oss.str());
    }
  }

  // String assertions
  static void assert_contains(const std::string& haystack, const std::string& needle,
                              const std::string& message = "");
  static void assert_starts_with(const std::string& str, const std::string& prefix,
                                 const std::string& message = "");
  static void assert_ends_with(const std::string& str, const std::string& suffix,
                               const std::string& message = "");

  // Exception assertions
  template <typename ExceptionType>
  static void assert_throws(const std::function<void()>& func, const std::string& message = "") {
    bool exception_thrown = false;
    try {
      func();
    } catch (const ExceptionType&) {
      exception_thrown = true;
    } catch (...) {
      std::ostringstream oss;
      oss << "Assertion failed: expected exception of type '" << typeid(ExceptionType).name()
          << "' but got different exception type";
      if (!message.empty()) {
        oss << " - " << message;
      }
      throw AssertionFailure(oss.str());
    }

    if (!exception_thrown) {
      std::ostringstream oss;
      oss << "Assertion failed: expected exception of type '" << typeid(ExceptionType).name()
          << "' but no exception was thrown";
      if (!message.empty()) {
        oss << " - " << message;
      }
      throw AssertionFailure(oss.str());
    }
  }

  static void assert_no_throw(const std::function<void()>& func, const std::string& message = "");

 private:
  static std::string format_message(const std::string& assertion, const std::string& message);
};

// Convenience macros
#define ASSERT_TRUE(condition) SolarSystem::Testing::Assertions::assert_true(condition, #condition)
#define ASSERT_FALSE(condition) \
  SolarSystem::Testing::Assertions::assert_false(condition, #condition)
#define ASSERT_EQ(expected, actual) \
  SolarSystem::Testing::Assertions::assert_equals(expected, actual, #expected " == " #actual)
#define ASSERT_NE(expected, actual) \
  SolarSystem::Testing::Assertions::assert_not_equals(expected, actual, #expected " != " #actual)
#define ASSERT_NEAR(expected, actual, tolerance)                             \
  SolarSystem::Testing::Assertions::assert_near(expected, actual, tolerance, \
                                                #expected " ≈ " #actual)
#define ASSERT_GT(actual, threshold) \
  SolarSystem::Testing::Assertions::assert_greater_than(actual, threshold, #actual " > " #threshold)
#define ASSERT_LT(actual, threshold) \
  SolarSystem::Testing::Assertions::assert_less_than(actual, threshold, #actual " < " #threshold)
#define ASSERT_CONTAINS(haystack, needle)                             \
  SolarSystem::Testing::Assertions::assert_contains(haystack, needle, \
                                                    #haystack " contains " #needle)
#define ASSERT_THROWS(exception_type, func)                        \
  SolarSystem::Testing::Assertions::assert_throws<exception_type>( \
      func, #func " throws " #exception_type)
#define ASSERT_NO_THROW(func) \
  SolarSystem::Testing::Assertions::assert_no_throw(func, #func " does not throw")

}  // namespace SolarSystem::Testing
