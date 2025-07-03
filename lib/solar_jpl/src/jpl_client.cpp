/**
 * @file jpl_client.cpp
 * @brief Implementation of modern JPL HORIZONS API client
 */

#include "solar_jpl/jpl_client.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <thread>

// For HTTP requests (using system curl for now)
#include <cstdio>
#include <memory>

namespace SolarSystem::JPL {

/**
 * @brief Internal implementation details
 */
struct JPLClient::Impl {
  std::mutex request_mutex;
  std::chrono::steady_clock::time_point last_request_time;

  /**
   * @brief Execute system command and capture output
   */
  [[nodiscard]] std::string execute_command(const std::string& command) {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
      return "";
    }

    std::string result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
      result += buffer;
    }

    return result;
  }

  /**
   * @brief Rate limiting for API requests
   */
  void rate_limit(std::chrono::milliseconds delay) {
    std::lock_guard<std::mutex> lock(request_mutex);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - last_request_time;

    if (elapsed < delay) {
      std::this_thread::sleep_for(delay - elapsed);
    }

    last_request_time = std::chrono::steady_clock::now();
  }
};

/**
 * @brief Configuration validation
 */
bool JPLClientConfig::is_valid(std::string* error) const {
  if (api_endpoint.empty()) {
    if (error) *error = "API endpoint cannot be empty";
    return false;
  }

  if (request_timeout <= std::chrono::seconds(0)) {
    if (error) *error = "Request timeout must be positive";
    return false;
  }

  if (max_concurrent_requests == 0) {
    if (error) *error = "Max concurrent requests must be positive";
    return false;
  }

  if (max_retries == 0) {
    if (error) *error = "Max retries must be positive";
    return false;
  }

  return true;
}

/**
 * @brief Cache metadata validation
 */
bool CacheMetadata::is_valid(std::chrono::hours max_age) const {
  auto now = std::chrono::system_clock::now();
  auto age = now - created_at;
  return age <= max_age;
}

/**
 * @brief Convert ephemeris data to celestial body
 */
SolarSystem::Bodies::CelestialBody EphemerisData::to_celestial_body() const {
  // TODO: Implement when CelestialBody constructor is available
  // This will be implemented when we integrate with the modern simulation system
  SolarSystem::Bodies::CelestialBody::Properties props = {};
  props.name = body_name;
  props.mass = mass;
  props.position = position * 1000;
  props.velocity = velocity * 1000;
  props.type = SolarSystem::Bodies::get_body_type_for_jpl_id(jpl_id);
  props.priority = SolarSystem::Bodies::get_priority_for_body_type(props.type);
  props.jpl_id = std::to_string(jpl_id);
  return SolarSystem::Bodies::CelestialBody(props);
}

/**
 * @brief JPL client constructor
 */
JPLClient::JPLClient(JPLClientConfig config)
    : config_(std::move(config)), impl_(std::make_unique<Impl>()) {
  // Validate configuration
  std::string error;
  if (!config_.is_valid(&error)) {
    throw std::invalid_argument("Invalid JPL client configuration: " + error);
  }

  // Create cache directory if it doesn't exist
  if (!std::filesystem::exists(config_.cache_directory)) {
    std::filesystem::create_directories(config_.cache_directory);
  }
}

/**
 * @brief JPL client destructor
 */
JPLClient::~JPLClient() = default;

/**
 * @brief Fetch single body asynchronously
 */
std::future<JPLResult<EphemerisData>> JPLClient::fetch_body_async(
    int jpl_id, std::chrono::system_clock::time_point epoch) {
  return std::async(std::launch::async, [this, jpl_id, epoch]() -> JPLResult<EphemerisData> {
    return fetch_body_internal(jpl_id, epoch);
  });
}

/**
 * @brief Fetch multiple bodies asynchronously
 */
