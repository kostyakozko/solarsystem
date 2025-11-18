/**
 * @file network_mock.cpp
 * @brief Network Mock Implementation
 */

#include "solar_test/mocks/network_mock.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>
#include <thread>

namespace SolarSystem::Testing::Mocks {

namespace {
// Global network mock instance for dependency injection
NetworkMock* g_global_network_mock = nullptr;
std::mutex g_global_mock_mutex;

// Default network conditions for different connection types
const std::map<std::string, NetworkCondition> kConnectionTypeDefaults = {
    {"dial-up",
     {56000, true, std::chrono::milliseconds(200), std::chrono::milliseconds(50), true, 0.05,
      true}},
    {"broadband",
     {1000000, false, std::chrono::milliseconds(50), std::chrono::milliseconds(10), false, 0.001,
      false}},
    {"mobile",
     {500000, true, std::chrono::milliseconds(150), std::chrono::milliseconds(100), true, 0.02,
      true}},
    {"satellite",
     {1000000, true, std::chrono::milliseconds(600), std::chrono::milliseconds(200), true, 0.01,
      true}}};

// Common HTTP status messages
const std::map<int, std::string> kHttpStatusMessages = {{200, "OK"},
                                                        {201, "Created"},
                                                        {204, "No Content"},
                                                        {400, "Bad Request"},
                                                        {401, "Unauthorized"},
                                                        {403, "Forbidden"},
                                                        {404, "Not Found"},
                                                        {429, "Too Many Requests"},
                                                        {500, "Internal Server Error"},
                                                        {502, "Bad Gateway"},
                                                        {503, "Service Unavailable"},
                                                        {504, "Gateway Timeout"}};

}  // namespace

NetworkMock::NetworkMock(NetworkMockConfig config)
    : config_(std::move(config)),
      current_condition_(config_.default_condition),
      connection_counter_(0),
      remaining_connection_failures_(0),
      remaining_timeouts_(0),
      remaining_resets_(0),
      remaining_dns_failures_(0),
      remaining_server_unavailable_(0),
      remaining_rate_limits_(0),
      total_bytes_sent_(0),
      total_bytes_received_(0),
      total_operation_time_(0),
      gen_(rd_()),
      error_dist_(0.0, 1.0),
      latency_dist_(0, 100) {
  // Initialize default response
  default_response_.status_code = 200;
  default_response_.status_message = "OK";
  default_response_.body = "Mock response";
  default_response_.response_time = config_.default_response_delay;

  // Initialize error responses
  for (auto error_type : {MockNetworkError::ConnectionFailed, MockNetworkError::ConnectionTimeout,
                          MockNetworkError::ServerError, MockNetworkError::ServiceUnavailable}) {
    MockHttpResponse error_response;
    error_response.status_code = (error_type == MockNetworkError::ServerError) ? 500 : 503;
    error_response.status_message = kHttpStatusMessages.at(error_response.status_code);
    error_response.body = "Error: " + std::to_string(error_response.status_code);
    error_responses_[error_type] = error_response;
  }
}

NetworkMock::~NetworkMock() {
  // Remove global mock if this instance was installed
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  if (g_global_network_mock == this) {
    g_global_network_mock = nullptr;
  }
}

void NetworkMock::set_network_condition(const NetworkCondition& condition) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  current_condition_ = condition;
}

void NetworkMock::set_network_condition_for_url(const std::string& url_pattern,
                                                const NetworkCondition& condition) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  url_conditions_[url_pattern] = condition;
}

void NetworkMock::simulate_slow_network(size_t bandwidth_bps) {
  NetworkCondition condition = current_condition_;
  condition.bandwidth_bps = bandwidth_bps;
  condition.simulate_bandwidth_limit = true;
  set_network_condition(condition);
}

void NetworkMock::simulate_high_latency(std::chrono::milliseconds latency) {
  NetworkCondition condition = current_condition_;
  condition.base_latency = latency;
  condition.latency_variance = latency / 4;  // 25% variance
  condition.simulate_variable_latency = true;
  set_network_condition(condition);
}

void NetworkMock::simulate_unstable_network(double packet_loss_rate) {
  NetworkCondition condition = current_condition_;
  condition.packet_loss_rate = packet_loss_rate;
  condition.simulate_packet_loss = true;
  condition.connection_drop_rate = packet_loss_rate / 2;  // Half the packet loss rate
  set_network_condition(condition);
}

