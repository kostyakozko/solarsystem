/**
 * @file analytics.hpp
 * @brief Advanced analytics and reporting for performance monitoring
 */

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "solar_core/export.hpp"

namespace SolarSystem::Performance {

/**
 * @brief KPI definition and tracking
 */
struct KPI {
  std::string name;
  std::string description;
  double target_value;
  double current_value;
  double warning_threshold;  // % deviation from target
  double critical_threshold;

  enum class Status { ON_TARGET, WARNING, CRITICAL };
  Status get_status() const {
    double deviation = std::abs(current_value - target_value) / target_value * 100;
    if (deviation >= critical_threshold) return Status::CRITICAL;
    if (deviation >= warning_threshold) return Status::WARNING;
    return Status::ON_TARGET;
  }
};

/**
 * @brief Performance report data
 */
struct PerformanceReport {
  std::string title;
  std::chrono::system_clock::time_point generated_at;
  std::chrono::system_clock::time_point period_start;
  std::chrono::system_clock::time_point period_end;

  struct MetricSummary {
    std::string name;
    double min, max, avg, p95, p99;
    size_t sample_count;
  };
  std::vector<MetricSummary> metrics;
  std::vector<KPI> kpis;
  std::vector<std::string> recommendations;
};

/**
 * @brief Performance reporting engine (Task 11.1)
 */
class SOLAR_CORE_API ReportingEngine {
 public:
  static ReportingEngine& instance();

  // KPI management
  void define_kpi(const KPI& kpi);
  void update_kpi(const std::string& name, double value);
  std::vector<KPI> get_kpis() const;
  std::vector<KPI> get_kpis_by_status(KPI::Status status) const;

  // Report generation
  PerformanceReport generate_report(const std::string& title,
                                    std::chrono::system_clock::time_point start,
                                    std::chrono::system_clock::time_point end) const;
  std::string render_report_text(const PerformanceReport& report) const;
  std::string render_report_json(const PerformanceReport& report) const;

  // Metric recording for reports
  void record_metric(const std::string& name, double value);

  void clear();

 private:
  ReportingEngine() = default;

  mutable std::mutex mutex_;
  std::map<std::string, KPI> kpis_;
  std::map<std::string, std::vector<std::pair<std::chrono::system_clock::time_point, double>>>
      metrics_;
  static constexpr size_t MAX_SAMPLES = 10000;
};

/**
 * @brief Forecast result
 */
struct Forecast {
  std::string metric_name;
  double current_value;
  double predicted_value;
  double confidence;  // 0-1
  std::chrono::system_clock::time_point prediction_time;
  std::string trend;  // "increasing", "decreasing", "stable"
};

/**
 * @brief Capacity recommendation
 */
struct CapacityRecommendation {
  std::string resource;
  double current_usage_percent;
  double predicted_usage_percent;
  std::chrono::system_clock::time_point exhaustion_time;  // When resource may be exhausted
  std::string recommendation;
  enum class Urgency { LOW, MEDIUM, HIGH, CRITICAL };
  Urgency urgency;
};

/**
 * @brief Predictive analytics engine (Task 11.2)
 */
class SOLAR_CORE_API PredictiveAnalytics {
 public:
  static PredictiveAnalytics& instance();

  // Data recording
  void record_sample(const std::string& metric, double value);

  // Forecasting
  Forecast forecast(const std::string& metric, std::chrono::seconds horizon) const;
  std::vector<Forecast> forecast_all(std::chrono::seconds horizon) const;

  // Capacity planning
  CapacityRecommendation analyze_capacity(const std::string& resource, double current_usage,
                                          double growth_rate) const;
  std::vector<CapacityRecommendation> get_capacity_recommendations() const;

  // Proactive detection
  struct PotentialIssue {
    std::string metric;
    std::string description;
    double probability;  // 0-1
    std::chrono::system_clock::time_point estimated_occurrence;
  };
  std::vector<PotentialIssue> detect_potential_issues() const;

  void clear();

 private:
  PredictiveAnalytics() = default;
  double calculate_trend(const std::vector<double>& values) const;
  double calculate_growth_rate(const std::vector<double>& values) const;

  mutable std::mutex mutex_;
  std::map<std::string, std::vector<std::pair<std::chrono::system_clock::time_point, double>>>
      samples_;
  static constexpr size_t MAX_SAMPLES = 1000;
  static constexpr size_t MIN_SAMPLES_FOR_FORECAST = 10;
};

}  // namespace SolarSystem::Performance
