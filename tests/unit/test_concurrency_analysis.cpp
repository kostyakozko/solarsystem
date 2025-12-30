/**
 * @file test_concurrency_analysis.cpp
 * @brief Concurrency debugging and analysis tools (Task 25)
 * @note Migrated to Google Test
 *
 * Tests concurrency analysis capabilities:
 * - Thread execution tracing and analysis
 * - Concurrency issue reproduction and debugging
 * - Thread safety validation and verification
 * - Concurrency performance analysis and optimization
 *
 * Requirements: 8.5
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
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

/**
 * @brief Thread execution event
 */
struct ThreadEvent {
  std::thread::id thread_id;
  std::chrono::steady_clock::time_point timestamp;
  std::string event_type;
  std::string description;
};

/**
 * @brief Thread execution tracer
 */
class ThreadTracer {
 private:
  std::vector<ThreadEvent> events_;
  mutable std::mutex mutex_;
  bool enabled_ = true;

 public:
  void log_event(const std::string& event_type, const std::string& description) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    ThreadEvent event;
    event.thread_id = std::this_thread::get_id();
    event.timestamp = std::chrono::steady_clock::now();
    event.event_type = event_type;
    event.description = description;
    events_.push_back(event);
  }

  std::vector<ThreadEvent> get_events() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
  }

  void enable() { enabled_ = true; }

  void disable() { enabled_ = false; }

  size_t event_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
  }

  // Analyze events for patterns
  std::map<std::thread::id, int> get_events_per_thread() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::thread::id, int> counts;
    for (const auto& event : events_) {
      counts[event.thread_id]++;
    }
    return counts;
  }

  std::vector<ThreadEvent> get_events_by_type(const std::string& event_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ThreadEvent> filtered;
    for (const auto& event : events_) {
      if (event.event_type == event_type) {
        filtered.push_back(event);
      }
    }
    return filtered;
  }
};

/**
 * @brief Lock contention analyzer
 */
class LockContentionAnalyzer {
 private:
  struct LockStats {
    std::string lock_name;
    std::atomic<long long> acquire_count{0};
    std::atomic<long long> contention_count{0};
    std::atomic<long long> total_wait_time_us{0};
  };

  std::map<std::string, std::shared_ptr<LockStats>> lock_stats_;
  mutable std::mutex mutex_;

 public:
  void record_lock_acquire(const std::string& lock_name, bool contended,
                          long long wait_time_us) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (lock_stats_.find(lock_name) == lock_stats_.end()) {
      lock_stats_[lock_name] = std::make_shared<LockStats>();
      lock_stats_[lock_name]->lock_name = lock_name;
    }

    auto& stats = lock_stats_[lock_name];
    stats->acquire_count++;
    if (contended) {
      stats->contention_count++;
      stats->total_wait_time_us += wait_time_us;
    }
  }

  double get_contention_rate(const std::string& lock_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = lock_stats_.find(lock_name);
    if (it == lock_stats_.end()) return 0.0;

    long long acquires = it->second->acquire_count.load();
    long long contentions = it->second->contention_count.load();
    return acquires > 0 ? static_cast<double>(contentions) / acquires : 0.0;
  }

  long long get_average_wait_time(const std::string& lock_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = lock_stats_.find(lock_name);
    if (it == lock_stats_.end()) return 0;

    long long contentions = it->second->contention_count.load();
    long long total_wait = it->second->total_wait_time_us.load();
    return contentions > 0 ? total_wait / contentions : 0;
  }

  std::vector<std::string> get_high_contention_locks(double threshold = 0.1) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> high_contention;

    for (const auto& [name, stats] : lock_stats_) {
      long long acquires = stats->acquire_count.load();
      long long contentions = stats->contention_count.load();
      double rate = acquires > 0 ? static_cast<double>(contentions) / acquires : 0.0;

      if (rate >= threshold) {
        high_contention.push_back(name);
      }
    }

    return high_contention;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    lock_stats_.clear();
  }
};

/**
 * @brief Deadlock detector
 */
