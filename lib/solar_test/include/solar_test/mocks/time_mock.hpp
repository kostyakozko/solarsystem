/**
 * @file time_mock.hpp
 * @brief Time Simulation Mock System for Testing
 *
 * Provides comprehensive time mocking capabilities for time-dependent tests:
 * - Controllable time simulation for deterministic testing
 * - Time travel and time manipulation capabilities
 * - Clock freezing and time acceleration/deceleration
 * - Integration with existing time-dependent components
 * - Support for multiple time zones and epochs
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace SolarSystem::Testing::Mocks {

/**
 * @brief Time operation types for tracking
 */
enum class MockTimeOperationType {
  GetCurrentTime,   // Get current system time
  Sleep,            // Sleep/delay operation
  SetTime,          // Set mock time
  AdvanceTime,      // Advance time by duration
  FreezeTime,       // Freeze time at current value
  UnfreezeTime,     // Resume normal time flow
  SetTimeScale,     // Set time acceleration/deceleration
  GetEpochTime,     // Get epoch-based time
  ConvertTimeZone,  // Time zone conversion
  FormatTime        // Time formatting operation
};

/**
 * @brief Mock time call information for verification
 */
struct MockTimeCallInfo {
  std::chrono::system_clock::time_point timestamp;
  MockTimeOperationType operation_type;
  std::chrono::system_clock::time_point requested_time;
  std::chrono::duration<double> duration_param;
  double time_scale;
  std::string operation_details;
  bool was_successful;
};

/**
 * @brief Configuration for time mock behavior
 */
struct TimeMockConfig {
  // Initial time settings
  std::optional<std::chrono::system_clock::time_point> initial_time;
  bool start_frozen = false;        // Start with time frozen
  double initial_time_scale = 1.0;  // 1.0 = normal speed, 2.0 = 2x speed, etc.

  // Time behavior
  bool allow_negative_time = false;  // Allow time to go backwards
  bool wrap_around_epochs = false;   // Wrap around at epoch boundaries
  std::chrono::system_clock::time_point min_time = std::chrono::system_clock::time_point::min();
  std::chrono::system_clock::time_point max_time = std::chrono::system_clock::time_point::max();

  // Precision and resolution
  std::chrono::nanoseconds time_resolution = std::chrono::microseconds(1);
  bool high_precision_mode = true;  // Use high precision timing

  // Sleep and delay simulation
  bool simulate_sleep_delays = true;  // Actually delay during sleep calls
  double sleep_accuracy = 1.0;        // Sleep accuracy factor (1.0 = perfect)
  std::chrono::milliseconds max_sleep_duration = std::chrono::hours(24);

  // Call tracking
  bool enable_call_history = true;  // Track all time operations
  size_t max_history_size = 10000;  // Maximum number of calls to remember

  // Time zone support
  std::string default_timezone = "UTC";  // Default time zone
  bool support_timezones = true;         // Enable time zone conversions

  // Performance simulation
  std::chrono::nanoseconds operation_delay = std::chrono::nanoseconds(0);
  bool simulate_clock_drift = false;  // Simulate clock drift over time
  double drift_rate_ppm = 0.0;        // Drift rate in parts per million
};

/**
 * @brief Time Mock Implementation
 *
 * Provides comprehensive mocking of time operations for testing purposes.
 * Allows complete control over time flow, enabling deterministic testing
 * of time-dependent functionality.
 */
class TimeMock {
 public:
  /**
   * @brief Construct time mock with configuration
   */
  explicit TimeMock(TimeMockConfig config = {});

  /**
   * @brief Destructor
   */
  ~TimeMock();

  // Non-copyable, non-movable (due to mutex and atomic operations)
  TimeMock(const TimeMock&) = delete;
  TimeMock& operator=(const TimeMock&) = delete;
  TimeMock(TimeMock&&) = delete;
  TimeMock& operator=(TimeMock&&) = delete;

  // === Time Control ===

  /**
   * @brief Set the current mock time
   */
  void set_time(std::chrono::system_clock::time_point time);

  /**
   * @brief Advance time by specified duration
   */
  void advance_time(std::chrono::duration<double> duration);

