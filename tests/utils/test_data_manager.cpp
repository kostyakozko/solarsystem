/**
 * @file test_data_manager.cpp
 * @brief Implementation of test data management utilities
 */

#include "test_data_manager.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace TestData {

// Static member definitions
std::vector<std::unique_ptr<TemporaryDirectory>> TestDataManager::temp_directories_;
std::vector<std::unique_ptr<TemporaryCache>> TestDataManager::temp_caches_;
bool TestDataManager::cleanup_registered_ = false;

// TemporaryDirectory implementation
TemporaryDirectory::TemporaryDirectory(const std::string& prefix) {
  // Create unique temporary directory
  auto temp_dir = std::filesystem::temp_directory_path();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1000, 9999);

  temp_path_ = temp_dir / (prefix + std::to_string(timestamp) + "_" + std::to_string(dis(gen)));

  std::filesystem::create_directories(temp_path_);
}

TemporaryDirectory::~TemporaryDirectory() {
  if (cleanup_on_destroy_ && std::filesystem::exists(temp_path_)) {
    std::error_code ec;
    std::filesystem::remove_all(temp_path_, ec);
    // Ignore errors during cleanup
  }
}

void TemporaryDirectory::create_file(const std::string& name, const std::string& content) {
  auto file_path = temp_path_ / name;
  std::ofstream file(file_path);
  if (file.is_open()) {
    file << content;
    file.close();
  }
}

void TemporaryDirectory::create_subdirectory(const std::string& name) {
  auto dir_path = temp_path_ / name;
  std::filesystem::create_directories(dir_path);
}

void TemporaryDirectory::copy_file(const std::filesystem::path& source,
                                   const std::string& dest_name) {
  auto dest_path = temp_path_ / dest_name;
  std::error_code ec;
  std::filesystem::copy_file(source, dest_path, ec);
}

bool TemporaryDirectory::exists() const { return std::filesystem::exists(temp_path_); }

bool TemporaryDirectory::is_empty() const { return std::filesystem::is_empty(temp_path_); }

size_t TemporaryDirectory::file_count() const {
  size_t count = 0;
  std::error_code ec;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(temp_path_, ec)) {
    if (entry.is_regular_file()) {
      ++count;
    }
  }
  return count;
}

// TemporaryCache implementation
TemporaryCache::TemporaryCache(const std::string& cache_type) : cache_type_(cache_type) {
  temp_dir_ = std::make_unique<TemporaryDirectory>("solar_cache_");
  cache_file_ = std::string("ephemeris_cache.") + (cache_type == "binary" ? "bin" : "json");
}

void TemporaryCache::populate_with_valid_data() {
  if (cache_type_ == "json") {
    std::string valid_cache_content = R"({
  "format_version": 1,
  "cache_type": "ephemeris",
  "created_timestamp": 1722348000,
  "body_count": 2,
  "checksum": "sha25test_checksum",
  "bodies": [
    {
      "index": 0,
      "name": "Sun",
      "id": 10,
      "mass": 1.9885e30,
      "position": {"x": 0.0, "y": 0.0, "z": 0.0},
      "velocity": {"x": 0.0, "y": 0.0, "z": 0.0}
    },
    {
      "index": 1,
      "name": "Earth",
      "id": 399,
      "mass": 5.9722e24,
      "position": {"x": 149597870.7, "y": 0.0, "z": 0.0},
      "velocity": {"x": 0.0, "y": 29.78, "z": 0.0}
    }
  ]
})";
    temp_dir_->create_file(cache_file_, valid_cache_content);
  } else if (cache_type_ == "binary") {
    // Create a simple binary cache file
    std::vector<uint8_t> binary_data;

    // Magic header "SOLR"
    binary_data.insert(binary_data.end(), {'S', 'O', 'L', 'R'});

    // Version (4 bytes, little-endian)
    uint32_t version = 1;
    binary_data.insert(binary_data.end(), reinterpret_cast<uint8_t*>(&version),
                       reinterpret_cast<uint8_t*>(&version) + sizeof(version));

    // Body count (4 bytes)
    uint32_t body_count = 2;
    binary_data.insert(binary_data.end(), reinterpret_cast<uint8_t*>(&body_count),
                       reinterpret_cast<uint8_t*>(&body_count) + sizeof(body_count));

    // Timestamp (8 bytes)
    uint64_t timestamp = 1722348000;
    binary_data.insert(binary_data.end(), reinterpret_cast<uint8_t*>(&timestamp),
                       reinterpret_cast<uint8_t*>(&timestamp) + sizeof(timestamp));

    // Write binary data to file
    auto cache_path = temp_dir_->path() / cache_file_;
    std::ofstream file(cache_path, std::ios::binary);
    if (file.is_open()) {
      file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
      file.close();
    }
  }
}

