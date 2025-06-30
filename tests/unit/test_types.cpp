/**
 * @file test_types.cpp
 * @brief Unit tests for type definitions
 */

#include "../../lib/solar_core/types.h"
#include "test_framework.h"

int main() {
  TEST_SUITE("Types Tests");

  TEST_CASE("Coordinate Structure") {
    coord pos = {1.0, 2.0, 3.0};
    ASSERT_EQ(pos.x, 1.0);
    ASSERT_EQ(pos.y, 2.0);
    ASSERT_EQ(pos.z, 3.0);
  });

  return current_suite->all_passed() ? 0 : 1;
}
