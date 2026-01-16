/**
 * @file response_cache.cpp
 * @brief Implementation of response caching
 */

#include "solar_core/performance/response_cache.hpp"

#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace SolarSystem::Performance {

struct ResponseCache::Impl {
  CacheConfig config;
  std::unordered_map<std::string, CacheEntry> cache;
  std::list<std::string> lru_list;  // Most recently used at front
  std::unordered_map<std::string, std::list<std::string>::iterator> lru_map;
  CacheStatistics stats;
  mutable std::mutex mutex;

  explicit Impl(CacheConfig cfg) : config(std::move(cfg)) {}

  void evict_lru() {
    if (lru_list.empty()) return;

    auto key = lru_list.back();
    lru_list.pop_back();
    lru_map.erase(key);
    cache.erase(key);
    stats.evictions++;
  }

  void touch(const std::string& key) {
    auto it = lru_map.find(key);
    if (it != lru_map.end()) {
      lru_list.erase(it->second);
      lru_list.push_front(key);
      lru_map[key] = lru_list.begin();
    }
  }
};

ResponseCache::ResponseCache(CacheConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

ResponseCache::~ResponseCache() = default;

ResponseCache::ResponseCache(ResponseCache&&) noexcept = default;
ResponseCache& ResponseCache::operator=(ResponseCache&&) noexcept = default;

std::optional<std::string> ResponseCache::get(const std::string& key) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  impl_->stats.total_requests++;

  auto it = impl_->cache.find(key);
  if (it == impl_->cache.end()) {
    impl_->stats.cache_misses++;
    return std::nullopt;
  }

  auto now = std::chrono::system_clock::now();
  if (now >= it->second.expires_at) {
    impl_->cache.erase(it);
    impl_->stats.cache_misses++;
    return std::nullopt;
  }

  impl_->stats.cache_hits++;
  impl_->stats.hit_rate = static_cast<double>(impl_->stats.cache_hits) /
                          static_cast<double>(impl_->stats.total_requests);

  it->second.access_count++;
  it->second.last_accessed = now;
  impl_->touch(key);

  return it->second.value;
}

void ResponseCache::put(const std::string& key, const std::string& value) {
  put(key, value, impl_->config.default_ttl);
}

void ResponseCache::put(const std::string& key, const std::string& value,
                        std::chrono::seconds ttl) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  // Check if we need to evict
  while (impl_->cache.size() >= impl_->config.max_entries) {
    impl_->evict_lru();
  }

  CacheEntry entry;
  entry.key = key;
  entry.value = value;
  entry.created_at = std::chrono::system_clock::now();
  entry.expires_at = entry.created_at + ttl;
  entry.last_accessed = entry.created_at;

  impl_->cache[key] = entry;
  impl_->lru_list.push_front(key);
  impl_->lru_map[key] = impl_->lru_list.begin();

  impl_->stats.current_entries = impl_->cache.size();
}

void ResponseCache::remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->lru_map.find(key);
  if (it != impl_->lru_map.end()) {
    impl_->lru_list.erase(it->second);
    impl_->lru_map.erase(it);
  }

  impl_->cache.erase(key);
  impl_->stats.current_entries = impl_->cache.size();
}

void ResponseCache::clear() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->cache.clear();
  impl_->lru_list.clear();
  impl_->lru_map.clear();
  impl_->stats.current_entries = 0;
}

bool ResponseCache::contains(const std::string& key) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->cache.find(key) != impl_->cache.end();
}

CacheStatistics ResponseCache::get_statistics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->stats;
}

void ResponseCache::cleanup_expired() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto now = std::chrono::system_clock::now();

  for (auto it = impl_->cache.begin(); it != impl_->cache.end();) {
    if (now >= it->second.expires_at) {
      auto lru_it = impl_->lru_map.find(it->first);
      if (lru_it != impl_->lru_map.end()) {
        impl_->lru_list.erase(lru_it->second);
        impl_->lru_map.erase(lru_it);
      }
      it = impl_->cache.erase(it);
    } else {
      ++it;
    }
  }

  impl_->stats.current_entries = impl_->cache.size();
}

size_t ResponseCache::size() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->cache.size();
}

// CacheKeyGenerator implementation
std::string CacheKeyGenerator::generate(const std::string& method, const std::string& path,
                                        const std::string& query_string) {
  std::string key = method + ":" + path;
  if (!query_string.empty()) {
    key += "?" + query_string;
  }
  return key;
}

std::string CacheKeyGenerator::generate_custom(const std::vector<std::string>& components) {
  std::string key;
  for (size_t i = 0; i < components.size(); ++i) {
    if (i > 0) key += ":";
    key += components[i];
  }
  return key;
}

}  // namespace SolarSystem::Performance
