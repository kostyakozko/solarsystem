/**
 * @file network_resource_manager.cpp
 * @brief Implementation of network resource management system
 */

#include "solar_utils/network_resource_manager.hpp"

#include <arpa/inet.h>
#include <curl/curl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <random>
#include <sstream>

#include "solar_utils/file_resource_manager.hpp"

namespace {
// RAII wrapper for CURL global initialization
class CurlGlobalInit {
 public:
  CurlGlobalInit() { curl_global_init(CURL_GLOBAL_ALL); }
  ~CurlGlobalInit() { curl_global_cleanup(); }
};

// Ensure curl is initialized once
CurlGlobalInit& get_curl_init() {
  static CurlGlobalInit init;
  return init;
}

// Callback for libcurl to write response data
size_t solar_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t total_size = size * nmemb;
  std::string* response = static_cast<std::string*>(userp);
  response->append(static_cast<char*>(contents), total_size);
  return total_size;
}

}  // anonymous namespace

namespace SolarSystem::Utils {

// NetworkCircuitBreaker implementation
NetworkCircuitBreaker::NetworkCircuitBreaker(const CircuitBreakerConfig& config) : config_(config) {
  state_change_time_ = std::chrono::system_clock::now();
}

bool NetworkCircuitBreaker::can_execute() const {
  std::lock_guard<std::mutex> lock(mutex_);

  switch (state_) {
    case CircuitBreakerState::Closed:
      return true;

    case CircuitBreakerState::Open:
      return should_attempt_reset();

    case CircuitBreakerState::HalfOpen:
      return true;

    default:
      return false;
  }
}

void NetworkCircuitBreaker::record_success() {
  std::lock_guard<std::mutex> lock(mutex_);

  success_count_++;
  total_requests_++;

  if (state_ == CircuitBreakerState::HalfOpen) {
    transition_to_closed();
  }
}

void NetworkCircuitBreaker::record_failure() {
  std::lock_guard<std::mutex> lock(mutex_);

  failure_count_++;
  total_requests_++;
  last_failure_time_ = std::chrono::system_clock::now();

  if (state_ == CircuitBreakerState::Closed) {
    if (failure_count_ >= config_.failure_threshold ||
        (total_requests_ >= config_.minimum_requests &&
         get_failure_rate() >= config_.failure_rate_threshold)) {
      transition_to_open();
    }
  } else if (state_ == CircuitBreakerState::HalfOpen) {
    transition_to_open();
  }
}

void NetworkCircuitBreaker::reset() {
  std::lock_guard<std::mutex> lock(mutex_);

  failure_count_ = 0;
  success_count_ = 0;
  total_requests_ = 0;
  transition_to_closed();
}

double NetworkCircuitBreaker::get_failure_rate() const {
  if (total_requests_ == 0) {
    return 0.0;
  }
  return static_cast<double>(failure_count_) / static_cast<double>(total_requests_);
}

bool NetworkCircuitBreaker::should_attempt_reset() const {
  auto now = std::chrono::system_clock::now();
  auto time_since_open = std::chrono::duration_cast<std::chrono::seconds>(now - state_change_time_);
  return time_since_open >= config_.timeout;
}

void NetworkCircuitBreaker::transition_to_open() {
  state_ = CircuitBreakerState::Open;
  state_change_time_ = std::chrono::system_clock::now();
}

void NetworkCircuitBreaker::transition_to_half_open() {
  state_ = CircuitBreakerState::HalfOpen;
  state_change_time_ = std::chrono::system_clock::now();
}

void NetworkCircuitBreaker::transition_to_closed() {
  state_ = CircuitBreakerState::Closed;
  state_change_time_ = std::chrono::system_clock::now();
  failure_count_ = 0;  // Reset failure count when closing
}

// ManagedNetworkConnection implementation
ManagedNetworkConnection::ManagedNetworkConnection(const std::string& endpoint, ConnectionType type)
    : endpoint_(endpoint), type_(type) {
  info_.connection_id = generate_connection_id();
  info_.endpoint = endpoint;
  info_.type = type;
  info_.state = ConnectionState::Closed;
  info_.created_at = std::chrono::system_clock::now();
  info_.last_used = info_.created_at;
}

ManagedNetworkConnection::~ManagedNetworkConnection() { disconnect(); }

ManagedNetworkConnection::ManagedNetworkConnection(ManagedNetworkConnection&& other) noexcept
    : endpoint_(std::move(other.endpoint_)),
      type_(other.type_),
      state_(other.state_),
      info_(std::move(other.info_)),
      resource_id_(std::move(other.resource_id_)),
      connection_handle_(other.connection_handle_) {
  other.state_ = ConnectionState::Closed;
  other.connection_handle_ = nullptr;
}

ManagedNetworkConnection& ManagedNetworkConnection::operator=(
    ManagedNetworkConnection&& other) noexcept {
  if (this != &other) {
    disconnect();

    endpoint_ = std::move(other.endpoint_);
    type_ = other.type_;
    state_ = other.state_;
    info_ = std::move(other.info_);
    resource_id_ = std::move(other.resource_id_);
    connection_handle_ = other.connection_handle_;

    other.state_ = ConnectionState::Closed;
    other.connection_handle_ = nullptr;
  }
  return *this;
}

bool ManagedNetworkConnection::connect(std::chrono::seconds timeout) {
  if (state_ == ConnectionState::Active) {
    return true;
  }

  state_ = ConnectionState::Connecting;
  info_.state = ConnectionState::Connecting;

  try {
    // Ensure curl is initialized
    get_curl_init();

    // Implement actual connection logic based on connection type
    switch (type_) {
      case ConnectionType::HTTP:
      case ConnectionType::HTTPS: {
        // Create real libcurl handle
        CURL* curl = curl_easy_init();
        if (!curl) {
          state_ = ConnectionState::Failed;
          info_.state = ConnectionState::Failed;
          info_.error_count++;
          return false;
        }

        // Set basic options
        curl_easy_setopt(curl, CURLOPT_URL, endpoint_.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout.count());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout.count());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "SolarSystem-Suite/4.0.0");

        // SSL/TLS settings for HTTPS
        if (type_ == ConnectionType::HTTPS) {
          curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
          curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        }

        connection_handle_ = curl;
        break;
      }
      case ConnectionType::TCP: {
        // Create real TCP socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
          state_ = ConnectionState::Failed;
          info_.state = ConnectionState::Failed;
          info_.error_count++;
          return false;
        }

        // Set socket timeout
        struct timeval tv;
        tv.tv_sec = timeout.count();
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        connection_handle_ = reinterpret_cast<void*>(static_cast<intptr_t>(sockfd));
        break;
      }
      case ConnectionType::UDP: {
        // Create real UDP socket
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
          state_ = ConnectionState::Failed;
          info_.state = ConnectionState::Failed;
          info_.error_count++;
          return false;
        }

