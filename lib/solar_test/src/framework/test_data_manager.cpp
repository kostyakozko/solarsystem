#include "solar_test/framework/test_data_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace solar_test {

// Static member definitions
std::vector<std::string> TestDataManager::active_temp_directories_;
bool TestDataManager::cleanup_registered_ = false;

// TemporaryDirectory implementation
TemporaryDirectory::TemporaryDirectory(const std::string& prefix) {
  // Create unique temporary directory
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(10000, 99999);

  auto temp_base = std::filesystem::temp_directory_path();
  temp_path_ = temp_base / (prefix + std::to_string(dis(gen)));

  // Ensure directory doesn't exist and create it
  int attempts = 0;
  while (std::filesystem::exists(temp_path_) && attempts < 10) {
    temp_path_ = temp_base / (prefix + std::to_string(dis(gen)));
    ++attempts;
  }

  std::filesystem::create_directories(temp_path_);

  // Register for cleanup
  TestDataManager::active_temp_directories_.push_back(temp_path_.string());
  if (!TestDataManager::cleanup_registered_) {
    TestDataManager::register_cleanup_handler();
  }
}

TemporaryDirectory::~TemporaryDirectory() {
  if (cleanup_on_destroy_ && std::filesystem::exists(temp_path_)) {
    std::error_code ec;
    std::filesystem::remove_all(temp_path_, ec);
    if (ec) {
      std::cerr << "Warning: Failed to cleanup temporary directory: " << temp_path_ << " - "
                << ec.message() << std::endl;
    }
  }

  // Remove from active directories list
  auto& dirs = TestDataManager::active_temp_directories_;
  dirs.erase(std::remove(dirs.begin(), dirs.end(), temp_path_.string()), dirs.end());
}

TemporaryDirectory::TemporaryDirectory(TemporaryDirectory&& other) noexcept
    : temp_path_(std::move(other.temp_path_)), cleanup_on_destroy_(other.cleanup_on_destroy_) {
  other.cleanup_on_destroy_ = false;
}

TemporaryDirectory& TemporaryDirectory::operator=(TemporaryDirectory&& other) noexcept {
  if (this != &other) {
    temp_path_ = std::move(other.temp_path_);
    cleanup_on_destroy_ = other.cleanup_on_destroy_;
    other.cleanup_on_destroy_ = false;
  }
  return *this;
}

std::string TemporaryDirectory::path() const { return temp_path_.string(); }

void TemporaryDirectory::create_file(const std::string& name, const std::string& content) {
  auto file_path = temp_path_ / name;
  std::ofstream file(file_path);
  if (!file) {
    throw std::runtime_error("Failed to create file: " + file_path.string());
  }
  file << content;
}

void TemporaryDirectory::create_subdirectory(const std::string& name) {
  auto dir_path = temp_path_ / name;
  std::filesystem::create_directories(dir_path);
}

bool TemporaryDirectory::exists() const { return std::filesystem::exists(temp_path_); }

// TemporaryCache implementation
TemporaryCache::TemporaryCache(const std::string& cache_type) : cache_type_(cache_type) {
  temp_dir_ = std::make_unique<TemporaryDirectory>("solar_cache_");
  cache_file_ = cache_type_ + "_cache.bin";
}

TemporaryCache::~TemporaryCache() = default;

TemporaryCache::TemporaryCache(TemporaryCache&& other) noexcept
    : temp_dir_(std::move(other.temp_dir_)),
      cache_file_(std::move(other.cache_file_)),
      cache_type_(std::move(other.cache_type_)) {}

TemporaryCache& TemporaryCache::operator=(TemporaryCache&& other) noexcept {
  if (this != &other) {
    temp_dir_ = std::move(other.temp_dir_);
    cache_file_ = std::move(other.cache_file_);
    cache_type_ = std::move(other.cache_type_);
  }
  return *this;
}

void TemporaryCache::populate_with_valid_data() {
  // Create a realistic binary cache file
  std::string cache_content;

  if (cache_type_ == "ephemeris") {
    // Create valid ephemeris cache data
    cache_content = create_valid_ephemeris_cache();
  } else if (cache_type_ == "jpl") {
    // Create valid JPL response cache
    cache_content = create_valid_jpl_cache();
  }

  temp_dir_->create_file(cache_file_, cache_content);

  // Also create JSON version
  std::string json_file = cache_type_ + "_data.json";
  temp_dir_->create_file(json_file, create_json_cache_data());
}

