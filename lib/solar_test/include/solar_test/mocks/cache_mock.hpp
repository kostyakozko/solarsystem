/**
 * @file cache_mock.hpp
 * @brief Cache Operation Mock System for Testing
 *
 * Provides comprehensive mocking capabilities for cache operations testing:
 * - File system operation simulation without actual I/O
 * - Cache corruption and disk full simulation
 * - Cache validation testing capabilities
 * - Temporary test cache environments
 * - Integration with existing JPL cache interface
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "solar_jpl/jpl_client.hpp"

namespace SolarSystem::Testing::Mocks {

/**
 * @brief Mock cache operation types for different scenarios
 */
enum class MockCacheOperationType {
  Read,            // Cache read operation
  Write,           // Cache write operation
  Validate,        // Cache validation operation
  Clear,           // Cache clear operation
  MetadataRead,    // Metadata read operation
  MetadataWrite,   // Metadata write operation
  DirectoryCreate  // Directory creation operation
};

/**
 * @brief Mock cache error types
 */
enum class MockCacheError {
  Success,           // Operation successful
  FileNotFound,      // Cache file doesn't exist
  PermissionDenied,  // No permission to access file
  DiskFull,          // Disk space exhausted
  CorruptedData,     // Cache data is corrupted
  InvalidFormat,     // Cache format is invalid
  IOError,           // General I/O error
  NetworkError,      // Network-related cache error
  ValidationFailed   // Cache validation failed
};

/**
 * @brief Mock cache call information for verification
 */
struct MockCacheCallInfo {
  std::chrono::system_clock::time_point timestamp;
  MockCacheOperationType operation_type;
  std::filesystem::path file_path;
  size_t data_size;
  MockCacheError result;
  std::chrono::milliseconds operation_duration;
  std::string operation_details;
};

/**
 * @brief Configuration for cache mock behavior
 */
struct CacheMockConfig {
  // File system simulation
  bool simulate_file_operations = true;
  std::chrono::milliseconds min_operation_delay = std::chrono::milliseconds(1);
  std::chrono::milliseconds max_operation_delay = std::chrono::milliseconds(50);

  // Error simulation rates
  double disk_full_rate = 0.0;         // 0.0 = never full, 1.0 = always full
  double corruption_rate = 0.0;        // Rate of data corruption
  double permission_error_rate = 0.0;  // Rate of permission errors
  double io_error_rate = 0.0;          // Rate of general I/O errors

  // Cache behavior
  bool cache_always_exists = false;     // Cache files always exist
  bool cache_never_exists = false;      // Cache files never exist
  bool cache_always_valid = false;      // Cache is always valid
  bool cache_always_invalid = false;    // Cache is always invalid
  bool cache_always_corrupted = false;  // Cache is always corrupted

  // Performance simulation
  size_t max_cache_size = 100 * 1024 * 1024;  // 100MB max cache size
  double cache_hit_rate = 0.8;                // Cache hit rate simulation
  bool simulate_slow_disk = false;            // Simulate slow disk operations

  // Call tracking
  bool enable_call_history = true;  // Track all calls for verification
  size_t max_history_size = 1000;   // Maximum number of calls to remember

  // Temporary environment
  std::string temp_directory_prefix = "solar_test_cache_";
  bool auto_cleanup = true;  // Automatically cleanup temporary files
};

/**
 * @brief Cache Operation Mock Implementation
 *
 * Provides comprehensive mocking of cache operations for testing purposes.
 * Can simulate various file system conditions, corruption scenarios, and failure modes.
 */
class CacheMock {
  friend class TemporaryCache;

 public:
  /**
   * @brief Construct cache mock with configuration
   */
  explicit CacheMock(CacheMockConfig config = {});

  /**
   * @brief Destructor with automatic cleanup
   */
  ~CacheMock();

  // Non-copyable, non-movable (due to mutex and file handles)
  CacheMock(const CacheMock&) = delete;
  CacheMock& operator=(const CacheMock&) = delete;
  CacheMock(CacheMock&&) = delete;
  CacheMock& operator=(CacheMock&&) = delete;

  // === Cache State Configuration ===

  /**
   * @brief Set whether cache exists
   */
  void set_cache_exists(bool exists);

  /**
   * @brief Set whether cache is valid
   */
  void set_cache_valid(bool valid);

  /**
   * @brief Set cache data content
   */
  void set_cache_data(const std::string& data);

  /**
   * @brief Set cache data as ephemeris data
   */
  void set_cache_data(const std::vector<SolarSystem::JPL::EphemerisData>& data);

  /**
   * @brief Set cache metadata
   */
  void set_cache_metadata(const SolarSystem::JPL::CacheMetadata& metadata);

