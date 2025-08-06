/**
 * @file jpl_client.cpp
 * @brief Implementation of modern JPL HORIZONS API client
 */

#include "solar_jpl/jpl_client.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
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
  SolarSystem::Bodies::CelestialBody::Properties props = {};
  props.name = body_name;
  props.mass = mass;
  props.position = position * 1000;  // Convert km to meters
  props.velocity = velocity * 1000;  // Convert km/s to m/s
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
  EphemerisData data;
  data.jpl_id = jpl_id;
  data.epoch = std::chrono::system_clock::now();

  // Find the body name from the response
  std::regex name_regex(R"(Target body name:\s*([^(]+))");
  std::smatch name_match;
  if (std::regex_search(response, name_match, name_regex)) {
    data.body_name = name_match[1].str();
    // Trim whitespace
    data.body_name.erase(data.body_name.find_last_not_of(" \t\n\r") + 1);
  } else {
    // If we can't parse the body name, this indicates a parsing error
    // Return an error instead of generating a fake name
    return JPLError::ParseError;
  }

  // Find mass information (if available)
  std::regex mass_regex(R"(Mass[^=]*=\s*([0-9.eE+-]+))");
  std::smatch mass_match;
  if (std::regex_search(response, mass_match, mass_regex)) {
    try {
      data.mass = std::stold(mass_match[1].str());
    } catch (const std::exception&) {
      data.mass = 1.0e24;  // Default mass
    }
  } else {
    // Use default masses based on body type
    auto body_type = Bodies::get_body_type_for_jpl_id(jpl_id);
    switch (body_type) {
      case Bodies::BodyType::Star:
        data.mass = 1.98847e30;  // Solar mass
        break;
      case Bodies::BodyType::Planet:
        data.mass = 5.97219e24;  // Earth-like mass
        break;
      case Bodies::BodyType::Moon:
        data.mass = 7.342e22;  // Moon-like mass
        break;
      case Bodies::BodyType::DwarfPlanet:
        data.mass = 1.303e22;  // Pluto-like mass
        break;
      default:
        data.mass = 1.0e20;  // Small body mass
        break;
    }
  }

  // Parse vector data (position and velocity)
  // Look for the ephemeris data section
  std::regex data_start_regex(R"(\$\$SOE)");
  std::regex data_end_regex(R"(\$\$EOE)");

  auto data_start = std::sregex_iterator(response.begin(), response.end(), data_start_regex);
  auto data_end = std::sregex_iterator(response.begin(), response.end(), data_end_regex);

  if (data_start != std::sregex_iterator() && data_end != std::sregex_iterator()) {
    size_t start_pos = static_cast<size_t>(data_start->position() + data_start->length());
    size_t end_pos = static_cast<size_t>(data_end->position());

    if (start_pos < end_pos) {
      std::string ephemeris_section = response.substr(start_pos, end_pos - start_pos);

      // Parse CSV format: Date, X, Y, Z, VX, VY, VZ
      std::istringstream stream(ephemeris_section);
      std::string line;

      while (std::getline(stream, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.find("JDTDB") != std::string::npos) {
          continue;
        }

        // Parse CSV line
        std::istringstream line_stream(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(line_stream, token, ',')) {
          // Trim whitespace
          token.erase(0, token.find_first_not_of(" \t"));
          token.erase(token.find_last_not_of(" \t") + 1);
          tokens.push_back(token);
        }

        // We need at least 7 tokens: Date, X, Y, Z, VX, VY, VZ
        if (tokens.size() >= 7) {
          try {
            // Position (km) - tokens 1, 2, 3
            double x = std::stod(tokens[1]);
            double y = std::stod(tokens[2]);
            double z = std::stod(tokens[3]);
            data.position = SolarSystem::Math::Vector3d{x, y, z};

            // Velocity (km/s) - tokens 4, 5, 6
            double vx = std::stod(tokens[4]);
            double vy = std::stod(tokens[5]);
            double vz = std::stod(tokens[6]);
            data.velocity = SolarSystem::Math::Vector3d{vx, vy, vz};

            // We found valid data, break out of loop
            break;
          } catch (const std::exception&) {
            // Continue to next line if parsing fails
            continue;
          }
        }
      }
    }
  }

  // If we couldn't parse the data, check if it's an error response
  if (data.position.magnitude() == 0.0 && data.velocity.magnitude() == 0.0) {
    if (response.find("ERROR") != std::string::npos ||
        response.find("No ephemeris") != std::string::npos ||
        response.find("Cannot find") != std::string::npos) {
      return JPLError::InvalidBody;
    }

    // Return placeholder data if parsing failed but no explicit error
    data.position = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
    data.velocity = SolarSystem::Math::Vector3d{0.0, 0.0, 0.0};
  }

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

  try {
    std::ifstream file(metadata_path);
    if (!file.is_open()) {
      return std::nullopt;
    }

    std::string line;
    std::string json_content;
    while (std::getline(file, line)) {
      json_content += line + "\n";
    }

    // Simple JSON parsing for metadata
    CacheMetadata metadata;

    // Parse created_at timestamp
    auto created_pos = json_content.find("\"created_at\":");
    if (created_pos != std::string::npos) {
      auto start = json_content.find(":", created_pos) + 1;
      auto end = json_content.find(",", start);
      if (end == std::string::npos) end = json_content.find("}", start);

      std::string timestamp_str = json_content.substr(start, end - start);
      // Remove quotes and whitespace
      timestamp_str.erase(std::remove_if(timestamp_str.begin(), timestamp_str.end(),
                                         [](char c) { return c == '"' || c == ' ' || c == '\t'; }),
                          timestamp_str.end());

      try {
        auto timestamp = std::stoll(timestamp_str);
        metadata.created_at = std::chrono::system_clock::from_time_t(timestamp);
      } catch (const std::exception&) {
        metadata.created_at = std::chrono::system_clock::now();
      }
    }

    // Parse epoch timestamp
    auto epoch_pos = json_content.find("\"epoch\":");
    if (epoch_pos != std::string::npos) {
      auto start = json_content.find(":", epoch_pos) + 1;
      auto end = json_content.find(",", start);
      if (end == std::string::npos) end = json_content.find("}", start);

      std::string timestamp_str = json_content.substr(start, end - start);
      timestamp_str.erase(std::remove_if(timestamp_str.begin(), timestamp_str.end(),
                                         [](char c) { return c == '"' || c == ' ' || c == '\t'; }),
                          timestamp_str.end());

      try {
        auto timestamp = std::stoll(timestamp_str);
        metadata.epoch = std::chrono::system_clock::from_time_t(timestamp);
      } catch (const std::exception&) {
        metadata.epoch = Utils::get_current_year_epoch();
      }
    }

    // Parse source
    auto source_pos = json_content.find("\"source\":");
    if (source_pos != std::string::npos) {
      auto start = json_content.find("\"", source_pos + 9) + 1;
      auto end = json_content.find("\"", start);
      if (end != std::string::npos) {
        metadata.source = json_content.substr(start, end - start);
      } else {
        metadata.source = "JPL_HORIZONS";
      }
    } else {
      metadata.source = "JPL_HORIZONS";
    }

    // Parse body_count
    auto count_pos = json_content.find("\"body_count\":");
    if (count_pos != std::string::npos) {
      auto start = json_content.find(":", count_pos) + 1;
      auto end = json_content.find(",", start);
      if (end == std::string::npos) end = json_content.find("}", start);

      std::string count_str = json_content.substr(start, end - start);
      count_str.erase(std::remove_if(count_str.begin(), count_str.end(),
                                     [](char c) { return c == ' ' || c == '\t'; }),
                      count_str.end());

      try {
        metadata.body_count = std::stoull(count_str);
      } catch (const std::exception&) {
        metadata.body_count = 0;
      }
    }

    // Parse checksum
    auto checksum_pos = json_content.find("\"checksum\":");
    if (checksum_pos != std::string::npos) {
      auto start = json_content.find(":", checksum_pos) + 1;
      auto end = json_content.find(",", start);
      if (end == std::string::npos) end = json_content.find("}", start);

      std::string checksum_str = json_content.substr(start, end - start);
      checksum_str.erase(std::remove_if(checksum_str.begin(), checksum_str.end(),
                                        [](char c) { return c == ' ' || c == '\t'; }),
                         checksum_str.end());

      try {
        metadata.checksum = std::stoull(checksum_str);
      } catch (const std::exception&) {
        metadata.checksum = 0;
      }
    }

    return metadata;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

/**
 * @brief Load ephemeris data from cache
 */
JPLResult<std::vector<EphemerisData>> JPLClient::load_from_cache() {
  // Try binary cache first (faster)
  auto binary_path = config_.cache_directory / "ephemeris_cache.bin";
  if (config_.enable_binary_cache && std::filesystem::exists(binary_path)) {
    try {
      std::ifstream file(binary_path, std::ios::binary);
      if (file.is_open()) {
        std::vector<EphemerisData> data;

        // Read number of bodies
        size_t body_count;
        file.read(reinterpret_cast<char*>(&body_count), sizeof(body_count));

        data.reserve(body_count);

        // Read each body's data
        for (size_t i = 0; i < body_count; ++i) {
          EphemerisData body_data;

          // Read JPL ID
          file.read(reinterpret_cast<char*>(&body_data.jpl_id), sizeof(body_data.jpl_id));

          // Read body name length and name
          size_t name_length;
          file.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));
          body_data.body_name.resize(name_length);
          file.read(&body_data.body_name[0], static_cast<std::streamsize>(name_length));

          // Read epoch
          auto epoch_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
          file.read(reinterpret_cast<char*>(&epoch_time), sizeof(epoch_time));
          body_data.epoch = std::chrono::system_clock::from_time_t(epoch_time);

          // Read position
          double pos[3];
          file.read(reinterpret_cast<char*>(pos), sizeof(pos));
          body_data.position = SolarSystem::Math::Vector3d{pos[0], pos[1], pos[2]};

          // Read velocity
          double vel[3];
          file.read(reinterpret_cast<char*>(vel), sizeof(vel));
          body_data.velocity = SolarSystem::Math::Vector3d{vel[0], vel[1], vel[2]};

          // Read mass
          file.read(reinterpret_cast<char*>(&body_data.mass), sizeof(body_data.mass));

          data.push_back(std::move(body_data));
        }

        if (file.good()) {
          return data;
        }
      }
    } catch (const std::exception&) {
      // Fall through to JSON cache
    }
  }

  // Try JSON cache as fallback
  auto json_path = config_.cache_directory / "ephemeris_data.json";
  if (config_.enable_json_cache && std::filesystem::exists(json_path)) {
    try {
      std::ifstream file(json_path);
      if (!file.is_open()) {
        return JPLError::CacheError;
      }

      std::string json_content;
      std::string line;
      while (std::getline(file, line)) {
        json_content += line + "\n";
      }

      std::vector<EphemerisData> data;

      // Simple JSON parsing for ephemeris data
      // Look for "bodies" array
      auto bodies_pos = json_content.find("\"bodies\":");
      if (bodies_pos == std::string::npos) {
        return JPLError::ParseError;
      }

      auto array_start = json_content.find("[", bodies_pos);
      auto array_end = json_content.rfind("]");

      if (array_start == std::string::npos || array_end == std::string::npos) {
        return JPLError::ParseError;
      }

      std::string bodies_json = json_content.substr(array_start + 1, array_end - array_start - 1);

      // Parse each body object
      size_t pos = 0;
      while (pos < bodies_json.length()) {
        auto obj_start = bodies_json.find("{", pos);
        if (obj_start == std::string::npos) break;

        auto obj_end = bodies_json.find("}", obj_start);
        if (obj_end == std::string::npos) break;

        std::string body_json = bodies_json.substr(obj_start, obj_end - obj_start + 1);

        EphemerisData body_data;

        // Parse JPL ID
        auto jpl_id_pos = body_json.find("\"jpl_id\":");
        if (jpl_id_pos != std::string::npos) {
          auto start = body_json.find(":", jpl_id_pos) + 1;
          auto end = body_json.find(",", start);
          if (end == std::string::npos) end = body_json.find("}", start);

          std::string id_str = body_json.substr(start, end - start);
          id_str.erase(std::remove_if(id_str.begin(), id_str.end(),
                                      [](char c) { return c == ' ' || c == '\t'; }),
                       id_str.end());

          try {
            body_data.jpl_id = std::stoi(id_str);
          } catch (const std::exception&) {
            body_data.jpl_id = 0;
          }
        }

        // Parse body name
        auto name_pos = body_json.find("\"body_name\":");
        if (name_pos != std::string::npos) {
          auto start = body_json.find("\"", name_pos + 12) + 1;
          auto end = body_json.find("\"", start);
          if (end != std::string::npos) {
            body_data.body_name = body_json.substr(start, end - start);
          }
        }

        // Parse position
        auto pos_pos = body_json.find("\"position\":");
        if (pos_pos != std::string::npos) {
          auto arr_start = body_json.find("[", pos_pos);
          auto arr_end = body_json.find("]", arr_start);
          if (arr_start != std::string::npos && arr_end != std::string::npos) {
            std::string pos_str = body_json.substr(arr_start + 1, arr_end - arr_start - 1);
            std::istringstream pos_stream(pos_str);
            std::string token;
            std::vector<double> coords;

            while (std::getline(pos_stream, token, ',')) {
              token.erase(std::remove_if(token.begin(), token.end(),
                                         [](char c) { return c == ' ' || c == '\t'; }),
                          token.end());
              try {
                coords.push_back(std::stod(token));
              } catch (const std::exception&) {
                coords.push_back(0.0);
              }
            }

            if (coords.size() >= 3) {
              body_data.position = SolarSystem::Math::Vector3d{coords[0], coords[1], coords[2]};
            }
          }
        }

        // Parse velocity
        auto vel_pos = body_json.find("\"velocity\":");
        if (vel_pos != std::string::npos) {
          auto arr_start = body_json.find("[", vel_pos);
          auto arr_end = body_json.find("]", arr_start);
          if (arr_start != std::string::npos && arr_end != std::string::npos) {
            std::string vel_str = body_json.substr(arr_start + 1, arr_end - arr_start - 1);
            std::istringstream vel_stream(vel_str);
            std::string token;
            std::vector<double> coords;

            while (std::getline(vel_stream, token, ',')) {
              token.erase(std::remove_if(token.begin(), token.end(),
                                         [](char c) { return c == ' ' || c == '\t'; }),
                          token.end());
              try {
                coords.push_back(std::stod(token));
              } catch (const std::exception&) {
                coords.push_back(0.0);
              }
            }

            if (coords.size() >= 3) {
              body_data.velocity = SolarSystem::Math::Vector3d{coords[0], coords[1], coords[2]};
            }
          }
        }

        // Parse mass
        auto mass_pos = body_json.find("\"mass\":");
        if (mass_pos != std::string::npos) {
          auto start = body_json.find(":", mass_pos) + 1;
          auto end = body_json.find(",", start);
          if (end == std::string::npos) end = body_json.find("}", start);

          std::string mass_str = body_json.substr(start, end - start);
          mass_str.erase(std::remove_if(mass_str.begin(), mass_str.end(),
                                        [](char c) { return c == ' ' || c == '\t'; }),
                         mass_str.end());

          try {
            body_data.mass = std::stold(mass_str);
          } catch (const std::exception&) {
            body_data.mass = 1.0e24;
          }
        }

        body_data.epoch = std::chrono::system_clock::now();
        data.push_back(std::move(body_data));

        pos = obj_end + 1;
      }

      return data;
    } catch (const std::exception&) {
      return JPLError::CacheError;
    }
  }

  return JPLError::CacheError;
}

