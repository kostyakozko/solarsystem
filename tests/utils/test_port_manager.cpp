/**
 * @file test_port_manager.cpp
 * @brief Implementation of thread-safe port allocation system
 */

#include "test_port_manager.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace TestUtils {

// TestPortManager implementation
TestPortManager& TestPortManager::instance() {
  static TestPortManager instance;
  return instance;
}

TestPortManager::~TestPortManager() { cleanup_all_allocations(); }

TestPortManager::PortAllocation TestPortManager::allocate_port(const std::string& test_name) {
  return allocate_port_in_range(port_range_.start_port, port_range_.end_port, test_name);
}

TestPortManager::PortAllocation TestPortManager::allocate_port_in_range(
    int start_port, int end_port, const std::string& test_name) {
  std::lock_guard<std::mutex> lock(allocation_mutex_);

  PortAllocation allocation;
  allocation.test_name = test_name;
  allocation.allocated_at = std::chrono::system_clock::now();

  // Try to find an available port
  int attempts = 0;
  int current_port = next_port_hint_.load();

  // Ensure we start within the valid range
  if (current_port < start_port || current_port > end_port) {
    current_port = start_port;
  }

  while (attempts < port_range_.max_attempts) {
    // Check if port is already allocated by us
    if (allocated_ports_.find(current_port) != allocated_ports_.end()) {
      current_port++;
      if (current_port > end_port) {
        current_port = start_port;
      }
      attempts++;
      continue;
    }

    // Check if port is available at system level
    if (is_port_available(current_port)) {
      // Successfully allocated
      allocation.port = current_port;
      allocation.success = true;
      allocated_ports_[current_port] = allocation;

      // Update hint for next allocation
      next_port_hint_.store(current_port + 1);

      return allocation;
    }

    current_port++;
    if (current_port > end_port) {
      current_port = start_port;
    }
    attempts++;
  }

  // Failed to allocate
  allocation.success = false;
  allocation.error_message = "No available ports in range " + std::to_string(start_port) + "-" +
                             std::to_string(end_port) + " after " + std::to_string(attempts) +
                             " attempts";

  return allocation;
}

bool TestPortManager::release_port(int port) {
  std::lock_guard<std::mutex> lock(allocation_mutex_);

  auto it = allocated_ports_.find(port);
  if (it != allocated_ports_.end()) {
    allocated_ports_.erase(it);
    return true;
  }

  return false;
}

void TestPortManager::release_all_ports_for_test(const std::string& test_name) {
  std::lock_guard<std::mutex> lock(allocation_mutex_);

  auto it = allocated_ports_.begin();
  while (it != allocated_ports_.end()) {
    if (it->second.test_name == test_name) {
      it = allocated_ports_.erase(it);
    } else {
      ++it;
    }
  }
}

bool TestPortManager::is_port_available(int port) const { return !is_port_in_use_system(port); }

