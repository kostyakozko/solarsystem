#pragma once

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace SolarSystem::Utils {

/**
 * @brief C++20 compatible Expected type for error handling without exceptions
 *
 * This is a simplified version of std::expected (C++23) that works with C++20.
 * It represents a value that can either contain a successful result or an error.
 *
 * @tparam T The success type
 * @tparam E The error type
 */
template <typename T, typename E>
class Expected {
 public:
  // Constructors for success case
  Expected(const T& value) : has_value_(true) { new (&storage_.value) T(value); }

  Expected(T&& value) : has_value_(true) { new (&storage_.value) T(std::move(value)); }

  // Constructor for error case
  Expected(const E& error) : has_value_(false) { new (&storage_.error) E(error); }

  Expected(E&& error) : has_value_(false) { new (&storage_.error) E(std::move(error)); }

  // Copy constructor
  Expected(const Expected& other) : has_value_(other.has_value_) {
    if (has_value_) {
      new (&storage_.value) T(other.storage_.value);
    } else {
      new (&storage_.error) E(other.storage_.error);
    }
  }

  // Move constructor
  Expected(Expected&& other) noexcept : has_value_(other.has_value_) {
    if (has_value_) {
      new (&storage_.value) T(std::move(other.storage_.value));
    } else {
      new (&storage_.error) E(std::move(other.storage_.error));
    }
  }

  // Destructor
  ~Expected() {
    if (has_value_) {
      storage_.value.~T();
    } else {
      storage_.error.~E();
    }
  }

  // Assignment operators
  Expected& operator=(const Expected& other) {
    if (this != &other) {
      this->~Expected();
      new (this) Expected(other);
    }
    return *this;
  }

  Expected& operator=(Expected&& other) noexcept {
    if (this != &other) {
      this->~Expected();
      new (this) Expected(std::move(other));
    }
    return *this;
  }

  // Check if contains value
  [[nodiscard]] bool has_value() const noexcept { return has_value_; }
  [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }

  // Value access
  [[nodiscard]] const T& value() const& {
    if (!has_value_) {
      throw std::runtime_error("Expected contains error, not value");
    }
    return storage_.value;
  }

  [[nodiscard]] T& value() & {
    if (!has_value_) {
      throw std::runtime_error("Expected contains error, not value");
    }
    return storage_.value;
  }

  [[nodiscard]] T&& value() && {
    if (!has_value_) {
      throw std::runtime_error("Expected contains error, not value");
    }
    return std::move(storage_.value);
  }

  // Error access
  [[nodiscard]] const E& error() const& {
    if (has_value_) {
      throw std::runtime_error("Expected contains value, not error");
    }
    return storage_.error;
  }

  [[nodiscard]] E& error() & {
    if (has_value_) {
      throw std::runtime_error("Expected contains value, not error");
    }
    return storage_.error;
  }

  // Value access with default
  template <typename U>
  [[nodiscard]] T value_or(U&& default_value) const& {
    return has_value_ ? storage_.value : static_cast<T>(std::forward<U>(default_value));
  }

  template <typename U>
  [[nodiscard]] T value_or(U&& default_value) && {
    return has_value_ ? std::move(storage_.value) : static_cast<T>(std::forward<U>(default_value));
  }

 private:
  union Storage {
    T value;
    E error;

    Storage() {}   // Uninitialized
    ~Storage() {}  // Destruction handled by Expected
  } storage_;

  bool has_value_;
};

// Specialization for void success type
template <typename E>
class Expected<void, E> {
 public:
  // Constructor for success case
  Expected() : has_value_(true), error_storage_() {}

  // Constructor for error case
  Expected(const E& error) : has_value_(false) { new (&error_storage_) E(error); }

  Expected(E&& error) : has_value_(false) { new (&error_storage_) E(std::move(error)); }

  // Copy constructor
  Expected(const Expected& other) : has_value_(other.has_value_) {
    if (!has_value_) {
      new (&error_storage_) E(reinterpret_cast<const E&>(other.error_storage_));
    }
  }

  // Move constructor
  Expected(Expected&& other) noexcept : has_value_(other.has_value_) {
    if (!has_value_) {
      new (&error_storage_) E(std::move(reinterpret_cast<E&>(other.error_storage_)));
    }
  }

  // Destructor
  ~Expected() {
    if (!has_value_) {
      reinterpret_cast<E*>(&error_storage_)->~E();
    }
  }

  // Assignment operators
  Expected& operator=(const Expected& other) {
    if (this != &other) {
      this->~Expected();
      new (this) Expected(other);
    }
    return *this;
  }

  Expected& operator=(Expected&& other) noexcept {
    if (this != &other) {
      this->~Expected();
      new (this) Expected(std::move(other));
    }
    return *this;
  }

  // Check if contains value
  [[nodiscard]] bool has_value() const noexcept { return has_value_; }
  [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }

  // Error access
  [[nodiscard]] const E& error() const& {
    if (has_value_) {
      throw std::runtime_error("Expected contains success, not error");
    }
    return *reinterpret_cast<const E*>(&error_storage_);
  }

  [[nodiscard]] E& error() & {
    if (has_value_) {
      throw std::runtime_error("Expected contains success, not error");
    }
    return *reinterpret_cast<E*>(&error_storage_);
  }

 private:
  bool has_value_;
  std::aligned_storage_t<sizeof(E), alignof(E)>
      error_storage_;  // Only valid when has_value_ is false
};

// Helper function to create success Expected
template <typename T>
[[nodiscard]] Expected<std::decay_t<T>, void> make_expected(T&& value) {
  return Expected<std::decay_t<T>, void>(std::forward<T>(value));
}

// Helper function to create error Expected
template <typename E, typename T = void>
[[nodiscard]] Expected<T, std::decay_t<E>> make_unexpected(E&& error) {
  return Expected<T, std::decay_t<E>>(std::forward<E>(error));
}

}  // namespace SolarSystem::Utils
