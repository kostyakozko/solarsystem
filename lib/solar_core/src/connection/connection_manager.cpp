/**
 * @file connection_manager.cpp
 * @brief Implementation of robust connection management
 */

#include "solar_core/connection/connection_manager.hpp"

#include <algorithm>
#include <random>
#include <thread>
#include <unordered_map>

#include "solar_utils/logging.hpp"

namespace SolarSystem::Connection {

using namespace SolarSystem::Utils;

/**
 * @brief Convert connection state to string
 */
std::string to_string(ConnectionState state) {
  switch (state) {
    case ConnectionState::DISCONNECTED:
      return "DISCONNECTED";
    case ConnectionState::CONNECTING:
      return "CONNECTING";
    case ConnectionState::CONNECTED:
      return "CONNECTED";
    case ConnectionState::RECONNECTING:
      return "RECONNECTING";
    case ConnectionState::DEGRADED:
      return "DEGRADED";
    case ConnectionState::FAILED:
      return "FAILED";
    default:
      return "UNKNOWN";
  }
}

/**
 * @brief Managed connection wrapper
 */
struct ManagedConnection {
  std::shared_ptr<IConnection> connection;
  ConnectionState state = ConnectionState::DISCONNECTED;
  ExponentialBackoff backoff;
  std::chrono::system_clock::time_point last_health_check;
  std::chrono::system_clock::time_point next_reconnect_time;
  size_t reconnection_attempts = 0;
  bool enabled = true;

  explicit ManagedConnection(std::shared_ptr<IConnection> conn) : connection(std::move(conn)) {}
};

/**
 * @brief ConnectionManager implementation
 */
struct ConnectionManager::Impl {
  ConnectionConfig config;
  std::unordered_map<std::string, ManagedConnection> connections;
  mutable std::mutex connections_mutex;

  std::atomic<bool> running{false};
  std::thread health_monitor_thread;
  std::thread reconnection_thread;

  StateCallback state_callback;
  HealthAlertCallback health_alert_callback;
  mutable std::mutex callback_mutex;

  explicit Impl(ConnectionConfig cfg) : config(std::move(cfg)) {}

  ~Impl() {
    if (running.load()) {
      stop();
    }
  }

  void start() {
    if (running.exchange(true)) {
      return;  // Already running
    }

    LOG_INFO("ConnectionManager", "Starting connection management");

    // Start health monitoring thread
    if (config.enable_health_monitoring) {
      health_monitor_thread = std::thread([this]() { health_monitor_loop(); });
    }

    // Start reconnection thread
    if (config.enable_auto_reconnect) {
      reconnection_thread = std::thread([this]() { reconnection_loop(); });
    }
  }

  void stop() {
    if (!running.exchange(false)) {
      return;  // Already stopped
    }

    LOG_INFO("ConnectionManager", "Stopping connection management");

    // Wait for threads to finish
    if (health_monitor_thread.joinable()) {
      health_monitor_thread.join();
    }
    if (reconnection_thread.joinable()) {
      reconnection_thread.join();
    }

    // Disconnect all connections
    std::lock_guard<std::mutex> lock(connections_mutex);
    for (auto& [id, managed] : connections) {
      if (managed.connection->is_connected()) {
        managed.connection->disconnect();
      }
    }
  }

  void health_monitor_loop() {
    LOG_INFO("ConnectionManager", "Health monitor thread started");

    while (running.load()) {
      auto now = std::chrono::system_clock::now();

      std::lock_guard<std::mutex> lock(connections_mutex);
      for (auto& [id, managed] : connections) {
        if (!managed.enabled) continue;

        // Check if it's time for health check
        auto time_since_check = now - managed.last_health_check;
        if (time_since_check >= config.health_check_interval) {
          perform_health_check(id, managed);
          managed.last_health_check = now;
        }
      }

      // Sleep for a short interval
      std::this_thread::sleep_for(std::chrono::seconds{1});
    }

    LOG_INFO("ConnectionManager", "Health monitor thread stopped");
  }

  void reconnection_loop() {
    LOG_INFO("ConnectionManager", "Reconnection thread started");

    while (running.load()) {
      auto now = std::chrono::system_clock::now();

      std::lock_guard<std::mutex> lock(connections_mutex);
      for (auto& [id, managed] : connections) {
        if (!managed.enabled) continue;

        // Check if reconnection is needed
        if (managed.state == ConnectionState::DISCONNECTED ||
            managed.state == ConnectionState::FAILED) {
          if (now >= managed.next_reconnect_time) {
            attempt_reconnection(id, managed);
          }
        }
      }

      // Sleep for a short interval
      std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }

    LOG_INFO("ConnectionManager", "Reconnection thread stopped");
  }

