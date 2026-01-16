/**
 * @file performance_regression_system.cpp
 * @brief Implementation of comprehensive performance regression testing system (Task 22)
 */

#include "performance_regression_system.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

namespace SolarSystem::Testing::Regression {

// Global instance
std::unique_ptr<PerformanceRegressionTestingSystem> g_performance_system = nullptr;

//=============================================================================
// ComponentBaselineManager Implementation
//=============================================================================

void ComponentBaselineManager::set_baseline(const std::string& component_name,
                                            const std::string& test_name,
                                            const PerformanceMetrics& metrics) {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  ComponentBaseline baseline;
  baseline.component_name = component_name;
  baseline.test_name = test_name;
  baseline.baseline_execution_time = metrics.execution_time;
  baseline.baseline_memory_kb = metrics.peak_memory_kb;
  baseline.baseline_cpu_percent = metrics.cpu_stats.total_percent;
  baseline.baseline_cache_miss_rate = metrics.cache_stats.cache_miss_rate;
  baseline.baseline_ops_per_second = 1000.0 / metrics.execution_time_ms();  // Rough estimate
  baseline.sample_count = 1;
  baseline.created_at = std::chrono::system_clock::now();
  baseline.last_updated = baseline.created_at;
  baseline.version = "4.0.0";
  baseline.build_config = "Release";
  baseline.platform = "macOS";

  calculate_confidence_interval(baseline);

  auto key = std::make_pair(component_name, test_name);
  baselines_[key] = baseline;
}

void ComponentBaselineManager::update_baseline(const std::string& component_name,
                                               const std::string& test_name,
                                               const PerformanceMetrics& metrics) {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  auto key = std::make_pair(component_name, test_name);
  auto it = baselines_.find(key);

  if (it == baselines_.end()) {
    set_baseline(component_name, test_name, metrics);
    return;
  }

  auto& baseline = it->second;

  // Use exponential moving average for updates
  double alpha = 0.1;  // Weight for new sample

  auto new_time_ns = static_cast<double>(metrics.execution_time.count());
  auto old_time_ns = static_cast<double>(baseline.baseline_execution_time.count());
  auto updated_time_ns = old_time_ns * (1.0 - alpha) + new_time_ns * alpha;

  baseline.baseline_execution_time =
      std::chrono::nanoseconds(static_cast<long long>(updated_time_ns));
  baseline.baseline_memory_kb = static_cast<size_t>(baseline.baseline_memory_kb * (1.0 - alpha) +
                                                    metrics.peak_memory_kb * alpha);
  baseline.baseline_cpu_percent =
      baseline.baseline_cpu_percent * (1.0 - alpha) + metrics.cpu_stats.total_percent * alpha;
  baseline.baseline_cache_miss_rate = baseline.baseline_cache_miss_rate * (1.0 - alpha) +
                                      metrics.cache_stats.cache_miss_rate * alpha;

  baseline.sample_count++;
  baseline.last_updated = std::chrono::system_clock::now();

  calculate_confidence_interval(baseline);
}

std::optional<ComponentBaseline> ComponentBaselineManager::get_baseline(
    const std::string& component_name, const std::string& test_name) const {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  o key = std::make_pair(component_name, test_name);
  auto it = baselines_.find(key);

  if (it != baselines_.end()) {
    return it->second;
  }

  return std::nullopt;
}

std::vector<ComponentBaseline> ComponentBaselineManager::get_all_baselines() const {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  std::vector<ComponentBaseline> result;
  result.reserve(baselines_.size());

  for (const auto& [key, baseline] : baselines_) {
    result.push_back(baseline);
  }

  return result;
}

void ComponentBaselineManager::save_baselines(const std::string& filename) const {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open baseline file for writing: " + filename);
  }

  file << "# Performance Baselines for Solar System Suite\n";
  file << "# Format: component_name test_name time_ns memory_kb cpu_percent cache_miss_rate "
          "ops_per_sec sample_count\n";

  for (const auto& [key, baseline] : baselines_) {
    file << baseline.component_name << " " << baseline.test_name << " "
         << baseline.baseline_execution_time.count() << " " << baseline.baseline_memory_kb << " "
         << baseline.baseline_cpu_percent << " " << baseline.baseline_cache_miss_rate << " "
         << baseline.baseline_ops_per_second << " " << baseline.sample_count << "\n";
  }
}

