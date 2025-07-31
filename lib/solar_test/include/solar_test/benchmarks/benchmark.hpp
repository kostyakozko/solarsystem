#pragma once

/**
 * @file benchmark.hpp
 * @brief Performance benchmarking framework for Solar System Suite
 *
 * This header provides comprehensive benchmarking capabilities including:
 * - High-precision timing measurements
 * - Memory usage monitoring
 * - Statistical analysis
 * - Performance threshold validation
 * - CSV output for CI integration
 */

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Testing {

/**
 * @brief Result of a single benchmark execution
 */
struct BenchmarkResult {
  std::string name;                             ///< Benchmark name
  std::chrono::nanoseconds min_time{0};         ///< Minimum execution time
  std::chrono::nanoseconds max_time{0};         ///< Maximum execution time
  std::chrono::nanoseconds mean_time{0};        ///< Mean execution time
  std::chrono::nanoseconds median_time{0};      ///< Median execution time
  std::chrono::nanoseconds std_dev{0};          ///< Standard deviation
  size_t iterations = 0;                        ///< Number of iterations
  double operations_per_second = 0.0;           ///< Operations per second
  size_t memory_usage_bytes = 0;                ///< Peak memory usage
  std::map<std::string, std::string> metadata;  ///< Additional metadata

  /**
   * @brief Check if benchmark passed all thresholds
   */
  [[nodiscard]] bool passed_thresholds() const;

  /**
   * @brief Convert to CSV format for CI integration
   */
  [[nodiscard]] std::string to_csv_row() const;

  /**
   * @brief Get CSV header for benchmark results
   */
  [[nodiscard]] static std::string csv_header();
};

/**
 * @brief Performance threshold configuration
 */
struct PerformanceThresholds {
  std::optional<std::chrono::nanoseconds> max_time;  ///< Maximum allowed time
  std::optional<size_t> max_memory_bytes;            ///< Maximum memory usage
  std::optional<double> min_operations_per_second;   ///< Minimum ops/sec
  std::optional<double> max_std_dev_percentage;      ///< Max std dev as % of mean
};

/**
 * @brief High-precision benchmark execution engine
 */
class Benchmark {
 public:
  /**
   * @brief Benchmark configuration
   */
  struct Configuration {
    size_t default_iterations = 1000;                             ///< Default number of iterations
    size_t warmup_iterations = 10;                                ///< Warmup iterations
    bool measure_memory = true;                                   ///< Enable memory measurement
    bool high_precision_timing = true;                            ///< Use high-precision timers
    std::chrono::milliseconds timeout = std::chrono::minutes(5);  ///< Benchmark timeout
  };

  /**
   * @brief Create benchmark with name and default configuration
   */
  explicit Benchmark(const std::string& name);

  /**
   * @brief Create benchmark with name and custom configuration
   */
  Benchmark(const std::string& name, Configuration config);

  /**
   * @brief Set performance thresholds for validation
   */
  void set_thresholds(const PerformanceThresholds& thresholds);

  /**
   * @brief Measure execution time of a function
   *
   * @param func Function to benchmark
   * @param iterations Number of iterations (0 = use default)
   * @return Benchmark result with timing statistics
   */
  template <typename Func>
  BenchmarkResult measure(Func&& func, size_t iterations = 0);

  /**
   * @brief Measure with setup and teardown functions
   *
   * @param setup Setup function called before each iteration
   * @param func Function to benchmark
   * @param teardown Teardown function called after each iteration
   * @param iterations Number of iterations (0 = use default)
   * @return Benchmark result
   */
  template <typename SetupFunc, typename Func, typename TeardownFunc>
  BenchmarkResult measure_with_lifecycle(SetupFunc&& setup, Func&& func, TeardownFunc&& teardown,
                                         size_t iterations = 0);

  /**
   * @brief Measure memory usage during function execution
   *
   * @param func Function to measure
   * @return Peak memory usage in bytes
   */
  template <typename Func>
  size_t measure_memory_usage(Func&& func);

  /**
   * @brief Add metadata to benchmark results
   */
  void add_metadata(const std::string& key, const std::string& value);

  /**
   * @brief Get benchmark name
   */
  [[nodiscard]] const std::string& name() const { return name_; }

  /**
   * @brief Get benchmark configuration
   */
  [[nodiscard]] const Configuration& config() const { return config_; }

 private:
  std::string name_;
  Configuration config_;
  std::optional<PerformanceThresholds> thresholds_;
  std::map<std::string, std::string> metadata_;

  /**
   * @brief Calculate statistical measures from timing data
   */
  BenchmarkResult calculate_statistics(const std::vector<std::chrono::nanoseconds>& timings,
                                       size_t memory_usage) const;

  /**
   * @brief Get current memory usage
   */
  [[nodiscard]] size_t get_current_memory_usage() const;

  /**
   * @brief Validate benchmark result against thresholds
   */
  void validate_thresholds(BenchmarkResult& result) const;
};

/**
 * @brief Benchmark suite for organizing multiple benchmarks
 */
class BenchmarkSuite {
 public:
  /**
   * @brief Create benchmark suite with name
   */
  explicit BenchmarkSuite(const std::string& name);

  /**
   * @brief Add benchmark to suite
   */
  void add_benchmark(std::unique_ptr<Benchmark> benchmark);

  /**
   * @brief Run all benchmarks in suite
   */
  std::vector<BenchmarkResult> run_all();

  /**
   * @brief Run specific benchmark by name
   */
  std::optional<BenchmarkResult> run_benchmark(const std::string& name);

  /**
   * @brief Generate CSV report for all results
   */
  void generate_csv_report(const std::string& output_path,
                           const std::vector<BenchmarkResult>& results);

  /**
   * @brief Get suite name
   */
  [[nodiscard]] const std::string& name() const { return name_; }

 private:
  std::string name_;
  std::vector<std::unique_ptr<Benchmark>> benchmarks_;
};

// Template implementations
template <typename Func>
BenchmarkResult Benchmark::measure(Func&& func, size_t iterations) {
  if (iterations == 0) {
    iterations = config_.default_iterations;
  }

  std::vector<std::chrono::nanoseconds> timings;
  timings.reserve(iterations + config_.warmup_iterations);

  size_t peak_memory = 0;

  // Warmup iterations
  for (size_t i = 0; i < config_.warmup_iterations; ++i) {
    func();
  }

  // Actual benchmark iterations
  for (size_t i = 0; i < iterations; ++i) {
    size_t memory_before = 0;
    if (config_.measure_memory) {
      memory_before = get_current_memory_usage();
    }

    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    if (config_.measure_memory) {
      size_t memory_after = get_current_memory_usage();
      peak_memory = std::max(peak_memory, memory_after - memory_before);
    }

    timings.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
  }

  auto result = calculate_statistics(timings, peak_memory);
  validate_thresholds(result);

  return result;
}

template <typename SetupFunc, typename Func, typename TeardownFunc>
BenchmarkResult Benchmark::measure_with_lifecycle(SetupFunc&& setup, Func&& func,
                                                  TeardownFunc&& teardown, size_t iterations) {
  if (iterations == 0) {
    iterations = config_.default_iterations;
  }

  std::vector<std::chrono::nanoseconds> timings;
  timings.reserve(iterations + config_.warmup_iterations);

  size_t peak_memory = 0;

  // Warmup iterations
  for (size_t i = 0; i < config_.warmup_iterations; ++i) {
    setup();
    func();
    teardown();
  }

  // Actual benchmark iterations
  for (size_t i = 0; i < iterations; ++i) {
    setup();

    size_t memory_before = 0;
    if (config_.measure_memory) {
      memory_before = get_current_memory_usage();
    }

    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    if (config_.measure_memory) {
      size_t memory_after = get_current_memory_usage();
      peak_memory = std::max(peak_memory, memory_after - memory_before);
    }

    teardown();

    timings.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
  }

  auto result = calculate_statistics(timings, peak_memory);
  validate_thresholds(result);

  return result;
}

template <typename Func>
size_t Benchmark::measure_memory_usage(Func&& func) {
  size_t memory_before = get_current_memory_usage();
  func();
  size_t memory_after = get_current_memory_usage();
  return memory_after - memory_before;
}

}  // namespace SolarSystem::Testing