  // === Error Simulation ===

  /**
   * @brief Simulate cache corruption
   */
  void simulate_cache_corruption();

  /**
   * @brief Simulate disk full condition
   */
  void simulate_disk_full();

  /**
   * @brief Simulate permission denied errors
   */
  void simulate_permission_denied();

  /**
   * @brief Simulate I/O errors for next N operations
   */
  void simulate_io_error(size_t operation_count = 1);

  /**
   * @brief Simulate partial cache corruption
   */
  void simulate_partial_corruption(double corruption_percentage = 0.1);

  /**
   * @brief Simulate slow disk operations
   */
  void simulate_slow_disk(std::chrono::milliseconds delay);

  // === Cache Content Management ===

  /**
   * @brief Populate cache with valid test data
   */
  void populate_with_valid_data();

  /**
   * @brief Populate cache with corrupted test data
   */
  void populate_with_corrupted_data();

  /**
   * @brief Populate cache with specific ephemeris data
   */
  void populate_with_ephemeris_data(const std::vector<SolarSystem::JPL::EphemerisData>& data);

  /**
   * @brief Clear all cache data
   */
  void clear_cache_data();

  // === Mock Interface Implementation ===

  /**
   * @brief Mock implementation of cache loading
   */
  [[nodiscard]] SolarSystem::JPL::JPLResult<std::vector<SolarSystem::JPL::EphemerisData>>
  mock_load_from_cache();

  /**
   * @brief Mock implementation of cache saving
   */
  [[nodiscard]] SolarSystem::JPL::JPLVoidResult mock_save_to_cache(
      const std::vector<SolarSystem::JPL::EphemerisData>& data);

  /**
   * @brief Mock implementation of cache validation
   */
  [[nodiscard]] SolarSystem::JPL::JPLResult<bool> mock_validate_cache() const;

  /**
   * @brief Mock implementation of cache clearing
   */
  [[nodiscard]] SolarSystem::JPL::JPLVoidResult mock_clear_cache();

  /**
   * @brief Mock implementation of cache metadata loading
   */
  [[nodiscard]] std::optional<SolarSystem::JPL::CacheMetadata> mock_get_cache_metadata() const;

  /**
   * @brief Mock implementation of cache rebuilding
   */
  [[nodiscard]] SolarSystem::JPL::JPLVoidResult mock_rebuild_cache();

  // === File System Simulation ===

  /**
   * @brief Check if file exists (mock)
   */
  [[nodiscard]] bool mock_file_exists(const std::filesystem::path& path) const;

  /**
   * @brief Get file size (mock)
   */
  [[nodiscard]] std::optional<size_t> mock_file_size(const std::filesystem::path& path) const;

  /**
   * @brief Read file content (mock)
   */
  [[nodiscard]] std::optional<std::string> mock_read_file(const std::filesystem::path& path);

  /**
   * @brief Write file content (mock)
   */
  [[nodiscard]] bool mock_write_file(const std::filesystem::path& path, const std::string& content);

  /**
   * @brief Create directory (mock)
   */
  [[nodiscard]] bool mock_create_directory(const std::filesystem::path& path);

  /**
   * @brief Remove file (mock)
   */
  [[nodiscard]] bool mock_remove_file(const std::filesystem::path& path);

  // === Call Verification ===

  /**
   * @brief Check if cache was read
   */
  [[nodiscard]] bool was_cache_read() const;

  /**
   * @brief Check if cache was written
   */
  [[nodiscard]] bool was_cache_written() const;

  /**
   * @brief Check if cache was validated
   */
  [[nodiscard]] bool was_cache_validated() const;

  /**
   * @brief Check if cache was cleared
   */
  [[nodiscard]] bool was_cache_cleared() const;

  /**
   * @brief Get last written data
   */
  [[nodiscard]] std::string last_written_data() const;

  /**
   * @brief Get total number of operations
   */
  [[nodiscard]] size_t operation_count() const;

  /**
   * @brief Get number of operations by type
   */
  [[nodiscard]] size_t operation_count(MockCacheOperationType type) const;

  /**
   * @brief Get complete operation history
   */
  [[nodiscard]] const std::vector<MockCacheCallInfo>& operation_history() const;

  /**
   * @brief Get operations of specific type
   */
  [[nodiscard]] std::vector<MockCacheCallInfo> operations_of_type(
      MockCacheOperationType type) const;

  /**
   * @brief Reset call history and counters
   */
  void reset_call_history();

  // === Configuration Access ===

  /**
   * @brief Get current configuration
   */
  [[nodiscard]] const CacheMockConfig& config() const noexcept { return config_; }

  /**
   * @brief Update configuration
   */
  void update_config(const CacheMockConfig& new_config);

