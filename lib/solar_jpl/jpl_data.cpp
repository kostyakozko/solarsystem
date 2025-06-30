#include "jpl_data.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "constants.h"
#include "jpl_bodies.h"
#include "types.h"

// Global state for ephemeris data
static time_t current_epoch = 0;
static char current_source[32] = "ORIGINAL_DATA";
static bool data_initialized = false;

// Thread-safe data structures for parallel fetching with retry logic
static std::mutex fetch_mutex;
static std::atomic<int> completed_fetches(0);
static std::atomic<int> successful_fetches(0);

// Retry configuration
static const int MAX_RETRIES = 3;
static const int RETRY_DELAY_MS = 500;    // 500ms between retries
static const int REQUEST_DELAY_MS = 200;  // 200ms between requests

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
  std::cout << "Checking ephemeris data currency..." << std::endl;

  // Get current year
  time_t now = time(NULL);
  const struct tm* tm_now = localtime(&now);
  int current_year = tm_now->tm_year + 1900;

  // Check if we already have current year's data
  if (has_current_year_ephemeris_data()) {
    const struct tm* tm_epoch = localtime(&current_epoch);
    int cached_year = tm_epoch->tm_year + 1900;

    std::cout << "✓ Ephemeris data is already up to date for " << cached_year << std::endl;
    std::cout << "  Source: " << current_source << std::endl;
    std::cout << "  Last updated: " << ctime(&current_epoch);
    std::cout << "No update needed." << std::endl;
    return true;
  }

  std::cout << "Updating ephemeris data from NASA JPL for " << current_year << "..." << std::endl;

  // Format date string for JPL query
  char date_str[32];
  snprintf(date_str, sizeof(date_str), "%d-01-01", current_year);

  std::cout << "Fetching data for: " << date_str << std::endl;

  // Collect all bodies to fetch
  std::vector<FetchTask> tasks;
  for (int i = 0; i < BODY_COUNT; i++) {
    int jpl_id = get_jpl_id_for_body(i);
    if (jpl_id != 0) {
      FetchTask task;
      task.body_index = i;
      task.jpl_id = jpl_id;
      task.date_str = std::string(date_str);
      task.success = false;
      tasks.push_back(task);
    }
  }

  std::cout << "Fetching data for " << tasks.size() << " celestial bodies..." << std::endl;

  // Reset counters
  completed_fetches = 0;
  successful_fetches = 0;

  // Determine optimal number of threads (2-3 concurrent requests to minimize rate limiting)
  const int max_threads = std::min(3, std::max(2, (int)std::thread::hardware_concurrency()));
  const int num_threads = std::min(max_threads, (int)tasks.size());

  std::cout << "Using " << num_threads << " parallel connections..." << std::endl;

  // Launch parallel fetch operations
  std::vector<std::future<void>> futures;

  for (int t = 0; t < num_threads; t++) {
    futures.push_back(std::async(std::launch::async, [&tasks, t, num_threads]() {
      // Each thread processes every nth task
      for (size_t i = t; i < tasks.size(); i += num_threads) {
        fetch_body_data_parallel(tasks[i]);
      }
    }));
  }

  // Wait for all threads to complete
  for (auto& future : futures) {
    future.wait();
  }

  std::cout << "Parallel phase: " << successful_fetches.load() << "/" << tasks.size()
            << " bodies fetched" << std::endl;

  // Sequential retry for failed bodies
  if (successful_fetches.load() < (int)tasks.size()) {
    std::cout << "Retrying failed bodies sequentially..." << std::endl;

    for (auto& task : tasks) {
      if (!task.success) {
        std::cout << "Sequential retry for " << SolarSystem[task.body_index].name << "..."
                  << std::endl;

        // Try up to MAX_RETRIES times with longer delays
        bool success = false;
        for (int attempt = 1; attempt <= MAX_RETRIES && !success; attempt++) {
          if (attempt > 1) {
            std::cout << "  Sequential attempt " << attempt << "/" << MAX_RETRIES << " for "
                      << SolarSystem[task.body_index].name << "..." << std::endl;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(RETRY_DELAY_MS * 2));  // Longer delay
          }

          success = fetch_jpl_horizons_data(task.date_str.c_str(), task.jpl_id, task.body_index);
        }

        if (success) {
          successful_fetches++;
          task.success = true;
          std::cout << "✓ Sequential retry successful for " << SolarSystem[task.body_index].name
                    << std::endl;
        } else {
          std::cerr << "✗ Sequential retry failed for " << SolarSystem[task.body_index].name
                    << std::endl;
        }
      }
    }
  }

  std::cout << "Successfully fetched " << successful_fetches.load() << "/" << tasks.size()
            << " bodies" << std::endl;

  // Apply proper success criteria based on body classification
  int essential_success = 0;
  int essential_total = 0;
  int important_success = 0;
  int important_total = 0;
  int optional_success = 0;
  int optional_total = 0;

  for (const auto& task : tasks) {
    BodyType body_type = get_body_type(task.body_index);

    switch (body_type) {
      case BODY_ESSENTIAL:
        essential_total++;
        if (task.success) essential_success++;
        break;
      case BODY_IMPORTANT:
        important_total++;
        if (task.success) important_success++;
        break;
      case BODY_OPTIONAL:
        optional_total++;
        if (task.success) optional_success++;
        break;
      default:
        important_total++;
        if (task.success) important_success++;
        break;
    }
  }

  std::cout << "📊 Console update summary:" << std::endl;
  std::cout << "  🌟 Essential: " << essential_success << "/" << essential_total
            << " (Sun, major planets)" << std::endl;
  std::cout << "  🌙 Important: " << important_success << "/" << important_total
            << " (moons, dwarf planets)" << std::endl;
  std::cout << "  🚀 Optional: " << optional_success << "/" << optional_total << " (spacecraft)"
            << std::endl;

  // Console success criteria: ALL essential + MOST important bodies must succeed
  bool essential_ok = (essential_success == essential_total) && (essential_total > 0);
  bool important_mostly_ok =
      (important_total == 0) ||
      (important_success >= (important_total * 0.8));  // 80% of important bodies

  if (!essential_ok) {
    std::cerr << "💥 CRITICAL: Failed to fetch essential bodies (Sun/planets)" << std::endl;
    std::cerr << "Cannot proceed with incomplete planetary data" << std::endl;
    return false;
  }

  if (!important_mostly_ok) {
    std::cerr << "💥 CRITICAL: Too many important bodies failed (need 80% success rate)"
              << std::endl;
    std::cerr << "Insufficient data for precise simulation" << std::endl;
    return false;
  }

  // Warn about optional body failures but continue
  if (optional_success < optional_total) {
    std::cout << "⚠️ WARNING: Some spacecraft failed to fetch (may slightly affect precision)"
              << std::endl;
  }

  // Save to cache if we have sufficient data
  if (save_ephemeris_to_json() && save_ephemeris_to_binary()) {
    std::cout << "✅ Console update successful - data saved to cache files" << std::endl;
    return true;
  } else {
    std::cerr << "❌ Failed to save ephemeris data to cache" << std::endl;
    return false;
  }
}