        // Set socket timeout
        struct timeval tv;
        tv.tv_sec = timeout.count();
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        connection_handle_ = reinterpret_cast<void*>(static_cast<intptr_t>(sockfd));
        break;
      }
      case ConnectionType::Custom: {
        // Custom connection type - delegate to user implementation
        connection_handle_ = reinterpret_cast<void*>(3);
        break;
      }
    }

    if (connection_handle_) {
      state_ = ConnectionState::Active;
      info_.state = ConnectionState::Active;
      info_.last_used = std::chrono::system_clock::now();

      register_with_resource_manager();
      return true;
    } else {
      state_ = ConnectionState::Failed;
      info_.state = ConnectionState::Failed;
      info_.error_count++;
      return false;
    }

  } catch (const std::exception&) {
    state_ = ConnectionState::Failed;
    info_.state = ConnectionState::Failed;
    info_.error_count++;
    return false;
  }
}

void ManagedNetworkConnection::disconnect() {
  if (state_ != ConnectionState::Closed) {
    unregister_from_resource_manager();

    state_ = ConnectionState::Disconnecting;
    info_.state = ConnectionState::Disconnecting;

    // Perform actual disconnection based on connection type
    if (connection_handle_) {
      switch (type_) {
        case ConnectionType::HTTP:
        case ConnectionType::HTTPS: {
          // Clean up libcurl handle
          curl_easy_cleanup(connection_handle_);
          break;
        }
        case ConnectionType::TCP:
        case ConnectionType::UDP: {
          // Close socket
          int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(connection_handle_));
          close(sockfd);
          break;
        }
        case ConnectionType::Custom: {
          // Custom disconnection logic
          break;
        }
      }
    }

    state_ = ConnectionState::Closed;
    info_.state = ConnectionState::Closed;
    connection_handle_ = nullptr;
  }
}

bool ManagedNetworkConnection::is_connected() const { return state_ == ConnectionState::Active; }

bool ManagedNetworkConnection::is_healthy() const {
  return is_connected() && info_.error_count < 5;  // Arbitrary threshold
}

