/**
 * @file test_time_mock.cpp
 * @brief Unit tests for TimeMock functionality
 */

#include <chrono>
#include <thread>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;
using namespace SolarSystem::Testing::Mocks;

/**
 * @brief Test basic time mock functionality
 */
class TimeMockBasicTest : public TestCase {
 public:
  TimeMockBasicTest()
      : TestCase({"TimeMockBasicTest", "Test basic time mock operations", {"unit", "time_mock"}}) {}

  void run() override {
    TimeMockConfig config;
    config.start_frozen = true;
    config.initial_time = std::chrono::system_clock::from_time_t(1000000000);  // Jan 9, 2001

    TimeMock time_mock(config);

    // Test initial time
    auto initial_time = time_mock.now();
    auto expected_initial = std::chrono::system_clock::from_time_t(1000000000);
    auto time_diff = std::abs(
        std::chrono::duration_cast<std::chrono::seconds>(initial_time - expected_initial).count());
    assert_true(time_diff < 2, "Initial time should match config");

    // Test time is frozen
    assert_true(time_mock.is_time_frozen(), "Time should be frozen initially");

    // Test advance time
    time_mock.advance_time_s(60.0);  // Advance 1 minute
    auto advanced_time = time_mock.now();
    auto advance_diff = std::abs(
        std::chrono::duration_cast<std::chrono::seconds>(advanced_time - initial_time).count() -
        60);
    assert_true(advance_diff < 2, "Time should advance by 60 seconds");

    // Test unfreeze time
    time_mock.unfreeze_time();
    assert_false(time_mock.is_time_frozen(), "Time should not be frozen after unfreeze");

    // Test time scale
    time_mock.set_time_scale(2.0);
    assert_equals(2.0, time_mock.get_time_scale(), "Time scale should be 2.0");
  }
};

/**
 * @brief Test time mock sleep functionality
 */
class TimeMockSleepTest : public TestCase {
 public:
  TimeMockSleepTest()
      : TestCase({"TimeMockSleepTest", "Test time mock sleep operations", {"unit", "time_mock"}}) {}

  void run() override {
    TimeMockConfig config;
    config.simulate_sleep_delays = false;  // Don't actually sleep
    config.start_frozen = true;

    TimeMock time_mock(config);

    auto start_time = time_mock.now();

    // Test sleep_for
    time_mock.sleep_for(std::chrono::seconds(30));

    // Verify operation was recorded
    assert_equals(static_cast<size_t>(1), time_mock.operation_count(MockTimeOperationType::Sleep),
                  "Sleep operation should be recorded");

    // Test sleep_ms
    time_mock.sleep_ms(5000);  // 5 seconds
    assert_equals(static_cast<size_t>(2), time_mock.operation_count(MockTimeOperationType::Sleep),
                  "Second sleep operation should be recorded");

    // Test total sleep duration
    auto total_sleep = time_mock.total_sleep_duration();
    auto expected_sleep = std::chrono::seconds(35);
    assert_true(std::abs(total_sleep.count() - expected_sleep.count()) < 0.1,
                "Total sleep duration should be ~35 seconds");
  }
};

/**
 * @brief Test time mock formatting functionality
 */
class TimeMockFormatTest : public TestCase {
 public:
  TimeMockFormatTest()
      : TestCase(
            {"TimeMockFormatTest", "Test time mock formatting operations", {"unit", "time_mock"}}) {
  }

  void run() override {
    TimeMock time_mock;

    auto test_time = std::chrono::system_clock::from_time_t(1000000000);  // Jan 9, 2001

    // Test ISO 8601 formatting
    auto iso_string = time_mock.format_iso8601(test_time);
    assert_false(iso_string.empty(), "ISO 8601 string should not be empty");

    // Test parsing ISO 8601
    auto parsed_time = time_mock.parse_iso8601(iso_string);
    assert_true(parsed_time.has_value(), "Should be able to parse ISO 8601 string");

    // Test Julian Date conversion
    auto jd = time_mock.to_julian_date(test_time);
    assert_true(jd > 2400000.0, "Julian Date should be reasonable");

    auto from_jd = time_mock.from_julian_date(jd);
    auto time_diff =
        std::abs(std::chrono::duration_cast<std::chrono::seconds>(from_jd - test_time).count());
    assert_true(time_diff < 2, "Round-trip Julian Date conversion should be accurate");
  }
};

/**
 * @brief Test time mock factory functionality
 */
class TimeMockFactoryTest : public TestCase {
 public:
  TimeMockFactoryTest()
      : TestCase({"TimeMockFactoryTest", "Test time mock factory methods", {"unit", "time_mock"}}) {
  }

  void run() override {
    // Test default factory
    auto default_mock = TimeMockFactory::create_default();
    assert_true(default_mock.get() != nullptr, "Default mock should be created");
    assert_false(default_mock->is_time_frozen(), "Default mock should not be frozen");

    // Test accelerated time factory
    auto accelerated_mock = TimeMockFactory::create_with_accelerated_time(5.0);
    assert_true(accelerated_mock.get() != nullptr, "Accelerated mock should be created");
    assert_equals(5.0, accelerated_mock->get_time_scale(),
                  "Accelerated mock should have correct scale");

    // Test performance testing factory
    auto perf_mock = TimeMockFactory::create_for_performance_testing();
    assert_true(perf_mock.get() != nullptr, "Performance mock should be created");
    assert_true(perf_mock->is_time_frozen(), "Performance mock should be frozen");

    // Test historical simulation factory
    auto historical_time = std::chrono::system_clock::from_time_t(946684800);  // Y2K
    auto historical_mock = TimeMockFactory::create_for_historical_simulation(historical_time);
    assert_true(historical_mock.get() != nullptr, "Historical mock should be created");

    auto mock_time = historical_mock->now();
    auto time_diff = std::abs(
        std::chrono::duration_cast<std::chrono::seconds>(mock_time - historical_time).count());
    assert_true(time_diff < 2, "Historical mock should start at correct time");
  }
};

/**
 * @brief Test scoped time mock functionality
 */
class ScopedTimeMockTest : public TestCase {
 public:
  ScopedTimeMockTest()
      : TestCase({"ScopedTimeMockTest", "Test scoped time mock RAII", {"unit", "time_mock"}}) {}

  void run() override {
    auto original_mock = TimeMockFactory::create_default();

    {
      // Create scoped mock
      ScopedTimeMock scoped_mock(TimeMockFactory::create_for_performance_testing());

      // Test access to mock
      assert_true(scoped_mock.mock().is_time_frozen(), "Scoped mock should be frozen");

      // Test const access
      const auto& const_scoped = scoped_mock;
      assert_true(const_scoped.mock().is_time_frozen(), "Const scoped mock should be frozen");
    }

    // Scoped mock should be cleaned up automatically
    // This test mainly verifies compilation and basic functionality
  }
};

// Register all tests
SOLAR_REGISTER_TEST_WITH_TAGS(TimeMockBasicTest, "unit", "time_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(TimeMockSleepTest, "unit", "time_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(TimeMockFormatTest, "unit", "time_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(TimeMockFactoryTest, "unit", "time_mock");
SOLAR_REGISTER_TEST_WITH_TAGS(ScopedTimeMockTest, "unit", "time_mock");

SOLAR_TEST_MAIN();
