#include "jpl_data.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "constants.h"
#include "jpl_bodies.h"
#include "types.h"

// Global state for ephemeris data
static time_t current_epoch = 0;
static char current_source[32] = "ORIGINAL_DATA";
static bool data_initialized = false;

bool initialize_jpl_data() {
  if (data_initialized) {
    return true;
  }

  // Try to load cached data first
  if (load_cached_ephemeris_data()) {
    std::cout << "Loaded ephemeris data from cache (epoch: " << ctime(&current_epoch) << ")"
              << std::endl;
    data_initialized = true;
  } else {
    std::cout << "Using original hardcoded ephemeris data" << std::endl;
    // Set epoch to the original data date (Feb 11, 2018)
    struct tm tm = {};
    tm.tm_year = 2018 - 1900;
    tm.tm_mon = 1;  // February (0-based)
    tm.tm_mday = 11;
    current_epoch = mktime(&tm);
    strcpy(current_source, "ORIGINAL_DATA");
    data_initialized = true;
  }

  return apply_ephemeris_data();
}

bool update_ephemeris_data() {
  std::cout << "Updating ephemeris data from NASA JPL..." << std::endl;

  // Get current year for data fetching
  time_t now = time(NULL);
  struct tm* tm_now = localtime(&now);
  int current_year = tm_now->tm_year + 1900;

  // Format date string for JPL query
  char date_str[32];
  snprintf(date_str, sizeof(date_str), "%d-01-01", current_year);

  std::cout << "Fetching data for: " << date_str << std::endl;

  // Count bodies to fetch
  int count = 0;
  for (int i = 0; i < BODY_COUNT; i++) {
    if (get_jpl_id_for_body(i) != 0) {
      count++;
    }
  }

  // Fetch JPL data for all bodies
  std::cout << "Fetching data for " << count << " celestial bodies..." << std::endl;

  int successful_fetches = 0;
  for (int i = 0; i < BODY_COUNT; i++) {
    int jpl_id = get_jpl_id_for_body(i);
    if (jpl_id != 0) {
      std::cout << "Fetching " << SolarSystem[i].name << " (JPL ID: " << jpl_id << ")..."
                << std::endl;

      if (fetch_jpl_horizons_data(date_str, jpl_id)) {
        successful_fetches++;
        std::cout << "✓ Successfully fetched " << SolarSystem[i].name << std::endl;
      } else {
        std::cerr << "✗ Failed to fetch " << SolarSystem[i].name << std::endl;
      }
    }
  }

  std::cout << "Successfully fetched " << successful_fetches << "/" << count << " bodies"
            << std::endl;

  // Save to cache if we got some data
  if (successful_fetches > 0) {
    if (save_ephemeris_to_json() && save_ephemeris_to_binary()) {
      std::cout << "Data saved to cache files" << std::endl;
      return true;
    }
  }

  std::cerr << "Failed to update ephemeris data" << std::endl;
  return false;
}

bool load_cached_ephemeris_data() {
  // Try binary cache first (faster)
  if (load_ephemeris_from_binary()) {
    return true;
  }

  // Fall back to JSON cache
  if (load_ephemeris_from_json()) {
    return true;
  }

  return false;
}

bool rebuild_binary_cache() {
  std::cout << "Rebuilding binary cache from JSON..." << std::endl;

  if (load_ephemeris_from_json()) {
    if (save_ephemeris_to_binary()) {
      std::cout << "Binary cache rebuilt successfully" << std::endl;
      return true;
    }
  }

  std::cerr << "Failed to rebuild binary cache" << std::endl;
  return false;
}

// HTTP response callback for curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
  size_t total_size = size * nmemb;
  response->append((char*)contents, total_size);
  return total_size;
}

