/**
 * @file network_mock.hpp
 * @brief Network Condition Mock System for Testing
 *
 * Provides comprehensive network mocking capabilities for network-dependent tests:
 * - Network condition simulation (slow, timeout, failure)
 * - HTTP request/response mocking
 * - Bandwidth and latency simulation
 * - Connection failure and retry testing
 * - Integration with existing network components
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace SolarSystem::Testing::Mocks {

/**
 * @brief Network operation types for tracking
 */
enum class MockNetworkOperationType {
  HttpGet,     // HTTP GET request
  HttpPost,    // HTTP POST request
  HttpPut,     // HTTP PUT request
  HttpDelete,  // HTTP DELETE request
  Connect,     // Connection establishment
  Disconnect,  // Connection termination
  Send,        // Data send operation
  Receive,     // Data receive operation
  Resolve,     // DNS resolution
  Timeout,     // Operation timeout
  Retry        // Retry operation
};

/**
 * @brief Network error types
 */
enum class MockNetworkError {
  Success,             // Operation successful
  ConnectionFailed,    // Failed to establish connection
  ConnectionTimeout,   // Connection timed out
  ConnectionReset,     // Connection reset by peer
  HostNotFound,        // DNS resolution failed
  NetworkUnreachable,  // Network is unreachable
  ServiceUnavailable,  // Service temporarily unavailable
  TooManyRequests,     // Rate limited (HTTP 429)
  ServerError,         // Server error (HTTP 5xx)
  ClientError,         // Client error (HTTP 4xx)
  SSLError,            // SSL/TLS error
  DataCorrupted,       // Data corruption during transfer
  PartialTransfer,     // Incomplete data transfer
  BandwidthExceeded,   // Bandwidth limit exceeded
  CustomError          // User-defined error
};

/**
 * @brief HTTP response structure
 */
struct MockHttpResponse {
  int status_code = 200;
  std::string status_message = "OK";
  std::map<std::string, std::string> headers;
  std::string body;
  std::chrono::milliseconds response_time = std::chrono::milliseconds(100);
  size_t bytes_transferred = 0;
};

/**
 * @brief Network call information for verification
 */
struct MockNetworkCallInfo {
  std::chrono::system_clock::time_point timestamp;
  MockNetworkOperationType operation_type;
  std::string url;
  std::string method;
  std::map<std::string, std::string> headers;
  std::string request_body;
  MockHttpResponse response;
  MockNetworkError error;
  std::chrono::milliseconds duration;
  size_t bytes_sent = 0;
  size_t bytes_received = 0;
  int retry_count = 0;
  std::string operation_details;
};

/**
 * @brief Network condition simulation parameters
 */
struct NetworkCondition {
  // Bandwidth simulation
  size_t bandwidth_bps = 1000000;  // Bytes per second
  bool simulate_bandwidth_limit = false;

  // Latency simulation
  std::chrono::milliseconds base_latency = std::chrono::milliseconds(50);
  std::chrono::milliseconds latency_variance = std::chrono::milliseconds(10);
  bool simulate_variable_latency = false;

  // Packet loss simulation
  double packet_loss_rate = 0.0;  // 0.0 = no loss, 1.0 = 100% loss
  bool simulate_packet_loss = false;

  // Connection stability
  double connection_drop_rate = 0.0;  // Rate of connection drops
  std::chrono::milliseconds connection_timeout = std::chrono::seconds(30);

  // Error rates
  double dns_failure_rate = 0.0;   // DNS resolution failure rate
  double server_error_rate = 0.0;  // Server error rate
  double timeout_rate = 0.0;       // Request timeout rate

  // Quality simulation
  std::string connection_type = "broadband";  // "dial-up", "broadband", "mobile", "satellite"
  bool simulate_congestion = false;           // Network congestion simulation
  double congestion_factor = 1.0;             // Congestion multiplier
};

/**
 * @brief Configuration for network mock behavior
 */
