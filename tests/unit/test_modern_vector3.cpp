/**
 * @file test_modern_vector3.cpp
 * @brief Unit tests for modern Vector3 class
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/math/constants.hpp"
#include "solar_core/math/vector3.hpp"

using namespace SolarSystem::Math;

// ============================================================================
// Constructor Tests
// ============================================================================

TEST(ModernVector3, DefaultConstructor) {
  Vector3d v;
  EXPECT_DOUBLE_EQ(0.0, v.x());
  EXPECT_DOUBLE_EQ(0.0, v.y());
  EXPECT_DOUBLE_EQ(0.0, v.z());
  EXPECT_TRUE(v.is_zero());
}

TEST(ModernVector3, ParameterizedConstructor) {
  Vector3d v(1.0, 2.0, 3.0);
  EXPECT_DOUBLE_EQ(1.0, v.x());
  EXPECT_DOUBLE_EQ(2.0, v.y());
  EXPECT_DOUBLE_EQ(3.0, v.z());
  EXPECT_FALSE(v.is_zero());
}

TEST(ModernVector3, CopyConstructor) {
  Vector3d v1(1.0, 2.0, 3.0);
  Vector3d v2(v1);
  EXPECT_EQ(v1.x(), v2.x());
  EXPECT_EQ(v1.y(), v2.y());
  EXPECT_EQ(v1.z(), v2.z());
}

TEST(ModernVector3, AssignmentOperator) {
  Vector3d v1(1.0, 2.0, 3.0);
  Vector3d v2;
  v2 = v1;
  EXPECT_EQ(v1.x(), v2.x());
  EXPECT_EQ(v1.y(), v2.y());
  EXPECT_EQ(v1.z(), v2.z());
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST(ModernVector3, EqualityComparison) {
  Vector3d v1(1.0, 2.0, 3.0);
  Vector3d v2(1.0, 2.0, 3.0);
  Vector3d v3(1.0, 2.0, 4.0);

  EXPECT_TRUE(v1 == v2);
  EXPECT_FALSE(v1 == v3);
  EXPECT_TRUE(v1 != v3);
}

// ============================================================================
// Arithmetic Tests
// ============================================================================

TEST(ModernVector3, VectorAddition) {
  Vector3d v1(1.0, 2.0, 3.0);
  Vector3d v2(4.0, 5.0, 6.0);
  Vector3d result = v1 + v2;

  EXPECT_DOUBLE_EQ(5.0, result.x());
  EXPECT_DOUBLE_EQ(7.0, result.y());
  EXPECT_DOUBLE_EQ(9.0, result.z());
}

TEST(ModernVector3, VectorSubtraction) {
  Vector3d v1(4.0, 5.0, 6.0);
  Vector3d v2(1.0, 2.0, 3.0);
  Vector3d result = v1 - v2;

  EXPECT_DOUBLE_EQ(3.0, result.x());
  EXPECT_DOUBLE_EQ(3.0, result.y());
  EXPECT_DOUBLE_EQ(3.0, result.z());
}

TEST(ModernVector3, ScalarMultiplication) {
  Vector3d v(1.0, 2.0, 3.0);
  Vector3d result = v * 2.0;

  EXPECT_DOUBLE_EQ(2.0, result.x());
  EXPECT_DOUBLE_EQ(4.0, result.y());
  EXPECT_DOUBLE_EQ(6.0, result.z());

  // Test commutative property
  Vector3d result2 = 2.0L * v;
  EXPECT_TRUE(result == result2);
}

TEST(ModernVector3, ScalarDivision) {
  Vector3d v(2.0, 4.0, 6.0);
  Vector3d result = v / 2.0;

  EXPECT_DOUBLE_EQ(1.0, result.x());
  EXPECT_DOUBLE_EQ(2.0, result.y());
  EXPECT_DOUBLE_EQ(3.0, result.z());
}

// ============================================================================
// Vector Operations Tests
// ============================================================================

TEST(ModernVector3, DotProduct) {
  Vector3d v1(1.0, 2.0, 3.0);
  Vector3d v2(4.0, 5.0, 6.0);
  double result = static_cast<double>(v1.dot(v2));

  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
  EXPECT_DOUBLE_EQ(32.0, result);
}

TEST(ModernVector3, CrossProduct) {
  Vector3d v1(1.0, 0.0, 0.0);
  Vector3d v2(0.0, 1.0, 0.0);
  Vector3d result = v1.cross(v2);

  EXPECT_DOUBLE_EQ(0.0, result.x());
  EXPECT_DOUBLE_EQ(0.0, result.y());
  EXPECT_DOUBLE_EQ(1.0, result.z());
}

TEST(ModernVector3, Magnitude) {
  Vector3d v(3.0, 4.0, 0.0);
  double mag = static_cast<double>(v.magnitude());

  EXPECT_DOUBLE_EQ(5.0, mag);  // 3-4-5 triangle
  EXPECT_DOUBLE_EQ(25.0, v.magnitude_squared());
}

TEST(ModernVector3, Normalization) {
  Vector3d v(3.0, 4.0, 0.0);
  Vector3d normalized = v.normalized();

  EXPECT_NEAR(0.6, normalized.x(), 1e-10);
  EXPECT_NEAR(0.8, normalized.y(), 1e-10);
  EXPECT_NEAR(0.0, normalized.z(), 1e-10);

  // Normalized vector should have magnitude 1
  double mag = static_cast<double>(normalized.magnitude());
  EXPECT_NEAR(1.0, mag, 1e-10);
}

TEST(ModernVector3, ZeroVectorNormalization) {
  Vector3d zero;
  Vector3d normalized = zero.normalized();

  EXPECT_TRUE(normalized.is_zero());
}

TEST(ModernVector3, UnaryMinus) {
  Vector3d v(1.0, -2.0, 3.0);
  Vector3d negated = -v;

  EXPECT_DOUBLE_EQ(-1.0, negated.x());
  EXPECT_DOUBLE_EQ(2.0, negated.y());
  EXPECT_DOUBLE_EQ(-3.0, negated.z());
}

// ============================================================================
// Compound Assignment Tests
// ============================================================================

TEST(ModernVector3, CompoundAssignment) {
  Vector3d v1(1.0, 2.0, 3.0);
  Vector3d v2(4.0, 5.0, 6.0);

  v1 += v2;
  EXPECT_DOUBLE_EQ(5.0, v1.x());
  EXPECT_DOUBLE_EQ(7.0, v1.y());
  EXPECT_DOUBLE_EQ(9.0, v1.z());

  v1 -= v2;
  EXPECT_DOUBLE_EQ(1.0, v1.x());
  EXPECT_DOUBLE_EQ(2.0, v1.y());
  EXPECT_DOUBLE_EQ(3.0, v1.z());

  v1 *= 2.0;
  EXPECT_DOUBLE_EQ(2.0, v1.x());
  EXPECT_DOUBLE_EQ(4.0, v1.y());
  EXPECT_DOUBLE_EQ(6.0, v1.z());

  v1 /= 2.0;
  EXPECT_DOUBLE_EQ(1.0, v1.x());
  EXPECT_DOUBLE_EQ(2.0, v1.y());
  EXPECT_DOUBLE_EQ(3.0, v1.z());
}

// ============================================================================
// Utility Function Tests
// ============================================================================

TEST(ModernVector3, DistanceFunctions) {
  Vector3d v1(0.0, 0.0, 0.0);
  Vector3d v2(3.0, 4.0, 0.0);

  double dist = static_cast<double>(distance(v1, v2));
  double dist_sq = static_cast<double>(distance_squared(v1, v2));

  EXPECT_DOUBLE_EQ(5.0, dist);
  EXPECT_DOUBLE_EQ(25.0, dist_sq);
}

TEST(ModernVector3, StringRepresentation) {
  Vector3d v(1.5, 2.5, 3.5);
  std::string str = to_string(v);

  // Should contain the coordinates
  EXPECT_NE(std::string::npos, str.find("1.5"));
  EXPECT_NE(std::string::npos, str.find("2.5"));
  EXPECT_NE(std::string::npos, str.find("3.5"));
}

TEST(ModernVector3, FloatTypeCompatibility) {
  Vector3f vf(1.0f, 2.0f, 3.0f);
  EXPECT_FLOAT_EQ(1.0f, vf.x());
  EXPECT_FLOAT_EQ(2.0f, vf.y());
  EXPECT_FLOAT_EQ(3.0f, vf.z());

  float mag = vf.magnitude();
  EXPECT_NEAR(std::sqrt(14.0f), mag, 1e-6f);
}
