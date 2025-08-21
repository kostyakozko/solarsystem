/**
 * @file network_resource_manager.hpp
 * @brief Network resource management system (Task 18)
 *
 * Implements requirements 8.3:
 * - Network connection pooling and cleanup
 * - Resource conflict detection and resolution
 */

#pragma once

#include "resource_manager.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace SolarSystem::Utils {

/**
 * @brief Network connection states
 */
enum class ConnectionState {
  Idle,
  Active,
  Connecting,
  Disconnecting,
  Failed,
  Closed
};

/**
 * @brief Network connection types
 */
enum class ConnectionType {
  HTTP,
  HTTPS,
  TCP,
  UDP,
  Custom
};

/**
 * @brief Network connection information
 */
struct NetworkConnectionInfo {
  std::string connection_id;
  std::string endpoint;
  ConnectionType type;
  ConnectionState state;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_used;
  size_t bytes_sent = 0;
  size_t bytes_received = 0;
  size_t request_count = 0;
  size_t error_count = 0;
  std::chrono::milliseconds total_response_time{0};
  std::string owner_id;
  void* connection_handle = nullptr;  // Platform-specific handle
};

/**
 * @brief Network operation statistics
 */
struct NetworkOperationStats {
  size_t total_connections_created = 0;
  size_t current_active_connections = 0;
  size_t peak_active_connections = 0;
  size_t total_requests_sent = 0;
  size_t total_responses_received = 0;
  size_t total_bytes_sent = 0;
  size_t total_bytes_received = 0;
  size_t connection_failures = 0;
  size_t timeout_errors = 0;
  std::chrono::milliseconds total_connection_time{0};
  std::chrono::milliseconds average_response_time{0};
};

/**
 * @brief Connection pool configuration
 */
struct ConnectionPoolConfig {
  size_t max_connections_per_host = 10;
  size_t max_total_connections = 100;
  std::chrono::seconds connection_timeout{30};
  std::chrono::seconds idle_timeout{300};  // 5 minutes
  std::chrono::seconds keep_alive_timeout{60};
  size_t max_retries = 3;
  std::chrono::milliseconds retry_delay{1000};
  bool enable_connection_reuse = true;
  bool enable_keep_alive = true;
  bool enable_compression = true;
};

/**
 * @brief Circuit breaker configuration
 */
struct CircuitBreakerConfig {
  size_t failure_threshold = 5;
  std::chrono::seconds timeout{60};
  std::chrono::seconds half_open_timeout{30};
  double failure_rate_threshold = 0.5;  // 50%
  size_t minimum_requests = 10;
};

/**
 * @brief Circuit breaker states
 */
enum class CircuitBreakerState {
  Closed,    // Normal operation
  Open,      // Failing, requests blocked
  HalfOpen   // Testing if service recovered
};

/**
 * @brief Circuit breaker for network failure management
 */
class NetworkCircuitBreaker {
public:
  explicit NetworkCircuitBreaker(const CircuitBreakerConfig& config = CircuitBreakerConfig{});

  bool can_execute() const;
  void record_success();
  void record_failure();
  void reset();

  CircuitBreakerState get_state() const { return state_; }
  size_t get_failure_count() const { return failure_count_; }
  double get_failure_rate() const;

private:
  mutable std::mutex mutex_;
  CircuitBreakerConfig config_;
  CircuitBreakerState state_ = CircuitBreakerState::Closed;
  size_t failure_count_ = 0;
  size_t success_count_ = 0;
  size_t total_requests_ = 0;
  std::chrono::system_clock::time_point last_failure_time_;
  std::chrono::system_clock::time_point state_change_time_;

  bool should_attempt_reset() const;
  void transition_to_open();
  void transition_to_half_open();
  void transition_to_closed();
};

/**
 * @brief RAII network connection wrapper
 */
class ManagedNetworkConnection {
public:
  ManagedNetworkConnection() = default;
  explicit ManagedNetworkConnection(const std::string& endpoint,
                                   ConnectionType type = ConnectionType::HTTP);
  ~ManagedNetworkConnection();

