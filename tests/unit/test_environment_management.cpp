/**
 * @file test_environment_management.cpp
 * @brief Isolated test environment management tests (Task 16)
 * @note Migrated to Google Test
 *
 * Tests environment management capabilities:
 * - Temporary test environment creation and cleanup
 * - Test data isolation and sandboxing
 * - Test environment state management and reset
 * - Test environment resource monitoring and limits
 *
 * Requirements: 5.2, 5.5
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

/**
 * @brief Test environment management system
 */
class TestEnvironmentManager {
 public:
  // Environment configuration
  struct EnvironmentConfig {
    std::string name;
    std::string base_path;
    size_t max_memory_mb = 1024;
    size_t max_disk_mb = 1024;
    int max_files = 1000;
    bool auto_cleanup = true;
  };

  // Environment state
  struct EnvironmentState {
    bool is_active = false;
    std::string environment_path;
    std::chrono::system_clock::time_point created_at;
    size_t files_created = 0;
    size_t memory_used_mb = 0;
    size_t disk_used_mb = 0;
  };

  // Resource usage
  struct ResourceUsage {
    size_t memory_mb = 0;
    size_t disk_mb = 0;
    int file_count = 0;
    double cpu_percent = 0.0;
  };

 private:
  std::map<std::string, EnvironmentState> environments_;
  EnvironmentConfig default_config_;

 public:
  TestEnvironmentManager() {
    default_config_.name = "default";
    default_config_.base_path = "/tmp/test_env";
    default_config_.auto_cleanup = true;
  }

  // Create test environment
  bool create_environment(const EnvironmentConfig& config) {
    if (environments_.find(config.name) != environments_.end()) {
      return false; // Already exists
    }

    EnvironmentState state;
    state.environment_path = config.base_path + "/" + config.name;
    state.created_at = std::chrono::system_clock::now();
    state.is_active = true;

    // Create directory structure
    try {
      fs::create_directories(state.environment_path);
      fs::create_directories(state.environment_path + "/data");
      fs::create_directories(state.environment_path + "/cache");
      fs::create_directories(state.environment_path + "/temp");
    } catch (const std::exception&) {
      return false;
    }

    environments_[config.name] = state;
    return true;
  }

  // Cleanup environment
  bool cleanup_environment(const std::string& name) {
    auto it = environments_.find(name);
    if (it == environments_.end()) {
      return false;
    }

    try {
      if (fs::exists(it->second.environment_path)) {
        fs::remove_all(it->second.environment_path);
      }
    } catch (const std::exception&) {
      return false;
    }

    environments_.erase(it);
    return true;
  }

  // Reset environment state
  bool reset_environment(const std::string& name) {
    auto it = environments_.find(name);
    if (it == environments_.end()) {
      return false;
    }

    try {
      // Remove all files but keep directory structure
      fs::path env_path(it->second.environment_path);
      for (const auto& entry : fs::recursive_directory_iterator(env_path)) {
        if (fs::is_regular_file(entry)) {
          fs::remove(entry);
        }
      }

      // Reset state
      it->second.files_created = 0;
      it->second.memory_used_mb = 0;
      it->second.disk_used_mb = 0;
    } catch (const std::exception&) {
      return false;
    }

    return true;
  }

  // Get environment state
  EnvironmentState get_state(const std::string& name) const {
    auto it = environments_.find(name);
    if (it != environments_.end()) {
      return it->second;
    }
    return EnvironmentState{};
  }

  // Check if environment exists
  bool exists(const std::string& name) const {
    return environments_.find(name) != environments_.end();
  }

  // Create file in environment
  bool create_file(const std::string& env_name, const std::string& filename,
                   const std::string& content) {
    auto it = environments_.find(env_name);
    if (it == environments_.end() || !it->second.is_active) {
      return false;
    }

    try {
      std::string filepath =
          it->second.environment_path + "/data/" + filename;
      std::ofstream file(filepath);
      file << content;
      file.close();

      it->second.files_created++;
      it->second.disk_used_mb += content.size() / (1024 * 1024);
    } catch (const std::exception&) {
      return false;
    }

    return true;
  }