std::string ManagedNetworkConnection::send_request(const std::string& request,
                                                   std::chrono::seconds timeout) {
  if (!is_connected()) {
    return "";
  }

  auto start_time = std::chrono::steady_clock::now();

  try {
    std::string response;

    switch (type_) {
      case ConnectionType::HTTP:
      case ConnectionType::HTTPS: {
        // Use real libcurl for HTTP/HTTPS requests
        if (!connection_handle_) {
          info_.error_count++;
          return "";
        }

        // Set timeout
        curl_easy_setopt(connection_handle_, CURLOPT_TIMEOUT, timeout.count());

        // Set write callback
        curl_easy_setopt(connection_handle_, CURLOPT_WRITEFUNCTION, solar_curl_write_callback);
        curl_easy_setopt(connection_handle_, CURLOPT_WRITEDATA, &response);

        // Parse request to determine HTTP method and data
        std::string method = "GET";
        std::string data;
        if (request.find("POST") == 0) {
          method = "POST";
          curl_easy_setopt(connection_handle_, CURLOPT_POST, 1L);
          // Extract data from request if present
          size_t body_start = request.find("\r\n\r\n");
          if (body_start != std::string::npos) {
            data = request.substr(body_start + 4);
            curl_easy_setopt(connection_handle_, CURLOPT_POSTFIELDS, data.c_str());
          }
        } else if (request.find("PUT") == 0) {
          method = "PUT";
          curl_easy_setopt(connection_handle_, CURLOPT_CUSTOMREQUEST, "PUT");
          size_t body_start = request.find("\r\n\r\n");
          if (body_start != std::string::npos) {
            data = request.substr(body_start + 4);
            curl_easy_setopt(connection_handle_, CURLOPT_POSTFIELDS, data.c_str());
          }
        } else if (request.find("DELETE") == 0) {
          method = "DELETE";
          curl_easy_setopt(connection_handle_, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        // Perform the request
        CURLcode res = curl_easy_perform(connection_handle_);

        if (res != CURLE_OK) {
          info_.error_count++;
          return "";
        }

        break;
      }
      case ConnectionType::TCP: {
        // For TCP, send data through socket
        int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(connection_handle_));
        ssize_t sent = send(sockfd, request.c_str(), request.size(), 0);
        if (sent < 0) {
          info_.error_count++;
          return "";
        }

        // Receive response
        char buffer[8192];
        ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
          buffer[received] = '\0';
          response = std::string(buffer, static_cast<size_t>(received));
        }
        break;
      }
      case ConnectionType::UDP: {
        // For UDP, send datagram
        int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(connection_handle_));
        ssize_t sent = send(sockfd, request.c_str(), request.size(), 0);
        if (sent < 0) {
          info_.error_count++;
          return "";
        }

        // Receive response
        char buffer[8192];
        ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
          buffer[received] = '\0';
          response = std::string(buffer, static_cast<size_t>(received));
        }
        break;
      }
      case ConnectionType::Custom: {
        // Custom protocol handling
        response = "CUSTOM_RESPONSE: " + request;
        break;
      }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto response_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    update_statistics(request.size(), response.size());
    info_.request_count++;
    info_.total_response_time += response_time;
    info_.last_used = std::chrono::system_clock::now();

    return response;

  } catch (const std::exception&) {
    info_.error_count++;
    return "";
  }
}

bool ManagedNetworkConnection::send_data(const std::vector<char>& data) {
  if (!is_connected()) {
    return false;
  }

  try {
    // Implement data sending based on connection type
    switch (type_) {
      case ConnectionType::HTTP:
      case ConnectionType::HTTPS: {
        // For HTTP, data would be sent as request body using libcurl
        if (!connection_handle_) {
          info_.error_count++;
          return false;
        }
        curl_easy_setopt(connection_handle_, CURLOPT_POSTFIELDS, data.data());
        curl_easy_setopt(connection_handle_, CURLOPT_POSTFIELDSIZE, data.size());
        break;
      }
      case ConnectionType::TCP: {
        // For TCP, send data through socket
        int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(connection_handle_));
        ssize_t sent = send(sockfd, data.data(), data.size(), 0);
        if (sent < 0) {
          info_.error_count++;
          return false;
        }
        break;
      }
      case ConnectionType::UDP: {
        // For UDP, send datagram
        int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(connection_handle_));
        ssize_t sent = send(sockfd, data.data(), data.size(), 0);
        if (sent < 0) {
          info_.error_count++;
          return false;
        }
        break;
      }
      case ConnectionType::Custom: {
        // Custom data sending logic
        break;
      }
    }

    update_statistics(data.size(), 0);
    info_.last_used = std::chrono::system_clock::now();
    return true;

  } catch (const std::exception&) {
    info_.error_count++;
    return false;
  }
}

