/**
 * @file test_modern_vector3.cpp
 * @brief Unit tests for modern Vector3 class
 */

#include "../utils/test_framework.h"
#include "solar_core/math/constants.hpp"
#include "solar_core/math/vector3.hpp"

using namespace SolarSystem::Math;

int main() {
  TestSuite suite("Modern Vector3 Tests");

  suite.run_test("Default Constructor", []() {
    Vector3d v;
    ASSERT_EQ(0.0, v.x());
    ASSERT_EQ(0.0, v.y());
    ASSERT_EQ(0.0, v.z());
    ASSERT_TRUE(v.is_zero());
  });

  suite.run_test("Parameterized Constructor", []() {
    Vector3d v(1.0, 2.0, 3.0);
    ASSERT_EQ(1.0, v.x());
    ASSERT_EQ(2.0, v.y());
    ASSERT_EQ(3.0, v.z());
    ASSERT_FALSE(v.is_zero());
  });

  suite.run_test("Copy Constructor", []() {
    Vector3d v1(1.0, 2.0, 3.0);
    Vector3d v2(v1);
    ASSERT_EQ(v1.x(), v2.x());
    ASSERT_EQ(v1.y(), v2.y());
    ASSERT_EQ(v1.z(), v2.z());
  });

  suite.run_test("Assignment Operator", []() {
    Vector3d v1(1.0, 2.0, 3.0);
    Vector3d v2;
    v2 = v1;
    ASSERT_EQ(v1.x(), v2.x());
    ASSERT_EQ(v1.y(), v2.y());
    ASSERT_EQ(v1.z(), v2.z());
  });

  suite.run_test("Equality Comparison", []() {
    Vector3d v1(1.0, 2.0, 3.0);
    Vector3d v2(1.0, 2.0, 3.0);
    Vector3d v3(1.0, 2.0, 4.0);

    ASSERT_TRUE(v1 == v2);
    ASSERT_FALSE(v1 == v3);
    ASSERT_TRUE(v1 != v3);
  });

  suite.run_test("Vector Addition", []() {
    Vector3d v1(1.0, 2.0, 3.0);
    Vector3d v2(4.0, 5.0, 6.0);
    Vector3d result = v1 + v2;

    ASSERT_EQ(5.0, result.x());
    ASSERT_EQ(7.0, result.y());
    ASSERT_EQ(9.0, result.z());
  });

  suite.run_test("Vector Subtraction", []() {
    Vector3d v1(4.0, 5.0, 6.0);
    Vector3d v2(1.0, 2.0, 3.0);
    Vector3d result = v1 - v2;

    ASSERT_EQ(3.0, result.x());
    ASSERT_EQ(3.0, result.y());
    ASSERT_EQ(3.0, result.z());
  });

  suite.run_test("Scalar Multiplication", []() {
    Vector3d v(1.0, 2.0, 3.0);
    Vector3d result = v * 2.0;

    ASSERT_EQ(2.0, result.x());
    ASSERT_EQ(4.0, result.y());
    ASSERT_EQ(6.0, result.z());

    // Test commutative property
    Vector3d result2 = 2.0 * v;
    ASSERT_TRUE(result == result2);
  });

  suite.run_test("Scalar Division", []() {
    Vector3d v(2.0, 4.0, 6.0);
    Vector3d result = v / 2.0;

    ASSERT_EQ(1.0, result.x());
    ASSERT_EQ(2.0, result.y());
    ASSERT_EQ(3.0, result.z());
  });

  suite.run_test("Dot Product", []() {
    Vector3d v1(1.0, 2.0, 3.0);
    Vector3d v2(4.0, 5.0, 6.0);
    double result = v1.dot(v2);

    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    ASSERT_EQ(32.0, result);
  });

  suite.run_test("Cross Product", []() {
    Vector3d v1(1.0, 0.0, 0.0);
    Vector3d v2(0.0, 1.0, 0.0);
    Vector3d result = v1.cross(v2);

    ASSERT_EQ(0.0, result.x());
    ASSERT_EQ(0.0, result.y());
    ASSERT_EQ(1.0, result.z());
  });

  suite.run_test("Magnitude", []() {
    Vector3d v(3.0, 4.0, 0.0);
    double mag = v.magnitude();

    ASSERT_EQ(5.0, mag);  // 3-4-5 triangle
    ASSERT_EQ(25.0, v.magnitude_squared());
  });

  suite.run_test("Normalization", []() {
    Vector3d v(3.0, 4.0, 0.0);
    Vector3d normalized = v.normalized();

    ASSERT_EQ(0.6, normalized.x());
    ASSERT_EQ(0.8, normalized.y());
    ASSERT_EQ(0.0, normalized.z());

    // Normalized vector should have magnitude 1
    double mag = normalized.magnitude();
    ASSERT_TRUE(std::abs(mag - 1.0) < 1e-10);
  });

  suite.run_test("Zero Vector Normalization", []() {
    Vector3d zero;
    Vector3d normalized = zero.normalized();

    ASSERT_TRUE(normalized.is_zero());
  });

  suite.run_test("Unary Minus", []() {
    Vector3d v(1.0, -2.0, 3.0);
    Vector3d negated = -v;

    ASSERT_EQ(-1.0, negated.x());
    ASSERT_EQ(2.0, negated.y());
    ASSERT_EQ(-3.0, negated.z());
  });

  suite.run_test("Compound Assignment", []() {
    Vector3d v1(1.0, 2.0, 3.0);
    Vector3d v2(4.0, 5.0, 6.0);

    v1 += v2;
    ASSERT_EQ(5.0, v1.x());
    ASSERT_EQ(7.0, v1.y());
    ASSERT_EQ(9.0, v1.z());

    v1 -= v2;
    ASSERT_EQ(1.0, v1.x());
    ASSERT_EQ(2.0, v1.y());
    ASSERT_EQ(3.0, v1.z());

    v1 *= 2.0;
    ASSERT_EQ(2.0, v1.x());
    ASSERT_EQ(4.0, v1.y());
    ASSERT_EQ(6.0, v1.z());

    v1 /= 2.0;
    ASSERT_EQ(1.0, v1.x());
    ASSERT_EQ(2.0, v1.y());
    ASSERT_EQ(3.0, v1.z());
  });

  suite.run_test("Distance Functions", []() {
    Vector3d v1(0.0, 0.0, 0.0);
    Vector3d v2(3.0, 4.0, 0.0);

    double dist = distance(v1, v2);
    double dist_sq = distance_squared(v1, v2);

    ASSERT_EQ(5.0, dist);
    ASSERT_EQ(25.0, dist_sq);
  });

  suite.run_test("String Representation", []() {
    Vector3d v(1.5, 2.5, 3.5);
    std::string str = to_string(v);

    // Should contain the coordinates
    ASSERT_TRUE(str.find("1.5") != std::string::npos);
    ASSERT_TRUE(str.find("2.5") != std::string::npos);
    ASSERT_TRUE(str.find("3.5") != std::string::npos);
  });

  suite.run_test("Float Type Compatibility", []() {
    Vector3f vf(1.0f, 2.0f, 3.0f);
    ASSERT_EQ(1.0f, vf.x());
    ASSERT_EQ(2.0f, vf.y());
    ASSERT_EQ(3.0f, vf.z());

    float mag = vf.magnitude();
    ASSERT_TRUE(std::abs(mag - std::sqrt(14.0f)) < 1e-6f);
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