void ComponentBaselineManager::load_baselines(const std::string& filename) {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  std::ifstream file(filename);
  if (!file.is_open()) {
    return;  // File doesn't exist, start with empty baselines
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::istringstream iss(line);
    std::string component_name, test_name;
    long long time_ns;
    size_t memory_kb, sample_count;
    double cpu_percent, cache_miss_rate, ops_per_sec;

    if (iss >> component_name >> test_name >> time_ns >> memory_kb >> cpu_percent >>
        cache_miss_rate >> ops_per_sec >> sample_count) {
      ComponentBaseline baseline;
      baseline.component_name = component_name;
      baseline.test_name = test_name;
      baseline.baseline_execution_time = std::chrono::nanoseconds(time_ns);
      baseline.baseline_memory_kb = memory_kb;
      baseline.baseline_cpu_percent = cpu_percent;
      baseline.baseline_cache_miss_rate = cache_miss_rate;
      baseline.baseline_ops_per_second = ops_per_sec;
      baseline.sample_count = sample_count;
      baseline.created_at = std::chrono::system_clock::now();
      baseline.last_updated = baseline.created_at;
      baseline.version = "4.0.0";
      baseline.build_config = "Release";
      baseline.platform = "macOS";

      calculate_confidence_interval(baseline);

      auto key = std::make_pair(component_name, test_name);
      baselines_[key] = baseline;
    }
  }
}

void ComponentBaselineManager::clear_baselines() {
  std::lock_guard<std::mutex> lock(baselines_mutex_);
  baselines_.clear();
}

size_t ComponentBaselineManager::get_baseline_count() const {
  std::lock_guard<std::mutex> lock(baselines_mutex_);
  return baselines_.size();
}

void ComponentBaselineManager::calculate_confidence_interval(ComponentBaseline& baseline) const {
  // Simplified confidence interval calculation
  // In a real implementation, this would use proper statistical methods
  if (baseline.sample_count < 2) {
    baseline.confidence_lower = baseline.baseline_execution_time;
    baseline.confidence_upper = baseline.baseline_execution_time;
    return;
  }

  // Assume 10% standard deviation for simplicity
  auto std_dev_ns = static_cast<long long>(baseline.baseline_execution_time.count() * 0.1);
  baseline.std_deviation = std::chrono::nanoseconds(std_dev_ns);

  // 95% confidence interval (approximately ±2 standard deviations)
  auto margin = std_dev_ns * 2;
  baseline.confidence_lower =
      std::chrono::nanoseconds(std::max(0LL, baseline.baseline_execution_time.count() - margin));
  baseline.confidence_upper =
      std::chrono::nanoseconds(baseline.baseline_execution_time.count() + margin);
}

//=============================================================================
// AutomatedRegressionDetector Implementation
//=============================================================================

AutomatedRegressionDetector::AutomatedRegressionDetector(const DetectionConfig& config)
    : config_(config) {}

void AutomatedRegressionDetector::set_baseline_manager(
    std::shared_ptr<ComponentBaselineManager> manager) {
  baseline_manager_ = manager;
}