void TemporaryCache::populate_with_corrupted_data() {
  // Create corrupted cache data
  std::string corrupted_content = "CORRUPTED_CACHE_DATA_INVALID_HEADER";
  temp_dir_->create_file(cache_file_, corrupted_content);
}

void TemporaryCache::simulate_partial_corruption() {
  // Create partially corrupted cache (valid header, corrupted body)
  std::string partial_content = create_valid_cache_header() + "CORRUPTED_BODY_DATA";
  temp_dir_->create_file(cache_file_, partial_content);
}

std::string TemporaryCache::cache_path() const { return temp_dir_->path(); }

std::string TemporaryCache::cache_file() const {
  return (std::filesystem::path(temp_dir_->path()) / cache_file_).string();
}

// TestFixture implementation
TemporaryDirectory* TestFixture::create_temp_directory(const std::string& prefix) {
  auto temp_dir = std::make_unique<TemporaryDirectory>(prefix);
  auto* ptr = temp_dir.get();
  temp_directories_.push_back(std::move(temp_dir));
  return ptr;
}

TemporaryCache* TestFixture::create_temp_cache(const std::string& cache_type) {
  auto temp_cache = std::make_unique<TemporaryCache>(cache_type);
  auto* ptr = temp_cache.get();
  temp_caches_.push_back(std::move(temp_cache));
  return ptr;
}

// TestDataManager implementation
TestDataSet TestDataManager::load_jpl_responses(const std::string& scenario) {
  TestDataSet dataset;
  dataset.name = "jpl_responses_" + scenario;
  dataset.description = "JPL HORIZONS API response samples for " + scenario;

  // Load scenario-specific JPL responses
  std::string data_path = "tests/data/jpl_responses/" + scenario + "/";

  if (scenario == "planets") {
    // Try to load from actual files first, fallback to generated data
    dataset.files["mercury.json"] =
        load_file_or_fallback(data_path + "mercury.json", load_sample_jpl_response("Mercury"));
    dataset.files["venus.json"] =
        load_file_or_fallback(data_path + "venus.json", load_sample_jpl_response("Venus"));
    dataset.files["earth.json"] =
        load_file_or_fallback(data_path + "earth.json", load_sample_jpl_response("Earth"));
    dataset.files["mars.json"] =
        load_file_or_fallback(data_path + "mars.json", load_sample_jpl_response("Mars"));
  } else if (scenario == "error_conditions") {
    dataset.files["timeout.json"] =
        load_file_or_fallback(data_path + "timeout.json", create_timeout_response());
    dataset.files["invalid_body.json"] = load_file_or_fallback(
        data_path + "invalid_body.json", create_error_response("Invalid body ID"));
    dataset.files["server_error.json"] =
        load_file_or_fallback(data_path + "server_error.json", create_server_error_response());
  }

  dataset.metadata["scenario"] = scenario;
  dataset.metadata["format"] = "json";

  return dataset;
}

TestDataSet TestDataManager::load_ephemeris_data(const std::string& time_period) {
  TestDataSet dataset;
  dataset.name = "ephemeris_" + time_period;
  dataset.description = "Ephemeris data for " + time_period;

  if (time_period == "2024") {
    dataset.files["ephemeris_2024.bin"] = create_ephemeris_binary_data();
    dataset.files["ephemeris_2024.json"] = create_ephemeris_json_data();
  } else if (time_period == "historical") {
    dataset.files["ephemeris_1900_2000.bin"] = create_historical_ephemeris_data();
  }

  dataset.metadata["time_period"] = time_period;
  dataset.metadata["bodies_count"] = "27";

  return dataset;
}

TestDataSet TestDataManager::load_cache_samples(const std::string& cache_type) {
  TestDataSet dataset;
  dataset.name = "cache_samples_" + cache_type;
  dataset.description = "Cache samples for " + cache_type + " testing";

  if (cache_type == "valid") {
    dataset.files["valid_cache.bin"] = create_valid_cache_binary();
    dataset.files["valid_cache.json"] = create_valid_cache_json();
  } else if (cache_type == "corrupted") {
    dataset.files["corrupted_header.bin"] = create_corrupted_header_cache();
    dataset.files["corrupted_body.bin"] = create_corrupted_body_cache();
    dataset.files["empty_cache.bin"] = "";
  }

  dataset.metadata["cache_type"] = cache_type;

  return dataset;
}

