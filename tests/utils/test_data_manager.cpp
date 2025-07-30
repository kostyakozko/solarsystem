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

}  // namespace TestData