std::vector<RegressionAlert> AutomatedRegressionDetector::detect_regressions(
    const std::string& component_name, const std::string& test_name,
    const PerformanceMetrics& metrics) const {
  std::vector<RegressionAlert> alerts;

  if (!baseline_manager_) {
    return alerts;
  }

  auto baseline_opt = baseline_manager_->get_baseline(component_name, test_name);
  if (!baseline_opt.has_value()) {
    return alerts;  // No baseline to compare against
  }

  const auto& baseline = baseline_opt.value();

  // Check execution time regression
  double time_factor = baseline.get_regression_factor(metrics);
  if (time_factor > config_.time_regression_threshold) {
    RegressionAlert alert;
    alert.component_name = component_name;
    alert.test_name = test_name;
    alert.severity = calculate_severity(time_factor, config_.time_regression_threshold);
    alert.metric_type = RegressionAlert::MetricType::EXECUTION_TIME;
    alert.regression_factor = time_factor;
    alert.threshold_exceeded = (time_factor - config_.time_regression_threshold) * 100.0;
    alert.description =
        "Execution time regression: " + std::to_string(time_factor) + "x slower than baseline";
    alert.recommendation = generate_recommendation(alert);
    alert.current_metrics = metrics;
    alert.baseline = baseline;
    alert.detected_at = std::chrono::system_clock::now();

    alerts.push_back(alert);
  }

  // Check memory regression
  if (baseline.baseline_memory_kb > 0) {
    double memory_factor =
        static_cast<double>(metrics.peak_memory_kb) / baseline.baseline_memory_kb;
    if (memory_factor > config_.memory_regression_threshold) {
      RegressionAlert alert;
      alert.component_name = component_name;
      alert.test_name = test_name;
      alert.severity = calculate_severity(memory_factor, config_.memory_regression_threshold);
      alert.metric_type = RegressionAlert::MetricType::MEMORY_USAGE;
      alert.regression_factor = memory_factor;
      alert.threshold_exceeded = (memory_factor - config_.memory_regression_threshold) * 100.0;
      alert.description = "Memory usage regression: " + std::to_string(memory_factor) +
                          "x more memory than baseline";
      alert.recommendation = generate_recommendation(alert);
      alert.current_metrics = metrics;
      alert.baseline = baseline;
      alert.detected_at = std::chrono::system_clock::now();

      alerts.push_back(alert);
    }
  }

  // Check CPU regression
  if (baseline.baseline_cpu_percent > 0) {
    double cpu_factor = metrics.cpu_stats.total_percent / baseline.baseline_cpu_percent;
    if (cpu_factor > config_.cpu_regression_threshold) {
      RegressionAlert alert;
      alert.component_name = component_name;
      alert.test_name = test_name;
      alert.severity = calculate_severity(cpu_factor, config_.cpu_regression_threshold);
      alert.metric_type = RegressionAlert::MetricType::CPU_USAGE;
      alert.regression_factor = cpu_factor;
      alert.threshold_exceeded = (cpu_factor - config_.cpu_regression_threshold) * 100.0;
      alert.description =
          "CPU usage regression: " + std::to_string(cpu_factor) + "x more CPU than baseline";
      alert.recommendation = generate_recommendation(alert);
      alert.current_metrics = metrics;
      alert.baseline = baseline;
      alert.detected_at = std::chrono::system_clock::now();

      alerts.push_back(alert);
    }
  }

  return alerts;
}

std::vector<RegressionAlert> AutomatedRegressionDetector::batch_detect_regressions(
    const std::vector<std::pair<std::string, PerformanceMetrics>>& test_results) const {
  std::vector<RegressionAlert> all_alerts;

  for (const auto& [test_identifier, metrics] : test_results) {
    // Parse component and test name from identifier
    size_t delimiter_pos = test_identifier.find("::");
    if (delimiter_pos == std::string::npos) continue;

    std::string component_name = test_identifier.substr(0, delimiter_pos);
    std::string test_name = test_identifier.substr(delimiter_pos + 2);

    auto alerts = detect_regressions(component_name, test_name, metrics);
    all_alerts.insert(all_alerts.end(), alerts.begin(), alerts.end());
  }

  return all_alerts;
}

void AutomatedRegressionDetector::configure_thresholds(const DetectionConfig& config) {
  config_ = config;
}

AutomatedRegressionDetector::DetectionConfig AutomatedRegressionDetector::get_configuration()
    const {
  return config_;
}

RegressionAlert::Severity AutomatedRegressionDetector::calculate_severity(double regression_factor,
                                                                          double threshold) const {
  double excess = regression_factor - threshold;

  if (excess < 0.1) {  // Less than 10% over threshold
    return RegressionAlert::Severity::INFO;
  } else if (excess < 0.5) {  // Less than 50% over threshold
    return RegressionAlert::Severity::WARNING;
  } else {
    return RegressionAlert::Severity::CRITICAL;
  }
}

