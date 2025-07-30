/**
 * @file time_mock.cpp
 * @brief Time Mock Implementation
 */

#include "solar_test/mocks/time_mock.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>

namespace SolarSystem::Testing::Mocks {

namespace {
// Global time mock instance for dependency injection
TimeMock* g_global_time_mock = nullptr;
std::mutex g_global_mock_mutex;

// Timezone offset data (simplified)
const std::map<std::string, int> kTimezoneOffsets = {
    {"UTC", 0}, {"GMT", 0},   {"EST", -5},      {"CST", -6},  {"MST", -7},  {"PST", -8}, {"CET", 1},
    {"JST", 9}, {"AEST", 10}, {"NZST", 12},     {"HST", -10}, {"AKST", -9}, {"CAT", 2},  {"EAT", 3},
    {"WAT", 1}, {"IST", 5.5}, {"CST_CHINA", 8}, {"KST", 9},   {"WIB", 7},   {"WIT", 9}};

// Julian Date constants
constexpr double kJulianEpoch = 2440587.5;  // Unix epoch in Julian Date
constexpr double kSecondsPerDay = 86400.0;
constexpr double kModifiedJulianOffset = 2400000.5;

}  // namespace

TimeMock::TimeMock(TimeMockConfig config)
    : config_(std::move(config)),
      time_frozen_(config_.start_frozen),
      time_scale_(config_.initial_time_scale),
      default_timezone_(config_.default_timezone),
      total_sleep_duration_(std::chrono::duration<double>::zero()),
      drift_accumulator_(0.0) {
  // Initialize time
  auto now = std::chrono::system_clock::now();
  current_time_.store(config_.initial_time.value_or(now));
  real_start_time_ = now;
  steady_start_time_.store(std::chrono::steady_clock::now());
  mock_start_time_ = std::chrono::steady_clock::now();
  last_drift_update_ = std::chrono::steady_clock::now();

  // Initialize timezone data
  initialize_timezone_data();
}

TimeMock::~TimeMock() {
  // Remove global mock if this instance was installed
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  if (g_global_time_mock == this) {
    g_global_time_mock = nullptr;
  }
}

void TimeMock::set_time(std::chrono::system_clock::time_point time) {
  std::lock_guard<std::mutex> lock(time_mutex_);

  auto validated_time = validate_time_bounds(time);
  current_time_.store(validated_time);

  // Reset timing references
  real_start_time_ = std::chrono::system_clock::now();
  mock_start_time_ = std::chrono::steady_clock::now();
  steady_start_time_.store(std::chrono::steady_clock::now());

  record_operation(MockTimeOperationType::SetTime, validated_time);
}

void TimeMock::advance_time(std::chrono::duration<double> duration) {
  std::lock_guard<std::mutex> lock(time_mutex_);

  auto current = current_time_.load();
  auto new_time =
      current + std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);
  auto validated_time = validate_time_bounds(new_time);

  current_time_.store(validated_time);

  record_operation(MockTimeOperationType::AdvanceTime, validated_time, duration);
}

void TimeMock::advance_time_ms(int64_t milliseconds) {
  advance_time(std::chrono::duration<double>(milliseconds / 1000.0));
}

void TimeMock::advance_time_s(double seconds) {
  advance_time(std::chrono::duration<double>(seconds));
}

void TimeMock::advance_time_min(double minutes) {
  advance_time(std::chrono::duration<double>(minutes * 60.0));
}

void TimeMock::advance_time_h(double hours) {
  advance_time(std::chrono::duration<double>(hours * 3600.0));
}

void TimeMock::advance_time_days(double days) {
  advance_time(std::chrono::duration<double>(days * 86400.0));
}

void TimeMock::freeze_time() {
  time_frozen_.store(true);
  record_operation(MockTimeOperationType::FreezeTime, current_time_.load());
}

void TimeMock::unfreeze_time() {
  time_frozen_.store(false);

  // Reset timing references when unfreezing
  real_start_time_ = std::chrono::system_clock::now();
  mock_start_time_ = std::chrono::steady_clock::now();
  steady_start_time_.store(std::chrono::steady_clock::now());

  record_operation(MockTimeOperationType::UnfreezeTime, current_time_.load());
}

bool TimeMock::is_time_frozen() const { return time_frozen_.load(); }

