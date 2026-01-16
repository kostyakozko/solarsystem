/**
 * @file resource_manager.cpp
 * @brief Implementation of comprehensive resource management system
 */

#include "solar_utils/resource_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

namespace SolarSystem::Utils {

// ResourceManager implementation
ResourceManager& ResourceManager::instance() {
  static ResourceManager instance;
  return instance;
}

ResourceManager::~ResourceManager() {
  stop_monitoring();

  // Perform final leak detection
  if (config_.enable_leak_detection) {
    auto leak_result = detect_leaks();
    if (leak_result.has_leaks && leak_detection_callback_) {
      leak_detection_callback_(leak_result);
    }
  }

  // Force cleanup of remaining resources
  force_cleanup_all();
}

void ResourceManager::configure(const ResourceManagerConfig& config) {
  std::lock_guard<std::mutex> lock(resources_mutex_);
  config_ = config;

  // Restart monitoring if it was active
  if (monitoring_active_.load()) {
    stop_monitoring();
    start_monitoring();
  }
}

std::string ResourceManager::register_resource(const ResourceInfo& info) {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  if (!config_.enable_resource_tracking) {
    return "";
  }

  // Generate unique ID if not provided
  std::string resource_id = info.id.empty() ? generate_resource_id() : info.id;

  // Create resource info copy with generated ID
  ResourceInfo resource_info = info;
  resource_info.id = resource_id;
  resource_info.allocated_at = std::chrono::system_clock::now();

  // Store the resource
  active_resources_[resource_id] = resource_info;

  // Update statistics
  update_statistics(resource_info, true);

  // Log if enabled
  if (config_.log_allocations) {
    log_resource_operation("ALLOCATE", resource_info);
  }

  return resource_id;
}

bool ResourceManager::unregister_resource(const std::string& resource_id) {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  auto it = active_resources_.find(resource_id);
  if (it == active_resources_.end()) {
    return false;
  }

  const ResourceInfo& info = it->second;

  // Execute cleanup function if provided
  if (info.cleanup_function) {
    try {
      info.cleanup_function();
    } catch (const std::exception& e) {
      log_to_file("Cleanup function failed for resource " + resource_id + ": " + e.what());
    }
  }

  // Update statistics
  update_statistics(info, false);

  // Log if enabled
  if (config_.log_deallocations) {
    log_resource_operation("DEALLOCATE", info);
  }

  // Remove from active res
  active_resources_.erase(it);

  return true;
}

void ResourceManager::update_resource_size(const std::string& resource_id, size_t new_size) {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  auto it = active_resources_.find(resource_id);
  if (it != active_resources_.end()) {
    size_t old_size = it->second.size_bytes;
    it->second.size_bytes = new_size;

    // Update statistics
    stats_.current_bytes_allocated = stats_.current_bytes_allocated - old_size + new_size;
    if (stats_.current_bytes_allocated > stats_.peak_bytes_allocated) {
      stats_.peak_bytes_allocated = stats_.current_bytes_allocated;
    }
  }
}

ResourceStats ResourceManager::get_statistics() const {
  std::lock_guard<std::mutex> lock(resources_mutex_);
  return stats_;
}

ResourceStats ResourceManager::get_statistics_by_type(ResourceType type) const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  ResourceStats type_stats;
  for (const auto& [id, info] : active_resources_) {
    if (info.type == type) {
      type_stats.current_allocations++;
      type_stats.current_bytes_allocated += info.size_bytes;
    }
  }

  // Get historical data from main stats
  auto type_it = stats_.allocations_by_type.find(type);
  if (type_it != stats_.allocations_by_type.end()) {
    type_stats.total_allocations = type_it->second;
  }

  auto bytes_it = stats_.bytes_by_type.find(type);
  if (bytes_it != stats_.bytes_by_type.end()) {
    type_stats.total_bytes_allocated = bytes_it->second;
  }

  return type_stats;
}

std::vector<ResourceInfo> ResourceManager::get_active_resources() const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  std::vector<ResourceInfo> resources;
  resources.reserve(active_resources_.size());

  for (const auto& [id, info] : active_resources_) {
    resources.push_back(info);
  }

  return resources;
}

