/**
 * @file rate_limiter.cpp
 * @brief Implementation of rate limiting
 */

#include "solar_core/security/rate_limiter.hpp"

#include <chrono>
#include <map>
#include <mutex>
#include <thread>

namespace SolarSystem::Security {

struct RateLimiter::Impl {
  RateLimitConfig config;
  std::map<std::string, std::vector<std::chrono::system_clock::time_point>> requests;
  mutable std::mutex mutex;
  std::thread cleanup_thread;
  std::atomic<bool> running{false};

  explicit Impl(RateLimitConfig cfg) : config(std::move(cfg)) {}

  ~Impl() {
    if (running.load()) {
      running.store(false);
      if (cleanup_thread.joinable()) {
        cleanup_thread.join();
      }
    }
  }
};

RateLimiter::RateLimiter(RateLimitConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

RateLimiter::~RateLimiter() = default;

RateLimiter::RateLimiter(RateLimiter&&) noexcept = default;
RateLimiter& RateLimiter::operator=(RateLimiter&&) noexcept = default;

RateLimitResult RateLimiter::check_limit(const std::string& identifier) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto now = std::chrono::system_clock::now();
  auto window_start = now - impl_->config.window;

  // Get or create request list
  auto& request_times = impl_->requests[identifier];

  // Remove old requests outside window
  request_times.erase(
      std::remove_if(request_times.begin(), request_times.end(),
                     [window_start](const auto& time) { return time < window_start; }),
      request_times.end());

  RateLimitResult result;
  result.total_requests = request_times.size();

  if (request_times.size() >= impl_->config.max_requests) {
    result.allowed = false;
    result.remaining = 0;

    // Calculate retry_after
    if (!request_times.empty()) {
      auto oldest = request_times.front();
      auto time_until_expire = std::chrono::duration_cast<std::chrono::seconds>(
          oldest + impl_->config.window - now);
      result.retry_after = time_until_expire;
    }
  } else {
    result.allowed = true;
    result.remaining = impl_->config.max_requests - request_times.size() - 1;
    request_times.push_back(now);
  }

  return result;
}

void RateLimiter::reset_limit(const std::string& identifier) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->requests.erase(identifier);
}

size_t RateLimiter::get_usage(const std::string& identifier) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->requests.find(identifier);
  if (it != impl_->requests.end()) {
    return it->second.size();
  }

  return 0;
}

void RateLimiter::cleanup() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto now = std::chrono::system_clock::now();
  auto window_start = now - impl_->config.window;

  for (auto it = impl_->requests.begin(); it != impl_->requests.end();) {
    auto& request_times = it->second;

    request_times.erase(
        std::remove_if(request_times.begin(), request_times.end(),
                       [window_start](const auto& time) { return time < window_start; }),
        request_times.end());

    if (request_times.empty()) {
      it = impl_->requests.erase(it);
    } else {
      ++it;
    }
  }
}

void RateLimiter::start_cleanup_thread() {
  if (impl_->running.exchange(true)) {
    return;
  }

  impl_->cleanup_thread = std::thread([this]() {
    while (impl_->running.load()) {
      std::this_thread::sleep_for(impl_->config.cleanup_interval);
      cleanup();
    }
  });
}

void RateLimiter::stop_cleanup_thread() {
  impl_->running.store(false);
  if (impl_->cleanup_thread.joinable()) {
    impl_->cleanup_thread.join();
  }
}

// MultiTierRateLimiter implementation
void MultiTierRateLimiter::add_tier(const std::string& name, RateLimitConfig config) {
  tiers_[name] = std::make_unique<RateLimiter>(config);
}

RateLimitResult MultiTierRateLimiter::check_all_tiers(const std::string& identifier) {
  for (const auto& [name, limiter] : tiers_) {
    auto result = limiter->check_limit(identifier);
    if (!result.allowed) {
      return result;
    }
  }

  RateLimitResult result;
  result.allowed = true;
  return result;
}

RateLimitResult MultiTierRateLimiter::check_tier(
    const std::string& tier_name,
    const std::string& identifier) {
  auto it = tiers_.find(tier_name);
  if (it != tiers_.end()) {
    return it->second->check_limit(identifier);
  }

  RateLimitResult result;
  result.allowed = true;
  return result;
}

}  // namespace SolarSystem::Security
