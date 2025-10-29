/**
 * @file network_security.cpp
 * @brief Implementation of network security
 */

#include "solar_core/security/network_security.hpp"

#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <curl/curl.h>
#include <cstring>

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

  // Use AES-256-CBC encryption with OpenSSL
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    return std::nullopt;
  }

  // Generate random IV
  unsigned char iv[EVP_MAX_IV_LENGTH];
  if (RAND_bytes(iv, EVP_MAX_IV_LENGTH) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::nullopt;
  }

  // Derive key from password using SHA-256
  unsigned char derived_key[32];
  SHA256(reinterpret_cast<const unsigned char*>(key.c_str()), key.size(), derived_key);

  // Initialize encryption
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derived_key, iv) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::nullopt;
  }

  // Allocate output buffer
  std::vector<unsigned char> ciphertext(data.size() + static_cast<size_t>(EVP_CIPHER_block_size(EVP_aes_256_cbc())));
  int len = 0;
  int ciphertext_len = 0;

  // Encrypt data
  if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                        reinterpret_cast<const unsigned char*>(data.c_str()),
                        static_cast<int>(data.size())) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::nullopt;
  }
  ciphertext_len = len;

  // Finalize encryption
  if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::nullopt;
  }
  ciphertext_len += len;

  EVP_CIPHER_CTX_free(ctx);

  // Prepend IV to ciphertext
  std::string result;
  result.append(reinterpret_cast<char*>(iv), EVP_MAX_IV_LENGTH);
  result.append(reinterpret_cast<char*>(ciphertext.data()), static_cast<size_t>(ciphertext_len));

  return result;
}

std::optional<std::string> EncryptionManager::decrypt(
    const std::string& encrypted_data,
    const std::string& key) const {

  if (!validate_key(key) || encrypted_data.size() < EVP_MAX_IV_LENGTH) {
    return std::nullopt;
  }

  // Extract IV from beginning of encrypted data
  unsigned char iv[EVP_MAX_IV_LENGTH];
  std::memcpy(iv, encrypted_data.data(), EVP_MAX_IV_LENGTH);

  // Derive key from password using SHA-256
  unsigned char derived_key[32];
  SHA256(reinterpret_cast<const unsigned char*>(key.c_str()), key.size(), derived_key);

  // Initialize decryption
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    return std::nullopt;
  }

  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derived_key, iv) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::nullopt;
  }

  // Allocate output buffer
  size_t ciphertext_len = encrypted_data.size() - EVP_MAX_IV_LENGTH;
  std::vector<unsigned char> plaintext(ciphertext_len + static_cast<size_t>(EVP_CIPHER_block_size(EVP_aes_256_cbc())));
  int len = 0;
  int plaintext_len = 0;

  // Decrypt data
  if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                        reinterpret_cast<const unsigned char*>(encrypted_data.data() + EVP_MAX_IV_LENGTH),
                        static_cast<int>(ciphertext_len)) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::nullopt;
  }
  plaintext_len = len;

  // Finalize decryption
  if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::nullopt;
  }
  plaintext_len += len;

  EVP_CIPHER_CTX_free(ctx);

  return std::string(reinterpret_cast<char*>(plaintext.data()), static_cast<size_t>(plaintext_len));
}

std::string EncryptionManager::generate_key(size_t key_size) const {
  // Use OpenSSL's cryptographically secure random number generator
  std::vector<unsigned char> random_bytes(key_size);

  if (RAND_bytes(random_bytes.data(), static_cast<int>(key_size)) != 1) {
    // Fallback to less secure method if OpenSSL fails
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

  // Convert random bytes to hex string
  std::ostringstream oss;
  for (unsigned char byte : random_bytes) {
    oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
  }

  std::string result = oss.str();
  if (result.size() > key_size) {
    result.resize(key_size);
  }

  return result;
}

bool EncryptionManager::validate_key(const std::string& key) const {
  return !key.empty() && key.size() >= 8;
}

std::string EncryptionManager::hash(const std::string& data) const {
  // Use SHA-256 for secure hashing
  unsigned char hash_bytes[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash_bytes);

  // Convert to hex string
  std::ostringstream oss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hash_bytes[i]);
  }

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

// Callback function to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
  size_t total_size = size * nmemb;
  response->append(static_cast<char*>(contents), total_size);
  return total_size;
}

// SecureNetworkOperations implementation
std::optional<std::string> SecureNetworkOperations::secure_request(
    const std::string& url,
    const std::string& method,
    const std::string& data) {

  if (!validate_url(url)) {
    return std::nullopt;
  }

  auto& security = NetworkSecurity::instance();
  if (security.get_minimum_security_level() >= SecurityLevel::HIGH &&
      !is_secure_url(url)) {
    return std::nullopt;
  }

  // Initialize libcurl
  CURL* curl = curl_easy_init();
  if (!curl) {
    return std::nullopt;
  }

  std::string response;
  CURLcode res;

  // Set URL
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  // Set callback function to capture response
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  // Set HTTP method
  if (method == "POST") {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    if (!data.empty()) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    }
  } else if (method == "PUT") {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    if (!data.empty()) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    }
  } else if (method == "DELETE") {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  }
  // GET is default

  // Security settings
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);       // Max 5 redirects
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);        // 30 second timeout
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);  // 10 second connect timeout

  // SSL/TLS settings for HTTPS
  if (is_secure_url(url)) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
  }

  // Set User-Agent
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "SolarSystem-Suite/4.0.0");

  // Perform the request
  res = curl_easy_perform(curl);

  // Check for errors
  if (res != CURLE_OK) {
    curl_easy_cleanup(curl);
    return std::nullopt;
  }

  // Check HTTP response code
  long response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  curl_easy_cleanup(curl);

  // Return response only for successful HTTP codes
  if (response_code >= 200 && response_code < 300) {
    return response;
  }

  return std::nullopt;
}

bool SecureNetworkOperations::validate_url(const std::string& url) {
  // Basic URL validation
  return url.find("http://") == 0 || url.find("https://") == 0;
}

bool SecureNetworkOperations::is_secure_url(const std::string& url) {
  return url.find("https://") == 0;
}

}  // namespace SolarSystem::Security