std::future<JPLResult<std::vector<EphemerisData>>> JPLClient::fetch_bodies_async(
    const std::vector<int>& jpl_ids, std::chrono::system_clock::time_point epoch) {
  return std::async(std::launch::async,
                    [this, jpl_ids, epoch]() -> JPLResult<std::vector<EphemerisData>> {
                      std::vector<EphemerisData> results;
                      results.reserve(jpl_ids.size());

                      // Fetch bodies with rate limiting
                      for (int jpl_id : jpl_ids) {
                        auto result = fetch_body_internal(jpl_id, epoch);
                        if (!is_success(result)) {
                          return get_error(result);
                        }

                        results.push_back(std::move(get_value(result)));

                        // Rate limiting between requests
                        if (&jpl_id != &jpl_ids.back()) {  // Not the last element
                          std::this_thread::sleep_for(config_.request_delay);
                        }
                      }

                      return results;
                    });
}

/**
 * @brief Fetch all known bodies asynchronously
 */
std::future<JPLResult<std::vector<EphemerisData>>> JPLClient::fetch_all_bodies_async(
    std::chrono::system_clock::time_point epoch) {
  auto all_ids = Utils::get_all_jpl_ids();
  return fetch_bodies_async(all_ids, epoch);
}

/**
 * @brief Fetch single body (internal implementation)
 */
JPLResult<EphemerisData> JPLClient::fetch_body_internal(
    int jpl_id, std::chrono::system_clock::time_point epoch) {
  // Rate limiting
  impl_->rate_limit(config_.request_delay);

  // Convert epoch to JPL date string
  auto date_str = Utils::to_jpl_date_string(epoch);

  // Build JPL HORIZONS API request
  std::ostringstream params;
  params << "format=text&COMMAND='" << jpl_id << "'";
  params << "&OBJ_DATA='YES'&MAKE_EPHEM='YES'&EPHEM_TYPE='VECTORS'";
  params << "&CENTER='500@0'&START_TIME='" << date_str << "'";
  params << "&STOP_TIME='" << date_str << "'&STEP_SIZE='1d'";
  params << "&VEC_TABLE='2'&REF_PLANE='ECLIPTIC'&REF_SYSTEM='J2000'";
  params << "&VEC_CORR='NONE'&VEC_DELTA_T='NO'&CSV_FORMAT='YES'";

  // Make HTTP request
  auto response = make_request(config_.api_endpoint, params.str());
  if (!is_success(response)) {
    return get_error(response);
  }

  // Parse response
  return parse_jpl_response(get_value(response), jpl_id);
}

/**
 * @brief Make HTTP request to JPL API
 */
JPLResult<std::string> JPLClient::make_request(const std::string& url, const std::string& params) {
  // Build curl command
  std::ostringstream cmd;
  cmd << "curl -s --max-time " << config_.request_timeout.count();
  cmd << " --data-urlencode '" << params << "'";
  cmd << " '" << url << "'";

  // Execute request with retries
  for (size_t attempt = 0; attempt < config_.max_retries; ++attempt) {
    auto response = impl_->execute_command(cmd.str());

    if (!response.empty() && response.find("ERROR") == std::string::npos) {
      return response;
    }

    if (attempt < config_.max_retries - 1) {
      std::this_thread::sleep_for(config_.retry_delay);
    }
  }

  return JPLError::NetworkError;
}

/**
 * @brief Parse JPL HORIZONS response
 */
JPLResult<EphemerisData> JPLClient::parse_jpl_response(const std::string& response, int jpl_id) {
  // This is a simplified parser - in reality, JPL responses are complex
  // For now, return placeholder data to demonstrate the structure

  EphemerisData data;
  data.jpl_id = jpl_id;
  data.body_name = "Body_" + std::to_string(jpl_id);
  data.epoch = std::chrono::system_clock::now();
  data.position = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
  data.velocity = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
  data.mass = 1.0e24;  // Placeholder mass

  // TODO: Implement actual JPL response parsing
  // This would involve parsing the complex JPL HORIZONS text format

  return data;
}

/**
 * @brief Check if current year data is available
 */
bool JPLClient::has_current_year_data() const {
  auto metadata = get_cache_metadata();
  if (!metadata) {
    return false;
  }

  auto current_year_epoch = Utils::get_current_year_epoch();
  return metadata->epoch == current_year_epoch && metadata->is_valid(config_.cache_validity);
}

/**
 * @brief Get cache metadata
 */
std::optional<CacheMetadata> JPLClient::get_cache_metadata() const { return load_cache_metadata(); }

/**
 * @brief Load cache metadata
 */
