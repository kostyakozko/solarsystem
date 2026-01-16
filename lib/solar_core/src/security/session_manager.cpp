/**
 * @file session_manager.cpp
 * @brief Implementation of session management
 */

#include "solar_core/security/session_manager.hpp"

#include <algorithm>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "solar_core/security/security_manager.hpp"

namespace SolarSystem::Security {

struct SessionManager::Impl {
  SessionConfig config;
  std::unordered_map<std::string, Session> sessions;
  std::unordered_map<std::string, std::vector<std::string>> user_sessions;
  mutable std::mutex mutex;
  std::thread cleanup_thread;
  std::atomic<bool> running{false};

  explicit Impl(SessionConfig cfg) : config(std::move(cfg)) {}

  ~Impl() {
    if (running.load()) {
      running.store(false);
      if (cleanup_thread.joinable()) {
        cleanup_thread.join();
      }
    }
  }
};

SessionManager::SessionManager(SessionConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

SessionManager::~SessionManager() = default;

SessionManager::SessionManager(SessionManager&&) noexcept = default;
SessionManager& SessionManager::operator=(SessionManager&&) noexcept = default;

std::string SessionManager::create_session(const std::string& user_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  Session session;
  session.session_id = TokenGenerator::generate_uuid();
  session.user_id = user_id;
  session.created_at = std::chrono::system_clock::now();
  session.last_accessed = session.created_at;
  session.expires_at = session.created_at + impl_->config.timeout;
  session.is_valid = true;

  impl_->sessions[session.session_id] = session;
  impl_->user_sessions[user_id].push_back(session.session_id);

  return session.session_id;
}

std::optional<Session> SessionManager::get_session(const std::string& session_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->sessions.find(session_id);
  if (it != impl_->sessions.end()) {
    return it->second;
  }

  return std::nullopt;
}

bool SessionManager::validate_session(const std::string& session_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->sessions.find(session_id);
  if (it == impl_->sessions.end()) {
    return false;
  }

  auto now = std::chrono::system_clock::now();
  if (now >= it->second.expires_at) {
    impl_->sessions.erase(it);
    return false;
  }

  return it->second.is_valid;
}

void SessionManager::touch_session(const std::string& session_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->sessions.find(session_id);
  if (it != impl_->sessions.end()) {
    it->second.last_accessed = std::chrono::system_clock::now();

    if (impl_->config.sliding_expiration) {
      it->second.expires_at = it->second.last_accessed + impl_->config.timeout;
    }
  }
}

void SessionManager::destroy_session(const std::string& session_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->sessions.find(session_id);
  if (it != impl_->sessions.end()) {
    auto& user_session_list = impl_->user_sessions[it->second.user_id];
    user_session_list.erase(
        std::remove(user_session_list.begin(), user_session_list.end(), session_id),
        user_session_list.end());

    impl_->sessions.erase(it);
  }
}

void SessionManager::destroy_user_sessions(const std::string& user_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  if (impl_->user_sessions.count(user_id)) {
    for (const auto& session_id : impl_->user_sessions[user_id]) {
      impl_->sessions.erase(session_id);
    }
    impl_->user_sessions.erase(user_id);
  }
}

void SessionManager::set_session_data(const std::string& session_id, const std::string& key,
                                      const std::string& value) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->sessions.find(session_id);
  if (it != impl_->sessions.end()) {
    it->second.data[key] = value;
  }
}

std::optional<std::string> SessionManager::get_session_data(const std::string& session_id,
                                                            const std::string& key) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->sessions.find(session_id);
  if (it != impl_->sessions.end()) {
    auto data_it = it->second.data.find(key);
    if (data_it != it->second.data.end()) {
      return data_it->second;
    }
  }

  return std::nullopt;
}

std::vector<Session> SessionManager::get_user_sessions(const std::string& user_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  std::vector<Session> sessions;
  if (impl_->user_sessions.count(user_id)) {
    for (const auto& session_id : impl_->user_sessions[user_id]) {
      auto it = impl_->sessions.find(session_id);
      if (it != impl_->sessions.end()) {
        sessions.push_back(it->second);
      }
    }
  }

  return sessions;
}

size_t SessionManager::get_active_session_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->sessions.size();
}

void SessionManager::cleanup_expired_sessions() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto now = std::chrono::system_clock::now();

  for (auto it = impl_->sessions.begin(); it != impl_->sessions.end();) {
    if (now >= it->second.expires_at) {
      auto& user_session_list = impl_->user_sessions[it->second.user_id];
      user_session_list.erase(
          std::remove(user_session_list.begin(), user_session_list.end(), it->first),
          user_session_list.end());

      it = impl_->sessions.erase(it);
    } else {
      ++it;
    }
  }
}

void SessionManager::start_cleanup_thread() {
  if (impl_->running.exchange(true)) {
    return;  // Already running
  }

  impl_->cleanup_thread = std::thread([this]() {
    while (impl_->running.load()) {
      std::this_thread::sleep_for(impl_->config.cleanup_interval);
      cleanup_expired_sessions();
    }
  });
}

void SessionManager::stop_cleanup_thread() {
  impl_->running.store(false);
  if (impl_->cleanup_thread.joinable()) {
    impl_->cleanup_thread.join();
  }
}

}  // namespace SolarSystem::Security