std::vector<char> ManagedNetworkConnection::receive_data(size_t max_bytes) {
  if (!is_connected()) {
    return {};
  }

  try {
    std::vector<char> data;

    switch (type_) {
      case ConnectionType::HTTP:
      case ConnectionType::HTTPS: {
        // For HTTP, receive response data using libcurl
        if (!connection_handle_) {
          info_.error_count++;
          return {};
        }

        std::string response;
        curl_easy_setopt(connection_handle_, CURLOPT_WRITEFUNCTION, solar_curl_write_callback);
        curl_easy_setopt(connection_handle_, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(connection_handle_);
        if (res != CURLE_OK) {
          info_.error_count++;
          return {};
        }

        data.assign(response.begin(), response.end());
        break;
      }
      case ConnectionType::TCP: {
        // For TCP, receive data from socket
        int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(connection_handle_));
        data.resize(max_bytes);
        ssize_t received = recv(sockfd, data.data(), max_bytes, 0);
        if (received < 0) {
          info_.error_count++;
          return {};
        }
        data.resize(static_cast<size_t>(received));
        break;
      }
      case ConnectionType::UDP: {
        // For UDP, receive datagram
        int sockfd = static_cast<int>(reinterpret_cast<intptr_t>(connection_handle_));
        data.resize(max_bytes);
        ssize_t received = recv(sockfd, data.data(), max_bytes, 0);
        if (received < 0) {
          info_.error_count++;
          return {};
        }
        data.resize(static_cast<size_t>(received));
        break;
      }
      case ConnectionType::Custom: {
        // Custom data receiving logic
        data.resize(std::min(max_bytes, size_t(256)));
        std::fill(data.begin(), data.end(), 'C');
        break;
      }
    }

    update_statistics(0, data.size());
    info_.last_used = std::chrono::system_clock::now();
    return data;

  } catch (const std::exception&) {
    info_.error_count++;
    return {};
  }
}

void ManagedNetworkConnection::register_with_resource_manager() {
  ResourceInfo info("", ResourceType::NetworkConnection, 0, "", "Network: " + endpoint_);
  info.resource_ptr = this;
  info.cleanup_function = [this]() { disconnect(); };

  resource_id_ = ResourceManager::instance().register_resource(info);
}

void ManagedNetworkConnection::unregister_from_resource_manager() {
  if (!resource_id_.empty()) {
    ResourceManager::instance().unregister_resource(resource_id_);
    resource_id_.clear();
  }
}

void ManagedNetworkConnection::update_statistics(size_t bytes_sent, size_t bytes_received) {
  info_.bytes_sent += bytes_sent;
  info_.bytes_received += bytes_received;
}

std::string ManagedNetworkConnection::generate_connection_id() const {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream id;
  id << "conn_";
  for (int i = 0; i < 8; ++i) {
    id << std::hex << dis(gen);
  }

  return id.str();
}

// NetworkConnectionPool implementation
NetworkConnectionPool::NetworkConnectionPool(const ConnectionPoolConfig& config)
    : config_(config) {}

NetworkConnectionPool::~NetworkConnectionPool() { close_all_connections(); }

std::shared_ptr<ManagedNetworkConnection> NetworkConnectionPool::acquire_connection(
    const std::string& endpoint, ConnectionType type) {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  std::string pool_key = get_pool_key(endpoint, type);

  // Try to reuse an idle connection
  auto idle_it = idle_connections_.find(pool_key);
  if (idle_it != idle_connections_.end() && !idle_it->second.empty()) {
    auto connection = idle_it->second.back();
    idle_it->second.pop_back();

    if (connection->is_healthy()) {
      active_connections_[pool_key].push_back(connection);
      return connection;
    }
  }

  // Create new connection if allowed
  if (can_create_new_connection(endpoint)) {
    auto connection = std::make_shared<ManagedNetworkConnection>(endpoint, type);
    if (connection->connect()) {
      active_connections_[pool_key].push_back(connection);
      stats_.total_connections_created++;
      return connection;
    }
  }

  return nullptr;
}

