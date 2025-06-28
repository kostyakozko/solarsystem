#include "jpl_data.h"
#include "jpl_bodies.h"
#include "constants.h"
#include "types.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>

// Global state for ephemeris data
static time_t current_epoch = 0;
static char current_source[32] = "ORIGINAL_DATA";
static bool data_initialized = false;

bool initialize_jpl_data() {
    if (data_initialized) {
        return true;
    }
    
    // Validate that we have JPL mappings for all bodies
    if (!validate_all_jpl_bodies()) {
        std::cerr << "Warning: JPL body mapping validation failed" << std::endl;
        return false;
    }
    
    // Try to load cached data
    if (load_cached_ephemeris_data()) {
        std::cout << "Loaded ephemeris data from cache (epoch: " << ctime(&current_epoch) << ")" << std::endl;
    } else {
        std::cout << "Using original hardcoded ephemeris data" << std::endl;
        // Set epoch to the original data date (Feb 11, 2018)
        struct tm original_date = {0, 0, 0, 11, 1, 2018 - 1900};
        current_epoch = mktime(&original_date);
        strcpy(current_source, "ORIGINAL_DATA");
    }
    
    data_initialized = true;
    return true;
}

bool update_ephemeris_data() {
    std::cout << "Updating ephemeris data from NASA JPL..." << std::endl;
    
    // Get current year for January 1st
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int current_year = tm_now->tm_year + 1900;
    
    // Format date as YYYY-01-01
    char date_str[16];
    snprintf(date_str, sizeof(date_str), "%d-01-01", current_year);
    
    std::cout << "Fetching data for: " << date_str << std::endl;
    
    // TODO: Implement HTTP client to fetch JPL data
    if (fetch_jpl_horizons_data(date_str)) {
        std::cout << "Successfully fetched JPL data" << std::endl;
        
        // Save to both JSON and binary
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
    
    // Fallback to JSON
    if (load_ephemeris_from_json()) {
        // Rebuild binary cache for next time
        save_ephemeris_to_binary();
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

bool has_current_ephemeris_data() {
    if (!data_initialized) {
        return false;
    }
    
    // Check if we have any JPL data (not original hardcoded data)
    return (strcmp(current_source, "ORIGINAL_DATA") != 0);
}

bool has_current_year_ephemeris_data() {
    if (!data_initialized || !has_current_ephemeris_data()) {
        return false;
    }
    
    // Check if we have data for current year
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    struct tm* tm_epoch = localtime(&current_epoch);
    
    return (tm_now->tm_year == tm_epoch->tm_year);
}

time_t get_ephemeris_epoch() {
    return current_epoch;
}

const char* get_ephemeris_source() {
    return current_source;
}

// Placeholder implementations - to be completed
bool fetch_jpl_horizons_data(const char* date) {
    std::cout << "TODO: Implement HTTP client for JPL HORIZONS API" << std::endl;
    std::cout << "Would fetch data for date: " << date << std::endl;
    
    // For now, return false to indicate not implemented
    return false;
}

bool parse_jpl_response(const char* response) {
    std::cout << "TODO: Parse JPL response and extract ephemeris data" << std::endl;
    return false;
}

bool save_ephemeris_to_json() {
    std::ofstream file(JPL_JSON_FILE);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << JPL_JSON_FILE << " for writing" << std::endl;
        return false;
    }
    
    // Write JSON header
    file << "{\n";
    file << "  \"format_version\": 1,\n";
    file << "  \"epoch\": " << current_epoch << ",\n";
    file << "  \"epoch_iso\": \"" << std::put_time(std::gmtime(&current_epoch), "%Y-%m-%dT%H:%M:%SZ") << "\",\n";
    file << "  \"source\": \"" << current_source << "\",\n";
    file << "  \"body_count\": " << count << ",\n";
    file << "  \"bodies\": [\n";
    
    // Write each body
    for (int i = 0; i < count; i++) {
        const planet& body = SolarSystem[i];
        const JPLBodyInfo& jpl_info = JPL_BODY_MAP[i];
        
        file << "    {\n";
        file << "      \"index\": " << i << ",\n";
        file << "      \"name\": \"" << body.name << "\",\n";
        file << "      \"jpl_id\": " << jpl_info.horizons_id << ",\n";
        file << "      \"jpl_name\": \"" << jpl_info.horizons_name << "\",\n";
        file << "      \"mass\": " << std::scientific << std::setprecision(15) << body.mass << ",\n";
        file << "      \"position\": {\n";
        file << "        \"x\": " << std::scientific << std::setprecision(15) << body.position.x << ",\n";
        file << "        \"y\": " << std::scientific << std::setprecision(15) << body.position.y << ",\n";
        file << "        \"z\": " << std::scientific << std::setprecision(15) << body.position.z << "\n";
        file << "      },\n";
        file << "      \"velocity\": {\n";
        file << "        \"x\": " << std::scientific << std::setprecision(15) << body.speed.x << ",\n";
        file << "        \"y\": " << std::scientific << std::setprecision(15) << body.speed.y << ",\n";
        file << "        \"z\": " << std::scientific << std::setprecision(15) << body.speed.z << "\n";
        file << "      }\n";
        file << "    }";
        if (i < count - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "Saved ephemeris data to " << JPL_JSON_FILE << std::endl;
    return true;
}

// Helper functions for JSON parsing
bool parse_epoch_field(const std::string& line) {
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
        std::string value = line.substr(pos + 1);
        size_t comma = value.find(',');
        if (comma != std::string::npos) value = value.substr(0, comma);
        current_epoch = std::stoll(value);
        return true;
    }
    return false;
}

bool parse_source_field(const std::string& line) {
    size_t start_quote = line.find('"', line.find(':'));
    size_t end_quote = line.find('"', start_quote + 1);
    if (start_quote != std::string::npos && end_quote != std::string::npos) {
        std::string source = line.substr(start_quote + 1, end_quote - start_quote - 1);
        strncpy(current_source, source.c_str(), sizeof(current_source) - 1);
        current_source[sizeof(current_source) - 1] = '\0';
        return true;
    }
    return false;
}

bool parse_index_field(const std::string& line, int& body_index) {
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
        std::string value = line.substr(pos + 1);
        size_t comma = value.find(',');
        if (comma != std::string::npos) value = value.substr(0, comma);
        body_index = std::stoi(value);
        return true;
    }
    return false;
}

bool parse_numeric_field(const std::string& line, long double& target) {
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
        std::string value = line.substr(pos + 1);
        size_t comma = value.find(',');
        if (comma != std::string::npos) value = value.substr(0, comma);
        target = std::stold(value);
        return true;
    }
    return false;
}

std::string trim_line(const std::string& line) {
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    return line.substr(start);
}

bool load_ephemeris_from_json() {
    std::ifstream file(JPL_JSON_FILE);
    if (!file.is_open()) {
        return false;  // File doesn't exist, not an error
    }
    
    std::cout << "Loading ephemeris data from " << JPL_JSON_FILE << std::endl;
    
    // Simple JSON parsing state
    std::string line;
    int body_index = -1;
    bool in_bodies_array = false;
    bool in_body_object = false;
    bool in_position = false;
    bool in_velocity = false;
    
    while (std::getline(file, line)) {
        line = trim_line(line);
        if (line.empty()) continue;
        
        // Parse root-level fields
        if (line.find("\"epoch\":") != std::string::npos) {
            parse_epoch_field(line);
        }
        else if (line.find("\"source\":") != std::string::npos) {
            parse_source_field(line);
        }
        else if (line.find("\"bodies\":") != std::string::npos) {
            in_bodies_array = true;
        }
        // Parse body array elements
        else if (in_bodies_array && line.find("\"index\":") != std::string::npos) {
            if (parse_index_field(line, body_index)) {
                in_body_object = true;
            }
        }
        // Parse body object fields
        else if (in_body_object && body_index >= 0 && body_index < count) {
            if (line.find("\"mass\":") != std::string::npos) {
                parse_numeric_field(line, SolarSystem[body_index].mass);
            }
            else if (line.find("\"position\":") != std::string::npos) {
                in_position = true;
                in_velocity = false;
            }
            else if (line.find("\"velocity\":") != std::string::npos) {
                in_velocity = true;
                in_position = false;
            }
            else if (in_position && line.find("\"x\":") != std::string::npos) {
                parse_numeric_field(line, SolarSystem[body_index].position.x);
            }
            else if (in_position && line.find("\"y\":") != std::string::npos) {
                parse_numeric_field(line, SolarSystem[body_index].position.y);
            }
            else if (in_position && line.find("\"z\":") != std::string::npos) {
                parse_numeric_field(line, SolarSystem[body_index].position.z);
            }
            else if (in_velocity && line.find("\"x\":") != std::string::npos) {
                parse_numeric_field(line, SolarSystem[body_index].speed.x);
            }
            else if (in_velocity && line.find("\"y\":") != std::string::npos) {
                parse_numeric_field(line, SolarSystem[body_index].speed.y);
            }
            else if (in_velocity && line.find("\"z\":") != std::string::npos) {
                parse_numeric_field(line, SolarSystem[body_index].speed.z);
            }
            else if (line.find("}") != std::string::npos && in_body_object) {
                // End of body object
                in_body_object = false;
                in_position = false;
                in_velocity = false;
                body_index = -1;
            }
        }
    }
    
    file.close();
    std::cout << "Loaded ephemeris data from JSON (epoch: " << current_epoch 
              << ", source: " << current_source << ")" << std::endl;
    return true;
}

bool save_ephemeris_to_binary() {
    std::ofstream file(JPL_BINARY_FILE, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << JPL_BINARY_FILE << " for writing" << std::endl;
        return false;
    }
    
    // Write header
    EphemerisHeader header;
    header.magic_number = 0x4A504C45;  // "JPLE"
    header.version = 1;
    header.body_count = count;
    header.epoch = current_epoch;
    strncpy(header.source, current_source, sizeof(header.source) - 1);
    header.source[sizeof(header.source) - 1] = '\0';
    header.json_checksum = calculate_json_checksum();
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write body data
    file.write(reinterpret_cast<const char*>(SolarSystem), sizeof(planet) * count);
    
    file.close();
    std::cout << "Saved ephemeris data to binary cache " << JPL_BINARY_FILE << std::endl;
    return true;
}

bool load_ephemeris_from_binary() {
    std::ifstream file(JPL_BINARY_FILE, std::ios::binary);
    if (!file.is_open()) {
        return false;  // File doesn't exist, not an error
    }
    
    // Read and validate header
    EphemerisHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (file.gcount() != sizeof(header)) {
        std::cerr << "Invalid binary cache file (header)" << std::endl;
        file.close();
        return false;
    }
    
    // Validate magic number and version
    if (header.magic_number != 0x4A504C45 || header.version != 1) {
        std::cerr << "Invalid binary cache file (magic/version)" << std::endl;
        file.close();
        return false;
    }
    
    // Validate body count
    if (header.body_count != count) {
        std::cerr << "Binary cache body count mismatch: " << header.body_count 
                  << " vs " << count << std::endl;
        file.close();
        return false;
    }
    
    // Check if JSON file has been modified (if it exists)
    uint64_t current_json_checksum = calculate_json_checksum();
    if (current_json_checksum != 0 && current_json_checksum != header.json_checksum) {
        std::cout << "JSON file has been modified, binary cache is stale" << std::endl;
        file.close();
        return false;
    }
    
    // Read body data
    file.read(reinterpret_cast<char*>(SolarSystem), sizeof(planet) * count);
    
    if (file.gcount() != sizeof(planet) * count) {
        std::cerr << "Invalid binary cache file (body data)" << std::endl;
        file.close();
        return false;
    }
    
    // Update global state
    current_epoch = header.epoch;
    strncpy(current_source, header.source, sizeof(current_source) - 1);
    current_source[sizeof(current_source) - 1] = '\0';
    
    file.close();
    std::cout << "Loaded ephemeris data from binary cache (epoch: " << current_epoch 
              << ", source: " << current_source << ")" << std::endl;
    return true;
}

bool validate_ephemeris_data() {
    std::cout << "TODO: Validate ephemeris data integrity" << std::endl;
    return true;
}

uint64_t calculate_json_checksum() {
    std::ifstream file(JPL_JSON_FILE);
    if (!file.is_open()) {
        return 0;  // No JSON file
    }
    
    // Simple checksum: sum of all bytes with some mixing
    uint64_t checksum = 0;
    char buffer[4096];
    
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        for (std::streamsize i = 0; i < file.gcount(); i++) {
            checksum = checksum * 31 + static_cast<unsigned char>(buffer[i]);
        }
    }
    
    file.close();
    return checksum;
}

bool apply_ephemeris_to_solar_system() {
    // This function would update SolarSystem array with loaded ephemeris data
    // For now, the data is already loaded directly into SolarSystem by the load functions
    std::cout << "Applied ephemeris data to solar system" << std::endl;
    return true;
}

// Test function to save current data (for testing storage system)
bool save_current_data_for_testing() {
    std::cout << "Saving current solar system data for testing..." << std::endl;
    
    // Set test metadata
    time_t now = time(NULL);
    current_epoch = now;
    strcpy(current_source, "TEST_DATA");
    
    // Save to both JSON and binary
    if (save_ephemeris_to_json() && save_ephemeris_to_binary()) {
        std::cout << "Test data saved successfully" << std::endl;
        return true;
    }
    
    std::cerr << "Failed to save test data" << std::endl;
    return false;
}
