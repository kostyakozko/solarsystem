/**
 * @file test_failure_simulation.cpp
 * @brief Realistic failure simulation testing (Task 18)
 * @note Migrated to Google Test
 *
 * Tests failure simulation capabilities:
 * - Hardware failure simulation (disk, network, memory)
 * - Software failure simulation (crashes, hangs, corruption)
 * - External service failure simulation (JPL API, databases)
 * - Partial failure and degraded service testing
 *
 * Requirements: 6.2
 */

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

/**
 * @brief Failure simulation system
 */
class FailureSimulator {
 public:
  // Failure types
  enum class FailureType {
    None,
    DiskFailure,
    NetworkFailure,
    MemoryFailure,
    CrashFailure,
    HangFailure,
    CorruptionFailure,
    ServiceUnavailable,
    PartialFailure
  };

  // Failure result
  struct FailureResult {
    FailureType type = FailureType::None;
    bool failed = false;
    std::string description;
    std::chrono::milliseconds duration{0};
    bool recovered = false;
  };

  // Service state
  enum class ServiceState { Available, Degraded, Unavailable };

 private:
  std::mt19937 rng_;
  std::uniform_real_distribution<double> prob_dist_{0.0, 1.0};

 public:
  FailureSimulator() : rng_(std::random_device{}()) {}

  explicit FailureSimulator(unsigned int seed) : rng_(seed) {}

  // Simulate disk failure
  FailureResult simulate_disk_failure(const std::string& failure_mode) {
    FailureResult result;
    result.type = FailureType::DiskFailure;
    result.failed = true;

    if (failure_mode == "read_error") {
      result.description = "Disk read error: I/O error";
    } else if (failure_mode == "write_error") {
      result.description = "Disk write error: No space left on device";
    } else if (failure_mode == "disk_full") {
      result.description = "Disk full: Cannot write data";
    } else if (failure_mode == "bad_sector") {
      result.description = "Bad sector detected: Data corruption possible";
    } else {
      result.description = "Unknown disk failure";
    }

    result.duration = std::chrono::milliseconds(10 + rng_() % 90);
    return result;
  }

  // Simulate network failure
  FailureResult simulate_network_failure(const std::string& failure_mode) {
    FailureResult result;
    result.type = FailureType::NetworkFailure;
    result.failed = true;

    if (failure_mode == "packet_loss") {
      result.description = "Network packet loss: 50% packets dropped";
    } else if (failure_mode == "high_latency") {
      result.description = "High network latency: 5000ms delay";
      result.duration = std::chrono::milliseconds(5000);
    } else if (failure_mode == "connection_drop") {
      result.description = "Connection dropped unexpectedly";
    } else if (failure_mode == "dns_failure") {
      result.description = "DNS resolution failed";
    } else {
      result.description = "Unknown network failure";
    }

    return result;
  }

  // Simulate memory failure
  FailureResult simulate_memory_failure(const std::string& failure_mode) {
    FailureResult result;
    result.type = FailureType::MemoryFailure;
    result.failed = true;

    if (failure_mode == "allocation_failure") {
      result.description = "Memory allocation failed: Out of memory";
    } else if (failure_mode == "memory_leak") {
      result.description = "Memory leak detected: Gradual exhaustion";
    } else if (failure_mode == "corruption") {
      result.description = "Memory corruption: Invalid data detected";
    } else {
      result.description = "Unknown memory failure";
    }

    return result;
  }

  // Simulate software crash
  FailureResult simulate_crash(const std::string& crash_type) {
    FailureResult result;
    result.type = FailureType::CrashFailure;
    result.failed = true;

    if (crash_type == "segfault") {
      result.description = "Segmentation fault: Invalid memory access";
    } else if (crash_type == "assertion") {
      result.description = "Assertion failed: Invariant violated";
    } else if (crash_type == "uncaught_exception") {
      result.description = "Uncaught exception: Unhandled error";
    } else {
      result.description = "Unknown crash type";
    }

    return result;
  }

