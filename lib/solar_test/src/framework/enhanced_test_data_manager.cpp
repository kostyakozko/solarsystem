#include "solar_test/framework/enhanced_test_data_manager.hpp"

#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

namespace solar_test {

// Static member definitions
std::vector<std::unique_ptr<IsolatedTestEnvironment>> EnhancedTestDataManager::active_environments_;
bool EnhancedTestDataManager::performance_monitoring_enabled_ = false;
std::unordered_map<std::string, double> EnhancedTestDataManager::performance_metrics_;
DataVersion EnhancedTestDataManager::current_version_{1, 2, 0, "Enhanced test data management",
                                                      std::chrono::system_clock::now()};

std::mt19937 TestDataGenerator::random_generator_(static_cast<std::mt19937::result_type>(
    std::chrono::steady_clock::now().time_since_epoch().count()));
std::uniform_real_distribution<double> TestDataGenerator::real_dist_(0.0, 1.0);
std::uniform_int_distribution<int> TestDataGenerator::int_dist_(0, 255);

// Validation patterns
const std::regex TestDataValidator::jpl_response_pattern_(R"(\*+.*EPHEMERIS.*\*+)");
const std::regex TestDataValidator::ephemeris_header_pattern_(
    R"(^JDTDB\s+X\s+Y\s+Z\s+VX\s+VY\s+VZ.*)");

// EnhancedTestDataSet implementation
bool EnhancedTestDataSet::is_current_version() const {
  return version.is_compatible_with(EnhancedTestDataManager::get_current_data_version());
}

bool EnhancedTestDataSet::needs_migration() const {
  auto current = EnhancedTestDataManager::get_current_data_version();
  return version.major < current.major ||
         (version.major == current.major && version.minor < current.minor);
}

void EnhancedTestDataSet::update_checksum(const std::string& file, const std::string& content) {
  checksums[file] = test_data_utils::IntegrityChecker::calculate_checksum(content);
}

bool EnhancedTestDataSet::verify_integrity() const {
  return test_data_utils::IntegrityChecker::verify_dataset_integrity(*this);
}

// IsolatedTestEnvironment implementation
IsolatedTestEnvironment::IsolatedTestEnvironment(const std::string& test_name)
    : test_name_(test_name), original_working_dir_(std::filesystem::current_path()) {
  setup_clean_environment();
}

IsolatedTestEnvironment::~IsolatedTestEnvironment() { cleanup_all_resources(); }

IsolatedTestEnvironment::IsolatedTestEnvironment(IsolatedTestEnvironment&& other) noexcept
    : test_name_(std::move(other.test_name_)),
      is_clean_(other.is_clean_),
      original_working_dir_(std::move(other.original_working_dir_)),
      original_env_vars_(std::move(other.original_env_vars_)),
      modified_env_vars_(std::move(other.modified_env_vars_)),
      temp_files_(std::move(other.temp_files_)),
      temp_directories_(std::move(other.temp_directories_)),
      spawned_processes_(std::move(other.spawned_processes_)),
      allocated_ports_(std::move(other.allocated_ports_)) {
  other.is_clean_ = true;  // Prevent cleanup in moved-from object
}

IsolatedTestEnvironment& IsolatedTestEnvironment::operator=(
    IsolatedTestEnvironment&& other) noexcept {
  if (this != &other) {
    cleanup_all_resources();

    test_name_ = std::move(other.test_name_);
    is_clean_ = other.is_clean_;
    original_working_dir_ = std::move(other.original_working_dir_);
    original_env_vars_ = std::move(other.original_env_vars_);
    modified_env_vars_ = std::move(other.modified_env_vars_);
    temp_files_ = std::move(other.temp_files_);
    temp_directories_ = std::move(other.temp_directories_);
    spawned_processes_ = std::move(other.spawned_processes_);
    allocated_ports_ = std::move(other.allocated_ports_);

    other.is_clean_ = true;
  }
  return *this;
}

void IsolatedTestEnvironment::setup_clean_environment() {
  if (is_clean_) return;

  // Create isolated temporary directory
  auto temp_dir = std::filesystem::temp_directory_path() /
                  ("solar_test_" + test_name_ + "_" +
                   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

  std::filesystem::create_directories(temp_dir);
  register_temp_directory(temp_dir.string());

  // Change to isolated directory
  change_working_directory(temp_dir.string());

  is_clean_ = true;
}

void IsolatedTestEnvironment::restore_original_environment() {
  restore_working_directory();
  restore_environment_variables();
}

void IsolatedTestEnvironment::cleanup_all_resources() {
  if (is_clean_) return;

  // Restore working directory first, before cleaning up temp directories
  restore_working_directory();
  cleanup_processes();
  cleanup_network_resources();
  restore_environment_variables();
  cleanup_temp_files();
  cleanup_temp_directories();

  is_clean_ = true;
}

void IsolatedTestEnvironment::register_temp_file(const std::string& path) {
  temp_files_.push_back(path);
}

void IsolatedTestEnvironment::register_temp_directory(const std::string& path) {
  temp_directories_.push_back(path);
}

void IsolatedTestEnvironment::register_process(int pid) { spawned_processes_.push_back(pid); }

void IsolatedTestEnvironment::register_network_port(int port) { allocated_ports_.push_back(port); }

void IsolatedTestEnvironment::set_env_var(const std::string& name, const std::string& value) {
  // Store original value if not already stored
  if (original_env_vars_.find(name) == original_env_vars_.end()) {
    const char* original = std::getenv(name.c_str());
    original_env_vars_[name] = original ? original : "";
  }

  modified_env_vars_[name] = value;
  setenv(name.c_str(), value.c_str(), 1);
}

void IsolatedTestEnvironment::unset_env_var(const std::string& name) {
  // Store original value if not already stored
  if (original_env_vars_.find(name) == original_env_vars_.end()) {
    const char* original = std::getenv(name.c_str());
    original_env_vars_[name] = original ? original : "";
  }

  modified_env_vars_.erase(name);
  unsetenv(name.c_str());
}

std::optional<std::string> IsolatedTestEnvironment::get_env_var(const std::string& name) const {
  const char* value = std::getenv(name.c_str());
  return value ? std::optional<std::string>(value) : std::nullopt;
}

void IsolatedTestEnvironment::change_working_directory(const std::string& path) {
  if (chdir(path.c_str()) != 0) {
    throw std::runtime_error("Failed to change working directory to: " + path);
  }
}

std::string IsolatedTestEnvironment::get_working_directory() const {
  return std::filesystem::current_path().string();
}

void IsolatedTestEnvironment::set_memory_limit(size_t /*bytes*/) {
  // Implementation would use setrlimit on Unix systems
  // For now, just store the limit for testing purposes
}

void IsolatedTestEnvironment::set_time_limit(std::chrono::seconds /*timeout*/) {
  // Implementation would set up alarm or timer
  // For now, just store the limit for testing purposes
}

void IsolatedTestEnvironment::set_file_descriptor_limit(int /*max_fds*/) {
  // Implementation would use setrlimit on Unix systems
  // For now, just store the limit for testing purposes
}

void IsolatedTestEnvironment::cleanup_temp_files() {
  for (const auto& file : temp_files_) {
    try {
      if (std::filesystem::exists(file)) {
        std::filesystem::remove(file);
      }
    } catch (const std::exception& e) {
      std::cerr << "Warning: Failed to cleanup temp file " << file << ": " << e.what() << std::endl;
    }
  }
  temp_files_.clear();
}

void IsolatedTestEnvironment::cleanup_temp_directories() {
  for (const auto& dir : temp_directories_) {
    try {
      if (std::filesystem::exists(dir)) {
        std::filesystem::remove_all(dir);
      }
    } catch (const std::exception& e) {
      std::cerr << "Warning: Failed to cleanup temp directory " << dir << ": " << e.what()
                << std::endl;
    }
  }
  temp_directories_.clear();
}

void IsolatedTestEnvironment::cleanup_processes() {
  for (int pid : spawned_processes_) {
    try {
      // Send SIGTERM first, then SIGKILL if needed
      kill(pid, SIGTERM);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      kill(pid, SIGKILL);
    } catch (const std::exception& e) {
      std::cerr << "Warning: Failed to cleanup process " << pid << ": " << e.what() << std::endl;
    }
  }
  spawned_processes_.clear();
}

void IsolatedTestEnvironment::cleanup_network_resources() {
  // Network port cleanup would be implementation-specific
  // For now, just clear the list
  allocated_ports_.clear();
}

void IsolatedTestEnvironment::restore_environment_variables() {
  for (const auto& [name, original_value] : original_env_vars_) {
    if (original_value.empty()) {
      unsetenv(name.c_str());
    } else {
      setenv(name.c_str(), original_value.c_str(), 1);
    }
  }
  original_env_vars_.clear();
  modified_env_vars_.clear();
}

void IsolatedTestEnvironment::restore_working_directory() {
  try {
    if (chdir(original_working_dir_.c_str()) != 0) {
      std::cerr << "Warning: Failed to restore working directory" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Warning: Failed to restore working directory: " << e.what() << std::endl;
  }
}

// TestDataGenerator implementation
std::string TestDataGenerator::generate_valid_jpl_response(const std::string& body_name) {
  std::ostringstream response;

  response << "*******************************************************************************\n";
  response << " Revised: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n";
  response << " " << body_name << " EPHEMERIS\n";
  response << "*******************************************************************************\n";
  response << "JDTDB            X                   Y                   Z\n";
  response << "                VX                  VY                  VZ\n";
  response << "*******************************************************************************\n";

  // Generate realistic ephemeris data
  for (int i = 0; i < 10; ++i) {
    double jd = 2451545.0 + i;  // J2000.0 + i days
    double x = generate_realistic_orbital_element() * 1e8;
    double y = generate_realistic_orbital_element() * 1e8;
    double z = generate_realistic_orbital_element() * 1e8;
    double vx = generate_realistic_orbital_element() * 1e4;
    double vy = generate_realistic_orbital_element() * 1e4;
    double vz = generate_realistic_orbital_element() * 1e4;

    response << std::fixed << std::setprecision(9);
    response << jd << " " << x << " " << y << " " << z << "\n";
    response << "                " << vx << " " << vy << " " << vz << "\n";
  }

  response << "*******************************************************************************\n";
  return response.str();
}

std::string TestDataGenerator::generate_error_jpl_response(const std::string& error_type) {
  if (error_type == "timeout") {
    return "ERROR: Request timed out after 30 seconds";
  } else if (error_type == "not_found") {
    return "ERROR: Target body not found in JPL database";
  } else if (error_type == "server_error") {
    return "ERROR: Internal server error (500)";
  } else {
    return "ERROR: Unknown error occurred";
  }
}

std::string TestDataGenerator::generate_timeout_jpl_response() {
  return generate_error_jpl_response("timeout");
}

std::string TestDataGenerator::generate_malformed_jpl_response() {
  return "INVALID RESPONSE: Missing headers and malformed data\n"
         "This is not a valid JPL response format\n"
         "Random data: " +
         generate_random_string(100);
}

std::string TestDataGenerator::generate_ephemeris_json(const std::string& time_range) {
  std::ostringstream json;

  json << "{\n";
  json << "  \"format\": \"ephemeris_json\",\n";
  json << "  \"version\": \"1.0\",\n";
  json << "  \"time_range\": \"" << time_range << "\",\n";
  json << "  \"created_at\": " << std::chrono::system_clock::now().time_since_epoch().count()
       << ",\n";
  json << "  \"bodies\": [\n";

  std::vector<std::string> body_names = {"Sun", "Mercury", "Venus", "Earth", "Mars"};

  for (size_t i = 0; i < body_names.size(); ++i) {
    json << generate_body_data_json(body_names[i]);
    if (i < body_names.size() - 1) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ]\n";
  json << "}\n";

  return json.str();
}

std::string TestDataGenerator::generate_ephemeris_binary(const std::string& /*time_range*/) {
  std::ostringstream binary_data;

  // Binary header
  binary_data << "EPHBIN10";  // Magic number + version
  uint32_t body_count = 5;
  binary_data.write(reinterpret_cast<const char*>(&body_count), sizeof(body_count));

  // Generate binary ephemeris data for each body
  for (uint32_t i = 0; i < body_count; ++i) {
    // Body ID
    binary_data.write(reinterpret_cast<const char*>(&i), sizeof(i));

    // Position and velocity data (simplified)
    for (int j = 0; j < 6; ++j) {  // x, y, z, vx, vy, vz
      double value = generate_realistic_orbital_element();
      binary_data.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
  }

  return binary_data.str();
}

std::string TestDataGenerator::generate_corrupted_ephemeris(MutationStrategy strategy) {
  std::string valid_data = generate_ephemeris_json("2025-01-01");
  return mutate_data(valid_data, strategy);
}

std::string TestDataGenerator::generate_valid_cache_file(const std::string& format) {
  if (format == "json") {
    return generate_ephemeris_json("cache_data");
  } else if (format == "binary") {
    return generate_ephemeris_binary("cache_data");
  } else {
    return "CACHE_HEADER_V1\n" + generate_ephemeris_json("cache_data");
  }
}

std::string TestDataGenerator::generate_corrupted_cache_file(MutationStrategy strategy) {
  std::string valid_cache = generate_valid_cache_file("json");
  return mutate_data(valid_cache, strategy);
}

std::string TestDataGenerator::generate_partial_cache_file() {
  std::string full_cache = generate_valid_cache_file("json");
  // Return only first half of the cache file
  return full_cache.substr(0, full_cache.length() / 2);
}

std::string TestDataGenerator::mutate_data(const std::string& original_data,
                                           MutationStrategy strategy) {
  std::string mutated = original_data;

  switch (strategy) {
    case MutationStrategy::CORRUPT_HEADER:
      if (mutated.length() > 10) {
        mutated[0] = 'X';
        mutated[1] = 'X';
      }
      break;

    case MutationStrategy::CORRUPT_BODY:
      if (mutated.length() > 100) {
        size_t pos = mutated.length() / 2;
        mutated[pos] = 'X';
        mutated[pos + 1] = 'X';
      }
      break;

    case MutationStrategy::TRUNCATE_DATA:
      mutated = mutated.substr(0, static_cast<size_t>(static_cast<double>(mutated.length()) * 0.7));
      break;

    case MutationStrategy::ADD_INVALID_FIELDS:
      mutated += "\nINVALID_FIELD: corrupted_value\n";
      break;

    case MutationStrategy::INJECT_MALFORMED_JSON:
      mutated += "{ invalid json syntax ][";
      break;

    default:
      // Add random corruption
      if (!mutated.empty()) {
        size_t pos = static_cast<size_t>(int_dist_(random_generator_)) % mutated.length();
        mutated[pos] = static_cast<char>(int_dist_(random_generator_));
      }
      break;
  }

  return mutated;
}

std::vector<std::string> TestDataGenerator::generate_mutation_variants(
    const std::string& original_data, const std::vector<MutationStrategy>& strategies) {
  std::vector<std::string> variants;

  for (const auto& strategy : strategies) {
    variants.push_back(mutate_data(original_data, strategy));
  }

  return variants;
}

EnhancedTestDataSet TestDataGenerator::generate_realistic_solar_system_data() {
  EnhancedTestDataSet dataset;
  dataset.name = "realistic_solar_system";
  dataset.description = "Realistic solar system data for comprehensive testing";
  dataset.version = EnhancedTestDataManager::get_current_data_version();
  dataset.created_at = std::chrono::system_clock::now();

  // Generate data for major solar system bodies
  std::vector<std::string> bodies = {"Sun",     "Mercury", "Venus",  "Earth",  "Mars",
                                     "Jupiter", "Saturn",  "Uranus", "Neptune"};

  for (const auto& body : bodies) {
    std::string jpl_response = generate_valid_jpl_response(body);
    dataset.files[body + "_ephemeris.txt"] = jpl_response;
    dataset.update_checksum(body + "_ephemeris.txt", jpl_response);
  }

  // Add metadata
  dataset.metadata["body_count"] = std::to_string(bodies.size());
  dataset.metadata["data_type"] = "ephemeris";
  dataset.metadata["coordinate_system"] = "ICRF";

  return dataset;
}

EnhancedTestDataSet TestDataGenerator::generate_historical_data_set(const std::string& epoch) {
  EnhancedTestDataSet dataset;
  dataset.name = "historical_" + epoch;
  dataset.description = "Historical ephemeris data for epoch " + epoch;
  dataset.version = EnhancedTestDataManager::get_current_data_version();
  dataset.created_at = std::chrono::system_clock::now();

  // Generate historical ephemeris data
  std::string ephemeris_data = generate_ephemeris_json(epoch);
  dataset.files["ephemeris_" + epoch + ".json"] = ephemeris_data;
  dataset.update_checksum("ephemeris_" + epoch + ".json", ephemeris_data);

  dataset.metadata["epoch"] = epoch;
  dataset.metadata["data_type"] = "historical_ephemeris";

  return dataset;
}

EnhancedTestDataSet TestDataGenerator::generate_stress_test_data_set(size_t data_size) {
  EnhancedTestDataSet dataset;
  dataset.name = "stress_test_" + std::to_string(data_size);
  dataset.description = "Large dataset for stress testing";
  dataset.version = EnhancedTestDataManager::get_current_data_version();
  dataset.created_at = std::chrono::system_clock::now();

  // Generate large amount of test data
  for (size_t i = 0; i < data_size; ++i) {
    std::string filename = "data_" + std::to_string(i) + ".txt";
    std::string content = generate_random_string(1000);  // 1KB per file
    dataset.files[filename] = content;
    dataset.update_checksum(filename, content);
  }

  dataset.metadata["file_count"] = std::to_string(data_size);
  dataset.metadata["data_type"] = "stress_test";

  return dataset;
}

std::string TestDataGenerator::generate_random_string(size_t length) {
  std::string result;
  result.reserve(length);

  const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  for (size_t i = 0; i < length; ++i) {
    result += chars[static_cast<size_t>(int_dist_(random_generator_)) % chars.length()];
  }

  return result;
}

double TestDataGenerator::generate_realistic_orbital_element() {
  return (real_dist_(random_generator_) - 0.5) * 2.0;  // Range: -1.0 to 1.0
}

std::string TestDataGenerator::generate_body_data_json(const std::string& body_name) {
  std::ostringstream json;

  json << "    {\n";
  json << "      \"name\": \"" << body_name << "\",\n";
  json << "      \"id\": " << (std::hash<std::string>{}(body_name) % 1000) << ",\n";
  json << "      \"position\": [";

  for (int i = 0; i < 3; ++i) {
    json << (generate_realistic_orbital_element() * 1e8);
    if (i < 2) json << ", ";
  }

  json << "],\n";
  json << "      \"velocity\": [";

  for (int i = 0; i < 3; ++i) {
    json << (generate_realistic_orbital_element() * 1e4);
    if (i < 2) json << ", ";
  }

  json << "],\n";
  json << "      \"timestamp\": " << std::chrono::system_clock::now().time_since_epoch().count()
       << "\n";
  json << "    }";

  return json.str();
}

// TestDataValidator implementation
ValidationResult TestDataValidator::validate_jpl_response_comprehensive(
    const std::string& response) {
  ValidationResult result;
  result.validation_type = "JPL Response Comprehensive";
  result.validated_at = std::chrono::system_clock::now();

  // Format validation
  auto format_result = validate_jpl_response_format(response);
  result.errors.insert(result.errors.end(), format_result.errors.begin(),
                       format_result.errors.end());
  result.warnings.insert(result.warnings.end(), format_result.warnings.begin(),
                         format_result.warnings.end());

  // Content validation
  auto content_result = validate_jpl_response_content(response);
  result.errors.insert(result.errors.end(), content_result.errors.begin(),
                       content_result.errors.end());
  result.warnings.insert(result.warnings.end(), content_result.warnings.begin(),
                         content_result.warnings.end());

  result.is_valid = result.errors.empty();
  return result;
}

ValidationResult TestDataValidator::validate_jpl_response_format(const std::string& response) {
  ValidationResult result;
  result.validation_type = "JPL Response Format";
  result.validated_at = std::chrono::system_clock::now();

  if (response.empty()) {
    result.add_error("Response is empty");
    return result;
  }

  if (!std::regex_search(response, jpl_response_pattern_)) {
    result.add_error("Response does not match expected JPL format pattern");
  }

  if (response.find(
          "*******************************************************************************") ==
      std::string::npos) {
    result.add_error("Missing JPL response header markers");
  }

  if (response.find("JDTDB") == std::string::npos) {
    result.add_warning("Missing JDTDB time format indicator");
  }

  result.is_valid = result.errors.empty();
  return result;
}

ValidationResult TestDataValidator::validate_jpl_response_content(const std::string& response) {
  ValidationResult result;
  result.validation_type = "JPL Response Content";
  result.validated_at = std::chrono::system_clock::now();

  // Check for error messages
  if (response.find("ERROR:") != std::string::npos) {
    result.add_error("Response contains error message");
  }

  // Check for reasonable data ranges
  std::istringstream iss(response);
  std::string line;
  int data_lines = 0;

  while (std::getline(iss, line)) {
    if (line.find_first_of("0123456789") != std::string::npos &&
        line.find("*") == std::string::npos) {
      data_lines++;

      // Basic sanity check for numerical values
      std::istringstream line_stream(line);
      double value;
      int value_count = 0;

      while (line_stream >> value && value_count < 10) {
        value_count++;
        if (std::abs(value) > 1e15) {
          result.add_warning("Extremely large value detected: " + std::to_string(value));
        }
      }
    }
  }

  if (data_lines == 0) {
    result.add_error("No numerical data found in response");
  } else if (data_lines < 5) {
    result.add_warning("Very few data points found: " + std::to_string(data_lines));
  }

  result.is_valid = result.errors.empty();
  return result;
}

ValidationResult TestDataValidator::validate_ephemeris_data_comprehensive(const std::string& data) {
  ValidationResult result;
  result.validation_type = "Ephemeris Data Comprehensive";
  result.validated_at = std::chrono::system_clock::now();

  if (data.empty()) {
    result.add_error("Ephemeris data is empty");
    return result;
  }

  std::istringstream iss(data);
  std::string line;
  int data_lines = 0;
  bool found_header = false;
  bool found_data_section = false;

  while (std::getline(iss, line)) {
    if (line.find("$$SOE") != std::string::npos) {
      found_data_section = true;
      continue;
    }
    if (line.find("$$EOE") != std::string::npos) {
      found_data_section = false;
      continue;
    }
    if (line.find("JDTDB") != std::string::npos || line.find("Julian") != std::string::npos) {
      found_header = true;
    }

    if (found_data_section) {
      data_lines++;
      std::istringstream ls(line);
      double val;
      int count = 0;
      while (ls >> val && count < 10) {
        count++;
        if (std::isnan(val) || std::isinf(val)) {
          result.add_error("NaN/Inf value found in ephemeris data line " +
                           std::to_string(data_lines));
        }
        if (std::abs(val) > 1e20) {
          result.add_warning("Extremely large value in ephemeris: " + std::to_string(val));
        }
      }
    }
  }

  if (!found_header) {
    result.add_warning("No ephemeris header markers found (JDTDB/Julian)");
  }
  if (data_lines == 0) {
    result.add_error("No ephemeris data points found between $$SOE/$$EOE markers");
  } else if (data_lines < 3) {
    result.add_warning("Very few ephemeris data points: " + std::to_string(data_lines));
  }

  result.is_valid = result.errors.empty();
  return result;
}

ValidationResult TestDataValidator::validate_cache_file_comprehensive(
    const std::string& cache_path) {
  ValidationResult result;
  result.validation_type = "Cache File Comprehensive";
  result.validated_at = std::chrono::system_clock::now();

  if (cache_path.empty()) {
    result.add_error("Cache path is empty");
    return result;
  }

  namespace fs = std::filesystem;
  std::error_code ec;

  if (!fs::exists(cache_path, ec)) {
    result.add_error("Cache file does not exist: " + cache_path);
    return result;
  }

  auto file_size = fs::file_size(cache_path, ec);
  if (ec) {
    result.add_error("Cannot read cache file size: " + ec.message());
    return result;
  }

  if (file_size == 0) {
    result.add_error("Cache file is empty");
    return result;
  }

  if (file_size < 16) {
    result.add_warning("Cache file is suspiciously small (" + std::to_string(file_size) +
                       " bytes)");
  }

  // Attempt to read and validate contents
  std::ifstream f(cache_path, std::ios::binary);
  if (!f.is_open()) {
    result.add_error("Cannot open cache file for reading");
    return result;
  }

  // Check for valid content by reading first bytes
  char header[4] = {};
  f.read(header, sizeof(header));
  if (!f) {
    result.add_error("Cannot read cache file header");
    return result;
  }

  // Check write time / staleness
  auto last_write = fs::last_write_time(cache_path, ec);
  if (!ec) {
    auto age = fs::file_time_type::clock::now() - last_write;
    auto age_hours = std::chrono::duration_cast<std::chrono::hours>(age).count();
    if (age_hours > 24 * 30) {
      result.add_warning("Cache file is older than 30 days (" + std::to_string(age_hours / 24) +
                         " days)");
    }
  }

  result.is_valid = result.errors.empty();
  return result;
}

std::optional<std::string> TestDataValidator::attempt_data_recovery(
    const std::string& corrupted_data, const std::string& data_type) {
  if (corrupted_data.empty()) return std::nullopt;

  if (data_type == "jpl_response" || data_type == "ephemeris") {
    // Try to extract usable data between standard JPL markers
    auto soe = corrupted_data.find("$$SOE");
    auto eoe = corrupted_data.find("$$EOE");
    if (soe != std::string::npos && eoe != std::string::npos && eoe > soe) {
      return corrupted_data.substr(soe, eoe - soe + 5);
    }
    // Try to salvage lines that look like numerical data
    std::ostringstream recovered;
    std::istringstream iss(corrupted_data);
    std::string line;
    int recovered_lines = 0;
    while (std::getline(iss, line)) {
      bool has_number = false;
      for (char c : line) {
        if (std::isdigit(c) || c == '.' || c == '-' || c == 'E' || c == 'e') {
          has_number = true;
          break;
        }
      }
      if (has_number && line.find("*") == std::string::npos) {
        recovered << line << "\n";
        recovered_lines++;
      }
    }
    if (recovered_lines > 0) return recovered.str();
  }

  if (data_type == "json") {
    // Attempt to find a valid JSON object or array
    auto first_brace = corrupted_data.find_first_of("{[");
    auto last_brace = corrupted_data.find_last_of("}]");
    if (first_brace != std::string::npos && last_brace != std::string::npos &&
        last_brace > first_brace) {
      return corrupted_data.substr(first_brace, last_brace - first_brace + 1);
    }
  }

  return std::nullopt;
}

std::vector<std::string> TestDataValidator::suggest_recovery_actions(
    const ValidationResult& validation_result) {
  std::vector<std::string> actions;

  if (validation_result.is_valid) return actions;

  for (const auto& error : validation_result.errors) {
    if (error.find("empty") != std::string::npos) {
      actions.push_back("Re-fetch data from the source (data is empty or missing)");
    } else if (error.find("does not exist") != std::string::npos) {
      actions.push_back("Ensure the file path is correct and the file has been created");
      actions.push_back("Run the data fetch/generation step before validation");
    } else if (error.find("format") != std::string::npos ||
               error.find("pattern") != std::string::npos) {
      actions.push_back("Verify the data source is returning the expected format");
      actions.push_back("Check for API changes or version mismatches");
    } else if (error.find("NaN") != std::string::npos || error.find("Inf") != std::string::npos) {
      actions.push_back("Check for corrupted numerical values in the data");
      actions.push_back("Re-fetch from the original source to replace corrupted data");
    } else if (error.find("Cannot open") != std::string::npos ||
               error.find("Cannot read") != std::string::npos) {
      actions.push_back("Check file permissions and ensure the file is not locked");
    } else if (error.find("header") != std::string::npos) {
      actions.push_back("Validate that the file was not truncated during download");
    }
  }

  for (const auto& warning : validation_result.warnings) {
    if (warning.find("older than") != std::string::npos) {
      actions.push_back("Consider refreshing stale cache data");
    } else if (warning.find("small") != std::string::npos) {
      actions.push_back("Verify the data source returned a complete response");
    } else if (warning.find("few") != std::string::npos) {
      actions.push_back("Check if the requested time range is too narrow");
    }
  }

  if (actions.empty()) {
    actions.push_back("Delete and regenerate the data from the original source");
    actions.push_back("Check system logs for underlying I/O or network errors");
  }

  return actions;
}

// EnhancedTestDataManager implementation
EnhancedTestDataSet EnhancedTestDataManager::load_validated_jpl_responses(
    const std::string& scenario) {
  auto start_time = std::chrono::high_resolution_clock::now();

  EnhancedTestDataSet dataset;
  dataset.name = "jpl_responses_" + scenario;
  dataset.description = "Validated JPL responses for scenario: " + scenario;
  dataset.version = current_version_;
  dataset.created_at = std::chrono::system_clock::now();

  // Load base data using parent class method
  auto base_dataset = TestDataManager::load_jpl_responses(scenario);
  dataset.files = base_dataset.files;
  dataset.metadata = base_dataset.metadata;

  // Perform comprehensive validation
  for (const auto& [filename, content] : dataset.files) {
    auto validation_result = TestDataValidator::validate_jpl_response_comprehensive(content);

    if (!validation_result.is_valid) {
      dataset.validation_result.errors.insert(dataset.validation_result.errors.end(),
                                              validation_result.errors.begin(),
                                              validation_result.errors.end());
    }

    dataset.validation_result.warnings.insert(dataset.validation_result.warnings.end(),
                                              validation_result.warnings.begin(),
                                              validation_result.warnings.end());

    // Update checksum
    dataset.update_checksum(filename, content);
  }

  dataset.validation_result.is_valid = dataset.validation_result.errors.empty();
  dataset.validation_result.validation_type = "JPL Responses Comprehensive";
  dataset.validation_result.validated_at = std::chrono::system_clock::now();
  dataset.last_validated_at = dataset.validation_result.validated_at;

  // Update performance metrics
  if (performance_monitoring_enabled_) {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    update_performance_metric("jpl_response_load_time_ms", static_cast<double>(duration.count()));
  }

  return dataset;
}

EnhancedTestDataSet EnhancedTestDataManager::generate_test_data_set(
    const std::string& type, const std::vector<MutationStrategy>& /*mutations*/) {
  if (type == "realistic") {
    return TestDataGenerator::generate_realistic_solar_system_data();
  } else if (type == "historical") {
    return TestDataGenerator::generate_historical_data_set("J2000");
  } else {
    return TestDataGenerator::generate_stress_test_data_set(10);
  }
}

std::vector<EnhancedTestDataSet> EnhancedTestDataManager::generate_mutation_test_suite(
    const std::string& /*base_type*/) {
  std::vector<EnhancedTestDataSet> suite;
  suite.push_back(TestDataGenerator::generate_realistic_solar_system_data());
  return suite;
}

std::unique_ptr<IsolatedTestEnvironment> EnhancedTestDataManager::create_isolated_environment(
    const std::string& test_name) {
  return std::make_unique<IsolatedTestEnvironment>(test_name);
}

void EnhancedTestDataManager::cleanup_all_test_environments() { active_environments_.clear(); }

DataVersion EnhancedTestDataManager::get_current_data_version() { return current_version_; }

bool EnhancedTestDataManager::migrate_data_set(EnhancedTestDataSet& dataset,
                                               const DataVersion& target_version) {
  if (dataset.version.is_compatible_with(target_version)) {
    dataset.version = target_version;
    return true;
  }

  // Major version mismatch requires re-validation
  if (dataset.version.major != target_version.major) {
    auto validation = TestDataValidator::validate_data_consistency(dataset);
    if (!validation.is_valid) return false;
  }

  // Re-validate all files and update checksums
  for (const auto& [filename, content] : dataset.files) {
    dataset.update_checksum(filename, content);
  }

  dataset.version = target_version;
  dataset.last_validated_at = std::chrono::system_clock::now();
  return true;
}

ValidationResult EnhancedTestDataManager::validate_test_environment() {
  ValidationResult result;
  result.validation_type = "Test Environment";
  result.validated_at = std::chrono::system_clock::now();

  namespace fs = std::filesystem;
  std::error_code ec;

  // Check current working directory is accessible
  auto cwd = fs::current_path(ec);
  if (ec) {
    result.add_error("Cannot determine current working directory: " + ec.message());
  }

  // Check that /tmp (or system temp) is writable
  auto temp = fs::temp_directory_path(ec);
  if (ec) {
    result.add_error("Cannot access temporary directory: " + ec.message());
  } else {
    auto test_file = temp / "solar_test_env_check";
    std::ofstream f(test_file);
    if (!f.is_open()) {
      result.add_error("Cannot write to temporary directory: " + temp.string());
    } else {
      f.close();
      fs::remove(test_file, ec);
    }
  }

  // Verify data directory exists if set
  const char* data_dir = std::getenv("SOLAR_TEST_DATA_DIR");
  if (data_dir) {
    if (!fs::exists(data_dir, ec)) {
      result.add_warning("SOLAR_TEST_DATA_DIR is set but path does not exist: " +
                         std::string(data_dir));
    }
  }

  // Check available memory
#ifdef __linux__
  long pages = sysconf(_SC_AVPHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages > 0 && page_size > 0) {
    size_t avail_mb =
        static_cast<size_t>(pages) * static_cast<size_t>(page_size) / (1024UL * 1024UL);
    if (avail_mb < 64) {
      result.add_warning("Low available memory: " + std::to_string(avail_mb) + " MB");
    }
  }
#endif

  result.is_valid = result.errors.empty();
  return result;
}

ValidationResult EnhancedTestDataManager::validate_all_test_data() {
  ValidationResult result;
  result.validation_type = "All Test Data";
  result.validated_at = std::chrono::system_clock::now();

  // Validate each active environment's integrity
  for (const auto& env : active_environments_) {
    if (env && !env->is_clean()) {
      result.add_warning("Active environment '" + env->get_test_name() +
                         "' is not in a clean state");
    }
  }

  // Validate standard test data scenarios
  const std::vector<std::string> scenarios = {"default", "earth", "mars"};
  for (const auto& scenario : scenarios) {
    try {
      auto dataset = load_validated_jpl_responses(scenario);
      if (!dataset.validation_result.is_valid) {
        for (const auto& err : dataset.validation_result.errors) {
          result.add_error("Scenario '" + scenario + "': " + err);
        }
      }
      for (const auto& warn : dataset.validation_result.warnings) {
        result.add_warning("Scenario '" + scenario + "': " + warn);
      }
    } catch (const std::exception& e) {
      result.add_warning("Could not validate scenario '" + scenario + "': " + e.what());
    }
  }

  // Check data version compatibility
  auto version = get_current_data_version();
  if (version.major < 1) {
    result.add_error("Data version is too old: " + version.to_string());
  }

  result.is_valid = result.errors.empty();
  return result;
}

std::vector<ValidationResult> EnhancedTestDataManager::run_comprehensive_validation_suite() {
  std::vector<ValidationResult> results;
  results.push_back(validate_test_environment());
  results.push_back(validate_all_test_data());
  return results;
}

bool EnhancedTestDataManager::attempt_automatic_recovery(const std::string& data_path) {
  namespace fs = std::filesystem;
  std::error_code ec;

  if (data_path.empty()) return false;

  // If file doesn't exist, nothing to recover
  if (!fs::exists(data_path, ec)) return false;

  // Try to read the file
  std::ifstream f(data_path);
  if (!f.is_open()) return false;

  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  f.close();

  if (content.empty()) return false;

  // Determine data type from extension
  std::string data_type;
  auto ext = fs::path(data_path).extension().string();
  if (ext == ".json") {
    data_type = "json";
  } else if (ext == ".bin" || ext == ".dat") {
    data_type = "ephemeris";
  } else {
    data_type = "jpl_response";
  }

  // Attempt recovery
  auto recovered = TestDataValidator::attempt_data_recovery(content, data_type);
  if (!recovered.has_value()) return false;

  // Backup original
  std::string backup_path = data_path + ".bak";
  fs::copy_file(data_path, backup_path, fs::copy_options::overwrite_existing, ec);

  // Write recovered data
  std::ofstream out(data_path, std::ios::trunc);
  if (!out.is_open()) return false;

  out << recovered.value();
  return out.good();
}

std::vector<std::string> EnhancedTestDataManager::generate_recovery_report(
    const std::vector<ValidationResult>& validation_results) {
  std::vector<std::string> report;

  if (validation_results.empty()) {
    report.push_back("No validation results to report.");
    return report;
  }

  int total = 0, passed = 0, failed = 0;
  for (const auto& vr : validation_results) {
    total++;
    if (vr.is_valid)
      passed++;
    else
      failed++;
  }

  report.push_back("=== Validation Recovery Report ===");
  report.push_back("Total validations: " + std::to_string(total));
  report.push_back("Passed: " + std::to_string(passed));
  report.push_back("Failed: " + std::to_string(failed));
  report.push_back("");

  for (const auto& vr : validation_results) {
    report.push_back("[" + std::string(vr.is_valid ? "PASS" : "FAIL") + "] " + vr.validation_type);

    for (const auto& error : vr.errors) {
      report.push_back("  ERROR: " + error);
    }
    for (const auto& warning : vr.warnings) {
      report.push_back("  WARN:  " + warning);
    }

    if (!vr.is_valid) {
      auto actions = TestDataValidator::suggest_recovery_actions(vr);
      if (!actions.empty()) {
        report.push_back("  Suggested actions:");
        for (const auto& action : actions) {
          report.push_back("    - " + action);
        }
      }
    }
  }

  report.push_back("");
  report.push_back("=== End of Report ===");
  return report;
}

void EnhancedTestDataManager::enable_performance_monitoring(bool enable) {
  performance_monitoring_enabled_ = enable;
}

std::unordered_map<std::string, double> EnhancedTestDataManager::get_performance_metrics() {
  return performance_metrics_;
}

void EnhancedTestDataManager::reset_performance_metrics() { performance_metrics_.clear(); }

void EnhancedTestDataManager::update_performance_metric(const std::string& metric_name,
                                                        double value) {
  performance_metrics_[metric_name] = value;
}

// Utility implementations
namespace test_data_utils {

AutoCleanupGuard::AutoCleanupGuard(std::function<void()> cleanup_func)
    : cleanup_func_(std::move(cleanup_func)) {}

AutoCleanupGuard::~AutoCleanupGuard() {
  if (!released_ && cleanup_func_) {
    try {
      cleanup_func_();
    } catch (const std::exception& e) {
      std::cerr << "Warning: Cleanup function threw exception: " << e.what() << std::endl;
    }
  }
}

void AutoCleanupGuard::release() { released_ = true; }

std::string IntegrityChecker::calculate_checksum(const std::string& data) {
  // Simple hash implementation using std::hash
  std::hash<std::string> hasher;
  size_t hash_value = hasher(data);

  std::ostringstream oss;
  oss << std::hex << hash_value;

  return oss.str();
}

bool IntegrityChecker::verify_checksum(const std::string& data, const std::string& expected) {
  return calculate_checksum(data) == expected;
}

std::unordered_map<std::string, std::string> IntegrityChecker::calculate_dataset_checksums(
    const EnhancedTestDataSet& dataset) {
  std::unordered_map<std::string, std::string> checksums;

  for (const auto& [filename, content] : dataset.files) {
    checksums[filename] = calculate_checksum(content);
  }

  return checksums;
}

bool IntegrityChecker::verify_dataset_integrity(const EnhancedTestDataSet& dataset) {
  for (const auto& [filename, expected_checksum] : dataset.checksums) {
    auto it = dataset.files.find(filename);
    if (it == dataset.files.end()) {
      return false;  // File missing
    }

    if (!verify_checksum(it->second, expected_checksum)) {
      return false;  // Checksum mismatch
    }
  }

  return true;
}

}  // namespace test_data_utils

}  // namespace solar_test