std::string AutomatedRegressionDetector::generate_recommendation(
    const RegressionAlert& alert) const {
  std::ostringstream oss;

  switch (alert.metric_type) {
    case RegressionAlert::MetricType::EXECUTION_TIME:
      oss << "Consider profiling the code to identify bottlenecks. ";
      oss << "Check for algorithmic changes, inefficient loops, or blocking operations.";
      break;

    case RegressionAlert::MetricType::MEMORY_USAGE:
      oss << "Review memory allocation patterns. ";
      oss << "Look for memory leaks, excessive allocations, or inefficient data structures.";
      break;

    case RegressionAlert::MetricType::CPU_USAGE:
      oss << "Analyze CPU-intensive operations. ";
      oss << "Consider optimizing computational algorithms or reducing unnecessary calculations.";
      break;

    case RegressionAlert::MetricType::CACHE_PERFORMANCE:
      oss << "Optimize memory access patterns. ";
      oss << "Consider data locality improvements and cache-friendly algorithms.";
      break;

    case RegressionAlert::MetricType::THROUGHPUT:
      oss << "Review system throughput bottlenecks. ";
      oss << "Consider parallelization or I/O optimization opportunities.";
      break;
  }

  return oss.str();
}

bool AutomatedRegressionDetector::is_statistically_significant(
    const PerformanceMetrics& metrics, const ComponentBaseline& baseline) const {
  if (!config_.enable_statistical_analysis || baseline.sample_count < config_.minimum_samples) {
    return true;  // Assume significant if we can't do proper analysis
  }

  // Simplified statistical significance test
  // In a real implementation, this would use proper statistical methods like t-test
  auto current_time = metrics.execution_time.count();
  auto baseline_time = baseline.baseline_execution_time.count();
  auto std_dev = baseline.std_deviation.count();

  if (std_dev == 0) return true;

  double z_score = std::abs(static_cast<double>(current_time - baseline_time)) / std_dev;
  double critical_value = 1.96;  // 95% confidence level

  return z_score > critical_value;
}

//=============================================================================
// PerformanceOptimizationAnalyzer Implementation
//=============================================================================

std::vector<OptimizationRecommendation> PerformanceOptimizationAnalyzer::analyze_performance(
    const std::string& component_name, const std::string& test_name,
    const PerformanceMetrics& metrics) const {
  std::vector<OptimizationRecommendation> recommendations;

  // Analyze different performance aspects
  auto memory_recs = analyze_memory_performance(component_name, metrics);
  auto cpu_recs = analyze_cpu_performance(component_name, metrics);
  auto cache_recs = analyze_cache_performance(component_name, metrics);
  auto algorithm_recs = analyze_algorithm_performance(component_name, metrics);

  recommendations.insert(recommendations.end(), memory_recs.begin(), memory_recs.end());
  recommendations.insert(recommendations.end(), cpu_recs.begin(), cpu_recs.end());
  recommendations.insert(recommendations.end(), cache_recs.begin(), cache_recs.end());
  recommendations.insert(recommendations.end(), algorithm_recs.begin(), algorithm_recs.end());

  // Run custom analyzers
  for (const auto& [name, analyzer] : custom_analyzers_) {
    auto custom_recs = analyzer(metrics);
    recommendations.insert(recommendations.end(), custom_recs.begin(), custom_recs.end());
  }

  // Sort by priority
  std::sort(recommendations.begin(), recommendations.end(),
            [](const OptimizationRecommendation& a, const OptimizationRecommendation& b) {
              return a.priority_score > b.priority_score;
            });

  return recommendations;
}

std::vector<OptimizationRecommendation> PerformanceOptimizationAnalyzer::analyze_regression_alerts(
    const std::vector<RegressionAlert>& alerts) const {
  std::vector<OptimizationRecommendation> recommendations;

  for (const auto& alert : alerts) {
    OptimizationRecommendation rec;
    rec.component_name = alert.component_name;
    rec.test_name = alert.test_name;
    rec.issue_description = alert.description;
    rec.recommendation = alert.recommendation;
    rec.potential_improvement_percent = (alert.regression_factor - 1.0) * 100.0;

    switch (alert.metric_type) {
      case RegressionAlert::MetricType::EXECUTION_TIME:
        rec.category = OptimizationRecommendation::Category::ALGORITHM;
        rec.priority_score = 8;
        break;
      case RegressionAlert::MetricType::MEMORY_USAGE:
        rec.category = OptimizationRecommendation::Category::MEMORY_MANAGEMENT;
        rec.priority_score = 7;
        break;
      case RegressionAlert::MetricType::CPU_USAGE:
        rec.category = OptimizationRecommendation::Category::CPU_OPTIMIZATION;
        rec.priority_score = 6;
        break;
      case RegressionAlert::MetricType::CACHE_PERFORMANCE:
        rec.category = OptimizationRecommendation::Category::CACHE_OPTIMIZATION;
        rec.priority_score = 5;
        break;
      default:
        rec.category = OptimizationRecommendation::Category::ALGORITHM;
        rec.priority_score = 4;
        break;
    }

    // Adjust priority based on severity
    switch (alert.severity) {
      case RegressionAlert::Severity::CRITICAL:
        rec.priority_score = std::min(10, rec.priority_score + 3);
        break;
      case RegressionAlert::Severity::WARNING:
        rec.priority_score = std::min(10, rec.priority_score + 1);
        break;
      default:
        break;
    }

    recommendations.push_back(rec);
  }

  return recommendations;
}

