/**
 * @file jpl_mock.hpp
 * @brief JPL API Mock System for Testing
 *
 * Provides comprehensive mocking capabilities for JPL HORIZONS API testing:
 * - Configurable response simulation
 * - Network delay and failure simulation
 * - Realistic JPL HORIZONS response generation
 * - Call verification and history tracking
 * - Integration with existing JPL client interface
 */

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "solar_jpl/jpl_client.hpp"

namespace SolarSystem::Testing::Mocks {

/**
 * @brief Mock response types for different scenarios
 */
enum class MockResponseType {
  Success,        // Valid JPL response
  NetworkError,   // Network connection failure
  Timeout,        // Request timeout
  ServerError,    // HTTP 500 error
  InvalidBody,    // Body not found
  ParseError,     // Malformed response
  RateLimited,    // Too many requests
  CustomResponse  // User-defined response
};

/**
 * @brief Mock call information for verification
 */
struct MockCallInfo {
  std::chrono::system_clock::time_point timestamp;
  int jpl_id;
  std::chrono::system_clock::time_point requested_epoch;
  std::string request_params;
  MockResponseType response_type;
  std::chrono::milliseconds response_delay;
  bool was_successful;
};

/**
 * @brief Configuration for JPL mock behavior
 */
struct JPLMockConfig {
  // Network simulation
  bool simulate_network_delays = false;
  std::chrono::milliseconds min_response_delay = std::chrono::milliseconds(50);
  std::chrono::milliseconds max_response_delay = std::chrono::milliseconds(500);

  // Failure simulation
  double network_failure_rate = 0.0;  // 0.0 = never fail, 1.0 = always fail
  double timeout_rate = 0.0;          // Rate of timeout failures
  double server_error_rate = 0.0;     // Rate of server errors
  double parse_error_rate = 0.0;      // Rate of parse errors

  // Response behavior
  bool use_realistic_responses = true;  // Generate realistic JPL responses
  std::string mock_data_directory = "tests/data/jpl_responses/";
  bool enable_call_history = true;  // Track all calls for verification
  size_t max_history_size = 1000;   // Maximum number of calls to remember

  // Rate limiting simulation
  bool simulate_rate_limiting = false;
  std::chrono::milliseconds rate_limit_delay = std::chrono::milliseconds(200);
  size_t max_concurrent_requests = 3;

  // Cache simulation
  bool simulate_cache_operations = true;
  bool cache_always_valid = false;
  bool cache_always_invalid = false;
};

/**
 * @brief JPL API Mock Implementation
 *
 * Provides comprehensive mocking of JPL HORIZONS API for testing purposes.
 * Can simulate various network conditions, response types, and failure scenarios.
 */
class JPLMock {
 public:
  /**
   * @brief Construct JPL mock with configuration
   */
  explicit JPLMock(JPLMockConfig config = {});

  /**
   * @brief Destructor
   */
  ~JPLMock() = default;

  // Non-copyable, non-movable (due to mutex)
  JPLMock(const JPLMock&) = delete;
  JPLMock& operator=(const JPLMock&) = delete;
  JPLMock(JPLMock&&) = delete;
  JPLMock& operator=(JPLMock&&) = delete;

  // === Response Configuration ===

  /**
   * @brief Set specific response for a JPL body ID
   */
  void set_response_for_body(int jpl_id, const std::string& response);

  /**
   * @brief Set response type for a specific body
   */
  void set_response_type_for_body(int jpl_id, MockResponseType type);

  /**
   * @brief Set default response type for all bodies
   */
  void set_default_response_type(MockResponseType type);

  /**
   * @brief Set custom error response
   */
  void set_error_response(int http_code, const std::string& error_message);

  /**
   * @brief Load mock responses from directory
   */
  bool load_mock_responses_from_directory(const std::string& directory_path);

  // === Network Simulation ===

  /**
   * @brief Simulate network failure for next N requests
   */
  void simulate_network_failure(size_t request_count = 1);

  /**
   * @brief Simulate timeout for next N requests
   */
  void simulate_timeout(size_t request_count = 1);