  // === Utility Functions ===

  /**
   * @brief Get current cache directory path
   */
  [[nodiscard]] std::filesystem::path cache_directory() const;

  /**
   * @brief Get binary cache file path
   */
  [[nodiscard]] std::filesystem::path binary_cache_path() const;

  /**
   * @brief Get JSON cache file path
   */
  [[nodiscard]] std::filesystem::path json_cache_path() const;

  /**
   * @brief Get metadata file path
   */
  [[nodiscard]] std::filesystem::path metadata_path() const;

  /**
   * @brief Install this mock as global cache mock
   */
  void install_as_global_mock();

  /**
   * @brief Remove global mock installation
   */
  static void remove_global_mock();

 private:
  CacheMockConfig config_;

  // Cache state
  bool cache_exists_ = true;
  bool cache_valid_ = true;
  bool cache_corrupted_ = false;
  std::string cache_data_;
  std::vector<SolarSystem::JPL::EphemerisData> ephemeris_data_;
  std::optional<SolarSystem::JPL::CacheMetadata> metadata_;

  // File system simulation
  std::unordered_map<std::string, std::string> mock_files_;
  std::unordered_map<std::string, bool> file_exists_map_;
  std::unordered_map<std::string, size_t> file_sizes_;

  // Error simulation state
  size_t remaining_io_errors_ = 0;
  bool disk_full_simulated_ = false;
  bool permission_denied_simulated_ = false;
  std::chrono::milliseconds slow_disk_delay_ = std::chrono::milliseconds(0);

  // Call tracking
  mutable std::mutex call_mutex_;
  std::vector<MockCacheCallInfo> operation_history_;
  std::unordered_map<MockCacheOperationType, size_t> operation_counts_;
  std::string last_written_data_;

  // Random number generation for simulation
  mutable std::random_device rd_;
  mutable std::mt19937 gen_;
  mutable std::uniform_real_distribution<double> error_dist_;

  // Cache paths
  std::filesystem::path cache_directory_;
  std::filesystem::path binary_cache_path_;
  std::filesystem::path json_cache_path_;
  std::filesystem::path metadata_path_;

  // === Internal Helper Methods ===

  /**
   * @brief Record a mock operation for verification
   */
  void record_operation(MockCacheOperationType type, const std::filesystem::path& path,
                        size_t data_size, MockCacheError result,
                        const std::string& details = "") const;

  /**
   * @brief Determine if should simulate error
   */
  [[nodiscard]] bool should_simulate_error(double error_rate) const;

  /**
   * @brief Simulate operation delay
   */
  void simulate_operation_delay() const;

  /**
   * @brief Check if operation should fail
   */
  [[nodiscard]] MockCacheError check_operation_errors() const;

  /**
   * @brief Generate corrupted data
   */
  [[nodiscard]] std::string generate_corrupted_data(const std::string& original_data,
                                                    double corruption_rate = 0.1) const;

  /**
   * @brief Generate valid test ephemeris data
   */
  [[nodiscard]] std::vector<SolarSystem::JPL::EphemerisData> generate_valid_test_data() const;

  /**
   * @brief Convert ephemeris data to binary format
   */
  [[nodiscard]] std::string ephemeris_data_to_binary(
      const std::vector<SolarSystem::JPL::EphemerisData>& data) const;

  /**
   * @brief Convert ephemeris data to JSON format
   */
  [[nodiscard]] std::string ephemeris_data_to_json(
      const std::vector<SolarSystem::JPL::EphemerisData>& data) const;

  /**
   * @brief Parse binary ephemeris data
   */
  [[nodiscard]] std::optional<std::vector<SolarSystem::JPL::EphemerisData>>
  parse_binary_ephemeris_data(const std::string& binary_data) const;

  /**
   * @brief Parse JSON ephemeris data
   */
  [[nodiscard]] std::optional<std::vector<SolarSystem::JPL::EphemerisData>>
  parse_json_ephemeris_data(const std::string& json_data) const;

  /**
   * @brief Generate metadata JSON
   */
  [[nodiscard]] std::string generate_metadata_json(
      const SolarSystem::JPL::CacheMetadata& metadata) const;

  /**
   * @brief Parse metadata JSON
   */
  [[nodiscard]] std::optional<SolarSystem::JPL::CacheMetadata> parse_metadata_json(
      const std::string& json_data) const;
};

/**
 * @brief Temporary test cache environment
 *
 * Creates an isolated cache environment for testing with automatic cleanup.
 * Provides realistic cache directory structure and file management.
 */
class TemporaryCache {
 public:
  /**
   * @brief Create temporary cache with specified type
   */
  explicit TemporaryCache(const std::string& cache_type = "ephemeris");

