/**
 * @file connection_manager.hpp
 * @brief Robust connection management for real-time monitoring
 *
 * Provides comprehensive connection management with:
 * - Connection health monitoring and diagnostics
 * - Automatic reconnection with exponential backoff
 * - Connection pooling and load balancing
 * - Graceful degradation for connection failures
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/export.hpp"

namespace SolarSystem::Connection {

/**
 * @brief Connection state enumeration
 */
enum class ConnectionState {
  DISCONNECTED,   ///< Not connected
  CONNECTING,     ///< Connection in progress
  CONNECTED,      ///< Successfully connected
  RECONNECTING,   ///< Attempting to reconnect
  DEGRADED,       ///< Connected but with reduced functionality
  FAILED          ///< Connection failed permanently
};

/**
 * @brief Convert connection state to string
 */
[[nodiscard]] SOLAR_CORE_API std::string to_string(ConnectionState state);

/**
 * @brief Connection health status
 */
struct ConnectionHealth {
  ConnectionState state = ConnectionState::DISCONNECTED;
  std::chrono::system_clock::time_point last_successful_connection;
  std::chrono::system_clock::time_point last_connection_attempt;
  std::chrono::milliseconds average_latency{0};
  size_t successful_operations = 0;
  size_t failed_operations = 0;
  size_t reconnection_attempts = 0;
  double health_score = 0.0;  ///< 0.0 to 1.0
  std::string last_error;
};

/**
 * @brief Connection configuration
 */
struct ConnectionConfig {
  std::chrono::seconds connection_timeout{30};
  std::chrono::seconds health_check_interval{10};
  size_t max_reconnection_attempts = 10;
  std::chrono::milliseconds initial_backoff{100};
  std::chrono::milliseconds max_backoff{30000};
  double backoff_multiplier = 2.0;
  bool enable_auto_reconnect = true;
  bool enable_health_monitoring = true;
  double min_health_score = 0.5;
};

/**
 * @brief Connection interface
 */
class IConnection {
 public:
  virtual ~IConnection() = default;

  /**
   * @brief Connect to the data source
   */
  [[nodiscard]] virtual bool connect() = 0;

  /**
   * @brief Disconnect from the data source
   */
  virtual void disconnect() = 0;

  /**
   * @brief Check if connected
   */
  [[nodiscard]] virtual bool is_connected() const = 0;

  /**
   * @brief Perform health check
   */
  [[nodiscard]] virtual bool health_check() = 0;

  /**
   * @brief Get connection health status
   */
  [[nodiscard]] virtual ConnectionHealth get_health() const = 0;

  /**
   * @brief Get connection identifier
   */
  [[nodiscard]] virtual std::string get_id() const = 0;
};

/**
 * @brief Connection manager with robust reconnection and health monitoring
 */
class SOLAR_CORE_API ConnectionManager {
 public:
  /**
   * @brief Construct connection manager with configuration
   */
  explicit ConnectionManager(ConnectionConfig config = {});

  /**
   * @brief Destructor - ensures proper cleanup
   */
  ~ConnectionManager();

  // Non-copyable, movable
  ConnectionManager(const ConnectionManager&) = delete;
  ConnectionManager& operator=(const ConnectionManager&) = delete;
  ConnectionManager(ConnectionManager&&) noexcept;
  ConnectionManager& operator=(ConnectionManager&&) noexcept;

  /**
   * @brief Add a connection to manage
   */
  void add_connection(std::shared_ptr<IConnection> connection);

  /**
   * @brief Remove a connection
   */
  void remove_connection(const std::string& connection_id);

  /**
   * @brief Start connection management
   */
  [[nodiscard]] bool start();

  /**
   * @brief Stop connection management
   */
  void stop();

  /**
   * @brief Check if manager is running
   */
  [[nodiscard]] bool is_running() const;

  /**
   * @brief Get overall connection health
   */
  [[nodiscard]] ConnectionHealth get_overall_health() const;

  /**
   * @brief Get health for specific connection
   */
  [[nodiscard]] std::optional<ConnectionHealth> get_connection_health(
      const std::string& connection_id) const;

  /**
   * @brief Get all connection healths
   */
  [[nodiscard]] std::vector<ConnectionHealth> get_all_health() const;

  /**
   * @brief Force reconnection of all connections
   */
  void reconnect_all();

  /**
   * @brief Force reconnection of specific connection
   */
  void reconnect(const std::string& connection_id);

  /**
   * @brief Set connection state callback
   */
  using StateCallback = std::function<void(const std::string&, ConnectionState, ConnectionState)>;
  void set_state_callback(StateCallback callback);

  /**
   * @brief Set health alert callback
   */
  using HealthAlertCallback = std::function<void(const std::string&, const ConnectionHealth&)>;
  void set_health_alert_callback(HealthAlertCallback callback);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Exponential backoff calculator
 */
class SOLAR_CORE_API ExponentialBackoff {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit ExponentialBackoff(
      std::chrono::milliseconds initial_delay = std::chrono::milliseconds{100},
      std::chrono::milliseconds max_delay = std::chrono::milliseconds{30000},
      double multiplier = 2.0);

  /**
   * @brief Get next backoff delay
   */
  [[nodiscard]] std::chrono::milliseconds next_delay();

  /**
   * @brief Reset backoff to initial delay
   */
  void reset();

  /**
   * @brief Get current attempt number
   */
  [[nodiscard]] size_t get_attempt_count() const;

 private:
  std::chrono::milliseconds initial_delay_;
  std::chrono::milliseconds max_delay_;
  std::chrono::milliseconds current_delay_;
  double multiplier_;
  size_t attempt_count_ = 0;
};

/**
 * @brief Connection pool for load balancing
 */
class SOLAR_CORE_API ConnectionPool {
 public:
  /**
   * @brief Construct connection pool
   */
  explicit ConnectionPool(size_t max_connections = 10);

  /**
   * @brief Destructor
   */
  ~ConnectionPool();

  // Non-copyable, movable
  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;
  ConnectionPool(ConnectionPool&&) noexcept;
  ConnectionPool& operator=(ConnectionPool&&) noexcept;

  /**
   * @brief Add connection to pool
   */
  void add_connection(std::shared_ptr<IConnection> connection);

  /**
   * @brief Get next available connection (round-robin)
   */
  [[nodiscard]] std::shared_ptr<IConnection> get_connection();

  /**
   * @brief Get connection by ID
   */
  [[nodiscard]] std::shared_ptr<IConnection> get_connection(const std::string& id);

  /**
   * @brief Get all connections
   */
  [[nodiscard]] std::vector<std::shared_ptr<IConnection>> get_all_connections() const;

  /**
   * @brief Get number of healthy connections
   */
  [[nodiscard]] size_t get_healthy_count() const;

  /**
   * @brief Get total number of connections
   */
  [[nodiscard]] size_t get_total_count() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Connection
