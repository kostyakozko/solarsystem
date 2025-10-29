/**
 * @file network_security.cpp
 * @brief Implementation of network security
 */

#include "solar_core/security/network_security.hpp"

#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace SolarSystem::Security {

// IPFilter implementation
void IPFilter::add_whitelist(const std::string& ip_pattern) {
  whitelist_.push_back(ip_pattern);
}

void IPFilter::add_blacklist(const std::string& ip_pattern) {
  blacklist_.push_back(ip_pattern);
}

void IPFilter::remove_whitelist(const std::string& ip_pattern) {
  whitelist_.erase(
      std::remove(whitelist_.begin(), whitelist_.end(), ip_pattern),
      whitelist_.end());
}

void IPFilter::remove_blacklist(const std::string& ip_pattern) {
  blacklist_.erase(
      std::remove(blacklist_.begin(), blacklist_.end(), ip_pattern),
      blacklist_.end());
}

bool IPFilter::is_allowed(const std::string& ip_address) const {
  // If blacklisted, deny
  for (const auto& pattern : blacklist_) {
    if (matches_pattern(ip_address, pattern)) {
      return false;
    }
  }

  // If whitelist is empty, allow all (except blacklisted)
  if (whitelist_.empty()) {
    return true;
  }

  // Check whitelist
  for (const auto& pattern : whitelist_) {
    if (matches_pattern(ip_address, pattern)) {
      return true;
    }
  }

  return false;
}

bool IPFilter::is_blocked(const std::string& ip_address) const {
  return !is_allowed(ip_address);
}

bool IPFilter::matches_pattern(const std::string& ip, const std::string& pattern) const {
  // Simple pattern matching (supports * wildcard)
  if (pattern == "*") return true;
  if (pattern == ip) return true;

  // Check for wildcard patterns like "192.168.*"
  size_t star_pos = pattern.find('*');
  if (star_pos != std::string::npos) {
    std::string prefix = pattern.substr(0, star_pos);
    return ip.find(prefix) == 0;
  }

  return false;
}

// EncryptionManager implementation
EncryptionManager& EncryptionManager::instance() {
  static EncryptionManager instance;
  return instance;
}

std::optional<std::string> EncryptionManager::encrypt(
    const std::string& data,
    const std::string& key) const {

  if (!validate_key(key)) {
    return std::nullopt;
  }

  // Simple XOR encryption for demonstration
  // In production, use proper encryption libraries
  std::string encrypted = data;
  for (size_t i = 0; i < encrypted.size(); ++i) {
    encrypted[i] ^= key[i % key.size()];
  }

  return encrypted;
}

std::optional<std::string> EncryptionManager::decrypt(
    const std::string& encrypted_data,
    const std::string& key) const {

  // XOR encryption is symmetric
  return encrypt(encrypted_data, key);
}

std::string EncryptionManager::generate_key(size_t key_size) const {
  static const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

  std::string key;
  key.reserve(key_size);
  for (size_t i = 0; i < key_size; ++i) {
    key += charset[dis(gen)];
  }

  return key;
}

bool EncryptionManager::validate_key(const std::string& key) const {
  return !key.empty() && key.size() >= 8;
}

std::string EncryptionManager::hash(const std::string& data) const {
  // Simple hash for demonstration
  // In production, use proper hashing libraries (SHA-256, etc.)
  std::hash<std::string> hasher;
  size_t hash_value = hasher(data);

  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash_value;
  return oss.str();
}

bool EncryptionManager::verify_hash(const std::string& data,
                                    const std::string& hash_value) const {
  return hash(data) == hash_value;
}

// NetworkSecurity implementation
NetworkSecurity& NetworkSecurity::instance() {
  static NetworkSecurity instance;
  return instance;
}

NetworkSecurityResult NetworkSecurity::validate_connection(
    const ConnectionInfo& conn_info) const {

  // Check IP filtering
  if (ip_filter_ && !ip_filter_->is_allowed(conn_info.remote_address)) {
    return NetworkSecurityResult(false, "IP address blocked", SecurityLevel::MAXIMUM);
  }

  // Check protocol security
  if (!is_secure_protocol(conn_info.protocol)) {
    if (min_security_level_ >= SecurityLevel::HIGH) {
      return NetworkSecurityResult(false, "Insecure protocol not allowed",
                                  min_security_level_);
    }
  }

  return NetworkSecurityResult(true, "Connection allowed", SecurityLevel::STANDARD);
}

bool NetworkSecurity::is_secure_protocol(NetworkProtocol protocol) const {
  return protocol == NetworkProtocol::HTTPS;
}

bool NetworkSecurity::requires_encryption(const ConnectionInfo& conn_info) const {
  return min_security_level_ >= SecurityLevel::STANDARD &&
         !is_secure_protocol(conn_info.protocol);
}

void NetworkSecurity::set_ip_filter(std::shared_ptr<IPFilter> filter) {
  ip_filter_ = std::move(filter);
}

std::shared_ptr<IPFilter> NetworkSecurity::get_ip_filter() const {
  return ip_filter_;
}

bool NetworkSecurity::is_ip_allowed(const std::string& ip_address) const {
  if (!ip_filter_) {
    return true;
  }
  return ip_filter_->is_allowed(ip_address);
}

bool NetworkSecurity::is_ip_blocked(const std::string& ip_address) const {
  return !is_ip_allowed(ip_address);
}

void NetworkSecurity::set_minimum_security_level(SecurityLevel level) {
  min_security_level_ = level;
}

SecurityLevel NetworkSecurity::get_minimum_security_level() const {
  return min_security_level_;
}

void NetworkSecurity::track_connection(const ConnectionInfo& conn_info) {
  active_connections_.push_back(conn_info);
}

std::vector<ConnectionInfo> NetworkSecurity::get_active_connections() const {
  return active_connections_;
}

size_t NetworkSecurity::get_connection_count() const {
  return active_connections_.size();
}

void NetworkSecurity::log_connection(const ConnectionInfo& conn_info,
                                    bool allowed) const {
  std::string log_entry = (allowed ? "ALLOWED: " : "DENIED: ") +
                         conn_info.remote_address + ":" +
                         std::to_string(conn_info.remote_port);
  audit_log_.push_back(log_entry);
}

std::vector<std::string> NetworkSecurity::get_audit_log() const {
  return audit_log_;
}

// SecureNetworkOperations implementation
std::optional<std::string> SecureNetworkOperations::secure_request(
    const std::string& url,
    const std::string& /* method */,
    const std::string& /* data */) {

  if (!validate_url(url)) {
    return std::nullopt;
  }

  auto& security = NetworkSecurity::instance();
  if (security.get_minimum_security_level() >= SecurityLevel::HIGH &&
      !is_secure_url(url)) {
    return std::nullopt;
  }

  // Placeholder for actual network request
  // In production, use proper HTTP library
  return "Response from " + url;
}

bool SecureNetworkOperations::validate_url(const std::string& url) {
  // Basic URL validation
  return url.find("http://") == 0 || url.find("https://") == 0;
}

bool SecureNetworkOperations::is_secure_url(const std::string& url) {
  return url.find("https://") == 0;
}

}  // namespace SolarSystem::Security