void TemporaryCache::populate_with_corrupted_data() {
  std::string corrupted_content = R"({
  "format_version": 1,
  "cache_type": "ephemeris",
  "created_timestamp": 1722348000,
  "body_count": 2,
  "checksum": "sha256:INVALID_CHECKSUM",
  "bodies": [
    {
      "index": 0,
      "name": "Sun",
      "id": 10,
      "mass": "INVALID_MASS",
      "position": {"x": "NaN", "y": "Infinity", "z": null},
      "velocity": {"x": 0.0, "y": 0.0, "z": 0.0}
    }
    // MISSING CLOSING BRACE AND SECOND BODY
)";
  temp_dir_->create_file(cache_file_, corrupted_content);
}

void TemporaryCache::simulate_partial_corruption() {
  populate_with_valid_data();

  // Corrupt part of the file by overwriting some bytes
  auto cache_path = temp_dir_->path() / cache_file_;
  std::fstream file(cache_path, std::ios::in | std::ios::out | std::ios::binary);
  if (file.is_open()) {
    file.seekp(50);  // Seek to position 50
    file.write("CORRUPTED_DATA", 14);
    file.close();
  }
}

void TemporaryCache::simulate_version_mismatch() {
  std::string version_mismatch_content = R"({
  "format_version": 999,
  "cache_type": "ephemeris",
  "created_timestamp": 1722348000,
  "body_count": 1,
  "checksum": "sha256:version_mismatch_test",
  "bodies": [
    {
      "index": 0,
      "name": "Sun",
      "id": 10,
      "mass": 1.9885e30,
      "position": {"x": 0.0, "y": 0.0, "z": 0.0},
      "velocity": {"x": 0.0, "y": 0.0, "z": 0.0}
    }
  ]
})";
  temp_dir_->create_file(cache_file_, version_mismatch_content);
}

void TemporaryCache::simulate_checksum_failure() {
  std::string checksum_fail_content = R"({
  "format_version": 1,
  "cache_type": "ephemeris",
  "created_timestamp": 1722348000,
  "body_count": 1,
  "checksum": "sha256:deliberately_wrong_checksum_for_testing",
  "bodies": [
    {
      "index": 0,
      "name": "Sun",
      "id": 10,
      "mass": 1.9885e30,
      "position": {"x": 0.0, "y": 0.0, "z": 0.0},
      "velocity": {"x": 0.0, "y": 0.0, "z": 0.0}
    }
  ]
})";
  temp_dir_->create_file(cache_file_, checksum_fail_content);
}

std::filesystem::path TemporaryCache::cache_path() const { return temp_dir_->path() / cache_file_; }

std::string TemporaryCache::cache_path_string() const { return cache_path().string(); }

bool TemporaryCache::cache_exists() const { return std::filesystem::exists(cache_path()); }

size_t TemporaryCache::cache_size() const {
  std::error_code ec;
  return std::filesystem::file_size(cache_path(), ec);
}

// TestDataManager implementation
std::filesystem::path TestDataManager::get_test_data_root() {
  // Try to find the test data directory relative to the executable
  std::filesystem::path current_path = std::filesystem::current_path();

  // Look for tests/data directory
  std::vector<std::filesystem::path> search_paths = {
      current_path / "tests" / "data", current_path / ".." / "tests" / "data",
      current_path / ".." / ".." / "tests" / "data",
      std::filesystem::path(__FILE__).parent_path().parent_path() / "data"};

  for (const auto& path : search_paths) {
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
      return path;
    }
  }

  // Fallback to current directory
  return current_path / "tests" / "data";
}

std::filesystem::path TestDataManager::get_jpl_responses_path() {
  return get_test_data_root() / "jpl_responses";
}

std::filesystem::path TestDataManager::get_ephemeris_data_path() {
  return get_test_data_root() / "ephemeris";
}

std::filesystem::path TestDataManager::get_cache_samples_path() {
  return get_test_data_root() / "cache";
}

std::filesystem::path TestDataManager::get_validation_data_path() {
  return get_test_data_root() / "validation";
}

std::optional<std::string> TestDataManager::read_file_content(
    const std::filesystem::path& file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::optional<std::vector<uint8_t>> TestDataManager::read_binary_file(
    const std::filesystem::path& file_path) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) {
    return std::nullopt;
  }

  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> data(size);
  file.read(reinterpret_cast<char*>(data.data()), size);

  return data;
}

