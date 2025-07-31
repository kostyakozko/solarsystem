/**
 * @file cache_mock.cpp
 * @brief Implementation of cache operation mock system
 */

#include "solar_test/mocks/cache_mock.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

#include "solar_core/bodies/body_mappings.hpp"

namespace SolarSystem::Testing::Mocks {

/**
 * @brief CacheMock constructor
 */
CacheMock::CacheMock(CacheMockConfig config)
    : config_(std::move(config)),
      gen_(rd_()),
      error_dist_(0.0, 1.0),
      cache_directory_("./cache"),
      binary_cache_path_(cache_directory_ / "ephemeris_cache.bin"),
      json_cache_path_(cache_directory_ / "ephemeris_data.json"),
      metadata_path_(cache_directory_ / "metadata.json") {
  // Initialize operation counts
  operation_counts_[MockCacheOperationType::Read] = 0;
  operation_counts_[MockCacheOperationType::Write] = 0;
  operation_counts_[MockCacheOperationType::Validate] = 0;
  operation_counts_[MockCacheOperationType::Clear] = 0;
  operation_counts_[MockCacheOperationType::MetadataRead] = 0;
  operation_counts_[MockCacheOperationType::MetadataWrite] = 0;
  operation_counts_[MockCacheOperationType::DirectoryCreate] = 0;

  // Initialize with default valid cache data
  if (config_.cache_always_exists) {
    populate_with_valid_data();
  }
}

/**
 * @brief CacheMock destructor
 */
CacheMock::~CacheMock() {
  if (config_.auto_cleanup) {
    // Cleanup any temporary files created during testing
    mock_files_.clear();
    file_exists_map_.clear();
    file_sizes_.clear();
  }
}

/**
 * @brief Set whether cache exists
 */
void CacheMock::set_cache_exists(bool exists) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  cache_exists_ = exists;

  // Update file existence simulation
  file_exists_map_[binary_cache_path_.string()] = exists;
  file_exists_map_[json_cache_path_.string()] = exists;
  file_exists_map_[metadata_path_.string()] = exists;
}

/**
 * @brief Set whether cache is valid
 */
void CacheMock::set_cache_valid(bool valid) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  cache_valid_ = valid;
  cache_corrupted_ = !valid;
}

/**
 * @brief Set cache data content
 */
void CacheMock::set_cache_data(const std::string& data) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  cache_data_ = data;

  // Store in mock file system
  mock_files_[binary_cache_path_.string()] = data;
  mock_files_[json_cache_path_.string()] = data;
  file_sizes_[binary_cache_path_.string()] = data.size();
  file_sizes_[json_cache_path_.string()] = data.size();
}

/**
 * @brief Set cache data as ephemeris data
 */
void CacheMock::set_cache_data(const std::vector<SolarSystem::JPL::EphemerisData>& data) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  ephemeris_data_ = data;

  // Convert to binary and JSON formats
  auto binary_data = ephemeris_data_to_binary(data);
  auto json_data = ephemeris_data_to_json(data);

  mock_files_[binary_cache_path_.string()] = binary_data;
  mock_files_[json_cache_path_.string()] = json_data;
  file_sizes_[binary_cache_path_.string()] = binary_data.size();
  file_sizes_[json_cache_path_.string()] = json_data.size();

  cache_data_ = binary_data;  // Use binary as primary
}

/**
 * @brief Set cache metadata
 */
void CacheMock::set_cache_metadata(const SolarSystem::JPL::CacheMetadata& metadata) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  metadata_ = metadata;

  // Generate metadata JSON
  auto metadata_json = generate_metadata_json(metadata);
  mock_files_[metadata_path_.string()] = metadata_json;
  file_sizes_[metadata_path_.string()] = metadata_json.size();
}

/**
 * @brief Simulate cache corruption
 */
void CacheMock::simulate_cache_corruption() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  cache_corrupted_ = true;
  cache_valid_ = false;

  // Corrupt existing cache data
  if (!cache_data_.empty()) {
    cache_data_ = generate_corrupted_data(cache_data_, 0.5);
    mock_files_[binary_cache_path_.string()] = cache_data_;
  }
}

/**
 * @brief Simulate disk full condition
 */
void CacheMock::simulate_disk_full() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  disk_full_simulated_ = true;
}

/**
 * @brief Simulate permission denied errors
 */
void CacheMock::simulate_permission_denied() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  permission_denied_simulated_ = true;
}

/**
 * @brief Simulate I/O errors for next N operations
 */
void CacheMock::simulate_io_error(size_t operation_count) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  remaining_io_errors_ = operation_count;
}

/**
 * @brief Simulate partial cache corruption
 */