void PerformanceOptimizationAnalyzer::add_custom_analyzer(
    const std::string& name,
    std::function<std::vector<OptimizationRecommendation>(const PerformanceMetrics&)> analyzer) {
  custom_analyzers_[name] = analyzer;
}

std::vector<OptimizationRecommendation> PerformanceOptimizationAnalyzer::analyze_memory_performance(
    const std::string& component_name, const PerformanceMetrics& metrics) const {
  std::vector<OptimizationRecommendation> recommendations;

  // High memory usage analysis
  if (metrics.peak_memory_kb > 100 * 1024) {  // > 100MB
    OptimizationRecommendation rec;
    rec.component_name = component_name;
    rec.category = OptimizationRecommendation::Category::MEMORY_MANAGEMENT;
    rec.issue_description =
        "High memory usage detected (" + std::to_string(metrics.peak_memory_kb / 1024) + " MB)";
    rec.recommendation =
        "Consider using memory pools, reducing object sizes, or implementing lazy loading";
    rec.potential_improvement_percent = 30.0;
    rec.priority_score = 7;
    rec.implementation_steps = {"Profile memory allocation patterns",
                                "Identify largest memory consumers",
                                "Implement memory pooling for frequent allocations",
                                "Consider using smaller data types where appropriate"};
    recommendations.push_back(rec);
  }

  return recommendations;
}

std::vector<OptimizationRecommendation> PerformanceOptimizationAnalyzer::analyze_cpu_performance(
    const std::string& component_name, const PerformanceMetrics& metrics) const {
  std::vector<OptimizationRecommendation> recommendations;

  // High CPU usage analysis
  if (metrics.cpu_stats.total_percent > 80.0) {
    OptimizationRecommendation rec;
    rec.component_name = component_name;
    rec.category = OptimizationRecommendation::Category::CPU_OPTIMIZATION;
    rec.issue_description =
        "High CPU usage detected (" + std::to_string(metrics.cpu_stats.total_percent) + "%)";
    rec.recommendation = "Consider algorithmic optimizations, vectorization, or parallelization";
    rec.potential_improvement_percent = 25.0;
    rec.priority_score = 6;
    rec.implementation_steps = {"Profile CPU hotspots", "Optimize critical loops",
                                "Consider SIMD instructions for mathematical operations",
                                "Evaluate parallelization opportunities"};
    recommendations.push_back(rec);
  }

  return recommendations;
}

std::vector<OptimizationRecommendation> PerformanceOptimizationAnalyzer::analyze_cache_performance(
    const std::string& component_name, const PerformanceMetrics& metrics) const {
  std::vector<OptimizationRecommendation> recommendations;

  // High cache miss rate analysis
  if (metrics.cache_stats.cache_miss_rate > 0.1) {  // > 10% miss rate
    OptimizationRecommendation rec;
    rec.component_name = component_name;
    rec.category = OptimizationRecommendation::Category::CACHE_OPTIMIZATION;
    rec.issue_description = "High cache miss rate detected (" +
                            std::to_string(metrics.cache_stats.cache_miss_rate * 100.0) + "%)";
    rec.recommendation = "Improve data locality and memory access patterns";
    rec.potential_improvement_percent = 20.0;
    rec.priority_score = 5;
    rec.implementation_steps = {
        "Analyze memory access patterns", "Reorganize data structures for better locality",
        "Consider cache-oblivious algorithms", "Implement data prefetching where appropriate"};
    recommendations.push_back(rec);
  }

  return recommendations;
}

