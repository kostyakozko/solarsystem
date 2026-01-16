/**
 * @file performance_regression_system_simple.cpp
 * @brief Implementation of simplified performance regression testing system (Task 22)
 */

#include "performance_regression_system_simple.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace SolarSystem::Testing::Regression {

// Global instance
std::unique_ptr<SimplePerformanceRegressionSystem> g_simple_performance_system = nullptr;

//=============================================================================
// SimpleBaselineManager Implementation
//=============================================================================

void SimpleBaselineManager::set_baseline(const std::string& component_name,
                                         const std::string& test_name,
                                         const SimplePerformanceMetrics& metrics) {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  PerformanceBaseline baseline;
  baseline.component_name = component_name;
  baseline.test_name = test_name;
  baseline.baseline_time = metrics.execution_time;
  baseline.baseline_memory_kb = metrics.memory_usage_kb;
  baseline.baseline_cpu_percent = metrics.cpu_usage_percent;
  baseline.sample_count = 1;

  auto key = std::make_pair(component_name, test_name);
  baselines_[key] = baseline;
}

void SimpleBaselineManager::update_baseline(const std::string& component_name,
                                            const std::string& test_name,
                                            const SimplePerformanceMetrics& metrics) {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  auto key = std::make_pair(component_name, test_name);
  auto it = baselines_.find(key);

  if (it == baselines_.end()) {
    set_baseline(component_name, test_name, metrics);
    return;
  }

  auto& baseline = it->second;

  // Use simple averaging for updates
  double alpha = 0.2;  // Weight for new sample

  auto new_time_ms = static_cast<double>(metrics.execution_time.count());
  auto old_time_ms = static_cast<double>(baseline.baseline_time.count());
  auto updated_time_ms = old_time_ms * (1.0 - alpha) + new_time_ms * alpha;

  baseline.baseline_time = std::chrono::milliseconds(static_cast<long long>(updated_time_ms));
  baseline.baseline_memory_kb = static_cast<size_t>(baseline.baseline_memory_kb * (1.0 - alpha) +
                                                    metrics.memory_usage_kb * alpha);
  baseline.baseline_cpu_percent =
      baseline.baseline_cpu_percent * (1.0 - alpha) + metrics.cpu_usage_percent * alpha;
  baseline.sample_count++;
}

PerformanceBaseline* SimpleBaselineManager::get_baseline(const std::string& component_name,
                                                         const std::string& test_name) {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  auto key = std::make_pair(component_name, test_name);
  auto it = baselines_.find(key);

  if (it != baselines_.end()) {
    return &it->second;
  }

  return nullptr;
}

void SimpleBaselineManager::save_baselines(const std::string& filename) const {
  std::lock_guard<std::mutex> lock(baselines_mutex_);

  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open baseline file for writing: " + filename);
  }

  file << "# Performance Baselines for Solar System Suite\n";
  file << "# Format: component_name test_name time_ms memory_kb cpu_percent sample_count\n";

  for (const auto& [key, baseline] : baselines_) {
    file << baseline.component_name << " " << baseline.test_name << " "
         << baseline.baseline_time.count() << " " << baseline.baseline_memory_kb << " "
         << baseline.baseline_cpu_percent << " " << baseline.sample_count << "\n";
  }
}

void SimpleBaselineManager::load_baselines(const std::string& filename) {
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
    long long time_ms;
    size_t memory_kb, sample_count;
    double cpu_percent;

    if (iss >> component_name >> test_name >> time_ms >> memory_kb >> cpu_percent >> sample_count) {
      PerformanceBaseline baseline;
      baseline.component_name = component_name;
      baseline.test_name = test_name;
      baseline.baseline_time = std::chrono::milliseconds(time_ms);
      baseline.baseline_memory_kb = memory_kb;
      baseline.baseline_cpu_percent = cpu_percent;
      baseline.sample_count = sample_count;

      auto key = std::make_pair(component_name, test_name);
      baselines_[key] = baseline;
    }
  }
}

size_t SimpleBaselineManager::get_baseline_count() const {
  std::lock_guard<std::mutex> lock(baselines_mutex_);
  return baselines_.size();
}

//=============================================================================
// SimpleRegressionDetector Implementation
//=============================================================================

std::vector<RegressionAlert> SimpleRegressionDetector::detect_regressions(
    const std::string& component_name, const std::string& test_name,
    const SimplePerformanceMetrics& metrics) const {
  std::vector<RegressionAlert> alerts;

  if (!baseline_manager_) {
    return alerts;
  }

  auto* baseline = baseline_manager_->get_baseline(component_name, test_name);
  if (!baseline) {
    return alerts;  // No baseline to compare against
  }

  if (baseline->is_regression(metrics, threshold_)) {
    RegressionAlert alert;
    alert.component_name = component_name;
    alert.test_name = test_name;
    alert.regression_factor = baseline->get_regression_factor(metrics);
    alert.description =
        "Performance regression detected: " + std::to_string(alert.regression_factor) +
        "x slower than baseline";
    alert.recommendation = generate_recommendation(alert);
    alert.current_metrics = metrics;
    alert.baseline = *baseline;

    alerts.push_back(alert);
  }

  return alerts;
}