std::optional<TestDataSet> TestDataManager::load_jpl_responses(const std::string& scenario) {
  auto jpl_path = get_jpl_responses_path();
  TestDataSet dataset;
  dataset.name = "jpl_responses_" + scenario;
  dataset.description = "JPL HORIZONS API response samples for " + scenario;
  dataset.version = "1.0";
  dataset.source = "JPL_HORIZONS_TEST_DATA";

  // Load metadata if available
  auto metadata_path = jpl_path / (scenario + "_metadata.json");
  if (std::filesystem::exists(metadata_path)) {
    auto metadata_content = read_file_content(metadata_path);
    if (metadata_content) {
      dataset = parse_metadata(*metadata_content);
    }
  }

  // Load response files based on scenario
  std::vector<std::string> response_files;
  if (scenario == "valid") {
    response_files = {"earth_j2000.txt", "mars_j2000.txt", "moon_j2000.txt", "sun_j2000.txt"};
  } else if (scenario == "invalid") {
    response_files = {"malformed_header.txt", "missing_ephemeris.txt", "network_error.txt"};
  } else if (scenario == "error_conditions") {
    response_files = {"invalid_body.json", "server_error.json", "timeout.json"};
  } else {
    // Try to find files matching the scenario pattern
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(jpl_path / scenario, ec)) {
      if (entry.is_regular_file()) {
        response_files.push_back(entry.path().filename().string());
      }
    }
  }

  // Load all response files
  for (const auto& filename : response_files) {
    auto file_path = jpl_path / scenario / filename;
    if (std::filesystem::exists(file_path)) {
      auto content = read_file_content(file_path);
      if (content) {
        dataset.files[filename] = *content;
      }
    }
  }

  if (dataset.files.empty()) {
    return std::nullopt;
  }

  // Calculate checksum
  std::string combined_content;
  for (const auto& [filename, content] : dataset.files) {
    combined_content += content;
  }
  dataset.checksum = calculate_checksum(combined_content);

  return dataset;
}

std::optional<TestDataSet> TestDataManager::load_ephemeris_data(const std::string& time_period) {
  auto ephemeris_path = get_ephemeris_data_path();
  TestDataSet dataset;
  dataset.name = "ephemeris_" + time_period;
  dataset.description = "Ephemeris data for " + time_period;
  dataset.version = "1.0";
  dataset.source = "JPL_DE441_EPHEMERIS";

  // Load the main ephemeris file
  auto main_file = ephemeris_path / "time_periods" / (time_period + ".json");
  if (std::filesystem::exists(main_file)) {
    auto content = read_file_content(main_file);
    if (content) {
      dataset.files[time_period + ".json"] = *content;
      dataset.checksum = calculate_checksum(*content);
      return dataset;
    }
  }

  return std::nullopt;
}

std::optional<TestDataSet> TestDataManager::load_cache_samples(const std::string& cache_type) {
  auto cache_path = get_cache_samples_path();
  TestDataSet dataset;
  dataset.name = "cache_samples_" + cache_type;
  dataset.description = "Cache file samples of type " + cache_type;
  dataset.version = "1.0";
  dataset.source = "SOLAR_SYSTEM_CACHE_SAMPLES";

  // Load cache files based on type
  std::vector<std::filesystem::path> cache_files;
  if (cache_type == "valid") {
    cache_files = {cache_path / "valid" / "ephemeris_cache_v1.json"};
  } else if (cache_type == "corrupted") {
    cache_files = {cache_path / "corrupted" / "invalid_values.json",
                   cache_path / "corrupted" / "truncated_cache.json"};
  } else if (cache_type == "formats") {
    cache_files = {cache_path / "formats" / "ephemeris_cache.bin",
                   cache_path / "formats" / "ephemeris_cache.csv"};
  } else {
    // Try to find files in the specified cache type directory
    auto type_path = cache_path / cache_type;
    if (std::filesystem::exists(type_path)) {
      std::error_code ec;
      for (const auto& entry : std::filesystem::directory_iterator(type_path, ec)) {
        if (entry.is_regular_file()) {
          cache_files.push_back(entry.path());
        }
      }
    }
  }

  // Load all cache files
  std::string combined_content;
  for (const auto& file_path : cache_files) {
    if (std::filesystem::exists(file_path)) {
      auto content = read_file_content(file_path);
      if (content) {
        dataset.files[file_path.filename().string()] = *content;
        combined_content += *content;
      }
    }
  }

  if (dataset.files.empty()) {
    return std::nullopt;
  }

  dataset.checksum = calculate_checksum(combined_content);
  return dataset;
}

std::optional<TestDataSet> TestDataManager::load_validation_data(const std::string& category) {
  auto validation_path = get_validation_data_path();
  TestDataSet dataset;
  dataset.name = "validation_" + category;
  dataset.description = "Validation data for " + category;
  dataset.version = "1.0";
  dataset.source = "REFERENCE_DATA";

  // Load validation files based on category
  std::vector<std::filesystem::path> validation_files;
  auto category_path = validation_path / category;

  if (std::filesystem::exists(category_path)) {
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(category_path, ec)) {
      if (entry.is_regular_file()) {
        validation_files.push_back(entry.path());
      }
    }
  }

  // Load all validation files
  std::string combined_content;
  for (const auto& file_path : validation_files) {
    auto content = read_file_content(file_path);
    if (content) {
      dataset.files[file_path.filename().string()] = *content;
      combined_content += *content;
    }
  }

  if (dataset.files.empty()) {
    return std::nullopt;
  }

  dataset.checksum = calculate_checksum(combined_content);
  return dataset;
}

