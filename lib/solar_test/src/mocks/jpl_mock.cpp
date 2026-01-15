/**
 * @file jpl_mock.cpp
 * @brief Implementation of JPL API Mock System
 */

#include "solar_test/mocks/jpl_mock.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <thread>

#include "solar_core/bodies/body_mappings.hpp"

namespace SolarSystem::Testing::Mocks {

// Global mock instance for dependency injection
static std::unique_ptr<JPLMock> g_global_mock = nullptr;
static std::mutex g_global_mock_mutex;

/**
 * @brief JPLMock constructor
 */
JPLMock::JPLMock(JPLMockConfig config)
    : config_(std::move(config)), gen_(rd_()), failure_dist_(0.0, 1.0) {
  // Reserve space for call history
  if (config_.enable_call_history) {
    call_history_.reserve(config_.max_history_size);
  }

  // Load mock responses if directory is specified
  if (!config_.mock_data_directory.empty()) {
    load_mock_responses_from_directory(config_.mock_data_directory);
  }
}

// === Response Configuration ===

void JPLMock::set_response_for_body(int jpl_id, const std::string& response) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  body_responses_[jpl_id] = response;
  body_response_types_[jpl_id] = MockResponseType::CustomResponse;
}

void JPLMock::set_response_type_for_body(int jpl_id, MockResponseType type) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  body_response_types_[jpl_id] = type;
}

void JPLMock::set_default_response_type(MockResponseType type) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  default_response_type_ = type;
}

void JPLMock::set_error_response(int http_code, const std::string& error_message) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  custom_error_code_ = http_code;
  custom_error_response_ = error_message;
}

bool JPLMock::load_mock_responses_from_directory(const std::string& directory_path) {
  if (!std::filesystem::exists(directory_path)) {
    return false;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  try {
    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
      if (entry.is_regular_file() && entry.path().extension() == ".txt") {
        // Extract JPL ID from filename (e.g., "jpl_399.txt" -> 399)
        std::string filename = entry.path().stem().string();
        std::regex jpl_regex(R"(jpl_(\d+))");
        std::smatch match;

        if (std::regex_search(filename, match, jpl_regex)) {
          int jpl_id = std::stoi(match[1].str());

          // Read file content
          std::ifstream file(entry.path());
          if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            body_responses_[jpl_id] = content;
            body_response_types_[jpl_id] = MockResponseType::CustomResponse;
          }
        }
      }
    }
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

// === Network Simulation ===

void JPLMock::simulate_network_failure(size_t request_count) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  remaining_network_failures_ = request_count;
}

void JPLMock::simulate_timeout(size_t request_count) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  remaining_timeouts_ = request_count;
}

void JPLMock::simulate_server_error(size_t request_count) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  remaining_server_errors_ = request_count;
}

void JPLMock::set_network_delay_range(std::chrono::milliseconds min_delay,
                                      std::chrono::milliseconds max_delay) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  config_.min_response_delay = min_delay;
  config_.max_response_delay = max_delay;
}

void JPLMock::enable_network_delays(bool enabled) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  config_.simulate_network_delays = enabled;
}

// === Realistic Response Generation ===

std::string JPLMock::generate_realistic_response(
    int jpl_id, std::chrono::system_clock::time_point epoch) const {
  std::ostringstream response;

  // Generate JPL HORIZONS header
  response << "*******************************************************************************\n";
  auto epoch_time_t = std::chrono::system_clock::to_time_t(epoch);
  response << " Revised: " << std::put_time(std::gmtime(&epoch_time_t), "%b %d, %Y") << "\n";
  response << " \n";
  response << " PHYSICAL DATA (updated 2021-May-11):\n";
  response << " \n";

  // Generate body information section
  response << generate_body_info_section(jpl_id);

  response << "*******************************************************************************\n";
  response << " \n";
  auto now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  response << "Ephemeris / WWW_USER "
           << std::put_time(std::gmtime(&now_time_t), "%Y-%b-%d %H:%M:%S") << "\n";
  response << " \n";
  response << "Target body name: " << get_body_name_for_jpl_id(jpl_id) << " (" << jpl_id << ")\n";
  response << "Center body name: Solar System Barycenter (0)\n";
  response << "Center-site name: BODY CENTER\n";
  response << "*******************************************************************************\n";
  response << "Initial IAU76/J2000 heliocentric ecliptic osculating elements (au, days, deg.):\n";
  response << " \n";

  // Generate ephemeris data section
  response << generate_ephemeris_section(jpl_id, epoch);

  response << "*******************************************************************************\n";

  return response.str();
}

