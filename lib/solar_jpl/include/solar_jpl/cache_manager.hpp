/**
 * @file cache_manager.hpp
 * @brief Intelligent Cache Management System for JPL HORIZONS Data
 *
 * Provides comprehensive cache management with:
 * - Multi-level cache validation and integrity checking
 * - Cache expiration and refresh strategies
 * - Cache optimization and compression
 * - Cache statistics and monitoring
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "jpl_client.hpp"
#include "solar_jpl/export.hpp"

// Forward declaration for data validator
namespace SolarSystem::JPL {
class DataValidator;
struct ValidationReport;
}  // namespace SolarSystem::JPL

namespace SolarSystem::JPL {

/**
 * @brief Cache compression algorithms
 */
enum class CompressionAlgorithm {
  None,
  LZ4,   // Fast compression/decompression
  ZSTD,  // High compression ratio
  GZIP   // Standard compression
};

/**
 * @brief Cache refresh strategies
 */
enum class RefreshStrategy {
  Manual,           // Only refresh when explicitly requested
  TimeBasedAuto,    // Refresh based on time intervals
  AccessBasedAuto,  // Refresh based on access patterns
  Intelligent       // AI-driven refresh based on usage patterns
};

/**
 * @brief Cache validation levels
 */
enum class ValidationLevel {
  Basic,         // Basic file existence and size checks
  Standard,      // Standard validation with checksum verification
  Comprehensive  // Full validation with cross-format consistency checks
};

/**
 * @brief Cache statistics
 */
struct CacheStatistics {
  // Access statistics
  size_t total_reads = 0;
  size_t total_writes = 0;
  size_t cache_hits = 0;
  size_t cache_misses = 0;

  // Performance statistics
  std::chrono::nanoseconds total_read_time{0};
  std::chrono::nanoseconds total_write_time{0};
  std::chrono::nanoseconds average_read_time{0};
  std::chrono::nanoseconds average_write_time{0};

  // Storage statistics
  size_t binary_cache_size = 0;
  size_t json_cache_size = 0;
  size_t compressed_cache_size = 0;
  double compression_ratio = 0.0;

  // Validation statistics
  size_t validation_attempts = 0;
  size_t validation_successes = 0;
  size_t validation_failures = 0;

  // Refresh statistics
  size_t refresh_attempts = 0;
  size_t refresh_successes = 0;
  size_t refresh_failures = 0;
  std::chrono::system_clock::time_point last_refresh_time;

  // Error statistics
  size_t corruption_detections = 0;
  size_t recovery_attempts = 0;
  size_t recovery_successes = 0;

  /**
   * @brief Calculate cache hit ratio
   */
  [[nodiscard]] double hit_ratio() const {
    if (total_reads == 0) return 0.0;
    return static_cast<double>(cache_hits) / static_cast<double>(total_reads);
  }

  /**
   * @brief Calculate validation success ratio
   */
  [[nodiscard]] double validation_success_ratio() const {
    if (validation_attempts == 0) return 0.0;
    return static_cast<double>(validation_successes) / static_cast<double>(validation_attempts);
  }

  /**
   * @brief Update average read time
   */
  void update_average_read_time() {
    if (total_reads > 0) {
      average_read_time = total_read_time / total_reads;
    }
  }

  /**
   * @brief Update average write time
   */
  void update_average_write_time() {
    if (total_writes > 0) {
      average_write_time = total_write_time / total_writes;
    }
  }
};

/**
 * @brief Cache configuration for intelligent management
 */
struct CacheManagerConfig {
  // Basic cache settings
  std::filesystem::path cache_directory = "./cache";
  bool enable_binary_cache = true;
  bool enable_json_cache = true;
  bool enable_compression = true;
  CompressionAlgorithm compression_algorithm = CompressionAlgorithm::ZSTD;