/**
 * @brief Save ephemeris data to cache
 */
JPLVoidResult JPLClient::save_to_cache(const std::vector<EphemerisData>& data) {
  try {
    // Ensure cache directory exists
    std::filesystem::create_directories(config_.cache_directory);

    // Save binary cache (faster loading)
    if (config_.enable_binary_cache) {
      auto binary_path = config_.cache_directory / "ephemeris_cache.bin";
      std::ofstream file(binary_path, std::ios::binary);
      if (file.is_open()) {
        // Write number of bodies
        size_t body_count = data.size();
        file.write(reinterpret_cast<const char*>(&body_count), sizeof(body_count));

        // Write each body's data
        for (const auto& body_data : data) {
          // Write JPL ID
          file.write(reinterpret_cast<const char*>(&body_data.jpl_id), sizeof(body_data.jpl_id));

          // Write body name length and name
          size_t name_length = body_data.body_name.length();
          file.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
          file.write(body_data.body_name.c_str(), static_cast<std::streamsize>(name_length));

          // Write epoch
          auto epoch_time = std::chrono::system_clock::to_time_t(body_data.epoch);
          file.write(reinterpret_cast<const char*>(&epoch_time), sizeof(epoch_time));

          // Write position
          double pos[3] = {static_cast<double>(body_data.position.x()),
                           static_cast<double>(body_data.position.y()),
                           static_cast<double>(body_data.position.z())};
          file.write(reinterpret_cast<const char*>(pos), sizeof(pos));

          // Write velocity
          double vel[3] = {static_cast<double>(body_data.velocity.x()),
                           static_cast<double>(body_data.velocity.y()),
                           static_cast<double>(body_data.velocity.z())};
          file.write(reinterpret_cast<const char*>(vel), sizeof(vel));

          // Write mass
          file.write(reinterpret_cast<const char*>(&body_data.mass), sizeof(body_data.mass));
        }
      }
    }

    // Save JSON cache (human readable)
    if (config_.enable_json_cache) {
      auto json_path = config_.cache_directory / "ephemeris_data.json";
      std::ofstream file(json_path);
      if (file.is_open()) {
        file << "{\n";
        file << "  \"metadata\": {\n";
        file << "    \"created_at\": "
             << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << ",\n";
        file << "    \"body_count\": " << data.size() << ",\n";
        file << "    \"source\": \"JPL_HORIZONS\"\n";
        file << "  },\n";
        file << "  \"bodies\": [\n";

        for (size_t i = 0; i < data.size(); ++i) {
          const auto& body_data = data[i];

          file << "    {\n";
          file << "      \"jpl_id\": " << body_data.jpl_id << ",\n";
          file << "      \"body_name\": \"" << body_data.body_name << "\",\n";
          file << "      \"epoch\": " << std::chrono::system_clock::to_time_t(body_data.epoch)
               << ",\n";
          file << "      \"position\": [" << std::scientific << std::setprecision(15)
               << body_data.position.x() << ", " << body_data.position.y() << ", "
               << body_data.position.z() << "],\n";
          file << "      \"velocity\": [" << std::scientific << std::setprecision(15)
               << body_data.velocity.x() << ", " << body_data.velocity.y() << ", "
               << body_data.velocity.z() << "],\n";
          file << "      \"mass\": " << std::scientific << std::setprecision(15) << body_data.mass
               << "\n";
          file << "    }";

          if (i < data.size() - 1) {
            file << ",";
          }
          file << "\n";
        }

        file << "  ]\n";
        file << "}\n";
      }
    }

    // Save metadata
    auto metadata_path = config_.cache_directory / "metadata.json";
    std::ofstream metadata_file(metadata_path);
    if (metadata_file.is_open()) {
      // Calculate simple checksum
      uint64_t checksum = 0;
      for (const auto& body_data : data) {
        checksum += static_cast<uint64_t>(body_data.jpl_id);
        checksum += static_cast<uint64_t>(body_data.position.magnitude() * 1000);
      }

      metadata_file << "{\n";
      metadata_file << "  \"created_at\": "
                    << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
                    << ",\n";
      metadata_file << "  \"epoch\": "
                    << std::chrono::system_clock::to_time_t(Utils::get_current_year_epoch())
                    << ",\n";
      metadata_file << "  \"source\": \"JPL_HORIZONS\",\n";
      metadata_file << "  \"body_count\": " << data.size() << ",\n";
      metadata_file << "  \"checksum\": " << checksum << "\n";
      metadata_file << "}\n";
    }

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Validate cache integrity
 */
JPLResult<bool> JPLClient::validate_cache() const {
  try {
    auto metadata = get_cache_metadata();
    if (!metadata) {
      return false;  // No metadata means invalid cache
    }

    // Check if cache files exist
    auto binary_path = config_.cache_directory / "ephemeris_cache.bin";
    auto json_path = config_.cache_directory / "ephemeris_data.json";

    bool has_binary = config_.enable_binary_cache && std::filesystem::exists(binary_path);
    bool has_json = config_.enable_json_cache && std::filesystem::exists(json_path);

    if (!has_binary && !has_json) {
      return false;  // No cache files
    }

    // Check if cache is still valid (not expired)
    if (!metadata->is_valid(config_.cache_validity)) {
      return false;  // Cache expired
    }

    // Try to load cache to verify integrity
    auto cache_result = const_cast<JPLClient*>(this)->load_from_cache();
    if (!is_success(cache_result)) {
      return false;  // Cache corrupted
    }

    auto cached_data = get_value(cache_result);
    if (cached_data.size() != metadata->body_count) {
      return false;  // Body count mismatch
    }

    // Calculate checksum and verify
    uint64_t calculated_checksum = 0;
    for (const auto& body_data : cached_data) {
      calculated_checksum += static_cast<uint64_t>(body_data.jpl_id);
      calculated_checksum += static_cast<uint64_t>(body_data.position.magnitude() * 1000);
    }

    if (calculated_checksum != metadata->checksum) {
      return false;  // Checksum mismatch
    }

    return true;  // Cache is valid
  } catch (const std::exception&) {
    return JPLError::ValidationError;
  }
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

JPLVoidResult JPLClient::rebuild_cache() {
  try {
    // Clear existing cache
    auto clear_result = clear_cache();
    if (!is_success(clear_result)) {
      return clear_result;
    }

    // Fetch fresh data from JPL
    auto current_epoch = Utils::get_current_year_epoch();
    auto fetch_future = fetch_all_bodies_async(current_epoch);

    // Wait for fetch to complete
    auto fetch_result = fetch_future.get();
    if (!is_success(fetch_result)) {
      return error(get_error(fetch_result));
    }

    // Save to cache
    auto data = get_value(fetch_result);
    return save_to_cache(data);
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

JPLVoidResult JPLClient::test_storage() {
  try {
    // Test cache directory creation
    std::filesystem::create_directories(config_.cache_directory);

    // Test writing a small test file
    auto test_path = config_.cache_directory / "test_file.tmp";
    {
      std::ofstream test_file(test_path);
      if (!test_file.is_open()) {
        return error(JPLError::CacheError);
      }
      test_file << "test data";
    }

    // Test reading the file back
    {
      std::ifstream test_file(test_path);
      if (!test_file.is_open()) {
        return error(JPLError::CacheError);
      }
      std::string content;
      test_file >> content;
      if (content != "test") {
        return error(JPLError::CacheError);
      }
    }

    // Clean up test file
    std::filesystem::remove(test_path);

    // Test metadata operations
    std::vector<EphemerisData> test_data;
    EphemerisData test_body;
    test_body.jpl_id = 399;
    test_body.body_name = "Earth";
    test_body.epoch = std::chrono::system_clock::now();
    test_body.position = SolarSystem::Math::Vector3d{1.0e8, 0.0, 0.0};
    test_body.velocity = SolarSystem::Math::Vector3d{0.0, 30000.0, 0.0};
    test_body.mass = 5.97219e24;
    test_data.push_back(test_body);

    // Test save and load cycle
    auto save_result = save_to_cache(test_data);
    if (!is_success(save_result)) {
      return save_result;
    }

    auto load_result = load_from_cache();
    if (!is_success(load_result)) {
      return error(get_error(load_result));
    }

    auto loaded_data = get_value(load_result);
    if (loaded_data.empty() || loaded_data[0].jpl_id != 399) {
      return error(JPLError::ValidationError);
    }

    // Test cache validation
    auto validate_result = validate_cache();
    if (!is_success(validate_result) || !get_value(validate_result)) {
      return error(JPLError::ValidationError);
    }

    // Clean up test cache
    auto cleanup_result = clear_cache();
    (void)cleanup_result;  // Ignore result for cleanup

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
  return Bodies::get_jpl_id_for_body_name(body_name);
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
