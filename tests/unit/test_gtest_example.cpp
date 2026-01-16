/**
 * @file test_gtest_example.cpp
 * @brief Example test file demonstrating native Google Test usage
 *
 * This file serves as a reference for migrating tests from the custom
 * test framework to Google Test. It demonstrates:
 * - Basic TEST macros
 * - TEST_F with fixtures
 * - Parameterized tests
 * - Common assertions
 * - Test utilities integration
 */

#include <gtest/gtest.h>

#include "solar_core/math/vector3.hpp"

using namespace SolarSystem::Math;

// ============================================================================
// Basic Tests (TEST macro)
// ============================================================================
// Use TEST(TestSuiteName, TestName) for simple tests without shared setup

TEST(Vector3BasicTests, DefaultConstructor) {
  Vector3d v;
  EXPECT_DOUBLE_EQ(0.0, v.x());
  EXPECT_DOUBLE_EQ(0.0, v.y());
  EXPECT_DOUBLE_EQ(0.0, v.z());
  EXPECT_TRUE(v.is_zero());
}

TEST(Vector3BasicTests, ParameterizedConstructor) {
  Vector3d v(1.0, 2.0, 3.0);
  EXPECT_DOUBLE_EQ(1.0, v.x());
  EXPECT_DOUBLE_EQ(2.0, v.y());
  EXPECT_DOUBLE_EQ(3.0, v.z());
  EXPECT_FALSE(v.is_zero());
}

TEST(Vector3BasicTests, CopyConstructor) {
  Vector3d v1(1.0, 2.0, 3.0);
  Vector3d v2(v1);
  EXPECT_EQ(v1.x(), v2.x());
  EXPECT_EQ(v1.y(), v2.y());
  EXPECT_EQ(v1.z(), v2.z());
}

// ============================================================================
// Test Fixtures (TEST_F macro)
// ============================================================================
// Use TEST_F when multiple tests share common setup/teardown

class Vector3OperationsTest : public ::testing::Test {
 protected:
  // Called before each test
  void SetUp() override {
    v1_ = Vector3d(1.0, 2.0, 3.0);
    v2_ = Vector3d(4.0, 5.0, 6.0);
    zero_ = Vector3d();
  }

  // Called after each test (optional)
  void TearDown() override {
    // Cleanup if needed
  }

  // Shared test data
  Vector3d v1_;
  Vector3d v2_;
  Vector3d zero_;
};

TEST_F(Vector3OperationsTest, Addition) {
  Vector3d result = v1_ + v2_;
  EXPECT_DOUBLE_EQ(5.0, result.x());
  EXPECT_DOUBLE_EQ(7.0, result.y());
  EXPECT_DOUBLE_EQ(9.0, result.z());
}

TEST_F(Vector3OperationsTest, Subtraction) {
  Vector3d result = v2_ - v1_;
  EXPECT_DOUBLE_EQ(3.0, result.x());
  EXPECT_DOUBLE_EQ(3.0, result.y());
  EXPECT_DOUBLE_EQ(3.0, result.z());
}

TEST_F(Vector3OperationsTest, ScalarMultiplication) {
  Vector3d result = v1_ * 2.0;
  EXPECT_DOUBLE_EQ(2.0, result.x());
  EXPECT_DOUBLE_EQ(4.0, result.y());
  EXPECT_DOUBLE_EQ(6.0, result.z());
}

TEST_F(Vector3OperationsTest, DotProduct) {
  double result = static_cast<double>(v1_.dot(v2_));
  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
  EXPECT_DOUBLE_EQ(32.0, result);
}

TEST_F(Vector3OperationsTest, CrossProduct) {
  Vector3d x_axis(1.0, 0.0, 0.0);
  Vector3d y_axis(0.0, 1.0, 0.0);
  Vector3d result = x_axis.cross(y_axis);

  EXPECT_DOUBLE_EQ(0.0, result.x());
  EXPECT_DOUBLE_EQ(0.0, result.y());
  EXPECT_DOUBLE_EQ(1.0, result.z());
}

TEST_F(Vector3OperationsTest, Magnitude) {
  Vector3d v(3.0, 4.0, 0.0);
  double mag = static_cast<double>(v.magnitude());
  EXPECT_DOUBLE_EQ(5.0, mag);  // 3-4-5 triangle
}

