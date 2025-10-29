/**
 * @file shared_data_manager.cpp
 * @brief Implementation of shared data management
 */

#include "solar_core/data/shared_data_manager.hpp"

#include <algorithm>
#include <map>
#include <mutex>

namespace SolarSystem::Data {

// SharedDataManager implementation
struct SharedDataManager::Impl {
  std::map<std::string, std::string> data_store;  // Simplified: string storage
  std::map<std::string, DataVersion> versions;
  std::map<std::string, std::string> locks;  // key -> owner
  std::map<std::string, std::chrono::system_clock::time_point> lock_expiry;
  mutable std::mutex mutex;
  ConflictResolution conflict_strategy = ConflictResolution::LAST_WRITE_WINS;
  Statistics stats;
  uint64_t next_version = 1;
};

SharedDataManager::SharedDataManager() : impl_(std::make_unique<Impl>()) {}

SharedDataManager::~SharedDataManager() = default;

SharedDataManager::SharedDataManager(SharedDataManager&&) noexcept = default;

SharedDataManager& SharedDataManager::operator=(SharedDataManager&&) noexcept = default;

bool SharedDataManager::remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->data_store.find(key);
  if (it == impl_->data_store.end()) {
    return false;
  }

  impl_->data_store.erase(it);
  impl_->versions.erase(key);
  impl_->locks.erase(key);
  impl_->lock_expiry.erase(key);

  impl_->stats.total_entries--;
  return true;
}

bool SharedDataManager::exists(const std::string& key) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->data_store.find(key) != impl_->data_store.end();
}

std::vector<std::string> SharedDataManager::get_keys() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  std::vector<std::string> keys;
  keys.reserve(impl_->data_store.size());
  for (const auto& [key, _] : impl_->data_store) {
    keys.push_back(key);
  }
  return keys;
}

SyncResult SharedDataManager::synchronize(const std::map<std::string, DataVersion>& remote_versions,
                                         ConflictResolution strategy) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  SyncResult result;
  result.success = true;

  for (const auto& [key, remote_version] : remote_versions) {
    auto local_it = impl_->versions.find(key);

    if (local_it == impl_->versions.end()) {
      // New data from remote
      result.items_synced++;
    } else {
      const auto& local_version = local_it->second;

      if (local_version.version != remote_version.version) {
        // Conflict detected
        DataConflict conflict;
        conflict.type = ConflictType::VERSION_MISMATCH;
        conflict.key = key;
        conflict.local_version = local_version;
        conflict.remote_version = remote_version;
        conflict.description = "Version mismatch detected";

        result.conflicts.push_back(conflict);
        result.conflicts_detected++;

        // Apply resolution strategy
        if (strategy == ConflictResolution::LAST_WRITE_WINS) {
          if (remote_version.timestamp > local_version.timestamp) {
            // Remote wins
            impl_->versions[key] = remote_version;
            result.items_synced++;
          }
        } else if (strategy == ConflictResolution::FIRST_WRITE_WINS) {
          if (local_version.timestamp < remote_version.timestamp) {
            // Local wins (do nothing)
          }
        }

        impl_->stats.conflicts_resolved++;
      }
    }
  }

  return result;
}

void SharedDataManager::set_conflict_resolution(ConflictResolution strategy) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->conflict_strategy = strategy;
}

std::optional<DataVersion> SharedDataManager::get_version(const std::string& key) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto it = impl_->versions.find(key);
  if (it != impl_->versions.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool SharedDataManager::lock(const std::string& key, const std::string& owner,
                             std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  // Check if already locked
  auto lock_it = impl_->locks.find(key);
  if (lock_it != impl_->locks.end()) {
    // Check if lock expired
    auto expiry_it = impl_->lock_expiry.find(key);
    if (expiry_it != impl_->lock_expiry.end()) {
      if (std::chrono::system_clock::now() < expiry_it->second) {
        return false;  // Still locked
      }
    }
  }

  // Acquire lock
  impl_->locks[key] = owner;
  impl_->lock_expiry[key] = std::chrono::system_clock::now() + timeout;
  impl_->stats.locked_entries++;

  return true;
}

void SharedDataManager::unlock(const std::string& key, const std::string& owner) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto lock_it = impl_->locks.find(key);
  if (lock_it != impl_->locks.end() && lock_it->second == owner) {
    impl_->locks.erase(lock_it);
    impl_->lock_expiry.erase(key);
    if (impl_->stats.locked_entries > 0) {
      impl_->stats.locked_entries--;
    }
  }
}

bool SharedDataManager::is_locked(const std::string& key) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto lock_it = impl_->locks.find(key);
  if (lock_it == impl_->locks.end()) {
    return false;
  }

  // Check if expired
  auto expiry_it = impl_->lock_expiry.find(key);
  if (expiry_it != impl_->lock_expiry.end()) {
    return std::chrono::system_clock::now() < expiry_it->second;
  }

  return true;
}

void SharedDataManager::clear() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->data_store.clear();
  impl_->versions.clear();
  impl_->locks.clear();
  impl_->lock_expiry.clear();
  impl_->stats = Statistics{};
}

SharedDataManager::Statistics SharedDataManager::get_statistics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->stats.total_entries = impl_->data_store.size();
  return impl_->stats;
}

// DistributedCache implementation
struct DistributedCache::Impl {
  std::map<std::string, std::string> cache_store;  // Simplified
  std::map<std::string, std::chrono::system_clock::time_point> expiry;
  mutable std::mutex mutex;
  CacheStats stats;
};

DistributedCache::DistributedCache() : impl_(std::make_unique<Impl>()) {}

DistributedCache::~DistributedCache() = default;

void DistributedCache::invalidate(const std::string& key) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->cache_store.erase(key);
  impl_->expiry.erase(key);
}

void DistributedCache::clear() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->cache_store.clear();
  impl_->expiry.clear();
  impl_->stats = CacheStats{};
}

DistributedCache::CacheStats DistributedCache::get_stats() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->stats.total_entries = impl_->cache_store.size();
  if (impl_->stats.hits + impl_->stats.misses > 0) {
    impl_->stats.hit_rate =
        static_cast<double>(impl_->stats.hits) / (impl_->stats.hits + impl_->stats.misses);
  }
  return impl_->stats;
}

}  // namespace SolarSystem::Data