void NetworkMock::simulate_network_congestion(double congestion_factor) {
  NetworkCondition condition = current_condition_;
  condition.simulate_congestion = true;
  condition.congestion_factor = congestion_factor;
  condition.bandwidth_bps = static_cast<size_t>(condition.bandwidth_bps / congestion_factor);
  condition.base_latency = std::chrono::milliseconds(
      static_cast<int64_t>(condition.base_latency.count() * congestion_factor));
  set_network_condition(condition);
}

void NetworkMock::simulate_mobile_network() {
  auto it = kConnectionTypeDefaults.find("mobile");
  if (it != kConnectionTypeDefaults.end()) {
    set_network_condition(it->second);
  }
}

void NetworkMock::simulate_satellite_network() {
  auto it = kConnectionTypeDefaults.find("satellite");
  if (it != kConnectionTypeDefaults.end()) {
    set_network_condition(it->second);
  }
}

void NetworkMock::reset_to_normal_conditions() {
  auto it = kConnectionTypeDefaults.find("broadband");
  if (it != kConnectionTypeDefaults.end()) {
    set_network_condition(it->second);
  }
}

void NetworkMock::simulate_connection_failure(size_t request_count) {
  remaining_connection_failures_.store(request_count);
}

void NetworkMock::simulate_connection_timeout(size_t request_count) {
  remaining_timeouts_.store(request_count);
}

void NetworkMock::simulate_connection_reset(size_t request_count) {
  remaining_resets_.store(request_count);
}

void NetworkMock::simulate_dns_failure(size_t request_count) {
  remaining_dns_failures_.store(request_count);
}

void NetworkMock::simulate_server_unavailable(size_t request_count) {
  remaining_server_unavailable_.store(request_count);
}

void NetworkMock::simulate_rate_limiting(size_t request_count,
                                         std::chrono::milliseconds retry_after) {
  remaining_rate_limits_.store(request_count);

  // Set up rate limiting response
  MockHttpResponse rate_limit_response;
  rate_limit_response.status_code = 429;
  rate_limit_response.status_message = "Too Many Requests";
  rate_limit_response.headers["Retry-After"] = std::to_string(retry_after.count() / 1000);
  rate_limit_response.body = "Rate limit exceeded";
  error_responses_[MockNetworkError::TooManyRequests] = rate_limit_response;
}

void NetworkMock::set_response_for_url(const std::string& url, const MockHttpResponse& response) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  url_responses_[url] = response;
}

void NetworkMock::set_response_for_pattern(const std::string& url_pattern,
                                           const MockHttpResponse& response) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  pattern_responses_[url_pattern] = response;
}

void NetworkMock::set_default_response(const MockHttpResponse& response) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  default_response_ = response;
}

bool NetworkMock::load_mock_responses_from_directory(const std::string& directory_path) {
  // Check if directory exists
  if (!std::filesystem::exists(directory_path)) {
    return false;
  }

  if (!std::filesystem::is_directory(directory_path)) {
    return false;
  }

  std::lock_guard<std::mutex> lock(network_mutex_);
  size_t loaded_count = 0;

  try {
    // Iterate through all files in the directory
    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
      if (!entry.is_regular_file()) {
        continue;
      }

      const auto& file_path = entry.path();
      std::string filename = file_path.filename().string();

      // Skip non-response files
      if (filename.find("response") == std::string::npos &&
          filename.find(".txt") == std::string::npos &&
          filename.find(".json") == std::string::npos) {
        continue;
      }

      // Read file content
      std::ifstream file(file_path);
      if (!file.is_open()) {
        continue;
      }

      std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
      file.close();

      // Extract URL pattern from filename
      // Expected format: response_<url_pattern>.txt or <url_pattern>_response.json
      std::string url_pattern;
      if (filename.find("response_") == 0) {
        url_pattern = filename.substr(9);  // Remove "response_" prefix
        auto ext_pos = url_pattern.find_last_of('.');
        if (ext_pos != std::string::npos) {
          url_pattern = url_pattern.substr(0, ext_pos);
        }
      } else {
        auto response_pos = filename.find("_response");
        if (response_pos != std::string::npos) {
          url_pattern = filename.substr(0, response_pos);
        } else {
          // Use filename without extension as pattern
          auto ext_pos = filename.find_last_of('.');
          url_pattern = (ext_pos != std::string::npos) ? filename.substr(0, ext_pos) : filename;
        }
      }

      // Replace underscores with slashes for URL pattern
      std::replace(url_pattern.begin(), url_pattern.end(), '_', '/');

      // Create mock response
      MockHttpResponse response;
      response.status_code = 200;
      response.body = content;
      response.headers["Content-Type"] = filename.ends_with(".json") ? "application/json" : "text/plain";
      response.headers["Content-Length"] = std::to_string(content.length());

      // Store response for URL pattern
      url_responses_[url_pattern] = response;
      loaded_count++;
    }

    return loaded_count > 0;

  } catch (const std::exception&) {
    return false;
  }
}

