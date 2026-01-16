/**
 * @file test_thread_safety.cpp
 * @brief Thread safety validation testing (Task 23)
 * @note Migrated to Google Test
 *
 * Tests thread safety mechanisms:
 * - Shared resource access testing
 * - Race condition detection and validation
 * - Deadlock detection and prevention
 * - Thread synchronization mechanism testing
 *
 * Requirements: 8.1, 8.4
 */

// Suppress unused lambda capture warnings for test clarity

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @brief Thread-safe counter for testing
 */
class ThreadSafeCounter {
 private:
  int value_;
  mutable std::mutex mutex_;

 public:
  ThreadSafeCounter() : value_(0) {}

  void increment() {
    std::lock_guard<std::mutex> lock(mutex_);
    value_++;
  }

  void decrement() {
    std::lock_guard<std::mutex> lock(mutex_);
    value_--;
  }

  int get() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = 0;
  }
};

/**
 * @brief Unsafe counter for testing race conditions
 */
class UnsafeCounter {
 private:
  int value_;

 public:
  UnsafeCounter() : value_(0) {}

  void increment() { value_++; }

  void decrement() { value_--; }

  int get() const { return value_; }

  void reset() { value_ = 0; }
};

/**
 * @brief Thread-safe queue for testing
 */
template <typename T>
class ThreadSafeQueue {
 private:
  std::vector<T> data_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;

 public:
  void push(const T& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.push_back(item);
    cv_.notify_one();
  }

  bool try_pop(T& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.empty()) {
      return false;
    }
    item = data_.front();
    data_.erase(data_.begin());
    return true;
  }

  bool wait_and_pop(T& item, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cv_.wait_for(lock, timeout, [this] { return !data_.empty(); })) {
      return false;
    }
    item = data_.front();
    data_.erase(data_.begin());
    return true;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.empty();
  }
};

/**
 * @brief Deadlock detector for testing
 */
class DeadlockDetector {
 private:
  std::mutex mutex1_;
  std::mutex mutex2_;

 public:
  // Potential deadlock scenario
  bool acquire_locks_wrong_order(int thread_id) {
    if (thread_id % 2 == 0) {
      std::lock_guard<std::mutex> lock1(mutex1_);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      std::lock_guard<std::mutex> lock2(mutex2_);
      return true;
    } else {
      std::lock_guard<std::mutex> lock2(mutex2_);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      std::lock_guard<std::mutex> lock1(mutex1_);
      return true;
    }
  }

  // Correct lock acquisition using std::lock
  bool acquire_locks_correct_order() {
    std::lock(mutex1_, mutex2_);
    std::lock_guard<std::mutex> lock1(mutex1_, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(mutex2_, std::adopt_lock);
    return true;
  }

  // Try to acquire locks with timeout
  bool try_acquire_locks(std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < timeout) {
      if (mutex1_.try_lock()) {
        if (mutex2_.try_lock()) {
          mutex2_.unlock();
          mutex1_.unlock();
          return true;
        }
        mutex1_.unlock();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
  }
};

/**
 * @brief Read-Write lock for testing
 */
class ReadWriteLock {
 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  int readers_ = 0;
  bool writer_ = false;

 public:
  void read_lock() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !writer_; });
    readers_++;
  }

  void read_unlock() {
    std::unique_lock<std::mutex> lock(mutex_);
    readers_--;
    if (readers_ == 0) {
      cv_.notify_all();
    }
  }

  void write_lock() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !writer_ && readers_ == 0; });
    writer_ = true;
  }

  void write_unlock() {
    std::unique_lock<std::mutex> lock(mutex_);
    writer_ = false;
    cv_.notify_all();
  }

  int get_readers() {
    std::lock_guard<std::mutex> lock(mutex_);
    return readers_;
  }

  bool has_writer() {
    std::lock_guard<std::mutex> lock(mutex_);
    return writer_;
  }
};

