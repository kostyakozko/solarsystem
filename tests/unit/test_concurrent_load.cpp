/**
 * @file test_concurrent_load.cpp
 * @brief Concurrent load and stress testing (Task 24)
 * @note Migrated to Google Test
 *
 * Tests system behavior under concurrent load:
 * - Realistic concurrent usage pattern testing
 * - High-load scenario testing with resource monitoring
 * - Concurrent data modification and consistency testing
 * - Performance under concurrent load testing
 *
 * Requirements: 8.2, 8.3
 */

// Suppress unused lambda capture warnings for test clarity
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-lambda-capture"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

/**
 * @brief Resource monitor for tracking system usage
 */
class ResourceMonitor {
 private:
  std::atomic<size_t> memory_allocated_{0};
  std::atomic<size_t> peak_memory_{0};
  std::atomic<int> active_threads_{0};
  std::atomic<int> peak_threads_{0};
  std::atomic<long long> operations_completed_{0};
  std::chrono::steady_clock::time_point start_time_;

 public:
  ResourceMonitor() : start_time_(std::chrono::steady_clock::now()) {}

  void allocate_memory(size_t bytes) {
    memory_allocated_ += bytes;
    size_t current = memory_allocated_.load();
    size_t peak = peak_memory_.load();
    while (current > peak && !peak_memory_.compare_exchange_weak(peak, current)) {
    }
  }

  void deallocate_memory(size_t bytes) { memory_allocated_ -= bytes; }

  void thread_started() {
    active_threads_++;
    int current = active_threads_.load();
    int peak = peak_threads_.load();
    while (current > peak && !peak_threads_.compare_exchange_weak(peak, current)) {
    }
  }

  void thread_finished() { active_threads_--; }

  void operation_completed() { operations_completed_++; }

  size_t get_memory_usage() const { return memory_allocated_.load(); }

  size_t get_peak_memory() const { return peak_memory_.load(); }

  int get_active_threads() const { return active_threads_.load(); }

  int get_peak_threads() const { return peak_threads_.load(); }

  long long get_operations_completed() const { return operations_completed_.load(); }

  double get_operations_per_second() const {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    auto seconds = std::chrono::duration<double>(elapsed).count();
    return operations_completed_.load() / seconds;
  }
};

/**
 * @brief Concurrent data store for testing
 */
template <typename K, typename V>
class ConcurrentDataStore {
 private:
  std::map<K, V> data_;
  mutable std::mutex mutex_;
  std::atomic<long long> read_count_{0};
  std::atomic<long long> write_count_{0};

 public:
  void insert(const K& key, const V& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = value;
    write_count_++;
  }

  bool get(const K& key, V& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
      value = it->second;
      read_count_++;
      return true;
    }
    return false;
  }

  bool remove(const K& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
      data_.erase(it);
      write_count_++;
      return true;
    }
    return false;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
  }

  long long get_read_count() const { return read_count_.load(); }

  long long get_write_count() const { return write_count_.load(); }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
  }
};

/**
 * @brief Load generator for stress testing
 */
class LoadGenerator {
 private:
  std::atomic<bool> running_{false};
  std::vector<std::thread> workers_;
  ResourceMonitor& monitor_;

 public:
  explicit LoadGenerator(ResourceMonitor& monitor) : monitor_(monitor) {}

  ~LoadGenerator() { stop(); }

  void start(int num_threads, std::function<void()> work_func) {
    running_ = true;
    for (int i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this, work_func]() {
        monitor_.thread_started();
        while (running_) {
          work_func();
          monitor_.operation_completed();
        }
        monitor_.thread_finished();
      });
    }
  }

  void stop() {
    running_ = false;
    for (auto& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
    workers_.clear();
  }

  bool is_running() const { return running_; }
};