void NetworkMock::set_error_response(MockNetworkError error_type,
                                     const MockHttpResponse& response) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  error_responses_[error_type] = response;
}

MockHttpResponse NetworkMock::mock_http_get(const std::string& url,
                                            const std::map<std::string, std::string>& headers) {
  return mock_http_request("GET", url, "", headers);
}

MockHttpResponse NetworkMock::mock_http_post(const std::string& url, const std::string& body,
                                             const std::map<std::string, std::string>& headers) {
  return mock_http_request("POST", url, body, headers);
}

MockHttpResponse NetworkMock::mock_http_put(const std::string& url, const std::string& body,
                                            const std::map<std::string, std::string>& headers) {
  return mock_http_request("PUT", url, body, headers);
}

MockHttpResponse NetworkMock::mock_http_delete(const std::string& url,
                                               const std::map<std::string, std::string>& headers) {
  return mock_http_request("DELETE", url, "", headers);
}

MockHttpResponse NetworkMock::mock_http_request(const std::string& method, const std::string& url,
                                                const std::string& body,
                                                const std::map<std::string, std::string>& headers) {
  auto start_time = std::chrono::steady_clock::now();

  // Determine network condition for this URL
  auto condition = get_condition_for_url(url);

  // Check for simulated errors
  auto error = check_for_simulated_errors();
  if (error != MockNetworkError::Success) {
    auto error_response = error_responses_.find(error);
    if (error_response != error_responses_.end()) {
      auto duration = std::chrono::steady_clock::now() - start_time;
      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

      record_operation(MockNetworkOperationType::HttpGet, url, method, headers, body,
                       error_response->second, error, duration_ms);

      return error_response->second;
    }
  }

  // Simulate network latency
  simulate_network_latency(condition);

  // Simulate packet loss
  if (simulate_packet_loss(condition)) {
    MockHttpResponse timeout_response;
    timeout_response.status_code = 0;  // Connection failed
    timeout_response.status_message = "Connection timeout";

    auto duration = std::chrono::steady_clock::now() - start_time;
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    record_operation(MockNetworkOperationType::HttpGet, url, method, headers, body,
                     timeout_response, MockNetworkError::ConnectionTimeout, duration_ms);

    return timeout_response;
  }

  // Find appropriate response
  auto response_opt = find_response_for_url(url);
  MockHttpResponse response = response_opt.value_or(default_response_);

  // Simulate bandwidth-limited transfer
  size_t total_bytes = body.size() + response.body.size();
  if (condition.simulate_bandwidth_limit) {
    simulate_bandwidth_limited_transfer(total_bytes);
  }

  // Update statistics
  total_bytes_sent_.fetch_add(body.size());
  total_bytes_received_.fetch_add(response.body.size());

  auto duration = std::chrono::steady_clock::now() - start_time;
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
  total_operation_time_.fetch_add(duration_ms.count());

  response.response_time = duration_ms;
  response.bytes_transferred = total_bytes;

  // Convert method to operation type
  MockNetworkOperationType op_type = MockNetworkOperationType::HttpGet;
  if (method == "POST")
    op_type = MockNetworkOperationType::HttpPost;
  else if (method == "PUT")
    op_type = MockNetworkOperationType::HttpPut;
  else if (method == "DELETE")
    op_type = MockNetworkOperationType::HttpDelete;

  record_operation(op_type, url, method, headers, body, response, MockNetworkError::Success,
                   duration_ms);

  return response;
}