std::optional<CacheMetadata> JPLClient::load_cache_metadata() const {
  auto metadata_path = config_.cache_directory / "metadata.json";

  if (!std::filesystem::exists(metadata_path)) {
    return std::nullopt;
  }

  // TODO: Implement JSON metadata loading
  // For now, return placeholder
  CacheMetadata metadata;
  metadata.created_at = std::chrono::system_clock::now();
  metadata.epoch = Utils::get_current_year_epoch();
  metadata.source = "JPL_HORIZONS";
  metadata.body_count = 27;
  metadata.checksum = 0;

  return metadata;
}

/**
 * @brief Load ephemeris data from cache
 */
JPLResult<std::vector<EphemerisData>> JPLClient::load_from_cache() {
  // TODO: Implement cache loading
  // For now, return empty result to indicate no cache
  return JPLError::CacheError;
}

/**
 * @brief Save ephemeris data to cache
 */
JPLVoidResult JPLClient::save_to_cache(const std::vector<EphemerisData>& data) {
  // TODO: Implement cache saving
  return success();
}

/**
 * @brief Validate cache integrity
 */
JPLResult<bool> JPLClient::validate_cache() const {
  // TODO: Implement cache validation
  return true;
}

/**
 * @brief Clear all cached data
 */
JPLVoidResult JPLClient::clear_cache() {
  try {
    if (std::filesystem::exists(config_.cache_directory)) {
      std::filesystem::remove_all(config_.cache_directory);
      std::filesystem::create_directories(config_.cache_directory);
    }
    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Factory methods
 */
std::unique_ptr<JPLClient> JPLClientFactory::create_default() {
  return std::make_unique<JPLClient>();
}

std::unique_ptr<JPLClient> JPLClientFactory::create(JPLClientConfig config) {
  return std::make_unique<JPLClient>(std::move(config));
}

std::unique_ptr<JPLClient> JPLClientFactory::create_for_testing() {
  JPLClientConfig config;
  config.api_endpoint = "http://localhost:8080/mock/jpl";  // Mock endpoint
  config.cache_directory = "./test_cache";
  return std::make_unique<JPLClient>(std::move(config));
}

/**
 * @brief Utility functions
 */
namespace Utils {

std::string to_jpl_date_string(std::chrono::system_clock::time_point tp) {
  auto time_t = std::chrono::system_clock::to_time_t(tp);
  auto tm = *std::gmtime(&time_t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d");
  return oss.str();
}

std::optional<std::chrono::system_clock::time_point> from_jpl_date_string(
    std::string_view date_str) {
  std::tm tm = {};
  std::string date_string(date_str);
  std::istringstream ss(date_string);
  ss >> std::get_time(&tm, "%Y-%m-%d");

  if (ss.fail()) {
    return std::nullopt;
  }

  auto time_t = std::mktime(&tm);
  return std::chrono::system_clock::from_time_t(time_t);
}

std::chrono::system_clock::time_point get_current_year_epoch() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto tm = *std::localtime(&time_t);

  tm.tm_mon = 0;   // January
  tm.tm_mday = 1;  // 1st
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;

  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::optional<int> get_jpl_id_for_body(std::string_view body_name) {
  // TODO: Implement body name to JPL ID mapping
  // This would use the existing jpl_bodies.h mapping
  return std::nullopt;
}

std::vector<int> get_all_jpl_ids() {
  std::vector<int> all_ids;
  for (const auto& [jpl_id, body_type] : Bodies::JPL_ID_TO_TYPE) {
    all_ids.push_back(jpl_id);
  }
  return all_ids;
}

std::string to_string(JPLError error) {
  switch (error) {
    case JPLError::NetworkError:
      return "Network error";
    case JPLError::ParseError:
      return "Parse error";
    case JPLError::InvalidBody:
      return "Invalid body";
    case JPLError::InvalidDate:
      return "Invalid date";
    case JPLError::RateLimited:
      return "Rate limited";
    case JPLError::ServerError:
      return "Server error";
    case JPLError::CacheError:
      return "Cache error";
    case JPLError::ValidationError:
      return "Validation error";
    default:
      return "Unknown error";
  }
}

}  // namespace Utils

}  // namespace SolarSystem::JPL