TEST_F(Vector3OperationsTest, Normalization) {
  Vector3d v(3.0, 4.0, 0.0);
  Vector3d normalized = v.normalized();

  EXPECT_NEAR(0.6, normalized.x(), 1e-10);
  EXPECT_NEAR(0.8, normalized.y(), 1e-10);
  EXPECT_NEAR(0.0, normalized.z(), 1e-10);

  // Normalized vector should have magnitude 1
  double mag = static_cast<double>(normalized.magnitude());
  EXPECT_NEAR(1.0, mag, 1e-10);
}

TEST_F(Vector3OperationsTest, ZeroVectorNormalization) {
  Vector3d normalized = zero_.normalized();
  EXPECT_TRUE(normalized.is_zero());
}

// ============================================================================
// Parameterized Tests
// ============================================================================
// Use for testing the same logic with different inputs

class Vector3MagnitudeTest
    : public ::testing::TestWithParam<std::tuple<double, double, double, double>> {};

TEST_P(Vector3MagnitudeTest, CalculatesMagnitudeCorrectly) {
  auto [x, y, z, expected_mag] = GetParam();
  Vector3d v(x, y, z);
  double mag = static_cast<double>(v.magnitude());
  EXPECT_NEAR(expected_mag, mag, 1e-10);
}

INSTANTIATE_TEST_SUITE_P(MagnitudeValues, Vector3MagnitudeTest,
                         ::testing::Values(std::make_tuple(3.0, 4.0, 0.0, 5.0),  // 3-4-5 triangle
                                           std::make_tuple(0.0, 0.0, 0.0, 0.0),  // Zero vector
                                           std::make_tuple(1.0, 0.0, 0.0, 1.0),  // Unit x
                                           std::make_tuple(0.0, 1.0, 0.0, 1.0),  // Unit y
                                           std::make_tuple(0.0, 0.0, 1.0, 1.0),  // Unit z
                                           std::make_tuple(1.0, 1.0, 1.0,
                                                           std::sqrt(3.0))  // Diagonal
                                           ));

// ============================================================================
// Assertion Examples
// ============================================================================
// Demonstrates various Google Test assertions

TEST(AssertionExamples, BooleanAssertions) {
  EXPECT_TRUE(true);
  EXPECT_FALSE(false);
  ASSERT_TRUE(true);    // Fatal - stops test on failure
  ASSERT_FALSE(false);  // Fatal
}

TEST(AssertionExamples, EqualityAssertions) {
  EXPECT_EQ(1, 1);
  EXPECT_NE(1, 2);
  EXPECT_LT(1, 2);
  EXPECT_LE(1, 1);
  EXPECT_GT(2, 1);
  EXPECT_GE(2, 2);
}

TEST(AssertionExamples, FloatingPointAssertions) {
  EXPECT_FLOAT_EQ(1.0f, 1.0f);
  EXPECT_DOUBLE_EQ(1.0, 1.0);
  EXPECT_NEAR(1.0, 1.001, 0.01);  // Within tolerance
}

TEST(AssertionExamples, StringAssertions) {
  std::string s = "hello";
  EXPECT_STREQ("hello", s.c_str());
  EXPECT_STRNE("world", s.c_str());
  EXPECT_EQ("hello", s);
}

TEST(AssertionExamples, PointerAssertions) {
  int* ptr = nullptr;
  EXPECT_EQ(nullptr, ptr);

  int value = 42;
  ptr = &value;
  EXPECT_NE(nullptr, ptr);
}

// ============================================================================
// Death Tests (for testing crashes/assertions)
// ============================================================================
// Use sparingly - tests that code properly handles fatal errors

// Example (commented out as it requires specific setup):
// TEST(DeathTests, DivisionByZeroHandled) {
//   EXPECT_DEATH(divide(1, 0), "division by zero");
// }

// ============================================================================
// Typed Tests
// ============================================================================
// For testing template classes with multiple types

template <typename T>
class Vector3TypedTest : public ::testing::Test {
 protected:
  using VectorType = Vector3<T>;
};

using TestTypes = ::testing::Types<float, double, long double>;
TYPED_TEST_SUITE(Vector3TypedTest, TestTypes);

TYPED_TEST(Vector3TypedTest, DefaultConstructorCreatesZeroVector) {
  typename TestFixture::VectorType v;
  EXPECT_TRUE(v.is_zero());
}

TYPED_TEST(Vector3TypedTest, MagnitudeCalculation) {
  typename TestFixture::VectorType v(static_cast<TypeParam>(3.0), static_cast<TypeParam>(4.0),
                                     static_cast<TypeParam>(0.0));
  TypeParam mag = v.magnitude();
  EXPECT_NEAR(static_cast<TypeParam>(5.0), mag, static_cast<TypeParam>(1e-5));
}