void CacheMock::simulate_partial_corruption(double corruption_percentage) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  if (!cache_data_.empty()) {
    cache_data_ = generate_corrupted_data(cache_data_, corruption_percentage);
    mock_files_[binary_cache_path_.string()] = cache_data_;
  }
  cache_corrupted_ = true;
  cache_valid_ = false;
}

/**
 * @brief Simulate slow disk operations
 */
void CacheMock::simulate_slow_disk(std::chrono::milliseconds delay) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  slow_disk_delay_ = delay;
}

/**
 * @brief Populate cache with valid test data
 */
void CacheMock::populate_with_valid_data() {
  auto test_data = generate_valid_test_data();
  set_cache_data(test_data);

  // Create valid metadata
  SolarSystem::JPL::CacheMetadata metadata;
  metadata.created_at = std::chrono::system_clock::now();
  metadata.epoch = std::chrono::system_clock::now();
  metadata.source = "TEST_DATA";
  metadata.body_count = test_data.size();
  metadata.checksum = 12345;  // Simple test checksum

  set_cache_metadata(metadata);
  set_cache_exists(true);
  set_cache_valid(true);
}

/**
 * @brief Populate cache with corrupted test data
 */
void CacheMock::populate_with_corrupted_data() {
  populate_with_valid_data();
  simulate_cache_corruption();
}

/**
 * @brief Populate cache with specific ephemeris data
 */
void CacheMock::populate_with_ephemeris_data(
    const std::vector<SolarSystem::JPL::EphemerisData>& data) {
  set_cache_data(data);

  // Create metadata for this data
  SolarSystem::JPL::CacheMetadata metadata;
  metadata.created_at = std::chrono::system_clock::now();
  metadata.epoch = std::chrono::system_clock::now();
  metadata.source = "TEST_EPHEMERIS";
  metadata.body_count = data.size();
  metadata.checksum = static_cast<uint64_t>(data.size() * 1000);  // Simple checksum

  set_cache_metadata(metadata);
  set_cache_exists(true);
  set_cache_valid(true);
}

/**
 * @brief Clear all cache data
 */
void CacheMock::clear_cache_data() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  cache_data_.clear();
  ephemeris_data_.clear();
  metadata_.reset();

  // Clear mock file system
  mock_files_.clear();
  file_exists_map_.clear();
  file_sizes_.clear();

  cache_exists_ = false;
  cache_valid_ = false;
  cache_corrupted_ = false;
}

/**
 * @brief Mock implementation of cache loading
 */
SolarSystem::JPL::JPLResult<std::vector<SolarSystem::JPL::EphemerisData>>
CacheMock::mock_load_from_cache() {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::Read, binary_cache_path_, 0, error);

    switch (error) {
      case MockCacheError::FileNotFound:
        return SolarSystem::JPL::JPLError::CacheError;
      case MockCacheError::CorruptedData:
        return SolarSystem::JPL::JPLError::ValidationError;
      case MockCacheError::PermissionDenied:
      case MockCacheError::IOError:
        return SolarSystem::JPL::JPLError::CacheError;
      default:
        return SolarSystem::JPL::JPLError::CacheError;
    }
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  if (!cache_exists_ || config_.cache_never_exists) {
    record_operation(MockCacheOperationType::Read, binary_cache_path_, 0,
                     MockCacheError::FileNotFound);
    return SolarSystem::JPL::JPLError::CacheError;
  }

  if (cache_corrupted_ || config_.cache_always_corrupted) {
    record_operation(MockCacheOperationType::Read, binary_cache_path_, cache_data_.size(),
                     MockCacheError::CorruptedData);
    return SolarSystem::JPL::JPLError::ValidationError;
  }

  if (!cache_valid_ || config_.cache_always_invalid) {
    record_operation(MockCacheOperationType::Read, binary_cache_path_, cache_data_.size(),
                     MockCacheError::ValidationFailed);
    return SolarSystem::JPL::JPLError::ValidationError;
  }

  // Return cached ephemeris data
  record_operation(MockCacheOperationType::Read, binary_cache_path_, cache_data_.size(),
                   MockCacheError::Success);
  return ephemeris_data_;
}

/**
 * @brief Mock implementation of cache saving
 */