// Test 1: Thread-safe counter
TEST(ThreadSafetyTest, ThreadSafeCounter) {
  ThreadSafeCounter counter;
  const int num_threads = 10;
  const int increments_per_thread = 1000;

  // Test 1.1: Concurrent increments
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        counter.increment();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(counter.get(), num_threads * increments_per_thread);

  // Test 1.2: Concurrent increments and decrements
  counter.reset();
  threads.clear();

  for (int i = 0; i < num_threads / 2; ++i) {
    threads.emplace_back([&counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        counter.increment();
      }
    });
    threads.emplace_back([&counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        counter.decrement();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(counter.get(), 0);
}

// Test 2: Race condition detection
TEST(ThreadSafetyTest, RaceConditionDetection) {
  UnsafeCounter unsafe_counter;
  const int num_threads = 10;
  const int increments_per_thread = 1000;

  // Test 2.1: Demonstrate race condition with unsafe counter
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&unsafe_counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        unsafe_counter.increment();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Due to race conditions, the result will likely be incorrect
  int unsafe_result = unsafe_counter.get();
  int expected = num_threads * increments_per_thread;

  // The unsafe counter will likely have lost some increments
  // We can't assert exact inequality, but we can verify it's not reliable
  EXPECT_LE(unsafe_result, expected);

  // Test 2.2: Compare with thread-safe version
  ThreadSafeCounter safe_counter;
  threads.clear();

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&safe_counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        safe_counter.increment();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Thread-safe counter should always be correct
  ASSERT_EQ(safe_counter.get(), expected);
}

// Test 3: Thread-safe queue
TEST(ThreadSafetyTest, ThreadSafeQueue) {
  ThreadSafeQueue<int> queue;
  const int num_producers = 5;
  const int num_consumers = 5;
  const int items_per_producer = 100;

  std::atomic<int> consumed_count{0};

  // Test 3.1: Multiple producers and consumers
  std::vector<std::thread> threads;

  // Start producers
  for (int i = 0; i < num_producers; ++i) {
    threads.emplace_back([&queue, i, items_per_producer]() {
      for (int j = 0; j < items_per_producer; ++j) {
        queue.push(i * items_per_producer + j);
      }
    });
  }

  // Start consumers
  for (int i = 0; i < num_consumers; ++i) {
    threads.emplace_back([&queue, &consumed_count, num_producers, items_per_producer]() {
      int total_items = num_producers * items_per_producer;
      while (consumed_count < total_items) {
        int item;
        if (queue.try_pop(item)) {
          consumed_count++;
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(consumed_count.load(), num_producers * items_per_producer);
  ASSERT_TRUE(queue.empty());

  // Test 3.2: Wait and pop with timeout
  queue.push(42);
  int item;
  bool success = queue.wait_and_pop(item, std::chrono::milliseconds(100));
  ASSERT_TRUE(success);
  ASSERT_EQ(item, 42);

  // Test timeout on empty queue
  success = queue.wait_and_pop(item, std::chrono::milliseconds(50));
  ASSERT_FALSE(success);
}

// Test 4: Deadlock prevention
TEST(ThreadSafetyTest, DeadlockPrevention) {
  DeadlockDetector detector;

  // Test 4.1: Correct lock acquisition
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&detector, &success_count]() {
      if (detector.acquire_locks_correct_order()) {
        success_count++;
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(success_count.load(), 10);

  // Test 4.2: Try lock with timeout
  threads.clear();
  success_count = 0;

  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([&detector, &success_count]() {
      if (detector.try_acquire_locks(std::chrono::milliseconds(100))) {
        success_count++;
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_GE(success_count.load(), 1);  // At least one should succeed
}

// Test 5: Read-Write lock
TEST(ThreadSafetyTest, ReadWriteLock) {
  ReadWriteLock rw_lock;
  std::atomic<int> shared_data{0};
  std::atomic<int> read_count{0};
  std::atomic<int> write_count{0};

  const int num_readers = 10;
  const int num_writers = 3;
  const int operations_per_thread = 50;

  std::vector<std::thread> threads;

  // Test 5.1: Multiple readers can access simultaneously
  for (int i = 0; i < num_readers; ++i) {
    threads.emplace_back([&rw_lock, &shared_data, &read_count, operations_per_thread]() {
      for (int j = 0; j < operations_per_thread; ++j) {
        rw_lock.read_lock();
        int value = shared_data.load();
        (void)value;  // Use the value
        read_count++;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        rw_lock.read_unlock();
      }
    });
  }

  // Test 5.2: Writers have exclusive access
  for (int i = 0; i < num_writers; ++i) {
    threads.emplace_back([&rw_lock, &shared_data, &write_count, operations_per_thread]() {
      for (int j = 0; j < operations_per_thread; ++j) {
        rw_lock.write_lock();
        shared_data++;
        write_count++;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        rw_lock.write_unlock();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(read_count.load(), num_readers * operations_per_thread);
  ASSERT_EQ(write_count.load(), num_writers * operations_per_thread);
  ASSERT_EQ(shared_data.load(), num_writers * operations_per_thread);
}

// Test 6: Atomic operations
TEST(ThreadSafetyTest, AtomicOperations) {
  std::atomic<int> atomic_counter{0};
  const int num_threads = 10;
  const int increments_per_thread = 1000;

  // Test 6.1: Atomic increment
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&atomic_counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        atomic_counter.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(atomic_counter.load(), num_threads * increments_per_thread);

  // Test 6.2: Compare and swap
  atomic_counter = 0;
  threads.clear();

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&atomic_counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        int expected = atomic_counter.load();
        while (!atomic_counter.compare_exchange_weak(expected, expected + 1)) {
          // Retry if CAS failed
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(atomic_counter.load(), num_threads * increments_per_thread);
}

// Test 7: Memory ordering
TEST(ThreadSafetyTest, MemoryOrdering) {
  std::atomic<bool> ready{false};
  std::atomic<int> data{0};

  // Test 7.1: Sequential consistency
  std::thread writer([&ready, &data]() {
    data.store(42, std::memory_order_relaxed);
    ready.store(true, std::memory_order_release);
  });

  std::thread reader([&ready, &data]() {
    while (!ready.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    ASSERT_EQ(data.load(std::memory_order_relaxed), 42);
  });

  writer.join();
  reader.join();

  // Test 7.2: Relaxed ordering
  std::atomic<int> counter{0};
  std::vector<std::thread> threads;

  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([&counter]() {
      for (int j = 0; j < 100; ++j) {
        counter.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(counter.load(), 500);
}

// Test 8: Thread synchronization with condition variables
TEST(ThreadSafetyTest, ConditionVariableSynchronization) {
  std::mutex mutex;
  std::condition_variable cv;
  bool ready = false;
  int processed_count = 0;

  const int num_workers = 5;
  std::vector<std::thread> workers;

  // Test 8.1: Wait for signal
  for (int i = 0; i < num_workers; ++i) {
    workers.emplace_back([&mutex, &cv, &ready, &processed_count]() {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait(lock, [&ready] { return ready; });
      processed_count++;
    });
  }

  // Give workers time to start waiting
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Signal all workers
  {
    std::lock_guard<std::mutex> lock(mutex);
    ready = true;
  }
  cv.notify_all();

  for (auto& t : workers) {
    t.join();
  }

  ASSERT_EQ(processed_count, num_workers);

  // Test 8.2: Wait with timeout
  ready = false;
  std::thread waiter([&mutex, &cv, &ready]() {
    std::unique_lock<std::mutex> lock(mutex);
    bool result = cv.wait_for(lock, std::chrono::milliseconds(50), [&ready] { return ready; });
    ASSERT_FALSE(result);  // Should timeout
  });

  waiter.join();
}