struct NetworkMockConfig {
  // Default network conditions
  NetworkCondition default_condition;

  // Response behavior
  std::chrono::milliseconds default_response_delay = std::chrono::milliseconds(100);
  std::chrono::milliseconds max_response_delay = std::chrono::seconds(30);
  size_t max_response_size = 10 * 1024 * 1024;  // 10MB

  // Retry behavior
  bool enable_automatic_retries = true;
  size_t max_retry_attempts = 3;
  std::chrono::milliseconds retry_delay = std::chrono::milliseconds(1000);
  double retry_backoff_factor = 2.0;

  // Connection pooling simulation
  size_t max_concurrent_connections = 10;
  std::chrono::milliseconds connection_reuse_timeout = std::chrono::minutes(5);
  bool simulate_connection_pooling = true;

  // SSL/TLS simulation
  bool simulate_ssl_handshake = false;
  std::chrono::milliseconds ssl_handshake_delay = std::chrono::milliseconds(200);
  double ssl_error_rate = 0.0;

  // Call tracking
  bool enable_call_history = true;
  size_t max_history_size = 10000;

  // Mock data
  std::string mock_data_directory = "tests/data/network_responses/";
  bool use_realistic_responses = true;

  // Performance simulation
  bool simulate_real_delays = true;   // Actually delay during network operations
  bool high_precision_timing = true;  // Use high precision timing
};

/**
 * @brief Network Mock Implementation
 *
 * Provides comprehensive mocking of network operations for testing purposes.
 * Can simulate various network conditions, connection failures, and response scenarios.
 */
class NetworkMock {
 public:
  /**
   * @brief Construct network mock with configuration
   */
  explicit NetworkMock(NetworkMockConfig config = {});

  /**
   * @brief Destructor
   */
  ~NetworkMock();

  // Non-copyable, non-movable (due to mutex and thread management)
  NetworkMock(const NetworkMock&) = delete;
  NetworkMock& operator=(const NetworkMock&) = delete;
  NetworkMock(NetworkMock&&) = delete;
  NetworkMock& operator=(NetworkMock&&) = delete;

  // === Network Condition Control ===

  /**
   * @brief Set network condition for all requests
   */
  void set_network_condition(const NetworkCondition& condition);

  /**
   * @brief Set network condition for specific URL pattern
   */
  void set_network_condition_for_url(const std::string& url_pattern,
                                     const NetworkCondition& condition);

  /**
   * @brief Simulate slow network (low bandwidth)
   */
  void simulate_slow_network(size_t bandwidth_bps = 56000);  // 56k modem speed

  /**
   * @brief Simulate high latency network
   */
  void simulate_high_latency(std::chrono::milliseconds latency);

  /**
   * @brief Simulate unstable network with packet loss
   */
  void simulate_unstable_network(double packet_loss_rate = 0.1);

  /**
   * @brief Simulate network congestion
   */
  void simulate_network_congestion(double congestion_factor = 3.0);

  /**
   * @brief Simulate mobile network conditions
   */
  void simulate_mobile_network();

  /**
   * @brief Simulate satellite network conditions
   */
  void simulate_satellite_network();

  /**
   * @brief Reset to normal network conditions
   */
  void reset_to_normal_conditions();

  // === Connection Control ===

  /**
   * @brief Simulate connection failure for next N requests
   */
  void simulate_connection_failure(size_t request_count = 1);

  /**
   * @brief Simulate connection timeout for next N requests
   */
  void simulate_connection_timeout(size_t request_count = 1);

  /**
   * @brief Simulate connection reset for next N requests
   */
  void simulate_connection_reset(size_t request_count = 1);

  /**
   * @brief Simulate DNS resolution failure
   */
  void simulate_dns_failure(size_t request_count = 1);

  /**
   * @brief Simulate server unavailable
   */
  void simulate_server_unavailable(size_t request_count = 1);