  // Simulate software hang
  FailureResult simulate_hang(int duration_ms) {
    FailureResult result;
    result.type = FailureType::HangFailure;
    result.failed = true;
    result.description = "Process hang: Unresponsive for " +
                         std::to_string(duration_ms) + "ms";

    // Actually hang for a short time (scaled down for testing)
    std::this_thread::sleep_for(
        std::chrono::milliseconds(std::min(duration_ms / 10, 100)));
    result.duration = std::chrono::milliseconds(duration_ms);

    return result;
  }

  // Simulate data corruption
  FailureResult simulate_corruption(const std::string& corruption_type) {
    FailureResult result;
    result.type = FailureType::CorruptionFailure;
    result.failed = true;

    if (corruption_type == "bit_flip") {
      result.description = "Bit flip detected: Single bit error";
    } else if (corruption_type == "partial_write") {
      result.description = "Partial write: Incomplete data";
    } else if (corruption_type == "checksum_mismatch") {
      result.description = "Checksum mismatch: Data integrity error";
    } else {
      result.description = "Unknown corruption type";
    }

    return result;
  }

  // Simulate external service failure
  FailureResult simulate_service_failure(const std::string& service_name,
                                         ServiceState state) {
    FailureResult result;
    result.type = FailureType::ServiceUnavailable;

    if (state == ServiceState::Unavailable) {
      result.failed = true;
      result.description = service_name + " service unavailable: 503 error";
    } else if (state == ServiceState::Degraded) {
      result.failed = false;
      result.description = service_name + " service degraded: Slow response";
      result.duration = std::chrono::milliseconds(2000);
    } else {
      result.failed = false;
      result.description = service_name + " service available";
    }

    return result;
  }

  // Simulate partial failure
  FailureResult simulate_partial_failure(double failure_rate) {
    FailureResult result;
    result.type = FailureType::PartialFailure;

    double random_value = prob_dist_(rng_);
    if (random_value < failure_rate) {
      result.failed = true;
      result.description = "Partial failure: Operation failed (" +
                           std::to_string(static_cast<int>(failure_rate * 100)) +
                           "% failure rate)";
    } else {
      result.failed = false;
      result.description = "Operation succeeded";
    }

    return result;
  }

  // Simulate cascading failures
  std::vector<FailureResult> simulate_cascading_failures(int num_services) {
    std::vector<FailureResult> results;

    for (int i = 0; i < num_services; ++i) {
      std::string service_name = "Service_" + std::to_string(i);

      // Each failure increases probability of next failure
      double failure_prob = 0.3 + (i * 0.15);
      if (prob_dist_(rng_) < failure_prob) {
        auto result = simulate_service_failure(service_name,
                                               ServiceState::Unavailable);
        results.push_back(result);
      } else {
        FailureResult success;
        success.description = service_name + " operational";
        results.push_back(success);
      }
    }

    return results;
  }

