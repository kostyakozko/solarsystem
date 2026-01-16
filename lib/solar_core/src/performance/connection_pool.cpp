/**
 * @file connection_pool.cpp
 * @brief Implementation of HTTP connection pooling
 */

#include "solar_core/performance/connection_pool.hpp"

#include <algorithm>
#include <mutex>
#include <vector>

namespace SolarSystem::Performance {

struct HttpConnectionPool::Impl {
  ConnectionPoolConfig config;
  std::vector<HttpConnection> connections;
  std::vector<HttpConnection> idle_connections;
  ConnectionPoolStats stats;
  mutable std::mutex mutex;

  explicit Impl(ConnectionPoolConfig cfg) : config(std::move(cfg)) {}

  void cleanup_expired() {
    auto now = std::chrono::system_clock::now();

    idle_connections.erase(
        std::remove_if(idle_connections.begin(), idle_connections.end(),
                       [this, now](const HttpConnection& conn) {
                         auto idle_time =
                             std::chrono::duration_cast<std::chrono::seconds>(now - conn.last_used);
                         return idle_time >= config.idle_timeout;
                       }),
        idle_connections.end());
  }
};

HttpConnectionPool::HttpConnectionPool(ConnectionPoolConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

HttpConnectionPool::~HttpConnectionPool() = default;

HttpConnectionPool::HttpConnectionPool(HttpConnectionPool&&) noexcept = default;
HttpConnectionPool& HttpConnectionPool::operator=(HttpConnectionPool&&) noexcept = default;

std::optional<HttpConnection> HttpConnectionPool::acquire() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  // Try to reuse idle connection
  if (!impl_->idle_connections.empty()) {
    auto conn = impl_->idle_connections.back();
    impl_->idle_connections.pop_back();

    conn.state = ConnectionState::ACTIVE;
    conn.last_used = std::chrono::system_clock::now();

    impl_->connections.push_back(conn);
    impl_->stats.active_connections++;
    impl_->stats.reused_connections++;
    impl_->stats.reuse_rate = static_cast<double>(impl_->stats.reused_connections) /
                              static_cast<double>(impl_->stats.total_requests + 1);

    return conn;
  }

  // Create new connection if under limit
  if (impl_->connections.size() < impl_->config.max_connections) {
    HttpConnection conn;
    conn.socket_fd = static_cast<int>(impl_->connections.size());  // Simplified
    conn.state = ConnectionState::ACTIVE;
    conn.created_at = std::chrono::system_clock::now();
    conn.last_used = conn.created_at;
    conn.keep_alive = impl_->config.enable_keep_alive;

    impl_->connections.push_back(conn);
    impl_->stats.total_connections++;
    impl_->stats.active_connections++;

    return conn;
  }

  return std::nullopt;
}

void HttpConnectionPool::release(const HttpConnection& connection) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  // Remove from active connections
  impl_->connections.erase(std::remove_if(impl_->connections.begin(), impl_->connections.end(),
                                          [&connection](const HttpConnection& conn) {
                                            return conn.socket_fd == connection.socket_fd;
                                          }),
                           impl_->connections.end());

  impl_->stats.active_connections--;
  impl_->stats.total_requests++;

  // Check if connection can be reused
  if (connection.keep_alive &&
      connection.requests_handled < impl_->config.max_requests_per_connection &&
      impl_->idle_connections.size() < impl_->config.max_idle_connections) {
    HttpConnection idle_conn = connection;
    idle_conn.state = ConnectionState::IDLE;
    idle_conn.last_used = std::chrono::system_clock::now();

    impl_->idle_connections.push_back(idle_conn);
    impl_->stats.idle_connections = impl_->idle_connections.size();
  }
}

void HttpConnectionPool::close(const HttpConnection& connection) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  impl_->connections.erase(std::remove_if(impl_->connections.begin(), impl_->connections.end(),
                                          [&connection](const HttpConnection& conn) {
                                            return conn.socket_fd == connection.socket_fd;
                                          }),
                           impl_->connections.end());

  impl_->idle_connections.erase(
      std::remove_if(impl_->idle_connections.begin(), impl_->idle_connections.end(),
                     [&connection](const HttpConnection& conn) {
                       return conn.socket_fd == connection.socket_fd;
                     }),
      impl_->idle_connections.end());

  impl_->stats.active_connections = impl_->connections.size();
  impl_->stats.idle_connections = impl_->idle_connections.size();
}

void HttpConnectionPool::cleanup_idle() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->cleanup_expired();
  impl_->stats.idle_connections = impl_->idle_connections.size();
}

ConnectionPoolStats HttpConnectionPool::get_statistics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->stats;
}

size_t HttpConnectionPool::get_active_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->connections.size();
}

size_t HttpConnectionPool::get_idle_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->idle_connections.size();
}

}  // namespace SolarSystem::Performance