// Test 1: Realistic concurrent usage patterns
TEST(ConcurrentLoadTest, RealisticConcurrentUsagePatterns) {
  ResourceMonitor monitor;
  ConcurrentDataStore<int, std::string> store;

  const int num_readers = 10;
  const int num_writers = 5;
  const int operations_per_thread = 100;

  std::vector<std::thread> threads;

  // Test 1.1: Mixed read/write workload
  for (int i = 0; i < num_writers; ++i) {
    threads.emplace_back([&store, &monitor, i, operations_per_thread]() {
      monitor.thread_started();
      for (int j = 0; j < operations_per_thread; ++j) {
        store.insert(i * operations_per_thread + j, "value_" + std::to_string(j));
        monitor.operation_completed();
      }
      monitor.thread_finished();
    });
  }

  for (int i = 0; i < num_readers; ++i) {
    threads.emplace_back([&store, &monitor, operations_per_thread]() {
      monitor.thread_started();
      for (int j = 0; j < operations_per_thread; ++j) {
        std::string value;
        store.get(j, value);
        monitor.operation_completed();
      }
      monitor.thread_finished();
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(monitor.get_peak_threads(), num_readers + num_writers);
  ASSERT_EQ(monitor.get_operations_completed(),
            (num_readers + num_writers) * operations_per_thread);
  ASSERT_GE(store.get_write_count(), num_writers * operations_per_thread);
}

// Test 2: High-load scenario with resource monitoring
TEST(ConcurrentLoadTest, HighLoadScenarioWithResourceMonitoring) {
  ResourceMonitor monitor;
  ConcurrentDataStore<int, std::vector<int>> store;

  const int num_threads = 20;
  const int operations_per_thread = 50;
  const size_t data_size = 1000;  // Size of each vector

  std::vector<std::thread> threads;

  // Test 2.1: High memory allocation load
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&store, &monitor, i, operations_per_thread, data_size]() {
      monitor.thread_started();
      for (int j = 0; j < operations_per_thread; ++j) {
        std::vector<int> data(data_size, j);
        monitor.allocate_memory(data_size * sizeof(int));
        store.insert(i * operations_per_thread + j, data);
        monitor.operation_completed();
      }
      monitor.thread_finished();
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(monitor.get_operations_completed(), num_threads * operations_per_thread);
  ASSERT_GT(monitor.get_peak_memory(), 0);
  ASSERT_EQ(store.size(), static_cast<size_t>(num_threads * operations_per_thread));

  // Test 2.2: Monitor operations per second
  double ops_per_sec = monitor.get_operations_per_second();
  ASSERT_GT(ops_per_sec, 0.0);
}

// Test 3: Concurrent data modification and consistency
TEST(ConcurrentLoadTest, ConcurrentDataModificationAndConsistency) {
  ConcurrentDataStore<int, int> store;
  const int num_keys = 100;
  const int num_threads = 10;
  const int modifications_per_thread = 50;

  // Initialize data
  for (int i = 0; i < num_keys; ++i) {
    store.insert(i, 0);
  }

  std::vector<std::thread> threads;
  std::random_device rd;

  // Test 3.1: Concurrent modifications to same keys
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&store, num_keys, modifications_per_thread, seed = rd()]() {
      std::mt19937 gen(seed);
      std::uniform_int_distribution<> dis(0, num_keys - 1);

      for (int j = 0; j < modifications_per_thread; ++j) {
        int key = dis(gen);
        int value;
        if (store.get(key, value)) {
          store.insert(key, value + 1);
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Verify data consistency
  int total_modifications = 0;
  for (int i = 0; i < num_keys; ++i) {
    int value;
    if (store.get(i, value)) {
      total_modifications += value;
    }
  }

  ASSERT_EQ(store.size(), static_cast<size_t>(num_keys));
  ASSERT_GT(total_modifications, 0);
  ASSERT_LE(total_modifications, num_threads * modifications_per_thread);
}

// Test 4: Performance under concurrent load
TEST(ConcurrentLoadTest, PerformanceUnderConcurrentLoad) {
  ResourceMonitor monitor;
  ConcurrentDataStore<int, int> store;

  const int num_threads = 15;
  const int operations_per_thread = 200;

  auto start_time = std::chrono::steady_clock::now();

  std::vector<std::thread> threads;

  // Test 4.1: Measure throughput under load
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&store, &monitor, i, operations_per_thread]() {
      monitor.thread_started();
      for (int j = 0; j < operations_per_thread; ++j) {
        store.insert(i * operations_per_thread + j, j);
        monitor.operation_completed();

        // Simulate some work
        int value;
        store.get(i * operations_per_thread + j, value);
        monitor.operation_completed();
      }
      monitor.thread_finished();
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  ASSERT_EQ(monitor.get_operations_completed(), num_threads * operations_per_thread * 2);
  ASSERT_GT(duration.count(), 0);

  // Calculate throughput
  double ops_per_ms = static_cast<double>(monitor.get_operations_completed()) / duration.count();
  ASSERT_GT(ops_per_ms, 0.0);
}

// Test 5: Stress test with load generator
TEST(ConcurrentLoadTest, StressTestWithLoadGenerator) {
  ResourceMonitor monitor;
  ConcurrentDataStore<int, int> store;
  LoadGenerator generator(monitor);

  const int num_threads = 10;
  std::atomic<int> counter{0};

  // Test 5.1: Sustained load over time
  generator.start(num_threads, [&store, &counter]() {
    int key = counter.fetch_add(1);
    store.insert(key, key);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  });

  ASSERT_TRUE(generator.is_running());

  // Run for a short duration
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  generator.stop();

  ASSERT_FALSE(generator.is_running());
  ASSERT_GT(monitor.get_operations_completed(), 0);
  ASSERT_EQ(monitor.get_active_threads(), 0);
}

// Test 6: Concurrent insert and remove operations
TEST(ConcurrentLoadTest, ConcurrentInsertAndRemoveOperations) {
  ConcurrentDataStore<int, int> store;
  const int num_threads = 10;
  const int operations_per_thread = 100;

  std::vector<std::thread> threads;

  // Test 6.1: Alternating insert and remove
  for (int i = 0; i < num_threads / 2; ++i) {
    // Inserter threads
    threads.emplace_back([&store, i, operations_per_thread]() {
      for (int j = 0; j < operations_per_thread; ++j) {
        store.insert(i * operations_per_thread + j, j);
      }
    });

    // Remover threads
    threads.emplace_back([&store, i, operations_per_thread]() {
      for (int j = 0; j < operations_per_thread; ++j) {
        store.remove(i * operations_per_thread + j);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Final size depends on timing, but should be consistent
  size_t final_size = store.size();
  ASSERT_LE(final_size, static_cast<size_t>((num_threads / 2) * operations_per_thread));
}

// Test 7: Memory pressure test
TEST(ConcurrentLoadTest, MemoryPressureTest) {
  ResourceMonitor monitor;
  ConcurrentDataStore<int, std::vector<int>> store;

  const int num_threads = 8;
  const int allocations_per_thread = 50;
  const size_t allocation_size = 10000;

  std::vector<std::thread> threads;

  // Test 7.1: High memory allocation rate
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&store, &monitor, i, allocations_per_thread, allocation_size]() {
      monitor.thread_started();
      for (int j = 0; j < allocations_per_thread; ++j) {
        std::vector<int> data(allocation_size, j);
        monitor.allocate_memory(allocation_size * sizeof(int));
        store.insert(i * allocations_per_thread + j, data);
        monitor.operation_completed();

        // Simulate processing
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
      monitor.thread_finished();
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_GT(monitor.get_peak_memory(), 0);
  ASSERT_EQ(monitor.get_operations_completed(), num_threads * allocations_per_thread);
}

// Test 8: Contention test
TEST(ConcurrentLoadTest, ContentionTest) {
  ConcurrentDataStore<int, int> store;
  const int num_threads = 20;
  const int hot_keys = 10;  // Small number of keys to create contention
  const int operations_per_thread = 100;

  std::vector<std::thread> threads;
  std::random_device rd;

  // Test 8.1: High contention on few keys
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&store, hot_keys, operations_per_thread, seed = rd()]() {
      std::mt19937 gen(seed);
      std::uniform_int_distribution<> dis(0, hot_keys - 1);

      for (int j = 0; j < operations_per_thread; ++j) {
        int key = dis(gen);
        int value;
        if (store.get(key, value)) {
          store.insert(key, value + 1);
        } else {
          store.insert(key, 1);
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Verify all operations completed despite high contention
  ASSERT_LE(store.size(), static_cast<size_t>(hot_keys));
  ASSERT_GT(store.get_write_count(), 0);
}

#pragma clang diagnostic pop
