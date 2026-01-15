/**
 * @file session_manager.hpp
 * @brief Secure session management
 *
 * Provides secure session handling with:
 * - Session creation and validation
 * - Automatic timeout and cleanup
 * - Session data storage
 * - Concurrent access protection
 */

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/export.hpp"

namespace SolarSystem::Security {

/**
 * @brief Session data
 */
struct Session {
  std::string session_id;
  std::string user_id;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_accessed;
  std::chrono::system_clock::time_point expires_at;
  std::map<std::string, std::string> data;
  bool is_valid = true;
};

/**
 * @brief Session configuration
 */
struct SessionConfig {
  std::chrono::seconds timeout{1800};  // 30 minutes
  std::chrono::seconds max_lifetime{86400};  // 24 hours
  bool sliding_expiration = true;
  size_t max_sessions_per_user = 5;
  bool enable_cleanup = true;
  std::chrono::minutes cleanup_interval{5};
};

/**
 * @brief Session manager
 */
class SOLAR_CORE_API SessionManager {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit SessionManager(SessionConfig config = {});

  /**
   * @brief Destructor
   */
  ~SessionManager();

  // Non-copyable, movable
  SessionManager(const SessionManager&) = delete;
  SessionManager& operator=(const SessionManager&) = delete;
  SessionManager(SessionManager&&) noexcept;
  SessionManager& operator=(SessionManager&&) noexcept;

  /**
   * @brief Create new session
   */
  [[nodiscard]] std::string create_session(const std::string& user_id);

  /**
   * @brief Get session
   */
  [[nodiscard]] std::optional<Session> get_session(const std::string& session_id);

  /**
   * @brief Validate session
   */
  [[nodiscard]] bool validate_session(const std::string& session_id);

  /**
   * @brief Update session access time
   */
  void touch_session(const std::string& session_id);

  /**
   * @brief Destroy session
   */
  void destroy_session(const std::string& session_id);

  /**
   * @brief Destroy all sessions for user
   */
  void destroy_user_sessions(const std::string& user_id);

  /**
   * @brief Set session data
   */
  void set_session_data(
      const std::string& session_id,
      const std::string& key,
      const std::string& value);

  /**
   * @brief Get session data
   */
  [[nodiscard]] std::optional<std::string> get_session_data(
      const std::string& session_id,
      const std::string& key);

  /**
   * @brief Get all sessions for user
   */
  [[nodiscard]] std::vector<Session> get_user_sessions(const std::string& user_id);

  /**
   * @brief Get active session count
   */
  [[nodiscard]] size_t get_active_session_count() const;

  /**
   * @brief Cleanup expired sessions
   */
  void cleanup_expired_sessions();

  /**
   * @brief Start automatic cleanup
   */
  void start_cleanup_thread();

  /**
   * @brief Stop automatic cleanup
   */
  void stop_cleanup_thread();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Security