std::unique_ptr<TemporaryDirectory> TestDataManager::create_test_environment() {
  return std::make_unique<TemporaryDirectory>("test_env_");
}

std::unique_ptr<TemporaryCache> TestDataManager::create_test_cache() {
  return std::make_unique<TemporaryCache>("ephemeris");
}

bool TestDataManager::validate_jpl_response(const std::string& response) {
  // Basic validation of JPL response format
  if (response.empty()) return false;

  // Check for required JPL response elements
  return response.find("$$SOE") != std::string::npos && response.find("$$EOE") != std::string::npos;
}

bool TestDataManager::validate_ephemeris_data(const std::string& data) {
  // Validate ephemeris data format
  if (data.empty()) return false;

  // Check for binary header or JSON structure
  return data.size() > 16 && (data.substr(0, 4) == "EPHE" || data[0] == '{');
}

bool TestDataManager::validate_cache_integrity(const std::string& cache_path) {
  if (!std::filesystem::exists(cache_path)) return false;

  std::ifstream file(cache_path, std::ios::binary);
  if (!file) return false;

  // Read and validate cache header
  char header[4];
  file.read(header, 4);

  if (!file.good() || std::string(header, 4) != "EPHE") {
    return false;
  }

  // Read version
  uint32_t version;
  file.read(reinterpret_cast<char*>(&version), sizeof(version));
  if (!file.good() || version != 1) {
    return false;
  }

  // Read body count
  uint32_t body_count;
  file.read(reinterpret_cast<char*>(&body_count), sizeof(body_count));
  if (!file.good() || body_count == 0 || body_count > 100) {
    return false;
  }

  // Check if we can read at least some body data
  file.seekg(64, std::ios::cur);  // Skip first body data
  return file.good();
}

void TestDataManager::create_test_database(const std::string& output_path) {
  std::filesystem::create_directories(output_path);

  // Create realistic solar system test data
  auto solar_data = load_realistic_solar_system_data();

  // Write test database files
  for (const auto& [filename, content] : solar_data.files) {
    std::ofstream file(std::filesystem::path(output_path) / filename);
    file << content;
  }

  // Create metadata file
  std::ofstream metadata_file(std::filesystem::path(output_path) / "metadata.json");
  metadata_file << "{\n";
  metadata_file << "  \"name\": \"" << solar_data.name << "\",\n";
  metadata_file << "  \"description\": \"" << solar_data.description << "\",\n";
  metadata_file << "  \"files\": [\n";

  bool first = true;
  for (const auto& [filename, _] : solar_data.files) {
    if (!first) metadata_file << ",\n";
    metadata_file << "    \"" << filename << "\"";
    first = false;
  }

  metadata_file << "\n  ]\n}\n";
}

TestDataSet TestDataManager::load_realistic_solar_system_data() {
  TestDataSet dataset;
  dataset.name = "realistic_solar_system";
  dataset.description = "Realistic solar system data for comprehensive testing";

  // Create data for all 27 celestial bodies
  std::vector<std::string> bodies = {
      "Sun",      "Mercury", "Venus",     "Earth",  "Mars",     "Jupiter", "Saturn",
      "Uranus",   "Neptune", "Pluto",     "Moon",   "Io",       "Europa",  "Ganymede",
      "Callisto", "Titan",   "Enceladus", "Mimas",  "Iapetus",  "Phobos",  "Deimos",
      "Ceres",    "Vesta",   "Pallas",    "Hygiea", "Voyager1", "Voyager2"};

  for (const auto& body : bodies) {
    dataset.files[body + "_ephemeris.json"] = create_realistic_body_data(body);
  }

  // Add orbital elements and physical parameters
  dataset.files["orbital_elements.json"] = create_orbital_elements_data();
  dataset.files["physical_parameters.json"] = create_physical_parameters_data();

  dataset.metadata["bodies_count"] = std::to_string(bodies.size());
  dataset.metadata["data_type"] = "comprehensive";

  return dataset;
}

void TestDataManager::cleanup_test_data() {
  for (const auto& dir_path : active_temp_directories_) {
    if (std::filesystem::exists(dir_path)) {
      std::error_code ec;
      std::filesystem::remove_all(dir_path, ec);
      if (ec) {
        std::cerr << "Warning: Failed to cleanup directory: " << dir_path << " - " << ec.message()
                  << std::endl;
      }
    }
  }
  active_temp_directories_.clear();
}

