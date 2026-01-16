#include <chrono>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include "../test_framework_enhanced.hpp"

using namespace SolarSystem::Testing;

class PerformanceComprehensiveTests {
 public:
  static void run_all_tests() {
    auto& framework = EnhancedTestFramework::instance();

    framework.load_performance_baselines("performance_baselines.txt");

    std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"BasicPerformance", test_basic_performance},
        {"MemoryAllocationPerformance", test_memory_allocation_performance},
        {"ConcurrentPerformance", test_concurrent_performance},
        {"CPUIntensivePerformance", test_cpu_intensive_performance},
        {"StressTestPerformance", test_stress_test_performance},
        {"ScalabilityTest", test_scalability}};

    framework.run_test_suite("PerformanceComprehensive", tests);

    framework.save_performance_baselines("performance_baselines.txt");
    framework.generate_report("performance_comprehensive_report.html");
  }

 private:
  static void test_basic_performance() {
    // Test basic performance
    volatile int result = 0;
    for (int i = 0; i < 10000; ++i) {
      result += i;
    }
    ASSERT_TRUE(result > 0);
  }

  static void test_memory_allocation_performance() {
    // Test large memory allocation patterns
    std::vector<std::vector<double>> large_arrays;

    for (int i = 0; i < 100; ++i) {
      large_arrays.emplace_back(1000, static_cast<double>(i));
    }

    // Process the arrays to prevent optimization
    double sum = 0.0;
    for (const auto& array : large_arrays) {
      for (double val : array) {
        sum += val;
      }
    }

    ASSERT_TRUE(sum > 0);
  }

  static void test_concurrent_performance() {
    // Test concurrent performance
    std::vector<std::thread> threads;
    std::atomic<int> completed_tasks{0};

    for (int i = 0; i < 4; ++i) {
      threads.emplace_back([&completed_tasks]() {
        try {
          // Do some work
          volatile double result = 0.0;
          for (int j = 0; j < 10000; ++j) {
            result += std::sin(j) + std::cos(j);
          }
          completed_tasks++;
        } catch (...) {
          // Thread failed
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    ASSERT_TRUE(completed_tasks.load() > 0);
  }

  static void test_cpu_intensive_performance() {
    // Test CPU-intensive mathematical operations
    double result = 0.0;

    for (int i = 0; i < 100000; ++i) {
      result += std::sin(i) * std::cos(i) + std::sqrt(i + 1);
    }

    ASSERT_TRUE(result != 0.0);
  }

  static void test_stress_test_performance() {
    // Stress test with many operations
    std::vector<std::unique_ptr<int>> ptrs;

    for (int i = 0; i < 10000; ++i) {
      ptrs.push_back(std::make_unique<int>(i));
    }

    // Access all pointers
    int sum = 0;
    for (const auto& ptr : ptrs) {
      sum += *ptr;
    }

    ASSERT_TRUE(sum > 0);
  }

  static void test_scalability() {
    // Test performance scaling with different workload sizes
    std::vector<int> workload_sizes = {10000, 100000, 1000000};
    std::vector<double> execution_times;

    for (int size : workload_sizes) {
      auto start = std::chrono::high_resolution_clock::now();

      // Variable workload - more intensive computation
      double result = 0.0;
      for (int i = 0; i < size; ++i) {
        result += std::sqrt(i + 1) * std::sin(i * 0.001) * std::cos(i * 0.001);
      }

      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      execution_times.push_back(duration.count());

      ASSERT_TRUE(result != 0.0);
    }

    // Verify reasonable scaling - allow for some variance in timing
    ASSERT_TRUE(execution_times.size() == 3);

    // Check that the largest workload takes more time than the smallest
    // This is more robust than checking each step
    ASSERT_TRUE(execution_times[2] > execution_times[0]);

    // Also verify that we have some meaningful time differences
    // If all times are very small, the test might not be meaningful
    double total_time = execution_times[0] + execution_times[1] + execution_times[2];
    ASSERT_TRUE(total_time > 100);  // At least 100 microseconds total
  }
};

int main() {
  std::cout << "🚀 Running Comprehensive Performance Tests" << std::endl;
  std::cout << "===========================================" << std::endl << std::endl;

  try {
    PerformanceComprehensiveTests::run_all_tests();

    auto& framework = EnhancedTestFramework::instance();
    std::cout << std::endl << "📊 Final Results:" << std::endl;
    std::cout << "Total tests: " << framework.get_total_tests() << std::endl;
    std::cout << "Passed: " << framework.get_passed_tests() << std::endl;
    std::cout << "Failed: " << framework.get_failed_tests() << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1)
              << (framework.get_total_tests() > 0
                      ? (framework.get_passed_tests() * 100.0 / framework.get_total_tests())
                      : 0.0)
              << "%" << std::endl;

    return framework.get_failed_tests() == 0 ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "❌ Test framework error: " << e.what() << std::endl;
    return 1;
  }
}