void TimeMock::set_time_scale(double scale) {
  if (scale <= 0.0) {
    scale = 1.0;  // Prevent invalid scale
  }

  time_scale_.store(scale);

  // Reset timing references when changing scale
  real_start_time_ = std::chrono::system_clock::now();
  mock_start_time_ = std::chrono::steady_clock::now();

  record_operation(MockTimeOperationType::SetTimeScale, current_time_.load(),
                   std::chrono::duration<double>(scale));
}

double TimeMock::get_time_scale() const { return time_scale_.load(); }

void TimeMock::reset_to_system_time() {
  std::lock_guard<std::mutex> lock(time_mutex_);

  auto system_time = std::chrono::system_clock::now();
  current_time_.store(system_time);
  time_frozen_.store(false);
  time_scale_.store(1.0);

  real_start_time_ = system_time;
  mock_start_time_ = std::chrono::steady_clock::now();
  steady_start_time_.store(std::chrono::steady_clock::now());

  record_operation(MockTimeOperationType::SetTime, system_time);
}

std::chrono::system_clock::time_point TimeMock::now() const {
  simulate_operation_delay();

  auto result = calculate_current_time();
  record_operation(MockTimeOperationType::GetCurrentTime, result);

  return result;
}

std::time_t TimeMock::now_time_t() const { return std::chrono::system_clock::to_time_t(now()); }

int64_t TimeMock::now_ms() const {
  auto time_point = now();
  auto duration = time_point.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

double TimeMock::now_seconds() const {
  auto time_point = now();
  auto duration = time_point.time_since_epoch();
  return std::chrono::duration<double>(duration).count();
}

std::chrono::steady_clock::time_point TimeMock::steady_now() const {
  simulate_operation_delay();

  if (time_frozen_.load()) {
    return steady_start_time_.load();
  }

  auto elapsed = std::chrono::steady_clock::now() - mock_start_time_;
  auto scaled_elapsed = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(elapsed.count() * time_scale_.load()));

  return steady_start_time_.load() + scaled_elapsed;
}

std::chrono::high_resolution_clock::time_point TimeMock::high_res_now() const {
  simulate_operation_delay();

  // Use steady clock as base for high resolution clock
  auto steady_time = steady_now();
  return std::chrono::high_resolution_clock::time_point(steady_time.time_since_epoch());
}

void TimeMock::sleep_for(std::chrono::duration<double> duration) {
  record_operation(MockTimeOperationType::Sleep, current_time_.load(), duration);

  {
    std::lock_guard<std::mutex> lock(call_mutex_);
    total_sleep_duration_ += duration;
  }

  if (!time_frozen_.load()) {
    advance_time(duration);
  }

  if (config_.simulate_sleep_delays) {
    perform_actual_sleep(duration);
  }
}

void TimeMock::sleep_until(std::chrono::system_clock::time_point time_point) {
  auto current = now();
  if (time_point > current) {
    auto duration = time_point - current;
    sleep_for(std::chrono::duration<double>(duration));
  }
}

void TimeMock::nanosleep(std::chrono::nanoseconds duration) {
  sleep_for(std::chrono::duration<double>(duration));
}

void TimeMock::usleep(std::chrono::microseconds duration) {
  sleep_for(std::chrono::duration<double>(duration));
}

void TimeMock::sleep_ms(int64_t milliseconds) {
  sleep_for(std::chrono::duration<double>(milliseconds / 1000.0));
}

void TimeMock::sleep_s(double seconds) { sleep_for(std::chrono::duration<double>(seconds)); }

std::chrono::system_clock::time_point TimeMock::to_timezone(
    std::chrono::system_clock::time_point time, const std::string& timezone) const {
  int offset_hours = get_timezone_offset(timezone);
  auto offset = std::chrono::hours(offset_hours);

  record_operation(MockTimeOperationType::ConvertTimeZone, time,
                   std::chrono::duration<double>(offset), "to_" + timezone);

  return time + offset;
}

std::chrono::system_clock::time_point TimeMock::from_timezone(
    std::chrono::system_clock::time_point time, const std::string& timezone) const {
  int offset_hours = get_timezone_offset(timezone);
  auto offset = std::chrono::hours(offset_hours);

  record_operation(MockTimeOperationType::ConvertTimeZone, time,
                   std::chrono::duration<double>(-offset), "from_" + timezone);

  return time - offset;
}

std::chrono::system_clock::time_point TimeMock::now_in_timezone(const std::string& timezone) const {
  return to_timezone(now(), timezone);
}

void TimeMock::set_default_timezone(const std::string& timezone) { default_timezone_ = timezone; }