std::vector<ResourceInfo> ResourceManager::get_resources_by_type(ResourceType type) const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  std::vector<ResourceInfo> resources;
  for (const auto& [id, info] : active_resources_) {
    if (info.type == type) {
      resources.push_back(info);
    }
  }

  return resources;
}

LeakDetectionResult ResourceManager::detect_leaks() const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  LeakDetectionResult result;

  if (!config_.enable_leak_detection) {
    return result;
  }

  auto now = std::chrono::system_clock::now();

  for (const auto& [id, info] : active_resources_) {
    // Skip expected leaks
    if (expected_leaks_.find(id) != expected_leaks_.end()) {
      continue;
    }

    // Check if resource has been alive too long
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - info.allocated_at);
    if (age > config_.resource_timeout) {
      result.leaked_resources.push_back(info);
      result.leaked_bytes += info.size_bytes;
    }
  }

  result.has_leaks = !result.leaked_resources.empty();
  result.leak_count = result.leaked_resources.size();

  if (result.has_leaks) {
    std::ostringstream summary;
    summary << "Detected " << result.leak_count << " potential resource leaks " << "totaling "
            << result.leaked_bytes << " bytes";
    result.summary = summary.str();

    // Generate recommendations
    result.recommendations.push_back("Review resource allocation patterns");
    result.recommendations.push_back("Ensure proper RAII usage");
    result.recommendations.push_back("Check for exception safety in cleanup code");
    if (result.leaked_bytes > 1024 * 1024) {
      result.recommendations.push_back("Large memory leaks detected - investigate immediately");
    }
  }

  return result;
}

LeakDetectionResult ResourceManager::detect_leaks_by_type(ResourceType type) const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  LeakDetectionResult result;

  if (!config_.enable_leak_detection) {
    return result;
  }

  auto now = std::chrono::system_clock::now();

  for (const auto& [id, info] : active_resources_) {
    if (info.type != type) {
      continue;
    }

    // Skip expected leaks
    if (expected_leaks_.find(id) != expected_leaks_.end()) {
      continue;
    }

    // Check if resource has been alive too long
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - info.allocated_at);
    if (age > config_.resource_timeout) {
      result.leaked_resources.push_back(info);
      result.leaked_bytes += info.size_bytes;
    }
  }

  result.has_leaks = !result.leaked_resources.empty();
  result.leak_count = result.leaked_resources.size();

  if (result.has_leaks) {
    std::ostringstream summary;
    summary << "Detected " << result.leak_count << " " << resource_type_to_string(type) << " leaks "
            << "totaling " << result.leaked_bytes << " bytes";
    result.summary = summary.str();
  }

  return result;
}

void ResourceManager::mark_resource_as_expected_leak(const std::string& resource_id) {
  std::lock_guard<std::mutex> lock(resources_mutex_);
  expected_leaks_.insert(resource_id);
}