SolarSystem::JPL::JPLVoidResult CacheMock::mock_save_to_cache(
    const std::vector<SolarSystem::JPL::EphemerisData>& data) {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::Write, binary_cache_path_, 0, error);

    switch (error) {
      case MockCacheError::DiskFull:
      case MockCacheError::PermissionDenied:
      case MockCacheError::IOError:
        return SolarSystem::JPL::error(SolarSystem::JPL::JPLError::CacheError);
      default:
        return SolarSystem::JPL::error(SolarSystem::JPL::JPLError::CacheError);
    }
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  // Store the data
  ephemeris_data_ = data;
  auto binary_data = ephemeris_data_to_binary(data);
  auto json_data = ephemeris_data_to_json(data);

  cache_data_ = binary_data;
  last_written_data_ = binary_data;

  // Update mock file system
  mock_files_[binary_cache_path_.string()] = binary_data;
  mock_files_[json_cache_path_.string()] = json_data;
  file_sizes_[binary_cache_path_.string()] = binary_data.size();
  file_sizes_[json_cache_path_.string()] = json_data.size();

  cache_exists_ = true;
  cache_valid_ = true;
  cache_corrupted_ = false;

  record_operation(MockCacheOperationType::Write, binary_cache_path_, binary_data.size(),
                   MockCacheError::Success);
  return SolarSystem::JPL::success();
}

/**
 * @brief Mock implementation of cache validation
 */
SolarSystem::JPL::JPLResult<bool> CacheMock::mock_validate_cache() const {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::Validate, binary_cache_path_, 0, error);
    return SolarSystem::JPL::JPLError::CacheError;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  bool is_valid = cache_exists_ && cache_valid_ && !cache_corrupted_ &&
                  !config_.cache_never_exists && !config_.cache_always_invalid &&
                  !config_.cache_always_corrupted;

  if (config_.cache_always_valid) {
    is_valid = true;
  }

  record_operation(MockCacheOperationType::Validate, binary_cache_path_, 0,
                   is_valid ? MockCacheError::Success : MockCacheError::ValidationFailed);

  return is_valid;
}

/**
 * @brief Mock implementation of cache clearing
 */
SolarSystem::JPL::JPLVoidResult CacheMock::mock_clear_cache() {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::Clear, cache_directory_, 0, error);
    return SolarSystem::JPL::error(SolarSystem::JPL::JPLError::CacheError);
  }

  clear_cache_data();
  record_operation(MockCacheOperationType::Clear, cache_directory_, 0, MockCacheError::Success);
  return SolarSystem::JPL::success();
}

/**
 * @brief Mock implementation of cache metadata loading
 */
std::optional<SolarSystem::JPL::CacheMetadata> CacheMock::mock_get_cache_metadata() const {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::MetadataRead, metadata_path_, 0, error);
    return std::nullopt;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  if (!cache_exists_ || config_.cache_never_exists) {
    record_operation(MockCacheOperationType::MetadataRead, metadata_path_, 0,
                     MockCacheError::FileNotFound);
    return std::nullopt;
  }

  record_operation(MockCacheOperationType::MetadataRead, metadata_path_, metadata_ ? 100 : 0,
                   MockCacheError::Success);
  return metadata_;
}

/**
 * @brief Mock implementation of cache rebuilding
 */
SolarSystem::JPL::JPLVoidResult CacheMock::mock_rebuild_cache() {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::Write, cache_directory_, 0, error);
    return SolarSystem::JPL::error(SolarSystem::JPL::JPLError::CacheError);
  }

  // Rebuild with fresh valid data
  populate_with_valid_data();
  record_operation(MockCacheOperationType::Write, cache_directory_, cache_data_.size(),
                   MockCacheError::Success);
  return SolarSystem::JPL::success();
}

/**
 * @brief Check if file exists (mock)
 */
bool CacheMock::mock_file_exists(const std::filesystem::path& path) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  auto it = file_exists_map_.find(path.string());
  if (it != file_exists_map_.end()) {
    return it->second;
  }

  // Default behavior based on cache state
  if (path == binary_cache_path_ || path == json_cache_path_ || path == metadata_path_) {
    return cache_exists_ && !config_.cache_never_exists;
  }

  return false;
}

/**
 * @brief Get file size (mock)
 */
std::optional<size_t> CacheMock::mock_file_size(const std::filesystem::path& path) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  if (!mock_file_exists(path)) {
    return std::nullopt;
  }

  auto it = file_sizes_.find(path.string());
  if (it != file_sizes_.end()) {
    return it->second;
  }

  // Default sizes
  if (path == binary_cache_path_) {
    return cache_data_.size();
  } else if (path == json_cache_path_) {
    return cache_data_.size() * 2;  // JSON is typically larger
  } else if (path == metadata_path_) {
    return 200;  // Typical metadata size
  }

  return 0;
}

/**
 * @brief Read file content (mock)
 */
std::optional<std::string> CacheMock::mock_read_file(const std::filesystem::path& path) {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::Read, path, 0, error);
    return std::nullopt;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  if (!mock_file_exists(path)) {
    record_operation(MockCacheOperationType::Read, path, 0, MockCacheError::FileNotFound);
    return std::nullopt;
  }

  auto it = mock_files_.find(path.string());
  if (it != mock_files_.end()) {
    record_operation(MockCacheOperationType::Read, path, it->second.size(),
                     MockCacheError::Success);
    return it->second;
  }

  record_operation(MockCacheOperationType::Read, path, 0, MockCacheError::FileNotFound);
  return std::nullopt;
}