std::string SimpleRegressionDetector::generate_recommendation(const RegressionAlert& alert) const {
  std::ostringstream oss;

  if (alert.regression_factor > 2.0) {
    oss << "CRITICAL: Severe performance regression detected. ";
    oss << "Immediate investigation required. ";
  } else if (alert.regression_factor > 1.5) {
    oss << "WARNING: Significant performance regression. ";
  } else {
    oss << "INFO: Minor performance regression. ";
  }

  oss << "Consider profiling the code to identify bottlenecks, ";
  oss << "check for algorithmic changes, or review recent modifications.";

  return oss.str();
}

//=============================================================================
// SimpleOptimizationAnalyzer Implementation
//=============================================================================

std::vector<OptimizationRecommendation> SimpleOptimizationAnalyzer::analyze_performance(
    const std::string& component_name, const std::string& test_name,
    const SimplePerformanceMetrics& metrics) const {
  std::vector<OptimizationRecommendation> recommendations;

  // High execution time analysis
  if (metrics.execution_time_ms() > 1000.0) {  // > 1 second
    OptimizationRecommendation rec;
    rec.component_name = component_name;
    rec.test_name = test_name;
    rec.issue_description =
        "Long execution time detected (" + std::to_string(metrics.execution_time_ms()) + " ms)";
    rec.recommendation = "Review algorithm complexity and consider more efficient approaches";
    rec.potential_improvement_percent = 50.0;
    rec.priority_score = 8;
    recommendations.push_back(rec);
  }

  // High memory usage analysis
  if (metrics.memory_usage_kb > 10 * 1024) {  // > 10MB
    OptimizationRecommendation rec;
    rec.component_name = component_name;
    rec.test_name = test_name;
    rec.issue_description =
        "High memory usage detected (" + std::to_string(metrics.memory_usage_kb / 1024) + " MB)";
    rec.recommendation =
        "Consider using memory pools, reducing object sizes, or implementing lazy loading";
    rec.potential_improvement_percent = 30.0;
    rec.priority_score = 7;
    recommendations.push_back(rec);
  }

  // High CPU usage analysis
  if (metrics.cpu_usage_percent > 80.0) {
    OptimizationRecommendation rec;
    rec.component_name = component_name;
    rec.test_name = test_name;
    rec.issue_description =
        "High CPU usage detected (" + std::to_string(metrics.cpu_usage_percent) + "%)";
    rec.recommendation = "Consider algorithmic optimizations, vectorization, or parallelization";
    rec.potential_improvement_percent = 25.0;
    rec.priority_score = 6;
    recommendations.push_back(rec);
  }

  return recommendations;
}

std::vector<OptimizationRecommendation> SimpleOptimizationAnalyzer::analyze_regression_alerts(
    const std::vector<RegressionAlert>& alerts) const {
  std::vector<OptimizationRecommendation> recommendations;

  for (const auto& alert : alerts) {
    OptimizationRecommendation rec;
    rec.component_name = alert.component_name;
    rec.test_name = alert.test_name;
    rec.issue_description = alert.description;
    rec.recommendation = alert.recommendation;
    rec.potential_improvement_percent = (alert.regression_factor - 1.0) * 100.0;
    rec.priority_score =
        alert.regression_factor > 2.0 ? 9 : (alert.regression_factor > 1.5 ? 7 : 5);
    recommendations.push_back(rec);
  }

  return recommendations;
}

//=============================================================================
// SimplePerformanceRegressionSystem Implementation
//=============================================================================

SimplePerformanceRegressionSystem::SimplePerformanceRegressionSystem()
    : baseline_file_("performance_baselines.txt") {
  baseline_manager_ = std::make_shared<SimpleBaselineManager>();
  regression_detector_ = std::make_shared<SimpleRegressionDetector>();
  optimization_analyzer_ = std::make_shared<SimpleOptimizationAnalyzer>();

  regression_detector_->set_baseline_manager(baseline_manager_);
}

void SimplePerformanceRegressionSystem::initialize(const std::string& baseline_file) {
  std::lock_guard<std::mutex> lock(system_mutex_);

  baseline_file_ = baseline_file;
  baseline_manager_->load_baselines(baseline_file_);

  std::cout << "Simple performance regression testing system initialized with "
            << baseline_manager_->get_baseline_count() << " baselines\n";
}