const std::string& TimeMock::get_default_timezone() const { return default_timezone_; }

std::string TimeMock::format_iso8601(std::chrono::system_clock::time_point time) const {
  auto time_t = std::chrono::system_clock::to_time_t(time);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

  record_operation(MockTimeOperationType::FormatTime, time, {}, "ISO8601");

  return ss.str();
}

std::string TimeMock::format_time(std::chrono::system_clock::time_point time,
                                  const std::string& format) const {
  auto time_t = std::chrono::system_clock::to_time_t(time);

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time_t), format.c_str());

  record_operation(MockTimeOperationType::FormatTime, time, {}, format);

  return ss.str();
}

std::optional<std::chrono::system_clock::time_point> TimeMock::parse_iso8601(
    const std::string& time_str) const {
  std::tm tm = {};
  std::istringstream ss(time_str);

  // Parse basic ISO8601 format: YYYY-MM-DDTHH:MM:SS.sssZ
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  if (ss.fail()) {
    return std::nullopt;
  }

  auto time_t = std::mktime(&tm);
  auto time_point = std::chrono::system_clock::from_time_t(time_t);

  // Handle milliseconds if present
  if (ss.peek() == '.') {
    ss.ignore(1);  // Skip '.'
    int ms;
    ss >> ms;
    time_point += std::chrono::milliseconds(ms);
  }

  record_operation(MockTimeOperationType::FormatTime, time_point, {}, "parse_ISO8601");

  return time_point;
}

std::optional<std::chrono::system_clock::time_point> TimeMock::parse_time(
    const std::string& time_str, const std::string& format) const {
  std::tm tm = {};
  std::istringstream ss(time_str);
  ss >> std::get_time(&tm, format.c_str());

  if (ss.fail()) {
    return std::nullopt;
  }

  auto time_t = std::mktime(&tm);
  auto time_point = std::chrono::system_clock::from_time_t(time_t);

  record_operation(MockTimeOperationType::FormatTime, time_point, {}, "parse_" + format);

  return time_point;
}

double TimeMock::to_julian_date(std::chrono::system_clock::time_point time) const {
  auto duration = time.time_since_epoch();
  auto seconds = std::chrono::duration<double>(duration).count();
  return kJulianEpoch + (seconds / kSecondsPerDay);
}

std::chrono::system_clock::time_point TimeMock::from_julian_date(double jd) const {
  auto days_since_epoch = jd - kJulianEpoch;
  auto seconds = days_since_epoch * kSecondsPerDay;
  auto duration = std::chrono::duration<double>(seconds);
  return std::chrono::system_clock::time_point(
      std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
}

double TimeMock::to_modified_julian_date(std::chrono::system_clock::time_point time) const {
  return to_julian_date(time) - kModifiedJulianOffset;
}

std::chrono::system_clock::time_point TimeMock::from_modified_julian_date(double mjd) const {
  return from_julian_date(mjd + kModifiedJulianOffset);
}

double TimeMock::get_sidereal_time(std::chrono::system_clock::time_point time,
                                   double longitude_deg) const {
  // Simplified sidereal time calculation
  auto jd = to_julian_date(time);
  auto t = (jd - 2451545.0) / 36525.0;  // Julian centuries since J2000.0

  // Greenwich Mean Sidereal Time
  auto gmst = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * t * t -
              t * t * t / 38710000.0;

  // Local sidereal time
  auto lst = gmst + longitude_deg;

  // Normalize to 0-360 degrees
  while (lst < 0.0) lst += 360.0;
  while (lst >= 360.0) lst -= 360.0;

  return lst;
}

size_t TimeMock::operation_count() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_history_.size();
}

size_t TimeMock::operation_count(MockTimeOperationType type) const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  auto it = operation_counts_.find(type);
  return (it != operation_counts_.end()) ? it->second : 0;
}

const std::vector<MockTimeCallInfo>& TimeMock::operation_history() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_history_;
}

std::vector<MockTimeCallInfo> TimeMock::operations_of_type(MockTimeOperationType type) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  std::vector<MockTimeCallInfo> result;
  for (const auto& call : operation_history_) {
    if (call.operation_type == type) {
      result.push_back(call);
    }
  }

  return result;
}

bool TimeMock::was_time_requested(std::chrono::system_clock::time_point time) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  for (const auto& call : operation_history_) {
    if (call.requested_time == time) {
      return true;
    }
  }

  return false;
}

std::chrono::duration<double> TimeMock::total_sleep_duration() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return total_sleep_duration_;
}