std::vector<OptimizationRecommendation>
PerformanceOptimizationAnalyzer::analyze_algorithm_performance(
    const std::string& component_name, const PerformanceMetrics& metrics) const {
  std::vector<OptimizationRecommendation> recommendations;

  // Long execution time analysis
  if (metrics.execution_time_ms() > 1000.0) {  // > 1 second
    OptimizationRecommendation rec;
    rec.component_name = component_name;
    rec.category = OptimizationRecommendation::Category::ALGORITHM;
    rec.issue_description =
        "Long execution time detected (" + std::to_string(metrics.execution_time_ms()) + " ms)";
    rec.recommendation = "Review algorithm complexity and consider more efficient approaches";
    rec.potential_improvement_percent = 50.0;
    rec.priority_score = 8;
    rec.implementation_steps = {
        "Analyze algorithm complexity", "Identify computational bottlenecks",
        "Consider alternative algorithms", "Implement early termination conditions where possible"};
    recommendations.push_back(rec);
  }

  return recommendations;
}

//=============================================================================
// PerformanceRegressionTestingSystem Implementation
//=============================================================================

PerformanceRegressionTestingSystem::PerformanceRegressionTestingSystem()
    : baseline_file_("performance_baselines.txt") {
  initialize_components();
}

PerformanceRegressionTestingSystem::~PerformanceRegressionTestingSystem() { shutdown(); }

void PerformanceRegressionTestingSystem::initialize(const std::string& baseline_file) {
  std::lock_guard<std::mutex> lock(system_mutex_);

  baseline_file_ = baseline_file;

  // Load existing baselines
  baseline_manager_->load_baselines(baseline_file_);

  // Setup default analyzers
  setup_default_analyzers();

  std::cout << "Performance regression testing system initialized with "
            << baseline_manager_->get_baseline_count() << " baselines\n";
}

void PerformanceRegressionTestingSystem::shutdown() {
  if (monitoring_system_) {
    monitoring_system_->stop_monitoring();
  }
}

void PerformanceRegressionTestingSystem::register_component(
    const std::string& component_name,
    const std::map<std::string, std::function<PerformanceMetrics()>>& tests) {
  std::lock_guard<std::mutex> lock(system_mutex_);
  registered_components_[component_name] = tests;

  // Register with monitoring system
  for (const auto& [test_name, test_func] : tests) {
    monitoring_system_->register_component_test(component_name, test_name, test_func);
  }
}

void PerformanceRegressionTestingSystem::run_component_tests(const std::string& component_name) {
  std::lock_guard<std::mutex> lock(system_mutex_);

  auto it = registered_components_.find(component_name);
  if (it == registered_components_.end()) {
    std::cerr << "Component not registered: " << component_name << std::endl;
    return;
  }

  std::cout << "Running performance tests for component: " << component_name << std::endl;

  for (const auto& [test_name, test_func] : it->second) {
    try {
      auto metrics = test_func();

      // Update baseline if this is an improvement
      auto baseline_opt = baseline_manager_->get_baseline(component_name, test_name);
      if (!baseline_opt.has_value()) {
        baseline_manager_->set_baseline(component_name, test_name, metrics);
        std::cout << "  Created baseline for " << test_name << std::endl;
      } else {
        // Check for regressions
        auto alerts = regression_detector_->detect_regressions(component_name, test_name, metrics);
        if (!alerts.empty()) {
          recent_alerts_.insert(recent_alerts_.end(), alerts.begin(), alerts.end());
          std::cout << "  ⚠️  " << alerts.size() << " regression(s) detected in " << test_name
                    << std::endl;
        } else {
          std::cout << "  ✅ " << test_name << " passed (" << metrics.execution_time_ms() << "ms)"
                    << std::endl;
        }
      }

    } catch (const std::exception& e) {
      std::cerr << "  ❌ Test failed: " << test_name << " - " << e.what() << std::endl;
    }
  }
}

void PerformanceRegressionTestingSystem::run_all_component_tests() {
  for (const auto& [component_name, tests] : registered_components_) {
    run_component_tests(component_name);
  }
}