  void perform_health_check(const std::string& id, ManagedConnection& managed) {
    if (!managed.connection->is_connected()) {
      return;  // Skip health check if not connected
    }

    bool healthy = managed.connection->health_check();
    auto health = managed.connection->get_health();

    if (!healthy || health.health_score < config.min_health_score) {
      LOG_WARN("ConnectionManager", "Connection " + id + " health check failed");

      // Transition to degraded state
      auto old_state = managed.state;
      managed.state = ConnectionState::DEGRADED;
      notify_state_change(id, old_state, managed.state);

      // Trigger health alert
      notify_health_alert(id, health);

      // Schedule reconnection
      managed.next_reconnect_time = std::chrono::system_clock::now() + managed.backoff.next_delay();
    } else if (managed.state == ConnectionState::DEGRADED) {
      // Recovered from degraded state
      auto old_state = managed.state;
      managed.state = ConnectionState::CONNECTED;
      managed.backoff.reset();
      notify_state_change(id, old_state, managed.state);
    }
  }

  void attempt_reconnection(const std::string& id, ManagedConnection& managed) {
    if (managed.reconnection_attempts >= config.max_reconnection_attempts) {
      LOG_ERROR("ConnectionManager", "Connection " + id + " exceeded max reconnection attempts");
      auto old_state = managed.state;
      managed.state = ConnectionState::FAILED;
      notify_state_change(id, old_state, managed.state);
      managed.enabled = false;
      return;
    }

    LOG_INFO("ConnectionManager", "Attempting to reconnect " + id + " (attempt " +
                                      std::to_string(managed.reconnection_attempts + 1) + ")");

    auto old_state = managed.state;
    managed.state = ConnectionState::RECONNECTING;
    notify_state_change(id, old_state, managed.state);

    // Disconnect first if needed
    if (managed.connection->is_connected()) {
      managed.connection->disconnect();
    }

    // Attempt connection
    bool success = managed.connection->connect();

    if (success) {
      LOG_INFO("ConnectionManager", "Successfully reconnected " + id);
      managed.state = ConnectionState::CONNECTED;
      managed.reconnection_attempts = 0;
      managed.backoff.reset();
      notify_state_change(id, ConnectionState::RECONNECTING, managed.state);
    } else {
      LOG_WARN("ConnectionManager", "Failed to reconnect " + id);
      managed.state = ConnectionState::DISCONNECTED;
      managed.reconnection_attempts++;

      // Schedule next reconnection with backoff
      auto delay = managed.backoff.next_delay();
      managed.next_reconnect_time = std::chrono::system_clock::now() + delay;

      LOG_INFO("ConnectionManager", "Next reconnection attempt for " + id + " in " +
                                        std::to_string(delay.count()) + "ms");

      notify_state_change(id, ConnectionState::RECONNECTING, managed.state);
    }
  }

  void notify_state_change(const std::string& id, ConnectionState old_state,
                           ConnectionState new_state) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    if (state_callback) {
      state_callback(id, old_state, new_state);
    }
  }

  void notify_health_alert(const std::string& id, const ConnectionHealth& health) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    if (health_alert_callback) {
      health_alert_callback(id, health);
    }
  }
};

/**
 * @brief ConnectionManager implementation
 */