/**
 * @brief Write file content (mock)
 */
bool CacheMock::mock_write_file(const std::filesystem::path& path, const std::string& content) {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::Write, path, 0, error);
    return false;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  // Check disk space
  if (content.size() > config_.max_cache_size) {
    record_operation(MockCacheOperationType::Write, path, 0, MockCacheError::DiskFull);
    return false;
  }

  mock_files_[path.string()] = content;
  file_exists_map_[path.string()] = true;
  file_sizes_[path.string()] = content.size();
  last_written_data_ = content;

  record_operation(MockCacheOperationType::Write, path, content.size(), MockCacheError::Success);
  return true;
}

/**
 * @brief Create directory (mock)
 */
bool CacheMock::mock_create_directory(const std::filesystem::path& path) {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    record_operation(MockCacheOperationType::DirectoryCreate, path, 0, error);
    return false;
  }

  record_operation(MockCacheOperationType::DirectoryCreate, path, 0, MockCacheError::Success);
  return true;
}

/**
 * @brief Remove file (mock)
 */
bool CacheMock::mock_remove_file(const std::filesystem::path& path) {
  simulate_operation_delay();

  auto error = check_operation_errors();
  if (error != MockCacheError::Success) {
    return false;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  mock_files_.erase(path.string());
  file_exists_map_[path.string()] = false;
  file_sizes_.erase(path.string());

  return true;
}

/**
 * @brief Check if cache was read
 */
bool CacheMock::was_cache_read() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_counts_.at(MockCacheOperationType::Read) > 0;
}

/**
 * @brief Check if cache was written
 */
bool CacheMock::was_cache_written() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_counts_.at(MockCacheOperationType::Write) > 0;
}

/**
 * @brief Check if cache was validated
 */
bool CacheMock::was_cache_validated() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_counts_.at(MockCacheOperationType::Validate) > 0;
}

/**
 * @brief Check if cache was cleared
 */
bool CacheMock::was_cache_cleared() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_counts_.at(MockCacheOperationType::Clear) > 0;
}

/**
 * @brief Get last written data
 */
std::string CacheMock::last_written_data() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return last_written_data_;
}

/**
 * @brief Get total number of operations
 */
size_t CacheMock::operation_count() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  size_t total = 0;
  for (const auto& [type, count] : operation_counts_) {
    total += count;
  }
  return total;
}

/**
 * @brief Get number of operations by type
 */
size_t CacheMock::operation_count(MockCacheOperationType type) const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  auto it = operation_counts_.find(type);
  return it != operation_counts_.end() ? it->second : 0;
}

/**
 * @brief Get complete operation history
 */
const std::vector<MockCacheCallInfo>& CacheMock::operation_history() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_history_;
}

/**
 * @brief Get operations of specific type
 */
std::vector<MockCacheCallInfo> CacheMock::operations_of_type(MockCacheOperationType type) const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  std::vector<MockCacheCallInfo> filtered;

  for (const auto& op : operation_history_) {
    if (op.operation_type == type) {
      filtered.push_back(op);
    }
  }

  return filtered;
}

/**
 * @brief Reset call history and counters
 */
void CacheMock::reset_call_history() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  operation_history_.clear();
  for (auto& [type, count] : operation_counts_) {
    count = 0;
  }
  last_written_data_.clear();
}

/**
 * @brief Update configuration
 */
void CacheMock::update_config(const CacheMockConfig& new_config) {
  std::lock_guard<std::mutex> lock(call_mutex_);
  config_ = new_config;
}

/**
 * @brief Get current cache directory path
 */
std::filesystem::path CacheMock::cache_directory() const { return cache_directory_; }

/**
 * @brief Get binary cache file path
 */
std::filesystem::path CacheMock::binary_cache_path() const { return binary_cache_path_; }

/**
 * @brief Get JSON cache file path
 */
std::filesystem::path CacheMock::json_cache_path() const { return json_cache_path_; }

/**
 * @brief Get metadata file path
 */
std::filesystem::path CacheMock::metadata_path() const { return metadata_path_; }

// Global mock instance for dependency injection
static std::unique_ptr<CacheMock> global_cache_mock = nullptr;
static std::mutex global_mock_mutex;

/**
 * @brief Install this mock as global cache mock
 */
void CacheMock::install_as_global_mock() {
  std::lock_guard<std::mutex> lock(global_mock_mutex);
  // Note: In a real implementation, this would integrate with the dependency injection system
  // For now, we just store the reference
}

/**
 * @brief Remove global mock installation
 */
