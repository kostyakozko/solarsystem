/**
 * @file test_constants.cpp
 * @brief Unit tests for physical constants
 */

#include "../../lib/solar_core/constants.h"
#include "test_framework.h"

int main() {
  TEST_SUITE("Constants Tests");

  // Test physical constants
  TEST_CASE("Physical Constants Values") {
    ASSERT_GT(G, 6.6e-11);
    ASSERT_LT(G, 6.7e-11);

    ASSERT_GT(AU, 1.4e11);
    ASSERT_LT(AU, 1.5e11);
  });

  return current_suite->all_passed() ? 0 : 1;
}