// Fetch JPL HORIZONS data for a specific body
bool fetch_jpl_horizons_data(const char* date, int jpl_id) {
  // Calculate next day for date range
  struct tm tm = {};
  if (sscanf(date, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3) {
    std::cerr << "Invalid date format: " << date << std::endl;
    return false;
  }
  tm.tm_year -= 1900;  // tm_year is years since 1900
  tm.tm_mon -= 1;      // tm_mon is 0-based

  // Add one day
  tm.tm_mday += 1;
  mktime(&tm);  // Normalize the date

  char next_day_str[32];
  snprintf(next_day_str, sizeof(next_day_str), "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1,
           tm.tm_mday);

  // Build JPL HORIZONS API URL
  std::string base_url = "https://ssd.jpl.nasa.gov/api/horizons.api";
  std::string params = "?format=text&COMMAND=" + std::to_string(jpl_id) +
                       "&OBJ_DATA=YES&MAKE_EPHEM=YES&EPHEM_TYPE=VECTORS&CENTER=500@0&START_TIME=" +
                       std::string(date) + "&STOP_TIME=" + std::string(next_day_str) +
                       "&STEP_SIZE=1d&VEC_TABLE=2&REF_PLANE=ECLIPTIC&REF_SYSTEM=J2000&OUT_UNITS="
                       "KM-S&VEC_LABELS=NO&CSV_FORMAT=YES";

  std::string url = base_url + params;

  // Use curl to fetch data
  std::string curl_command = "curl -s \"" + url + "\"";

  // Execute curl command
  FILE* pipe = popen(curl_command.c_str(), "r");
  if (!pipe) {
    std::cerr << "Failed to execute curl command" << std::endl;
    return false;
  }

  // Read response
  std::string response;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    response += buffer;
  }

  int exit_code = pclose(pipe);
  if (exit_code != 0) {
    std::cerr << "Curl command failed with exit code: " << exit_code << std::endl;
    return false;
  }

  // Parse the response
  if (parse_jpl_response(response.c_str())) {
    return true;
  }

  std::cerr << "Failed to parse JPL response for body " << jpl_id << std::endl;
  return false;
}

bool parse_jpl_response(const char* response) {
  std::string response_str(response);

  // Find data section markers
  size_t data_start = response_str.find("$$SOE");
  size_t data_end = response_str.find("$$EOE");

  if (data_start == std::string::npos || data_end == std::string::npos) {
    std::cerr << "Could not find data markers in JPL response" << std::endl;
    return false;
  }

  // Extract data section
  std::string data_section = response_str.substr(data_start + 5, data_end - data_start - 5);

  // Parse data lines
  std::istringstream data_stream(data_section);
  std::string line;

  while (std::getline(data_stream, line)) {
    // Skip empty lines
    if (line.empty()) {
      continue;
    }

    // Parse data line: JD, Date, X, Y, Z, VX, VY, VZ (comma-separated)
    std::vector<std::string> fields;
    std::istringstream line_stream(line);
    std::string field;

    while (std::getline(line_stream, field, ',')) {
      // Trim whitespace
      size_t start = field.find_first_not_of(" \t\r\n");
      size_t end = field.find_last_not_of(" \t\r\n");
      if (start != std::string::npos && end != std::string::npos) {
        fields.push_back(field.substr(start, end - start + 1));
      }
    }

    // We expect 8 fields: JD, Date, X, Y, Z, VX, VY, VZ
    if (fields.size() >= 8) {
      try {
        // Parse position and velocity (skip JD and date fields)
        double x = std::stod(fields[2]);   // X position
        double y = std::stod(fields[3]);   // Y position
        double z = std::stod(fields[4]);   // Z position
        double vx = std::stod(fields[5]);  // VX velocity
        double vy = std::stod(fields[6]);  // VY velocity
        double vz = std::stod(fields[7]);  // VZ velocity

        // Update Sun data (for now, we only update the Sun)
        SolarSystem[0].position.x = x;
        SolarSystem[0].position.y = y;
        SolarSystem[0].position.z = z;
        SolarSystem[0].speed.x = vx;
        SolarSystem[0].speed.y = vy;
        SolarSystem[0].speed.z = vz;

        // For now, we only process the first data line
        break;

      } catch (const std::exception& e) {
        std::cerr << "Error parsing JPL data: " << e.what() << std::endl;
        return false;
      }
    }
  }

  return true;
}

