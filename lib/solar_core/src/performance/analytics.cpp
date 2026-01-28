/**
 * @file analytics.cpp
 * @brief Advanced analytics and reporting implementation
 */

#include "solar_core/performance/analytics.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <numeric>
#include <sstream>

using json = nlohmann::json;

namespace SolarSystem::Performance {

// ReportingEngine implementation
ReportingEngine& ReportingEngine::instance() {
  static ReportingEngine instance;
  return instance;
}

void ReportingEngine::define_kpi(const KPI& kpi) {
  std::lock_guard<std::mutex> lock(mutex_);
  kpis_[kpi.name] = kpi;
}

void ReportingEngine::update_kpi(const std::string& name, double value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = kpis_.find(name);
  if (it != kpis_.end()) {
    it->second.current_value = value;
  }
}

std::vector<KPI> ReportingEngine::get_kpis() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<KPI> result;
  result.reserve(kpis_.size());
  for (const auto& [name, kpi] : kpis_) {
    result.push_back(kpi);
  }
  return result;
}

std::vector<KPI> ReportingEngine::get_kpis_by_status(KPI::Status status) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<KPI> result;
  for (const auto& [name, kpi] : kpis_) {
    if (kpi.get_status() == status) {
      result.push_back(kpi);
    }
  }
  return result;
}

void ReportingEngine::record_metric(const std::string& name, double value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto& samples = metrics_[name];
  if (samples.size() >= MAX_SAMPLES) {
    samples.erase(samples.begin());
  }
  samples.emplace_back(std::chrono::system_clock::now(), value);
}

PerformanceReport ReportingEngine::generate_report(
    const std::string& title, std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
  std::lock_guard<std::mutex> lock(mutex_);
  PerformanceReport report;
  report.title = title;
  report.generated_at = std::chrono::system_clock::now();
  report.period_start = start;
  report.period_end = end;

  // Generate metric summaries
  for (const auto& [name, samples] : metrics_) {
    std::vector<double> values;
    for (const auto& [ts, val] : samples) {
      if (ts >= start && ts <= end) {
        values.push_back(val);
      }
    }
    if (values.empty()) continue;

    std::sort(values.begin(), values.end());
    PerformanceReport::MetricSummary summary;
    summary.name = name;
    summary.sample_count = values.size();
    summary.min = values.front();
    summary.max = values.back();
    summary.avg =
        std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    summary.p95 = values[static_cast<size_t>(static_cast<double>(values.size()) * 0.95)];
    summary.p99 = values[static_cast<size_t>(static_cast<double>(values.size()) * 0.99)];
    report.metrics.push_back(summary);
  }

  // Add KPIs
  for (const auto& [name, kpi] : kpis_) {
    report.kpis.push_back(kpi);
  }

  // Generate recommendations
  for (const auto& kpi : report.kpis) {
    if (kpi.get_status() == KPI::Status::CRITICAL) {
      report.recommendations.push_back("CRITICAL: " + kpi.name + " requires immediate attention");
    } else if (kpi.get_status() == KPI::Status::WARNING) {
      report.recommendations.push_back("WARNING: Monitor " + kpi.name + " closely");
    }
  }

  return report;
}

std::string ReportingEngine::render_report_text(const PerformanceReport& report) const {
  std::ostringstream oss;
  oss << "=== " << report.title << " ===\n";
  oss << "Generated: " << std::chrono::system_clock::to_time_t(report.generated_at) << "\n\n";

  oss << "-- Metrics --\n";
  for (const auto& m : report.metrics) {
    oss << m.name << ": avg=" << std::fixed << std::setprecision(2) << m.avg << ", p95=" << m.p95
        << ", p99=" << m.p99 << " (" << m.sample_count << " samples)\n";
  }

  oss << "\n-- KPIs --\n";
  for (const auto& kpi : report.kpis) {
    const char* status = kpi.get_status() == KPI::Status::CRITICAL
                             ? "CRITICAL"
                             : (kpi.get_status() == KPI::Status::WARNING ? "WARNING" : "OK");
    oss << kpi.name << ": " << kpi.current_value << " (target: " << kpi.target_value << ") ["
        << status << "]\n";
  }

  if (!report.recommendations.empty()) {
    oss << "\n-- Recommendations --\n";
    for (const auto& rec : report.recommendations) {
      oss << "* " << rec << "\n";
    }
  }

  return oss.str();
}

std::string ReportingEngine::render_report_json(const PerformanceReport& report) const {
  json j;
  j["title"] = report.title;
  j["generated_at"] = std::chrono::system_clock::to_time_t(report.generated_at);

  j["metrics"] = json::array();
  for (const auto& m : report.metrics) {
    j["metrics"].push_back({{"name", m.name},
                            {"avg", m.avg},
                            {"p95", m.p95},
                            {"p99", m.p99},
                            {"count", m.sample_count}});
  }

  j["kpis"] = json::array();
  for (const auto& kpi : report.kpis) {
    j["kpis"].push_back({{"name", kpi.name},
                         {"current", kpi.current_value},
                         {"target", kpi.target_value},
                         {"status", static_cast<int>(kpi.get_status())}});
  }

  j["recommendations"] = report.recommendations;
  return j.dump();
}

void ReportingEngine::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  kpis_.clear();
  metrics_.clear();
}

// PredictiveAnalytics implementation
PredictiveAnalytics& PredictiveAnalytics::instance() {
  static PredictiveAnalytics instance;
  return instance;
}