void TimeMock::reset_call_history() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  operation_history_.clear();
  operation_counts_.clear();
  total_sleep_duration_ = std::chrono::duration<double>::zero();
}

void TimeMock::update_config(const TimeMockConfig& new_config) {
  std::lock_guard<std::mutex> lock(time_mutex_);
  config_ = new_config;

  // Update timezone data if needed
  if (config_.support_timezones) {
    initialize_timezone_data();
  }
}

void TimeMock::install_as_global_mock() {
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  g_global_time_mock = this;
}

void TimeMock::remove_global_mock() {
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  g_global_time_mock = nullptr;
}

void TimeMock::create_test_scenario(const std::string& scenario_name,
                                    std::chrono::system_clock::time_point start_time,
                                    std::chrono::duration<double> duration, double time_scale) {
  set_time(start_time);
  set_time_scale(time_scale);

  record_operation(MockTimeOperationType::SetTime, start_time, duration,
                   "scenario_" + scenario_name);
}

// Private helper methods

void TimeMock::record_operation(MockTimeOperationType type,
                                std::chrono::system_clock::time_point requested_time,
                                std::chrono::duration<double> duration_param,
                                const std::string& details) const {
  if (!config_.enable_call_history) {
    return;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  MockTimeCallInfo call_info;
  call_info.timestamp = std::chrono::system_clock::now();
  call_info.operation_type = type;
  call_info.requested_time = requested_time;
  call_info.duration_param = duration_param;
  call_info.time_scale = time_scale_.load();
  call_info.operation_details = details;
  call_info.was_successful = true;

  operation_history_.push_back(call_info);
  operation_counts_[type]++;

  // Limit history size
  if (operation_history_.size() > config_.max_history_size) {
    operation_history_.erase(operation_history_.begin());
  }
}

std::chrono::system_clock::time_point TimeMock::calculate_current_time() const {
  if (time_frozen_.load()) {
    return current_time_.load();
  }

  auto current_steady = std::chrono::steady_clock::now();
  auto elapsed = current_steady - mock_start_time_;

  // Apply time scale
  auto scaled_elapsed = std::chrono::duration<double>(elapsed.count() * time_scale_.load());

  // Apply clock drift if enabled
  if (config_.simulate_clock_drift) {
    update_clock_drift();
    scaled_elapsed += std::chrono::duration<double>(drift_accumulator_.load());
  }

  auto scaled_duration =
      std::chrono::duration_cast<std::chrono::system_clock::duration>(scaled_elapsed);
  return current_time_.load() + scaled_duration;
}

void TimeMock::update_clock_drift() const {
  if (config_.drift_rate_ppm == 0.0) {
    return;
  }

  auto now = std::chrono::steady_clock::now();
  auto elapsed = now - last_drift_update_;
  auto elapsed_seconds = std::chrono::duration<double>(elapsed).count();

  // Calculate drift: ppm * elapsed_time
  auto drift = (config_.drift_rate_ppm / 1000000.0) * elapsed_seconds;
  drift_accumulator_.store(drift_accumulator_.load() + drift);

  last_drift_update_ = now;
}

std::chrono::system_clock::time_point TimeMock::validate_time_bounds(
    std::chrono::system_clock::time_point time) const {
  if (time < config_.min_time) {
    return config_.wrap_around_epochs ? config_.max_time : config_.min_time;
  }

  if (time > config_.max_time) {
    return config_.wrap_around_epochs ? config_.min_time : config_.max_time;
  }

  return time;
}

void TimeMock::simulate_operation_delay() const {
  if (config_.operation_delay > std::chrono::nanoseconds::zero()) {
    std::this_thread::sleep_for(config_.operation_delay);
  }
}

int TimeMock::get_timezone_offset(const std::string& timezone) const {
  auto it = timezone_offsets_.find(timezone);
  if (it != timezone_offsets_.end()) {
    return it->second;
  }

  // Check global timezone data
  auto global_it = kTimezoneOffsets.find(timezone);
  if (global_it != kTimezoneOffsets.end()) {
    return static_cast<int>(global_it->second);
  }

  return 0;  // Default to UTC
}

void TimeMock::initialize_timezone_data() {
  timezone_offsets_.clear();

  if (config_.support_timezones) {
    // Copy global timezone data
    for (const auto& [tz, offset] : kTimezoneOffsets) {
      timezone_offsets_[tz] = static_cast<int>(offset);
    }
  }
}

void TimeMock::perform_actual_sleep(std::chrono::duration<double> duration) const {
  if (duration > config_.max_sleep_duration) {
    duration = config_.max_sleep_duration;
  }

  // Apply sleep accuracy factor
  auto actual_duration = std::chrono::duration<double>(duration.count() * config_.sleep_accuracy);

  if (actual_duration > std::chrono::duration<double>::zero()) {
    std::this_thread::sleep_for(actual_duration);
  }
}

// Factory implementations

std::unique_ptr<TimeMock> TimeMockFactory::create_default() { return std::make_unique<TimeMock>(); }

std::unique_ptr<TimeMock> TimeMockFactory::create(TimeMockConfig config) {
  return std::make_unique<TimeMock>(std::move(config));
}

std::unique_ptr<TimeMock> TimeMockFactory::create_for_performance_testing() {
  TimeMockConfig config;
  config.start_frozen = true;
  config.simulate_sleep_delays = false;
  config.enable_call_history = false;
  config.operation_delay = std::chrono::nanoseconds::zero();

  return std::make_unique<TimeMock>(config);
}

std::unique_ptr<TimeMock> TimeMockFactory::create_for_astronomical_testing() {
  TimeMockConfig config;
  config.high_precision_mode = true;
  config.time_resolution = std::chrono::nanoseconds(1);
  config.support_timezones = true;
  config.initial_time = std::chrono::system_clock::from_time_t(946684800);  // Y2K

  return std::make_unique<TimeMock>(config);
}

std::unique_ptr<TimeMock> TimeMockFactory::create_with_accelerated_time(double scale) {
  TimeMockConfig config;
  config.initial_time_scale = scale;
  config.simulate_sleep_delays = false;

  return std::make_unique<TimeMock>(config);
}

std::unique_ptr<TimeMock> TimeMockFactory::create_at_time(
    std::chrono::system_clock::time_point start_time) {
  TimeMockConfig config;
  config.initial_time = start_time;
  config.start_frozen = true;

  return std::make_unique<TimeMock>(config);
}

std::unique_ptr<TimeMock> TimeMockFactory::create_for_historical_simulation(
    std::chrono::system_clock::time_point historical_time) {
  TimeMockConfig config;
  config.initial_time = historical_time;
  config.start_frozen = false;
  config.initial_time_scale = 1.0;
  config.support_timezones = true;

  return std::make_unique<TimeMock>(config);
}

// Scoped time mock implementation

ScopedTimeMock::ScopedTimeMock(std::unique_ptr<TimeMock> mock) : mock_(std::move(mock)) {
  mock_->install_as_global_mock();
}

ScopedTimeMock::~ScopedTimeMock() { TimeMock::remove_global_mock(); }

// Time test utilities implementation

void TimeTestUtils::run_time_progression_test(
    std::chrono::system_clock::time_point start_time, std::chrono::duration<double> duration,
    std::chrono::duration<double> step_size,
    std::function<void(std::chrono::system_clock::time_point)> test_function) {
  auto current_time = start_time;
  auto end_time =
      start_time + std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);

  while (current_time < end_time) {
    test_function(current_time);
    current_time += std::chrono::duration_cast<std::chrono::system_clock::duration>(step_size);
  }
}

std::vector<std::chrono::system_clock::time_point> TimeTestUtils::create_time_sequence(
    std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end,
    size_t num_points) {
  std::vector<std::chrono::system_clock::time_point> sequence;
  sequence.reserve(num_points);

  if (num_points <= 1) {
    sequence.push_back(start);
    return sequence;
  }

  auto total_duration = end - start;
  auto step_duration = total_duration / (num_points - 1);

  for (size_t i = 0; i < num_points; ++i) {
    sequence.push_back(start + step_duration * i);
  }

  return sequence;
}

bool TimeTestUtils::validate_time_dependent_behavior(
    std::function<double(std::chrono::system_clock::time_point)> func,
    std::chrono::system_clock::time_point start_time, std::chrono::duration<double> duration,
    double expected_change_rate, double tolerance) {
  auto end_time =
      start_time + std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);

  auto start_value = func(start_time);
  auto end_value = func(end_time);

  auto actual_change = end_value - start_value;
  auto duration_seconds = duration.count();
  auto actual_rate = actual_change / duration_seconds;

  auto rate_difference = std::abs(actual_rate - expected_change_rate);
  return rate_difference <= tolerance;
}

}  // namespace SolarSystem::Testing::Mocks