void CacheMock::remove_global_mock() {
  std::lock_guard<std::mutex> lock(global_mock_mutex);
  global_cache_mock.reset();
}

/**
 * @brief Record a mock operation for verification
 */
void CacheMock::record_operation(MockCacheOperationType type, const std::filesystem::path& path,
                                 size_t data_size, MockCacheError result,
                                 const std::string& details) const {
  if (!config_.enable_call_history) {
    return;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  MockCacheCallInfo call_info;
  call_info.timestamp = std::chrono::system_clock::now();
  call_info.operation_type = type;
  call_info.file_path = path;
  call_info.data_size = data_size;
  call_info.result = result;
  call_info.operation_duration = std::chrono::milliseconds(1);  // Simulated duration
  call_info.operation_details = details;

  const_cast<CacheMock*>(this)->operation_history_.push_back(call_info);
  const_cast<CacheMock*>(this)->operation_counts_[type]++;

  // Limit history size
  if (operation_history_.size() > config_.max_history_size) {
    const_cast<CacheMock*>(this)->operation_history_.erase(operation_history_.begin());
  }
}

/**
 * @brief Determine if should simulate error
 */
bool CacheMock::should_simulate_error(double error_rate) const {
  return error_dist_(gen_) < error_rate;
}

/**
 * @brief Simulate operation delay
 */
void CacheMock::simulate_operation_delay() const {
  if (!config_.simulate_file_operations) {
    return;
  }

  auto delay = config_.min_operation_delay;
  if (config_.max_operation_delay > config_.min_operation_delay) {
    std::uniform_int_distribution<long long> delay_dist(config_.min_operation_delay.count(),
                                                        config_.max_operation_delay.count());
    delay = std::chrono::milliseconds(delay_dist(gen_));
  }

  // Add slow disk delay if simulated
  if (slow_disk_delay_ > std::chrono::milliseconds(0)) {
    delay += slow_disk_delay_;
  }

  if (delay > std::chrono::milliseconds(0)) {
    std::this_thread::sleep_for(delay);
  }
}

/**
 * @brief Check if operation should fail
 */
MockCacheError CacheMock::check_operation_errors() const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  // Check for simulated errors
  if (remaining_io_errors_ > 0) {
    const_cast<CacheMock*>(this)->remaining_io_errors_--;
    return MockCacheError::IOError;
  }

  if (disk_full_simulated_) {
    return MockCacheError::DiskFull;
  }

  if (permission_denied_simulated_) {
    return MockCacheError::PermissionDenied;
  }

  // Check random error rates
  if (should_simulate_error(config_.disk_full_rate)) {
    return MockCacheError::DiskFull;
  }

  if (should_simulate_error(config_.corruption_rate)) {
    return MockCacheError::CorruptedData;
  }

  if (should_simulate_error(config_.permission_error_rate)) {
    return MockCacheError::PermissionDenied;
  }

  if (should_simulate_error(config_.io_error_rate)) {
    return MockCacheError::IOError;
  }

  return MockCacheError::Success;
}

/**
 * @brief Generate corrupted data
 */
std::string CacheMock::generate_corrupted_data(const std::string& original_data,
                                               double corruption_rate) const {
  if (original_data.empty()) {
    return original_data;
  }

  std::string corrupted = original_data;
  std::uniform_int_distribution<size_t> pos_dist(0, corrupted.size() - 1);
  std::uniform_int_distribution<int> byte_dist(0, 255);

  size_t corruption_count = static_cast<size_t>(corrupted.size() * corruption_rate);

  for (size_t i = 0; i < corruption_count; ++i) {
    size_t pos = pos_dist(gen_);
    corrupted[pos] = static_cast<char>(byte_dist(gen_));
  }

  return corrupted;
}

/**
 * @brief Generate valid test ephemeris data
 */
