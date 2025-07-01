#pragma once

#include <optional>
#include <variant>

namespace SolarSystem::Utils {

/**
 * @brief Simple Expected<T, E> implementation for error handling
 *
 * A lightweight alternative to std::expected (C++23) that provides
 * type-safe error handling without exceptions.
 */
template <typename T, typename E>
class Expected {
 public:
  /**
   * @brief Construct a successful result
   */
  static Expected success(T value) {
    Expected result;
    result.value_ = std::move(value);
    return result;
  }

  /**
   * @brief Construct an error result
   */
  static Expected error(E error) {
    Expected result;
    result.value_ = std::move(error);
    return result;
  }

  /**
   * @brief Check if the result contains a value
   */
  bool has_value() const noexcept { return std::holds_alternative<T>(value_); }

  /**
   * @brief Check if the result contains an error
   */
  bool has_error() const noexcept { return std::holds_alternative<E>(value_); }

  /**
   * @brief Get the value (undefined behavior if has_error())
   */
  const T& value() const& { return std::get<T>(value_); }

  /**
   * @brief Get the value (undefined behavior if has_error())
   */
  T& value() & { return std::get<T>(value_); }

  /**
   * @brief Get the value (undefined behavior if has_error())
   */
  T&& value() && { return std::get<T>(std::move(value_)); }

  /**
   * @brief Get the error (undefined behavior if has_value())
   */
  const E& error() const& { return std::get<E>(value_); }

  /**
   * @brief Get the error (undefined behavior if has_value())
   */
  E& error() & { return std::get<E>(value_); }

  /**
   * @brief Get the error (undefined behavior if has_value())
   */
  E&& error() && { return std::get<E>(std::move(value_)); }

  /**
   * @brief Dereference operator (same as value())
   */
  const T& operator*() const& { return value(); }

  /**
   * @brief Dereference operator (same as value())
   */
  T& operator*() & { return value(); }

  /**
   * @brief Dereference operator (same as value())
   */
  T&& operator*() && { return std::move(*this).value(); }

  /**
   * @brief Arrow operator
   */
  const T* operator->() const { return &value(); }

  /**
   * @brief Arrow operator
   */
  T* operator->() { return &value(); }

  /**
   * @brief Boolean conversion (same as has_value())
   */
  explicit operator bool() const noexcept { return has_value(); }

 private:
  std::variant<T, E> value_;
};

/**
 * @brief Specialization for void success type
 */
template <typename E>
class Expected<void, E> {
 public:
  /**
   * @brief Construct a successful result
   */
  static Expected success() {
    Expected result;
    result.has_value_ = true;
    return result;
  }

  /**
   * @brief Construct an error result
   */
  static Expected error(E error) {
    Expected result;
    result.has_value_ = false;
    result.error_ = std::move(error);
    return result;
  }

  /**
   * @brief Check if the result is successful
   */
  bool has_value() const noexcept { return has_value_; }

  /**
   * @brief Check if the result contains an error
   */
  bool has_error() const noexcept { return !has_value_; }

  /**
   * @brief Get the error (undefined behavior if has_value())
   */
  const E& error() const& { return error_; }

  /**
   * @brief Get the error (undefined behavior if has_value())
   */
  E& error() & { return error_; }

  /**
   * @brief Get the error (undefined behavior if has_value())
   */
  E&& error() && { return std::move(error_); }

  /**
   * @brief Boolean conversion (same as has_value())
   */
  explicit operator bool() const noexcept { return has_value(); }

 private:
  bool has_value_ = false;
  E error_;
};

}  // namespace SolarSystem::Utils