void NetworkConnectionPool::release_connection(
    std::shared_ptr<ManagedNetworkConnection> connection) {
  if (!connection) {
    return;
  }

  std::lock_guard<std::mutex> lock(pool_mutex_);

  std::string pool_key = get_pool_key(connection->endpoint(), connection->type());

  // Remove from active connections
  auto active_it = active_connections_.find(pool_key);
  if (active_it != active_connections_.end()) {
    auto& active_list = active_it->second;
    active_list.erase(std::remove(active_list.begin(), active_list.end(), connection),
                      active_list.end());
  }

  // Add to idle connections if healthy and reuse is enabled
  if (config_.enable_connection_reuse && connection->is_healthy()) {
    idle_connections_[pool_key].push_back(connection);
  } else {
    connection->disconnect();
  }
}

void NetworkConnectionPool::close_all_connections() {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  // Close all active connections
  for (auto& [key, connections] : active_connections_) {
    for (auto& connection : connections) {
      connection->disconnect();
    }
  }
  active_connections_.clear();

  // Close all idle connections
  for (auto& [key, connections] : idle_connections_) {
    for (auto& connection : connections) {
      connection->disconnect();
    }
  }
  idle_connections_.clear();
}

void NetworkConnectionPool::cleanup_idle_connections() {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  auto now = std::chrono::system_clock::now();

  for (auto& [key, connections] : idle_connections_) {
    auto it = connections.begin();
    while (it != connections.end()) {
      auto connection = *it;
      auto idle_time =
          std::chrono::duration_cast<std::chrono::seconds>(now - connection->info().last_used);

      if (idle_time > config_.idle_timeout || !connection->is_healthy()) {
        connection->disconnect();
        it = connections.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void NetworkConnectionPool::cleanup_failed_connections() {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  // Clean up failed active connections
  for (auto& [key, connections] : active_connections_) {
    auto it = connections.begin();
    while (it != connections.end()) {
      if (!(*it)->is_healthy()) {
        (*it)->disconnect();
        it = connections.erase(it);
      } else {
        ++it;
      }
    }
  }

  // Clean up failed idle connections
  for (auto& [key, connections] : idle_connections_) {
    auto it = connections.begin();
    while (it != connections.end()) {
      if (!(*it)->is_healthy()) {
        (*it)->disconnect();
        it = connections.erase(it);
      } else {
        ++it;
      }
    }
  }
}

size_t NetworkConnectionPool::get_active_connection_count() const {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  size_t count = 0;
  for (const auto& [key, connections] : active_connections_) {
    count += connections.size();
  }
  return count;
}

size_t NetworkConnectionPool::get_idle_connection_count() const {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  size_t count = 0;
  for (const auto& [key, connections] : idle_connections_) {
    count += connections.size();
  }
  return count;
}

void NetworkConnectionPool::update_config(const ConnectionPoolConfig& config) {
  std::lock_guard<std::mutex> lock(pool_mutex_);
  config_ = config;
}

NetworkOperationStats NetworkConnectionPool::get_statistics() const {
  std::lock_guard<std::mutex> lock(pool_mutex_);

  NetworkOperationStats pool_stats = stats_;
  pool_stats.current_active_connections = get_active_connection_count();

  return pool_stats;
}

std::string NetworkConnectionPool::get_pool_key(const std::string& endpoint,
                                                ConnectionType type) const {
  return endpoint + ":" + std::to_string(static_cast<int>(type));
}

bool NetworkConnectionPool::can_create_new_connection(const std::string& endpoint) const {
  // Check global connection limit
  size_t total_connections = get_active_connection_count() + get_idle_connection_count();
  if (total_connections >= config_.max_total_connections) {
    return false;
  }

  // Check per-host connection limit
  size_t host_connections = 0;
  for (const auto& [key, connections] : active_connections_) {
    if (key.find(endpoint) == 0) {
      host_connections += connections.size();
    }
  }
  for (const auto& [key, connections] : idle_connections_) {
    if (key.find(endpoint) == 0) {
      host_connections += connections.size();
    }
  }

  return host_connections < config_.max_connections_per_host;
}

// NetworkResourceManager implementation
NetworkResourceManager& NetworkResourceManager::instance() {
  static NetworkResourceManager instance;
  return instance;
}

NetworkResourceManager::~NetworkResourceManager() {
  stop_health_monitoring();
  force_close_all_connections();
}

std::shared_ptr<ManagedNetworkConnection> NetworkResourceManager::create_connection(
    const std::string& endpoint, ConnectionType type) {
  std::lock_guard<std::mutex> lock(network_mutex_);

  if (!check_connection_limits()) {
    return nullptr;
  }

  auto connection = std::make_shared<ManagedNetworkConnection>(endpoint, type);
  if (connection->connect(global_timeout_)) {
    active_connections_[connection->info().connection_id] = connection->info();
    update_global_statistics();
    return connection;
  }

  return nullptr;
}

std::shared_ptr<ManagedNetworkConnection> NetworkResourceManager::get_pooled_connection(
    const std::string& endpoint, ConnectionType type) {
  if (!connection_pool_) {
    connection_pool_ = std::make_unique<NetworkConnectionPool>();
  }

  return connection_pool_->acquire_connection(endpoint, type);
}

void NetworkResourceManager::register_circuit_breaker(const std::string& endpoint,
                                                      const CircuitBreakerConfig& config) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  circuit_breakers_[endpoint] = std::make_unique<NetworkCircuitBreaker>(config);
}

NetworkCircuitBreaker* NetworkResourceManager::get_circuit_breaker(const std::string& endpoint) {
  std::lock_guard<std::mutex> lock(network_mutex_);

  auto it = circuit_breakers_.find(endpoint);
  if (it != circuit_breakers_.end()) {
    return it->second.get();
  }

  return nullptr;
}

void NetworkResourceManager::remove_circuit_breaker(const std::string& endpoint) {
  std::lock_guard<std::mutex> lock(network_mutex_);
  circuit_breakers_.erase(endpoint);
}

std::string NetworkResourceManager::make_resilient_request(const std::string& endpoint,
                                                           const std::string& request,
                                                           size_t max_retries) {
  auto circuit_breaker = get_circuit_breaker(endpoint);
  if (circuit_breaker && !circuit_breaker->can_execute()) {
    return "";  // Circuit breaker is open
  }

  for (size_t attempt = 0; attempt <= max_retries; ++attempt) {
    auto connection = get_pooled_connection(endpoint);
    if (!connection) {
      continue;
    }

    std::string response = connection->send_request(request, global_timeout_);
    if (!response.empty()) {
      if (circuit_breaker) {
        circuit_breaker->record_success();
      }
      return response;
    }

    if (circuit_breaker) {
      circuit_breaker->record_failure();
    }

    // Exponential backoff
    if (attempt < max_retries) {
      auto delay = std::chrono::milliseconds(100 * (1 << attempt));  // 100ms, 200ms, 400ms, etc.
      std::this_thread::sleep_for(delay);
    }
  }

  return "";  // All attempts failed
}

NetworkOperationStats NetworkResourceManager::get_statistics() const {
  std::lock_guard<std::mutex> lock(network_mutex_);

  NetworkOperationStats combined_stats = stats_;

  if (connection_pool_) {
    auto pool_stats = connection_pool_->get_statistics();
    combined_stats.total_connections_created += pool_stats.total_connections_created;
    combined_stats.current_active_connections += pool_stats.current_active_connections;
  }

  return combined_stats;
}

std::vector<NetworkConnectionInfo> NetworkResourceManager::get_active_connections() const {
  std::lock_guard<std::mutex> lock(network_mutex_);

  std::vector<NetworkConnectionInfo> connections;
  connections.reserve(active_connections_.size());

  for (const auto& [id, info] : active_connections_) {
    connections.push_back(info);
  }

  return connections;
}

void NetworkResourceManager::generate_network_usage_report(std::ostream& output) const {
  std::lock_guard<std::mutex> lock(network_mutex_);

  output << "=== Network Resource Usage Report ===\n";
  output << "Total connections created: " << stats_.total_connections_created << "\n";
  output << "Current active connections: " << stats_.current_active_connections << "\n";
  output << "Peak active connections: " << stats_.peak_active_connections << "\n";
  output << "Total requests sent: " << stats_.total_requests_sent << "\n";
  output << "Total responses received: " << stats_.total_responses_received << "\n";
  output << "Total bytes sent: " << stats_.total_bytes_sent << "\n";
  output << "Total bytes received: " << stats_.total_bytes_received << "\n";
  output << "Connection failures: " << stats_.connection_failures << "\n";
  output << "Timeout errors: " << stats_.timeout_errors << "\n";
  output << "Average response time: " << stats_.average_response_time.count() << "ms\n\n";

  output << "Active Connections:\n";
  for (const auto& [id, info] : active_connections_) {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() -
                                                                info.created_at);

    output << "  " << id << " (" << info.endpoint << "): " << "age " << age.count() << "s, "
           << "requests " << info.request_count << ", " << "errors " << info.error_count << "\n";
  }

  output << "\nCircuit Breakers: " << circuit_breakers_.size() << "\n";
  for (const auto& [endpoint, breaker] : circuit_breakers_) {
    output << "  " << endpoint << ": " << static_cast<int>(breaker->get_state())
           << " (failures: " << breaker->get_failure_count() << ")\n";
  }
}

void NetworkResourceManager::configure_connection_pool(const ConnectionPoolConfig& config) {
  if (!connection_pool_) {
    connection_pool_ = std::make_unique<NetworkConnectionPool>(config);
  } else {
    connection_pool_->update_config(config);
  }
}

void NetworkResourceManager::cleanup_expired_connections() {
  if (connection_pool_) {
    connection_pool_->cleanup_idle_connections();
    connection_pool_->cleanup_failed_connections();
  }

  std::lock_guard<std::mutex> lock(network_mutex_);

  // Clean up tracked connections
  auto now = std::chrono::system_clock::now();
  auto it = active_connections_.begin();

  while (it != active_connections_.end()) {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_used);
    if (age > std::chrono::minutes(10)) {  // 10 minute timeout
      it = active_connections_.erase(it);
    } else {
      ++it;
    }
  }
}

void NetworkResourceManager::force_close_all_connections() {
  if (connection_pool_) {
    connection_pool_->close_all_connections();
  }

  std::lock_guard<std::mutex> lock(network_mutex_);
  active_connections_.clear();
}

void NetworkResourceManager::reset_all_circuit_breakers() {
  std::lock_guard<std::mutex> lock(network_mutex_);

  for (auto& [endpoint, breaker] : circuit_breakers_) {
    breaker->reset();
  }
}

void NetworkResourceManager::start_health_monitoring() {
  if (health_monitoring_active_.load()) {
    return;
  }

  health_monitoring_active_.store(true);
  health_monitoring_thread_ =
      std::make_unique<std::thread>(&NetworkResourceManager::health_monitoring_loop, this);
}

void NetworkResourceManager::stop_health_monitoring() {
  if (!health_monitoring_active_.load()) {
    return;
  }

  health_monitoring_active_.store(false);
  if (health_monitoring_thread_ && health_monitoring_thread_->joinable()) {
    health_monitoring_thread_->join();
  }
  health_monitoring_thread_.reset();
}

bool NetworkResourceManager::is_endpoint_healthy(const std::string& endpoint) const {
  auto circuit_breaker = const_cast<NetworkResourceManager*>(this)->get_circuit_breaker(endpoint);
  if (circuit_breaker) {
    return circuit_breaker->get_state() == CircuitBreakerState::Closed;
  }

  return true;  // Assume healthy if no circuit breaker
}

void NetworkResourceManager::health_monitoring_loop() {
  while (health_monitoring_active_.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(30));

    if (!health_monitoring_active_.load()) {
      break;
    }

    cleanup_expired_connections();
    update_global_statistics();
  }
}