std::vector<SolarSystem::JPL::EphemerisData> CacheMock::generate_valid_test_data() const {
  std::vector<SolarSystem::JPL::EphemerisData> data;

  // Generate test data for a few common bodies
  std::vector<int> test_jpl_ids = {10, 399, 301, 499, 599};  // Sun, Earth, Moon, Mars, Jupiter

  for (int jpl_id : test_jpl_ids) {
    SolarSystem::JPL::EphemerisData body_data;
    body_data.jpl_id = jpl_id;
    body_data.epoch = std::chrono::system_clock::now();

    // Generate realistic body names
    switch (jpl_id) {
      case 10:
        body_data.body_name = "Sun";
        break;
      case 399:
        body_data.body_name = "Earth";
        break;
      case 301:
        body_data.body_name = "Moon";
        break;
      case 499:
        body_data.body_name = "Mars";
        break;
      case 599:
        body_data.body_name = "Jupiter";
        break;
      default:
        body_data.body_name = "Body_" + std::to_string(jpl_id);
        break;
    }

    // Generate realistic positions (in km)
    std::uniform_real_distribution<double> pos_dist(-1e9, 1e9);
    body_data.position =
        SolarSystem::Math::Vector3d{pos_dist(gen_), pos_dist(gen_), pos_dist(gen_)};

    // Generate realistic velocities (in km/s)
    std::uniform_real_distribution<double> vel_dist(-50.0, 50.0);
    body_data.velocity =
        SolarSystem::Math::Vector3d{vel_dist(gen_), vel_dist(gen_), vel_dist(gen_)};

    // Set realistic masses
    switch (jpl_id) {
      case 10:
        body_data.mass = 1.98847e30;
        break;  // Sun
      case 399:
        body_data.mass = 5.97219e24;
        break;  // Earth
      case 301:
        body_data.mass = 7.342e22;
        break;  // Moon
      case 499:
        body_data.mass = 6.4171e23;
        break;  // Mars
      case 599:
        body_data.mass = 1.8982e27;
        break;  // Jupiter
      default:
        body_data.mass = 1.0e20;
        break;  // Default
    }

    data.push_back(body_data);
  }

  return data;
}

/**
 * @brief Convert ephemeris data to binary format
 */
std::string CacheMock::ephemeris_data_to_binary(
    const std::vector<SolarSystem::JPL::EphemerisData>& data) const {
  std::ostringstream binary_stream(std::ios::binary);

  // Write number of bodies
  size_t body_count = data.size();
  binary_stream.write(reinterpret_cast<const char*>(&body_count), sizeof(body_count));

  // Write each body's data
  for (const auto& body_data : data) {
    // Write JPL ID
    binary_stream.write(reinterpret_cast<const char*>(&body_data.jpl_id), sizeof(body_data.jpl_id));

    // Write body name length and name
    size_t name_length = body_data.body_name.length();
    binary_stream.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
    binary_stream.write(body_data.body_name.c_str(), static_cast<std::streamsize>(name_length));

    // Write epoch
    auto epoch_time = std::chrono::system_clock::to_time_t(body_data.epoch);
    binary_stream.write(reinterpret_cast<const char*>(&epoch_time), sizeof(epoch_time));

    // Write position
    double pos[3] = {static_cast<double>(body_data.position.x()),
                     static_cast<double>(body_data.position.y()),
                     static_cast<double>(body_data.position.z())};
    binary_stream.write(reinterpret_cast<const char*>(pos), sizeof(pos));

    // Write velocity
    double vel[3] = {static_cast<double>(body_data.velocity.x()),
                     static_cast<double>(body_data.velocity.y()),
                     static_cast<double>(body_data.velocity.z())};
    binary_stream.write(reinterpret_cast<const char*>(vel), sizeof(vel));

    // Write mass
    binary_stream.write(reinterpret_cast<const char*>(&body_data.mass), sizeof(body_data.mass));
  }

  return binary_stream.str();
}

/**
 * @brief Convert ephemeris data to JSON format
 */
std::string CacheMock::ephemeris_data_to_json(
    const std::vector<SolarSystem::JPL::EphemerisData>& data) const {
  std::ostringstream json_stream;

  json_stream << "{\n";
  json_stream << "  \"metadata\": {\n";
  json_stream << "    \"created_at\": "
              << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << ",\n";
  json_stream << "    \"body_count\": " << data.size() << ",\n";
  json_stream << "    \"source\": \"TEST_MOCK\"\n";
  json_stream << "  },\n";
  json_stream << "  \"bodies\": [\n";

  for (size_t i = 0; i < data.size(); ++i) {
    const auto& body_data = data[i];

    json_stream << "    {\n";
    json_stream << "      \"jpl_id\": " << body_data.jpl_id << ",\n";
    json_stream << "      \"body_name\": \"" << body_data.body_name << "\",\n";
    json_stream << "      \"epoch\": " << std::chrono::system_clock::to_time_t(body_data.epoch)
                << ",\n";
    json_stream << "      \"position\": [" << std::scientific << std::setprecision(15)
                << body_data.position.x() << ", " << body_data.position.y() << ", "
                << body_data.position.z() << "],\n";
    json_stream << "      \"velocity\": [" << std::scientific << std::setprecision(15)
                << body_data.velocity.x() << ", " << body_data.velocity.y() << ", "
                << body_data.velocity.z() << "],\n";
    json_stream << "      \"mass\": " << std::scientific << std::setprecision(15) << body_data.mass
                << "\n";
    json_stream << "    }";

    if (i < data.size() - 1) {
      json_stream << ",";
    }
    json_stream << "\n";
  }

  json_stream << "  ]\n";
  json_stream << "}\n";

  return json_stream.str();
}

/**
 * @brief Generate metadata JSON
 */