void TestDataManager::register_cleanup_handler() {
  if (cleanup_registered_) return;

  std::atexit(cleanup_test_data);
  cleanup_registered_ = true;
}

// Helper function to load file or use fallback
std::string load_file_or_fallback(const std::string& file_path, const std::string& fallback) {
  if (std::filesystem::exists(file_path)) {
    std::ifstream file(file_path);
    if (file) {
      std::ostringstream content;
      content << file.rdbuf();
      return content.str();
    }
  }
  return fallback;
}

// Helper functions for creating test data
std::string create_valid_ephemeris_cache() {
  std::string cache_data;
  cache_data.reserve(4 + 4 + 4 + 27 * 64);  // Header + version + count + data

  // Header
  cache_data += "EPHE";

  // Version (little-endian)
  uint32_t version = 1;
  cache_data.append(reinterpret_cast<const char*>(&version), sizeof(version));

  // Body count (little-endian)
  uint32_t body_count = 27;
  cache_data.append(reinterpret_cast<const char*>(&body_count), sizeof(body_count));

  // Add sample ephemeris data for each body (64 bytes per body)
  for (int i = 0; i < 27; ++i) {
    cache_data +=
        std::string(64, static_cast<char>('A' + (i % 26)));  // Sample position/velocity data
  }

  return cache_data;
}

std::string create_valid_jpl_cache() {
  return R"($$SOE
2024-Jan-01 00:00     1.234567890E+08  2.345678901E+08  3.456789012E+08
                      4.567890123E+03  5.678901234E+03  6.789012345E+03
$$EOE)";
}

std::string create_json_cache_data() {
  return R"({
  "cache_version": "1.0",
  "timestamp": "2024-01-01T00:00:00Z",
  "bodies": {
    "Earth": {
      "position": [1.234567890E+08, 2.345678901E+08, 3.456789012E+08],
      "velocity": [4.567890123E+03, 5.678901234E+03, 6.789012345E+03]
    }
  }
})";
}

std::string create_valid_cache_header() { return "EPHE\x01\x00\x00\x00\x1B\x00\x00\x00"; }

std::string load_sample_jpl_response(const std::string& body) {
  return "$$SOE\n" + body + " ephemeris data\n$$EOE";
}

std::string create_timeout_response() {
  return R"({"error": "timeout", "message": "Request timed out"})";
}

std::string create_error_response(const std::string& message) {
  return R"({"error": "invalid_request", "message": ")" + message + R"("})";
}

std::string create_server_error_response() {
  return R"({"error": "server_error", "code": 500, "message": "Internal server error"})";
}

std::string create_ephemeris_binary_data() { return create_valid_ephemeris_cache(); }

std::string create_ephemeris_json_data() { return create_json_cache_data(); }

std::string create_historical_ephemeris_data() {
  return create_valid_ephemeris_cache() + std::string(1000, 'H');  // Extended data
}

std::string create_valid_cache_binary() { return create_valid_ephemeris_cache(); }

std::string create_valid_cache_json() { return create_json_cache_data(); }

std::string create_corrupted_header_cache() {
  return "CORR\x01\x00\x00\x00";  // Invalid header
}

std::string create_corrupted_body_cache() {
  return create_valid_cache_header() + std::string(100, '\xFF');  // Corrupted body
}

std::string create_realistic_body_data(const std::string& body) {
  return R"({
  "name": ")" +
         body + R"(",
  "ephemeris": {
    "position": [1.0e8, 2.0e8, 3.0e8],
    "velocity": [1.0e4, 2.0e4, 3.0e4]
  },
  "timestamp": "2024-01-01T00:00:00Z"
})";
}

std::string create_orbital_elements_data() {
  return R"({
  "orbital_elements": {
    "Earth": {
      "semi_major_axis": 1.0,
      "eccentricity": 0.0167,
      "inclination": 0.0,
      "longitude_ascending_node": 0.0,
      "argument_periapsis": 102.9,
      "mean_anomaly": 100.5
    }
  }
})";
}

std::string create_physical_parameters_data() {
  return R"({
  "physical_parameters": {
    "Earth": {
      "mass": 5.972e24,
      "radius": 6371000,
      "rotation_period": 86400
    }
  }
})";
}

}  // namespace solar_test