std::string JPLMock::generate_ephemeris_section(int jpl_id,
                                                std::chrono::system_clock::time_point epoch) const {
  std::ostringstream section;

  // Generate ephemeris header
  section << "$$SOE\n";

  // Generate date string
  auto epoch_time_t = std::chrono::system_clock::to_time_t(epoch);
  auto* tm = std::gmtime(&epoch_time_t);

  section << std::put_time(tm, "%Y-%b-%d %H:%M") << ",";

  // Generate realistic position and velocity
  auto position = generate_realistic_position(jpl_id, epoch);
  auto velocity = generate_realistic_velocity(jpl_id, epoch);

  // Format as CSV: Date, X, Y, Z, VX, VY, VZ
  section << std::scientific << std::setprecision(15);
  section << position.x() << "," << position.y() << "," << position.z() << ",";
  section << velocity.x() << "," << velocity.y() << "," << velocity.z() << "\n";

  section << "$$EOE\n";

  return section.str();
}

std::string JPLMock::generate_body_info_section(int jpl_id) const {
  std::ostringstream info;

  std::string body_name = get_body_name_for_jpl_id(jpl_id);
  long double mass = get_realistic_mass(jpl_id);

  info << "Target body name: " << body_name << " (" << jpl_id << ")\n";
  info << " \n";

  // Add mass information if available
  if (mass > 0) {
    info << "Mass (10^24 kg)      = " << std::scientific << std::setprecision(6) << (mass / 1e24)
         << "\n";
  }

  // Add body-specific information
  auto body_type = Bodies::get_body_type_for_jpl_id(jpl_id);
  switch (body_type) {
    case Bodies::BodyType::Star:
      info << "Mean radius (km)     = 6.96000E+05\n";
      info << "Surface gravity      = 274.0 m/s^2\n";
      break;
    case Bodies::BodyType::Planet:
      info << "Mean radius (km)     = 6.371E+03\n";
      info << "Surface gravity      = 9.80665 m/s^2\n";
      break;
    case Bodies::BodyType::Moon:
      info << "Mean radius (km)     = 1.737E+03\n";
      info << "Surface gravity      = 1.62 m/s^2\n";
      break;
    default:
      info << "Mean radius (km)     = 1.000E+03\n";
      break;
  }

  info << " \n";

  return info.str();
}

// === Call Verification ===

size_t JPLMock::call_count() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return call_history_.size();
}

size_t JPLMock::call_count_for_body(int jpl_id) const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  auto it = body_call_counts_.find(jpl_id);
  return (it != body_call_counts_.end()) ? it->second : 0;
}

std::vector<int> JPLMock::requested_bodies() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  std::vector<int> bodies;
  bodies.reserve(body_call_counts_.size());

  for (const auto& [jpl_id, count] : body_call_counts_) {
    bodies.push_back(jpl_id);
  }

  std::sort(bodies.begin(), bodies.end());
  return bodies;
}

const std::vector<MockCallInfo>& JPLMock::call_history() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return call_history_;
}

std::vector<MockCallInfo> JPLMock::calls_for_body(int jpl_id) const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  std::vector<MockCallInfo> body_calls;

  for (const auto& call : call_history_) {
    if (call.jpl_id == jpl_id) {
      body_calls.push_back(call);
    }
  }

  return body_calls;
}

bool JPLMock::was_body_requested(int jpl_id) const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return body_call_counts_.find(jpl_id) != body_call_counts_.end();
}

std::optional<MockCallInfo> JPLMock::last_call() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  if (call_history_.empty()) {
    return std::nullopt;
  }
  return call_history_.back();
}

void JPLMock::reset_call_history() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  call_history_.clear();
  body_call_counts_.clear();
}

// === Mock Interface Implementation ===

