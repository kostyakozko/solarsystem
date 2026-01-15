/**
 * @file resource_manager.hpp
 * @brief Comprehensive resource management system (Task 17)
 *
 * Implements requirements 8.1 and 8.5:
 * - RAII-based resource management
 * - Resource leak detection and prevention
 * - Resource usage monitoring and reporting
 * - Resource cleanup verification
 */

#pragma once

#include "solar_utils/export.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SolarSystem::Utils {

/**
 * @brief Resource types tracked by the system
 */
enum class ResourceType {
  Memory,
  FileHandle,
  NetworkConnection,
  TemporaryFile,
  ThreadHandle,
  MutexLock,
  Custom
};

/**
 * @brief Resource allocation information
 */
struct ResourceInfo {
  std::string id;
  ResourceType type;
  size_t size_bytes = 0;
  std::chrono::system_clock::time_point allocated_at;
  std::string location;  // File:line where allocated
  std::string description;
  void* resource_ptr = nullptr;
  std::function<void()> cleanup_function;

  ResourceInfo() = default;
  ResourceInfo(std::string res_id, ResourceType res_type, size_t size = 0, std::string res_location = "",
               std::string res_description = "")
      : id(std::move(res_id)),
        type(res_type),
        size_bytes(size),
        allocated_at(std::chrono::system_clock::now()),
        location(std::move(res_location)),
        description(std::move(res_description)) {}
};

/**
 * @brief Resource usage statistics
 */
struct ResourceStats {
  size_t total_allocations = 0;
  size_t current_allocations = 0;
  size_t peak_allocations = 0;
  size_t total_bytes_allocated = 0;
  size_t current_bytes_allocated = 0;
  size_t peak_bytes_allocated = 0;
  size_t leak_count = 0;
  std::chrono::milliseconds total_cleanup_time{0};
  std::chrono::system_clock::time_point last_cleanup;

  // Per-type statistics
  std::unordered_map<ResourceType, size_t> allocations_by_type;
  std::unordered_map<ResourceType, size_t> bytes_by_type;
};

/**
 * @brief Resource leak detection result
 */
struct LeakDetectionResult {
  bool has_leaks = false;
  size_t leak_count = 0;
  size_t leaked_bytes = 0;
  std::vector<ResourceInfo> leaked_resources;
  std::string summary;
  std::vector<std::string> recommendations;
};

/**
 * @brief Configuration for resource management
 */
struct ResourceManagerConfig {
  bool enable_leak_detection = true;
  bool enable_usage_monitoring = true;
  bool enable_automatic_cleanup = true;
  bool enable_resource_tracking = true;

  // Thresholds
  size_t max_memory_bytes = 1024 * 1024 * 1024;  // 1GB
  size_t max_file_handles = 1000;
  size_t max_network_connections = 100;
  std::chrono::seconds cleanup_interval{30};
  std::chrono::seconds resource_timeout{300};  // 5 minutes

  // Monitoring
  bool log_allocations = false;
  bool log_deallocations = false;
  bool log_cleanup_operations = true;
  std::string log_file_path;
};

/**
 * @brief RAII-based resource guard
 */
template <typename T>
class ResourceGuard {
 public:
  ResourceGuard() = default;

  explicit ResourceGuard(T* resource, std::function<void(T*)> deleter = nullptr)
      : resource_(resource), deleter_(deleter) {
    if (resource_) {
      register_resource();
    }
  }

  // Move constructor
  ResourceGuard(ResourceGuard&& other) noexcept
      : resource_(other.resource_),
        deleter_(std::move(other.deleter_)),
        resource_id_(std::move(other.resource_id_)) {
    other.resource_ = nullptr;
  }

  // Move assignment
  ResourceGuard& operator=(ResourceGuard&& other) noexcept {
    if (this != &other) {
      cleanup();
      resource_ = other.resource_;
      deleter_ = std::move(other.deleter_);
      resource_id_ = std::move(other.resource_id_);
      other.resource_ = nullptr;
    }
    return *this;
  }

  // Disable copy
  ResourceGuard(const ResourceGuard&) = delete;
  ResourceGuard& operator=(const ResourceGuard&) = delete;

  ~ResourceGuard() { cleanup(); }

  T* get() const { return resource_; }
  T* operator->() const { return resource_; }
  T& operator*() const { return *resource_; }

  explicit operator bool() const { return resource_ != nullptr; }

  T* release() {
    T* temp = resource_;
    resource_ = nullptr;
    unregister_resource();
    return temp;
  }

  void reset(T* new_resource = nullptr, std::function<void(T*)> new_deleter = nullptr) {
    cleanup();
    resource_ = new_resource;
    deleter_ = new_deleter;
    if (resource_) {
      register_resource();
    }
  }

 private:
  T* resource_ = nullptr;
  std::function<void(T*)> deleter_;
  std::string resource_id_;

  void register_resource();
  void unregister_resource();
  void cleanup();
};

/**
 * @brief Comprehensive resource management system
 */
class SOLAR_UTILS_API ResourceManager {
 public:
  static ResourceManager& instance();

  // Configuration
  void configure(const ResourceManagerConfig& config);
  const ResourceManagerConfig& get_config() const { return config_; }

  // Resource registration and tracking
  std::string register_resource(const ResourceInfo& info);
  bool unregister_resource(const std::string& resource_id);
  void update_resource_size(const std::string& resource_id, size_t new_size);