bool force_update_ephemeris_data() {
  std::cout << "Force updating ephemeris data (bypassing smart caching)..." << std::endl;

  // Get current year
  time_t now = time(NULL);
  const struct tm* tm_now = localtime(&now);
  int current_year = tm_now->tm_year + 1900;

  std::cout << "Updating ephemeris data from NASA JPL for " << current_year << "..." << std::endl;

  // Format date string for JPL query
  char date_str[32];
  snprintf(date_str, sizeof(date_str), "%d-01-01", current_year);

  std::cout << "Fetching data for: " << date_str << std::endl;

  // Collect all bodies to fetch
  std::vector<FetchTask> tasks;
  for (int i = 0; i < BODY_COUNT; i++) {
    int jpl_id = get_jpl_id_for_body(i);
    if (jpl_id != 0) {
      FetchTask task;
      task.body_index = i;
      task.jpl_id = jpl_id;
      task.date_str = std::string(date_str);
      task.success = false;
      tasks.push_back(task);
    }
  }

  std::cout << "Fetching data for " << tasks.size() << " celestial bodies..." << std::endl;

  // Reset counters
  completed_fetches = 0;
  successful_fetches = 0;

  // Determine optimal number of threads (2-3 concurrent requests to minimize rate limiting)
  const int max_threads = std::min(3, std::max(2, (int)std::thread::hardware_concurrency()));
  const int num_threads = std::min(max_threads, (int)tasks.size());

  std::cout << "Using " << num_threads << " parallel connections..." << std::endl;

  // Launch parallel fetch operations
  std::vector<std::future<void>> futures;

  for (int t = 0; t < num_threads; t++) {
    futures.push_back(std::async(std::launch::async, [&tasks, t, num_threads]() {
      // Each thread processes every nth task
      for (size_t i = t; i < tasks.size(); i += num_threads) {
        fetch_body_data_parallel(tasks[i]);
      }
    }));
  }

  // Wait for all threads to complete
  for (auto& future : futures) {
    future.wait();
  }

  std::cout << "Parallel phase: " << successful_fetches.load() << "/" << tasks.size()
            << " bodies fetched" << std::endl;

  // Sequential retry for failed bodies
  if (successful_fetches.load() < (int)tasks.size()) {
    std::cout << "Retrying failed bodies sequentially..." << std::endl;

    for (auto& task : tasks) {
      if (!task.success) {
        std::cout << "Sequential retry for " << SolarSystem[task.body_index].name << "..."
                  << std::endl;

        // Try up to MAX_RETRIES times with longer delays
        bool success = false;
        for (int attempt = 1; attempt <= MAX_RETRIES && !success; attempt++) {
          if (attempt > 1) {
            std::cout << "  Sequential attempt " << attempt << "/" << MAX_RETRIES << " for "
                      << SolarSystem[task.body_index].name << "..." << std::endl;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(RETRY_DELAY_MS * 2));  // Longer delay
          }

          success = fetch_jpl_horizons_data(task.date_str.c_str(), task.jpl_id, task.body_index);
        }

        if (success) {
          successful_fetches++;
          task.success = true;
          std::cout << "✓ Sequential retry successful for " << SolarSystem[task.body_index].name
                    << std::endl;
        } else {
          std::cerr << "✗ Sequential retry failed for " << SolarSystem[task.body_index].name
                    << std::endl;
        }
      }
    }
  }

  std::cout << "Successfully fetched " << successful_fetches.load() << "/" << tasks.size()
            << " bodies" << std::endl;

  // Apply proper success criteria based on body classification
  int essential_success = 0;
  int essential_total = 0;
  int important_success = 0;
  int important_total = 0;
  int optional_success = 0;
  int optional_total = 0;

  for (const auto& task : tasks) {
    BodyType body_type = get_body_type(task.body_index);

    switch (body_type) {
      case BODY_ESSENTIAL:
        essential_total++;
        if (task.success) essential_success++;
        break;
      case BODY_IMPORTANT:
        important_total++;
        if (task.success) important_success++;
        break;
      case BODY_OPTIONAL:
        optional_total++;
        if (task.success) optional_success++;
        break;
      default:
        important_total++;
        if (task.success) important_success++;
        break;
    }
  }

  std::cout << "📊 Force update summary:" << std::endl;
  std::cout << "  🌟 Essential: " << essential_success << "/" << essential_total
            << " (Sun, major planets)" << std::endl;
  std::cout << "  🌙 Important: " << important_success << "/" << important_total
            << " (moons, dwarf planets)" << std::endl;
  std::cout << "  🚀 Optional: " << optional_success << "/" << optional_total << " (spacecraft)"
            << std::endl;

  // Console success criteria: ALL essential + MOST important bodies must succeed
  bool essential_ok = (essential_success == essential_total) && (essential_total > 0);
  bool important_mostly_ok =
      (important_total == 0) ||
      (important_success >= (important_total * 0.8));  // 80% of important bodies

  if (!essential_ok) {
    std::cerr << "💥 CRITICAL: Failed to fetch essential bodies (Sun/planets)" << std::endl;
    std::cerr << "Cannot proceed with incomplete planetary data" << std::endl;
    return false;
  }

  if (!important_mostly_ok) {
    std::cerr << "💥 CRITICAL: Too many important bodies failed (need 80% success rate)"
              << std::endl;
    std::cerr << "Insufficient data for precise simulation" << std::endl;
    return false;
  }

  // Warn about optional body failures but continue
  if (optional_success < optional_total) {
    std::cout << "⚠️ WARNING: Some spacecraft failed to fetch (may slightly affect precision)"
              << std::endl;
  }

  // Save to cache if we have sufficient data
  if (save_ephemeris_to_json() && save_ephemeris_to_binary()) {
    std::cout << "✅ Force update successful - data saved to cache files" << std::endl;
    return true;
  } else {
    std::cerr << "❌ Failed to save ephemeris data to cache" << std::endl;
    return false;
  }
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