void ResourceManager::cleanup_expired_resources() {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  if (!config_.enable_automatic_cleanup) {
    return;
  }

  auto start_time = std::chrono::steady_clock::now();
  size_t cleaned_count = 0;

  auto it = active_resources_.begin();
  while (it != active_resources_.end()) {
    const ResourceInfo& info = it->second;

    if (is_resource_expired(info)) {
      // Execute cleanup function if provided
      if (info.cleanup_function) {
        try {
          info.cleanup_function();
          cleaned_count++;
        } catch (const std::exception& e) {
          log_to_file("Cleanup function failed for expired resource " + info.id + ": " + e.what());
        }
      }

      // Update statistics
      update_statistics(info, false);

      // Log cleanup
      if (config_.log_cleanup_operations) {
        log_resource_operation("CLEANUP_EXPIRED", info);
      }

      // Notify callback
      if (cleanup_callback_) {
        cleanup_callback_(info);
      }

      it = active_resources_.erase(it);
    } else {
      ++it;
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  auto cleanup_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  stats_.total_cleanup_time += cleanup_duration;
  stats_.last_cleanup = std::chrono::system_clock::now();

  if (cleaned_count > 0) {
    log_to_file("Cleaned up " + std::to_string(cleaned_count) + " expired resources in " +
                std::to_string(cleanup_duration.count()) + "ms");
  }
}

void ResourceManager::cleanup_resources_by_type(ResourceType type) {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  auto it = active_resources_.begin();
  while (it != active_resources_.end()) {
    const ResourceInfo& info = it->second;

    if (info.type == type) {
      // Execute cleanup function if provided
      if (info.cleanup_function) {
        try {
          info.cleanup_function();
        } catch (const std::exception& e) {
          log_to_file("Cleanup function failed for resource " + info.id + ": " + e.what());
        }
      }

      // Update statistics
      update_statistics(info, false);

      // Log cleanup
      if (config_.log_cleanup_operations) {
        log_resource_operation("CLEANUP_BY_TYPE", info);
      }

      // Notify callback
      if (cleanup_callback_) {
        cleanup_callback_(info);
      }

      it = active_resources_.erase(it);
    } else {
      ++it;
    }
  }
}

void ResourceManager::force_cleanup_all() {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  for (const auto& [id, info] : active_resources_) {
    // Execute cleanup function if provided
    if (info.cleanup_function) {
      try {
        info.cleanup_function();
      } catch (const std::exception& e) {
        log_to_file("Cleanup function failed for resource " + info.id + ": " + e.what());
      }
    }

    // Notify callback
    if (cleanup_callback_) {
      cleanup_callback_(info);
    }
  }

  active_resources_.clear();

  // Reset current counters
  stats_.current_allocations = 0;
  stats_.current_bytes_allocated = 0;
}

size_t ResourceManager::cleanup_resources_older_than(std::chrono::seconds age) {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  auto now = std::chrono::system_clock::now();
  size_t cleaned_count = 0;

  auto it = active_resources_.begin();
  while (it != active_resources_.end()) {
    const ResourceInfo& info = it->second;

    auto resource_age = std::chrono::duration_cast<std::chrono::seconds>(now - info.allocated_at);
    if (resource_age > age) {
      // Execute cleanup function if provided
      if (info.cleanup_function) {
        try {
          info.cleanup_function();
          cleaned_count++;
        } catch (const std::exception& e) {
          log_to_file("Cleanup function failed for old resource " + info.id + ": " + e.what());
        }
      }

      // Update statistics
      update_statistics(info, false);

      // Log cleanup
      if (config_.log_cleanup_operations) {
        log_resource_operation("CLEANUP_OLD", info);
      }

      // Notify callback
      if (cleanup_callback_) {
        cleanup_callback_(info);
      }

      it = active_resources_.erase(it);
    } else {
      ++it;
    }
  }

  return cleaned_count;
}

bool ResourceManager::check_resource_limits() const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  // Check memory limit
  if (stats_.current_bytes_allocated > config_.max_memory_bytes) {
    return false;
  }

  // Check file handle limit
  auto file_stats = get_statistics_by_type(ResourceType::FileHandle);
  if (file_stats.current_allocations > config_.max_file_handles) {
    return false;
  }

  // Check network connection limit
  auto network_stats = get_statistics_by_type(ResourceType::NetworkConnection);
  if (network_stats.current_allocations > config_.max_network_connections) {
    return false;
  }

  return true;
}

bool ResourceManager::can_allocate_resource(ResourceType type, size_t size_bytes) const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  switch (type) {
    case ResourceType::Memory:
      return (stats_.current_bytes_allocated + size_bytes) <= config_.max_memory_bytes;

    case ResourceType::FileHandle: {
      auto file_stats = get_statistics_by_type(ResourceType::FileHandle);
      return file_stats.current_allocations < config_.max_file_handles;
    }

    case ResourceType::NetworkConnection: {
      auto network_stats = get_statistics_by_type(ResourceType::NetworkConnection);
      return network_stats.current_allocations < config_.max_network_connections;
    }

    default:
      return true;  // No specific limits for other types
  }
}

void ResourceManager::enforce_resource_limits() {
  if (!check_resource_limits()) {
    // Try to free up resources by cleaning up expired ones
    cleanup_expired_resources();

    // If still over limits, log warning
    if (!check_resource_limits()) {
      log_to_file("WARNING: Resource limits exceeded even after cleanup");
    }
  }
}

void ResourceManager::start_monitoring() {
  if (monitoring_active_.load()) {
    return;
  }

  monitoring_active_.store(true);
  monitoring_thread_ = std::make_unique<std::thread>(&ResourceManager::monitoring_loop, this);
}