  // Resource monitoring
  ResourceStats get_statistics() const;
  ResourceStats get_statistics_by_type(ResourceType type) const;
  std::vector<ResourceInfo> get_active_resources() const;
  std::vector<ResourceInfo> get_resources_by_type(ResourceType type) const;

  // Leak detection
  LeakDetectionResult detect_leaks() const;
  LeakDetectionResult detect_leaks_by_type(ResourceType type) const;
  void mark_resource_as_expected_leak(const std::string& resource_id);

  // Cleanup operations
  void cleanup_expired_resources();
  void cleanup_resources_by_type(ResourceType type);
  void force_cleanup_all();
  size_t cleanup_resources_older_than(std::chrono::seconds age);

  // Resource limits and validation
  bool check_resource_limits() const;
  bool can_allocate_resource(ResourceType type, size_t size_bytes = 0) const;
  void enforce_resource_limits();

  // Monitoring and reporting
  void start_monitoring();
  void stop_monitoring();
  void generate_usage_report(std::ostream& output) const;
  void log_resource_operation(const std::string& operation, const ResourceInfo& info);

  // Utility methods
  void set_cleanup_callback(std::function<void(const ResourceInfo&)> callback);
  void set_leak_detection_callback(std::function<void(const LeakDetectionResult&)> callback);

 private:
  ResourceManager() = default;
  ~ResourceManager();

  // Disable copy and move
  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;

  mutable std::mutex resources_mutex_;
  std::unordered_map<std::string, ResourceInfo> active_resources_;
  std::unordered_set<std::string> expected_leaks_;
  ResourceStats stats_;
  ResourceManagerConfig config_;

  // Monitoring
  std::atomic<bool> monitoring_active_{false};
  std::unique_ptr<std::thread> monitoring_thread_;
  std::function<void(const ResourceInfo&)> cleanup_callback_;
  std::function<void(const LeakDetectionResult&)> leak_detection_callback_;

  // Internal methods
  void monitoring_loop();
  std::string generate_resource_id() const;
  void update_statistics(const ResourceInfo& info, bool is_allocation);
  bool is_resource_expired(const ResourceInfo& info) const;
  void log_to_file(const std::string& message) const;
};

/**
 * @brief RAII wrapper for memory allocations
 */
class ManagedMemory {
 public:
  explicit ManagedMemory(size_t size, const std::string& description = "");
  ~ManagedMemory();

  // Disable copy, enable move
  ManagedMemory(const ManagedMemory&) = delete;
  ManagedMemory& operator=(const ManagedMemory&) = delete;
  ManagedMemory(ManagedMemory&& other) noexcept;
  ManagedMemory& operator=(ManagedMemory&& other) noexcept;

  void* get() const { return memory_; }
  size_t size() const { return size_; }
  bool is_valid() const { return memory_ != nullptr; }

  template <typename T>
  T* as() const {
    return static_cast<T*>(memory_);
  }

 private:
  void* memory_ = nullptr;
  size_t size_ = 0;
  std::string resource_id_;
};

/**
 * @brief RAII wrapper for temporary files
 */
class ManagedTempFile {
 public:
  explicit ManagedTempFile(const std::string& prefix = "solar_temp",
                           const std::string& suffix = ".tmp");
  ~ManagedTempFile();

  // Disable copy, enable move
  ManagedTempFile(const ManagedTempFile&) = delete;
  ManagedTempFile& operator=(const ManagedTempFile&) = delete;
  ManagedTempFile(ManagedTempFile&& other) noexcept;
  ManagedTempFile& operator=(ManagedTempFile&& other) noexcept;

  const std::string& path() const { return file_path_; }
  bool exists() const;
  bool remove();

 private:
  std::string file_path_;
  std::string resource_id_;
  bool auto_remove_ = true;
};

/**
 * @brief Resource usage monitor for scoped monitoring
 */
class ResourceUsageMonitor {
 public:
  explicit ResourceUsageMonitor(const std::string& scope_name);
  ~ResourceUsageMonitor();

  // Get current usage in this scope
  ResourceStats get_scope_usage() const;
  void log_checkpoint(const std::string& checkpoint_name);

 private:
  std::string scope_name_;
  ResourceStats initial_stats_;
  std::chrono::system_clock::time_point start_time_;
  std::vector<std::pair<std::string, ResourceStats>> checkpoints_;
};

/**
 * @brief Utility macros for resource tracking
 */
#define SOLAR_REGISTER_RESOURCE(type, ptr, size, desc)                                            \
  SolarSystem::Utils::ResourceManager::instance().register_resource(                              \
      SolarSystem::Utils::ResourceInfo(#ptr, type, size, __FILE__ ":" + std::to_string(__LINE__), \
                                       desc))

#define SOLAR_UNREGISTER_RESOURCE(id) \
  SolarSystem::Utils::ResourceManager::instance().unregister_resource(id)

#define SOLAR_MONITOR_SCOPE(name) SolarSystem::Utils::ResourceUsageMonitor _scope_monitor(name)

#define SOLAR_MANAGED_MEMORY(size, desc) SolarSystem::Utils::ManagedMemory(size, desc)

#define SOLAR_MANAGED_TEMP_FILE(prefix, suffix) SolarSystem::Utils::ManagedTempFile(prefix, suffix)

/**
 * @brief Resource type to string conversion
 */
std::string resource_type_to_string(ResourceType type);

/**
 * @brief String to resource type conversion
 */
ResourceType string_to_resource_type(const std::string& type_str);

}  // namespace SolarSystem::Utils