SolarSystem::JPL::JPLResult<std::string> JPLMock::mock_request(const std::string&,
                                                               const std::string& params) {
  // Parse JPL ID and epoch from parameters
  int jpl_id = parse_jpl_id_from_params(params);
  auto epoch = parse_epoch_from_params(params);

  // Simulate network delay if enabled
  auto start_time = std::chrono::steady_clock::now();
  if (config_.simulate_network_delays) {
    simulate_network_delay();
  }
  auto end_time = std::chrono::steady_clock::now();
  auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Determine response type
  MockResponseType response_type = determine_response_type(jpl_id);

  // Record the call
  record_call(jpl_id, epoch, params, response_type, delay,
              response_type == MockResponseType::Success);

  // Generate response based on type
  switch (response_type) {
    case MockResponseType::Success: {
      // Check if we have a custom response for this body
      std::lock_guard<std::mutex> lock(call_mutex_);
      auto it = body_responses_.find(jpl_id);
      if (it != body_responses_.end()) {
        return it->second;
      }

      // Generate realistic response
      if (config_.use_realistic_responses) {
        return generate_realistic_response(jpl_id, epoch);
      } else {
        return "Mock response for JPL ID " + std::to_string(jpl_id);
      }
    }

    case MockResponseType::NetworkError:
      return SolarSystem::JPL::JPLError::NetworkError;

    case MockResponseType::Timeout:
      return SolarSystem::JPL::JPLError::NetworkError;  // Timeout is treated as network error

    case MockResponseType::ServerError:
      return SolarSystem::JPL::JPLError::ServerError;

    case MockResponseType::InvalidBody:
      return SolarSystem::JPL::JPLError::InvalidBody;

    case MockResponseType::ParseError:
      return generate_error_response(MockResponseType::ParseError);

    case MockResponseType::RateLimited:
      return SolarSystem::JPL::JPLError::RateLimited;

    case MockResponseType::CustomResponse: {
      std::lock_guard<std::mutex> lock(call_mutex_);
      if (!custom_error_response_.empty()) {
        return custom_error_response_;
      }
      return generate_error_response(MockResponseType::ServerError);
    }
  }

  return SolarSystem::JPL::JPLError::NetworkError;
}

SolarSystem::JPL::JPLResult<std::vector<SolarSystem::JPL::EphemerisData>>
JPLMock::mock_load_from_cache() {
  std::lock_guard<std::mutex> lock(call_mutex_);

  if (config_.cache_always_invalid || !cache_has_data_) {
    return SolarSystem::JPL::JPLError::CacheError;
  }

  if (config_.cache_always_valid || !mock_cache_data_.empty()) {
    return mock_cache_data_;
  }

  return SolarSystem::JPL::JPLError::CacheError;
}

SolarSystem::JPL::JPLVoidResult JPLMock::mock_save_to_cache(
    const std::vector<SolarSystem::JPL::EphemerisData>& data) {
  std::lock_guard<std::mutex> lock(call_mutex_);

  if (config_.simulate_cache_operations) {
    mock_cache_data_ = data;
    cache_has_data_ = true;
    return SolarSystem::JPL::success();
  }

  return SolarSystem::JPL::error(SolarSystem::JPL::JPLError::CacheError);
}

SolarSystem::JPL::JPLResult<bool> JPLMock::mock_validate_cache() const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  if (config_.cache_always_valid) {
    return true;
  }

  if (config_.cache_always_invalid) {
    return false;
  }

  return cache_has_data_;
}

// === Configuration Access ===

void JPLMock::update_config(const JPLMockConfig& new_config) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  config_ = new_config;
}

// === Utility Functions ===

void JPLMock::install_as_global_mock() {
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  // Note: In a real implementation, this would integrate with the JPL client
  // to use this mock instead of making real HTTP requests
}

void JPLMock::remove_global_mock() {
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  g_global_mock.reset();
}

// === Internal Helper Methods ===