std::string CacheMock::generate_metadata_json(
    const SolarSystem::JPL::CacheMetadata& metadata) const {
  std::ostringstream json_stream;

  json_stream << "{\n";
  json_stream << "  \"created_at\": " << std::chrono::system_clock::to_time_t(metadata.created_at)
              << ",\n";
  json_stream << "  \"epoch\": " << std::chrono::system_clock::to_time_t(metadata.epoch) << ",\n";
  json_stream << "  \"source\": \"" << metadata.source << "\",\n";
  json_stream << "  \"body_count\": " << metadata.body_count << ",\n";
  json_stream << "  \"checksum\": " << metadata.checksum << "\n";
  json_stream << "}\n";

  return json_stream.str();
}

// TemporaryCache implementation

/**
 * @brief TemporaryCache constructor
 */
TemporaryCache::TemporaryCache(const std::string& cache_type)
    : cache_type_(cache_type), cleanup_on_destroy_(true) {
  // Create temporary directory
  auto temp_path = std::filesystem::temp_directory_path();
  temp_directory_ =
      temp_path / ("solar_test_cache_" + cache_type + "_" +
                   std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));

  cache_directory_ = temp_directory_ / "cache";

  create_directory_structure();
}

/**
 * @brief TemporaryCache destructor
 */
TemporaryCache::~TemporaryCache() {
  if (cleanup_on_destroy_ && std::filesystem::exists(temp_directory_)) {
    std::error_code ec;
    std::filesystem::remove_all(temp_directory_, ec);
    // Ignore errors during cleanup
  }
}

/**
 * @brief Populate cache with valid data
 */
void TemporaryCache::populate_with_valid_data() {
  auto test_data = generate_test_data();
  write_binary_cache(test_data);
  write_json_cache(test_data);
  write_metadata(test_data);
}

/**
 * @brief Populate cache with corrupted data
 */
void TemporaryCache::populate_with_corrupted_data() {
  populate_with_valid_data();

  // Corrupt the binary cache file
  corrupt_file(binary_cache_file(), 0.3);
}

/**
 * @brief Simulate partial corruption
 */
void TemporaryCache::simulate_partial_corruption(double corruption_percentage) {
  if (cache_files_exist()) {
    corrupt_file(binary_cache_file(), corruption_percentage);
    corrupt_file(json_cache_file(), corruption_percentage * 0.5);  // Less corruption in JSON
  }
}

/**
 * @brief Get cache directory path
 */
std::filesystem::path TemporaryCache::cache_path() const { return cache_directory_; }

/**
 * @brief Get binary cache file path
 */
std::filesystem::path TemporaryCache::binary_cache_file() const {
  return cache_directory_ / "ephemeris_cache.bin";
}

/**
 * @brief Get JSON cache file path
 */
std::filesystem::path TemporaryCache::json_cache_file() const {
  return cache_directory_ / "ephemeris_data.json";
}

/**
 * @brief Get metadata file path
 */
std::filesystem::path TemporaryCache::metadata_file() const {
  return cache_directory_ / "metadata.json";
}

/**
 * @brief Check if cache files exist
 */
bool TemporaryCache::cache_files_exist() const {
  return std::filesystem::exists(binary_cache_file()) || std::filesystem::exists(json_cache_file());
}

/**
 * @brief Get cache file sizes
 */
std::map<std::string, size_t> TemporaryCache::cache_file_sizes() const {
  std::map<std::string, size_t> sizes;

  std::error_code ec;
  if (std::filesystem::exists(binary_cache_file())) {
    sizes["binary"] = std::filesystem::file_size(binary_cache_file(), ec);
  }
  if (std::filesystem::exists(json_cache_file())) {
    sizes["json"] = std::filesystem::file_size(json_cache_file(), ec);
  }
  if (std::filesystem::exists(metadata_file())) {
    sizes["metadata"] = std::filesystem::file_size(metadata_file(), ec);
  }

  return sizes;
}

/**
 * @brief Create JPL client config using this cache
 */
SolarSystem::JPL::JPLClientConfig TemporaryCache::create_client_config() const {
  SolarSystem::JPL::JPLClientConfig config;
  config.cache_directory = cache_directory_;
  config.enable_binary_cache = true;
  config.enable_json_cache = true;
  return config;
}

/**
 * @brief Create directory structure
 */
void TemporaryCache::create_directory_structure() {
  std::filesystem::create_directories(cache_directory_);
}

/**
 * @brief Generate test ephemeris data
 */
std::vector<SolarSystem::JPL::EphemerisData> TemporaryCache::generate_test_data() const {
  // Use the same generation logic as CacheMock
  CacheMock mock;
  return mock.generate_valid_test_data();
}

/**
 * @brief Write binary cache file
 */