void PerformanceRegressionTestingSystem::create_baselines() {
  std::cout << "Creating performance baselines..." << std::endl;

  for (const auto& [component_name, tests] : registered_components_) {
    for (const auto& [test_name, test_func] : tests) {
      try {
        auto metrics = test_func();
        baseline_manager_->set_baseline(component_name, test_name, metrics);
        std::cout << "  Created baseline: " << component_name << "::" << test_name << std::endl;
      } catch (const std::exception& e) {
        std::cerr << "  Failed to create baseline for " << component_name << "::" << test_name
                  << " - " << e.what() << std::endl;
      }
    }
  }

  save_baselines();
}

void PerformanceRegressionTestingSystem::update_baselines() {
  std::cout << "Updating performance baselines..." << std::endl;

  for (const auto& [component_name, tests] : registered_components_) {
    for (const auto& [test_name, test_func] : tests) {
      try {
        auto metrics = test_func();
        baseline_manager_->update_baseline(component_name, test_name, metrics);
        std::cout << "  Updated baseline: " << component_name << "::" << test_name << std::endl;
      } catch (const std::exception& e) {
        std::cerr << "  Failed to update baseline for " << component_name << "::" << test_name
                  << " - " << e.what() << std::endl;
      }
    }
  }

  save_baselines();
}

void PerformanceRegressionTestingSystem::save_baselines(const std::string& filename) {
  std::string file = filename.empty() ? baseline_file_ : filename;
  baseline_manager_->save_baselines(file);
  std::cout << "Baselines saved to: " << file << std::endl;
}

void PerformanceRegressionTestingSystem::load_baselines(const std::string& filename) {
  std::string file = filename.empty() ? baseline_file_ : filename;
  baseline_manager_->load_baselines(file);
  std::cout << "Loaded " << baseline_manager_->get_baseline_count() << " baselines from: " << file
            << std::endl;
}

std::vector<RegressionAlert> PerformanceRegressionTestingSystem::detect_regressions() {
  recent_alerts_.clear();

  for (const auto& [component_name, tests] : registered_components_) {
    for (const auto& [test_name, test_func] : tests) {
      try {
        auto metrics = test_func();
        auto alerts = regression_detector_->detect_regressions(component_name, test_name, metrics);
        recent_alerts_.insert(recent_alerts_.end(), alerts.begin(), alerts.end());
      } catch (const std::exception& e) {
        std::cerr << "Failed to run regression test for " << component_name << "::" << test_name
                  << " - " << e.what() << std::endl;
      }
    }
  }

  return recent_alerts_;
}

bool PerformanceRegressionTestingSystem::has_regressions() const { return !recent_alerts_.empty(); }

std::vector<OptimizationRecommendation>
PerformanceRegressionTestingSystem::get_optimization_recommendations() {
  recent_recommendations_ = optimization_analyzer_->analyze_regression_alerts(recent_alerts_);
  return recent_recommendations_;
}