  /**
   * @brief Simulate server error for next N requests
   */
  void simulate_server_error(size_t request_count = 1);

  /**
   * @brief Set network delay range
   */
  void set_network_delay_range(std::chrono::milliseconds min_delay,
                               std::chrono::milliseconds max_delay);

  /**
   * @brief Enable/disable network delay simulation
   */
  void enable_network_delays(bool enabled);

  // === Realistic Response Generation ===

  /**
   * @brief Generate realistic JPL HORIZONS response for body
   */
  [[nodiscard]] std::string generate_realistic_response(
      int jpl_id, std::chrono::system_clock::time_point epoch) const;

  /**
   * @brief Generate ephemeris data section
   */
  [[nodiscard]] std::string generate_ephemeris_section(
      int jpl_id, std::chrono::system_clock::time_point epoch) const;

  /**
   * @brief Generate body information section
   */
  [[nodiscard]] std::string generate_body_info_section(int jpl_id) const;

  // === Call Verification ===

  /**
   * @brief Get total number of calls made
   */
  [[nodiscard]] size_t call_count() const;

  /**
   * @brief Get number of calls for specific body
   */
  [[nodiscard]] size_t call_count_for_body(int jpl_id) const;

  /**
   * @brief Get list of all requested body IDs
   */
  [[nodiscard]] std::vector<int> requested_bodies() const;

  /**
   * @brief Get complete call history
   */
  [[nodiscard]] const std::vector<MockCallInfo>& call_history() const;

  /**
   * @brief Get calls for specific body
   */
  [[nodiscard]] std::vector<MockCallInfo> calls_for_body(int jpl_id) const;

  /**
   * @brief Check if body was requested
   */
  [[nodiscard]] bool was_body_requested(int jpl_id) const;

  /**
   * @brief Get last call information
   */
  [[nodiscard]] std::optional<MockCallInfo> last_call() const;

  /**
   * @brief Reset call history and counters
   */
  void reset_call_history();

  // === Mock Interface Implementation ===

  /**
   * @brief Mock implementation of JPL API request
   * This is called by the mocked JPL client
   */
  [[nodiscard]] SolarSystem::JPL::JPLResult<std::string> mock_request(const std::string& url,
                                                                      const std::string& params);

  /**
   * @brief Mock implementation of cache operations
   */
  [[nodiscard]] SolarSystem::JPL::JPLResult<std::vector<SolarSystem::JPL::EphemerisData>>
  mock_load_from_cache();

  /**
   * @brief Mock implementation of cache save
   */
  [[nodiscard]] SolarSystem::JPL::JPLVoidResult mock_save_to_cache(
      const std::vector<SolarSystem::JPL::EphemerisData>& data);

  /**
   * @brief Mock implementation of cache validation
   */
  [[nodiscard]] SolarSystem::JPL::JPLResult<bool> mock_validate_cache() const;

  // === Configuration Access ===

  /**
   * @brief Get current configuration
   */
  [[nodiscard]] const JPLMockConfig& config() const noexcept { return config_; }

  /**
   * @brief Update configuration
   */
  void update_config(const JPLMockConfig& new_config);

  // === Utility Functions ===

  /**
   * @brief Create mock JPL client that uses this mock
   */
  [[nodiscard]] std::unique_ptr<SolarSystem::JPL::JPLClient> create_mock_client();

  /**
   * @brief Install this mock as global JPL mock (for dependency injection)
   */
  void install_as_global_mock();

  /**
   * @brief Remove global mock installation
   */
  static void remove_global_mock();

 private:
  JPLMockConfig config_;

  // Response storage
  std::unordered_map<int, std::string> body_responses_;
  std::unordered_map<int, MockResponseType> body_response_types_;
  MockResponseType default_response_type_ = MockResponseType::Success;
  std::string custom_error_response_;
  int custom_error_code_ = 500;

  // Call tracking
  mutable std::mutex call_mutex_;
  std::vector<MockCallInfo> call_history_;
  std::unordered_map<int, size_t> body_call_counts_;