bool TestPortManager::wait_for_port_available(int port, int timeout_seconds) const {
  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::seconds(timeout_seconds);

  while (std::chrono::steady_clock::now() - start_time < timeout_duration) {
    if (is_port_available(port)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return false;
}

bool TestPortManager::wait_for_server_ready(int port, int timeout_seconds) const {
  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::seconds(timeout_seconds);

  while (std::chrono::steady_clock::now() - start_time < timeout_duration) {
    if (!is_port_available(port)) {  // Port is in use = server is ready
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return false;
}

void TestPortManager::cleanup_expired_allocations(int /* max_age_minutes */) {
  std::lock_guard<std::mutex> lock(allocation_mutex_);
  cleanup_stale_allocations_unsafe();
}

void TestPortManager::cleanup_all_allocations() {
  std::lock_guard<std::mutex> lock(allocation_mutex_);
  allocated_ports_.clear();
}

std::vector<int> TestPortManager::get_allocated_ports() const {
  std::lock_guard<std::mutex> lock(allocation_mutex_);

  std::vector<int> ports;
  ports.reserve(allocated_ports_.size());

  for (const auto& pair : allocated_ports_) {
    ports.push_back(pair.first);
  }

  return ports;
}

size_t TestPortManager::get_allocation_count() const {
  std::lock_guard<std::mutex> lock(allocation_mutex_);
  return allocated_ports_.size();
}

void TestPortManager::set_port_range(const PortRange& range) {
  std::lock_guard<std::mutex> lock(allocation_mutex_);
  port_range_ = range;
  next_port_hint_.store(range.start_port);
}

TestPortManager::PortRange TestPortManager::get_port_range() const {
  std::lock_guard<std::mutex> lock(allocation_mutex_);
  return port_range_;
}

void TestPortManager::print_allocation_status() const {
  std::cout << get_allocation_report() << std::endl;
}

std::string TestPortManager::get_allocation_report() const {
  std::lock_guard<std::mutex> lock(allocation_mutex_);

  std::ostringstream oss;
  oss << "TestPortManager Status:\n";
  oss << "  Port Range: " << port_range_.start_port << "-" << port_range_.end_port << "\n";
  oss << "  Allocated Ports: " << allocated_ports_.size() << "\n";

  return oss.str();
}

bool TestPortManager::is_port_in_use_system(int port) const {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return true;  // Assume in use if we can't create socket
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port));
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  int result = bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
  close(sock);

  return result != 0;  // Port is in use if bind fails
}

void TestPortManager::cleanup_stale_allocations_unsafe() {
  auto now = std::chrono::system_clock::now();
  auto max_age = std::chrono::minutes(30);  // 30 minute timeout

  auto it = allocated_ports_.begin();
  while (it != allocated_ports_.end()) {
    if (now - it->second.allocated_at > max_age) {
      // Check if port is actually still in use
      if (is_port_available(it->first)) {
        // Port is not in use, safe to remove allocation
        it = allocated_ports_.erase(it);
      } else {
        ++it;
      }
    } else {
      ++it;
    }
  }
}

// ScopedPortAllocation implementation
ScopedPortAllocation::ScopedPortAllocation(const std::string& test_name) {
  allocation_ = TestPortManager::instance().allocate_port(test_name);
}

ScopedPortAllocation::ScopedPortAllocation(int start_port, int end_port,
                                           const std::string& test_name) {
  allocation_ = TestPortManager::instance().allocate_port_in_range(start_port, end_port, test_name);
}

ScopedPortAllocation::~ScopedPortAllocation() {
  if (allocation_.success && !released_) {
    TestPortManager::instance().release_port(allocation_.port);
  }
}

ScopedPortAllocation::ScopedPortAllocation(ScopedPortAllocation&& other) noexcept
    : allocation_(std::move(other.allocation_)), released_(other.released_) {
  other.released_ = true;
}

ScopedPortAllocation& ScopedPortAllocation::operator=(ScopedPortAllocation&& other) noexcept {
  if (this != &other) {
    if (allocation_.success && !released_) {
      TestPortManager::instance().release_port(allocation_.port);
    }

    allocation_ = std::move(other.allocation_);
    released_ = other.released_;
    other.released_ = true;
  }
  return *this;
}

bool ScopedPortAllocation::wait_for_server_ready(int timeout_seconds) const {
  if (!allocation_.success) {
    return false;
  }

  return TestPortManager::instance().wait_for_server_ready(allocation_.port, timeout_seconds);
}

std::string ScopedPortAllocation::base_url() const {
  if (!allocation_.success) {
    return "";
  }

  return "http://localhost:" + std::to_string(allocation_.port);
}

// Simplified TestResourceManager implementation
TestResourceManager& TestResourceManager::instance() {
  static TestResourceManager instance;
  return instance;
}

TestResourceManager::~TestResourceManager() { cleanup_all_resources(); }

void TestResourceManager::register_resource(
    ResourceType /* type */, const std::string& /* identifier */,
    const std::string& /* test_name */, const std::map<std::string, std::string>& /* metadata */) {
  // Simplified implementation - just track count
  std::lock_guard<std::mutex> lock(resource_mutex_);
  resource_count_++;
}

void TestResourceManager::unregister_resource(ResourceType /* type */,
                                              const std::string& /* identifier */) {
  std::lock_guard<std::mutex> lock(resource_mutex_);
  if (resource_count_ > 0) {
    resource_count_--;
  }
}

void TestResourceManager::unregister_all_resources_for_test(const std::string& /* test_name */) {
  // Simplified implementation
}

void TestResourceManager::cleanup_resources_for_test(const std::string& /* test_name */) {
  // Simplified implementation
}

void TestResourceManager::cleanup_expired_resources(int /* max_age_minutes */) {
  // Simplified implementation
}

void TestResourceManager::cleanup_all_resources() {
  std::lock_guard<std::mutex> lock(resource_mutex_);
  resource_count_ = 0;
}

std::vector<TestResourceManager::ResourceInfo> TestResourceManager::get_resources_for_test(
    const std::string& /* test_name */) const {
  return {};  // Simplified implementation
}

std::vector<TestResourceManager::ResourceInfo> TestResourceManager::get_resources_by_type(
    ResourceType /* type */) const {
  return {};  // Simplified implementation
}

size_t TestResourceManager::get_resource_count() const {
  std::lock_guard<std::mutex> lock(resource_mutex_);
  return resource_count_;
}

void TestResourceManager::print_resource_status() const {
  std::cout << get_resource_report() << std::endl;
}

std::string TestResourceManager::get_resource_report() const {
  std::lock_guard<std::mutex> lock(resource_mutex_);

  std::ostringstream oss;
  oss << "TestResourceManager Status:\n";
  oss << "  Total Resources: " << resource_count_ << "\n";

  return oss.str();
}

// Simplified TestEnvironmentIsolation implementation
std::unique_ptr<TestEnvironmentIsolation::IsolatedEnvironment>
TestEnvironmentIsolation::create_environment(const EnvironmentConfig& config) {
  return std::make_unique<IsolatedEnvironment>(config);
}

std::unique_ptr<TestEnvironmentIsolation::IsolatedEnvironment>
TestEnvironmentIsolation::create_environment(const std::string& test_name) {
  EnvironmentConfig config;
  config.test_name = test_name;
  return create_environment(config);
}

// IsolatedEnvironment implementation
TestEnvironmentIsolation::IsolatedEnvironment::IsolatedEnvironment(const EnvironmentConfig& config)
    : config_(config) {}

TestEnvironmentIsolation::IsolatedEnvironment::~IsolatedEnvironment() {
  if (!cleaned_up_) {
    cleanup();
  }
}

ScopedPortAllocation TestEnvironmentIsolation::IsolatedEnvironment::allocate_port() {
  if (config_.isolate_ports) {
    return ScopedPortAllocation(config_.port_range_start, config_.port_range_end,
                                config_.test_name);
  } else {
    return ScopedPortAllocation(config_.test_name);
  }
}

std::string TestEnvironmentIsolation::IsolatedEnvironment::create_temp_file(
    const std::string& /* content */) {
  // Simplified implementation
  return "/tmp/test_file_" + config_.test_name;
}

std::string TestEnvironmentIsolation::IsolatedEnvironment::create_temp_directory() {
  // Simplified implementation
  return "/tmp/test_dir_" + config_.test_name;
}

void TestEnvironmentIsolation::IsolatedEnvironment::cleanup() {
  if (cleaned_up_) {
    return;
  }

  // Cleanup all resources for this test
  TestPortManager::instance().release_all_ports_for_test(config_.test_name);

  cleaned_up_ = true;
}

}  // namespace TestUtils
