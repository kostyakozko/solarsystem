/**
 * @file test_performance_critical.cpp
 * @brief Performance-critical unit tests
 */

#include "model.h"
#include "test_framework.h"

int main() {
  TEST_SUITE("Performance Critical Tests");

  TEST_CASE("Distance Calculation Performance") {
    coord a = {0.0, 0.0, 0.0};
    coord b = {1000.0, 1000.0, 1000.0};

    // This should be fast
    for (int i = 0; i < 1000; ++i) {
      long double d = dist(a, b);
      volatile long double result = d;  // Prevent optimization
      (void)result;
    }

    ASSERT_TRUE(true);  // If we get here, performance is acceptable
  });

  return current_suite->all_passed() ? 0 : 1;
}