void ResourceManager::stop_monitoring() {
  if (!monitoring_active_.load()) {
    return;
  }

  monitoring_active_.store(false);
  if (monitoring_thread_ && monitoring_thread_->joinable()) {
    monitoring_thread_->join();
  }
  monitoring_thread_.reset();
}

void ResourceManager::generate_usage_report(std::ostream& output) const {
  std::lock_guard<std::mutex> lock(resources_mutex_);

  output << "=== Resource Usage Report ===\n";
  output << "Generated at: " << std::chrono::system_clock::now().time_since_epoch().count()
         << "\n\n";

  output << "Overall Statistics:\n";
  output << "  Total allocations: " << stats_.total_allocations << "\n";
  output << "  Current allocations: " << stats_.current_allocations << "\n";
  output << "  Peak allocations: " << stats_.peak_allocations << "\n";
  output << "  Total bytes allocated: " << stats_.total_bytes_allocated << "\n";
  output << "  Current bytes allocated: " << stats_.current_bytes_allocated << "\n";
  output << "  Peak bytes allocated: " << stats_.peak_bytes_allocated << "\n";
  output << "  Leak count: " << stats_.leak_count << "\n";
  output << "  Total cleanup time: " << stats_.total_cleanup_time.count() << "ms\n\n";

  output << "By Resource Type:\n";
  for (const auto& [type, count] : stats_.allocations_by_type) {
    output << "  " << resource_type_to_string(type) << ": " << count << " allocations";
    auto bytes_it = stats_.bytes_by_type.find(type);
    if (bytes_it != stats_.bytes_by_type.end()) {
      output << " (" << bytes_it->second << " bytes)";
    }
    output << "\n";
  }

  output << "\nActive Resources:\n";
  for (const auto& [id, info] : active_resources_) {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() -
                                                                info.allocated_at);

    output << "  " << id << " (" << resource_type_to_string(info.type) << "): " << info.size_bytes
           << " bytes, age " << age.count() << "s";
    if (!info.description.empty()) {
      output << " - " << info.description;
    }
    if (!info.location.empty()) {
      output << " [" << info.location << "]";
    }
    output << "\n";
  }

  // Leak detection
  auto leak_result = detect_leaks();
  if (leak_result.has_leaks) {
    output << "\nPotential Leaks Detected:\n";
    output << "  " << leak_result.summary << "\n";
    for (const auto& recommendation : leak_result.recommendations) {
      output << "  - " << recommendation << "\n";
    }
  }
}

void ResourceManager::log_resource_operation(const std::string& operation,
                                             const ResourceInfo& info) {
  if (!config_.log_file_path.empty()) {
    std::ostringstream log_entry;
    log_entry << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] "
              << operation << " " << info.id << " (" << resource_type_to_string(info.type) << ") "
              << info.size_bytes << " bytes";
    if (!info.description.empty()) {
      log_entry << " - " << info.description;
    }
    if (!info.location.empty()) {
      log_entry << " [" << info.location << "]";
    }

    log_to_file(log_entry.str());
  }
}

void ResourceManager::set_cleanup_callback(std::function<void(const ResourceInfo&)> callback) {
  cleanup_callback_ = callback;
}

void ResourceManager::set_leak_detection_callback(
    std::function<void(const LeakDetectionResult&)> callback) {
  leak_detection_callback_ = callback;
}

void ResourceManager::monitoring_loop() {
  while (monitoring_active_.load()) {
    std::this_thread::sleep_for(config_.cleanup_interval);

    if (!monitoring_active_.load()) {
      break;
    }

    // Perform periodic cleanup
    cleanup_expired_resources();

    // Check resource limits
    enforce_resource_limits();

    // Perform leak detection
    if (config_.enable_leak_detection) {
      auto leak_result = detect_leaks();
      if (leak_result.has_leaks && leak_detection_callback_) {
        leak_detection_callback_(leak_result);
      }
    }
  }
}

std::string ResourceManager::generate_resource_id() const {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream id;
  id << "res_";
  for (int i = 0; i < 8; ++i) {
    id << std::hex << dis(gen);
  }

  return id.str();
}

