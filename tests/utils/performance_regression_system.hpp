/**
 * @file performance_regression_system.hpp
 * @brief Comprehensive performance regression testing system (Task 22)
 *
 * Provides automated performance regression detection, baseline management,
 * performance monitoring, alerting, and optimization recommendations for
 * all enhanced components in the Solar System Suite.
 */

#ifndef PERFORMANCE_REGRESSION_SYSTEM_HPP
#define PERFORMANCE_REGRESSION_SYSTEM_HPP

#include <chrono>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <optional>
#include <set>
#include <sstream>
#include <iostream>
#include <limits>

#include "../performance_monitor.hpp"
#include "benchmark_utils.h"

namespace SolarSystem::Testing::Regression {

/**
 * @brief Performance baseline for a specific component
 */
struct ComponentBaseline {
    std::string component_name;
    std::string test_name;

    // Core performance metrics
    std::chrono::nanoseconds baseline_execution_time{0};
    size_t baseline_memory_kb = 0;
    double baseline_cpu_percent = 0.0;
    double baseline_cache_miss_rate = 0.0;
    double baseline_ops_per_second = 0.0;

    // Statistical data
    std::chrono::nanoseconds std_deviation{0};
    size_t sample_count = 0;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_updated;

    // Confidence intervals (95%)
    std::chrono::nanoseconds confidence_lower{0};
    std::chrono::nanoseconds confidence_upper{0};

    // Metadata
    std::string version;
    std::string build_config;
    std::string platform;

    [[nodiscard]] bool is_valid() const {
        return baseline_execution_time.count() > 0 && sample_count > 0;
    }

    [[nodiscard]] double get_regression_factor(const PerformanceMetrics& metrics) const {
        if (!is_valid()) return 1.0;
        return static_cast<double>(metrics.execution_time.count()) /
               baseline_execution_time.count();
    }

    [[nodiscard]] bool is_significant_regression(const PerformanceMetrics& metrics,
                                                 double threshold = 1.1) const {
        return get_regression_factor(metrics) > threshold;
    }
};

/**
 * @brief Performance regression alert with detailed analysis
 */
struct RegressionAlert {
    enum class Severity {
        INFO,       // Minor performance change
        WARNING,    // Moderate regression
        CRITICAL    // Severe regression
    };

    enum class MetricType {
        EXECUTION_TIME,
        MEMORY_USAGE,
        CPU_USAGE,
        CACHE_PERFORMANCE,
        THROUGHPUT
    };

    std::string component_name;
    std::string test_name;
    Severity severity;
    MetricType metric_type;

    double regression_factor;
    double threshold_exceeded;
    std::string description;
    std::string recommendation;

    PerformanceMetrics current_metrics;
    ComponentBaseline baseline;

    std::chrono::system_clock::time_point detected_at;

    [[nodiscard]] std::string severity_string() const {
        switch (severity) {
            case Severity::INFO: return "INFO";
            case Severity::WARNING: return "WARNING";
            case Severity::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }

    [[nodiscard]] std::string metric_type_string() const {
        switch (metric_type) {
            case MetricType::EXECUTION_TIME: return "Execution Time";
            case MetricType::MEMORY_USAGE: return "Memory Usage";
            case MetricType::CPU_USAGE: return "CPU Usage";
            case MetricType::CACHE_PERFORMANCE: return "Cache Performance";
            case MetricType::THROUGHPUT: return "Throughput";
            default: return "Unknown";
        }
    }
};

/**
 * @brief Performance optimization recommendation
 */
struct OptimizationRecommendation {
    enum class Category {
        ALGORITHM,
        MEMORY_MANAGEMENT,
        CPU_OPTIMIZATION,
        CACHE_OPTIMIZATION,
        IO_OPTIMIZATION,
        CONCURRENCY
    };

    std::string component_name;
    std::string test_name;
    Category category;

    std::string issue_description;
    std::string recommendation;
    std::string code_example;

    double potential_improvement_percent;
    int priority_score;  // 1-10, 10 being highest priority

    std::vector<std::string> related_metrics;
    std::vector<std::string> implementation_steps;

    [[nodiscard]] std::string category_string() const {
        switch (category) {
            case Category::ALGORITHM: return "Algorithm";
            case Category::MEMORY_MANAGEMENT: return "Memory Management";
            case Category::CPU_OPTIMIZATION: return "CPU Optimization";
            case Category::CACHE_OPTIMIZATION: return "Cache Optimization";
            case Category::IO_OPTIMIZATION: return "I/O Optimization";
            case Category::CONCURRENCY: return "Concurrency";
            default: return "Unknown";
        }
    }
};

/**
 * @brief Component-specific performance baseline manager
 */
class ComponentBaselineManager {
public:
    ComponentBaselineManager() = default;

    void set_baseline(const std::string& component_name,
                     const std::string& test_name,
                     const PerformanceMetrics& metrics);

    void update_baseline(const std::string& component_name,
                        const std::string& test_name,
                        const PerformanceMetrics& metrics);

    [[nodiscard]] std::optional<ComponentBaseline> get_baseline(
        const std::string& component_name,
        const std::string& test_name) const;