void TemporaryCache::write_binary_cache(const std::vector<SolarSystem::JPL::EphemerisData>& data) {
  CacheMock mock;
  auto binary_data = mock.ephemeris_data_to_binary(data);

  std::ofstream file(binary_cache_file(), std::ios::binary);
  if (file.is_open()) {
    file.write(binary_data.c_str(), static_cast<std::streamsize>(binary_data.size()));
  }
}

/**
 * @brief Write JSON cache file
 */
void TemporaryCache::write_json_cache(const std::vector<SolarSystem::JPL::EphemerisData>& data) {
  CacheMock mock;
  auto json_data = mock.ephemeris_data_to_json(data);

  std::ofstream file(json_cache_file());
  if (file.is_open()) {
    file << json_data;
  }
}

/**
 * @brief Write metadata file
 */
void TemporaryCache::write_metadata(const std::vector<SolarSystem::JPL::EphemerisData>& data) {
  SolarSystem::JPL::CacheMetadata metadata;
  metadata.created_at = std::chrono::system_clock::now();
  metadata.epoch = std::chrono::system_clock::now();
  metadata.source = "TEMPORARY_TEST";
  metadata.body_count = data.size();
  metadata.checksum = static_cast<uint64_t>(data.size() * 12345);

  CacheMock mock;
  auto metadata_json = mock.generate_metadata_json(metadata);

  std::ofstream file(metadata_file());
  if (file.is_open()) {
    file << metadata_json;
  }
}

/**
 * @brief Corrupt file content
 */
void TemporaryCache::corrupt_file(const std::filesystem::path& file_path, double corruption_rate) {
  if (!std::filesystem::exists(file_path)) {
    return;
  }

  // Read file content
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) {
    return;
  }

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  // Corrupt content
  CacheMock mock;
  auto corrupted_content = mock.generate_corrupted_data(content, corruption_rate);

  // Write back corrupted content
  std::ofstream out_file(file_path, std::ios::binary);
  if (out_file.is_open()) {
    out_file.write(corrupted_content.c_str(),
                   static_cast<std::streamsize>(corrupted_content.size()));
  }
}

// Factory implementations

/**
 * @brief Create default cache mock
 */
std::unique_ptr<CacheMock> CacheMockFactory::create_default() {
  return std::make_unique<CacheMock>();
}

/**
 * @brief Create cache mock with custom configuration
 */
std::unique_ptr<CacheMock> CacheMockFactory::create(CacheMockConfig config) {
  return std::make_unique<CacheMock>(std::move(config));
}

/**
 * @brief Create cache mock for corruption testing
 */
std::unique_ptr<CacheMock> CacheMockFactory::create_for_corruption_testing() {
  CacheMockConfig config;
  config.corruption_rate = 0.5;
  config.cache_always_corrupted = true;
  return std::make_unique<CacheMock>(config);
}

/**
 * @brief Create cache mock for disk full testing
 */
std::unique_ptr<CacheMock> CacheMockFactory::create_for_disk_full_testing() {
  CacheMockConfig config;
  config.disk_full_rate = 1.0;
  config.max_cache_size = 1024;  // Very small cache size
  return std::make_unique<CacheMock>(config);
}

/**
 * @brief Create cache mock for performance testing
 */
std::unique_ptr<CacheMock> CacheMockFactory::create_for_performance_testing() {
  CacheMockConfig config;
  config.simulate_file_operations = true;
  config.min_operation_delay =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(1));
  config.max_operation_delay =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(10));
  config.simulate_slow_disk = false;
  return std::make_unique<CacheMock>(config);
}

/**
 * @brief Create cache mock with always valid cache
 */
std::unique_ptr<CacheMock> CacheMockFactory::create_with_valid_cache() {
  CacheMockConfig config;
  config.cache_always_exists = true;
  config.cache_always_valid = true;
  return std::make_unique<CacheMock>(config);
}

/**
 * @brief Create cache mock with always invalid cache
 */
std::unique_ptr<CacheMock> CacheMockFactory::create_with_invalid_cache() {
  CacheMockConfig config;
  config.cache_always_invalid = true;
  config.cache_never_exists = true;
  return std::make_unique<CacheMock>(config);
}

// ScopedCacheMock implementation

/**
 * @brief ScopedCacheMock constructor
 */
ScopedCacheMock::ScopedCacheMock(std::unique_ptr<CacheMock> mock) : mock_(std::move(mock)) {
  if (mock_) {
    mock_->install_as_global_mock();
  }
}

/**
 * @brief ScopedCacheMock destructor
 */
ScopedCacheMock::~ScopedCacheMock() {
  if (mock_) {
    CacheMock::remove_global_mock();
  }
}

}  // namespace SolarSystem::Testing::Mocks
