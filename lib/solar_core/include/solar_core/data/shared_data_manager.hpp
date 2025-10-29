/**
 * @file shared_data_manager.hpp
 * @brief Shared data management and synchronization system
 *
 * Provides data sharing between applications with:
 * - Thread-safe shared data access
 * - Data consistency and synchronization
 * - Distributed caching
 * - Conflict resolution
 * - Version tracking
 */

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/utils/expected.hpp"

namespace SolarSystem::Data {

/**
 * @brief Data version for conflict detection
 */
struct DataVersion {
  uint64_t version = 0;
  std::chrono::system_clock::time_point timestamp;
  std::string modified_by;

  [[nodiscard]] bool operator<(const DataVersion& other) const { return version < other.version; }
  [[nodiscard]] bool operator==(const DataVersion& other) const {
    return version == other.version;
  }
};

/**
 * @brief Shared data entry
 */
template <typename T>
struct SharedDataEntry {
  std::string key;
  T value;
  DataVersion version;
  std::chrono::system_clock::time_point expires_at;
  bool is_dirty = false;
  std::string owner;
};

/**
 * @brief Data conflict types
 */
enum class ConflictType {
  VERSION_MISMATCH,    ///< Version conflict
  CONCURRENT_MODIFY,   ///< Concurrent modification
  STALE_DATA,          ///< Data is stale
  OWNERSHIP_CONFLICT   ///< Ownership conflict
};

/**
 * @brief Data conflict information
 */
struct DataConflict {
  ConflictType type;
  std::string key;
  DataVersion local_version;
  DataVersion remote_version;
  std::string description;
};

/**
 * @brief Conflict resolution strategy
 */
enum class ConflictResolution {
  LAST_WRITE_WINS,     ///< Use most recent write
  FIRST_WRITE_WINS,    ///< Use first write
  MANUAL,              ///< Require manual resolution
  MERGE,               ///< Attempt to merge
  REJECT               ///< Reject conflicting write
};

/**
 * @brief Data synchronization result
 */
struct SyncResult {
  bool success = false;
  size_t items_synced = 0;
  size_t conflicts_detected = 0;
  std::vector<DataConflict> conflicts;
  std::string error_message;
};

/**
 * @brief Shared data manager for inter-application data sharing
 */
class SharedDataManager {
 public:
  SharedDataManager();
  ~SharedDataManager();

  // Non-copyable, movable
  SharedDataManager(const SharedDataManager&) = delete;
  SharedDataManager& operator=(const SharedDataManager&) = delete;
  SharedDataManager(SharedDataManager&&) noexcept;
  SharedDataManager& operator=(SharedDataManager&&) noexcept;

  /**
   * @brief Store data with key
   */
  template <typename T>
  [[nodiscard]] SolarSystem::Utils::Expected<DataVersion, std::string> store(
      const std::string& key, const T& value, const std::string& owner = "");

  /**
   * @brief Retrieve data by key
   */
  template <typename T>
  [[nodiscard]] std::optional<SharedDataEntry<T>> retrieve(const std::string& key);

  /**
   * @brief Update existing data
   */
  template <typename T>
  [[nodiscard]] SolarSystem::Utils::Expected<DataVersion, std::string> update(
      const std::string& key, const T& value, const DataVersion& expected_version,
      const std::string& owner = "");

  /**
   * @brief Delete data
   */
  [[nodiscard]] bool remove(const std::string& key);

  /**
   * @brief Check if key exists
   */
  [[nodiscard]] bool exists(const std::string& key) const;

  /**
   * @brief Get all keys
   */
  [[nodiscard]] std::vector<std::string> get_keys() const;

  /**
   * @brief Synchronize with remote data source
   */
  [[nodiscard]] SyncResult synchronize(
      const std::map<std::string, DataVersion>& remote_versions,
      ConflictResolution strategy = ConflictResolution::LAST_WRITE_WINS);

  /**
   * @brief Set conflict resolution strategy
   */
  void set_conflict_resolution(ConflictResolution strategy);

  /**
   * @brief Get data version
   */
  [[nodiscard]] std::optional<DataVersion> get_version(const std::string& key) const;

  /**
   * @brief Lock data for exclusive access
   */
  [[nodiscard]] bool lock(const std::string& key, const std::string& owner,
                         std::chrono::milliseconds timeout = std::chrono::seconds(30));

