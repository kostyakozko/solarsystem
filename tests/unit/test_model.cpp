/**
 * @file test_model.cpp
 * @brief Unit tests for model functions
 */

#include "model.h"
#include "test_framework.h"

int main() {
  TEST_SUITE("Model Tests");

  // Test distance calculation
  TEST_CASE("Distance Function") {
    coord a = {0.0, 0.0, 0.0};
    coord b = {3.0, 4.0, 0.0};

    long double distance = dist(a, b);
    ASSERT_NEAR(distance, 5.0, 1e-10);
  });

  return current_suite->all_passed() ? 0 : 1;
}