  /**
   * @brief Simulate rate limiting (HTTP 429)
   */
  void simulate_rate_limiting(size_t request_count = 1,
                              std::chrono::milliseconds retry_after = std::chrono::seconds(60));

  // === Response Configuration ===

  /**
   * @brief Set response for specific URL
   */
  void set_response_for_url(const std::string& url, const MockHttpResponse& response);

  /**
   * @brief Set response for URL pattern
   */
  void set_response_for_pattern(const std::string& url_pattern, const MockHttpResponse& response);

  /**
   * @brief Set default response for all requests
   */
  void set_default_response(const MockHttpResponse& response);

  /**
   * @brief Load mock responses from directory
   */
  bool load_mock_responses_from_directory(const std::string& directory_path);

  /**
   * @brief Set custom error response
   */
  void set_error_response(MockNetworkError error_type, const MockHttpResponse& response);

  // === HTTP Mock Interface ===

  /**
   * @brief Mock HTTP GET request
   */
  [[nodiscard]] MockHttpResponse mock_http_get(
      const std::string& url, const std::map<std::string, std::string>& headers = {});

  /**
   * @brief Mock HTTP POST request
   */
  [[nodiscard]] MockHttpResponse mock_http_post(
      const std::string& url, const std::string& body,
      const std::map<std::string, std::string>& headers = {});

  /**
   * @brief Mock HTTP PUT request
   */
  [[nodiscard]] MockHttpResponse mock_http_put(
      const std::string& url, const std::string& body,
      const std::map<std::string, std::string>& headers = {});

  /**
   * @brief Mock HTTP DELETE request
   */
  [[nodiscard]] MockHttpResponse mock_http_delete(
      const std::string& url, const std::map<std::string, std::string>& headers = {});

  /**
   * @brief Mock generic HTTP request
   */
  [[nodiscard]] MockHttpResponse mock_http_request(
      const std::string& method, const std::string& url, const std::string& body = "",
      const std::map<std::string, std::string>& headers = {});

  // === Connection Management ===

  /**
   * @brief Mock connection establishment
   */
  [[nodiscard]] bool mock_connect(const std::string& host, int port);

  /**
   * @brief Mock connection termination
   */
  void mock_disconnect(const std::string& host, int port);

  /**
   * @brief Check if connection is active
   */
  [[nodiscard]] bool is_connected(const std::string& host, int port) const;

  /**
   * @brief Get number of active connections
   */
  [[nodiscard]] size_t active_connection_count() const;

  // === Data Transfer Simulation ===

  /**
   * @brief Simulate data send operation
   */
  [[nodiscard]] size_t mock_send_data(const std::string& data);

  /**
   * @brief Simulate data receive operation
   */
  [[nodiscard]] std::string mock_receive_data(size_t max_bytes);

  /**
   * @brief Calculate transfer time based on bandwidth
   */
  [[nodiscard]] std::chrono::milliseconds calculate_transfer_time(size_t bytes) const;

  /**
   * @brief Simulate bandwidth-limited transfer
   */
  void simulate_bandwidth_limited_transfer(size_t bytes) const;

  // === Call Verification ===

  /**
   * @brief Get total number of network operations
   */
  [[nodiscard]] size_t operation_count() const;

  /**
   * @brief Get number of operations by type
   */
  [[nodiscard]] size_t operation_count(MockNetworkOperationType type) const;

  /**
   * @brief Get complete operation history
   */
  [[nodiscard]] const std::vector<MockNetworkCallInfo>& operation_history() const;

  /**
   * @brief Get operations of specific type
   */
  [[nodiscard]] std::vector<MockNetworkCallInfo> operations_of_type(
      MockNetworkOperationType type) const;

  /**
   * @brief Get operations for specific URL
   */
  [[nodiscard]] std::vector<MockNetworkCallInfo> operations_for_url(const std::string& url) const;

  /**
   * @brief Check if URL was requested
   */
  [[nodiscard]] bool was_url_requested(const std::string& url) const;