  /**
   * @brief Advance time by milliseconds
   */
  void advance_time_ms(int64_t milliseconds);

  /**
   * @brief Advance time by seconds
   */
  void advance_time_s(double seconds);

  /**
   * @brief Advance time by minutes
   */
  void advance_time_min(double minutes);

  /**
   * @brief Advance time by hours
   */
  void advance_time_h(double hours);

  /**
   * @brief Advance time by days
   */
  void advance_time_days(double days);

  /**
   * @brief Freeze time at current value
   */
  void freeze_time();

  /**
   * @brief Unfreeze time and resume normal flow
   */
  void unfreeze_time();

  /**
   * @brief Check if time is currently frozen
   */
  [[nodiscard]] bool is_time_frozen() const;

  /**
   * @brief Set time scale (1.0 = normal, 2.0 = 2x speed, 0.5 = half speed)
   */
  void set_time_scale(double scale);

  /**
   * @brief Get current time scale
   */
  [[nodiscard]] double get_time_scale() const;

  /**
   * @brief Reset time to system time and normal flow
   */
  void reset_to_system_time();

  // === Time Queries ===

  /**
   * @brief Get current mock time
   */
  [[nodiscard]] std::chrono::system_clock::time_point now() const;

  /**
   * @brief Get current mock time as time_t
   */
  [[nodiscard]] std::time_t now_time_t() const;

  /**
   * @brief Get current mock time in milliseconds since epoch
   */
  [[nodiscard]] int64_t now_ms() const;

  /**
   * @brief Get current mock time in seconds since epoch
   */
  [[nodiscard]] double now_seconds() const;

  /**
   * @brief Get steady clock time (for duration measurements)
   */
  [[nodiscard]] std::chrono::steady_clock::time_point steady_now() const;

  /**
   * @brief Get high resolution clock time
   */
  [[nodiscard]] std::chrono::high_resolution_clock::time_point high_res_now() const;

  // === Sleep and Delay Simulation ===

  /**
   * @brief Mock sleep for specified duration
   */
  void sleep_for(std::chrono::duration<double> duration);

  /**
   * @brief Mock sleep until specified time point
   */
  void sleep_until(std::chrono::system_clock::time_point time_point);

  /**
   * @brief Mock nanosleep
   */
  void nanosleep(std::chrono::nanoseconds duration);

  /**
   * @brief Mock usleep (microseconds)
   */
  void usleep(std::chrono::microseconds duration);

  /**
   * @brief Mock millisecond sleep
   */
  void sleep_ms(int64_t milliseconds);

  /**
   * @brief Mock second sleep
   */
  void sleep_s(double seconds);

  // === Time Zone Support ===

  /**
   * @brief Convert time to specified time zone
   */
  [[nodiscard]] std::chrono::system_clock::time_point to_timezone(
      std::chrono::system_clock::time_point time, const std::string& timezone) const;

  /**
   * @brief Convert time from specified time zone to UTC
   */
  [[nodiscard]] std::chrono::system_clock::time_point from_timezone(
      std::chrono::system_clock::time_point time, const std::string& timezone) const;

  /**
   * @brief Get current time in specified time zone
   */
  [[nodiscard]] std::chrono::system_clock::time_point now_in_timezone(
      const std::string& timezone) const;

  /**
   * @brief Set default time zone
   */
  void set_default_timezone(const std::string& timezone);

  /**
   * @brief Get default time zone
   */
  [[nodiscard]] const std::string& get_default_timezone() const;

  // === Time Formatting ===

  /**
   * @brief Format time as ISO 8601 string
   */
  [[nodiscard]] std::string format_iso8601(std::chrono::system_clock::time_point time) const;

  /**
   * @brief Format time with custom format string
   */
  [[nodiscard]] std::string format_time(std::chrono::system_clock::time_point time,
                                        const std::string& format) const;

  /**
   * @brief Parse ISO 8601 time string
   */
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_iso8601(
      const std::string& time_str) const;