void SimplePerformanceRegressionSystem::register_component(
    const std::string& component_name,
    const std::map<std::string, std::function<SimplePerformanceMetrics()>>& tests) {
  std::lock_guard<std::mutex> lock(system_mutex_);
  registered_components_[component_name] = tests;
}

void SimplePerformanceRegressionSystem::run_component_tests(const std::string& component_name) {
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

    } catch (const std::exception& e) {
      std::cerr << "  ❌ Test failed: " << test_name << " - " << e.what() << std::endl;
    }
  }
}

void SimplePerformanceRegressionSystem::run_all_component_tests() {
  for (const auto& [component_name, tests] : registered_components_) {
    run_component_tests(component_name);
  }
}

void SimplePerformanceRegressionSystem::create_baselines() {
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

void SimplePerformanceRegressionSystem::save_baselines(const std::string& filename) {
  std::string file = filename.empty() ? baseline_file_ : filename;
  baseline_manager_->save_baselines(file);
  std::cout << "Baselines saved to: " << file << std::endl;
}

void SimplePerformanceRegressionSystem::load_baselines(const std::string& filename) {
  std::string file = filename.empty() ? baseline_file_ : filename;
  baseline_manager_->load_baselines(file);
  std::cout << "Loaded " << baseline_manager_->get_baseline_count() << " baselines from: " << file
            << std::endl;
}

std::vector<RegressionAlert> SimplePerformanceRegressionSystem::detect_regressions() {
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

bool SimplePerformanceRegressionSystem::has_regressions() const { return !recent_alerts_.empty(); }

std::vector<OptimizationRecommendation>
SimplePerformanceRegressionSystem::get_optimization_recommendations() {
  recent_recommendations_ = optimization_analyzer_->analyze_regression_alerts(recent_alerts_);
  return recent_recommendations_;
}

void SimplePerformanceRegressionSystem::generate_report(const std::string& filename) {
  std::ofstream file(filename);
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
      file << "- **Regression Factor**: " << std::fixed << std::setprecision(2)
           << alert.regression_factor << "x\n";
      file << "- **Description**: " << alert.description << "\n";
      file << "- **Recommendation**: " << alert.recommendation << "\n\n";
    }
  }

  if (!recent_recommendations_.empty()) {
    file << "## Optimization Recommendations\n\n";
    for (const auto& rec : recent_recommendations_) {
      file << "### " << rec.component_name << " - Priority " << rec.priority_score << "/10\n\n";
      file << "- **Potential Improvement**: " << std::fixed << std::setprecision(1)
           << rec.potential_improvement_percent << "%\n";
      file << "- **Issue**: " << rec.issue_description << "\n";
      file << "- **Recommendation**: " << rec.recommendation << "\n\n";
    }
  }

  std::cout << "Performance regression report generated: " << filename << std::endl;
}

size_t SimplePerformanceRegressionSystem::get_registered_component_count() const {
  return registered_components_.size();
}

size_t SimplePerformanceRegressionSystem::get_total_test_count() const {
  size_t count = 0;
  for (const auto& [component_name, tests] : registered_components_) {
    count += tests.size();
  }
  return count;
}

size_t SimplePerformanceRegressionSystem::get_baseline_count() const {
  return baseline_manager_->get_baseline_count();
}

//=============================================================================
// Convenience Functions
//=============================================================================

void initialize_simple_performance_regression_testing(const std::string& baseline_file) {
  g_simple_performance_system = std::make_unique<SimplePerformanceRegressionSystem>();
  g_simple_performance_system->initialize(baseline_file);
}

void register_simple_component_performance_tests(
    const std::string& component_name,
    const std::map<std::string, std::function<SimplePerformanceMetrics()>>& tests) {
  if (g_simple_performance_system) {
    g_simple_performance_system->register_component(component_name, tests);
  }
}

std::vector<RegressionAlert> run_simple_performance_regression_tests() {
  if (g_simple_performance_system) {
    return g_simple_performance_system->detect_regressions();
  }
  return {};
}

std::vector<OptimizationRecommendation> get_simple_performance_optimization_recommendations() {
  if (g_simple_performance_system) {
    return g_simple_performance_system->get_optimization_recommendations();
  }
  return {};
}

void generate_simple_performance_reports() {
  if (g_simple_performance_system) {
    g_simple_performance_system->generate_report("simple_performance_regression_report.md");
  }
}

SimplePerformanceMetrics create_simple_metrics(const std::string& test_name,
                                               std::chrono::milliseconds execution_time,
                                               size_t memory_kb, double cpu_percent) {
  SimplePerformanceMetrics metrics;
  metrics.test_name = test_name;
  metrics.execution_time = execution_time;
  metrics.memory_usage_kb = memory_kb;
  metrics.cpu_usage_percent = cpu_percent;
  metrics.timestamp = std::chrono::system_clock::now();
  return metrics;
}

}  // namespace SolarSystem::Testing::Regression