  /**
   * @brief Get total bytes transferred
   */
  [[nodiscard]] size_t total_bytes_transferred() const;

  /**
   * @brief Get total network operation time
   */
  [[nodiscard]] std::chrono::milliseconds total_operation_time() const;

  /**
   * @brief Reset call history and counters
   */
  void reset_call_history();

  // === Configuration Access ===

  /**
   * @brief Get current configuration
   */
  [[nodiscard]] const NetworkMockConfig& config() const noexcept { return config_; }

  /**
   * @brief Update configuration
   */
  void update_config(const NetworkMockConfig& new_config);

  /**
   * @brief Get current network condition
   */
  [[nodiscard]] const NetworkCondition& current_condition() const;

  // === Utility Functions ===

  /**
   * @brief Install this mock as global network mock
   */
  void install_as_global_mock();

  /**
   * @brief Remove global mock installation
   */
  static void remove_global_mock();

  /**
   * @brief Create network test scenario
   */
  void create_test_scenario(const std::string& scenario_name, const NetworkCondition& condition,
                            std::chrono::duration<double> duration);

  /**
   * @brief Execute function with network condition
   */
  template <typename Func>
  auto execute_with_network_condition(const NetworkCondition& condition, Func&& func)
      -> decltype(func());

 private:
  NetworkMockConfig config_;

  // Network state
  mutable std::mutex network_mutex_;
  NetworkCondition current_condition_;
  std::map<std::string, NetworkCondition> url_conditions_;

  // Connection tracking
  std::map<std::pair<std::string, int>, std::chrono::system_clock::time_point> active_connections_;
  std::atomic<size_t> connection_counter_;

  // Response storage
  std::map<std::string, MockHttpResponse> url_responses_;
  std::map<std::string, MockHttpResponse> pattern_responses_;
  MockHttpResponse default_response_;
  std::map<MockNetworkError, MockHttpResponse> error_responses_;

  // Error simulation state
  mutable std::atomic<size_t> remaining_connection_failures_;
  mutable std::atomic<size_t> remaining_timeouts_;
  mutable std::atomic<size_t> remaining_resets_;
  mutable std::atomic<size_t> remaining_dns_failures_;
  mutable std::atomic<size_t> remaining_server_unavailable_;
  mutable std::atomic<size_t> remaining_rate_limits_;

  // Call tracking
  mutable std::mutex call_mutex_;
  std::vector<MockNetworkCallInfo> operation_history_;
  std::map<MockNetworkOperationType, size_t> operation_counts_;
  std::atomic<size_t> total_bytes_sent_;
  std::atomic<size_t> total_bytes_received_;
  std::atomic<std::chrono::milliseconds::rep> total_operation_time_;

  // Random number generation for simulation
  mutable std::random_device rd_;
  mutable std::mt19937 gen_;
  mutable std::uniform_real_distribution<double> error_dist_;
  mutable std::uniform_int_distribution<int> latency_dist_;

  // Bandwidth simulation
  mutable std::mutex bandwidth_mutex_;
  mutable std::queue<std::pair<std::chrono::steady_clock::time_point, size_t>> bandwidth_queue_;

  // === Internal Helper Methods ===

  /**
   * @brief Record a network operation for verification
   */
  void record_operation(MockNetworkOperationType type, const std::string& url,
                        const std::string& method,
                        const std::map<std::string, std::string>& headers,
                        const std::string& request_body, const MockHttpResponse& response,
                        MockNetworkError error, std::chrono::milliseconds duration,
                        const std::string& details = "");

  /**
   * @brief Determine network condition for URL
   */
  [[nodiscard]] NetworkCondition get_condition_for_url(const std::string& url) const;

  /**
   * @brief Check if should simulate error
   */
  [[nodiscard]] MockNetworkError check_for_simulated_errors() const;

  /**
   * @brief Simulate network latency
   */
  void simulate_network_latency(const NetworkCondition& condition) const;