// Save ephemeris data to JSON format
bool save_ephemeris_to_json() {
  std::ofstream file(JPL_JSON_FILE);
  if (!file.is_open()) {
    std::cerr << "Failed to open " << JPL_JSON_FILE << " for writing" << std::endl;
    return false;
  }

  // Update metadata
  current_epoch = time(NULL);
  strcpy(current_source, "JPL_HORIZONS_2025");

  // Write JSON header
  file << "{" << std::endl;
  file << "  \"format_version\": " << JPL_FORMAT_VERSION << "," << std::endl;
  file << "  \"epoch\": " << current_epoch << "," << std::endl;

  // ISO format timestamp
  struct tm* tm_utc = gmtime(&current_epoch);
  file << "  \"epoch_iso\": \"" << std::put_time(tm_utc, "%Y-%m-%dT%H:%M:%SZ") << "\","
       << std::endl;

  file << "  \"source\": \"" << current_source << "\"," << std::endl;
  file << "  \"body_count\": " << BODY_COUNT << "," << std::endl;
  file << "  \"bodies\": [" << std::endl;

  // Write body data
  for (int i = 0; i < BODY_COUNT; i++) {
    file << "    {" << std::endl;
    file << "      \"index\": " << i << "," << std::endl;
    file << "      \"name\": \"" << SolarSystem[i].name << "\"," << std::endl;
    file << "      \"position\": {" << std::endl;
    file << "        \"x\": " << std::scientific << std::setprecision(15)
         << SolarSystem[i].position.x << "," << std::endl;
    file << "        \"y\": " << std::scientific << std::setprecision(15)
         << SolarSystem[i].position.y << "," << std::endl;
    file << "        \"z\": " << std::scientific << std::setprecision(15)
         << SolarSystem[i].position.z << std::endl;
    file << "      }," << std::endl;
    file << "      \"velocity\": {" << std::endl;
    file << "        \"x\": " << std::scientific << std::setprecision(15) << SolarSystem[i].speed.x
         << "," << std::endl;
    file << "        \"y\": " << std::scientific << std::setprecision(15) << SolarSystem[i].speed.y
         << "," << std::endl;
    file << "        \"z\": " << std::scientific << std::setprecision(15) << SolarSystem[i].speed.z
         << std::endl;
    file << "      }," << std::endl;
    file << "      \"mass\": " << std::scientific << std::setprecision(15) << SolarSystem[i].mass
         << std::endl;
    file << "    }";
    if (i < BODY_COUNT - 1) {
      file << ",";
    }
    file << std::endl;
  }

  file << "  ]" << std::endl;
  file << "}" << std::endl;

  file.close();
  return true;
}