  // Disable copy, enable move
  ManagedNetworkConnection(const ManagedNetworkConnection&) = delete;
  ManagedNetworkConnection& operator=(const ManagedNetworkConnection&) = delete;
  ManagedNetworkConnection(ManagedNetworkConnection&& other) noexcept;
  ManagedNetworkConnection& operator=(ManagedNetworkConnection&& other) noexcept;

  // Connection management
  bool connect(std::chrono::seconds timeout = std::chrono::seconds(30));
  void disconnect();
  bool is_connected() const;
  bool is_healthy() const;

  // Data operations
  std::string send_request(const std::string& request,
                          std::chrono::seconds timeout = std::chrono::seconds(30));
  bool send_data(const std::vector<char>& data);
  std::vector<char> receive_data(size_t max_bytes = 8192);

  // Connection information
  const std::string& endpoint() const { return endpoint_; }
  ConnectionType type() const { return type_; }
  ConnectionState state() const { return state_; }
  const NetworkConnectionInfo& info() const { return info_; }

  // Statistics
  size_t bytes_sent() const { return info_.bytes_sent; }
  size_t bytes_received() const { return info_.bytes_received; }
  size_t request_count() const { return info_.request_count; }

private:
  std::string endpoint_;
  ConnectionType type_ = ConnectionType::HTTP;
  ConnectionState state_ = ConnectionState::Closed;
  NetworkConnectionInfo info_;
  std::string resource_id_;
  void* connection_handle_ = nullptr;

  void register_with_resource_manager();
  void unregister_from_resource_manager();
  void update_statistics(size_t bytes_sent, size_t bytes_received);
  std::string generate_connection_id() const;
};

/**
 * @brief Network connection pool manager
 */
class NetworkConnectionPool {
public:
  explicit NetworkConnectionPool(const ConnectionPoolConfig& config = ConnectionPoolConfig{});
  ~NetworkConnectionPool();

  // Connection management
  std::shared_ptr<ManagedNetworkConnection> acquire_connection(const std::string& endpoint,
                                                              ConnectionType type = ConnectionType::HTTP);
  void release_connection(std::shared_ptr<ManagedNetworkConnection> connection);
  void close_all_connections();

  // Pool management
  void cleanup_idle_connections();
  void cleanup_failed_connections();
  size_t get_active_connection_count() const;
  size_t get_idle_connection_count() const;

  // Configuration
  void update_config(const ConnectionPoolConfig& config);
  const ConnectionPoolConfig& get_config() const { return config_; }

  // Statistics
  NetworkOperationStats get_statistics() const;

private:
  mutable std::mutex pool_mutex_;
  ConnectionPoolConfig config_;
  std::unordered_map<std::string, std::vector<std::shared_ptr<ManagedNetworkConnection>>> idle_connections_;
  std::unordered_map<std::string, std::vector<std::shared_ptr<ManagedNetworkConnection>>> active_connections_;
  NetworkOperationStats stats_;

  std::string get_pool_key(const std::string& endpoint, ConnectionType type) const;
  bool can_create_new_connection(const std::string& endpoint) const;
  void update_pool_statistics();
};

/**
 * @brief Network resource manager
 */
class NetworkResourceManager {
public:
  static NetworkResourceManager& instance();

  // Connection management
  std::shared_ptr<ManagedNetworkConnection> create_connection(const std::string& endpoint,
                                                             ConnectionType type = ConnectionType::HTTP);

  std::shared_ptr<ManagedNetworkConnection> get_pooled_connection(const std::string& endpoint,
                                                                 ConnectionType type = ConnectionType::HTTP);

  // Circuit breaker management
  void register_circuit_breaker(const std::string& endpoint, const CircuitBreakerConfig& config = CircuitBreakerConfig{});
  NetworkCircuitBreaker* get_circuit_breaker(const std::string& endpoint);
  void remove_circuit_breaker(const std::string& endpoint);

  // Request management with resilience
  std::string make_resilient_request(const std::string& endpoint,
                                   const std::string& request,
                                   size_t max_retries = 3);