bool NetworkMock::mock_connect(const std::string& host, int port) {
  auto connection_key = make_connection_key(host, port);

  // Check for connection failures
  auto error = check_for_simulated_errors();
  if (error == MockNetworkError::ConnectionFailed || error == MockNetworkError::ConnectionTimeout) {
    record_operation(MockNetworkOperationType::Connect, host + ":" + std::to_string(port),
                     "CONNECT", {}, "", {}, error, std::chrono::milliseconds(0));
    return false;
  }

  // Simulate connection establishment delay
  auto condition = get_condition_for_url(host);
  simulate_network_latency(condition);

  // Add to active connections
  {
    std::lock_guard<std::mutex> lock(network_mutex_);
    active_connections_[connection_key] = std::chrono::system_clock::now();
  }

  connection_counter_.fetch_add(1);

  record_operation(MockNetworkOperationType::Connect, host + ":" + std::to_string(port), "CONNECT",
                   {}, "", {}, MockNetworkError::Success, std::chrono::milliseconds(0));

  return true;
}

void NetworkMock::mock_disconnect(const std::string& host, int port) {
  auto connection_key = make_connection_key(host, port);

  {
    std::lock_guard<std::mutex> lock(network_mutex_);
    active_connections_.erase(connection_key);
  }

  record_operation(MockNetworkOperationType::Disconnect, host + ":" + std::to_string(port),
                   "DISCONNECT", {}, "", {}, MockNetworkError::Success,
                   std::chrono::milliseconds(0));
}

bool NetworkMock::is_connected(const std::string& host, int port) const {
  std::lock_guard<std::mutex> lock(network_mutex_);
  auto connection_key = make_connection_key(host, port);
  return active_connections_.find(connection_key) != active_connections_.end();
}

size_t NetworkMock::active_connection_count() const {
  std::lock_guard<std::mutex> lock(network_mutex_);
  return active_connections_.size();
}

size_t NetworkMock::mock_send_data(const std::string& data) {
  auto condition = current_condition_;

  if (condition.simulate_bandwidth_limit) {
    simulate_bandwidth_limited_transfer(data.size());
  }

  total_bytes_sent_.fetch_add(data.size());

  record_operation(MockNetworkOperationType::Send, "", "SEND", {}, data, {},
                   MockNetworkError::Success, std::chrono::milliseconds(0));

  return data.size();
}

std::string NetworkMock::mock_receive_data(size_t max_bytes) {
  auto condition = current_condition_;

  // Generate mock data
  std::string data(std::min(max_bytes, size_t(1024)), 'X');

  if (condition.simulate_bandwidth_limit) {
    simulate_bandwidth_limited_transfer(data.size());
  }

  total_bytes_received_.fetch_add(data.size());

  record_operation(MockNetworkOperationType::Receive, "", "RECEIVE", {}, "", {},
                   MockNetworkError::Success, std::chrono::milliseconds(0));

  return data;
}

std::chrono::milliseconds NetworkMock::calculate_transfer_time(size_t bytes) const {
  auto condition = current_condition_;

  if (!condition.simulate_bandwidth_limit || condition.bandwidth_bps == 0) {
    return std::chrono::milliseconds(0);
  }

  double seconds = static_cast<double>(bytes) / condition.bandwidth_bps;
  return std::chrono::milliseconds(static_cast<int64_t>(seconds * 1000));
}

void NetworkMock::simulate_bandwidth_limited_transfer(size_t bytes) const {
  if (!config_.simulate_real_delays) {
    return;
  }

  auto transfer_time = calculate_transfer_time(bytes);
  if (transfer_time > std::chrono::milliseconds::zero()) {
    std::this_thread::sleep_for(transfer_time);
  }
}

size_t NetworkMock::operation_count() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_history_.size();
}

size_t NetworkMock::operation_count(MockNetworkOperationType type) const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  auto it = operation_counts_.find(type);
  return (it != operation_counts_.end()) ? it->second : 0;
}

const std::vector<MockNetworkCallInfo>& NetworkMock::operation_history() const {
  std::lock_guard<std::mutex> lock(call_mutex_);
  return operation_history_;
}

std::vector<MockNetworkCallInfo> NetworkMock::operations_of_type(
    MockNetworkOperationType type) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  std::vector<MockNetworkCallInfo> result;
  for (const auto& call : operation_history_) {
    if (call.operation_type == type) {
      result.push_back(call);
    }
  }

  return result;
}

std::vector<MockNetworkCallInfo> NetworkMock::operations_for_url(const std::string& url) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  std::vector<MockNetworkCallInfo> result;
  for (const auto& call : operation_history_) {
    if (call.url == url) {
      result.push_back(call);
    }
  }

  return result;
}