  /**
   * @brief Simulate packet loss
   */
  [[nodiscard]] bool simulate_packet_loss(const NetworkCondition& condition) const;

  /**
   * @brief Find response for URL
   */
  [[nodiscard]] std::optional<MockHttpResponse> find_response_for_url(const std::string& url) const;

  /**
   * @brief Generate realistic response
   */
  [[nodiscard]] MockHttpResponse generate_realistic_response(const std::string& url,
                                                             const std::string& method) const;

  /**
   * @brief Parse URL components
   */
  [[nodiscard]] std::tuple<std::string, std::string, int, std::string> parse_url(
      const std::string& url) const;

  /**
   * @brief Check URL pattern match
   */
  [[nodiscard]] bool matches_pattern(const std::string& url, const std::string& pattern) const;

  /**
   * @brief Update bandwidth tracking
   */
  void update_bandwidth_tracking(size_t bytes) const;

  /**
   * @brief Calculate current bandwidth usage
   */
  [[nodiscard]] size_t calculate_current_bandwidth_usage() const;

  /**
   * @brief Generate connection key
   */
  [[nodiscard]] std::pair<std::string, int> make_connection_key(const std::string& host,
                                                                int port) const;
};

/**
 * @brief Factory for creating network mocks
 */
class NetworkMockFactory {
 public:
  /**
   * @brief Create default network mock
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create_default();

  /**
   * @brief Create network mock with custom configuration
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create(NetworkMockConfig config);

  /**
   * @brief Create network mock for connection failure testing
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create_for_connection_failure_testing();

  /**
   * @brief Create network mock for slow network testing
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create_for_slow_network_testing();

  /**
   * @brief Create network mock for timeout testing
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create_for_timeout_testing();

  /**
   * @brief Create network mock for mobile network simulation
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create_for_mobile_network();

  /**
   * @brief Create network mock for satellite network simulation
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create_for_satellite_network();

  /**
   * @brief Create network mock with perfect conditions
   */
  [[nodiscard]] static std::unique_ptr<NetworkMock> create_with_perfect_conditions();
};

/**
 * @brief RAII helper for installing/removing global network mock
 */
class ScopedNetworkMock {
 public:
  explicit ScopedNetworkMock(std::unique_ptr<NetworkMock> mock);
  ~ScopedNetworkMock();

  // Non-copyable, non-movable
  ScopedNetworkMock(const ScopedNetworkMock&) = delete;
  ScopedNetworkMock& operator=(const ScopedNetworkMock&) = delete;
  ScopedNetworkMock(ScopedNetworkMock&&) = delete;
  ScopedNetworkMock& operator=(ScopedNetworkMock&&) = delete;

  /**
   * @brief Get access to the mock
   */
  [[nodiscard]] NetworkMock& mock() { return *mock_; }
  [[nodiscard]] const NetworkMock& mock() const { return *mock_; }

 private:
  std::unique_ptr<NetworkMock> mock_;
};

/**
 * @brief Network-based test utilities
 */
class NetworkTestUtils {
 public:
  /**
   * @brief Test network resilience with various conditions
   */
  static bool test_network_resilience(std::function<bool()> network_operation,
                                      const std::vector<NetworkCondition>& conditions,
                                      size_t max_retries = 3);

  /**
   * @brief Measure network operation performance
   */
  template <typename Func>
  static std::chrono::milliseconds measure_network_operation_time(Func&& func);

  /**
   * @brief Create network condition sequence for testing
   */
  static std::vector<NetworkCondition> create_network_condition_sequence(
      const NetworkCondition& start_condition, const NetworkCondition& end_condition, size_t steps);

  /**
   * @brief Validate network error handling
   */
  static bool validate_network_error_handling(std::function<bool(MockNetworkError)> error_handler,
                                              const std::vector<MockNetworkError>& error_types);
};

}  // namespace SolarSystem::Testing::Mocks