void JPLMock::record_call(int jpl_id, std::chrono::system_clock::time_point epoch,
                          const std::string& params, MockResponseType response_type,
                          std::chrono::milliseconds delay, bool successful) {
  if (!config_.enable_call_history) {
    return;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  // Create call info
  MockCallInfo call_info;
  call_info.timestamp = std::chrono::system_clock::now();
  call_info.jpl_id = jpl_id;
  call_info.requested_epoch = epoch;
  call_info.request_params = params;
  call_info.response_type = response_type;
  call_info.response_delay = delay;
  call_info.was_successful = successful;

  // Add to history (with size limit)
  if (call_history_.size() >= config_.max_history_size) {
    call_history_.erase(call_history_.begin());
  }
  call_history_.push_back(call_info);

  // Update body call count
  body_call_counts_[jpl_id]++;
}

MockResponseType JPLMock::determine_response_type(int jpl_id) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  // Check for forced failures first
  if (remaining_network_failures_ > 0) {
    const_cast<JPLMock*>(this)->remaining_network_failures_--;
    return MockResponseType::NetworkError;
  }

  if (remaining_timeouts_ > 0) {
    const_cast<JPLMock*>(this)->remaining_timeouts_--;
    return MockResponseType::Timeout;
  }

  if (remaining_server_errors_ > 0) {
    const_cast<JPLMock*>(this)->remaining_server_errors_--;
    return MockResponseType::ServerError;
  }

  // Check for body-specific response type
  auto it = body_response_types_.find(jpl_id);
  if (it != body_response_types_.end()) {
    return it->second;
  }

  // Check for random failures
  if (should_simulate_failure(config_.network_failure_rate)) {
    return MockResponseType::NetworkError;
  }

  if (should_simulate_failure(config_.timeout_rate)) {
    return MockResponseType::Timeout;
  }

  if (should_simulate_failure(config_.server_error_rate)) {
    return MockResponseType::ServerError;
  }

  if (should_simulate_failure(config_.parse_error_rate)) {
    return MockResponseType::ParseError;
  }

  return default_response_type_;
}

void JPLMock::simulate_network_delay() const {
  if (!config_.simulate_network_delays) {
    return;
  }

  // Generate random delay within configured range
  std::uniform_int_distribution<long long> delay_dist(config_.min_response_delay.count(),
                                                      config_.max_response_delay.count());

  auto delay = std::chrono::milliseconds(delay_dist(gen_));
  std::this_thread::sleep_for(delay);
}

bool JPLMock::should_simulate_failure(double failure_rate) const {
  if (failure_rate <= 0.0) {
    return false;
  }

  if (failure_rate >= 1.0) {
    return true;
  }

  return failure_dist_(gen_) < failure_rate;
}

int JPLMock::parse_jpl_id_from_params(const std::string& params) const {
  // Look for COMMAND='<id>' pattern
  std::regex command_regex(R"(COMMAND='([^']+)')");
  std::smatch match;

  if (std::regex_search(params, match, command_regex)) {
    try {
      return std::stoi(match[1].str());
    } catch (const std::exception&) {
      return 399;  // Default to Earth
    }
  }

  return 399;  // Default to Earth
}