  // Expiration settings
  std::chrono::hours cache_validity = std::chrono::hours(24 * 30);     // 30 days
  std::chrono::hours warning_threshold = std::chrono::hours(24 * 25);  // 25 days
  RefreshStrategy refresh_strategy = RefreshStrategy::TimeBasedAuto;
  std::chrono::hours auto_refresh_interval = std::chrono::hours(24 * 7);  // 7 days

  // Validation settings
  ValidationLevel default_validation_level = ValidationLevel::Standard;
  bool enable_background_validation = true;
  std::chrono::hours validation_interval = std::chrono::hours(24);  // Daily validation

  // Performance settings
  size_t max_cache_size = 1024 * 1024 * 1024;  // 1GB
  bool enable_cache_preloading = true;
  bool enable_statistics_collection = true;

  // Recovery settings
  bool enable_auto_recovery = true;
  size_t max_recovery_attempts = 3;
  bool enable_backup_cache = true;
  size_t max_backup_versions = 5;

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] bool is_valid(std::string* error = nullptr) const;
};

/**
 * @brief Cache entry metadata
 */
struct CacheEntryMetadata {
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_accessed;
  std::chrono::system_clock::time_point last_validated;
  size_t access_count = 0;
  size_t validation_count = 0;
  bool is_compressed = false;
  CompressionAlgorithm compression_algorithm = CompressionAlgorithm::None;
  size_t original_size = 0;
  size_t compressed_size = 0;
  uint64_t checksum = 0;

  /**
   * @brief Check if entry needs refresh
   */
  [[nodiscard]] bool needs_refresh(const CacheManagerConfig& config) const;

  /**
   * @brief Check if entry needs validation
   */
  [[nodiscard]] bool needs_validation(const CacheManagerConfig& config) const;
};

/**
 * @brief Intelligent Cache Manager
 */
class SOLAR_JPL_API CacheManager {
 public:
  /**
   * @brief Construct cache manager with configuration
   */
  explicit CacheManager(CacheManagerConfig config = {});

  /**
   * @brief Destructor ensures cleanup
   */
  ~CacheManager();

  // Non-copyable and non-movable
  CacheManager(const CacheManager&) = delete;
  CacheManager& operator=(const CacheManager&) = delete;
  CacheManager(CacheManager&&) = delete;
  CacheManager& operator=(CacheManager&&) = delete;

  /**
   * @brief Initialize cache manager
   */
  [[nodiscard]] JPLVoidResult initialize();

  /**
   * @brief Shutdown cache manager
   */
  [[nodiscard]] JPLVoidResult shutdown();

  // Cache Operations

  /**
   * @brief Load data from cache with intelligent validation
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> load_cache(
      ValidationLevel validation_level = ValidationLevel::Standard);

  /**
   * @brief Save data to cache with optimization
   */
  [[nodiscard]] JPLVoidResult save_cache(const std::vector<EphemerisData>& data,
                                         bool enable_compression = true);

  /**
   * @brief Validate cache integrity with specified level
   */
  [[nodiscard]] JPLResult<bool> validate_cache(
      ValidationLevel validation_level = ValidationLevel::Standard);

  /**
   * @brief Validate cache with comprehensive data validation
   */
  [[nodiscard]] JPLResult<ValidationReport> validate_cache_comprehensive(
      ValidationLevel validation_level = ValidationLevel::Comprehensive);

  /**
   * @brief Clear cache with optional backup
   */
  [[nodiscard]] JPLVoidResult clear_cache(bool create_backup = true);

  /**
   * @brief Rebuild cache from source
   */
  [[nodiscard]] JPLVoidResult rebuild_cache();

  // Cache Optimization

  /**
   * @brief Optimize cache storage
   */
  [[nodiscard]] JPLVoidResult optimize_cache();

  /**
   * @brief Compress cache files
   */
  [[nodiscard]] JPLVoidResult compress_cache(
      CompressionAlgorithm algorithm = CompressionAlgorithm::ZSTD);

