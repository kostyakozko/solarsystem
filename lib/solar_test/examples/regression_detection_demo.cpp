/**
 * @file regression_detection_demo.cpp
 * @brief Demonstration of the performance regression detection system
 *
 * This example shows how to use the regression detection framework to:
 * - Store and compare performance baselines
 * - Detect performance regressions automatically
 * - Analyze performance trends over time
 * - Generate CI-compatible reports
 */

#include <iostream>
#include <random>
#include <thread>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;

/**
 * @brief Simulate a benchmark with variable performance
 */
BenchmarkResult simulate_benchmark(const std::string& name, double performance_factor = 1.0) {
  BenchmarkResult result;
  result.name = name;
  result.iterations = 1000;

  // Simulate variable performance
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<> time_dist(100000.0 * performance_factor, 5000.0);  // nanoseconds
  std::normal_distribution<> memory_dist(1024.0 * 1024.0 * performance_factor, 50000.0);  // bytes

  result.mean_time =
      std::chrono::nanoseconds(static_cast<long long>(std::max(1000.0, time_dist(gen))));
  result.min_time = result.mean_time * 0.8;
  result.max_time = result.mean_time * 1.2;
  result.std_dev = result.mean_time * 0.1;
  result.memory_usage_bytes = static_cast<size_t>(std::max(1024.0, memory_dist(gen)));

  if (result.mean_time.count() > 0) {
    result.operations_per_second = 1e9 / static_cast<double>(result.mean_time.count());
  }

  result.metadata["category"] = "simulation";
  result.metadata["timestamp"] =
      std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count());

  return result;
}

/**
 * @brief Demonstrate baseline creation and storage
 */
void demonstrate_baseline_management() {
  std::cout << "\n=== Baseline Management ===\n";

  RegressionDetectorConfig config;
  config.baseline_storage_path = "demo_baseline.json";
  config.time_regression_threshold = 10.0;    // 10% threshold
  config.memory_regression_threshold = 15.0;  // 15% threshold
  config.auto_update_baseline = true;

  RegressionDetector detector(config);

  // Create some baseline benchmarks
  std::vector<std::string> benchmark_names = {"VectorSort", "MatrixMultiply", "StringProcessing",
                                              "FileIO"};

  std::cout << "Creating baselines for " << benchmark_names.size() << " benchmarks...\n";

  for (const auto& name : benchmark_names) {
    auto result = simulate_benchmark(name, 1.0);  // Normal performance
    detector.set_baseline(name, result, "v1.0.0");

    std::cout << "  " << name << ": " << (result.mean_time.count() / 1000.0) << " μs, "
              << result.memory_usage_bytes / 1024 << " KB\n";
  }

  std::cout << "Baselines saved to: " << config.baseline_storage_path << "\n";
}

/**
 * @brief Demonstrate regression detection
 */
void demonstrate_regression_detection() {
  std::cout << "\n=== Regression Detection ===\n";

  RegressionDetectorConfig config;
  config.baseline_storage_path = "demo_baseline.json";
  config.time_regression_threshold = 10.0;
  config.memory_regression_threshold = 15.0;

  RegressionDetector detector(config);

  // Simulate some benchmarks with varying performance
  std::vector<std::pair<std::string, double>> test_cases = {
      {"VectorSort", 1.05},        // 5% slower (no regression)
      {"MatrixMultiply", 1.15},    // 15% slower (regression!)
      {"StringProcessing", 0.95},  // 5% faster (improvement)
      {"FileIO", 1.25}             // 25% slower (major regression!)
  };

  std::vector<RegressionAnalysis> analyses;

  for (const auto& [name, factor] : test_cases) {
    auto result = simulate_benchmark(name, factor);
    auto analysis = detector.analyze_regression(result);
    analyses.push_back(analysis);

    std::cout << "Benchmark: " << name << "\n";
    std::cout << "  Regression: " << (analysis.has_regression ? "YES" : "NO") << "\n";
    std::cout << "  Severity: " << analysis.severity << "\n";
    std::cout << "  Time change: " << std::fixed << std::setprecision(1)
              << analysis.time_regression_percentage << "%\n";
    std::cout << "  Memory change: " << analysis.memory_regression_percentage << "%\n";

    if (!analysis.alerts.empty()) {
      std::cout << "  Alerts:\n";
      for (const auto& alert : analysis.alerts) {
        std::cout << "    - " << alert << "\n";
      }
    }
    std::cout << "\n";
  }

  // Generate comprehensive report
  std::cout << "=== Regression Report ===\n";
  std::cout << detector.generate_regression_report(analyses);

  // Check for critical regressions
  if (detector.has_critical_regressions(analyses)) {
    std::cout << "⚠️  CRITICAL REGRESSIONS DETECTED!\n";
  } else {
    std::cout << "✅ No critical regressions found.\n";
  }

  // Generate CI output
  try {
    detector.generate_ci_output(analyses, "regression_report.json");
    std::cout << "CI report generated: regression_report.json\n";
  } catch (const std::exception& e) {
    std::cout << "Failed to generate CI report: " << e.what() << "\n";
  }
}

/**
 * @brief Demonstrate trend analysis
 */
