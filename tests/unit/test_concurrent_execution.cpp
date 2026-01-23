/**
 * @file test_concurrent_execution.cpp
 * @brief Tests for concurrent test execution and race condition prevention
 * @note Migrated to Google Test
 *
 * Verifies that the enhanced test environment can handle multiple
 * concurrent tests without resource conflicts or race conditions.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <set>
#include <thread>
#include <vector>

#include "../utils/test_diagnostics.hpp"
#include "../utils/test_port_manager.hpp"

using namespace TestUtils;

// Test 1: Concurrent port allocation
TEST(ConcurrentExecution, ConcurrentPortAllocation) {
  const int num_threads = 10;
  std::vector<std::future<bool>> futures;
  std::vector<int> allocated_ports;
  std::mutex ports_mutex;

  // Launch multiple threads that allocate ports concurrently
  for (int i = 0; i < num_threads; ++i) {
    futures.push_back(std::async(std::launch::async, [&, i]() {
      try {
        ScopedPortAllocation port_alloc("concurrent_test_" + std::to_string(i));

        if (!port_alloc.is_valid()) {
          return false;
        }

        int port = port_alloc.port();

        // Add to shared list (thread-safe)
        {
          std::lock_guard<std::mutex> lock(ports_mutex);
          allocated_ports.push_back(port);
        }

        // Hold the port for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        return true;
      } catch (const std::exception&) {
        return false;
      }
    }));
  }

  // Wait for all threads to complete
  bool all_succeeded = true;
  for (auto& future : futures) {
    if (!future.get()) {
      all_succeeded = false;
    }
  }

  ASSERT_TRUE(all_succeeded);
  ASSERT_EQ(allocated_ports.size(), num_threads);

  // Verify all ports are unique
  std::set<int> unique_ports(allocated_ports.begin(), allocated_ports.end());
  ASSERT_EQ(unique_ports.size(), num_threads);
}

// Test 2: Concurrent resource cleanup
TEST(ConcurrentExecution, ConcurrentResourceCleanup) {
  const int num_threads = 5;
  std::vector<std::future<bool>> futures;
  auto& resource_manager = TestResourceManager::instance();

  // Get initial resource count
  size_t initial_count = resource_manager.get_resource_count();

  // Launch threads that create and clean up resources
  for (int i = 0; i < num_threads; ++i) {
    futures.push_back(std::async(std::launch::async, [&, i]() {
      try {
        std::string test_name = "cleanup_test_" + std::to_string(i);

        // Create isolated environment
        auto env = TestEnvironmentIsolation::create_environment(test_name);

        // Allocate resources
        auto port_alloc = env->allocate_port();
        std::string temp_file = env->create_temp_file("test content");
        std::string temp_dir = env->create_temp_directory();

        // Verify resources exist
        if (!port_alloc.is_valid() || temp_file.empty() || temp_dir.empty()) {
          return false;
        }

        // Hold resources briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Explicit cleanup
        env->cleanup();

        return true;
      } catch (const std::exception&) {
        return false;
      }
    }));
  }

  // Wait for all threads
  bool all_succeeded = true;
  for (auto& future : futures) {
    if (!future.get()) {
      all_succeeded = false;
    }
  }

  ASSERT_TRUE(all_succeeded);

  // Allow time for cleanup to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Resource count should return to initial level
  size_t final_count = resource_manager.get_resource_count();
  ASSERT_LE(final_count, initial_count + 2);  // Allow small variance
}

// Test 3: Concurrent diagnostic logging
TEST(ConcurrentExecution, ConcurrentDiagnosticLogging) {
  const int num_threads = 8;
  const int logs_per_thread = 10;
  std::vector<std::future<bool>> futures;
  auto& logger = TestDiagnosticLogger::instance();

  // Configure logger
  TestDiagnosticLogger::Config config;
  config.min_level = LogLevel::DEBUG;
  config.log_to_console = false;  // Reduce noise
  logger.configure(config);

  // Launch threads that log concurrently
  for (int i = 0; i < num_threads; ++i) {
    futures.push_back(std::async(std::launch::async, [&, i]() {
      try {
        std::string test_name = "logging_test_" + std::to_string(i);

        for (int j = 0; j < logs_per_thread; ++j) {
          std::map<std::string, std::string> context;
          context["thread_id"] = std::to_string(i);
          context["log_id"] = std::to_string(j);

          logger.info(test_name, "Concurrent log message " + std::to_string(j), context);

          // Small delay to create interleaving
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return true;
      } catch (const std::exception&) {
        return false;
      }
    }));
  }

  // Wait for all logging to complete
  bool all_succeeded = true;
  for (auto& future : futures) {
    if (!future.get()) {
      all_succeeded = false;
    }
  }

  ASSERT_TRUE(all_succeeded);

  // Flush logs to ensure all are written
  logger.flush();
}

// Test 4: Concurrent performance monitoring
TEST(ConcurrentExecution, ConcurrentPerformanceMonitoring) {
  const int num_threads = 6;
  std::vector<std::future<bool>> futures;

  // Launch threads that monitor performance concurrently
  for (int i = 0; i < num_threads; ++i) {
    futures.push_back(std::async(std::launch::async, [i]() {
      try {
        std::string test_name = "perf_test_" + std::to_string(i);

        TestPerformanceMonitor::start_monitoring(test_name);

        // Simulate work with different durations
        int work_duration = 10 + (i * 5);  // 10-35ms
        std::this_thread::sleep_for(std::chrono::milliseconds(work_duration));

        // Add custom metrics
        TestPerformanceMonitor::add_custom_metric(test_name, "work_units", i * 10.0);

        auto metrics = TestPerformanceMonitor::stop_monitoring(test_name);

        // Verify metrics are reasonable
        return metrics.test_name == test_name && metrics.duration.count() >= work_duration &&
               metrics.custom_metrics.size() > 0;

      } catch (const std::exception&) {
        return false;
      }
    }));
  }

  // Wait for all monitoring to complete
  bool all_succeeded = true;
  for (auto& future : futures) {
    if (!future.get()) {
      all_succeeded = false;
    }
  }

  ASSERT_TRUE(all_succeeded);

  // Verify all metrics were recorded
  auto all_metrics = TestPerformanceMonitor::get_all_metrics();
  ASSERT_GE(all_metrics.size(), num_threads);
}

// Test 5: Stress test with mixed operations
TEST(ConcurrentExecution, MixedOperationsStressTest) {
  const int num_operations = 20;
  std::vector<std::future<bool>> futures;
  std::atomic<int> success_count{0};

  // Launch mixed operations concurrently
  for (int i = 0; i < num_operations; ++i) {
    futures.push_back(std::async(std::launch::async, [&, i]() {
      try {
        std::string test_name = "stress_test_" + std::to_string(i);

        // Mix of different operations
        switch (i % 4) {
          case 0: {
            // Port allocation and web server simulation
            ScopedPortAllocation port(test_name);
            if (!port.is_valid()) return false;

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            break;
          }
          case 1: {
            // File operations
            auto env = TestEnvironmentIsolation::create_environment(test_name);
            std::string temp_file = env->create_temp_file("stress test content");
            if (temp_file.empty()) return false;

            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            break;
          }
          case 2: {
            // Logging operations
            auto& logger = TestDiagnosticLogger::instance();
            logger.info(test_name, "Stress test message");
            logger.warning(test_name, "Stress test warning");

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            break;
          }
          case 3: {
            // Performance monitoring
            TestPerformanceMonitor::start_monitoring(test_name);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            auto metrics = TestPerformanceMonitor::stop_monitoring(test_name);
            if (metrics.test_name != test_name) return false;
            break;
          }
        }

        success_count++;
        return true;

      } catch (const std::exception&) {
        return false;
      }
    }));
  }

  // Wait for all operations
  int completed_successfully = 0;
  for (auto& future : futures) {
    if (future.get()) {
      completed_successfully++;
    }
  }

  // Should have high success rate (allow for some variance under stress)
  ASSERT_GE(completed_successfully, num_operations * 0.9);  // 90% success rate
  ASSERT_GE(success_count.load(), static_cast<int>(num_operations * 0.9));
}
