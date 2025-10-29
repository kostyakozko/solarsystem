/**
 * @file test_file_network_security.cpp
 * @brief Unit tests for file system and network security
 */

#include "../utils/test_framework.h"

#include "solar_core/security/file_security.hpp"
#include "solar_core/security/network_security.hpp"

#include <filesystem>
#include <fstream>

using namespace SolarSystem::Security;

int main() {
  TestSuite suite("File and Network Security Tests");

  // File Security Tests
  suite.run_test("Whitelist Policy Add Directory", []() {
    WhitelistPolicy policy;
    policy.add_allowed_directory("/tmp");

    auto result = policy.check_access("/tmp/test.txt", FileAccessMode::READ);
    if (!result.allowed) throw std::runtime_error("Should allow access to whitelisted directory");
  });

  suite.run_test("Whitelist Policy Add File", []() {
    WhitelistPolicy policy;
    policy.add_allowed_file("/etc/hosts");

    auto result = policy.check_access("/etc/hosts", FileAccessMode::READ);
    if (!result.allowed) throw std::runtime_error("Should allow access to whitelisted file");
  });

  suite.run_test("Whitelist Policy Deny Unlisted", []() {
    WhitelistPolicy policy;
    policy.add_allowed_directory("/tmp");

    auto result = policy.check_access("/etc/passwd", FileAccessMode::READ);
    if (result.allowed) throw std::runtime_error("Should deny access to non-whitelisted file");
  });

  suite.run_test("FileSystemSecurity Policy Management", []() {
    auto& fs_security = FileSystemSecurity::instance();

    auto policy = std::make_shared<WhitelistPolicy>();
    policy->add_allowed_directory("/tmp");

    fs_security.set_policy(policy);

    auto retrieved = fs_security.get_policy();
    if (!retrieved) throw std::runtime_error("Should retrieve policy");
  });

  suite.run_test("FileSystemSecurity Access Checks", []() {
    auto& fs_security = FileSystemSecurity::instance();

    auto policy = std::make_shared<WhitelistPolicy>();
    policy->add_allowed_directory("/tmp");
    fs_security.set_policy(policy);

    if (!fs_security.can_read("/tmp/test.txt")) {
      throw std::runtime_error("Should allow read access");
    }

    if (fs_security.can_read("/etc/passwd")) {
      throw std::runtime_error("Should deny read access");
    }
  });

  suite.run_test("FileSystemSecurity Path Validation", []() {
    auto& fs_security = FileSystemSecurity::instance();

    if (!fs_security.is_safe_path("/tmp/test.txt")) {
      throw std::runtime_error("Should consider normal path safe");
    }

    if (fs_security.is_safe_path("/tmp/../etc/passwd")) {
      throw std::runtime_error("Should consider path traversal unsafe");
    }
  });

  suite.run_test("SecureFileOperations Read", []() {
    // Create a test file
    std::filesystem::path test_file = "/tmp/secure_test.txt";
    {
      std::ofstream file(test_file);
      file << "test content";
    }

    auto& fs_security = FileSystemSecurity::instance();
    auto policy = std::make_shared<WhitelistPolicy>();
    policy->add_allowed_directory("/tmp");
    fs_security.set_policy(policy);

    auto content = SecureFileOperations::read_file(test_file);
    if (!content) throw std::runtime_error("Should read file");
    if (*content != "test content") throw std::runtime_error("Wrong content");

    // Clean up
    std::filesystem::remove(test_file);
  });

  // Network Security Tests
  suite.run_test("IPFilter Whitelist", []() {
    IPFilter filter;
    filter.add_whitelist("192.168.1.*");
    filter.add_whitelist("10.0.0.1");

    if (!filter.is_allowed("192.168.1.100")) {
      throw std::runtime_error("Should allow whitelisted IP");
    }

    if (!filter.is_allowed("10.0.0.1")) {
      throw std::runtime_error("Should allow exact whitelisted IP");
    }

    if (filter.is_allowed("172.16.0.1")) {
      throw std::runtime_error("Should deny non-whitelisted IP");
    }
  });

  suite.run_test("IPFilter Blacklist", []() {
    IPFilter filter;
    filter.add_blacklist("192.168.1.100");

    if (filter.is_allowed("192.168.1.100")) {
      throw std::runtime_error("Should block blacklisted IP");
    }

    if (!filter.is_allowed("192.168.1.101")) {
      throw std::runtime_error("Should allow non-blacklisted IP");
    }
  });

  suite.run_test("EncryptionManager Key Generation", []() {
    auto& enc = EncryptionManager::instance();

    auto key = enc.generate_key(32);
    if (key.size() != 32) throw std::runtime_error("Wrong key size");
    if (!enc.validate_key(key)) throw std::runtime_error("Generated key should be valid");
  });

  suite.run_test("EncryptionManager Encryption", []() {
    auto& enc = EncryptionManager::instance();

    std::string data = "secret message";
    std::string key = "encryption_key_12345";

    auto encrypted = enc.encrypt(data, key);
    if (!encrypted) throw std::runtime_error("Encryption should succeed");
    if (*encrypted == data) throw std::runtime_error("Encrypted data should differ");

    auto decrypted = enc.decrypt(*encrypted, key);
    if (!decrypted) throw std::runtime_error("Decryption should succeed");
    if (*decrypted != data) throw std::runtime_error("Decrypted data should match original");
  });

  suite.run_test("EncryptionManager Hashing", []() {
    auto& enc = EncryptionManager::instance();

    std::string data = "test data";
    auto hash1 = enc.hash(data);
    auto hash2 = enc.hash(data);

    if (hash1 != hash2) throw std::runtime_error("Same data should produce same hash");
    if (!enc.verify_hash(data, hash1)) throw std::runtime_error("Hash verification should succeed");
  });

  suite.run_test("NetworkSecurity Protocol Validation", []() {
    auto& net_security = NetworkSecurity::instance();

    if (!net_security.is_secure_protocol(NetworkProtocol::HTTPS)) {
      throw std::runtime_error("HTTPS should be secure");
    }

    if (net_security.is_secure_protocol(NetworkProtocol::HTTP)) {
      throw std::runtime_error("HTTP should not be secure");
    }
  });

  suite.run_test("NetworkSecurity Connection Validation", []() {
    auto& net_security = NetworkSecurity::instance();

    ConnectionInfo conn;
    conn.remote_address = "192.168.1.100";
    conn.remote_port = 8080;
    conn.protocol = NetworkProtocol::HTTPS;

    auto result = net_security.validate_connection(conn);
    if (!result.allowed) throw std::runtime_error("Should allow valid connection");
  });

  suite.run_test("NetworkSecurity IP Filtering", []() {
    auto& net_security = NetworkSecurity::instance();

    auto filter = std::make_shared<IPFilter>();
    filter->add_whitelist("192.168.*");
    net_security.set_ip_filter(filter);

    if (!net_security.is_ip_allowed("192.168.1.1")) {
      throw std::runtime_error("Should allow whitelisted IP");
    }

    if (net_security.is_ip_allowed("10.0.0.1")) {
      throw std::runtime_error("Should block non-whitelisted IP");
    }
  });

  suite.run_test("NetworkSecurity Security Level", []() {
    auto& net_security = NetworkSecurity::instance();

    net_security.set_minimum_security_level(SecurityLevel::HIGH);

    if (net_security.get_minimum_security_level() != SecurityLevel::HIGH) {
      throw std::runtime_error("Should set security level");
    }
  });

  suite.run_test("NetworkSecurity Connection Tracking", []() {
    auto& net_security = NetworkSecurity::instance();

    ConnectionInfo conn;
    conn.remote_address = "192.168.1.100";
    conn.remote_port = 8080;

    size_t initial_count = net_security.get_connection_count();
    net_security.track_connection(conn);

    if (net_security.get_connection_count() != initial_count + 1) {
      throw std::runtime_error("Should track connection");
    }
  });

  suite.run_test("SecureNetworkOperations URL Validation", []() {
    if (!SecureNetworkOperations::validate_url("http://example.com")) {
      throw std::runtime_error("Should validate HTTP URL");
    }

    if (!SecureNetworkOperations::validate_url("https://example.com")) {
      throw std::runtime_error("Should validate HTTPS URL");
    }

    if (SecureNetworkOperations::validate_url("ftp://example.com")) {
      throw std::runtime_error("Should reject non-HTTP URL");
    }
  });

  suite.run_test("SecureNetworkOperations Secure URL Check", []() {
    if (!SecureNetworkOperations::is_secure_url("https://example.com")) {
      throw std::runtime_error("HTTPS should be secure");
    }

    if (SecureNetworkOperations::is_secure_url("http://example.com")) {
      throw std::runtime_error("HTTP should not be secure");
    }
  });

  suite.print_summary();
  return suite.all_passed() ? 0 : 1;
}
