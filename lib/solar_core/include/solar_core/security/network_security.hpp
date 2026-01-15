/**
 * @file network_security.hpp
 * @brief Network security and encryption
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <map>
#include <memory>

#include "solar_core/export.hpp"

namespace SolarSystem::Security {

/**
 * @brief Network protocol type
 */
enum class NetworkProtocol {
  HTTP,
  HTTPS,
  TCP,
  UDP
};

/**
 * @brief Network security level
 */
enum class SecurityLevel {
  NONE,
  BASIC,
  STANDARD,
  HIGH,
  MAXIMUM
};

/**
 * @brief Network connection info
 */
struct ConnectionInfo {
  std::string remote_address;
  int remote_port;
  NetworkProtocol protocol;
  std::chrono::system_clock::time_point timestamp;

  ConnectionInfo()
      : remote_port(0),
        protocol(NetworkProtocol::TCP),
        timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Network security result
 */
struct NetworkSecurityResult {
  bool allowed;
  std::string reason;
  SecurityLevel required_level;

  NetworkSecurityResult(bool allow = false, std::string msg = "",
                       SecurityLevel level = SecurityLevel::STANDARD)
      : allowed(allow), reason(std::move(msg)), required_level(level) {}
};

/**
 * @brief IP address filter
 */
class SOLAR_CORE_API IPFilter {
public:
  void add_whitelist(const std::string& ip_pattern);
  void add_blacklist(const std::string& ip_pattern);
  void remove_whitelist(const std::string& ip_pattern);
  void remove_blacklist(const std::string& ip_pattern);

  bool is_allowed(const std::string& ip_address) const;
  bool is_blocked(const std::string& ip_address) const;

private:
  std::vector<std::string> whitelist_;
  std::vector<std::string> blacklist_;

  bool matches_pattern(const std::string& ip, const std::string& pattern) const;
};

/**
 * @brief Network encryption manager
 */
class SOLAR_CORE_API EncryptionManager {
public:
  static EncryptionManager& instance();

  // Encryption/Decryption
  std::optional<std::string> encrypt(const std::string& data,
                                    const std::string& key) const;
  std::optional<std::string> decrypt(const std::string& encrypted_data,
                                    const std::string& key) const;

  // Key management
  std::string generate_key(size_t key_size = 32) const;
  bool validate_key(const std::string& key) const;

  // Hashing
  std::string hash(const std::string& data) const;
  bool verify_hash(const std::string& data, const std::string& hash) const;

private:
  EncryptionManager() = default;
};

/**
 * @brief Network security manager
 */
class SOLAR_CORE_API NetworkSecurity {
public:
  static NetworkSecurity& instance();

  // Connection validation
  NetworkSecurityResult validate_connection(
      const ConnectionInfo& conn_info) const;

  bool is_secure_protocol(NetworkProtocol protocol) const;
  bool requires_encryption(const ConnectionInfo& conn_info) const;

  // IP filtering
  void set_ip_filter(std::shared_ptr<IPFilter> filter);
  std::shared_ptr<IPFilter> get_ip_filter() const;

  bool is_ip_allowed(const std::string& ip_address) const;
  bool is_ip_blocked(const std::string& ip_address) const;

  // Security level management
  void set_minimum_security_level(SecurityLevel level);
  SecurityLevel get_minimum_security_level() const;

  // Connection tracking
  void track_connection(const ConnectionInfo& conn_info);
  std::vector<ConnectionInfo> get_active_connections() const;
  size_t get_connection_count() const;

  // Audit logging
  void log_connection(const ConnectionInfo& conn_info, bool allowed) const;
  std::vector<std::string> get_audit_log() const;

private:
  NetworkSecurity() = default;

  std::shared_ptr<IPFilter> ip_filter_;
  SecurityLevel min_security_level_{SecurityLevel::STANDARD};
  std::vector<ConnectionInfo> active_connections_;
  mutable std::vector<std::string> audit_log_;
};

/**
 * @brief Secure network operations wrapper
 */
class SOLAR_CORE_API SecureNetworkOperations {
public:
  static SOLAR_CORE_API std::optional<std::string> secure_request(
      const std::string& url,
      const std::string& method = "GET",
      const std::string& data = "");

  static SOLAR_CORE_API bool validate_url(const std::string& url);
  static SOLAR_CORE_API bool is_secure_url(const std::string& url);
};

}  // namespace SolarSystem::Security
