/**
 * @file connection_pool.hpp
 * @brief HTTP connection pooling and keep-alive management
 *
 * Provides connection pooling with:
 * - Connection reuse
 * - Keep-alive support
 * - Connection timeout management
 * - Health checking
 */

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace SolarSystem::Performance {

/**
 * @brief Connection state
 */
enum class ConnectionState {
  IDLE,
  ACTIVE,
  CLOSING,
  CLOSED
};

/**
 * @brief HTTP connection
 */
struct HttpConnection {
  int socket_fd = -1;
  std::string client_ip;
  ConnectionState state = ConnectionState::IDLE;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_used;
  size_t requests_handled = 0;
  bool keep_alive = true;
};

/**
 * @brief Connection pool configuration
 */
struct ConnectionPoolConfig {
  size_t max_connections = 100;
  size_t max_idle_connections = 20;
  std::chrono::seconds idle_timeout{30};
  std::chrono::seconds connection_timeout{60};
  size_t max_requests_per_connection = 100;
  bool enable_keep_alive = true;
};

/**
 * @brief Connection pool statistics
 */
struct ConnectionPoolStats {
  size_t total_connections = 0;
  size_t active_connections = 0;
  size_t idle_connections = 0;
  size_t total_requests = 0;
  size_t reused_connections = 0;
  double reuse_rate = 0.0;
};

/**
 * @brief HTTP connection pool
 */
class HttpConnectionPool {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit HttpConnectionPool(ConnectionPoolConfig config = {});

  /**
   * @brief Destructor
   */
  ~HttpConnectionPool();

  // Non-copyable, movable
  HttpConnectionPool(const HttpConnectionPool&) = delete;
  HttpConnectionPool& operator=(const HttpConnectionPool&) = delete;
  HttpConnectionPool(HttpConnectionPool&&) noexcept;
  HttpConnectionPool& operator=(HttpConnectionPool&&) noexcept;

  /**
   * @brief Acquire connection from pool
   */
  [[nodiscard]] std::optional<HttpConnection> acquire();

  /**
   * @brief Release connection back to pool
   */
  void release(const HttpConnection& connection);

  /**
   * @brief Close connection
   */
  void close(const HttpConnection& connection);

  /**
   * @brief Cleanup idle connections
   */
  void cleanup_idle();

  /**
   * @brief Get pool statistics
   */
  [[nodiscard]] ConnectionPoolStats get_statistics() const;

  /**
   * @brief Get active connection count
   */
  [[nodiscard]] size_t get_active_count() const;

  /**
   * @brief Get idle connection count
   */
  [[nodiscard]] size_t get_idle_count() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Performance
