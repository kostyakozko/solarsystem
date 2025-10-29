/**
 * @file security_manager.cpp
 * @brief Implementation of security manager
 */

#include "solar_core/security/security_manager.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <unordered_map>
#include <cstring>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "solar_utils/logging.hpp"

namespace SolarSystem::Security {

using namespace SolarSystem::Utils;

/**
 * @brief Convert role to string
 */
std::string to_string(UserRole role) {
  switch (role) {
    case UserRole::GUEST: return "GUEST";
    case UserRole::USER: return "USER";
    case UserRole::ADMIN: return "ADMIN";
    case UserRole::SUPER_ADMIN: return "SUPER_ADMIN";
    default: return "UNKNOWN";
  }
}

/**
 * @brief Parse role from string
 */
std::optional<UserRole> parse_role(const std::string& str) {
  if (str == "GUEST") return UserRole::GUEST;
  if (str == "USER") return UserRole::USER;
  if (str == "ADMIN") return UserRole::ADMIN;
  if (str == "SUPER_ADMIN") return UserRole::SUPER_ADMIN;
  return std::nullopt;
}

/**
 * @brief Password hasher implementation using PBKDF2
 */
std::string PasswordHasher::hash(const std::string& password) {
  // Use PBKDF2 with SHA-256 for secure password hashing
  auto salt = generate_salt();

  // PBKDF2 parameters
  const int iterations = 100000;  // OWASP recommended minimum
  const int key_length = 32;      // 256 bits

  unsigned char derived_key[32];

  // Derive key using PBKDF2
  if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                        reinterpret_cast<const unsigned char*>(salt.c_str()),
                        static_cast<int>(salt.size()),
                        iterations, EVP_sha256(),
                        key_length, derived_key) != 1) {
    // Fallback to SHA-256 if PBKDF2 fails
    unsigned char hash_bytes[SHA256_DIGEST_LENGTH];
    std::string salted = password + salt;
    SHA256(reinterpret_cast<const unsigned char*>(salted.c_str()),
           salted.size(), hash_bytes);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
      oss << std::hex << std::setfill('0') << std::setw(2)
          << static_cast<int>(hash_bytes[i]);
    }
    return oss.str() + ":" + salt;
  }

  // Convert derived key to hex string
  std::ostringstream oss;
  for (int i = 0; i < key_length; ++i) {
    oss << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<int>(derived_key[i]);
  }

  return oss.str() + ":" + salt;
}

bool PasswordHasher::verify(const std::string& password, const std::string& hash) {
  // Extract salt from hash
  auto colon_pos = hash.find(':');
  if (colon_pos == std::string::npos) return false;

  auto stored_hash = hash.substr(0, colon_pos);
  auto salt = hash.substr(colon_pos + 1);

  // PBKDF2 parameters (must match hash() function)
  const int iterations = 100000;
  const int key_length = 32;

  unsigned char derived_key[32];

  // Derive key using PBKDF2
  if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                        reinterpret_cast<const unsigned char*>(salt.c_str()),
                        static_cast<int>(salt.size()),
                        iterations, EVP_sha256(),
                        key_length, derived_key) != 1) {
    return false;
  }

  // Convert derived key to hex string
  std::ostringstream oss;
  for (int i = 0; i < key_length; ++i) {
    oss << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<int>(derived_key[i]);
  }

  return oss.str() == stored_hash;
}

std::string PasswordHasher::generate_salt() {
  return TokenGenerator::generate(16);
}

/**
 * @brief Token generator implementation
 */
std::string TokenGenerator::generate(size_t length) {
  static const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

  std::string token;
  token.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    token += charset[dis(gen)];
  }

  return token;
}

std::string TokenGenerator::generate_uuid() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);
  std::uniform_int_distribution<> dis2(8, 11);

  std::ostringstream oss;
  oss << std::hex;

  for (int i = 0; i < 8; i++) oss << dis(gen);
  oss << "-";
  for (int i = 0; i < 4; i++) oss << dis(gen);
  oss << "-4";
  for (int i = 0; i < 3; i++) oss << dis(gen);
  oss << "-";
  oss << dis2(gen);
  for (int i = 0; i < 3; i++) oss << dis(gen);
  oss << "-";
  for (int i = 0; i < 12; i++) oss << dis(gen);

  return oss.str();
}

/**
 * @brief SecurityManager implementation
 */