std::string TestDataManager::load_jpl_response_file(const std::string& body_name,
                                                    const std::string& epoch) {
  auto jpl_path = get_jpl_responses_path() / "valid" / (body_name + "_" + epoch + ".txt");
  auto content = read_file_content(jpl_path);
  return content.value_or("");
}

std::string TestDataManager::load_ephemeris_json(const std::string& time_period) {
  auto ephemeris_path = get_ephemeris_data_path() / "time_periods" / (time_period + ".json");
  auto content = read_file_content(ephemeris_path);
  return content.value_or("{}");
}

std::vector<uint8_t> TestDataManager::load_binary_cache(const std::string& cache_name) {
  auto cache_path = get_cache_samples_path() / "formats" / (cache_name + ".bin");
  auto content = read_binary_file(cache_path);
  return content.value_or(std::vector<uint8_t>());
}

std::unique_ptr<TemporaryDirectory> TestDataManager::create_test_environment() {
  auto temp_dir = std::make_unique<TemporaryDirectory>("solar_test_env_");

  // Register for cleanup
  if (!cleanup_registered_) {
    register_cleanup_handler();
  }

  // Note: We don't store the pointer since we're returning ownership
  return temp_dir;
}

std::unique_ptr<TemporaryCache> TestDataManager::create_test_cache(const std::string& type) {
  auto temp_cache = std::make_unique<TemporaryCache>(type);

  // Register for cleanup
  if (!cleanup_registered_) {
    register_cleanup_handler();
  }

  // Note: We don't store the pointer since we're returning ownership
  return temp_cache;
}

bool TestDataManager::validate_jpl_response(const std::string& response) {
  return JPLDataValidator::validate_jpl_response_format(response);
}

bool TestDataManager::validate_ephemeris_data(const std::string& data) {
  return validate_json_format(data);
}

bool TestDataManager::validate_json_format(const std::string& json_data) {
  // Simple JSON validation - check for basic structure
  if (json_data.empty()) return false;

  // Must start with { and end with }
  auto trimmed = json_data;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  return !trimmed.empty() && trimmed.front() == '{' && trimmed.back() == '}';
}

bool TestDataManager::validate_binary_format(const std::vector<uint8_t>& binary_data) {
  if (binary_data.size() < 4) return false;

  // Check for "SOLR" magic header
  return binary_data[0] == 'S' && binary_data[1] == 'O' && binary_data[2] == 'L' &&
         binary_data[3] == 'R';
}

std::string TestDataManager::calculate_checksum(const std::string& data) {
  // Simple checksum implementation (in real implementation, use SHA-256)
  std::hash<std::string> hasher;
  auto hash_value = hasher(data);

  std::stringstream ss;
  ss << "sha256:" << std::hex << hash_value;
  return ss.str();
}

void TestDataManager::cleanup_test_data() {
  temp_directories_.clear();
  temp_caches_.clear();
}

bool TestDataManager::validate_cache_integrity(const std::filesystem::path& cache_path) {
  if (!std::filesystem::exists(cache_path)) {
    return false;
  }

  // Check file extension to determine validation method
  auto extension = cache_path.extension().string();
  if (extension == ".json") {
    auto content = read_file_content(cache_path);
    if (!content) return false;

    // Validate JSON structure
    if (!validate_json_format(*content)) return false;

    // Check for required fields
    std::vector<std::string> required_fields = {"format_version", "cache_type", "body_count",
                                                "bodies"};
    return DataValidator::validate_json_structure(*content, required_fields);
  } else if (extension == ".bin") {
    auto binary_data = read_binary_file(cache_path);
    if (!binary_data) return false;

    return validate_binary_format(*binary_data);
  }

  return false;
}

bool TestDataManager::write_file_content(const std::filesystem::path& file_path,
                                         const std::string& content) {
  std::ofstream file(file_path);
  if (!file.is_open()) {
    return false;
  }

  file << content;
  return file.good();
}

std::string TestDataManager::calculate_file_checksum(const std::filesystem::path& file_path) {
  auto content = read_file_content(file_path);
  if (!content) {
    return "";
  }
  return calculate_checksum(*content);
}

bool TestDataManager::verify_checksum(const std::string& data,
                                      const std::string& expected_checksum) {
  auto computed_checksum = calculate_checksum(data);
  return computed_checksum == expected_checksum;
}

