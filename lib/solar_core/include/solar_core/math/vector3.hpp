#pragma once

#include <cmath>
#include <compare>
#include <concepts>

namespace SolarSystem::Math {

/**
 * @brief Modern 3D vector class with C++20 concepts and constexpr support
 *
 * @tparam T Floating point type (float, double, long double)
 */
template <std::floating_point T = double>
class Vector3 {
 public:
  // Constructors
  constexpr Vector3() = default;
  constexpr Vector3(T x, T y, T z) noexcept : x_(x), y_(y), z_(z) {}

  // Copy/move constructors and assignment operators
  constexpr Vector3(const Vector3&) = default;
  constexpr Vector3(Vector3&&) = default;
  constexpr Vector3& operator=(const Vector3&) = default;
  constexpr Vector3& operator=(Vector3&&) = default;

  // Destructor
  ~Vector3() = default;

  // C++20 three-way comparison
  constexpr auto operator<=>(const Vector3&) const = default;
  constexpr bool operator==(const Vector3&) const = default;

  // Accessors
  [[nodiscard]] constexpr T x() const noexcept { return x_; }
  [[nodiscard]] constexpr T y() const noexcept { return y_; }
  [[nodiscard]] constexpr T z() const noexcept { return z_; }

  // Mutators
  constexpr void set_x(T x) noexcept { x_ = x; }
  constexpr void set_y(T y) noexcept { y_ = y; }
  constexpr void set_z(T z) noexcept { z_ = z; }
  constexpr void set(T x, T y, T z) noexcept {
    x_ = x;
    y_ = y;
    z_ = z;
  }

  // Mathematical operations
  [[nodiscard]] constexpr Vector3 operator+(const Vector3& other) const noexcept {
    return Vector3{x_ + other.x_, y_ + other.y_, z_ + other.z_};
  }

  [[nodiscard]] constexpr Vector3 operator-(const Vector3& other) const noexcept {
    return Vector3{x_ - other.x_, y_ - other.y_, z_ - other.z_};
  }

  [[nodiscard]] constexpr Vector3 operator*(T scalar) const noexcept {
    return Vector3{x_ * scalar, y_ * scalar, z_ * scalar};
  }

  [[nodiscard]] constexpr Vector3 operator/(T scalar) const noexcept {
    return Vector3{x_ / scalar, y_ / scalar, z_ / scalar};
  }

  constexpr Vector3& operator+=(const Vector3& other) noexcept {
    x_ += other.x_;
    y_ += other.y_;
    z_ += other.z_;
    return *this;
  }

  constexpr Vector3& operator-=(const Vector3& other) noexcept {
    x_ -= other.x_;
    y_ -= other.y_;
    z_ -= other.z_;
    return *this;
  }

  constexpr Vector3& operator*=(T scalar) noexcept {
    x_ *= scalar;
    y_ *= scalar;
    z_ *= scalar;
    return *this;
  }

  constexpr Vector3& operator/=(T scalar) noexcept {
    x_ /= scalar;
    y_ /= scalar;
    z_ /= scalar;
    return *this;
  }

  // Vector operations
  [[nodiscard]] constexpr T dot(const Vector3& other) const noexcept {
    return x_ * other.x_ + y_ * other.y_ + z_ * other.z_;
  }

  [[nodiscard]] constexpr Vector3 cross(const Vector3& other) const noexcept {
    return Vector3{y_ * other.z_ - z_ * other.y_, z_ * other.x_ - x_ * other.z_,
                   x_ * other.y_ - y_ * other.x_};
  }

  [[nodiscard]] constexpr T magnitude_squared() const noexcept {
    return x_ * x_ + y_ * y_ + z_ * z_;
  }

  [[nodiscard]] T magnitude() const noexcept { return std::sqrt(magnitude_squared()); }

  [[nodiscard]] Vector3 normalized() const noexcept {
    const T mag = magnitude();
    if (mag > T{0}) {
      return *this / mag;
    }
    return Vector3{};
  }

  [[nodiscard]] constexpr bool is_zero() const noexcept {
    return x_ == T{0} && y_ == T{0} && z_ == T{0};
  }

  // Unary operators
  [[nodiscard]] constexpr Vector3 operator-() const noexcept { return Vector3{-x_, -y_, -z_}; }

 private:
  T x_{};
  T y_{};
  T z_{};
};

// Type aliases for common use cases
using Vector3d = Vector3<double>;
using Vector3f = Vector3<float>;

// Free functions for scalar multiplication (scalar * vector)
template <std::floating_point T>
[[nodiscard]] constexpr Vector3<T> operator*(T scalar, const Vector3<T>& vec) noexcept {
  return vec * scalar;
}

// Distance function
template <std::floating_point T>
[[nodiscard]] T distance(const Vector3<T>& a, const Vector3<T>& b) noexcept {
  return (b - a).magnitude();
}

// Distance squared (more efficient when you don't need the actual distance)
template <std::floating_point T>
[[nodiscard]] constexpr T distance_squared(const Vector3<T>& a, const Vector3<T>& b) noexcept {
  return (b - a).magnitude_squared();
}

}  // namespace SolarSystem::Math