  /**
   * @brief Parse time string with custom format
   */
  [[nodiscard]] std::optional<std::chrono::system_clock::time_point> parse_time(
      const std::string& time_str, const std::string& format) const;

  // === Astronomical Time Support ===

  /**
   * @brief Convert to Julian Date
   */
  [[nodiscard]] double to_julian_date(std::chrono::system_clock::time_point time) const;

  /**
   * @brief Convert from Julian Date
   */
  [[nodiscard]] std::chrono::system_clock::time_point from_julian_date(double jd) const;

  /**
   * @brief Convert to Modified Julian Date
   */
  [[nodiscard]] double to_modified_julian_date(std::chrono::system_clock::time_point time) const;

  /**
   * @brief Convert from Modified Julian Date
   */
  [[nodiscard]] std::chrono::system_clock::time_point from_modified_julian_date(double mjd) const;

  /**
   * @brief Get sidereal time for given location
   */
  [[nodiscard]] double get_sidereal_time(std::chrono::system_clock::time_point time,
                                         double longitude_deg) const;

  // === Call Verification ===

  /**
   * @brief Get total number of time operations
   */
  [[nodiscard]] size_t operation_count() const;

  /**
   * @brief Get number of operations by type
   */
  [[nodiscard]] size_t operation_count(MockTimeOperationType type) const;

  /**
   * @brief Get complete operation history
   */
  [[nodiscard]] const std::vector<MockTimeCallInfo>& operation_history() const;

  /**
   * @brief Get operations of specific type
   */
  [[nodiscard]] std::vector<MockTimeCallInfo> operations_of_type(MockTimeOperationType type) const;

  /**
   * @brief Check if specific time was requested
   */
  [[nodiscard]] bool was_time_requested(std::chrono::system_clock::time_point time) const;

  /**
   * @brief Get total sleep duration requested
   */
  [[nodiscard]] std::chrono::duration<double> total_sleep_duration() const;

  /**
   * @brief Reset call history and counters
   */
  void reset_call_history();

  // === Configuration Access ===

  /**
   * @brief Get current configuration
   */
  [[nodiscard]] const TimeMockConfig& config() const noexcept { return config_; }

  /**
   * @brief Update configuration
   */
  void update_config(const TimeMockConfig& new_config);

  // === Utility Functions ===

  /**
   * @brief Install this mock as global time mock
   */
  void install_as_global_mock();

  /**
   * @brief Remove global mock installation
   */
  static void remove_global_mock();

  /**
   * @brief Create time-dependent test scenario
   */
  void create_test_scenario(const std::string& scenario_name,
                            std::chrono::system_clock::time_point start_time,
                            std::chrono::duration<double> duration, double time_scale = 1.0);

  /**
   * @brief Execute function with time control
   */
  template <typename Func>
  auto execute_with_time_control(std::chrono::system_clock::time_point start_time,
                                 double time_scale, Func&& func) -> decltype(func());

 private:
  TimeMockConfig config_;

  // Time state
  mutable std::mutex time_mutex_;
  std::atomic<std::chrono::system_clock::time_point> current_time_;
  std::atomic<std::chrono::steady_clock::time_point> steady_start_time_;
  std::atomic<bool> time_frozen_;
  std::atomic<double> time_scale_;
  std::chrono::system_clock::time_point real_start_time_;
  std::chrono::steady_clock::time_point mock_start_time_;

  // Time zone data
  std::string default_timezone_;
  std::map<std::string, int> timezone_offsets_;  // Simplified timezone support

  // Call tracking
  mutable std::mutex call_mutex_;
  mutable std::vector<MockTimeCallInfo> operation_history_;
  mutable std::map<MockTimeOperationType, size_t> operation_counts_;
  mutable std::chrono::duration<double> total_sleep_duration_;

  // Clock drift simulation
  mutable std::atomic<double> drift_accumulator_;
  mutable std::chrono::steady_clock::time_point last_drift_update_;

  // === Internal Helper Methods ===

  /**
   * @brief Record a time operation for verification
   */
  void record_operation(MockTimeOperationType type,
                        std::chrono::system_clock::time_point requested_time,
                        std::chrono::duration<double> duration_param = {},
                        const std::string& details = "") const;