TestDataSet TestDataManager::parse_metadata(const std::string& json_content) {
  TestDataSet dataset;

  // Simple JSON parsing for metadata (in a real implementation, use a JSON library)
  // For now, just set default values and try to extract basic information
  dataset.name = "parsed_dataset";
  dataset.description = "Dataset parsed from metadata";
  dataset.version = "1.0";
  dataset.source = "METADATA_FILE";

  // Try to extract some basic fields using simple string matching
  if (json_content.find("\"name\"") != std::string::npos) {
    // Extract name field if present
    std::regex name_pattern(R"("name"\s*:\s*"([^"]+)") ");
        std::smatch match;
    if (std::regex_search(json_content, match, name_pattern)) {
      dataset.name = match[1].str();
    }
  }

  if (json_content.find("\"description\"") != std::string::npos) {
    std::regex desc_pattern(R"("description"\s*:\s*"([^"]+)") ");
        std::smatch match;
    if (std::regex_search(json_content, match, desc_pattern)) {
      dataset.description = match[1].str();
    }
  }

  if (json_content.find("\"version\"") != std::string::npos) {
    std::regex version_pattern(R"("version"\s*:\s*"([^"]+)") ");
        std::smatch match;
    if (std::regex_search(json_content, match, version_pattern)) {
      dataset.version = match[1].str();
    }
  }

  dataset.checksum = calculate_checksum(json_content);
  return dataset;
}

void TestDataManager::register_cleanup_handler() {
  std::atexit(cleanup_test_data);
  cleanup_registered_ = true;
}

// DataValidator implementation
bool DataValidator::validate_mass_positive(double mass) {
  return mass > 0.0 && std::isfinite(mass);
}

bool DataValidator::validate_position_bounds(const SolarSystem::Math::Vector3d& position,
                                             double max_distance_au) {
  double distance = position.magnitude();
  double max_distance_km = max_distance_au * 149597870.7;  // AU to km
  return distance <= max_distance_km && std::isfinite(distance);
}

bool DataValidator::validate_velocity_bounds(const SolarSystem::Math::Vector3d& velocity,
                                             double max_velocity_kms) {
  double speed = velocity.magnitude();
  return speed <= max_velocity_kms && std::isfinite(speed);
}

double DataValidator::calculate_relative_error(double computed, double reference) {
  if (std::abs(reference) < 1e-15) {
    return std::abs(computed);
  }
  return std::abs((computed - reference) / reference);
}

bool DataValidator::within_tolerance(double value, double reference, double tolerance) {
  return std::abs(value - reference) <= tolerance;
}

bool DataValidator::within_relative_tolerance(double value, double reference,
                                              double relative_tolerance) {
  return calculate_relative_error(value, reference) <= relative_tolerance;
}

bool DataValidator::validate_orbital_energy(double kinetic_energy, double potential_energy,
                                            double expected_total_energy, double tolerance) {
  double total_energy = kinetic_energy + potential_energy;
  return within_tolerance(total_energy, expected_total_energy, tolerance);
}

bool DataValidator::validate_angular_momentum(const SolarSystem::Math::Vector3d& position,
                                              const SolarSystem::Math::Vector3d& velocity,
                                              const SolarSystem::Math::Vector3d& expected_momentum,
                                              double tolerance) {
  auto computed_momentum = position.cross(velocity);
  double error = (computed_momentum - expected_momentum).magnitude();
  return error <= tolerance;
}

bool DataValidator::validate_json_structure(const std::string& json_data,
                                            const std::vector<std::string>& required_fields) {
  if (!TestDataManager::validate_json_format(json_data)) {
    return false;
  }

  // Check for required fields using simple string search
  for (const auto& field : required_fields) {
    std::string field_pattern = "\"" + field + "\"";
    if (json_data.find(field_pattern) == std::string::npos) {
      return false;
    }
  }

  return true;
}

bool DataValidator::validate_binary_header(const std::vector<uint8_t>& binary_data,
                                           const std::string& expected_magic) {
  if (binary_data.size() < expected_magic.size()) {
    return false;
  }

  for (size_t i = 0; i < expected_magic.size(); ++i) {
    if (binary_data[i] != static_cast<uint8_t>(expected_magic[i])) {
      return false;
    }
  }

  return true;
}

bool DataValidator::validate_csv_format(const std::string& csv_data, size_t expected_columns) {
  if (csv_data.empty()) return false;

  // Find the first line
  auto first_newline = csv_data.find('\n');
  std::string first_line =
      (first_newline != std::string::npos) ? csv_data.substr(0, first_newline) : csv_data;

  // Count commas + 1 for column count
  size_t comma_count = std::count(first_line.begin(), first_line.end(), ',');
  return (comma_count + 1) == expected_columns;
}

double DataValidator::calculate_position_error(const SolarSystem::Math::Vector3d& computed,
                                               const SolarSystem::Math::Vector3d& reference) {
  return (computed - reference).magnitude();
}

double DataValidator::calculate_velocity_error(const SolarSystem::Math::Vector3d& computed,
                                               const SolarSystem::Math::Vector3d& reference) {
  return (computed - reference).magnitude();
}

// JPLDataValidator implementation
bool JPLDataValidator::validate_jpl_response_format(const std::string& response) {
  if (response.empty()) return false;

  // Check for required sections
  bool has_header =
      response.find(
          "*******************************************************************************") !=
      std::string::npos;
  bool has_physical_data = response.find("PHYSICAL DATA") != std::string::npos;
  bool has_ephemeris =
      response.find("$SOE") != std::string::npos && response.find("$EOE") != std::string::npos;

  return has_header && has_physical_data && has_ephemeris;
}

bool JPLDataValidator::validate_jpl_header(const std::string& response) {
  // Look for body name and ID in header
  std::regex body_pattern(R"((\w+)\s+(\d+))");
  return std::regex_search(response, body_pattern);
}

std::optional<std::string> JPLDataValidator::extract_body_name(const std::string& response) {
  std::regex name_pattern(R"(Target body name:\s*(\w+))");
  std::smatch match;
  if (std::regex_search(response, match, name_pattern)) {
    return match[1].str();
  }
  return std::nullopt;
}

std::optional<int> JPLDataValidator::extract_body_id(const std::string& response) {
  std::regex id_pattern(R"(Target body name:.*\((\d+)\))");
  std::smatch match;
  if (std::regex_search(response, match, id_pattern)) {
    return std::stoi(match[1].str());
  }
  return std::nullopt;
}

bool JPLDataValidator::validate_jpl_ephemeris_section(const std::string& response) {
  // Check for Start of Ephemeris (SOE) and End of Ephemeris (EOE) markers
  bool has_soe = response.find("$SOE") != std::string::npos;
  bool has_eoe = response.find("$EOE") != std::string::npos;

  if (!has_soe || !has_eoe) return false;

  // Extract ephemeris data between SOE and EOE
  auto soe_pos = response.find("$SOE");
  auto eoe_pos = response.find("$EOE");

  if (eoe_pos <= soe_pos) return false;

  std::string ephemeris_section = response.substr(soe_pos + 4, eoe_pos - soe_pos - 4);

  // Check for at least one line of ephemeris data
  return !ephemeris_section.empty() && ephemeris_section.find(',') != std::string::npos;
}

bool JPLDataValidator::validate_jpl_physical_data(const std::string& response) {
  // Check for physical data section
  return response.find("PHYSICAL DATA") != std::string::npos &&
         (response.find("Mass") != std::string::npos ||
          response.find("radius") != std::string::npos);
}

std::optional<std::vector<double>> JPLDataValidator::extract_ephemeris_coordinates(
    const std::string& response) {
  auto soe_pos = response.find("$SOE");
  auto eoe_pos = response.find("$EOE");

  if (soe_pos == std::string::npos || eoe_pos == std::string::npos || eoe_pos <= soe_pos) {
    return std::nullopt;
  }

  std::string ephemeris_section = response.substr(soe_pos + 4, eoe_pos - soe_pos - 4);

  // Find the first line with coordinate data
  std::istringstream stream(ephemeris_section);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.empty() || line.find(',') == std::string::npos) continue;

    // Parse comma-separated values
    std::vector<double> coordinates;
    std::istringstream line_stream(line);
    std::string value;

    while (std::getline(line_stream, value, ',')) {
      try {
        // Skip the date field (first field)
        if (coordinates.empty() && value.find('-') != std::string::npos) {
          continue;
        }
        coordinates.push_back(std::stod(value));
      } catch (const std::exception&) {
        // Skip invalid values
      }
    }

    if (coordinates.size() >= 6) {  // x, y, z, vx, vy, vz
      return coordinates;
    }
  }

  return std::nullopt;
}

std::string JPLDataValidator::generate_error_response(int http_code,
                                                      const std::string& error_message) {
  std::stringstream response;
  response << "HTTP/" << http_code << " Error\n";
  response << "Content-Type: text/plain\n\n";
  response << "ERROR: " << error_message << "\n";
  response << "JPL HORIZONS System - Error Response\n";
  response << "Please check your request parameters and try again.\n";
  return response.str();
}

std::string JPLDataValidator::generate_malformed_response(const std::string& corruption_type) {
  if (corruption_type == "truncated") {
    return "*******************************************************************************\n"
           " Revised: April 12, 2021                 Earth                            399\n"
           " PHYSICAL DATA (updated 2021-May-11):\n"
           " Vol. mean radius (km) = 6371.01+-0.02   Mass x10^24 (kg)   = 5.97219+-0.0006\n"
           "*********************";  // Truncated
  } else if (corruption_type == "missing_ephemeris") {
    return "*******************************************************************************\n"
           " Revised: April 12, 2021                 Earth                            399\n"
           " PHYSICAL DATA (updated 2021-May-11):\n"
           " Vol. mean radius (km) = 6371.01+-0.02   Mass x10^24 (kg)   = 5.97219+-0.0006\n"
           "*******************************************************************************\n"
           "Ephemeris / WWW_USER Mon Dec 18 15:30:45 2023\n"
           "Target body name: Earth (399)                    {source: DE441}\n"
           "*******************************************************************************\n";
    // Missing $SOE/$EOE section
  } else if (corruption_type == "invalid_data") {
    return "*******************************************************************************\n"
           " Revised: April 12, 2021                 Earth                            399\n"
           " PHYSICAL DATA (updated 2021-May-11):\n"
           " Vol. mean radius (km) = INVALID   Mass x10^24 (kg)   = NaN\n"
           "*******************************************************************************\n"
           "$SOE\n"
           "2000-Jan-01 12:00,INVALID,INVALID,INVALID,INVALID,INVALID,INVALID\n"
           "$EOE\n";
  }

  return "CORRUPTED_RESPONSE_DATA";
}

double JPLDataValidator::compare_with_reference(const std::string& computed_response,
                                                const std::string& reference_file) {
  // Load reference data
  auto reference_content = TestDataManager::read_file_content(reference_file);
  if (!reference_content) {
    return std::numeric_limits<double>::max();  // Error loading reference
  }

  // Extract coordinates from both responses
  auto computed_coords = extract_ephemeris_coordinates(computed_response);
  auto reference_coords = extract_ephemeris_coordinates(*reference_content);

  if (!computed_coords || !reference_coords) {
    return std::numeric_limits<double>::max();  // Error extracting coordinates
  }

  // Calculate RMS error between coordinate sets
  double sum_squared_error = 0.0;
  size_t min_size = std::min(computed_coords->size(), reference_coords->size());

  for (size_t i = 0; i < min_size; ++i) {
    double error = (*computed_coords)[i] - (*reference_coords)[i];
    sum_squared_error += error * error;
  }

  return std::sqrt(sum_squared_error / min_size);
}

bool JPLDataValidator::validate_against_physical_constants(const std::string& response) {
  // Extract body name and check against known physical constants
  auto body_name = extract_body_name(response);
  if (!body_name) return false;

  // Simple validation against known ranges
  if (*body_name == "Earth") {
    // Check if mass is in reasonable range for Earth
    if (response.find("5.97") != std::string::npos) {  // Earth's mass ~5.97e24 kg
      return true;
    }
  } else if (*body_name == "Sun") {
    // Check if mass is in reasonable range for Sun
    if (response.find("1.98") != std::string::npos) {  // Sun's mass ~1.98e30 kg
      return true;
    }
  }

  // For other bodies, just check that some physical data is present
  return response.find("PHYSICAL DATA") != std::string::npos;
}

std::string JPLDataValidator::generate_mock_jpl_response(const std::string& body_name, int body_id,
                                                         const std::string& date) {
  std::stringstream response;
  response << "*******************************************************************************\n";
  response << " Revised: April 12, 2021                 " << body_name
           << "                            " << body_id << "\n\n";
  response << " PHYSICAL DATA (updated 2021-May-11):\n";
  response << " Vol. mean radius (km) = 6371.01+-0.02   Mass x10^24 (kg)   = 5.97219+-0.0006\n";
  response << "*******************************************************************************\n\n";
  response << "Ephemeris / WWW_USER Mon Dec 18 15:30:45 2023\n\n";
  response << "Target body name: " << body_name << " (" << body_id
           << ")                    {source: DE441}\n";
  response << "Center body name: Solar System Barycenter (0)    {source: DE441}\n";
  response << "*******************************************************************************\n";
  response << "$SOE\n";
  response << date
           << ",1.000000000000E+08,2.000000000000E+08,3.000000000000E+07,1.000000000000E+01,2."
              "000000000000E+01,3.000000000000E+00\n";
  response << "$EOE\n";
  response << "*******************************************************************************\n";

  return response.str();
}

// PerformanceTestData implementation
std::vector<TestDataSet> PerformanceTestData::generate_scalability_test_data(
    const std::vector<size_t>& body_counts) {
  std::vector<TestDataSet> datasets;

  for (size_t body_count : body_counts) {
    datasets.push_back(generate_large_system_data(body_count));
  }

  return datasets;
}

TestDataSet PerformanceTestData::generate_large_system_data(size_t body_count) {
  TestDataSet dataset;
  dataset.name = "large_system_" + std::to_string(body_count) + "_bodies";
  dataset.description = "Large system test data with " + std::to_string(body_count) + " bodies";
  dataset.version = "1.0";
  dataset.source = "PERFORMANCE_TEST_GENERATOR";

  // Generate JSON data for large system
  std::stringstream json_data;
  json_data << "{\n";
  json_data << "  \"body_count\": " << body_count << ",\n";
  json_data << "  \"coordinate_system\": \"J2000 Ecliptic\",\n";
  json_data << "  \"bodies\": [\n";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> pos_dist(-1e9, 1e9);    // Position range in km
  std::uniform_real_distribution<> vel_dist(-50.0, 50.0);  // Velocity range in km/s
  std::uniform_real_distribution<> mass_dist(1e20, 1e30);  // Mass range in kg

  for (size_t i = 0; i < body_count; ++i) {
    json_data << "    {\n";
    json_data << "      \"index\": " << i << ",\n";
    json_data << "      \"name\": \"TestBody" << i << "\",\n";
    json_data << "      \"id\": " << (1000 + i) << ",\n";
    json_data << "      \"mass\": " << std::scientific << mass_dist(gen) << ",\n";
    json_data << "      \"position\": {\n";
    json_data << "        \"x\": " << pos_dist(gen) << ",\n";
    json_data << "        \"y\": " << pos_dist(gen) << ",\n";
    json_data << "        \"z\": " << pos_dist(gen) << "\n";
    json_data << "      },\n";
    json_data << "      \"velocity\": {\n";
    json_data << "        \"x\": " << vel_dist(gen) << ",\n";
    json_data << "        \"y\": " << vel_dist(gen) << ",\n";
    json_data << "        \"z\": " << vel_dist(gen) << "\n";
    json_data << "      }\n";
    json_data << "    }";
    if (i < body_count - 1) json_data << ",";
    json_data << "\n";
  }

  json_data << "  ]\n";
  json_data << "}\n";

  std::string json_content = json_data.str();
  dataset.files["system_data.json"] = json_content;
  dataset.checksum = TestDataManager::calculate_checksum(json_content);

  return dataset;
}

TestDataSet PerformanceTestData::generate_long_duration_data(double duration_days,
                                                             double time_step_seconds) {
  TestDataSet dataset;
  dataset.name = "long_duration_" + std::to_string(static_cast<int>(duration_days)) + "d";
  dataset.description = "Long duration test data for " + std::to_string(duration_days) + " days";
  dataset.version = "1.0";
  dataset.source = "PERFORMANCE_TEST_GENERATOR";

  // Generate simulation parameters
  std::stringstream json_data;
  json_data << "{\n";
  json_data << "  \"simulation_parameters\": {\n";
  json_data << "    \"duration_days\": " << duration_days << ",\n";
  json_data << "    \"time_step_seconds\": " << time_step_seconds << ",\n";
  json_data << "    \"total_steps\": "
            << static_cast<size_t>(duration_days * 86400.0 / time_step_seconds) << ",\n";
  json_data << "    \"output_interval_steps\": " << static_cast<size_t>(3600.0 / time_step_seconds)
            << "\n";
  json_data << "  },\n";
  json_data << "  \"expected_performance\": {\n";
  json_data << "    \"max_step_time_microseconds\": 1000,\n";
  json_data << "    \"max_memory_mb\": "
            << estimate_memory_usage(10, duration_days, time_step_seconds) / (1024 * 1024) << ",\n";
  json_data << "    \"min_steps_per_second\": " << 1.0 / (time_step_seconds * 1e-6) << "\n";
  json_data << "  }\n";
  json_data << "}\n";

  std::string json_content = json_data.str();
  dataset.files["simulation_config.json"] = json_content;
  dataset.checksum = TestDataManager::calculate_checksum(json_content);

  return dataset;
}

std::map<std::string, double> PerformanceTestData::load_performance_baselines() {
  std::map<std::string, double> baselines;

  // Load from baseline file if it exists
  auto baseline_path =
      TestDataManager::get_validation_data_path() / "benchmarks" / "performance_baseline.json";
  auto content = TestDataManager::read_file_content(baseline_path);

  if (content) {
    // Parse baseline data (simplified parsing)
    if (content->find("cache_loading_ms") != std::string::npos) {
      baselines["cache_loading"] = 1.0;  // 1ms baseline
    }
    if (content->find("simulation_step_us") != std::string::npos) {
      baselines["simulation_step"] = 1000.0;  // 1000μs baseline
    }
    if (content->find("jpl_parsing_ms") != std::string::npos) {
      baselines["jpl_parsing"] = 10.0;  // 10ms baseline
    }
  } else {
    // Default baselines
    baselines["cache_loading"] = 1.0;       // 1ms
    baselines["simulation_step"] = 1000.0;  // 1000μs
    baselines["jpl_parsing"] = 10.0;        // 10ms
    baselines["web_response"] = 100.0;      // 100ms
  }

  return baselines;
}

bool PerformanceTestData::validate_performance_regression(const std::string& test_name,
                                                          double measured_time,
                                                          double baseline_time, double tolerance) {
  if (baseline_time <= 0.0) return true;  // No baseline to compare against

  double relative_change = (measured_time - baseline_time) / baseline_time;
  return relative_change <= tolerance;  // Allow up to tolerance increase
}

size_t PerformanceTestData::estimate_memory_usage(size_t body_count, double duration_days,
                                                  double time_step_seconds) {
  // Estimate memory usage based on simulation parameters
  size_t body_data_size =
      body_count * (sizeof(double) * 6 + sizeof(int) + 64);  // Position, velocity, mass, name
  size_t history_points = static_cast<size_t>(duration_days * 86400.0 / time_step_seconds /
                                              100.0);  // Store every 100th point
  size_t history_size = body_count * history_points * sizeof(double) * 6;
  size_t overhead = 1024 * 1024;  // 1MB overhead

  return body_data_size + history_size + overhead;
}

bool PerformanceTestData::validate_memory_bounds(size_t measured_memory, size_t expected_memory,
                                                 double tolerance) {
  if (expected_memory == 0) return true;  // No expectation to validate against

  double relative_usage =
      static_cast<double>(measured_memory) / static_cast<double>(expected_memory);
  return relative_usage <= (1.0 + tolerance);  // Allow up to tolerance increase
}

}  // namespace TestData