bool NetworkMock::was_url_requested(const std::string& url) const {
  std::lock_guard<std::mutex> lock(call_mutex_);

  for (const auto& call : operation_history_) {
    if (call.url == url) {
      return true;
    }
  }

  return false;
}

size_t NetworkMock::total_bytes_transferred() const {
  return total_bytes_sent_.load() + total_bytes_received_.load();
}

std::chrono::milliseconds NetworkMock::total_operation_time() const {
  return std::chrono::milliseconds(total_operation_time_.load());
}

void NetworkMock::reset_call_history() {
  std::lock_guard<std::mutex> lock(call_mutex_);
  operation_history_.clear();
  operation_counts_.clear();
  total_bytes_sent_.store(0);
  total_bytes_received_.store(0);
  total_operation_time_.store(0);
}

void NetworkMock::update_config(const NetworkMockConfig& new_config) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  config_ = new_config;
}

const NetworkCondition& NetworkMock::current_condition() const {
  std::lock_guard<std::mutex> lock(network_mutex_);
  return current_condition_;
}

void NetworkMock::install_as_global_mock() {
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  g_global_network_mock = this;
}

void NetworkMock::remove_global_mock() {
  std::lock_guard<std::mutex> lock(g_global_mock_mutex);
  g_global_network_mock = nullptr;
}

void NetworkMock::create_test_scenario(const std::string& scenario_name,
                                       const NetworkCondition& condition,
                                       std::chrono::duration<double>) {
  set_network_condition(condition);

  record_operation(MockNetworkOperationType::Connect, scenario_name, "SCENARIO", {}, "", {},
                   MockNetworkError::Success, std::chrono::milliseconds(0),
                   "scenario_" + scenario_name);
}

// Private helper methods

void NetworkMock::record_operation(MockNetworkOperationType type, const std::string& url,
                                   const std::string& method,
                                   const std::map<std::string, std::string>& headers,
                                   const std::string& request_body,
                                   const MockHttpResponse& response, MockNetworkError error,
                                   std::chrono::milliseconds duration, const std::string& details) {
  if (!config_.enable_call_history) {
    return;
  }

  std::lock_guard<std::mutex> lock(call_mutex_);

  MockNetworkCallInfo call_info;
  call_info.timestamp = std::chrono::system_clock::now();
  call_info.operation_type = type;
  call_info.url = url;
  call_info.method = method;
  call_info.headers = headers;
  call_info.request_body = request_body;
  call_info.response = response;
  call_info.error = error;
  call_info.duration = duration;
  call_info.bytes_sent = request_body.size();
  call_info.bytes_received = response.body.size();
  call_info.retry_count = 0;
  call_info.operation_details = details;

  operation_history_.push_back(call_info);
  operation_counts_[type]++;

  // Limit history size
  if (operation_history_.size() > config_.max_history_size) {
    operation_history_.erase(operation_history_.begin());
  }
}

NetworkCondition NetworkMock::get_condition_for_url(const std::string& url) const {
  std::lock_guard<std::mutex> lock(network_mutex_);

  // Check for exact URL match
  auto it = url_conditions_.find(url);
  if (it != url_conditions_.end()) {
    return it->second;
  }

  // Check for pattern matches
  for (const auto& [pattern, condition] : url_conditions_) {
    if (matches_pattern(url, pattern)) {
      return condition;
    }
  }

  return current_condition_;
}

MockNetworkError NetworkMock::check_for_simulated_errors() const {
  // Check for specific error simulations
  if (remaining_connection_failures_.load() > 0) {
    remaining_connection_failures_.fetch_sub(1);
    return MockNetworkError::ConnectionFailed;
  }

  if (remaining_timeouts_.load() > 0) {
    remaining_timeouts_.fetch_sub(1);
    return MockNetworkError::ConnectionTimeout;
  }

  if (remaining_resets_.load() > 0) {
    remaining_resets_.fetch_sub(1);
    return MockNetworkError::ConnectionReset;
  }

  if (remaining_dns_failures_.load() > 0) {
    remaining_dns_failures_.fetch_sub(1);
    return MockNetworkError::HostNotFound;
  }

  if (remaining_server_unavailable_.load() > 0) {
    remaining_server_unavailable_.fetch_sub(1);
    return MockNetworkError::ServiceUnavailable;
  }

  if (remaining_rate_limits_.load() > 0) {
    remaining_rate_limits_.fetch_sub(1);
    return MockNetworkError::TooManyRequests;
  }

  // Check for random errors based on condition
  auto condition = current_condition_;

  if (condition.dns_failure_rate > 0.0 && error_dist_(gen_) < condition.dns_failure_rate) {
    return MockNetworkError::HostNotFound;
  }

  if (condition.server_error_rate > 0.0 && error_dist_(gen_) < condition.server_error_rate) {
    return MockNetworkError::ServerError;
  }

  if (condition.timeout_rate > 0.0 && error_dist_(gen_) < condition.timeout_rate) {
    return MockNetworkError::ConnectionTimeout;
  }

  return MockNetworkError::Success;
}

