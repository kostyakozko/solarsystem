#ifndef __JPL_DATA_H__
#define __JPL_DATA_H__

#include <cstdint>
#include <ctime>
#include <string>

#include "jpl_bodies.h"  // For BodyType enum

// Forward declarations for parallel fetching
struct FetchTask {
  int body_index;
  int jpl_id;
  std::string date_str;
  bool success;
  std::string error_message;
};

// Constants
#define JPL_BINARY_MAGIC 0x4A504C45  // "JPLE"
#define JPL_FORMAT_VERSION 1
#define BODY_COUNT 27

// File paths
#define JPL_JSON_FILE "ephemeris_data.json"
#define JPL_BINARY_FILE "ephemeris_cache.bin"

// Binary cache file header
struct EphemerisHeader {
  uint32_t magic_number;   // 0x4A504C45 ("JPLE")
  uint32_t version;        // Format version
  uint32_t body_count;     // Number of bodies
  time_t epoch;            // Data epoch (Jan 1st of year)
  char source[32];         // "JPL_HORIZONS_2025" etc.
  uint64_t json_checksum;  // To detect JSON changes
};

// JPL data management functions

// Main update function - fetches data for January 1st of current year
bool update_ephemeris_data();

// Force update function - bypasses smart caching
bool force_update_ephemeris_data();

// Load data from cache (binary first, then JSON fallback)
bool load_cached_ephemeris_data();

// Rebuild binary cache from JSON
bool rebuild_binary_cache();

// Check if we have current year's data
bool has_current_year_ephemeris_data();

// Check if we have any JPL ephemeris data (vs original hardcoded data)
bool has_current_ephemeris_data();

// Fetch JPL data for all bodies for a specific date (for web server)
bool fetch_jpl_data_for_date(const char* date);

// Fetch JPL data for console applications (stricter requirements)
bool fetch_jpl_data_for_console(const char* date);

// Get body type classification for error handling
BodyType get_body_type(int body_index);

// Get the epoch of currently loaded data
time_t get_ephemeris_epoch();

// Get data source description
const char* get_ephemeris_source();

// Initialize JPL data system
bool initialize_jpl_data();

// Update SolarSystem array from loaded ephemeris data
bool apply_ephemeris_to_solar_system();

// Test function to save current data (for testing storage system)
bool save_current_data_for_testing();
bool test_storage_system();

// Internal functions
bool fetch_jpl_horizons_data(const char* date, int jpl_id, int body_index);
void fetch_body_data_parallel(FetchTask& task);
bool parse_jpl_response(const char* response, int body_index);
bool save_ephemeris_to_json();
bool load_ephemeris_from_json();
bool save_ephemeris_to_binary();
bool load_ephemeris_from_binary();
bool validate_ephemeris_data();
bool apply_ephemeris_data();
uint64_t calculate_json_checksum();

#endif  //__JPL_DATA_H__