class DeadlockDetector {
 private:
  struct ThreadLockInfo {
    std::thread::id thread_id;
    std::vector<std::string> held_locks;
    std::string waiting_for;
    std::chrono::steady_clock::time_point wait_start;
  };

  std::map<std::thread::id, ThreadLockInfo> thread_info_;
  mutable std::mutex mutex_;

 public:
  void thread_acquired_lock(const std::string& lock_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto tid = std::this_thread::get_id();
    thread_info_[tid].thread_id = tid;
    thread_info_[tid].held_locks.push_back(lock_name);
    thread_info_[tid].waiting_for.clear();
  }

  void thread_released_lock(const std::string& lock_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto tid = std::this_thread::get_id();
    auto& locks = thread_info_[tid].held_locks;
    locks.erase(std::remove(locks.begin(), locks.end(), lock_name), locks.end());
  }

  void thread_waiting_for_lock(const std::string& lock_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto tid = std::this_thread::get_id();
    thread_info_[tid].thread_id = tid;
    thread_info_[tid].waiting_for = lock_name;
    thread_info_[tid].wait_start = std::chrono::steady_clock::now();
  }

  bool detect_potential_deadlock() const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Simple cycle detection in wait-for graph
    for (const auto& [tid, info] : thread_info_) {
      if (!info.waiting_for.empty()) {
        // Check if any thread holding the lock we're waiting for is also waiting
        for (const auto& [other_tid, other_info] : thread_info_) {
          if (other_tid == tid) continue;

          // Check if other thread holds the lock we want
          bool holds_lock = std::find(other_info.held_locks.begin(),
                                     other_info.held_locks.end(),
                                     info.waiting_for) != other_info.held_locks.end();

          // And is waiting for a lock we hold
          if (holds_lock && !other_info.waiting_for.empty()) {
            bool we_hold_their_lock =
                std::find(info.held_locks.begin(), info.held_locks.end(),
                         other_info.waiting_for) != info.held_locks.end();

            if (we_hold_their_lock) {
              return true;  // Potential deadlock detected
            }
          }
        }
      }
    }

    return false;
  }

  std::vector<std::thread::id> get_waiting_threads() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::thread::id> waiting;

    for (const auto& [tid, info] : thread_info_) {
      if (!info.waiting_for.empty()) {
        waiting.push_back(tid);
      }
    }

    return waiting;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    thread_info_.clear();
  }
};

/**
 * @brief Performance profiler for concurrent operations
 */
class ConcurrencyProfiler {
 private:
  struct OperationStats {
    std::string operation_name;
    std::atomic<long long> call_count{0};
    std::atomic<long long> total_duration_us{0};
    std::atomic<long long> min_duration_us{LLONG_MAX};
    std::atomic<long long> max_duration_us{0};
  };

  std::map<std::string, std::shared_ptr<OperationStats>> operation_stats_;
  mutable std::mutex mutex_;

 public:
  void record_operation(const std::string& operation_name, long long duration_us) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (operation_stats_.find(operation_name) == operation_stats_.end()) {
      operation_stats_[operation_name] = std::make_shared<OperationStats>();
      operation_stats_[operation_name]->operation_name = operation_name;
    }

    auto& stats = operation_stats_[operation_name];
    stats->call_count++;
    stats->total_duration_us += duration_us;

    // Update min
    long long current_min = stats->min_duration_us.load();
    while (duration_us < current_min &&
           !stats->min_duration_us.compare_exchange_weak(current_min, duration_us)) {
    }

    // Update max
    long long current_max = stats->max_duration_us.load();
    while (duration_us > current_max &&
           !stats->max_duration_us.compare_exchange_weak(current_max, duration_us)) {
    }
  }

  long long get_average_duration(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = operation_stats_.find(operation_name);
    if (it == operation_stats_.end()) return 0;

    long long count = it->second->call_count.load();
    long long total = it->second->total_duration_us.load();
    return count > 0 ? total / count : 0;
  }

  long long get_min_duration(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = operation_stats_.find(operation_name);
    if (it == operation_stats_.end()) return 0;
    long long min_val = it->second->min_duration_us.load();
    return min_val == LLONG_MAX ? 0 : min_val;
  }

  long long get_max_duration(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = operation_stats_.find(operation_name);
    if (it == operation_stats_.end()) return 0;
    return it->second->max_duration_us.load();
  }

  long long get_call_count(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = operation_stats_.find(operation_name);
    if (it == operation_stats_.end()) return 0;
    return it->second->call_count.load();
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    operation_stats_.clear();
  }
};