ConnectionManager::ConnectionManager(ConnectionConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

ConnectionManager::~ConnectionManager() = default;

ConnectionManager::ConnectionManager(ConnectionManager&&) noexcept = default;
ConnectionManager& ConnectionManager::operator=(ConnectionManager&&) noexcept = default;

void ConnectionManager::add_connection(std::shared_ptr<IConnection> connection) {
  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  auto id = connection->get_id();
  impl_->connections.emplace(id, ManagedConnection(std::move(connection)));
  LOG_INFO("ConnectionManager", "Added connection: " + id);
}

void ConnectionManager::remove_connection(const std::string& connection_id) {
  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  auto it = impl_->connections.find(connection_id);
  if (it != impl_->connections.end()) {
    if (it->second.connection->is_connected()) {
      it->second.connection->disconnect();
    }
    impl_->connections.erase(it);
    LOG_INFO("ConnectionManager", "Removed connection: " + connection_id);
  }
}

bool ConnectionManager::start() {
  impl_->start();
  return true;
}

void ConnectionManager::stop() { impl_->stop(); }

bool ConnectionManager::is_running() const { return impl_->running.load(); }

ConnectionHealth ConnectionManager::get_overall_health() const {
  std::lock_guard<std::mutex> lock(impl_->connections_mutex);

  if (impl_->connections.empty()) {
    return ConnectionHealth{};
  }

  ConnectionHealth overall;
  overall.state = ConnectionState::CONNECTED;
  double total_health = 0.0;

  for (const auto& [id, managed] : impl_->connections) {
    auto health = managed.connection->get_health();

    // Overall state is worst of all connections
    if (static_cast<int>(managed.state) > static_cast<int>(overall.state)) {
      overall.state = managed.state;
    }

    overall.successful_operations += health.successful_operations;
    overall.failed_operations += health.failed_operations;
    overall.reconnection_attempts += health.reconnection_attempts;
    total_health += health.health_score;
  }

  overall.health_score = total_health / static_cast<double>(impl_->connections.size());

  return overall;
}

std::optional<ConnectionHealth> ConnectionManager::get_connection_health(
    const std::string& connection_id) const {
  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  auto it = impl_->connections.find(connection_id);
  if (it != impl_->connections.end()) {
    return it->second.connection->get_health();
  }
  return std::nullopt;
}

std::vector<ConnectionHealth> ConnectionManager::get_all_health() const {
  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  std::vector<ConnectionHealth> healths;
  healths.reserve(impl_->connections.size());

  for (const auto& [id, managed] : impl_->connections) {
    healths.push_back(managed.connection->get_health());
  }

  return healths;
}

void ConnectionManager::reconnect_all() {
  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  for (auto& [id, managed] : impl_->connections) {
    managed.next_reconnect_time = std::chrono::system_clock::now();
    managed.reconnection_attempts = 0;
    managed.backoff.reset();
  }
  LOG_INFO("ConnectionManager", "Scheduled reconnection for all connections");
}

void ConnectionManager::reconnect(const std::string& connection_id) {
  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  auto it = impl_->connections.find(connection_id);
  if (it != impl_->connections.end()) {
    it->second.next_reconnect_time = std::chrono::system_clock::now();
    it->second.reconnection_attempts = 0;
    it->second.backoff.reset();
    LOG_INFO("ConnectionManager", "Scheduled reconnection for: " + connection_id);
  }
}

void ConnectionManager::set_state_callback(StateCallback callback) {
  std::lock_guard<std::mutex> lock(impl_->callback_mutex);
  impl_->state_callback = std::move(callback);
}

void ConnectionManager::set_health_alert_callback(HealthAlertCallback callback) {
  std::lock_guard<std::mutex> lock(impl_->callback_mutex);
  impl_->health_alert_callback = std::move(callback);
}

/**
 * @brief ExponentialBackoff implementation
 */
ExponentialBackoff::ExponentialBackoff(std::chrono::milliseconds initial_delay,
                                       std::chrono::milliseconds max_delay, double multiplier)
    : initial_delay_(initial_delay),
      max_delay_(max_delay),
      current_delay_(initial_delay),
      multiplier_(multiplier) {}

std::chrono::milliseconds ExponentialBackoff::next_delay() {
  auto delay = current_delay_;
  attempt_count_++;

  // Calculate next delay with jitter
  auto next = std::chrono::milliseconds(
      static_cast<long long>(static_cast<double>(current_delay_.count()) * multiplier_));

  // Add random jitter (±10%)
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.9, 1.1);
  next = std::chrono::milliseconds(
      static_cast<long long>(static_cast<double>(next.count()) * dis(gen)));

  // Cap at max delay
  current_delay_ = std::min(next, max_delay_);

  return delay;
}

void ExponentialBackoff::reset() {
  current_delay_ = initial_delay_;
  attempt_count_ = 0;
}

size_t ExponentialBackoff::get_attempt_count() const { return attempt_count_; }

/**
 * @brief ConnectionPool implementation
 */
struct ConnectionPool::Impl {
  std::vector<std::shared_ptr<IConnection>> connections;
  size_t max_connections;
  size_t next_index = 0;
  mutable std::mutex mutex;

  explicit Impl(size_t max) : max_connections(max) {}
};

ConnectionPool::ConnectionPool(size_t max_connections)
    : impl_(std::make_unique<Impl>(max_connections)) {}

ConnectionPool::~ConnectionPool() = default;

ConnectionPool::ConnectionPool(ConnectionPool&&) noexcept = default;
ConnectionPool& ConnectionPool::operator=(ConnectionPool&&) noexcept = default;

void ConnectionPool::add_connection(std::shared_ptr<IConnection> connection) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (impl_->connections.size() < impl_->max_connections) {
    impl_->connections.push_back(std::move(connection));
  }
}

std::shared_ptr<IConnection> ConnectionPool::get_connection() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  if (impl_->connections.empty()) {
    return nullptr;
  }

  // Round-robin selection
  auto connection = impl_->connections[impl_->next_index];
  impl_->next_index = (impl_->next_index + 1) % impl_->connections.size();

  return connection;
}

std::shared_ptr<IConnection> ConnectionPool::get_connection(const std::string& id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = std::find_if(impl_->connections.begin(), impl_->connections.end(),
                         [&id](const auto& conn) { return conn->get_id() == id; });

  if (it != impl_->connections.end()) {
    return *it;
  }

  return nullptr;
}

std::vector<std::shared_ptr<IConnection>> ConnectionPool::get_all_connections() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->connections;
}

size_t ConnectionPool::get_healthy_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto count =
      std::count_if(impl_->connections.begin(), impl_->connections.end(), [](const auto& conn) {
        auto health = conn->get_health();
        return health.state == ConnectionState::CONNECTED && health.health_score > 0.7;
      });
  return static_cast<size_t>(count);
}

size_t ConnectionPool::get_total_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->connections.size();
}

}  // namespace SolarSystem::Connection