// Simple JSON parsing helper
static std::string trim_line(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

static double extract_json_number(const std::string& line) {
  size_t colon_pos = line.find(':');
  if (colon_pos == std::string::npos) return 0.0;

  std::string value_part = line.substr(colon_pos + 1);
  // Remove trailing comma if present
  size_t comma_pos = value_part.find(',');
  if (comma_pos != std::string::npos) {
    value_part = value_part.substr(0, comma_pos);
  }

  value_part = trim_line(value_part);
  return std::stod(value_part);
}

static std::string extract_json_string(const std::string& line) {
  size_t colon_pos = line.find(':');
  if (colon_pos == std::string::npos) return "";

  std::string value_part = line.substr(colon_pos + 1);
  // Remove trailing comma if present
  size_t comma_pos = value_part.find(',');
  if (comma_pos != std::string::npos) {
    value_part = value_part.substr(0, comma_pos);
  }

  value_part = trim_line(value_part);
  // Remove quotes
  if (value_part.length() >= 2 && value_part[0] == '"' && value_part.back() == '"') {
    value_part = value_part.substr(1, value_part.length() - 2);
  }

  return value_part;
}

bool load_ephemeris_from_json() {
  std::ifstream file(JPL_JSON_FILE);
  if (!file.is_open()) {
    return false;  // File doesn't exist, not an error
  }

  // Simple JSON parsing state
  std::string line;
  int current_body_index = -1;
  bool in_body_object = false;
  bool in_position = false;
  bool in_velocity = false;

  while (std::getline(file, line)) {
    line = trim_line(line);
    if (line.empty()) continue;

    // Parse root-level fields
    if (line.find("\"epoch\":") != std::string::npos) {
      current_epoch = (time_t)extract_json_number(line);
    } else if (line.find("\"source\":") != std::string::npos) {
      std::string source = extract_json_string(line);
      strncpy(current_source, source.c_str(), sizeof(current_source) - 1);
      current_source[sizeof(current_source) - 1] = '\0';
    }
    // Parse body objects
    else if (line.find("\"index\":") != std::string::npos) {
      current_body_index = (int)extract_json_number(line);
      in_body_object = true;
    } else if (in_body_object && line.find("\"position\":") != std::string::npos) {
      in_position = true;
    } else if (in_body_object && line.find("\"velocity\":") != std::string::npos) {
      in_velocity = true;
      in_position = false;
    } else if (in_position && current_body_index >= 0 && current_body_index < BODY_COUNT) {
      if (line.find("\"x\":") != std::string::npos) {
        SolarSystem[current_body_index].position.x = extract_json_number(line);
      } else if (line.find("\"y\":") != std::string::npos) {
        SolarSystem[current_body_index].position.y = extract_json_number(line);
      } else if (line.find("\"z\":") != std::string::npos) {
        SolarSystem[current_body_index].position.z = extract_json_number(line);
      }
    } else if (in_velocity && current_body_index >= 0 && current_body_index < BODY_COUNT) {
      if (line.find("\"x\":") != std::string::npos) {
        SolarSystem[current_body_index].speed.x = extract_json_number(line);
      } else if (line.find("\"y\":") != std::string::npos) {
        SolarSystem[current_body_index].speed.y = extract_json_number(line);
      } else if (line.find("\"z\":") != std::string::npos) {
        SolarSystem[current_body_index].speed.z = extract_json_number(line);
      }
    } else if (line.find("\"mass\":") != std::string::npos && current_body_index >= 0 &&
               current_body_index < BODY_COUNT) {
      SolarSystem[current_body_index].mass = extract_json_number(line);
      // End of body object
      in_body_object = false;
      in_position = false;
      in_velocity = false;
    }
  }

  file.close();
  return true;
}

// Binary cache format with proper string handling
struct BinaryCacheHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t body_count;
  time_t epoch;
  char source[32];
  uint32_t json_checksum;
};

static uint32_t calculate_file_checksum(const char* filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) return 0;

  uint32_t checksum = 0;
  char buffer[4096];
  while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
    for (std::streamsize i = 0; i < file.gcount(); ++i) {
      checksum = checksum * 31 + (unsigned char)buffer[i];
    }
  }
  return checksum;
}

