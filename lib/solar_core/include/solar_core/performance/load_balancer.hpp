/**
 * @file load_balancer.hpp
 * @brief Load balancing for request distribution
 *
 * Provides load balancing with:
 * - Round-robin distribution
 * - Least connections algorithm
 * - Health checking
 * - Backend management
 */

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Performance {

/**
 * @brief Load balancing algorithm
 */
enum class LoadBalancingAlgorithm {
  ROUND_ROBIN,
  LEAST_CONNECTIONS,
  RANDOM,
  WEIGHTED_ROUND_ROBIN
};

/**
 * @brief Backend server
 */
struct Backend {
  std::string id;
  std::string host;
  int port;
  int weight = 1;
  bool healthy = true;
  size_t active_connections = 0;
  std::chrono::system_clock::time_point last_health_check;
};

/**
 * @brief Load balancer configuration
 */
struct LoadBalancerConfig {
  LoadBalancingAlgorithm algorithm = LoadBalancingAlgorithm::ROUND_ROBIN;
  std::chrono::seconds health_check_interval{10};
  size_t max_retries = 3;
  bool enable_sticky_sessions = false;
};

/**
 * @brief Load balancer statistics
 */
struct LoadBalancerStats {
  size_t total_requests = 0;
  size_t successful_requests = 0;
  size_t failed_requests = 0;
  size_t total_backends = 0;
  size_t healthy_backends = 0;
};

/**
 * @brief Load balancer
 */
class LoadBalancer {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit LoadBalancer(LoadBalancerConfig config = {});

  /**
   * @brief Destructor
   */
  ~LoadBalancer();

  // Non-copyable, movable
  LoadBalancer(const LoadBalancer&) = delete;
  LoadBalancer& operator=(const LoadBalancer&) = delete;
  LoadBalancer(LoadBalancer&&) noexcept;
  LoadBalancer& operator=(LoadBalancer&&) noexcept;

  /**
   * @brief Add backend server
   */
  void add_backend(const Backend& backend);

  /**
   * @brief Remove backend server
   */
  void remove_backend(const std::string& backend_id);

  /**
   * @brief Select backend for request
   */
  [[nodiscard]] std::optional<Backend> select_backend();

  /**
   * @brief Mark backend as healthy/unhealthy
   */
  void set_backend_health(const std::string& backend_id, bool healthy);

  /**
   * @brief Update backend connection count
   */
  void update_backend_connections(const std::string& backend_id, int delta);

  /**
   * @brief Get all backends
   */
  [[nodiscard]] std::vector<Backend> get_backends() const;

  /**
   * @brief Get healthy backends
   */
  [[nodiscard]] std::vector<Backend> get_healthy_backends() const;

  /**
   * @brief Get load balancer statistics
   */
  [[nodiscard]] LoadBalancerStats get_statistics() const;

  /**
   * @brief Start health checking
   */
  void start_health_checks();

  /**
   * @brief Stop health checking
   */
  void stop_health_checks();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Performance