void NetworkMock::simulate_network_latency(const NetworkCondition& condition) const {
  if (!config_.simulate_real_delays) {
    return;
  }

  auto latency = condition.base_latency;

  if (condition.simulate_variable_latency) {
    auto variance = latency_dist_(gen_) % condition.latency_variance.count();
    latency += std::chrono::milliseconds(variance);
  }

  if (condition.simulate_congestion) {
    latency = std::chrono::milliseconds(
        static_cast<int64_t>(latency.count() * condition.congestion_factor));
  }

  if (latency > std::chrono::milliseconds::zero()) {
    std::this_thread::sleep_for(latency);
  }
}

bool NetworkMock::simulate_packet_loss(const NetworkCondition& condition) const {
  if (!condition.simulate_packet_loss || condition.packet_loss_rate <= 0.0) {
    return false;
  }

  return error_dist_(gen_) < condition.packet_loss_rate;
}

std::optional<MockHttpResponse> NetworkMock::find_response_for_url(const std::string& url) const {
  std::lock_guard<std::mutex> lock(network_mutex_);

  // Check for exact URL match
  auto it = url_responses_.find(url);
  if (it != url_responses_.end()) {
    return it->second;
  }

  // Check for pattern matches
  for (const auto& [pattern, response] : pattern_responses_) {
    if (matches_pattern(url, pattern)) {
      return response;
    }
  }

  return std::nullopt;
}

MockHttpResponse NetworkMock::generate_realistic_response(const std::string&,
                                                          const std::string&) const {
  MockHttpResponse response;
  response.status_code = 200;
  response.status_message = "OK";
  response.headers["Content-Type"] = "application/json";
  response.headers["Server"] = "MockServer/1.0";
  response.body = R"({"status": "success", "data": "mock data"})";
  response.response_time = config_.default_response_delay;
  response.bytes_transferred = response.body.size();

  return response;
}

std::tuple<std::string, std::string, int, std::string> NetworkMock::parse_url(
    const std::string& url) const {
  // Simplified URL parsing
  std::regex url_regex(R"(^(https?):\/\/([^:\/\s]+)(?::(\d+))?(\/.*)?$)");
  std::smatch matches;

  if (std::regex_match(url, matches, url_regex)) {
    std::string protocol = matches[1].str();
    std::string host = matches[2].str();
    int port = matches[3].matched ? std::stoi(matches[3].str()) : (protocol == "https" ? 443 : 80);
    std::string path = matches[4].matched ? matches[4].str() : "/";

    return std::make_tuple(protocol, host, port, path);
  }

  return std::make_tuple("http", "localhost", 80, "/");
}

bool NetworkMock::matches_pattern(const std::string& url, const std::string& pattern) const {
  // Simple pattern matching - in a real implementation, this could use regex
  return url.find(pattern) != std::string::npos;
}

void NetworkMock::update_bandwidth_tracking(size_t bytes) const {
  std::lock_guard<std::mutex> lock(bandwidth_mutex_);

  auto now = std::chrono::steady_clock::now();
  bandwidth_queue_.push({now, bytes});

  // Remove old entries (older than 1 second)
  auto cutoff = now - std::chrono::seconds(1);
  while (!bandwidth_queue_.empty() && bandwidth_queue_.front().first < cutoff) {
    bandwidth_queue_.pop();
  }
}

size_t NetworkMock::calculate_current_bandwidth_usage() const {
  std::lock_guard<std::mutex> lock(bandwidth_mutex_);

  size_t total_bytes = 0;
  auto queue_copy = bandwidth_queue_;

  while (!queue_copy.empty()) {
    total_bytes += queue_copy.front().second;
    queue_copy.pop();
  }

  return total_bytes;
}

std::pair<std::string, int> NetworkMock::make_connection_key(const std::string& host,
                                                             int port) const {
  return std::make_pair(host, port);
}

// Factory implementations