struct SecurityManager::Impl {
  SecurityConfig config;
  std::unordered_map<std::string, User> users;
  std::unordered_map<std::string, std::string> passwords;  // user_id -> hashed_password
  std::unordered_map<std::string, AuthToken> tokens;  // token -> AuthToken
  std::unordered_map<std::string, std::vector<std::string>> user_tokens;  // user_id -> tokens
  std::vector<SecurityEvent> events;
  std::unordered_map<std::string, size_t> failed_attempts;  // user_id -> count
  std::unordered_map<std::string, std::chrono::system_clock::time_point> lockouts;  // user_id -> unlock_time

  mutable std::mutex mutex;
  SecurityEventCallback event_callback;

  explicit Impl(SecurityConfig cfg) : config(std::move(cfg)) {
    // Create default admin user
    User admin;
    admin.id = TokenGenerator::generate_uuid();
    admin.username = "admin";
    admin.email = "admin@solarsystem.local";
    admin.role = UserRole::SUPER_ADMIN;
    admin.permissions = {
        Permission::READ_DATA,
        Permission::WRITE_DATA,
        Permission::EXECUTE_SIMULATION,
        Permission::MANAGE_USERS,
        Permission::CONFIGURE_SYSTEM,
        Permission::VIEW_LOGS,
        Permission::MANAGE_SECURITY
    };
    admin.created_at = std::chrono::system_clock::now();
    admin.is_active = true;

    users[admin.id] = admin;
    passwords[admin.id] = PasswordHasher::hash("admin123");  // Default password

    LOG_INFO("SecurityManager", "Initialized with default admin user");
  }

  bool is_user_locked_out(const std::string& user_id) {
    auto it = lockouts.find(user_id);
    if (it == lockouts.end()) return false;

    auto now = std::chrono::system_clock::now();
    if (now >= it->second) {
      lockouts.erase(it);
      failed_attempts.erase(user_id);
      return false;
    }

    return true;
  }

  void record_failed_attempt(const std::string& user_id) {
    failed_attempts[user_id]++;

    if (failed_attempts[user_id] >= config.max_login_attempts) {
      auto unlock_time = std::chrono::system_clock::now() + config.lockout_duration;
      lockouts[user_id] = unlock_time;

      LOG_WARN("SecurityManager", "User " + user_id + " locked out due to failed attempts");
    }
  }

  void cleanup_expired_tokens() {
    auto now = std::chrono::system_clock::now();

    for (auto it = tokens.begin(); it != tokens.end();) {
      if (now >= it->second.expires_at) {
        // Remove from user_tokens
        auto& user_token_list = user_tokens[it->second.user_id];
        user_token_list.erase(
            std::remove(user_token_list.begin(), user_token_list.end(), it->first),
            user_token_list.end());

        it = tokens.erase(it);
      } else {
        ++it;
      }
    }
  }
};

SecurityManager::SecurityManager(SecurityConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

SecurityManager::~SecurityManager() = default;

SecurityManager::SecurityManager(SecurityManager&&) noexcept = default;
SecurityManager& SecurityManager::operator=(SecurityManager&&) noexcept = default;

std::optional<AuthToken> SecurityManager::authenticate(const Credentials& credentials) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  // Find user by username
  auto user_it = std::find_if(impl_->users.begin(), impl_->users.end(),
                               [&](const auto& pair) {
                                 return pair.second.username == credentials.username;
                               });

  if (user_it == impl_->users.end()) {
    LOG_WARN("SecurityManager", "Authentication failed: user not found");

    SecurityEvent event;
    event.type = SecurityEventType::LOGIN_FAILURE;
    event.details = "User not found: " + credentials.username;
    event.timestamp = std::chrono::system_clock::now();
    impl_->events.push_back(event);

    if (impl_->event_callback) {
      impl_->event_callback(event);
    }

    return std::nullopt;
  }

  const auto& user = user_it->second;

  // Check if user is locked out
  if (impl_->is_user_locked_out(user.id)) {
    LOG_WARN("SecurityManager", "Authentication failed: user locked out");

    SecurityEvent event;
    event.type = SecurityEventType::LOGIN_FAILURE;
    event.user_id = user.id;
    event.details = "User locked out";
    event.timestamp = std::chrono::system_clock::now();
    impl_->events.push_back(event);

    return std::nullopt;
  }

  // Verify password
  auto password_it = impl_->passwords.find(user.id);
  if (password_it == impl_->passwords.end() ||
      !PasswordHasher::verify(credentials.password, password_it->second)) {
    impl_->record_failed_attempt(user.id);

    LOG_WARN("SecurityManager", "Authentication failed: invalid password");

    SecurityEvent event;
    event.type = SecurityEventType::LOGIN_FAILURE;
    event.user_id = user.id;
    event.details = "Invalid password";
    event.timestamp = std::chrono::system_clock::now();
    impl_->events.push_back(event);

    if (impl_->event_callback) {
      impl_->event_callback(event);
    }

    return std::nullopt;
  }

  // Clear failed attempts
  impl_->failed_attempts.erase(user.id);

  // Generate token
  AuthToken token;
  token.token = TokenGenerator::generate(32);
  token.user_id = user.id;
  token.issued_at = std::chrono::system_clock::now();
  token.expires_at = token.issued_at + impl_->config.token_lifetime;
  token.is_valid = true;

  impl_->tokens[token.token] = token;
  impl_->user_tokens[user.id].push_back(token.token);

  LOG_INFO("SecurityManager", "User authenticated: " + user.username);

  SecurityEvent event;
  event.type = SecurityEventType::LOGIN_SUCCESS;
  event.user_id = user.id;
  event.details = "Successful login";
  event.timestamp = std::chrono::system_clock::now();
  impl_->events.push_back(event);

  if (impl_->event_callback) {
    impl_->event_callback(event);
  }

  return token;
}