void PerformanceRegressionTestingSystem::generate_comprehensive_report(
    const std::string& filename) {
    std::ofstream file(fil;
    if (!file.is_open()) {
    throw std::runtime_error("Failed to open report file: " + filename);
    }

    file << "# Solar System Suite - Performance Regression Report\n\n";
    file << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";

    file << "## Summary\n\n";
    file << "- Registered components: " << registered_components_.size() << "\n";
    file << "- Total tests: " << get_total_test_count() << "\n";
    file << "- Baselines: " << baseline_manager_->get_baseline_count() << "\n";
    file << "- Recent alerts: " << recent_alerts_.size() << "\n\n";

    if (!recent_alerts_.empty()) {
    file << "## Regression Alerts\n\n";
    for (const auto& alert : recent_alerts_) {
      file << "### " << alert.component_name << "::" << alert.test_name << "\n\n";
      file << "- **Severity**: " << alert.severity_string() << "\n";
      file << "- **Metric**: " << alert.metric_type_string() << "\n";
      file << "- **Regression Factor**: " << std::fixed << std::setprecision(2)
           << alert.regression_factor << "x\n";
      file << "- **Description**: " << alert.description << "\n";
      file << "- **Recommendation**: " << alert.recommendation << "\n\n";
    }
    }

    if (!recent_recommendations_.empty()) {
    file << "## Optimization Recommendations\n\n";
    for (const auto& rec : recent_recommendations_) {
      file << "### " << rec.component_name << " - " << rec.category_string() << "\n\n";
      file << "- **Priority**: " << rec.priority_score << "/10\n";
      file << "- **Potential Improvement**: " << std::fixed << std::setprecision(1)
           << rec.potential_improvement_percent << "%\n";
      file << "- **Issue**: " << rec.issue_description << "\n";
      file << "- **Recommendation**: " << rec.recommendation << "\n\n";
    }
    }

    std::cout << "Comprehensive report generated: " << filename << std::endl;
}

void PerformanceRegressionTestingSystem::initialize_components() {
  baseline_manager_ = std::make_shared<ComponentBaselineManager>();
  regression_detector_ = std::make_shared<AutomatedRegressionDetector>();
  optimization_analyzer_ = std::make_shared<PerformanceOptimizationAnalyzer>();
  monitoring_system_ = std::make_shared<PerformanceMonitoringSystem>();

  regression_detector_->set_baseline_manager(baseline_manager_);
  monitoring_system_->set_baseline_manager(baseline_manager_);
  monitoring_system_->set_regression_detector(regression_detector_);
  monitoring_system_->set_optimization_analyzer(optimization_analyzer_);
}

void PerformanceRegressionTestingSystem::setup_default_analyzers() {
  // Add Solar System Suite specific analyzers
  optimization_analyzer_->add_custom_analyzer(
      "JPL_Data_Loading",
      [](const PerformanceMetrics& metrics) -> std::vector<OptimizationRecommendation> {
        std::vector<OptimizationRecommendation> recs;
        if (metrics.execution_time_ms() > 5000) {  // > 5 seconds for JPL data loading
          OptimizationRecommendation rec;
          rec.category = OptimizationRecommendation::Category::IO_OPTIMIZATION;
          rec.issue_description = "Slow JPL data loading";
          rec.recommendation = "Consider implementing parallel downloads or better caching";
          rec.potential_improvement_percent = 60.0;
          rec.priority_score = 9;
          recs.push_back(rec);
        }
        return recs;
      });

  optimization_analyzer_->add_custom_analyzer(
      "Simulation_Performance",
      [](const PerformanceMetrics& metrics) -> std::vector<OptimizationRecommendation> {
        std::vector<OptimizationRecommendation> recs;
        if (metrics.execution_time_ms() > 100) {  // > 100ms for simulation step
          OptimizationRecommendation rec;
          rec.category = OptimizationRecommendation::Category::ALGORITHM;
          rec.issue_description = "Slow simulation step execution";
          rec.recommendation =
              "Consider optimizing N-body calculations or using approximation methods";
          rec.potential_improvement_percent = 40.0;
          rec.priority_score = 8;
          recs.push_back(rec);
        }
        return recs;
      });
}

size_t PerformanceRegressionTestingSystem::get_total_test_count() const {
  size_t count = 0;
  for (const auto& [component_name, tests] : registered_components_) {
    count += tests.size();
  }
  return count;
}

//=============================================================================
// Convenience Functions
//=============================================================================

void initialize_performance_regression_testing(const std::string& baseline_file) {
  g_performance_system = std::make_unique<PerformanceRegressionTestingSystem>();
  g_performance_system->initialize(baseline_file);
}

void register_component_performance_tests(
    const std::string& component_name,
    const std::map<std::string, std::function<PerformanceMetrics()>>& tests) {
  if (g_performance_system) {
    g_performance_system->register_component(component_name, tests);
  }
}

std::vector<RegressionAlert> run_performance_regression_tests() {
  if (g_performance_system) {
    return g_performance_system->detect_regressions();
  }
  return {};
}

std::vector<OptimizationRecommendation> get_performance_optimization_recommendations() {
  if (g_performance_system) {
    return g_performance_system->get_optimization_recommendations();
  }
  return {};
}

void generate_performance_reports() {
  if (g_performance_system) {
    g_performance_system->generate_comprehensive_report("performance_regression_report.md");
  }
}

}  // namespace SolarSystem::Testing::Regression
