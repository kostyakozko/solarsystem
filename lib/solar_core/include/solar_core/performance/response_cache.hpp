/**
 * @file response_cache.hpp
 * @brief Response caching for performance optimization
 *
 * Provides response caching with:
 * - LRU (Least Recently Used) eviction
 * - TTL (Time To Live) support
 * - Cache invalidation
 * - Memory management
 */

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace SolarSystem::Performance {

/**
 * @brief Cache entry
 */
struct CacheEntry {
  std::string key;
  std::string value;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point expires_at;
  size_t access_count = 0;
  std::chrono::system_clock::time_point last_accessed;
};

/**
 * @brief Cache configuration
 */
struct CacheConfig {
  size_t max_entries = 1000;
  size_t max_memory_bytes = 100 * 1024 * 1024;  // 100 MB
  std::chrono::seconds default_ttl{300};  // 5 minutes
  bool enable_compression = false;
  bool enable_statistics = true;
};

/**
 * @brief Cache statistics
 */
struct CacheStatistics {
  size_t total_requests = 0;
  size_t cache_hits = 0;
  size_t cache_misses = 0;
  size_t evictions = 0;
  size_t current_entries = 0;
  size_t current_memory_bytes = 0;
  double hit_rate = 0.0;
};

/**
 * @brief Response cache with LRU eviction
 */
class ResponseCache {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit ResponseCache(CacheConfig config = {});

  /**
   * @brief Destructor
   */
  ~ResponseCache();

  // Non-copyable, movable
  ResponseCache(const ResponseCache&) = delete;
  ResponseCache& operator=(const ResponseCache&) = delete;
  ResponseCache(ResponseCache&&) noexcept;
  ResponseCache& operator=(ResponseCache&&) noexcept;

  /**
   * @brief Get cached value
   */
  [[nodiscard]] std::optional<std::string> get(const std::string& key);

  /**
   * @brief Put value in cache
   */
  void put(const std::string& key, const std::string& value);

  /**
   * @brief Put value with custom TTL
   */
  void put(const std::string& key, const std::string& value, std::chrono::seconds ttl);

  /**
   * @brief Remove entry from cache
   */
  void remove(const std::string& key);

  /**
   * @brief Clear all entries
   */
  void clear();

  /**
   * @brief Check if key exists
   */
  [[nodiscard]] bool contains(const std::string& key) const;

  /**
   * @brief Get cache statistics
   */
  [[nodiscard]] CacheStatistics get_statistics() const;

  /**
   * @brief Cleanup expired entries
   */
  void cleanup_expired();

  /**
   * @brief Get cache size
   */
  [[nodiscard]] size_t size() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Cache key generator
 */
class CacheKeyGenerator {
 public:
  /**
   * @brief Generate cache key from request components
   */
  [[nodiscard]] static std::string generate(
      const std::string& method,
      const std::string& path,
      const std::string& query_string = "");

  /**
   * @brief Generate cache key with custom components
   */
  [[nodiscard]] static std::string generate_custom(
      const std::vector<std::string>& components);
};

}  // namespace SolarSystem::Performance
