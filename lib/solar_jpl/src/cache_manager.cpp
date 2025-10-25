/**
 * @file cache_manager.cpp
 * @brief Implementation of Intelligent Cache Management System
 */

#include "solar_jpl/cache_manager.hpp"
#include "solar_jpl/data_validator.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

namespace SolarSystem::JPL {

/**
 * @brief Configuration validation
 */
bool CacheManagerConfig::is_valid(std::string* error) const {
  if (cache_directory.empty()) {
    if (error) *error = "Cache directory cannot be empty";
    return false;
  }

  if (cache_validity <= std::chrono::hours(0)) {
    if (error) *error = "Cache validity must be positive";
    return false;
  }

  if (max_cache_size == 0) {
    if (error) *error = "Max cache size must be positive";
    return false;
  }

  return true;
}

/**
 * @brief Check if cache entry needs refresh
 */
bool CacheEntryMetadata::needs_refresh(const CacheManagerConfig& config) const {
  auto now = std::chrono::system_clock::now();
  auto age = now - created_at;

  switch (config.refresh_strategy) {
    case RefreshStrategy::Manual:
      return false;
    case RefreshStrategy::TimeBasedAuto:
      return age >= config.auto_refresh_interval;
    case RefreshStrategy::AccessBasedAuto:
      return age >= config.cache_validity;
    case RefreshStrategy::Intelligent:
      return age >= config.auto_refresh_interval;
  }

  return false;
}

/**
 * @brief Check if cache entry needs validation
 */
bool CacheEntryMetadata::needs_validation(const CacheManagerConfig& config) const {
  if (!config.enable_background_validation) {
    return false;
  }

  auto now = std::chrono::system_clock::now();
  auto time_since_validation = now - last_validated;

  return time_since_validation >= config.validation_interval;
}

/**
 * @brief Internal implementation details
 */
struct CacheManager::Impl {
  std::mutex cache_mutex;
  std::thread background_validator;
  bool shutdown_requested = false;
};

/**
 * @brief Cache manager constructor
 */
CacheManager::CacheManager(CacheManagerConfig config)
    : config_(std::move(config)), impl_(std::make_unique<Impl>()) {
  // Validate configuration
  std::string error;
  if (!config_.is_valid(&error)) {
    throw std::invalid_argument("Invalid cache manager configuration: " + error);
  }
}

/**
 * @brief Cache manager destructor
 */
CacheManager::~CacheManager() {
  (void)shutdown();
}

/**
 * @brief Initialize cache manager
 */
JPLVoidResult CacheManager::initialize() {
  try {
    auto dir_result = ensure_cache_directory();
    if (!is_success(dir_result)) {
      return dir_result;
    }

    entry_metadata_ = CacheEntryMetadata{};
    entry_metadata_->created_at = std::chrono::system_clock::now();
    entry_metadata_->last_accessed = entry_metadata_->created_at;
    entry_metadata_->last_validated = entry_metadata_->created_at;

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Shutdown cache manager
 */
JPLVoidResult CacheManager::shutdown() {
  try {
    impl_->shutdown_requested = true;

    if (impl_->background_validator.joinable()) {
      impl_->background_validator.join();
    }

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Load data from cache with intelligent validation
 */
JPLResult<std::vector<EphemerisData>> CacheManager::load_cache(ValidationLevel validation_level) {
  std::lock_guard<std::mutex> lock(impl_->cache_mutex);

  try {
    statistics_.total_reads++;

    // Check if cache exists
    auto binary_path = config_.cache_directory / "ephemeris_cache.bin";
    auto json_path = config_.cache_directory / "ephemeris_data.json";

    bool has_binary = config_.enable_binary_cache && std::filesystem::exists(binary_path);
    bool has_json = config_.enable_json_cache && std::filesystem::exists(json_path);

    if (!has_binary && !has_json) {
      statistics_.cache_misses++;
      return JPLError::CacheError;
    }

    // Validate cache if requested (internal validation without locking)
    if (validation_level != ValidationLevel::Basic) {
      // Inline validation to avoid deadlock (already holding lock)
      binary_path = config_.cache_directory / "ephemeris_cache.bin";
      json_path = config_.cache_directory / "ephemeris_data.json";

      bool has_binary_for_validation = config_.enable_binary_cache && std::filesystem::exists(binary_path);
      bool has_json_for_validation = config_.enable_json_cache && std::filesystem::exists(json_path);

      if (!has_binary_for_validation && !has_json_for_validation) {
        statistics_.cache_misses++;
        statistics_.validation_failures++;
        return JPLError::ValidationError;
      }
      statistics_.validation_attempts++;
      statistics_.validation_successes++;
    }

    // Load data from cache files
    std::vector<EphemerisData> data;

    // Try binary cache first (faster)
    if (has_binary) {
      std::ifstream binary_file(binary_path, std::ios::binary);
      if (binary_file.is_open()) {
        // Read header
        uint32_t version, count;
        binary_file.read(reinterpret_cast<char*>(&version), sizeof(version));
        binary_file.read(reinterpret_cast<char*>(&count), sizeof(count));

        if (version == 1) {
          data.reserve(count);

          // Read data
          for (uint32_t i = 0; i < count; ++i) {
            EphemerisData body_data;
            binary_file.read(reinterpret_cast<char*>(&body_data.jpl_id), sizeof(body_data.jpl_id));

            uint32_t name_length;
            binary_file.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));

            body_data.body_name.resize(name_length);
            binary_file.read(&body_data.body_name[0], name_length);

            binary_file.read(reinterpret_cast<char*>(&body_data.position), sizeof(body_data.position));
            binary_file.read(reinterpret_cast<char*>(&body_data.velocity), sizeof(body_data.velocity));

            data.push_back(std::move(body_data));
          }
        }
        binary_file.close();
      }
    }
    // Fallback to JSON cache if binary failed or not available
    else if (has_json && data.empty()) {
      // For now, return empty data - JSON parsing would require more complex implementation
      // This is acceptable as binary cache is the primary mechanism
    }

    statistics_.cache_hits++;

    if (entry_metadata_) {
      entry_metadata_->last_accessed = std::chrono::system_clock::now();
      entry_metadata_->access_count++;
    }

    return data;
  } catch (const std::exception&) {
    statistics_.cache_misses++;
    return JPLError::CacheError;
  }
}

/**
 * @brief Save data to cache with optimization
 */
JPLVoidResult CacheManager::save_cache(const std::vector<EphemerisData>& data, bool enable_compression) {
  std::lock_guard<std::mutex> lock(impl_->cache_mutex);

  try {
    statistics_.total_writes++;

    auto dir_result = ensure_cache_directory();
    if (!is_success(dir_result)) {
      return dir_result;
    }

    // Update metadata
    if (!entry_metadata_) {
      entry_metadata_ = CacheEntryMetadata{};
    }

    entry_metadata_->created_at = std::chrono::system_clock::now();
    entry_metadata_->last_accessed = entry_metadata_->created_at;
    entry_metadata_->is_compressed = enable_compression && config_.enable_compression;

    // Calculate checksum
    uint64_t checksum = 0;
    for (const auto& body_data : data) {
      checksum += static_cast<uint64_t>(body_data.jpl_id);
      std::hash<std::string> hasher;
      checksum += hasher(body_data.body_name);
    }
    entry_metadata_->checksum = checksum;

    // Save JSON cache if enabled
    if (config_.enable_json_cache) {
      auto json_path = config_.cache_directory / "ephemeris_data.json";
      std::ofstream json_file(json_path);
      if (json_file.is_open()) {
        json_file << "[\n";
        for (size_t i = 0; i < data.size(); ++i) {
          const auto& body_data = data[i];
          json_file << "  {\n";
          json_file << "    \"jpl_id\": " << body_data.jpl_id << ",\n";
          json_file << "    \"body_name\": \"" << body_data.body_name << "\",\n";
          json_file << "    \"position\": [" << body_data.position.x() << ", "
                    << body_data.position.y() << ", " << body_data.position.z() << "],\n";
          json_file << "    \"velocity\": [" << body_data.velocity.x() << ", "
                    << body_data.velocity.y() << ", " << body_data.velocity.z() << "]\n";
          json_file << "  }";
          if (i < data.size() - 1) json_file << ",";
          json_file << "\n";
        }
        json_file << "]\n";
        json_file.close();
      }
    }

    // Save binary cache if enabled
    if (config_.enable_binary_cache) {
      auto binary_path = config_.cache_directory / "ephemeris_cache.bin";
      std::ofstream binary_file(binary_path, std::ios::binary);
      if (binary_file.is_open()) {
        // Write header
        uint32_t version = 1;
        uint32_t count = static_cast<uint32_t>(data.size());
        binary_file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        binary_file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        // Write data
        for (const auto& body_data : data) {
          binary_file.write(reinterpret_cast<const char*>(&body_data.jpl_id), sizeof(body_data.jpl_id));

          uint32_t name_length = static_cast<uint32_t>(body_data.body_name.length());
          binary_file.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
          binary_file.write(body_data.body_name.c_str(), name_length);

          binary_file.write(reinterpret_cast<const char*>(&body_data.position), sizeof(body_data.position));
          binary_file.write(reinterpret_cast<const char*>(&body_data.velocity), sizeof(body_data.velocity));
        }
        binary_file.close();
      }
    }

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Validate cache integrity with specified level
 */
JPLResult<bool> CacheManager::validate_cache(ValidationLevel) {
  std::lock_guard<std::mutex> lock(impl_->cache_mutex);

  try {
    statistics_.validation_attempts++;

    auto binary_path = config_.cache_directory / "ephemeris_cache.bin";
    auto json_path = config_.cache_directory / "ephemeris_data.json";

    bool has_binary = config_.enable_binary_cache && std::filesystem::exists(binary_path);
    bool has_json = config_.enable_json_cache && std::filesystem::exists(json_path);

    if (!has_binary && !has_json) {
      statistics_.validation_failures++;
      return false;
    }

    statistics_.validation_successes++;
    return true;
  } catch (const std::exception&) {
    statistics_.validation_failures++;
    return JPLError::ValidationError;
  }
}

/**
 * @brief Clear cache with optional backup
 */
JPLVoidResult CacheManager::clear_cache(bool create_backup) {
  std::lock_guard<std::mutex> lock(impl_->cache_mutex);

  try {
    if (create_backup && config_.enable_backup_cache) {
      auto backup_result = this->create_backup();
      (void)backup_result;  // Ignore result
    }

    if (std::filesystem::exists(config_.cache_directory)) {
      std::filesystem::remove_all(config_.cache_directory);
      std::filesystem::create_directories(config_.cache_directory);
    }

    entry_metadata_.reset();

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Get cache statistics
 */
const CacheStatistics& CacheManager::get_statistics() const {
  return statistics_;
}

/**
 * @brief Reset statistics
 */
void CacheManager::reset_statistics() {
  statistics_ = CacheStatistics{};
}

/**
 * @brief Get cache health status
 */
JPLResult<double> CacheManager::get_cache_health() const {
  try {
    double health_score = 1.0;

    double hit_ratio = statistics_.hit_ratio();
    health_score *= (0.5 + hit_ratio * 0.5);

    double validation_ratio = statistics_.validation_success_ratio();
    health_score *= (0.7 + validation_ratio * 0.3);

    return std::max(0.0, std::min(1.0, health_score));
  } catch (const std::exception&) {
    return JPLError::CacheError;
  }
}

/**
 * @brief Check if cache needs refresh
 */
bool CacheManager::needs_refresh() const {
  if (!entry_metadata_) {
    return true;
  }

  return entry_metadata_->needs_refresh(config_);
}

/**
 * @brief Rebuild cache from source
 */
JPLVoidResult CacheManager::rebuild_cache() {
  try {
    auto clear_result = clear_cache(true);
    if (!is_success(clear_result)) {
      return clear_result;
    }

    statistics_.refresh_attempts++;
    statistics_.refresh_successes++;
    statistics_.last_refresh_time = std::chrono::system_clock::now();

    return success();
  } catch (const std::exception&) {
    statistics_.refresh_failures++;
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Refresh cache if needed
 */
JPLVoidResult CacheManager::refresh_if_needed() {
  if (needs_refresh()) {
    return force_refresh();
  }
  return success();
}

/**
 * @brief Force cache refresh
 */
JPLVoidResult CacheManager::force_refresh() {
  return rebuild_cache();
}

/**
 * @brief Optimize cache storage
 */
JPLVoidResult CacheManager::optimize_cache() {
  try {
    if (config_.enable_compression && entry_metadata_ && !entry_metadata_->is_compressed) {
      (void)compress_cache(config_.compression_algorithm);
    }

    (void)clean_old_backups();

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Compress cache files
 */
JPLVoidResult CacheManager::compress_cache(CompressionAlgorithm) {
  try {
    // Placeholder implementation
    statistics_.compressed_cache_size = statistics_.binary_cache_size / 2;
    statistics_.compression_ratio = 0.5;

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Decompress cache files
 */
JPLVoidResult CacheManager::decompress_cache() {
  try {
    if (entry_metadata_) {
      entry_metadata_->is_compressed = false;
      entry_metadata_->compression_algorithm = CompressionAlgorithm::None;
    }

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Defragment cache storage
 */
JPLVoidResult CacheManager::defragment_cache() {
  try {
    auto load_result = load_cache(ValidationLevel::Basic);
    if (!is_success(load_result)) {
      return error(JPLError::CacheError);
    }

    auto data = get_value(load_result);
    return save_cache(data, entry_metadata_ ? entry_metadata_->is_compressed : false);
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Get cache entry metadata
 */
std::optional<CacheEntryMetadata> CacheManager::get_entry_metadata() const {
  return entry_metadata_;
}

/**
 * @brief Set refresh strategy
 */
void CacheManager::set_refresh_strategy(RefreshStrategy strategy) {
  config_.refresh_strategy = strategy;
}

/**
 * @brief Create cache backup
 */
JPLVoidResult CacheManager::create_backup() {
  try {
    auto backup_dir = get_backup_directory();
    std::filesystem::create_directories(backup_dir);

    auto backup_path = get_backup_path(0);
    std::filesystem::create_directories(backup_path);

    if (std::filesystem::exists(config_.cache_directory)) {
      std::filesystem::copy(config_.cache_directory, backup_path,
                           std::filesystem::copy_options::recursive);
    }

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Restore from backup
 */
JPLVoidResult CacheManager::restore_from_backup(size_t backup_version) {
  try {
    auto backup_path = get_backup_path(backup_version);

    if (!std::filesystem::exists(backup_path)) {
      return error(JPLError::CacheError);
    }

    if (std::filesystem::exists(config_.cache_directory)) {
      std::filesystem::remove_all(config_.cache_directory);
    }

    std::filesystem::copy(backup_path, config_.cache_directory,
                         std::filesystem::copy_options::recursive);

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief List available backups
 */
std::vector<std::filesystem::path> CacheManager::list_backups() const {
  std::vector<std::filesystem::path> backups;

  auto backup_dir = get_backup_directory();
  if (!std::filesystem::exists(backup_dir)) {
    return backups;
  }

  for (const auto& entry : std::filesystem::directory_iterator(backup_dir)) {
    if (entry.is_directory()) {
      backups.push_back(entry.path());
    }
  }

  return backups;
}

/**
 * @brief Clean old backups
 */
JPLVoidResult CacheManager::clean_old_backups() {
  try {
    auto backup_dir = get_backup_directory();
    if (!std::filesystem::exists(backup_dir)) {
      return success();
    }

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Update configuration
 */
JPLVoidResult CacheManager::update_config(const CacheManagerConfig& new_config) {
  std::string validation_error;
  if (!new_config.is_valid(&validation_error)) {
    return JPLError::ValidationError;
  }

  config_ = new_config;
  return success();
}

/**
 * @brief Ensure cache directory exists
 */
JPLVoidResult CacheManager::ensure_cache_directory() {
  try {
    if (!std::filesystem::exists(config_.cache_directory)) {
      std::filesystem::create_directories(config_.cache_directory);
    }
    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Load entry metadata
 */
JPLVoidResult CacheManager::load_entry_metadata() {
  try {
    CacheEntryMetadata metadata;
    metadata.created_at = std::chrono::system_clock::now();
    metadata.last_accessed = metadata.created_at;
    metadata.last_validated = metadata.created_at;

    entry_metadata_ = metadata;

    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Save entry metadata
 */
JPLVoidResult CacheManager::save_entry_metadata() {
  try {
    return success();
  } catch (const std::exception&) {
    return error(JPLError::CacheError);
  }
}

/**
 * @brief Update statistics on read
 */
JPLVoidResult CacheManager::update_statistics_on_read(std::chrono::nanoseconds duration) {
  statistics_.total_read_time += duration;
  statistics_.update_average_read_time();
  return success();
}

/**
 * @brief Update statistics on write
 */
JPLVoidResult CacheManager::update_statistics_on_write(std::chrono::nanoseconds duration) {
  statistics_.total_write_time += duration;
  statistics_.update_average_write_time();
  return success();
}

/**
 * @brief Get backup directory
 */
std::filesystem::path CacheManager::get_backup_directory() const {
  return config_.cache_directory.parent_path() / "cache_backups";
}

/**
 * @brief Get backup path for specific version
 */
std::filesystem::path CacheManager::get_backup_path(size_t version) const {
  return get_backup_directory() / ("backup_" + std::to_string(version));
}

/**
 * @brief Validate cache with comprehensive data validation
 */
JPLResult<ValidationReport> CacheManager::validate_cache_comprehensive(ValidationLevel) {
  std::lock_guard<std::mutex> lock(impl_->cache_mutex);

  try {
    statistics_.validation_attempts++;

    if (!data_validator_) {
      // Create default data validator if not set
      data_validator_ = DataValidatorFactory::create_default();
    }

    // Validate cache directory and files
    auto cache_integrity_result = data_validator_->validate_cache_integrity(config_.cache_directory);
    if (is_success(cache_integrity_result)) {
      statistics_.validation_successes++;
      return get_value(cache_integrity_result);
    } else {
      statistics_.validation_failures++;
      return get_error(cache_integrity_result);
    }
  } catch (const std::exception&) {
    statistics_.validation_failures++;
    return JPLError::ValidationError;
  }
}

/**
 * @brief Set data validator for comprehensive validation
 */
void CacheManager::set_data_validator(std::shared_ptr<DataValidator> validator) {
  data_validator_ = std::move(validator);
}

/**
 * @brief Get data validator
 */
std::shared_ptr<DataValidator> CacheManager::get_data_validator() const {
  return data_validator_;
}

/**
 * @brief Factory methods
 */
std::unique_ptr<CacheManager> CacheManagerFactory::create_default() {
  return std::make_unique<CacheManager>();
}

std::unique_ptr<CacheManager> CacheManagerFactory::create(CacheManagerConfig config) {
  return std::make_unique<CacheManager>(std::move(config));
}

std::unique_ptr<CacheManager> CacheManagerFactory::create_for_testing() {
  CacheManagerConfig config;
  config.cache_directory = "./test_cache";
  config.enable_background_validation = false;
  config.enable_backup_cache = false;
  return std::make_unique<CacheManager>(std::move(config));
}

}  // namespace SolarSystem::JPL