  // Test recovery from failure
  FailureResult test_recovery(FailureResult failure,
                              std::function<bool()> recovery_func) {
    if (!failure.failed) {
      return failure; // No recovery needed
    }

    try {
      if (recovery_func && recovery_func()) {
        failure.recovered = true;
        failure.description += " [RECOVERED]";
      }
    } catch (...) {
      failure.recovered = false;
    }

    return failure;
  }
};
  // Test 1: Hardware failure simulation
  TEST(FailureSimulationTestingTest, Hardware_Failure_Simulation) {
    FailureSimulator simulator(42);

    // Test 1.1: Disk read error
    {
      auto result = simulator.simulate_disk_failure("read_error");
      ASSERT_TRUE(result.failed);
      ASSERT_TRUE(result.type == FailureSimulator::FailureType::DiskFailure);
      EXPECT_NE(std::string::npos, result.description.find("read error"));
    }

    // Test 1.2: Disk write error
    {
      auto result = simulator.simulate_disk_failure("write_error");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("write error"));
    }

    // Test 1.3: Disk full
    {
      auto result = simulator.simulate_disk_failure("disk_full");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("full"));
    }

    // Test 1.4: Bad sector
    {
      auto result = simulator.simulate_disk_failure("bad_sector");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("Bad sector"));
    }
  }

  // Test 2: Network failure simulation
  TEST(FailureSimulationTestingTest, Network_Failure_Simulation) {
    FailureSimulator simulator(123);

    // Test 2.1: Packet loss
    {
      auto result = simulator.simulate_network_failure("packet_loss");
      ASSERT_TRUE(result.failed);
      ASSERT_TRUE(result.type ==
                  FailureSimulator::FailureType::NetworkFailure);
      EXPECT_NE(std::string::npos, result.description.find("packet loss"));
    }

    // Test 2.2: High latency
    {
      auto result = simulator.simulate_network_failure("high_latency");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("latency"));
      ASSERT_GT(result.duration.count(), 0);
    }

    // Test 2.3: Connection drop
    {
      auto result = simulator.simulate_network_failure("connection_drop");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("dropped"));
    }

    // Test 2.4: DNS failure
    {
      auto result = simulator.simulate_network_failure("dns_failure");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("DNS"));
    }
  }

  // Test 3: Memory failure simulation
  TEST(FailureSimulationTestingTest, Memory_Failure_Simulation) {
    FailureSimulator simulator(456);

    // Test 3.1: Allocation failure
    {
      auto result = simulator.simulate_memory_failure("allocation_failure");
      ASSERT_TRUE(result.failed);
      ASSERT_TRUE(result.type == FailureSimulator::FailureType::MemoryFailure);
      EXPECT_NE(std::string::npos, result.description.find("allocation"));
    }

    // Test 3.2: Memory leak
    {
      auto result = simulator.simulate_memory_failure("memory_leak");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("leak"));
    }

    // Test 3.3: Memory corruption
    {
      auto result = simulator.simulate_memory_failure("corruption");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("corruption"));
    }
  }

  // Test 4: Software crash simulation
  TEST(FailureSimulationTestingTest, Software_Crash_Simulation) {
    FailureSimulator simulator(789);

    // Test 4.1: Segmentation fault
    {
      auto result = simulator.simulate_crash("segfault");
      ASSERT_TRUE(result.failed);
      ASSERT_TRUE(result.type == FailureSimulator::FailureType::CrashFailure);
      EXPECT_NE(std::string::npos, result.description.find("Segmentation"));
    }

    // Test 4.2: Assertion failure
    {
      auto result = simulator.simulate_crash("assertion");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("Assertion"));
    }

    // Test 4.3: Uncaught exception
    {
      auto result = simulator.simulate_crash("uncaught_exception");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("exception"));
    }
  }

  // Test 5: Software hang simulation
  TEST(FailureSimulationTestingTest, Software_Hang_Simulation) {
    FailureSimulator simulator(321);

    // Test 5.1: Short hang
    {
      auto start = std::chrono::steady_clock::now();
      auto result = simulator.simulate_hang(100);
      auto elapsed = std::chrono::steady_clock::now() - start;

      ASSERT_TRUE(result.failed);
      ASSERT_TRUE(result.type == FailureSimulator::FailureType::HangFailure);
      EXPECT_NE(std::string::npos, result.description.find("hang"));
      ASSERT_GT(elapsed.count(), 0);
    }

    // Test 5.2: Long hang (scaled down)
    {
      auto result = simulator.simulate_hang(5000);
      ASSERT_TRUE(result.failed);
      ASSERT_EQ(result.duration.count(), 5000);
    }
  }

  // Test 6: Data corruption simulation
  TEST(FailureSimulationTestingTest, Data_Corruption_Simulation) {
    FailureSimulator simulator(654);

    // Test 6.1: Bit flip
    {
      auto result = simulator.simulate_corruption("bit_flip");
      ASSERT_TRUE(result.failed);
      ASSERT_TRUE(result.type ==
                  FailureSimulator::FailureType::CorruptionFailure);
      EXPECT_NE(std::string::npos, result.description.find("Bit flip"));
    }

    // Test 6.2: Partial write
    {
      auto result = simulator.simulate_corruption("partial_write");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("Partial"));
    }

    // Test 6.3: Checksum mismatch
    {
      auto result = simulator.simulate_corruption("checksum_mismatch");
      ASSERT_TRUE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("Checksum"));
    }
  }

  // Test 7: External service failure simulation
  TEST(FailureSimulationTestingTest, External_Service_Failure_Simulation) {
    FailureSimulator simulator(987);

    // Test 7.1: Service unavailable
    {
      auto result = simulator.simulate_service_failure(
          "JPL_API", FailureSimulator::ServiceState::Unavailable);
      ASSERT_TRUE(result.failed);
      ASSERT_TRUE(result.type ==
                  FailureSimulator::FailureType::ServiceUnavailable);
      EXPECT_NE(std::string::npos, result.description.find("unavailable"));
    }

    // Test 7.2: Service degraded
    {
      auto result = simulator.simulate_service_failure(
          "Database", FailureSimulator::ServiceState::Degraded);
      ASSERT_FALSE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("degraded"));
      ASSERT_GT(result.duration.count(), 0);
    }

    // Test 7.3: Service available
    {
      auto result = simulator.simulate_service_failure(
          "Cache", FailureSimulator::ServiceState::Available);
      ASSERT_FALSE(result.failed);
      EXPECT_NE(std::string::npos, result.description.find("available"));
    }
  }

  // Test 8: Partial failure simulation
  TEST(FailureSimulationTestingTest, Partial_Failure_Simulation) {
    FailureSimulator simulator(111);

    // Test 8.1: High failure rate
    {
      int failures = 0;
      int trials = 100;

      for (int i = 0; i < trials; ++i) {
        auto result = simulator.simulate_partial_failure(0.7); // 70% failure
        if (result.failed) failures++;
      }

      // Should be around 70% failures (allow 20% variance)
      ASSERT_GT(failures, 50);
      ASSERT_LT(failures, 90);
    }

    // Test 8.2: Low failure rate
    {
      int failures = 0;
      int trials = 100;

      for (int i = 0; i < trials; ++i) {
        auto result = simulator.simulate_partial_failure(0.1); // 10% failure
        if (result.failed) failures++;
      }

      // Should be around 10% failures
      ASSERT_LT(failures, 30);
    }

    // Test 8.3: Zero failure rate
    {
      auto result = simulator.simulate_partial_failure(0.0);
      ASSERT_FALSE(result.failed);
    }
  }

  // Test 9: Cascading failures
  TEST(FailureSimulationTestingTest, Cascading_Failures) {
    FailureSimulator simulator(222);

    // Test 9.1: Multiple service failures
    {
      auto results = simulator.simulate_cascading_failures(5);
      ASSERT_EQ(results.size(), 5);

      // Count failures
      int failures = 0;
      for (const auto& result : results) {
        if (result.failed) failures++;
      }

      // Should have some failures due to cascading effect
      ASSERT_GT(failures, 0);
    }

    // Test 9.2: Large cascade
    {
      auto results = simulator.simulate_cascading_failures(10);
      ASSERT_EQ(results.size(), 10);

      // Later services should have higher failure probability
      bool has_failures = false;
      for (const auto& result : results) {
        if (result.failed) {
          has_failures = true;
          break;
        }
      }
      ASSERT_TRUE(has_failures);
    }
  }

  // Test 10: Recovery from failures
  TEST(FailureSimulationTestingTest, Recovery_from_Failures) {
    FailureSimulator simulator(333);

    // Test 10.1: Successful recovery
    {
      auto failure = simulator.simulate_disk_failure("read_error");
      ASSERT_TRUE(failure.failed);

      auto recovery_func = []() { return true; }; // Successful recovery
      auto result = simulator.test_recovery(failure, recovery_func);

      ASSERT_TRUE(result.recovered);
      EXPECT_NE(std::string::npos, result.description.find("RECOVERED"));
    }

    // Test 10.2: Failed recovery
    {
      auto failure = simulator.simulate_network_failure("connection_drop");
      ASSERT_TRUE(failure.failed);

      auto recovery_func = []() { return false; }; // Failed recovery
      auto result = simulator.test_recovery(failure, recovery_func);

      ASSERT_FALSE(result.recovered);
    }

    // Test 10.3: No recovery needed
    {
      FailureSimulator::FailureResult success;
      success.failed = false;
      success.description = "No failure";

      auto result = simulator.test_recovery(success, nullptr);
      ASSERT_FALSE(result.failed);
      ASSERT_FALSE(result.recovered);
    }
  }