void NetworkResourceManager::update_global_statistics() {
  stats_.current_active_connections = active_connections_.size();
  if (stats_.current_active_connections > stats_.peak_active_connections) {
    stats_.peak_active_connections = stats_.current_active_connections;
  }
}

bool NetworkResourceManager::check_connection_limits() const {
  return stats_.current_active_connections < max_concurrent_connections_;
}

std::string NetworkResourceManager::generate_connection_id() const {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream id;
  id << "net_";
  for (int i = 0; i < 8; ++i) {
    id << std::hex << dis(gen);
  }

  return id.str();
}

// NetworkUtils implementation
namespace NetworkUtils {

NetworkResult<std::string> make_http_request(
    const std::string& url, const std::string& method, const std::string& data,
    const std::unordered_map<std::string, std::string>& headers) {
  try {
    // Use the managed network connection for HTTP requests
    auto& network_manager = NetworkResourceManager::instance();
    auto connection = network_manager.get_pooled_connection(url, ConnectionType::HTTP);

    if (!connection) {
      return NetworkResult<std::string>("Failed to establish connection to " + url, -1);
    }

    // Build HTTP request
    std::ostringstream request_stream;
    request_stream << method << " " << url << " HTTP/1.1\r\n";
    request_stream << "Host: " << url << "\r\n";

    // Add custom headers
    for (const auto& [key, value] : headers) {
      request_stream << key << ": " << value << "\r\n";
    }

    // Add content if present
    if (!data.empty()) {
      request_stream << "Content-Length: " << data.size() << "\r\n";
      request_stream << "Content-Type: application/json\r\n";
    }

    request_stream << "\r\n";
    if (!data.empty()) {
      request_stream << data;
    }

    std::string request = request_stream.str();
    std::string response = connection->send_request(request);

    if (response.empty()) {
      return NetworkResult<std::string>("Empty response from server", -2);
    }

    return NetworkResult<std::string>(std::move(response));

  } catch (const std::exception& e) {
    return NetworkResult<std::string>("HTTP request failed: " + std::string(e.what()), -1);
  }
}

NetworkResult<std::string> download_file(const std::string& url, const std::string& output_path) {
  try {
    // Make HTTP GET request to download file
    auto response_result = make_http_request(url, "GET");
    if (!response_result.is_success()) {
      return NetworkResult<std::string>(
          "Failed to download from " + url + ": " + response_result.error(), -1);
    }

    std::string response = response_result.value();

    // Extract body from HTTP response (simplified parsing)
    size_t body_start = response.find("\r\n\r\n");
    if (body_start == std::string::npos) {
      return NetworkResult<std::string>("Invalid HTTP response format", -2);
    }

    std::string body = response.substr(body_start + 4);

    // Write to file using file resource manager
    auto& file_manager = FileResourceManager::instance();
    bool write_success = file_manager.atomic_write(output_path, body);

    if (!write_success) {
      return NetworkResult<std::string>("Failed to write downloaded content to " + output_path, -3);
    }

    std::string success_msg =
        "Download completed successfully: " + std::to_string(body.size()) + " bytes";
    return NetworkResult<std::string>(std::move(success_msg));

  } catch (const std::exception& e) {
    return NetworkResult<std::string>("Download failed: " + std::string(e.what()), -1);
  }
}

bool is_endpoint_reachable(const std::string& endpoint, std::chrono::seconds timeout) {
  try {
    // Attempt to create a connection to test reachability
    auto& network_manager = NetworkResourceManager::instance();
    auto connection = network_manager.create_connection(endpoint, ConnectionType::HTTP);

    if (!connection) {
      return false;
    }

    // Try to connect with the specified timeout
    bool connected = connection->connect(timeout);

    // Clean up the test connection
    connection->disconnect();

    return connected;

  } catch (const std::exception&) {
    return false;
  }
}

std::string resolve_hostname(const std::string& hostname) {
  try {
    // Check if it's already an IP address
    if (hostname.find_first_not_of("0123456789.") == std::string::npos) {
      // Looks like an IPv4 address
      return hostname;
    }

    // Use getaddrinfo() for real DNS resolution
    struct addrinfo hints, *result = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0) {
      return "";
    }

    if (!result) {
      return "";
    }

    // Extract IP address from first result
    std::string ip_address;
    if (result->ai_family == AF_INET) {
      struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
      char ip_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
      ip_address = ip_str;
    }

    freeaddrinfo(result);
    return ip_address;

  } catch (const std::exception&) {
    return "";
  }
}