    [[nodiscard]] std::vector<ComponentBaseline> get_all_baselines() const;

    void save_baselines(const std::string& filename) const;
    void load_baselines(const std::string& filename);

    void clear_baselines();

    [[nodiscard]] size_t get_baseline_count() const;

private:
    mutable std::mutex baselines_mutex_;
    std::map<std::pair<std::string, std::string>, ComponentBaseline> baselines_;

    void calculate_confidence_interval(ComponentBaseline& baseline) const;
};

/**
 * @brief Automated regression detector with intelligent analysis
 */
class AutomatedRegressionDetector {
public:
    struct DetectionConfig {
        double time_regression_threshold = 1.15;      // 15% slower
        double memory_regression_threshold = 1.20;    // 20% more memory
        double cpu_regression_threshold = 1.25;       // 25% more CPU
        double cache_regression_threshold = 1.30;     // 30% worse cache performance
        double throughput_regression_threshold = 0.85; // 15% less throughput

        bool enable_statistical_analysis = true;
        double confidence_level = 0.95;
        size_t minimum_samples = 3;

        bool enable_trend_analysis = true;
        size_t trend_window_size = 10;
    };

    AutomatedRegressionDetector(const DetectionConfig& config = DetectionConfig{});

    void set_baseline_manager(std::shared_ptr<ComponentBaselineManager> manager);

    [[nodiscard]] std::vector<RegressionAlert> detect_regressions(
        const std::string& component_name,
        const std::string& test_name,
        const PerformanceMetrics& metrics) const;

    [[nodiscard]] std::vector<RegressionAlert> batch_detect_regressions(
        const std::vector<std::pair<std::string, PerformanceMetrics>>& test_results) const;

    void configure_thresholds(const DetectionConfig& config);

    [[nodiscard]] DetectionConfig get_configuration() const;

private:
    DetectionConfig config_;
    std::shared_ptr<ComponentBaselineManager> baseline_manager_;

    [[nodiscard]] RegressionAlert::Severity calculate_severity(double regression_factor,
                                                              double threshold) const;

    [[nodiscard]] std::string generate_recommendation(
        const RegressionAlert& alert) const;

    [[nodiscard]] bool is_statistically_significant(
        const PerformanceMetrics& metrics,
        const ComponentBaseline& baseline) const;
};

/**
 * @brief Performance optimization analyzer
 */
class PerformanceOptimizationAnalyzer {
public:
    PerformanceOptimizationAnalyzer() = default;

    [[nodiscard]] std::vector<OptimizationRecommendation> analyze_performance(
        const std::string& component_name,
        const std::string& test_name,
        const PerformanceMetrics& metrics) const;

    [[nodiscard]] std::vector<OptimizationRecommendation> analyze_regression_alerts(
        const std::vector<RegressionAlert>& alerts) const;

    void add_custom_analyzer(
        const std::string& name,
        std::function<std::vector<OptimizationRecommendation>(const PerformanceMetrics&)> analyzer);

private:
    std::map<std::string, std::function<std::vector<OptimizationRecommendation>(const PerformanceMetrics&)>>
        custom_analyzers_;

    [[nodiscard]] std::vector<OptimizationRecommendation> analyze_memory_performance(
        const std::string& component_name,
        const PerformanceMetrics& metrics) const;

    [[nodiscard]] std::vector<OptimizationRecommendation> analyze_cpu_performance(
        const std::string& component_name,
        const PerformanceMetrics& metrics) const;

    [[nodiscard]] std::vector<OptimizationRecommendation> analyze_cache_performance(
        const std::string& component_name,
        const PerformanceMetrics& metrics) const;

    [[nodiscard]] std::vector<OptimizationRecommendation> analyze_algorithm_performance(
        const std::string& component_name,
        const PerformanceMetrics& metrics) const;
};

/**
 * @brief Performance monitoring and alerting system
 */
class PerformanceMonitoringSystem {
public:
    struct MonitoringConfig {
        bool enable_real_time_monitoring = true;
        std::chrono::seconds monitoring_interval{30};

        bool enable_alerting = true;
        std::string alert_output_file = "performance_alerts.log";

        bool enable_baseline_updates = true;
        double baseline_update_threshold = 0.05;  // 5% improvement to update baseline

        bool enable_optimization_analysis = true;
        std::string optimization_report_file = "optimization_recommendations.md";

        size_t max_alert_history = 1000;
        size_t max_metrics_history = 10000;
    };

    PerformanceMonitoringSystem(const MonitoringConfig& config = MonitoringConfig{});
    ~PerformanceMonitoringSystem();

    void start_monitoring();
    void stop_monitoring();

    void register_component_test(const std::string& component_name,
                                const std::string& test_name,
                                std::function<PerformanceMetrics()> test_function);

    void run_component_test(const std::string& component_name,
                           const std::string& test_name);

    void run_all_registered_tests();

    [[nodiscard]] std::vector<RegressionAlert> get_recent_alerts(
        std::chrono::hours lookback = std::chrono::hours{24}) const;

    [[nodiscard]] std::vector<OptimizationRecommendation> get_optimization_recommendations() const;