// HTTP response callback for curl (legacy - currently unused)
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
  size_t total_size = size * nmemb;
  response->append(static_cast<char*>(contents), total_size);
  return total_size;
}

// Thread-safe parallel fetch function
void fetch_body_data_parallel(FetchTask& task) {
  // Small delay to be respectful to JPL servers
  std::this_thread::sleep_for(std::chrono::milliseconds(REQUEST_DELAY_MS));

  // Thread-safe progress reporting
  {
    std::lock_guard<std::mutex> lock(fetch_mutex);
    std::cout << "Fetching " << SolarSystem[task.body_index].name << " (JPL ID: " << task.jpl_id
              << ")..." << std::endl;
  }

  // Retry logic
  bool success = false;
  int attempt = 0;

  while (!success && attempt < MAX_RETRIES) {
    attempt++;

    if (attempt > 1) {
      // Thread-safe retry notification
      {
        std::lock_guard<std::mutex> lock(fetch_mutex);
        std::cout << "  Retry " << (attempt - 1) << "/" << (MAX_RETRIES - 1) << " for "
                  << SolarSystem[task.body_index].name << "..." << std::endl;
      }

      // Wait before retry
      std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
    }

    // Perform the actual fetch
    success = fetch_jpl_horizons_data(task.date_str.c_str(), task.jpl_id, task.body_index);
  }

  // Update task result
  task.success = success;

  // Thread-safe result reporting and counter updates
  {
    std::lock_guard<std::mutex> lock(fetch_mutex);
    completed_fetches++;

    if (success) {
      successful_fetches++;
      if (attempt > 1) {
        std::cout << "✓ Successfully fetched " << SolarSystem[task.body_index].name << " after "
                  << attempt << " attempts (" << completed_fetches.load() << "/" << BODY_COUNT
                  << ")" << std::endl;
      } else {
        std::cout << "✓ Successfully fetched " << SolarSystem[task.body_index].name << " ("
                  << completed_fetches.load() << "/" << BODY_COUNT << ")" << std::endl;
      }
    } else {
      std::cerr << "✗ Failed to fetch " << SolarSystem[task.body_index].name << " after "
                << MAX_RETRIES << " attempts (" << completed_fetches.load() << "/" << BODY_COUNT
                << ")" << std::endl;
    }
  }
}