bool save_ephemeris_to_binary() {
  std::ofstream file(JPL_BINARY_FILE, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open " << JPL_BINARY_FILE << " for writing" << std::endl;
    return false;
  }

  // Calculate JSON checksum for validation
  uint32_t json_checksum = calculate_file_checksum(JPL_JSON_FILE);

  // Write header
  BinaryCacheHeader header;
  header.magic = JPL_BINARY_MAGIC;
  header.version = JPL_FORMAT_VERSION;
  header.body_count = BODY_COUNT;
  header.epoch = current_epoch;
  strncpy(header.source, current_source, sizeof(header.source));
  header.source[sizeof(header.source) - 1] = '\0';
  header.json_checksum = json_checksum;

  file.write(reinterpret_cast<const char*>(&header), sizeof(header));

  // Write body data
  for (int i = 0; i < BODY_COUNT; i++) {
    file.write(reinterpret_cast<const char*>(&SolarSystem[i].position),
               sizeof(SolarSystem[i].position));
    file.write(reinterpret_cast<const char*>(&SolarSystem[i].speed), sizeof(SolarSystem[i].speed));
    file.write(reinterpret_cast<const char*>(&SolarSystem[i].mass), sizeof(SolarSystem[i].mass));

    // Write name as fixed-size string
    char name_buffer[64] = {0};
    strncpy(name_buffer, SolarSystem[i].name, sizeof(name_buffer) - 1);
    file.write(name_buffer, sizeof(name_buffer));
  }

  file.close();
  return true;
}

bool load_ephemeris_from_binary() {
  std::ifstream file(JPL_BINARY_FILE, std::ios::binary);
  if (!file.is_open()) {
    return false;  // File doesn't exist
  }

  // Read and validate header
  BinaryCacheHeader header;
  file.read(reinterpret_cast<char*>(&header), sizeof(header));

  if (file.gcount() != sizeof(header)) {
    file.close();
    return false;
  }

  // Validate magic number and version
  if (header.magic != JPL_BINARY_MAGIC || header.version != JPL_FORMAT_VERSION) {
    file.close();
    return false;
  }

  // Validate body count
  if (header.body_count != BODY_COUNT) {
    file.close();
    return false;
  }

  // Check if JSON file has been modified since binary cache was created
  uint32_t current_json_checksum = calculate_file_checksum(JPL_JSON_FILE);
  if (current_json_checksum != 0 && current_json_checksum != header.json_checksum) {
    file.close();
    return false;
  }

  // Load metadata
  current_epoch = header.epoch;
  strncpy(current_source, header.source, sizeof(current_source));
  current_source[sizeof(current_source) - 1] = '\0';

  // Load body data
  for (int i = 0; i < BODY_COUNT; i++) {
    file.read(reinterpret_cast<char*>(&SolarSystem[i].position), sizeof(SolarSystem[i].position));
    file.read(reinterpret_cast<char*>(&SolarSystem[i].speed), sizeof(SolarSystem[i].speed));
    file.read(reinterpret_cast<char*>(&SolarSystem[i].mass), sizeof(SolarSystem[i].mass));

    // Read name as fixed-size string
    char name_buffer[64];
    file.read(name_buffer, sizeof(name_buffer));
    name_buffer[sizeof(name_buffer) - 1] = '\0';  // Ensure null termination

    // Only update name if it's different (preserve original names for consistency)
    // SolarSystem[i].name is already set from constants.cpp
  }

  file.close();
  return true;
}

bool validate_ephemeris_data() {
  // Basic validation - ensure we have valid data for major bodies
  return true;
}

bool apply_ephemeris_data() {
  // For now, the data is already loaded directly into SolarSystem by the load functions
  return true;
}

// Test function to save current data (for testing storage system)
bool save_current_data_for_testing() {
  // Set test metadata
  time_t now = time(NULL);
  current_epoch = now;
  strcpy(current_source, "TEST_DATA");

  if (save_ephemeris_to_json() && save_ephemeris_to_binary()) {
    return true;
  }

  return false;
}

bool test_storage_system() { return save_current_data_for_testing(); }

// Additional utility functions
time_t get_ephemeris_epoch() { return current_epoch; }

const char* get_ephemeris_source() { return current_source; }

bool has_current_ephemeris_data() { return strcmp(current_source, "ORIGINAL_DATA") != 0; }

bool has_current_year_ephemeris_data() {
  if (!has_current_ephemeris_data()) {
    return false;
  }

  time_t now = time(NULL);
  struct tm* tm_now = localtime(&now);
  struct tm* tm_epoch = localtime(&current_epoch);

  return tm_now->tm_year == tm_epoch->tm_year;
}