  // Network simulation state
  size_t remaining_network_failures_ = 0;
  size_t remaining_timeouts_ = 0;
  size_t remaining_server_errors_ = 0;

  // Random number generation for simulation
  mutable std::random_device rd_;
  mutable std::mt19937 gen_;
  mutable std::uniform_real_distribution<double> failure_dist_;

  // Cache simulation data
  std::vector<SolarSystem::JPL::EphemerisData> mock_cache_data_;
  bool cache_has_data_ = false;

  // === Internal Helper Methods ===

  /**
   * @brief Record a mock call for verification
   */
  void record_call(int jpl_id, std::chrono::system_clock::time_point epoch,
                   const std::string& params, MockResponseType response_type,
                   std::chrono::milliseconds delay, bool successful);

  /**
   * @brief Determine response type for a request
   */
  [[nodiscard]] MockResponseType determine_response_type(int jpl_id) const;

  /**
   * @brief Simulate network delay
   */
  void simulate_network_delay() const;

  /**
   * @brief Check if should simulate failure
   */
  [[nodiscard]] bool should_simulate_failure(double failure_rate) const;

  /**
   * @brief Parse JPL ID from request parameters
   */
  [[nodiscard]] int parse_jpl_id_from_params(const std::string& params) const;

  /**
   * @brief Parse epoch from request parameters
   */
  [[nodiscard]] std::chrono::system_clock::time_point parse_epoch_from_params(
      const std::string& params) const;

  /**
   * @brief Generate error response
   */
  [[nodiscard]] std::string generate_error_response(MockResponseType error_type) const;

  /**
   * @brief Get body name for JPL ID
   */
  [[nodiscard]] std::string get_body_name_for_jpl_id(int jpl_id) const;

  /**
   * @brief Generate realistic position/velocity data
   */
  [[nodiscard]] SolarSystem::Math::Vector3d generate_realistic_position(
      int jpl_id, std::chrono::system_clock::time_point epoch) const;

  [[nodiscard]] SolarSystem::Math::Vector3d generate_realistic_velocity(
      int jpl_id, std::chrono::system_clock::time_point epoch) const;

  /**
   * @brief Get realistic mass for body
   */
  [[nodiscard]] long double get_realistic_mass(int jpl_id) const;
};

/**
 * @brief Factory for creating JPL mocks
 */
class JPLMockFactory {
 public:
  /**
   * @brief Create default JPL mock
   */
  [[nodiscard]] static std::unique_ptr<JPLMock> create_default();

  /**
   * @brief Create JPL mock with custom configuration
   */
  [[nodiscard]] static std::unique_ptr<JPLMock> create(JPLMockConfig config);

  /**
   * @brief Create JPL mock for network failure testing
   */
  [[nodiscard]] static std::unique_ptr<JPLMock> create_for_network_failure_testing();

  /**
   * @brief Create JPL mock for performance testing
   */
  [[nodiscard]] static std::unique_ptr<JPLMock> create_for_performance_testing();

  /**
   * @brief Create JPL mock with realistic responses
   */
  [[nodiscard]] static std::unique_ptr<JPLMock> create_with_realistic_responses();
};

/**
 * @brief RAII helper for installing/removing global mock
 */
class ScopedJPLMock {
 public:
  explicit ScopedJPLMock(std::unique_ptr<JPLMock> mock);
  ~ScopedJPLMock();

  // Non-copyable, non-movable
  ScopedJPLMock(const ScopedJPLMock&) = delete;
  ScopedJPLMock& operator=(const ScopedJPLMock&) = delete;
  ScopedJPLMock(ScopedJPLMock&&) = delete;
  ScopedJPLMock& operator=(ScopedJPLMock&&) = delete;

  /**
   * @brief Get access to the mock
   */
  [[nodiscard]] JPLMock& mock() { return *mock_; }
  [[nodiscard]] const JPLMock& mock() const { return *mock_; }

 private:
  std::unique_ptr<JPLMock> mock_;
};

}  // namespace SolarSystem::Testing::Mocks
