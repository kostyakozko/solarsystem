/**
 * @file security_manager.hpp
 * @brief Comprehensive security management system
 *
 * Provides centralized security management with:
 * - Authentication and authorization
 * - Session management
 * - Security monitoring and threat detection
 * - Audit logging
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Security {

/**
 * @brief User role enumeration
 */
enum class UserRole {
  GUEST,       ///< Read-only access
  USER,        ///< Standard user access
  ADMIN,       ///< Administrative access
  SUPER_ADMIN  ///< Full system access
};

/**
 * @brief Convert role to string
 */
[[nodiscard]] std::string to_string(UserRole role);

/**
 * @brief Parse role from string
 */
[[nodiscard]] std::optional<UserRole> parse_role(const std::string& str);

/**
 * @brief Permission enumeration
 */
enum class Permission {
  READ_DATA,
  WRITE_DATA,
  EXECUTE_SIMULATION,
  MANAGE_USERS,
  CONFIGURE_SYSTEM,
  VIEW_LOGS,
  MANAGE_SECURITY
};

/**
 * @brief Security event type
 */
enum class SecurityEventType {
  LOGIN_SUCCESS,
  LOGIN_FAILURE,
  LOGOUT,
  UNAUTHORIZED_ACCESS,
  SUSPICIOUS_ACTIVITY,
  RATE_LIMIT_EXCEEDED,
  SESSION_EXPIRED,
  PERMISSION_DENIED
};

/**
 * @brief User credentials
 */
struct Credentials {
  std::string username;
  std::string password;  // Should be hashed in production
};

/**
 * @brief User information
 */
struct User {
  std::string id;
  std::string username;
  std::string email;
  UserRole role;
  std::vector<Permission> permissions;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_login;
  bool is_active = true;
};

/**
 * @brief Authentication token
 */
struct AuthToken {
  std::string token;
  std::string user_id;
  std::chrono::system_clock::time_point issued_at;
  std::chrono::system_clock::time_point expires_at;
  bool is_valid = true;
};

/**
 * @brief Security event
 */
struct SecurityEvent {
  SecurityEventType type;
  std::string user_id;
  std::string ip_address;
  std::string details;
  std::chrono::system_clock::time_point timestamp;
  std::string resource;
};

/**
 * @brief Security configuration
 */
struct SecurityConfig {
  std::chrono::seconds token_lifetime{3600};   // 1 hour
  std::chrono::seconds session_timeout{1800};  // 30 minutes
  size_t max_login_attempts = 5;
  std::chrono::minutes lockout_duration{15};
  bool enable_rate_limiting = true;
  size_t max_requests_per_minute = 60;
  bool enable_audit_logging = true;
  bool require_strong_passwords = true;
  size_t min_password_length = 8;
};

/**
 * @brief Security manager interface
 */
class ISecurityManager {
 public:
  virtual ~ISecurityManager() = default;

  /**
   * @brief Authenticate user
   */
  [[nodiscard]] virtual std::optional<AuthToken> authenticate(const Credentials& credentials) = 0;

  /**
   * @brief Validate authentication token
   */
  [[nodiscard]] virtual bool validate_token(const std::string& token) = 0;

  /**
   * @brief Get user from token
   */
  [[nodiscard]] virtual std::optional<User> get_user_from_token(const std::string& token) = 0;

  /**
   * @brief Authorize action
   */
  [[nodiscard]] virtual bool authorize(const std::string& token, Permission permission) = 0;

  /**
   * @brief Revoke token (logout)
   */
  virtual void revoke_token(const std::string& token) = 0;

  /**
   * @brief Log security event
   */
  virtual void log_security_event(const SecurityEvent& event) = 0;

  /**
   * @brief Check rate limit
   */
  [[nodiscard]] virtual bool check_rate_limit(const std::string& identifier) = 0;
};

/**
 * @brief Default security manager implementation
 */
class SecurityManager : public ISecurityManager {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit SecurityManager(SecurityConfig config = {});

  /**
   * @brief Destructor
   */
  ~SecurityManager() override;

  // Non-copyable, movable
  SecurityManager(const SecurityManager&) = delete;
  SecurityManager& operator=(const SecurityManager&) = delete;
  SecurityManager(SecurityManager&&) noexcept;
  SecurityManager& operator=(SecurityManager&&) noexcept;

  // ISecurityManager interface
  [[nodiscard]] std::optional<AuthToken> authenticate(const Credentials& credentials) override;

  [[nodiscard]] bool validate_token(const std::string& token) override;

  [[nodiscard]] std::optional<User> get_user_from_token(const std::string& token) override;

  [[nodiscard]] bool authorize(const std::string& token, Permission permission) override;

  void revoke_token(const std::string& token) override;

  void log_security_event(const SecurityEvent& event) override;

  [[nodiscard]] bool check_rate_limit(const std::string& identifier) override;

  /**
   * @brief Add user
   */
  void add_user(const User& user, const std::string& password);

  /**
   * @brief Remove user
   */
  void remove_user(const std::string& user_id);

  /**
   * @brief Update user
   */
  void update_user(const User& user);

  /**
   * @brief Get user by ID
   */
  [[nodiscard]] std::optional<User> get_user(const std::string& user_id) const;

  /**
   * @brief Get all security events
   */
  [[nodiscard]] std::vector<SecurityEvent> get_security_events(
      std::chrono::system_clock::time_point since = {}) const;

  /**
   * @brief Set security event callback
   */
  using SecurityEventCallback = std::function<void(const SecurityEvent&)>;
  void set_security_event_callback(SecurityEventCallback callback);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Password hasher utility
 */
class PasswordHasher {
 public:
  /**
   * @brief Hash password
   */
  [[nodiscard]] static std::string hash(const std::string& password);

  /**
   * @brief Verify password against hash
   */
  [[nodiscard]] static bool verify(const std::string& password, const std::string& hash);

  /**
   * @brief Generate random salt
   */
  [[nodiscard]] static std::string generate_salt();
};

/**
 * @brief Token generator utility
 */
class TokenGenerator {
 public:
  /**
   * @brief Generate secure random token
   */
  [[nodiscard]] static std::string generate(size_t length = 32);

  /**
   * @brief Generate UUID
   */
  [[nodiscard]] static std::string generate_uuid();
};

}  // namespace SolarSystem::Security