// Test 1: Thread execution tracing
TEST(ConcurrencyAnalysisTest, ThreadExecutionTracing) {
  ThreadTracer tracer;
  const int num_threads = 5;
  const int events_per_thread = 10;

  std::vector<std::thread> threads;

  // Test 1.1: Basic event logging
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&tracer, i, events_per_thread]() {
      for (int j = 0; j < events_per_thread; ++j) {
        tracer.log_event("OPERATION", "Thread " + std::to_string(i) + " event " +
                                         std::to_string(j));
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(tracer.event_count(), static_cast<size_t>(num_threads * events_per_thread));

  // Test 1.2: Events per thread analysis
  auto events_per_thread_map = tracer.get_events_per_thread();
  ASSERT_EQ(events_per_thread_map.size(), static_cast<size_t>(num_threads));

  for (const auto& [tid, count] : events_per_thread_map) {
    ASSERT_EQ(count, events_per_thread);
  }

  // Test 1.3: Filter events by type
  auto operation_events = tracer.get_events_by_type("OPERATION");
  ASSERT_EQ(operation_events.size(), static_cast<size_t>(num_threads * events_per_thread));
}

// Test 2: Lock contention analysis
TEST(ConcurrencyAnalysisTest, LockContentionAnalysis) {
  LockContentionAnalyzer analyzer;

  // Test 2.1: Record lock acquisitions
  analyzer.record_lock_acquire("lock1", false, 0);
  analyzer.record_lock_acquire("lock1", true, 100);
  analyzer.record_lock_acquire("lock1", true, 200);
  analyzer.record_lock_acquire("lock1", false, 0);

  double contention_rate = analyzer.get_contention_rate("lock1");
  ASSERT_EQ(contention_rate, 0.5);  // 2 out of 4 were contended

  long long avg_wait = analyzer.get_average_wait_time("lock1");
  ASSERT_EQ(avg_wait, 150);  // (100 + 200) / 2

  // Test 2.2: High contention detection
  analyzer.record_lock_acquire("lock2", true, 50);
  analyzer.record_lock_acquire("lock2", true, 50);
  analyzer.record_lock_acquire("lock2", false, 0);

  auto high_contention = analyzer.get_high_contention_locks(0.5);
  ASSERT_GE(high_contention.size(), 1);
}

// Test 3: Deadlock detection
TEST(ConcurrencyAnalysisTest, DeadlockDetection) {
  DeadlockDetector detector;

  // Test 3.1: No deadlock scenario
  std::thread t1([&detector]() {
    detector.thread_acquired_lock("lock1");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    detector.thread_released_lock("lock1");
  });

  std::thread t2([&detector]() {
    detector.thread_acquired_lock("lock2");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    detector.thread_released_lock("lock2");
  });

  t1.join();
  t2.join();

  ASSERT_FALSE(detector.detect_potential_deadlock());

  // Test 3.2: Simulated deadlock scenario (without actual deadlock)
  detector.clear();

  // Simulate thread 1 holding lockA and waiting for lockB
  detector.thread_acquired_lock("lockA");
  detector.thread_waiting_for_lock("lockB");

  // Simulate thread 2 (in a separate scope) holding lockB and waiting for lockA
  std::thread t3([&detector]() {
    detector.thread_acquired_lock("lockB");
    detector.thread_waiting_for_lock("lockA");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    detector.thread_released_lock("lockB");
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  bool deadlock_detected = detector.detect_potential_deadlock();
  ASSERT_TRUE(deadlock_detected);

  // Clean up
  detector.thread_released_lock("lockA");
  t3.join();
}

// Test 4: Performance profiling
TEST(ConcurrencyAnalysisTest, PerformanceProfiling) {
  ConcurrencyProfiler profiler;

  // Test 4.1: Record operations
  profiler.record_operation("fast_op", 10);
  profiler.record_operation("fast_op", 20);
  profiler.record_operation("fast_op", 15);

  profiler.record_operation("slow_op", 100);
  profiler.record_operation("slow_op", 200);

  ASSERT_EQ(profiler.get_call_count("fast_op"), 3);
  ASSERT_EQ(profiler.get_call_count("slow_op"), 2);

  ASSERT_EQ(profiler.get_average_duration("fast_op"), 15);  // (10+20+15)/3
  ASSERT_EQ(profiler.get_average_duration("slow_op"), 150);  // (100+200)/2

  ASSERT_EQ(profiler.get_min_duration("fast_op"), 10);
  ASSERT_EQ(profiler.get_max_duration("fast_op"), 20);

  // Test 4.2: Concurrent profiling
  const int num_threads = 10;
  const int operations_per_thread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&profiler, operations_per_thread]() {
      for (int j = 0; j < operations_per_thread; ++j) {
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        auto end = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        profiler.record_operation("concurrent_op", duration);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(profiler.get_call_count("concurrent_op"),
            num_threads * operations_per_thread);
  ASSERT_GT(profiler.get_average_duration("concurrent_op"), 0);
}

// Test 5: Integrated analysis
TEST(ConcurrencyAnalysisTest, IntegratedConcurrencyAnalysis) {
  ThreadTracer tracer;
  LockContentionAnalyzer lock_analyzer;
  ConcurrencyProfiler profiler;

  const int num_threads = 5;
  std::mutex shared_mutex;

  std::vector<std::thread> threads;

  // Test 5.1: Comprehensive analysis
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&tracer, &lock_analyzer, &profiler, &shared_mutex, i]() {
      tracer.log_event("START", "Thread " + std::to_string(i) + " started");

      for (int j = 0; j < 10; ++j) {
        auto start = std::chrono::steady_clock::now();

        bool contended = !shared_mutex.try_lock();
        if (contended) {
          shared_mutex.lock();
        }

        auto lock_acquired = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::microseconds>(
                            lock_acquired - start)
                            .count();

        lock_analyzer.record_lock_acquire("shared_mutex", contended, wait_time);

        // Simulate work
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        shared_mutex.unlock();

        auto end = std::chrono::steady_clock::now();
        auto total_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        profiler.record_operation("critical_section", total_duration);
        tracer.log_event("OPERATION", "Completed operation " + std::to_string(j));
      }

      tracer.log_event("END", "Thread " + std::to_string(i) + " finished");
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Verify all analysis tools captured data
  ASSERT_GT(tracer.event_count(), 0);
  ASSERT_GT(lock_analyzer.get_contention_rate("shared_mutex"), 0.0);
  ASSERT_GT(profiler.get_call_count("critical_section"), 0);
}

// Test 6: Race condition reproduction
TEST(ConcurrencyAnalysisTest, RaceConditionReproduction) {
  ThreadTracer tracer;
  int unsafe_counter = 0;

  const int num_threads = 10;
  const int increments = 100;

  // Test 6.1: Trace race condition
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&tracer, &unsafe_counter, increments, i]() {
      for (int j = 0; j < increments; ++j) {
        tracer.log_event("BEFORE_INCREMENT",
                        "Thread " + std::to_string(i) + " value: " +
                            std::to_string(unsafe_counter));
        unsafe_counter++;
        tracer.log_event("AFTER_INCREMENT",
                        "Thread " + std::to_string(i) + " value: " +
                            std::to_string(unsafe_counter));
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // The tracer should have captured all events
  ASSERT_EQ(tracer.event_count(), static_cast<size_t>(num_threads * increments * 2));

  // The unsafe counter likely has lost updates
  ASSERT_LE(unsafe_counter, num_threads * increments);
}

#pragma clang diagnostic pop