  // Resource monitoring
  NetworkOperationStats get_statistics() const;
  std::vector<NetworkConnectionInfo> get_active_connections() const;
  void generate_network_usage_report(std::ostream& output) const;

  // Configuration
  void configure_connection_pool(const ConnectionPoolConfig& config);
  void set_global_timeout(std::chrono::seconds timeout) { global_timeout_ = timeout; }
  void set_max_concurrent_connections(size_t max_connections) { max_concurrent_connections_ = max_connections; }

  // Cleanup operations
  void cleanup_expired_connections();
  void force_close_all_connections();
  void reset_all_circuit_breakers();

  // Health monitoring
  void start_health_monitoring();
  void stop_health_monitoring();
  bool is_endpoint_healthy(const std::string& endpoint) const;

private:
  NetworkResourceManager() = default;
  ~NetworkResourceManager();

  // Disable copy and move
  NetworkResourceManager(const NetworkResourceManager&) = delete;
  NetworkResourceManager& operator=(const NetworkResourceManager&) = delete;

  mutable std::mutex network_mutex_;
  std::unique_ptr<NetworkConnectionPool> connection_pool_;
  std::unordered_map<std::string, std::unique_ptr<NetworkCircuitBreaker>> circuit_breakers_;
  std::unordered_map<std::string, NetworkConnectionInfo> active_connections_;

  NetworkOperationStats stats_;
  std::chrono::seconds global_timeout_{30};
  size_t max_concurrent_connections_ = 100;

  // Health monitoring
  std::atomic<bool> health_monitoring_active_{false};
  std::unique_ptr<std::thread> health_monitoring_thread_;

  // Internal methods
  void health_monitoring_loop();
  void update_global_statistics();
  bool check_connection_limits() const;
  std::string generate_connection_id() const;
};

/**
 * @brief Network operation result with error information
 */
template<typename T>
class NetworkResult {
public:
  template<typename U = T, typename = std::enable_if_t<!std::is_same_v<U, std::string>>>
  NetworkResult(T value) : value_(std::move(value)), success_(true) {}

  NetworkResult(const char* error, int error_code = 0)
      : error_(error), error_code_(error_code), success_(false) {}
  NetworkResult(std::string error, int error_code = 0)
      : error_(std::move(error)), error_code_(error_code), success_(false) {}

  bool is_success() const { return success_; }
  const T& value() const { return value_; }
  const std::string& error() const { return error_; }
  int error_code() const { return error_code_; }

  explicit operator bool() const { return success_; }

private:
  T value_{};
  std::string error_;
  int error_code_ = 0;
  bool success_ = false;
};

/**
 * @brief Utility functions for network operations
 */
namespace NetworkUtils {
  // HTTP utilities
  NetworkResult<std::string> make_http_request(const std::string& url,
                                              const std::string& method = "GET",
                                              const std::string& data = "",
                                              const std::unordered_map<std::string, std::string>& headers = {});

  NetworkResult<std::string> download_file(const std::string& url, const std::string& output_path);

  // Connection utilities
  bool is_endpoint_reachable(const std::string& endpoint, std::chrono::seconds timeout = std::chrono::seconds(5));
  std::string resolve_hostname(const std::string& hostname);

  // URL utilities
  std::string encode_url(const std::string& url);
  std::string decode_url(const std::string& encoded_url);
  std::unordered_map<std::string, std::string> parse_query_string(const std::string& query);
}

/**
 * @brief Utility macros for network resource management
 */
#define SOLAR_NETWORK_CONNECTION(endpoint, type) \
  SolarSystem::Utils::NetworkResourceManager::instance().create_connection(endpoint, type)

#define SOLAR_POOLED_CONNECTION(endpoint, type) \
  SolarSystem::Utils::NetworkResourceManager::instance().get_pooled_connection(endpoint, type)

#define SOLAR_RESILIENT_REQUEST(endpoint, request, retries) \
  SolarSystem::Utils::NetworkResourceManager::instance().make_resilient_request(endpoint, request, retries)

} // namespace SolarSystem::Utils