  /**
   * @brief Decompress cache files
   */
  [[nodiscard]] JPLVoidResult decompress_cache();

  /**
   * @brief Defragment cache storage
   */
  [[nodiscard]] JPLVoidResult defragment_cache();

  // Cache Monitoring

  /**
   * @brief Get cache statistics
   */
  [[nodiscard]] const CacheStatistics& get_statistics() const;

  /**
   * @brief Reset statistics
   */
  void reset_statistics();

  /**
   * @brief Get cache health status
   */
  [[nodiscard]] JPLResult<double> get_cache_health() const;

  /**
   * @brief Get cache entry metadata
   */
  [[nodiscard]] std::optional<CacheEntryMetadata> get_entry_metadata() const;

  // Cache Refresh Strategies

  /**
   * @brief Check if cache needs refresh
   */
  [[nodiscard]] bool needs_refresh() const;

  /**
   * @brief Refresh cache if needed
   */
  [[nodiscard]] JPLVoidResult refresh_if_needed();

  /**
   * @brief Force cache refresh
   */
  [[nodiscard]] JPLVoidResult force_refresh();

  /**
   * @brief Set refresh strategy
   */
  void set_refresh_strategy(RefreshStrategy strategy);

  // Cache Backup and Recovery

  /**
   * @brief Create cache backup
   */
  [[nodiscard]] JPLVoidResult create_backup();

  /**
   * @brief Restore from backup
   */
  [[nodiscard]] JPLVoidResult restore_from_backup(size_t backup_version = 0);

  /**
   * @brief List available backups
   */
  [[nodiscard]] std::vector<std::filesystem::path> list_backups() const;

  /**
   * @brief Clean old backups
   */
  [[nodiscard]] JPLVoidResult clean_old_backups();

  // Configuration Management

  /**
   * @brief Get current configuration
   */
  [[nodiscard]] const CacheManagerConfig& config() const { return config_; }

  /**
   * @brief Update configuration
   */
  [[nodiscard]] JPLVoidResult update_config(const CacheManagerConfig& new_config);

  /**
   * @brief Set data validator for comprehensive validation
   */
  void set_data_validator(std::shared_ptr<DataValidator> validator);

  /**
   * @brief Get data validator
   */
  [[nodiscard]] std::shared_ptr<DataValidator> get_data_validator() const;

 private:
  CacheManagerConfig config_;
  CacheStatistics statistics_;
  std::optional<CacheEntryMetadata> entry_metadata_;

  // Internal implementation
  struct Impl;
  std::unique_ptr<Impl> impl_;

  // Data validator for comprehensive validation
  std::shared_ptr<DataValidator> data_validator_;

  // Internal methods
  [[nodiscard]] JPLVoidResult ensure_cache_directory();
  [[nodiscard]] JPLVoidResult load_entry_metadata();
  [[nodiscard]] JPLVoidResult save_entry_metadata();
  [[nodiscard]] JPLVoidResult update_statistics_on_read(std::chrono::nanoseconds duration);
  [[nodiscard]] JPLVoidResult update_statistics_on_write(std::chrono::nanoseconds duration);
  [[nodiscard]] JPLVoidResult detect_and_recover_corruption();
  [[nodiscard]] std::filesystem::path get_backup_directory() const;
  [[nodiscard]] std::filesystem::path get_backup_path(size_t version) const;
};

/**
 * @brief Cache Manager Factory
 */
class SOLAR_JPL_API CacheManagerFactory {
 public:
  /**
   * @brief Create default cache manager
   */
  [[nodiscard]] static std::unique_ptr<CacheManager> create_default();

  /**
   * @brief Create cache manager with custom configuration
   */
  [[nodiscard]] static std::unique_ptr<CacheManager> create(CacheManagerConfig config);

  /**
   * @brief Create cache manager for testing
   */
  [[nodiscard]] static std::unique_ptr<CacheManager> create_for_testing();
};

}  // namespace SolarSystem::JPL