  /**
   * @brief Unlock data
   */
  void unlock(const std::string& key, const std::string& owner);

  /**
   * @brief Check if data is locked
   */
  [[nodiscard]] bool is_locked(const std::string& key) const;

  /**
   * @brief Clear all data
   */
  void clear();

  /**
   * @brief Get statistics
   */
  struct Statistics {
    size_t total_entries = 0;
    size_t locked_entries = 0;
    size_t dirty_entries = 0;
    size_t total_reads = 0;
    size_t total_writes = 0;
    size_t conflicts_resolved = 0;
  };

  [[nodiscard]] Statistics get_statistics() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Distributed cache for shared data
 */
class DistributedCache {
 public:
  DistributedCache();
  ~DistributedCache();

  /**
   * @brief Cache data with TTL
   */
  template <typename T>
  void cache(const std::string& key, const T& value,
            std::chrono::seconds ttl = std::chrono::minutes(10));

  /**
   * @brief Get cached data
   */
  template <typename T>
  [[nodiscard]] std::optional<T> get(const std::string& key);

  /**
   * @brief Invalidate cache entry
   */
  void invalidate(const std::string& key);

  /**
   * @brief Clear all cache
   */
  void clear();

  /**
   * @brief Get cache statistics
   */
  struct CacheStats {
    size_t total_entries = 0;
    size_t hits = 0;
    size_t misses = 0;
    double hit_rate = 0.0;
  };

  [[nodiscard]] CacheStats get_stats() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Template method implementations must be in header for templates
// Note: This is a simplified mock implementation for testing
// A production implementation would use proper serialization

template <typename T>
SolarSystem::Utils::Expected<SolarSystem::Data::DataVersion, std::string>
SolarSystem::Data::SharedDataManager::store(
    const std::string& /* key */, const T& /* value */, const std::string& owner) {
  // Simplified mock implementation for testing
  // In production, use proper serialization (JSON, protobuf, etc.)
  DataVersion version;
  version.version = 1;
  version.timestamp = std::chrono::system_clock::now();
  version.modified_by = owner;

  return SolarSystem::Utils::Expected<DataVersion, std::string>(version);
}

template <typename T>
std::optional<SolarSystem::Data::SharedDataEntry<T>>
SolarSystem::Data::SharedDataManager::retrieve(const std::string& key) {
  // Simplified mock implementation for testing
  // In production, deserialize from internal storage
  if (!exists(key)) {
    return std::nullopt;
  }

  SharedDataEntry<T> entry;
  entry.key = key;
  // value would be deserialized here
  auto version = get_version(key);
  if (version) {
    entry.version = *version;
  }

  return entry;
}

template <typename T>
SolarSystem::Utils::Expected<SolarSystem::Data::DataVersion, std::string>
SolarSystem::Data::SharedDataManager::update(
    const std::string& key, const T& /* value */, const DataVersion& expected_version,
    const std::string& owner) {
  // Simplified mock implementation for testing
  // Check if key exists
  if (!exists(key)) {
    return SolarSystem::Utils::Expected<DataVersion, std::string>(
        std::string("Key does not exist"));
  }

  // Check version
  auto current_version = get_version(key);
  if (!current_version || current_version->version != expected_version.version) {
    return SolarSystem::Utils::Expected<DataVersion, std::string>(
        std::string("Version mismatch"));
  }

  // Update (simplified - just create new version)
  DataVersion new_version;
  new_version.version = expected_version.version + 1;
  new_version.timestamp = std::chrono::system_clock::now();
  new_version.modified_by = owner;

  return SolarSystem::Utils::Expected<DataVersion, std::string>(new_version);
}

template <typename T>
void SolarSystem::Data::DistributedCache::cache(
    const std::string& /* key */, const T& /* value */, std::chrono::seconds /* ttl */) {
  // Simplified mock implementation for testing
  // In production, serialize T and store with TTL
}

template <typename T>
std::optional<T> SolarSystem::Data::DistributedCache::get(const std::string& /* key */) {
  // Simplified mock implementation for testing
  // In production, check TTL and deserialize
  return std::nullopt;
}

}  // namespace SolarSystem::Data