// Fetch JPL HORIZONS data for a specific body
bool fetch_jpl_horizons_data(const char* date, int jpl_id, int body_index) {
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

  // Use curl to fetch data with better error handling
  std::string curl_command = "curl -s --max-time 30 --retry 2 --retry-delay 1 \"" + url + "\"";

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
  if (parse_jpl_response(response.c_str(), body_index)) {
    return true;
  }

  std::cerr << "Failed to parse JPL response for body " << jpl_id << std::endl;
  return false;
}

bool parse_jpl_response(const char* response, int body_index) {
  std::string response_str(response);

  // Find data section markers
  size_t data_start = response_str.find("$$SOE");
  size_t data_end = response_str.find("$$EOE");

  if (data_start == std::string::npos || data_end == std::string::npos) {
    std::cerr << "Could not find data markers in JPL response for body " << body_index << std::endl;
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

        // Thread-safe update of the correct body
        {
          std::lock_guard<std::mutex> lock(fetch_mutex);
          if (body_index >= 0 && body_index < BODY_COUNT) {
            // Convert JPL data from kilometers to meters (SI units)
            SolarSystem[body_index].position.x = x * 1000.0;
            SolarSystem[body_index].position.y = y * 1000.0;
            SolarSystem[body_index].position.z = z * 1000.0;
            // Convert JPL velocity from km/s to m/s
            SolarSystem[body_index].speed.x = vx * 1000.0;
            SolarSystem[body_index].speed.y = vy * 1000.0;
            SolarSystem[body_index].speed.z = vz * 1000.0;
          }
        }

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
    value_part.resize(comma_pos);
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
    value_part.resize(comma_pos);
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
        // Convert from kilometers to meters
        SolarSystem[current_body_index].position.x = extract_json_number(line) * 1000.0;
      } else if (line.find("\"y\":") != std::string::npos) {
        // Convert from kilometers to meters
        SolarSystem[current_body_index].position.y = extract_json_number(line) * 1000.0;
      } else if (line.find("\"z\":") != std::string::npos) {
        // Convert from kilometers to meters
        SolarSystem[current_body_index].position.z = extract_json_number(line) * 1000.0;
      }
    } else if (in_velocity && current_body_index >= 0 && current_body_index < BODY_COUNT) {
      if (line.find("\"x\":") != std::string::npos) {
        // Convert from km/s to m/s
        SolarSystem[current_body_index].speed.x = extract_json_number(line) * 1000.0;
      } else if (line.find("\"y\":") != std::string::npos) {
        // Convert from km/s to m/s
        SolarSystem[current_body_index].speed.y = extract_json_number(line) * 1000.0;
      } else if (line.find("\"z\":") != std::string::npos) {
        // Convert from km/s to m/s
        SolarSystem[current_body_index].speed.z = extract_json_number(line) * 1000.0;
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

// Get body type classification for error handling
BodyType get_body_type(int body_index) {
  if (body_index < 0 || body_index >= JPL_BODY_COUNT) {
    return BODY_UNKNOWN;
  }

  return JPL_BODY_MAP[body_index].body_type;
}

// Fetch JPL data for all bodies for a specific date (for web server)
bool fetch_jpl_data_for_date(const char* date) {
  std::cout << "🌐 Fetching JPL data for all bodies on date: " << date << std::endl;

  int essential_success = 0;
  int essential_total = 0;
  int important_success = 0;
  int important_total = 0;
  int optional_success = 0;
  int optional_total = 0;
  int total_failed = 0;

  // Fetch data for each body
  for (int i = 0; i < BODY_COUNT; i++) {
    int jpl_id = get_jpl_id_for_body(i);
    if (jpl_id != 0) {
      BodyType body_type = get_body_type(i);
      const char* type_str = (body_type == BODY_ESSENTIAL)   ? "ESSENTIAL"
                             : (body_type == BODY_IMPORTANT) ? "IMPORTANT"
                             : (body_type == BODY_OPTIONAL)  ? "OPTIONAL"
                                                             : "UNKNOWN";

      std::cout << "📡 Fetching " << SolarSystem[i].name << " (ID: " << jpl_id
                << ", Type: " << type_str << ")..." << std::endl;

      bool success = fetch_jpl_horizons_data(date, jpl_id, i);

      // Count by category
      switch (body_type) {
        case BODY_ESSENTIAL:
          essential_total++;
          if (success) essential_success++;
          break;
        case BODY_IMPORTANT:
          important_total++;
          if (success) important_success++;
          break;
        case BODY_OPTIONAL:
          optional_total++;
          if (success) optional_success++;
          break;
        default:
          important_total++;
          if (success) important_success++;
          break;
      }

      if (success) {
        std::cout << "✅ " << SolarSystem[i].name << " data fetched successfully" << std::endl;
      } else {
        total_failed++;
        if (body_type == BODY_OPTIONAL) {
          std::cout << "⚠️ " << SolarSystem[i].name << " data failed (expected for historical dates)"
                    << std::endl;
        } else {
          std::cout << "❌ Failed to fetch " << SolarSystem[i].name << " data" << std::endl;
        }
      }
    } else {
      std::cout << "⚠️ Skipping " << SolarSystem[i].name << " (no JPL ID)" << std::endl;
    }
  }

  std::cout << "📊 JPL fetch summary:" << std::endl;
  std::cout << "  🌟 Essential: " << essential_success << "/" << essential_total
            << " (Sun, major planets)" << std::endl;
  std::cout << "  🌙 Important: " << important_success << "/" << important_total
            << " (moons, dwarf planets)" << std::endl;
  std::cout << "  🚀 Optional: " << optional_success << "/" << optional_total << " (spacecraft)"
            << std::endl;
  std::cout << "  ❌ Total failed: " << total_failed << std::endl;

  // Success criteria: ALL essential bodies must succeed
  bool essential_ok = (essential_success == essential_total) && (essential_total > 0);

  if (!essential_ok) {
    std::cout << "💥 CRITICAL: Failed to fetch essential bodies (Sun/planets)" << std::endl;
    return false;
  }

  // Warn about important body failures but don't fail completely
  if (important_success < important_total) {
    std::cout << "⚠️ WARNING: Some moons/dwarf planets failed to fetch" << std::endl;
  }

  std::cout << "✅ JPL fetch successful - all essential bodies retrieved" << std::endl;
  return true;
}

// Fetch JPL data for console applications (stricter requirements)
bool fetch_jpl_data_for_console(const char* date) {
  std::cout << "🖥️ Fetching JPL data for console simulation on date: " << date << std::endl;

  int essential_success = 0;
  int essential_total = 0;
  int important_success = 0;
  int important_total = 0;
  int optional_success = 0;
  int optional_total = 0;
  int total_failed = 0;

  // Fetch data for each body
  for (int i = 0; i < BODY_COUNT; i++) {
    int jpl_id = get_jpl_id_for_body(i);
    if (jpl_id != 0) {
      BodyType body_type = get_body_type(i);
      const char* type_str = (body_type == BODY_ESSENTIAL)   ? "ESSENTIAL"
                             : (body_type == BODY_IMPORTANT) ? "IMPORTANT"
                             : (body_type == BODY_OPTIONAL)  ? "OPTIONAL"
                                                             : "UNKNOWN";

      std::cout << "📡 Fetching " << SolarSystem[i].name << " (ID: " << jpl_id
                << ", Type: " << type_str << ")..." << std::endl;

      bool success = fetch_jpl_horizons_data(date, jpl_id, i);

      // Count by category
      switch (body_type) {
        case BODY_ESSENTIAL:
          essential_total++;
          if (success) essential_success++;
          break;
        case BODY_IMPORTANT:
          important_total++;
          if (success) important_success++;
          break;
        case BODY_OPTIONAL:
          optional_total++;
          if (success) optional_success++;
          break;
        default:
          important_total++;
          if (success) important_success++;
          break;
      }

      if (success) {
        std::cout << "✅ " << SolarSystem[i].name << " data fetched successfully" << std::endl;
      } else {
        total_failed++;
        std::cout << "❌ Failed to fetch " << SolarSystem[i].name << " data" << std::endl;
      }
    } else {
      std::cout << "⚠️ Skipping " << SolarSystem[i].name << " (no JPL ID)" << std::endl;
    }
  }

  std::cout << "📊 Console JPL fetch summary:" << std::endl;
  std::cout << "  🌟 Essential: " << essential_success << "/" << essential_total
            << " (Sun, major planets)" << std::endl;
  std::cout << "  🌙 Important: " << important_success << "/" << important_total
            << " (moons, dwarf planets)" << std::endl;
  std::cout << "  🚀 Optional: " << optional_success << "/" << optional_total << " (spacecraft)"
            << std::endl;
  std::cout << "  ❌ Total failed: " << total_failed << std::endl;

  // Console success criteria: ALL essential bodies + MOST important bodies must succeed
  bool essential_ok = (essential_success == essential_total) && (essential_total > 0);
  bool important_mostly_ok =
      (important_total == 0) ||
      (important_success >= (important_total * 0.8));  // 80% of important bodies

  if (!essential_ok) {
    std::cout << "💥 CRITICAL: Failed to fetch essential bodies (Sun/planets)" << std::endl;
    return false;
  }

  if (!important_mostly_ok) {
    std::cout << "💥 CRITICAL: Too many important bodies failed (need 80% success rate)"
              << std::endl;
    return false;
  }

  // Warn about optional body failures but don't fail completely
  if (optional_success < optional_total) {
    std::cout << "⚠️ WARNING: Some spacecraft failed to fetch (may affect precision)" << std::endl;
  }

  std::cout << "✅ Console JPL fetch successful - sufficient bodies for precise simulation"
            << std::endl;
  return true;
}

bool has_current_year_ephemeris_data() {
  if (!has_current_ephemeris_data()) {
    return false;
  }

  time_t now = time(NULL);
  const struct tm* tm_now = localtime(&now);
  const struct tm* tm_epoch = localtime(&current_epoch);

  return tm_now->tm_year == tm_epoch->tm_year;
}