  /**
   * @brief Calculate current time with scale and drift
   */
  [[nodiscard]] std::chrono::system_clock::time_point calculate_current_time() const;

  /**
   * @brief Update clock drift
   */
  void update_clock_drift() const;

  /**
   * @brief Validate time bounds
   */
  [[nodiscard]] std::chrono::system_clock::time_point validate_time_bounds(
      std::chrono::system_clock::time_point time) const;

  /**
   * @brief Simulate operation delay
   */
  void simulate_operation_delay() const;

  /**
   * @brief Get timezone offset in hours
   */
  [[nodiscard]] int get_timezone_offset(const std::string& timezone) const;

  /**
   * @brief Initialize timezone data
   */
  void initialize_timezone_data();

  /**
   * @brief Perform actual sleep if configured
   */
  void perform_actual_sleep(std::chrono::duration<double> duration) const;
};

/**
 * @brief Factory for creating time mocks
 */
class TimeMockFactory {
 public:
  /**
   * @brief Create default time mock
   */
  [[nodiscard]] static std::unique_ptr<TimeMock> create_default();

  /**
   * @brief Create time mock with custom configuration
   */
  [[nodiscard]] static std::unique_ptr<TimeMock> create(TimeMockConfig config);

  /**
   * @brief Create time mock for performance testing (frozen time)
   */
  [[nodiscard]] static std::unique_ptr<TimeMock> create_for_performance_testing();

  /**
   * @brief Create time mock for astronomical calculations
   */
  [[nodiscard]] static std::unique_ptr<TimeMock> create_for_astronomical_testing();

  /**
   * @brief Create time mock with accelerated time
   */
  [[nodiscard]] static std::unique_ptr<TimeMock> create_with_accelerated_time(double scale);

  /**
   * @brief Create time mock starting at specific time
   */
  [[nodiscard]] static std::unique_ptr<TimeMock> create_at_time(
      std::chrono::system_clock::time_point start_time);

  /**
   * @brief Create time mock for historical simulation
   */
  [[nodiscard]] static std::unique_ptr<TimeMock> create_for_historical_simulation(
      std::chrono::system_clock::time_point historical_time);
};

/**
 * @brief RAII helper for installing/removing global time mock
 */
class ScopedTimeMock {
 public:
  explicit ScopedTimeMock(std::unique_ptr<TimeMock> mock);
  ~ScopedTimeMock();

  // Non-copyable, non-movable
  ScopedTimeMock(const ScopedTimeMock&) = delete;
  ScopedTimeMock& operator=(const ScopedTimeMock&) = delete;
  ScopedTimeMock(ScopedTimeMock&&) = delete;
  ScopedTimeMock& operator=(ScopedTimeMock&&) = delete;

  /**
   * @brief Get access to the mock
   */
  [[nodiscard]] TimeMock& mock() { return *mock_; }
  [[nodiscard]] const TimeMock& mock() const { return *mock_; }

 private:
  std::unique_ptr<TimeMock> mock_;
};

/**
 * @brief Time-based test utilities
 */
class TimeTestUtils {
 public:
  /**
   * @brief Create test scenario with time progression
   */
  static void run_time_progression_test(
      std::chrono::system_clock::time_point start_time, std::chrono::duration<double> duration,
      std::chrono::duration<double> step_size,
      std::function<void(std::chrono::system_clock::time_point)> test_function);

  /**
   * @brief Test function execution time
   */
  template <typename Func>
  static std::chrono::duration<double> measure_execution_time(Func&& func);

  /**
   * @brief Create deterministic time sequence
   */
  static std::vector<std::chrono::system_clock::time_point> create_time_sequence(
      std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end,
      size_t num_points);

  /**
   * @brief Validate time-dependent behavior
   */
  static bool validate_time_dependent_behavior(
      std::function<double(std::chrono::system_clock::time_point)> func,
      std::chrono::system_clock::time_point start_time, std::chrono::duration<double> duration,
      double expected_change_rate, double tolerance = 0.01);
};

}  // namespace SolarSystem::Testing::Mocks