void demonstrate_trend_analysis() {
  std::cout << "\n=== Trend Analysis ===\n";

  RegressionDetectorConfig config;
  config.trend_storage_path = "demo_trends.json";
  config.min_samples_for_trend = 5;

  RegressionDetector detector(config);

  // Simulate performance data over time
  std::string benchmark_name = "TrendDemo";
  std::vector<std::string> versions = {"v1.0", "v1.1", "v1.2", "v1.3", "v1.4", "v1.5", "v2.0"};

  std::cout << "Simulating performance data over " << versions.size() << " versions...\n";

  // Simulate gradual performance degradation
  for (size_t i = 0; i < versions.size(); ++i) {
    double degradation_factor = 1.0 + (i * 0.02);  // 2% degradation per version
    auto result = simulate_benchmark(benchmark_name, degradation_factor);

    detector.add_trend_data_point(benchmark_name, result, versions[i]);

    std::cout << "  " << versions[i] << ": " << (result.mean_time.count() / 1000.0) << " μs\n";

    // Small delay to simulate time passing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Analyze trends
  auto trend_analysis = detector.analyze_trends(benchmark_name);

  std::cout << "\n" << trend_analysis.generate_report();
}

/**
 * @brief Demonstrate CI integration features
 */
void demonstrate_ci_integration() {
  std::cout << "\n=== CI Integration ===\n";

  // Simulate some regression analyses
  std::vector<RegressionAnalysis> analyses;

  // Create a mix of results
  for (int i = 0; i < 3; ++i) {
    RegressionAnalysis analysis;
    analysis.benchmark_name = "CITest" + std::to_string(i);
    analysis.has_regression = (i == 1);  // Second one has regression
    analysis.time_regression_percentage = (i == 1) ? 15.0 : 2.0;
    analysis.memory_regression_percentage = (i == 1) ? 20.0 : -1.0;
    analysis.ops_regression_percentage = (i == 1) ? -12.0 : 3.0;
    analysis.severity = (i == 1) ? "major" : "negligible";

    if (analysis.has_regression) {
      analysis.alerts.push_back("Performance degradation detected");
    }

    analyses.push_back(analysis);
  }

  // Generate GitHub Actions output
  try {
    CIIntegration::generate_github_actions_output(analyses, "github_actions_output.json");
    std::cout << "GitHub Actions output generated: github_actions_output.json\n";
  } catch (const std::exception& e) {
    std::cout << "Failed to generate GitHub Actions output: " << e.what() << "\n";
  }

  // Generate performance badge data
  std::string badge_data = CIIntegration::generate_performance_badge(analyses);
  std::cout << "Performance badge data: " << badge_data << "\n";

  // Demonstrate exit code logic
  std::cout << "CI exit code logic:\n";
  std::cout << "  Would exit with code: ";

  bool has_major_regressions = std::any_of(analyses.begin(), analyses.end(), [](const auto& a) {
    return a.has_regression && a.severity != "negligible";
  });

  std::cout << (has_major_regressions ? "1 (failure)" : "0 (success)") << "\n";
}

/**
 * @brief Demonstrate performance alert system
 */
void demonstrate_alert_system() {
  std::cout << "\n=== Alert System ===\n";

  PerformanceAlertSystem::AlertConfig alert_config;
  alert_config.enable_email_alerts = false;   // Disabled for demo
  alert_config.enable_slack_alerts = false;   // Disabled for demo
  alert_config.enable_github_issues = false;  // Disabled for demo
  alert_config.critical_threshold = 20.0;

  PerformanceAlertSystem alert_system(alert_config);

  // Create a critical regression
  RegressionAnalysis critical_analysis;
  critical_analysis.benchmark_name = "CriticalTest";
  critical_analysis.has_regression = true;
  critical_analysis.time_regression_percentage = 25.0;
  critical_analysis.severity = "critical";
  critical_analysis.alerts.push_back("Execution time increased by 25%");

  std::cout << "Alert system configured (alerts disabled for demo)\n";
  std::cout << "Critical regression detected:\n";
  std::cout << "  Benchmark: " << critical_analysis.benchmark_name << "\n";
  std::cout << "  Regression: " << critical_analysis.time_regression_percentage << "%\n";
  std::cout << "  Severity: " << critical_analysis.severity << "\n";

  // In a real scenario, this would send actual alerts
  std::cout << "  Alert would be sent if enabled\n";
}

/**
 * @brief Main demonstration function
 */
int main() {
  std::cout << "Solar System Testing Framework - Regression Detection Demo\n";
  std::cout << "=========================================================\n";

  try {
    demonstrate_baseline_management();
    demonstrate_regression_detection();
    demonstrate_trend_analysis();
    demonstrate_ci_integration();
    demonstrate_alert_system();

    std::cout << "\n=== Demo completed successfully ===\n";
    std::cout << "Generated files:\n";
    std::cout << "  - demo_baseline.json (performance baselines)\n";
    std::cout << "  - demo_trends.json (trend data)\n";
    std::cout << "  - regression_report.json (CI report)\n";
    std::cout << "  - github_actions_output.json (GitHub Actions integration)\n";

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Error during regression detection demo: " << e.what() << "\n";
    return 1;
  }
}