std::chrono::system_clock::time_point JPLMock::parse_epoch_from_params(
    const std::string& params) const {
  // Look for START_TIME='<date>' pattern
  std::regex time_regex(R"(START_TIME='([^']+)')");
  std::smatch match;

  if (std::regex_search(params, match, time_regex)) {
    std::string date_str = match[1].str();

    // Try to parse the date string in various formats
    // Format 1: YYYY-MM-DD HH:MM:SS
    std::tm tm = {};
    std::istringstream ss1(date_str);
    ss1 >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (!ss1.fail()) {
      return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    // Format 2: YYYY-MM-DD
    tm = {};
    std::istringstream ss2(date_str);
    ss2 >> std::get_time(&tm, "%Y-%m-%d");
    if (!ss2.fail()) {
      return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    // Format 3: YYYY/MM/DD
    tm = {};
    std::istringstream ss3(date_str);
    ss3 >> std::get_time(&tm, "%Y/%m/%d");
    if (!ss3.fail()) {
      return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    // Format 4: Julian Date (JD)
    if (date_str.find("JD") != std::string::npos || date_str.find("jd") != std::string::npos) {
      try {
        // Extract numeric part
        std::regex jd_regex(R"((\d+\.?\d*))");
        std::smatch jd_match;
        if (std::regex_search(date_str, jd_match, jd_regex)) {
          double jd = std::stod(jd_match[1].str());
          // Convert Julian Date to Unix timestamp
          // JD 2440587.5 = Unix epoch (1970-01-01 00:00:00)
          double days_since_epoch = jd - 2440587.5;
          auto seconds = static_cast<long long>(days_since_epoch * 86400.0);
          return std::chrono::system_clock::from_time_t(seconds);
        }
      } catch (...) {
        // Fall through to default
      }
    }

    // If all parsing attempts fail, return current time as fallback
    return std::chrono::system_clock::now();
  }

  return std::chrono::system_clock::now();
}

std::string JPLMock::generate_error_response(MockResponseType error_type) const {
  switch (error_type) {
    case MockResponseType::NetworkError:
      return "ERROR: Network connection failed";

    case MockResponseType::Timeout:
      return "ERROR: Request timeout";

    case MockResponseType::ServerError:
      return "ERROR: Internal server error (500)";

    case MockResponseType::InvalidBody:
      return "ERROR: Cannot find body with specified ID";

    case MockResponseType::ParseError:
      return "MALFORMED_RESPONSE_DATA_INVALID_FORMAT";

    case MockResponseType::RateLimited:
      return "ERROR: Too many requests. Please try again later.";

    default:
      return "ERROR: Unknown error occurred";
  }
}

std::string JPLMock::get_body_name_for_jpl_id(int jpl_id) const {
  // Use the body mappings to get the name
  for (const auto& [name, id] : Bodies::BODY_NAME_TO_JPL_ID) {
    if (id == jpl_id) {
      return name;
    }
  }

  return "Body_" + std::to_string(jpl_id);
}

SolarSystem::Math::Vector3d JPLMock::generate_realistic_position(
    int jpl_id, std::chrono::system_clock::time_point epoch) const {
  // Generate realistic orbital positions based on body type
  auto body_type = Bodies::get_body_type_for_jpl_id(jpl_id);

  // Use time-based seed for consistent but varying positions
  auto epoch_time = std::chrono::system_clock::to_time_t(epoch);
  std::mt19937 pos_gen(static_cast<unsigned int>(epoch_time + jpl_id));
  std::uniform_real_distribution<double> angle_dist(0.0, 2.0 * M_PI);

  double angle = angle_dist(pos_gen);

  switch (body_type) {
    case Bodies::BodyType::Star:  // Sun
      return SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};

    case Bodies::BodyType::Planet: {
      // Generate planetary orbital distances (AU converted to km)
      double distance_au = 0.5 + (jpl_id / 100.0);     // Rough approximation
      double distance_km = distance_au * 149597870.7;  // AU to km

      return SolarSystem::Math::Vector3d{
          distance_km * std::cos(angle), distance_km * std::sin(angle),
          distance_km * 0.1 * std::sin(angle * 2)  // Small inclination
      };
    }

    case Bodies::BodyType::Moon: {
      // Generate moon positions relative to their planet
      double distance_km = 100000.0 + (jpl_id * 1000.0);  // Rough approximation

      return SolarSystem::Math::Vector3d{distance_km * std::cos(angle),
                                         distance_km * std::sin(angle),
                                         distance_km * 0.05 * std::sin(angle * 3)};
    }

    default: {
      // Default orbital position
      double distance_km = 1000000.0 + (jpl_id * 10000.0);

      return SolarSystem::Math::Vector3d{distance_km * std::cos(angle),
                                         distance_km * std::sin(angle),
                                         distance_km * 0.2 * std::sin(angle)};
    }
  }
}

SolarSystem::Math::Vector3d JPLMock::generate_realistic_velocity(
    int jpl_id, std::chrono::system_clock::time_point epoch) const {
  auto body_type = Bodies::get_body_type_for_jpl_id(jpl_id);

  // Use time-based seed for consistent velocities
  auto epoch_time = std::chrono::system_clock::to_time_t(epoch);
  std::mt19937 vel_gen(static_cast<unsigned int>(epoch_time + jpl_id + 1000));
  std::uniform_real_distribution<double> vel_dist(-1.0, 1.0);

  switch (body_type) {
    case Bodies::BodyType::Star:  // Sun
      return SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};

    case Bodies::BodyType::Planet: {
      // Planetary orbital velocities (km/s)
      double base_velocity = 10.0 + (jpl_id / 50.0);

      return SolarSystem::Math::Vector3d{base_velocity * vel_dist(vel_gen),
                                         base_velocity * vel_dist(vel_gen),
                                         base_velocity * 0.1 * vel_dist(vel_gen)};
    }

    case Bodies::BodyType::Moon: {
      // Moon orbital velocities
      double base_velocity = 1.0 + (jpl_id / 100.0);

      return SolarSystem::Math::Vector3d{base_velocity * vel_dist(vel_gen),
                                         base_velocity * vel_dist(vel_gen),
                                         base_velocity * 0.1 * vel_dist(vel_gen)};
    }

    default: {
      // Default velocity
      double base_velocity = 5.0;

      return SolarSystem::Math::Vector3d{base_velocity * vel_dist(vel_gen),
                                         base_velocity * vel_dist(vel_gen),
                                         base_velocity * 0.2 * vel_dist(vel_gen)};
    }
  }
}

long double JPLMock::get_realistic_mass(int jpl_id) const {
  auto body_type = Bodies::get_body_type_for_jpl_id(jpl_id);

  switch (body_type) {
    case Bodies::BodyType::Star:
      return 1.98847e30;  // Solar mass

    case Bodies::BodyType::Planet:
      // Approximate planetary masses based on JPL ID
      switch (jpl_id) {
        case 199:
          return 3.3011e23;  // Mercury
        case 299:
          return 4.8675e24;  // Venus
        case 399:
          return 5.97219e24;  // Earth
        case 499:
          return 6.4171e23;  // Mars
        case 599:
          return 1.8982e27;  // Jupiter
        case 699:
          return 5.6834e26;  // Saturn
        case 799:
          return 8.6810e25;  // Uranus
        case 899:
          return 1.02413e26;  // Neptune
        default:
          return 5.97219e24;  // Earth-like default
      }

    case Bodies::BodyType::Moon:
      // Approximate moon masses
      switch (jpl_id) {
        case 301:
          return 7.342e22;  // Moon
        case 501:
          return 8.9319e22;  // Io
        case 502:
          return 4.7998e22;  // Europa
        case 503:
          return 1.4819e23;  // Ganymede
        case 504:
          return 1.0759e23;  // Callisto
        case 606:
          return 1.3452e23;  // Titan
        default:
          return 7.342e22;  // Moon-like default
      }

    case Bodies::BodyType::DwarfPlanet:
      switch (jpl_id) {
        case 999:
          return 1.303e22;  // Pluto
        default:
          return 1.0e22;  // Small dwarf planet
      }

    default:
      return 1.0e20;  // Small body default
  }
}

// === Factory Implementation ===

std::unique_ptr<JPLMock> JPLMockFactory::create_default() { return std::make_unique<JPLMock>(); }

std::unique_ptr<JPLMock> JPLMockFactory::create(JPLMockConfig config) {
  return std::make_unique<JPLMock>(std::move(config));
}

std::unique_ptr<JPLMock> JPLMockFactory::create_for_network_failure_testing() {
  JPLMockConfig config;
  config.network_failure_rate = 0.3;  // 30% failure rate
  config.timeout_rate = 0.1;          // 10% timeout rate
  config.server_error_rate = 0.05;    // 5% server error rate
  config.simulate_network_delays = true;
  config.min_response_delay = std::chrono::milliseconds(100);
  config.max_response_delay = std::chrono::milliseconds(1000);

  return std::make_unique<JPLMock>(config);
}

std::unique_ptr<JPLMock> JPLMockFactory::create_for_performance_testing() {
  JPLMockConfig config;
  config.simulate_network_delays = false;  // No delays for performance testing
  config.network_failure_rate = 0.0;       // No failures
  config.use_realistic_responses = false;  // Simple responses
  config.enable_call_history = false;      // Reduce overhead

  return std::make_unique<JPLMock>(config);
}

std::unique_ptr<JPLMock> JPLMockFactory::create_with_realistic_responses() {
  JPLMockConfig config;
  config.use_realistic_responses = true;
  config.simulate_network_delays = true;
  config.min_response_delay = std::chrono::milliseconds(50);
  config.max_response_delay = std::chrono::milliseconds(200);

  return std::make_unique<JPLMock>(config);
}

// === Scoped Mock Implementation ===

ScopedJPLMock::ScopedJPLMock(std::unique_ptr<JPLMock> mock) : mock_(std::move(mock)) {
  if (mock_) {
    mock_->install_as_global_mock();
  }
}

ScopedJPLMock::~ScopedJPLMock() { JPLMock::remove_global_mock(); }

}  // namespace SolarSystem::Testing::Mocks