bool SecurityManager::validate_token(const std::string& token) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->tokens.find(token);
  if (it == impl_->tokens.end()) {
    return false;
  }

  auto now = std::chrono::system_clock::now();
  if (now >= it->second.expires_at) {
    impl_->tokens.erase(it);
    return false;
  }

  return it->second.is_valid;
}

std::optional<User> SecurityManager::get_user_from_token(const std::string& token) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto token_it = impl_->tokens.find(token);
  if (token_it == impl_->tokens.end()) {
    return std::nullopt;
  }

  auto user_it = impl_->users.find(token_it->second.user_id);
  if (user_it == impl_->users.end()) {
    return std::nullopt;
  }

  return user_it->second;
}

bool SecurityManager::authorize(const std::string& token, Permission permission) {
  auto user = get_user_from_token(token);
  if (!user) {
    return false;
  }

  // Super admin has all permissions
  if (user->role == UserRole::SUPER_ADMIN) {
    return true;
  }

  // Check if user has specific permission
  return std::find(user->permissions.begin(), user->permissions.end(), permission) !=
         user->permissions.end();
}

void SecurityManager::revoke_token(const std::string& token) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->tokens.find(token);
  if (it != impl_->tokens.end()) {
    // Remove from user_tokens
    auto& user_token_list = impl_->user_tokens[it->second.user_id];
    user_token_list.erase(
        std::remove(user_token_list.begin(), user_token_list.end(), token),
        user_token_list.end());

    impl_->tokens.erase(it);

    LOG_INFO("SecurityManager", "Token revoked");

    SecurityEvent event;
    event.type = SecurityEventType::LOGOUT;
    event.details = "Token revoked";
    event.timestamp = std::chrono::system_clock::now();
    impl_->events.push_back(event);
  }
}

void SecurityManager::log_security_event(const SecurityEvent& event) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->events.push_back(event);

  if (impl_->event_callback) {
    impl_->event_callback(event);
  }
}

bool SecurityManager::check_rate_limit(const std::string& /* identifier */) {
  // Simple rate limiting - can be enhanced with RateLimiter class
  return true;
}

void SecurityManager::add_user(const User& user, const std::string& password) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->users[user.id] = user;
  impl_->passwords[user.id] = PasswordHasher::hash(password);

  LOG_INFO("SecurityManager", "User added: " + user.username);
}

void SecurityManager::remove_user(const std::string& user_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->users.erase(user_id);
  impl_->passwords.erase(user_id);

  // Revoke all user tokens
  if (impl_->user_tokens.count(user_id)) {
    for (const auto& token : impl_->user_tokens[user_id]) {
      impl_->tokens.erase(token);
    }
    impl_->user_tokens.erase(user_id);
  }

  LOG_INFO("SecurityManager", "User removed: " + user_id);
}

void SecurityManager::update_user(const User& user) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->users[user.id] = user;

  LOG_INFO("SecurityManager", "User updated: " + user.username);
}

std::optional<User> SecurityManager::get_user(const std::string& user_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto it = impl_->users.find(user_id);
  if (it != impl_->users.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<SecurityEvent> SecurityManager::get_security_events(
    std::chrono::system_clock::time_point since) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  if (since == std::chrono::system_clock::time_point{}) {
    return impl_->events;
  }

  std::vector<SecurityEvent> filtered;
  std::copy_if(impl_->events.begin(), impl_->events.end(),
               std::back_inserter(filtered),
               [since](const SecurityEvent& event) {
                 return event.timestamp >= since;
               });

  return filtered;
}

void SecurityManager::set_security_event_callback(SecurityEventCallback callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->event_callback = std::move(callback);
}

}  // namespace SolarSystem::Security