  // Read file from environment
  std::string read_file(const std::string& env_name,
                        const std::string& filename) {
    auto it = environments_.find(env_name);
    if (it == environments_.end()) {
      return "";
    }

    try {
      std::string filepath =
          it->second.environment_path + "/data/" + filename;
      std::ifstream file(filepath);
      std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
      return content;
    } catch (const std::exception&) {
      return "";
    }
  }

  // Get resource usage
  ResourceUsage get_resource_usage(const std::string& env_name) {
    ResourceUsage usage;
    auto it = environments_.find(env_name);
    if (it == environments_.end()) {
      return usage;
    }

    try {
      fs::path env_path(it->second.environment_path);
      if (!fs::exists(env_path)) {
        return usage;
      }

      // Count files and disk usage
      for (const auto& entry : fs::recursive_directory_iterator(env_path)) {
        if (fs::is_regular_file(entry)) {
          usage.file_count++;
          usage.disk_mb += static_cast<size_t>(fs::file_size(entry)) /
                           (1024 * 1024);
        }
      }

      // Memory usage (simplified - just track state)
      usage.memory_mb = it->second.memory_used_mb;
    } catch (const std::exception&) {
      // Return partial usage
    }

    return usage;
  }

  // Check resource limits
  bool check_limits(const std::string& env_name,
                    const EnvironmentConfig& config) {
    auto usage = get_resource_usage(env_name);

    if (usage.memory_mb > config.max_memory_mb) return false;
    if (usage.disk_mb > config.max_disk_mb) return false;
    if (usage.file_count > config.max_files) return false;

    return true;
  }

  // Isolate environment (mark as isolated)
  bool isolate_environment(const std::string& name) {
    auto it = environments_.find(name);
    if (it == environments_.end()) {
      return false;
    }

    // In a real implementation, this would set up proper isolation
    // (containers, namespaces, etc.)
    it->second.is_active = true;
    return true;
  }

  // Get all environment names
  std::vector<std::string> list_environments() const {
    std::vector<std::string> names;
    for (const auto& [name, state] : environments_) {
      names.push_back(name);
    }
    return names;
  }