void PredictiveAnalytics::record_sample(const std::string& metric, double value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto& samples = samples_[metric];
  if (samples.size() >= MAX_SAMPLES) {
    samples.erase(samples.begin());
  }
  samples.emplace_back(std::chrono::system_clock::now(), value);
}

double PredictiveAnalytics::calculate_trend(const std::vector<double>& values) const {
  if (values.size() < 2) return 0;
  auto n = static_cast<double>(values.size());
  double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    auto di = static_cast<double>(i);
    sum_x += di;
    sum_y += values[i];
    sum_xy += di * values[i];
    sum_xx += di * di;
  }
  double denom = n * sum_xx - sum_x * sum_x;
  return denom != 0 ? (n * sum_xy - sum_x * sum_y) / denom : 0;
}

double PredictiveAnalytics::calculate_growth_rate(const std::vector<double>& values) const {
  if (values.size() < 2 || values.front() == 0) return 0;
  return (values.back() - values.front()) / values.front() / static_cast<double>(values.size());
}

Forecast PredictiveAnalytics::forecast(const std::string& metric,
                                       std::chrono::seconds horizon) const {
  std::lock_guard<std::mutex> lock(mutex_);
  Forecast result;
  result.metric_name = metric;
  result.prediction_time = std::chrono::system_clock::now() + horizon;

  auto it = samples_.find(metric);
  if (it == samples_.end() || it->second.size() < MIN_SAMPLES_FOR_FORECAST) {
    result.confidence = 0;
    result.trend = "unknown";
    return result;
  }

  std::vector<double> values;
  for (const auto& [ts, val] : it->second) {
    values.push_back(val);
  }

  result.current_value = values.back();
  double trend = calculate_trend(values);
  double steps = static_cast<double>(horizon.count()) / 60.0;  // Assume 1-minute sample intervals
  result.predicted_value = result.current_value + trend * steps;
  result.confidence = std::min(1.0, static_cast<double>(values.size()) / 100.0);

  if (trend > 0.01)
    result.trend = "increasing";
  else if (trend < -0.01)
    result.trend = "decreasing";
  else
    result.trend = "stable";

  return result;
}

std::vector<Forecast> PredictiveAnalytics::forecast_all(std::chrono::seconds horizon) const {
  std::vector<std::string> metrics;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, samples] : samples_) {
      metrics.push_back(name);
    }
  }

  std::vector<Forecast> results;
  for (const auto& m : metrics) {
    results.push_back(forecast(m, horizon));
  }
  return results;
}

CapacityRecommendation PredictiveAnalytics::analyze_capacity(const std::string& resource,
                                                             double current_usage,
                                                             double growth_rate) const {
  CapacityRecommendation rec;
  rec.resource = resource;
  rec.current_usage_percent = current_usage;

  // Project 30 days ahead
  rec.predicted_usage_percent = current_usage * (1 + growth_rate * 30);

  if (growth_rate > 0 && current_usage > 0) {
    double days_to_full = (100 - current_usage) / (current_usage * growth_rate);
    rec.exhaustion_time =
        std::chrono::system_clock::now() + std::chrono::hours(static_cast<int>(days_to_full * 24));
  } else {
    rec.exhaustion_time = std::chrono::system_clock::time_point::max();
  }

  if (rec.predicted_usage_percent > 95) {
    rec.urgency = CapacityRecommendation::Urgency::CRITICAL;
    rec.recommendation = "Immediate capacity expansion required";
  } else if (rec.predicted_usage_percent > 80) {
    rec.urgency = CapacityRecommendation::Urgency::HIGH;
    rec.recommendation = "Plan capacity expansion within 2 weeks";
  } else if (rec.predicted_usage_percent > 60) {
    rec.urgency = CapacityRecommendation::Urgency::MEDIUM;
    rec.recommendation = "Monitor and plan for future expansion";
  } else {
    rec.urgency = CapacityRecommendation::Urgency::LOW;
    rec.recommendation = "Capacity is adequate";
  }

  return rec;
}

std::vector<CapacityRecommendation> PredictiveAnalytics::get_capacity_recommendations() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<CapacityRecommendation> results;

  for (const auto& [metric, samples] : samples_) {
    if (samples.size() < MIN_SAMPLES_FOR_FORECAST) continue;

    std::vector<double> values;
    for (const auto& [ts, val] : samples) {
      values.push_back(val);
    }

    double growth = calculate_growth_rate(values);
    if (values.back() > 50) {  // Only for metrics that look like percentages
      results.push_back(analyze_capacity(metric, values.back(), growth));
    }
  }

  return results;
}

std::vector<PredictiveAnalytics::PotentialIssue> PredictiveAnalytics::detect_potential_issues()
    const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<PotentialIssue> issues;

  for (const auto& [metric, samples] : samples_) {
    if (samples.size() < MIN_SAMPLES_FOR_FORECAST) continue;

    std::vector<double> values;
    for (const auto& [ts, val] : samples) {
      values.push_back(val);
    }

    double trend = calculate_trend(values);
    double avg =
        std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    double recent_avg =
        values.size() > 10 ? std::accumulate(values.end() - 10, values.end(), 0.0) / 10.0 : avg;

    // Detect rapid increase
    if (trend > 0.1 && recent_avg > avg * 1.2) {
      PotentialIssue issue;
      issue.metric = metric;
      issue.description = "Rapid increase detected - may indicate resource exhaustion";
      issue.probability = std::min(0.9, trend * 5);
      issue.estimated_occurrence = std::chrono::system_clock::now() + std::chrono::hours(24);
      issues.push_back(issue);
    }
  }

  return issues;
}

void PredictiveAnalytics::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  samples_.clear();
}

}  // namespace SolarSystem::Performance