  /**
   * @brief Destructor with automatic cleanup
   */
  ~TemporaryCache();

  // Non-copyable, non-movable
  TemporaryCache(const TemporaryCache&) = delete;
  TemporaryCache& operator=(const TemporaryCache&) = delete;
  TemporaryCache(TemporaryCache&&) = delete;
  TemporaryCache& operator=(TemporaryCache&&) = delete;

  /**
   * @brief Populate cache with valid data
   */
  void populate_with_valid_data();

  /**
   * @brief Populate cache with corrupted data
   */
  void populate_with_corrupted_data();

  /**
   * @brief Simulate partial corruption
   */
  void simulate_partial_corruption(double corruption_percentage = 0.1);

  /**
   * @brief Get cache directory path
   */
  [[nodiscard]] std::filesystem::path cache_path() const;

  /**
   * @brief Get binary cache file path
   */
  [[nodiscard]] std::filesystem::path binary_cache_file() const;

  /**
   * @brief Get JSON cache file path
   */
  [[nodiscard]] std::filesystem::path json_cache_file() const;

  /**
   * @brief Get metadata file path
   */
  [[nodiscard]] std::filesystem::path metadata_file() const;

  /**
   * @brief Check if cache files exist
   */
  [[nodiscard]] bool cache_files_exist() const;

  /**
   * @brief Get cache file sizes
   */
  [[nodiscard]] std::map<std::string, size_t> cache_file_sizes() const;

  /**
   * @brief Create JPL client config using this cache
   */
  [[nodiscard]] SolarSystem::JPL::JPLClientConfig create_client_config() const;

 private:
  std::string cache_type_;
  std::filesystem::path temp_directory_;
  std::filesystem::path cache_directory_;
  bool cleanup_on_destroy_;

  /**
   * @brief Create directory structure
   */
  void create_directory_structure();

  /**
   * @brief Generate test ephemeris data
   */
  [[nodiscard]] std::vector<SolarSystem::JPL::EphemerisData> generate_test_data() const;

  /**
   * @brief Write binary cache file
   */
  void write_binary_cache(const std::vector<SolarSystem::JPL::EphemerisData>& data);

  /**
   * @brief Write JSON cache file
   */
  void write_json_cache(const std::vector<SolarSystem::JPL::EphemerisData>& data);

  /**
   * @brief Write metadata file
   */
  void write_metadata(const std::vector<SolarSystem::JPL::EphemerisData>& data);

  /**
   * @brief Corrupt file content
   */
  void corrupt_file(const std::filesystem::path& file_path, double corruption_rate);
};

/**
 * @brief Factory for creating cache mocks
 */
class CacheMockFactory {
 public:
  /**
   * @brief Create default cache mock
   */
  [[nodiscard]] static std::unique_ptr<CacheMock> create_default();

  /**
   * @brief Create cache mock with custom configuration
   */
  [[nodiscard]] static std::unique_ptr<CacheMock> create(CacheMockConfig config);

  /**
   * @brief Create cache mock for corruption testing
   */
  [[nodiscard]] static std::unique_ptr<CacheMock> create_for_corruption_testing();

  /**
   * @brief Create cache mock for disk full testing
   */
  [[nodiscard]] static std::unique_ptr<CacheMock> create_for_disk_full_testing();

  /**
   * @brief Create cache mock for performance testing
   */
  [[nodiscard]] static std::unique_ptr<CacheMock> create_for_performance_testing();

  /**
   * @brief Create cache mock with always valid cache
   */
  [[nodiscard]] static std::unique_ptr<CacheMock> create_with_valid_cache();

  /**
   * @brief Create cache mock with always invalid cache
   */
  [[nodiscard]] static std::unique_ptr<CacheMock> create_with_invalid_cache();
};

/**
 * @brief RAII helper for installing/removing global cache mock
 */
class ScopedCacheMock {
 public:
  explicit ScopedCacheMock(std::unique_ptr<CacheMock> mock);
  ~ScopedCacheMock();

  // Non-copyable, non-movable
  ScopedCacheMock(const ScopedCacheMock&) = delete;
  ScopedCacheMock& operator=(const ScopedCacheMock&) = delete;
  ScopedCacheMock(ScopedCacheMock&&) = delete;
  ScopedCacheMock& operator=(ScopedCacheMock&&) = delete;

  /**
   * @brief Get access to the mock
   */
  [[nodiscard]] CacheMock& mock() { return *mock_; }
  [[nodiscard]] const CacheMock& mock() const { return *mock_; }

 private:
  std::unique_ptr<CacheMock> mock_;
};

}  // namespace SolarSystem::Testing::Mocks
