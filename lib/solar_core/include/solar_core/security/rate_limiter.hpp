/**
 * @file rate_limiter.hpp
 * @brief Rate limiting for DDoS protection
 *
 * Provides rate limiting with:
 * - Token bucket algorithm
 * - Sliding window counters
 * - Per-user and per-IP limits
 * - Automatic cleanup
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "solar_core/export.hpp"

namespace SolarSystem::Security {

/**
 * @brief Rate limit configuration
 */
struct RateLimitConfig {
  size_t max_requests = 60;
  std::chrono::seconds window{60};
  size_t burst_size = 10;
  bool enable_burst = true;
  std::chrono::minutes cleanup_interval{5};
};

/**
 * @brief Rate limit result
 */
struct RateLimitResult {
  bool allowed = false;
  size_t remaining = 0;
  std::chrono::seconds retry_after{0};
  size_t total_requests = 0;
};

/**
 * @brief Rate limiter using token bucket algorithm
 */
class SOLAR_CORE_API RateLimiter {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit RateLimiter(RateLimitConfig config = {});

  /**
   * @brief Destructor
   */
  ~RateLimiter();

  // Non-copyable, movable
  RateLimiter(const RateLimiter&) = delete;
  RateLimiter& operator=(const RateLimiter&) = delete;
  RateLimiter(RateLimiter&&) noexcept;
  RateLimiter& operator=(RateLimiter&&) noexcept;

  /**
   * @brief Check if request is allowed
   */
  [[nodiscard]] RateLimitResult check_limit(const std::string& identifier);

  /**
   * @brief Reset limit for identifier
   */
  void reset_limit(const std::string& identifier);

  /**
   * @brief Get current usage
   */
  [[nodiscard]] size_t get_usage(const std::string& identifier) const;

  /**
   * @brief Cleanup expired entries
   */
  void cleanup();

  /**
   * @brief Start automatic cleanup
   */
  void start_cleanup_thread();

  /**
   * @brief Stop automatic cleanup
   */
  void stop_cleanup_thread();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Multi-tier rate limiter
 */
class MultiTierRateLimiter {
 public:
  /**
   * @brief Add rate limit tier
   */
  void add_tier(const std::string& name, RateLimitConfig config);

  /**
   * @brief Check all tiers
   */
  [[nodiscard]] RateLimitResult check_all_tiers(const std::string& identifier);

  /**
   * @brief Check specific tier
   */
  [[nodiscard]] RateLimitResult check_tier(const std::string& tier_name,
                                           const std::string& identifier);

 private:
  std::unordered_map<std::string, std::unique_ptr<RateLimiter>> tiers_;
};

}  // namespace SolarSystem::Security
