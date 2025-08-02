/**
 * @file test_port_manager.hpp
 * @brief Thread-safe port allocation system for test environment stability
 *
 * Provides centralized port management to prevent conflicts between concurrent tests.
 * Ensures proper resource allocation and cleanup for network-based testing.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace TestUtils {

/**
 * @brief Thread-safe port allocation manager for tests
 *
 * Manages port allocation across concurrent test execution to prevent conflicts.
 * Provides automatic cleanup and resource tracking.
 */
class TestPortManager {
 public:
  /**
   * @brief Port allocation result
   */
  struct PortAllocation {
    int port = -1;
    bool success = false;
    std::string error_message;
    std::chrono::system_clock::time_point allocated_at;
    std::string test_name;
  };

  /**
   * @brief Port range configuration
   */
  struct PortRange {
    int start_port = 8100;   // Start higher to avoid common conflicts
    int end_port = 8999;     // Large range for concurrent tests
    int max_attempts = 100;  // Maximum allocation attempts
  };

  // Singleton access
  static TestPortManager& instance();

  // Port allocation
  PortAllocation allocate_port(const std::string& test_name = "");
  PortAllocation allocate_port_in_range(int start_port, int end_port,
                                        const std::string& test_name = "");
  bool release_port(int port);
  void release_all_ports_for_test(const std::string& test_name);

  // Port availability checking
  bool is_port_available(int port) const;
  bool wait_for_port_available(int port, int timeout_seconds = 10) const;
  bool wait_for_server_ready(int port, int timeout_seconds = 10) const;

  // Resource management
  void cleanup_expired_allocations(int max_age_minutes = 30);
  void cleanup_all_allocations();
  std::vector<int> get_allocated_ports() const;
  size_t get_allocation_count() const;

  // Configuration
  void set_port_range(const PortRange& range);
  PortRange get_port_range() const;

  // Diagnostics
  void print_allocation_status() const;
  std::string get_allocation_report() const;

 private:
  TestPortManager() = default;
  ~TestPortManager();

  // Non-copyable, non-movable
  TestPortManager(const TestPortManager&) = delete;
  TestPortManager& operator=(const TestPortManager&) = delete;
  TestPortManager(TestPortManager&&) = delete;
  TestPortManager& operator=(TestPortManager&&) = delete;

  // Internal state
  mutable std::mutex allocation_mutex_;
  std::map<int, PortAllocation> allocated_ports_;
  PortRange port_range_;
  std::atomic<int> next_port_hint_{8100};

  // Helper methods
  bool is_port_in_use_system(int port) const;
  int find_next_available_port(int start_port, int end_port) const;
  void cleanup_stale_allocations_unsafe();
};

/**
 * @brief RAII port allocation wrapper
 *
 * Automatically releases port when going out of scope.
 * Provides exception-safe port management.
 */
class ScopedPortAllocation {
 public:
  explicit ScopedPortAllocation(const std::string& test_name = "");
  explicit ScopedPortAllocation(int start_port, int end_port, const std::string& test_name = "");
  ~ScopedPortAllocation();

  // Non-copyable, movable
  ScopedPortAllocation(const ScopedPortAllocation&) = delete;
  ScopedPortAllocation& operator=(const ScopedPortAllocation&) = delete;
  ScopedPortAllocation(ScopedPortAllocation&& other) noexcept;
  ScopedPortAllocation& operator=(ScopedPortAllocation&& other) noexcept;

  // Access
  int port() const { return allocation_.port; }
  bool is_valid() const { return allocation_.success; }
  const std::string& error_message() const { return allocation_.error_message; }
  const TestPortManager::PortAllocation& allocation() const { return allocation_; }

  // Operations
  bool wait_for_server_ready(int timeout_seconds = 10) const;
  std::string base_url() const;

 private:
  TestPortManager::PortAllocation allocation_;
  bool released_ = false;
};

/**
 * @brief Test resource cleanup manager
 *
 * Tracks and cleans up test resources including ports, temporary files,
 * and processes to ensure test isolation.
 */