void ResourceManager::update_statistics(const ResourceInfo& info, bool is_allocation) {
  if (is_allocation) {
    stats_.total_allocations++;
    stats_.current_allocations++;
    stats_.total_bytes_allocated += info.size_bytes;
    stats_.current_bytes_allocated += info.size_bytes;

    if (stats_.current_allocations > stats_.peak_allocations) {
      stats_.peak_allocations = stats_.current_allocations;
    }
    if (stats_.current_bytes_allocated > stats_.peak_bytes_allocated) {
      stats_.peak_bytes_allocated = stats_.current_bytes_allocated;
    }

    stats_.allocations_by_type[info.type]++;
    stats_.bytes_by_type[info.type] += info.size_bytes;
  } else {
    if (stats_.current_allocations > 0) {
      stats_.current_allocations--;
    }
    if (stats_.current_bytes_allocated >= info.size_bytes) {
      stats_.current_bytes_allocated -= info.size_bytes;
    }
  }
}

bool ResourceManager::is_resource_expired(const ResourceInfo& info) const {
  auto now = std::chrono::system_clock::now();
  auto age = std::chrono::duration_cast<std::chrono::seconds>(now - info.allocated_at);
  return age > config_.resource_timeout;
}

void ResourceManager::log_to_file(const std::string& message) const {
  if (config_.log_file_path.empty()) {
    return;
  }

  try {
    std::ofstream log_file(config_.log_file_path, std::ios::app);
    if (log_file.is_open()) {
      log_file << message << std::endl;
    }
  } catch (const std::exception&) {
    // Ignore logging errors to avoid infinite recursion
  }
}

// ManagedMemory implementation
ManagedMemory::ManagedMemory(size_t size, const std::string& description) : size_(size) {
  if (size > 0) {
    memory_ = std::malloc(size);
    if (memory_) {
      ResourceInfo info("", ResourceType::Memory, size, "", description);
      info.resource_ptr = memory_;
      info.cleanup_function = [this]() {
        if (memory_) {
          std::free(memory_);
          memory_ = nullptr;
        }
      };

      resource_id_ = ResourceManager::instance().register_resource(info);
    }
  }
}

ManagedMemory::~ManagedMemory() {
  if (!resource_id_.empty()) {
    ResourceManager::instance().unregister_resource(resource_id_);
  }
  if (memory_) {
    std::free(memory_);
  }
}

ManagedMemory::ManagedMemory(ManagedMemory&& other) noexcept
    : memory_(other.memory_), size_(other.size_), resource_id_(std::move(other.resource_id_)) {
  other.memory_ = nullptr;
  other.size_ = 0;
}

ManagedMemory& ManagedMemory::operator=(ManagedMemory&& other) noexcept {
  if (this != &other) {
    // Clean up current resource
    if (!resource_id_.empty()) {
      ResourceManager::instance().unregister_resource(resource_id_);
    }
    if (memory_) {
      std::free(memory_);
    }

    // Move from other
    memory_ = other.memory_;
    size_ = other.size_;
    resource_id_ = std::move(other.resource_id_);

    // Clear other
    other.memory_ = nullptr;
    other.size_ = 0;
  }
  return *this;
}

// ManagedTempFile implementation
ManagedTempFile::ManagedTempFile(const std::string& prefix, const std::string& suffix) {
  // Generate temporary file path
  std::filesystem::path temp_dir = std::filesystem::temp_directory_path();

  // Generate unique filename
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream filename;
  filename << prefix << "_";
  for (int i = 0; i < 8; ++i) {
    filename << std::hex << dis(gen);
  }
  filename << suffix;

  file_path_ = (temp_dir / filename.str()).string();

  // Register with resource manager
  ResourceInfo info("", ResourceType::TemporaryFile, 0, "", "Temporary file: " + file_path_);
  info.resource_ptr = const_cast<char*>(file_path_.c_str());
  info.cleanup_function = [this]() {
    if (auto_remove_ && exists()) {
      remove();
    }
  };

  resource_id_ = ResourceManager::instance().register_resource(info);
}

ManagedTempFile::~ManagedTempFile() {
  if (!resource_id_.empty()) {
    ResourceManager::instance().unregister_resource(resource_id_);
  }
  if (auto_remove_ && exists()) {
    remove();
  }
}

