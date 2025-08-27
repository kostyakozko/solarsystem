/**
 * @file performance_regression_system_simple.hpp
 * @brief Simplified performance regression testing system (Task 22)
 *
 * Provides basic performance regression detection, baseline management,
 * and optimization recommendations for Solar System Suite components.
 */

#ifndef PERFORMANCE_REGRESSION_SYSTEM_SIMPLE_HPP
#define PERFORMANCE_REGRESSION_SYSTEM_SIMPLE_HPP

#include <chrono>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <iostream>
#include <sstream>

namespace SolarSystem::Testing::Regression {

/**
 * @brief Simple performance metrics
 */
struct SimplePerformanceMetrics {
    std::string test_name;
    std::chrono::milliseconds execution_time{0};
    size_t memory_usage_kb = 0;
    double cpu_usage_percent = 0.0;
    std::chrono::system_clock::time_point timestamp;

    [[nodiscard]] double execution_time_ms() const {
        return static_cast<double>(execution_time.count());
    }
};

/**
 * @brief Performance baseline
 */
struct PerformanceBaseline {
    std::string component_name;
    std::string test_name;
    std::chrono::milliseconds baseline_time{0};
    size_t baseline_memory_kb = 0;
    double baseline_cpu_percent = 0.0;
    size_t sample_count = 0;

    [[nodiscard]] bool is_regression(const SimplePerformanceMetrics& metrics, double threshold = 1.2) const {
        if (baseline_time.count() == 0) return false;

        double time_ratio = static_cast<double>(metrics.execution_time.count()) / baseline_time.count();
        double memory_ratio = baseline_memory_kb > 0 ?
                             static_cast<double>(metrics.memory_usage_kb) / baseline_memory_kb : 1.0;

        return time_ratio > threshold || memory_ratio > threshold;
    }

    [[nodiscard]] double get_regression_factor(const SimplePerformanceMetrics& metrics) const {
        if (baseline_time.count() == 0) return 1.0;
        return static_cast<double>(metrics.execution_time.count()) / baseline_time.count();
    }
};

/**
 * @brief Regression alert
 */
struct RegressionAlert {
    std::string component_name;
    std::string test_name;
    double regression_factor;
    std::string description;
    std::string recommendation;
    SimplePerformanceMetrics current_metrics;
    PerformanceBaseline baseline;
};

/**
 * @brief Optimization recommendation
 */
struct OptimizationRecommendation {
    std::string component_name;
    std::string test_name;
    std::string issue_description;
    std::string recommendation;
    double potential_improvement_percent;
    int priority_score; // 1-10
};

/**
 * @brief Simple baseline manager
 */
class SimpleBaselineManager {
public:
    void set_baseline(const std::string& component_name,
                     const std::string& test_name,
                     const SimplePerformanceMetrics& metrics);

    void update_baseline(const std::string& component_name,
                        const std::string& test_name,
                        const SimplePerformanceMetrics& metrics);

    PerformanceBaseline* get_baseline(const std::string& component_name,
                                     const std::string& test_name);

    void save_baselines(const std::string& filename) const;
    void load_baselines(const std::string& filename);

    size_t get_baseline_count() const;

private:
    mutable std::mutex baselines_mutex_;
    std::map<std::pair<std::string, std::string>, PerformanceBaseline> baselines_;
};

/**
 * @brief Simple regression detector
 */
class SimpleRegressionDetector {
public:
    SimpleRegressionDetector(double threshold = 1.2) : threshold_(threshold) {}

    void set_baseline_manager(std::shared_ptr<SimpleBaselineManager> manager) {
        baseline_manager_ = manager;
    }

    std::vector<RegressionAlert> detect_regressions(
        const std::string& component_name,
        const std::string& test_name,
        const SimplePerformanceMetrics& metrics) const;

private:
    double threshold_;
    std::shared_ptr<SimpleBaselineManager> baseline_manager_;

    std::string generate_recommendation(const RegressionAlert& alert) const;
};

/**
 * @brief Simple optimization analyzer
 */
class SimpleOptimizationAnalyzer {
public:
    std::vector<OptimizationRecommendation> analyze_performance(
        const std::string& component_name,
        const std::string& test_name,
        const SimplePerformanceMetrics& metrics) const;

    std::vector<OptimizationRecommendation> analyze_regression_alerts(
        const std::vector<RegressionAlert>& alerts) const;
};

/**
 * @brief Simple performance regression testing system
 */
class SimplePerformanceRegressionSystem {
public:
    SimplePerformanceRegressionSystem();
    ~SimplePerformanceRegressionSystem() = default;

    // System initialization
    void initialize(const std::string& baseline_file = "performance_baselines.txt");

    // Component registration
    void register_component(const std::string& component_name,
                           const std::map<std::string, std::function<SimplePerformanceMetrics()>>& tests);

    // Test execution
    void run_component_tests(const std::string& component_name);
    void run_all_component_tests();

    // Baseline management
    void create_baselines();
    void save_baselines(const std::string& filename = "");
    void load_baselines(const std::string& filename = "");

    // Regression detection
    std::vector<RegressionAlert> detect_regressions();
    bool has_regressions() const;

    // Optimization analysis
    std::vector<OptimizationRecommendation> get_optimization_recommendations();

    // Reporting
    void generate_report(const std::string& filename = "performance_regression_report.md");

    // Statistics
    size_t get_registered_component_count() const;
    size_t get_total_test_count() const;
    size_t get_baseline_count() const;

private:
    std::string baseline_file_;

    std::shared_ptr<SimpleBaselineManager> baseline_manager_;
    std::shared_ptr<SimpleRegressionDetector> regression_detector_;
    std::shared_ptr<SimpleOptimizationAnalyzer> optimization_analyzer_;

    std::map<std::string, std::map<std::string, std::function<SimplePerformanceMetrics()>>> registered_components_;

    mutable std::mutex system_mutex_;
    std::vector<RegressionAlert> recent_alerts_;
    std::vector<OptimizationRecommendation> recent_recommendations_;
};

// Global instance for easy access
extern std::unique_ptr<SimplePerformanceRegressionSystem> g_simple_performance_system;

// Convenience functions
void initialize_simple_performance_regression_testing(const std::string& baseline_file = "performance_baselines.txt");
void register_simple_component_performance_tests(const std::string& component_name,
                                                const std::map<std::string, std::function<SimplePerformanceMetrics()>>& tests);
std::vector<RegressionAlert> run_simple_performance_regression_tests();
std::vector<OptimizationRecommendation> get_simple_performance_optimization_recommendations();
void generate_simple_performance_reports();

// Utility function to create simple metrics
SimplePerformanceMetrics create_simple_metrics(const std::string& test_name,
                                              std::chrono::milliseconds execution_time,
                                              size_t memory_kb = 0,
                                              double cpu_percent = 0.0);

}  // namespace SolarSystem::Testing::Regression

#endif  // PERFORMANCE_REGRESSION_SYSTEM_SIMPLE_HPP