std::string encode_url(const std::string& url) {
  std::ostringstream encoded;

  for (char c : url) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded << c;
    } else {
      encoded << '%' << std::hex << std::uppercase << static_cast<unsigned char>(c);
    }
  }

  return encoded.str();
}

std::string decode_url(const std::string& encoded_url) {
  std::ostringstream decoded;

  for (size_t i = 0; i < encoded_url.length(); ++i) {
    if (encoded_url[i] == '%' && i + 2 < encoded_url.length()) {
      std::string hex = encoded_url.substr(i + 1, 2);
      char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
      decoded << decoded_char;
      i += 2;
    } else {
      decoded << encoded_url[i];
    }
  }

  return decoded.str();
}

std::unordered_map<std::string, std::string> parse_query_string(const std::string& query) {
  std::unordered_map<std::string, std::string> params;

  std::istringstream query_stream(query);
  std::string pair;

  while (std::getline(query_stream, pair, '&')) {
    size_t equals_pos = pair.find('=');
    if (equals_pos != std::string::npos) {
      std::string key = pair.substr(0, equals_pos);
      std::string value = pair.substr(equals_pos + 1);
      params[decode_url(key)] = decode_url(value);
    }
  }

  return params;
}

}  // namespace NetworkUtils

}  // namespace SolarSystem::Utils