    void generate_monitoring_report(const std::string& filename) const;

    void set_baseline_manager(std::shared_ptr<ComponentBaselineManager> manager);
    void set_regression_detector(std::shared_ptr<AutomatedRegressionDetector> detector);
    void set_optimization_analyzer(std::shared_ptr<PerformanceOptimizationAnalyzer> analyzer);

private:
    MonitoringConfig config_;

    std::shared_ptr<ComponentBaselineManager> baseline_manager_;
    std::shared_ptr<AutomatedRegressionDetector> regression_detector_;
    std::shared_ptr<PerformanceOptimizationAnalyzer> optimization_analyzer_;

    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;

    mutable std::mutex data_mutex_;
    std::map<std::pair<std::string, std::string>, std::function<PerformanceMetrics()>> registered_tests_;
    std::vector<RegressionAlert> alert_history_;
    std::vector<OptimizationRecommendation> optimization_recommendations_;

    void monitoring_loop();
    void process_test_result(const std::string& component_name,
                           const std::string& test_name,
                           const PerformanceMetrics& metrics);

    void log_alert(const RegressionAlert& alert);
    void update_optimization_recommendations();
};

/**
 * @brief Comprehensive performance regression testing system
 */
class PerformanceRegressionTestingSystem {
public:
    PerformanceRegressionTestingSystem();
    ~PerformanceRegressionTestingSystem();

    // System initialization
    void initialize(const std::string& baseline_file = "performance_baselines.json");
    void shutdown();

    // Component registration
    void register_component(const std::string& component_name,
                           const std::map<std::string, std::function<PerformanceMetrics()>>& tests);

    // Test execution
    void run_component_tests(const std::string& component_name);
    void run_all_component_tests();

    // Baseline management
    void create_baselines();
    void update_baselines();
    void save_baselines(const std::string& filename = "");
    void load_baselines(const std::string& filename = "");

    // Regression detection
    [[nodiscard]] std::vector<RegressionAlert> detect_regressions();
    [[nodiscard]] bool has_regressions() const;

    // Optimization analysis
    [[nodiscard]] std::vector<OptimizationRecommendation> get_optimization_recommendations();

    // Reporting
    void generate_comprehensive_report(const std::string& filename = "performance_regression_report.md");
    void generate_baseline_report(const std::string& filename = "performance_baselines_report.md");
    void generate_optimization_report(const std::string& filename = "performance_optimization_report.md");

    // Monitoring
    void start_continuous_monitoring();
    void stop_continuous_monitoring();

    // Configuration
    void configure_regression_detection(const AutomatedRegressionDetector::DetectionConfig& config);
    void configure_monitoring(const PerformanceMonitoringSystem::MonitoringConfig& config);

    // Statistics
    [[nodiscard]] size_t get_registered_component_count() const;
    [[nodiscard]] size_t get_total_test_count() const;
    [[nodiscard]] size_t get_baseline_count() const;
    [[nodiscard]] size_t get_alert_count() const;

private:
    std::string baseline_file_;

    std::shared_ptr<ComponentBaselineManager> baseline_manager_;
    std::shared_ptr<AutomatedRegressionDetector> regression_detector_;
    std::shared_ptr<PerformanceOptimizationAnalyzer> optimization_analyzer_;
    std::shared_ptr<PerformanceMonitoringSystem> monitoring_system_;

    std::map<std::string, std::map<std::string, std::function<PerformanceMetrics()>>> registered_components_;

    mutable std::mutex system_mutex_;
    std::vector<RegressionAlert> recent_alerts_;
    std::vector<OptimizationRecommendation> recent_recommendations_;

    void initialize_components();
    void setup_default_analyzers();
};

// Global instance for easy access
extern std::unique_ptr<PerformanceRegressionTestingSystem> g_performance_system;

// Convenience functions
void initialize_performance_regression_testing(const std::string& baseline_file = "performance_baselines.json");
void register_component_performance_tests(const std::string& component_name,
                                        const std::map<std::string, std::function<PerformanceMetrics()>>& tests);
std::vector<RegressionAlert> run_performance_regression_tests();
std::vector<OptimizationRecommendation> get_performance_optimization_recommendations();
void generate_performance_reports();

// Macros for easy component test registration
#define REGISTER_PERFORMANCE_TEST(component, test_name, test_code) \
    register_component_performance_tests(component, {{test_name, [](){ \
        ComprehensivePerformanceMonitor::instance().start_test_monitoring(test_name); \
        test_code; \
        return ComprehensivePerformanceMonitor::instance().stop_test_monitoring(test_name); \
    }}})

#define PERFORMANCE_REGRESSION_TEST(component, test_name, test_code) \
    do { \
        if (g_performance_system) { \
            g_performance_system->register_component(component, {{test_name, [](){ \
                ComprehensivePerformanceMonitor::instance().start_test_monitoring(test_name); \
                test_code; \
                return ComprehensivePerformanceMonitor::instance().stop_test_monitoring(test_name); \
            }}}); \
        } \
    } while(0)

}  // namespace SolarSystem::Testing::Regression

#endif  // PERFORMANCE_REGRESSION_SYSTEM_HPP