class TestResourceManager {
 public:
  /**
   * @brief Resource types that can be tracked
   */
  enum class ResourceType {
    PORT,
    TEMPORARY_FILE,
    TEMPORARY_DIRECTORY,
    PROCESS,
    NETWORK_CONNECTION
  };

  /**
   * @brief Resource registration information
   */
  struct ResourceInfo {
    ResourceType type;
    std::string identifier;
    std::string test_name;
    std::chrono::system_clock::time_point created_at;
    std::map<std::string, std::string> metadata;
  };

  // Singleton access
  static TestResourceManager& instance();

  // Resource registration
  void register_resource(ResourceType type, const std::string& identifier,
                         const std::string& test_name = "",
                         const std::map<std::string, std::string>& metadata = {});
  void unregister_resource(ResourceType type, const std::string& identifier);
  void unregister_all_resources_for_test(const std::string& test_name);

  // Cleanup operations
  void cleanup_resources_for_test(const std::string& test_name);
  void cleanup_expired_resources(int max_age_minutes = 30);
  void cleanup_all_resources();

  // Resource queries
  std::vector<ResourceInfo> get_resources_for_test(const std::string& test_name) const;
  std::vector<ResourceInfo> get_resources_by_type(ResourceType type) const;
  size_t get_resource_count() const;

  // Diagnostics
  void print_resource_status() const;
  std::string get_resource_report() const;

 private:
  TestResourceManager() = default;
  ~TestResourceManager();

  // Non-copyable, non-movable
  TestResourceManager(const TestResourceManager&) = delete;
  TestResourceManager& operator=(const TestResourceManager&) = delete;

  // Internal state
  mutable std::mutex resource_mutex_;
  std::map<std::pair<ResourceType, std::string>, ResourceInfo> resources_;
  size_t resource_count_ = 0;

  // Cleanup methods
  void cleanup_port_resource(const ResourceInfo& resource);
  void cleanup_file_resource(const ResourceInfo& resource);
  void cleanup_directory_resource(const ResourceInfo& resource);
  void cleanup_process_resource(const ResourceInfo& resource);
  void cleanup_network_resource(const ResourceInfo& resource);
};

/**
 * @brief Test environment isolation helper
 *
 * Provides utilities for creating isolated test environments
 * with proper resource management and cleanup.
 */
class TestEnvironmentIsolation {
 public:
  /**
   * @brief Isolated test environment configuration
   */
  struct EnvironmentConfig {
    std::string test_name;
    bool isolate_ports = true;
    bool isolate_files = true;
    bool isolate_processes = true;
    int port_range_start = 8100;
    int port_range_end = 8999;
    std::string temp_dir_prefix = "solar_test_";
    int cleanup_timeout_seconds = 30;
  };

  /**
   * @brief Isolated environment instance
   */
  class IsolatedEnvironment {
   public:
    explicit IsolatedEnvironment(const EnvironmentConfig& config);
    ~IsolatedEnvironment();

    // Non-copyable, movable
    IsolatedEnvironment(const IsolatedEnvironment&) = delete;
    IsolatedEnvironment& operator=(const IsolatedEnvironment&) = delete;
    IsolatedEnvironment(IsolatedEnvironment&&) = default;
    IsolatedEnvironment& operator=(IsolatedEnvironment&&) = default;

    // Resource allocation
    ScopedPortAllocation allocate_port();
    std::string create_temp_file(const std::string& content = "");
    std::string create_temp_directory();

    // Environment access
    const std::string& test_name() const { return config_.test_name; }
    const EnvironmentConfig& config() const { return config_; }

    // Cleanup
    void cleanup();

   private:
    EnvironmentConfig config_;
    std::vector<std::string> temp_files_;
    std::vector<std::string> temp_directories_;
    bool cleaned_up_ = false;
  };

  // Environment creation
  static std::unique_ptr<IsolatedEnvironment> create_environment(const EnvironmentConfig& config);
  static std::unique_ptr<IsolatedEnvironment> create_environment(const std::string& test_name);

 private:
  TestEnvironmentIsolation() = delete;
};

}  // namespace TestUtils