std::unique_ptr<NetworkMock> NetworkMockFactory::create_default() {
  return std::make_unique<NetworkMock>();
}

std::unique_ptr<NetworkMock> NetworkMockFactory::create(NetworkMockConfig config) {
  return std::make_unique<NetworkMock>(std::move(config));
}

std::unique_ptr<NetworkMock> NetworkMockFactory::create_for_connection_failure_testing() {
  NetworkMockConfig config;
  config.default_condition.connection_drop_rate = 0.5;
  config.default_condition.dns_failure_rate = 0.2;
  config.default_condition.server_error_rate = 0.3;

  return std::make_unique<NetworkMock>(config);
}

std::unique_ptr<NetworkMock> NetworkMockFactory::create_for_slow_network_testing() {
  NetworkMockConfig config;
  config.default_condition = kConnectionTypeDefaults.at("dial-up");

  return std::make_unique<NetworkMock>(config);
}

std::unique_ptr<NetworkMock> NetworkMockFactory::create_for_timeout_testing() {
  NetworkMockConfig config;
  config.default_condition.timeout_rate = 0.8;
  config.default_condition.connection_timeout = std::chrono::milliseconds(100);

  return std::make_unique<NetworkMock>(config);
}

std::unique_ptr<NetworkMock> NetworkMockFactory::create_for_mobile_network() {
  NetworkMockConfig config;
  config.default_condition = kConnectionTypeDefaults.at("mobile");

  return std::make_unique<NetworkMock>(config);
}

std::unique_ptr<NetworkMock> NetworkMockFactory::create_for_satellite_network() {
  NetworkMockConfig config;
  config.default_condition = kConnectionTypeDefaults.at("satellite");

  return std::make_unique<NetworkMock>(config);
}

std::unique_ptr<NetworkMock> NetworkMockFactory::create_with_perfect_conditions() {
  NetworkMockConfig config;
  config.default_condition.bandwidth_bps = 1000000000;  // 1 Gbps
  config.default_condition.base_latency = std::chrono::milliseconds(1);
  config.default_condition.packet_loss_rate = 0.0;
  config.simulate_real_delays = false;

  return std::make_unique<NetworkMock>(config);
}

// Scoped network mock implementation

ScopedNetworkMock::ScopedNetworkMock(std::unique_ptr<NetworkMock> mock) : mock_(std::move(mock)) {
  mock_->install_as_global_mock();
}

ScopedNetworkMock::~ScopedNetworkMock() { NetworkMock::remove_global_mock(); }

// Network test utilities implementation

bool NetworkTestUtils::test_network_resilience(std::function<bool()> network_operation,
                                               const std::vector<NetworkCondition>& conditions,
                                               size_t max_retries) {
  for ([[maybe_unused]] const auto& condition : conditions) {
    bool success = false;
    size_t attempts = 0;

    while (!success && attempts < max_retries) {
      success = network_operation();
      attempts++;
    }

    if (!success) {
      return false;
    }
  }

  return true;
}

std::vector<NetworkCondition> NetworkTestUtils::create_network_condition_sequence(
    const NetworkCondition& start_condition, const NetworkCondition& end_condition, size_t steps) {
  std::vector<NetworkCondition> sequence;
  sequence.reserve(steps);

  for (size_t i = 0; i < steps; ++i) {
    double factor = static_cast<double>(i) / (steps - 1);

    NetworkCondition condition;
    condition.bandwidth_bps =
        static_cast<size_t>(start_condition.bandwidth_bps +
                            factor * (end_condition.bandwidth_bps - start_condition.bandwidth_bps));

    auto start_latency_ms = start_condition.base_latency.count();
    auto end_latency_ms = end_condition.base_latency.count();
    condition.base_latency = std::chrono::milliseconds(
        static_cast<int64_t>(start_latency_ms + factor * (end_latency_ms - start_latency_ms)));

    condition.packet_loss_rate =
        start_condition.packet_loss_rate +
        factor * (end_condition.packet_loss_rate - start_condition.packet_loss_rate);

    sequence.push_back(condition);
  }

  return sequence;
}

bool NetworkTestUtils::validate_network_error_handling(
    std::function<bool(MockNetworkError)> error_handler,
    const std::vector<MockNetworkError>& error_types) {
  for (auto error_type : error_types) {
    if (!error_handler(error_type)) {
      return false;
    }
  }

  return true;
}

}  // namespace SolarSystem::Testing::Mocks