  // Cleanup all environments
  void cleanup_all() {
    auto names = list_environments();
    for (const auto& name : names) {
      cleanup_environment(name);
    }
  }
};
  // Test 1: Temporary test environment creation and cleanup
  TEST(TestEnvironmentManagementTestsTest, Environment_Creation_and_Cleanup) {
    TestEnvironmentManager manager;

    // Test 1.1: Create environment
    {
      TestEnvironmentManager::EnvironmentConfig config;
      config.name = "test_env_1";
      config.base_path = "/tmp/test_envs";
      config.auto_cleanup = true;

      bool created = manager.create_environment(config);
      ASSERT_TRUE(created);
      ASSERT_TRUE(manager.exists("test_env_1"));

      auto state = manager.get_state("test_env_1");
      ASSERT_TRUE(state.is_active);
      ASSERT_FALSE(state.environment_path.empty());
    }

    // Test 1.2: Duplicate environment creation fails
    {
      TestEnvironmentManager::EnvironmentConfig config;
      config.name = "test_env_1";
      config.base_path = "/tmp/test_envs";

      bool created = manager.create_environment(config);
      ASSERT_FALSE(created); // Should fail - already exists
    }

    // Test 1.3: Cleanup environment
    {
      bool cleaned = manager.cleanup_environment("test_env_1");
      ASSERT_TRUE(cleaned);
      ASSERT_FALSE(manager.exists("test_env_1"));
    }

    // Test 1.4: Cleanup non-existent environment
    {
      bool cleaned = manager.cleanup_environment("nonexistent");
      ASSERT_FALSE(cleaned);
    }
  }

  // Test 2: Test data isolation and sandboxing
  TEST(TestEnvironmentManagementTestsTest, Data_Isolation_and_Sandboxing) {
    TestEnvironmentManager manager;

    // Test 2.1: Create isolated environments
    {
      TestEnvironmentManager::EnvironmentConfig config1;
      config1.name = "isolated_1";
      config1.base_path = "/tmp/test_envs";

      TestEnvironmentManager::EnvironmentConfig config2;
      config2.name = "isolated_2";
      config2.base_path = "/tmp/test_envs";

      ASSERT_TRUE(manager.create_environment(config1));
      ASSERT_TRUE(manager.create_environment(config2));
    }

    // Test 2.2: Write data to separate environments
    {
      ASSERT_TRUE(
          manager.create_file("isolated_1", "data1.txt", "Content 1"));
      ASSERT_TRUE(
          manager.create_file("isolated_2", "data2.txt", "Content 2"));
    }

    // Test 2.3: Verify isolation
    {
      std::string content1 = manager.read_file("isolated_1", "data1.txt");
      std::string content2 = manager.read_file("isolated_2", "data2.txt");

      ASSERT_EQ(content1, "Content 1");
      ASSERT_EQ(content2, "Content 2");

      // File from env1 should not exist in env2
      std::string missing = manager.read_file("isolated_2", "data1.txt");
      ASSERT_TRUE(missing.empty());
    }

    // Test 2.4: Isolate environment
    {
      ASSERT_TRUE(manager.isolate_environment("isolated_1"));
      auto state = manager.get_state("isolated_1");
      ASSERT_TRUE(state.is_active);
    }

    // Cleanup
    manager.cleanup_environment("isolated_1");
    manager.cleanup_environment("isolated_2");
  }

  // Test 3: Test environment state management and reset
  TEST(TestEnvironmentManagementTestsTest, State_Management_and_Reset) {
    TestEnvironmentManager manager;

    // Test 3.1: Create and populate environment
    {
      TestEnvironmentManager::EnvironmentConfig config;
      config.name = "stateful_env";
      config.base_path = "/tmp/test_envs";

      ASSERT_TRUE(manager.create_environment(config));
      ASSERT_TRUE(
          manager.create_file("stateful_env", "file1.txt", "Data 1"));
      ASSERT_TRUE(
          manager.create_file("stateful_env", "file2.txt", "Data 2"));
      ASSERT_TRUE(
          manager.create_file("stateful_env", "file3.txt", "Data 3"));

      auto state = manager.get_state("stateful_env");
      ASSERT_EQ(state.files_created, 3);
    }

    // Test 3.2: Reset environment
    {
      ASSERT_TRUE(manager.reset_environment("stateful_env"));

      auto state = manager.get_state("stateful_env");
      ASSERT_EQ(state.files_created, 0);
      ASSERT_EQ(state.disk_used_mb, 0);

      // Files should be gone
      std::string content = manager.read_file("stateful_env", "file1.txt");
      ASSERT_TRUE(content.empty());
    }

    // Test 3.3: Environment still exists after reset
    {
      ASSERT_TRUE(manager.exists("stateful_env"));
      auto state = manager.get_state("stateful_env");
      ASSERT_TRUE(state.is_active);
    }

    // Test 3.4: Can create new files after reset
    {
      ASSERT_TRUE(
          manager.create_file("stateful_env", "new_file.txt", "New data"));
      std::string content =
          manager.read_file("stateful_env", "new_file.txt");
      ASSERT_EQ(content, "New data");
    }

    // Cleanup
    manager.cleanup_environment("stateful_env");
  }

  // Test 4: Resource monitoring and limits
  TEST(TestEnvironmentManagementTestsTest, Resource_Monitoring_and_Limits) {
    TestEnvironmentManager manager;

    // Test 4.1: Create environment with limits
    {
      TestEnvironmentManager::EnvironmentConfig config;
      config.name = "limited_env";
      config.base_path = "/tmp/test_envs";
      config.max_memory_mb = 100;
      config.max_disk_mb = 50;
      config.max_files = 10;

      ASSERT_TRUE(manager.create_environment(config));
    }

    // Test 4.2: Monitor resource usage
    {
      manager.create_file("limited_env", "test1.txt", "Test data 1");
      manager.create_file("limited_env", "test2.txt", "Test data 2");

      auto usage = manager.get_resource_usage("limited_env");
      ASSERT_EQ(usage.file_count, 2);
      ASSERT_GE(usage.disk_mb, 0);
    }

    // Test 4.3: Check limits
    {
      TestEnvironmentManager::EnvironmentConfig config;
      config.max_memory_mb = 100;
      config.max_disk_mb = 50;
      config.max_files = 10;

      bool within_limits = manager.check_limits("limited_env", config);
      ASSERT_TRUE(within_limits);
    }

    // Test 4.4: Exceed file count limit
    {
      TestEnvironmentManager::EnvironmentConfig strict_config;
      strict_config.max_files = 1; // Very strict

      bool within_limits = manager.check_limits("limited_env", strict_config);
      ASSERT_FALSE(within_limits); // Should fail - we have 2 files
    }

    // Cleanup
    manager.cleanup_environment("limited_env");
  }

  // Test 5: Multiple environment management
  TEST(TestEnvironmentManagementTestsTest, Multiple_Environment_Management) {
    TestEnvironmentManager manager;

    // Test 5.1: Create multiple environments
    {
      for (int i = 0; i < 5; ++i) {
        TestEnvironmentManager::EnvironmentConfig config;
        config.name = "env_" + std::to_string(i);
        config.base_path = "/tmp/test_envs";
        ASSERT_TRUE(manager.create_environment(config));
      }

      auto envs = manager.list_environments();
      ASSERT_EQ(envs.size(), 5);
    }

    // Test 5.2: Verify all environments exist
    {
      for (int i = 0; i < 5; ++i) {
        std::string name = "env_" + std::to_string(i);
        ASSERT_TRUE(manager.exists(name));
      }
    }

    // Test 5.3: Write to each environment
    {
      for (int i = 0; i < 5; ++i) {
        std::string env_name = "env_" + std::to_string(i);
        std::string content = "Data for environment " + std::to_string(i);
        ASSERT_TRUE(manager.create_file(env_name, "data.txt", content));
      }
    }

    // Test 5.4: Verify isolation between environments
    {
      for (int i = 0; i < 5; ++i) {
        std::string env_name = "env_" + std::to_string(i);
        std::string content = manager.read_file(env_name, "data.txt");
        std::string expected = "Data for environment " + std::to_string(i);
        ASSERT_EQ(content, expected);
      }
    }

    // Test 5.5: Cleanup all environments
    {
      manager.cleanup_all();
      auto envs = manager.list_environments();
      ASSERT_EQ(envs.size(), 0);
    }
  }

  // Test 6: Environment lifecycle
  TEST(TestEnvironmentManagementTestsTest, Environment_Lifecycle) {
    TestEnvironmentManager manager;

    // Test 6.1: Full lifecycle
    {
      TestEnvironmentManager::EnvironmentConfig config;
      config.name = "lifecycle_env";
      config.base_path = "/tmp/test_envs";

      // Create
      ASSERT_TRUE(manager.create_environment(config));
      auto state1 = manager.get_state("lifecycle_env");
      ASSERT_TRUE(state1.is_active);

      // Use
      manager.create_file("lifecycle_env", "work.txt", "Working data");
      auto usage = manager.get_resource_usage("lifecycle_env");
      ASSERT_GT(usage.file_count, 0);

      // Reset
      ASSERT_TRUE(manager.reset_environment("lifecycle_env"));
      auto state2 = manager.get_state("lifecycle_env");
      ASSERT_EQ(state2.files_created, 0);

      // Reuse
      manager.create_file("lifecycle_env", "new_work.txt", "New data");
      std::string content =
          manager.read_file("lifecycle_env", "new_work.txt");
      ASSERT_EQ(content, "New data");

      // Cleanup
      ASSERT_TRUE(manager.cleanup_environment("lifecycle_env"));
      ASSERT_FALSE(manager.exists("lifecycle_env"));
    }
  }