ManagedTempFile::ManagedTempFile(ManagedTempFile&& other) noexcept
    : file_path_(std::move(other.file_path_)),
      resource_id_(std::move(other.resource_id_)),
      auto_remove_(other.auto_remove_) {
  other.auto_remove_ = false;
}

ManagedTempFile& ManagedTempFile::operator=(ManagedTempFile&& other) noexcept {
  if (this != &other) {
    // Clean up current resource
    if (!resource_id_.empty()) {
      ResourceManager::instance().unregister_resource(resource_id_);
    }
    if (auto_remove_ && exists()) {
      remove();
    }

    // Move from other
    file_path_ = std::move(other.file_path_);
    resource_id_ = std::move(other.resource_id_);
    auto_remove_ = other.auto_remove_;

    // Clear other
    other.auto_remove_ = false;
  }
  return *this;
}

bool ManagedTempFile::exists() const { return std::filesystem::exists(file_path_); }

bool ManagedTempFile::remove() {
  try {
    return std::filesystem::remove(file_path_);
  } catch (const std::exception&) {
    return false;
  }
}

// ResourceUsageMonitor implementation
ResourceUsageMonitor::ResourceUsageMonitor(const std::string& scope_name)
    : scope_name_(scope_name), start_time_(std::chrono::system_clock::now()) {
  initial_stats_ = ResourceManager::instance().get_statistics();
}

ResourceUsageMonitor::~ResourceUsageMonitor() {
  auto final_stats = ResourceManager::instance().get_statistics();
  auto end_time = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);

  // Log scope statistics
  std::ostringstream log_message;
  log_message << "Scope '" << scope_name_ << "' completed in " << duration.count() << "ms. ";
  log_message << "Allocations: "
              << (final_stats.total_allocations - initial_stats_.total_allocations);
  log_message << ", Bytes: "
              << (final_stats.total_bytes_allocated - initial_stats_.total_bytes_allocated);

  ResourceManager::instance().log_resource_operation(
      "SCOPE_END", ResourceInfo(scope_name_, ResourceType::Custom, 0, "", log_message.str()));
}

ResourceStats ResourceUsageMonitor::get_scope_usage() const {
  auto current_stats = ResourceManager::instance().get_statistics();

  ResourceStats scope_stats;
  scope_stats.total_allocations =
      current_stats.total_allocations - initial_stats_.total_allocations;
  scope_stats.total_bytes_allocated =
      current_stats.total_bytes_allocated - initial_stats_.total_bytes_allocated;
  scope_stats.current_allocations = current_stats.current_allocations;
  scope_stats.current_bytes_allocated = current_stats.current_bytes_allocated;

  return scope_stats;
}

void ResourceUsageMonitor::log_checkpoint(const std::string& checkpoint_name) {
  auto current_stats = ResourceManager::instance().get_statistics();
  checkpoints_.emplace_back(checkpoint_name, current_stats);
}

// Utility functions
std::string resource_type_to_string(ResourceType type) {
  switch (type) {
    case ResourceType::Memory:
      return "Memory";
    case ResourceType::FileHandle:
      return "FileHandle";
    case ResourceType::NetworkConnection:
      return "NetworkConnection";
    case ResourceType::TemporaryFile:
      return "TemporaryFile";
    case ResourceType::ThreadHandle:
      return "ThreadHandle";
    case ResourceType::MutexLock:
      return "MutexLock";
    case ResourceType::Custom:
      return "Custom";
    default:
      return "Unknown";
  }
}

ResourceType string_to_resource_type(const std::string& type_str) {
  if (type_str == "Memory") return ResourceType::Memory;
  if (type_str == "FileHandle") return ResourceType::FileHandle;
  if (type_str == "NetworkConnection") return ResourceType::NetworkConnection;
  if (type_str == "TemporaryFile") return ResourceType::TemporaryFile;
  if (type_str == "ThreadHandle") return ResourceType::ThreadHandle;
  if (type_str == "MutexLock") return ResourceType::MutexLock;
  if (type_str == "Custom") return ResourceType::Custom;
  return ResourceType::Custom;
}

}  // namespace SolarSystem::Utils
