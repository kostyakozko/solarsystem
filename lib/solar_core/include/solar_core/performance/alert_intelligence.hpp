/**
 * @file alert_intelligence.hpp
 * @brief Intelligent alert management - aggregation, correlation, escalation
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
#include "solar_core/performance/performance_monitor.hpp"

namespace SolarSystem::Performance {

/**
 * @brief Alert state for tracking acknowledgment and resolution
 */
enum class AlertState { ACTIVE, ACKNOWLEDGED, RESOLVED };

/**
 * @brief Extended alert with tracking information
 */
struct TrackedAlert {
  PerformanceAlert alert;
  AlertState state = AlertState::ACTIVE;
  std::chrono::system_clock::time_point first_seen;
  std::chrono::system_clock::time_point last_seen;
  size_t occurrence_count = 1;
  std::string acknowledged_by;
  std::string resolution_notes;
  bool escalated = false;
  PerformanceAlert::Severity original_severity = PerformanceAlert::Severity::INFO;
};

/**
 * @brief Alert aggregation configuration
 */
struct AggregationConfig {
  std::chrono::seconds window = std::chrono::seconds(60);
  size_t max_alerts_per_metric = 5;
  bool aggregate_by_severity = true;
};

/**
 * @brief Alert escalation rule
 */
struct EscalationRule {
  std::chrono::seconds duration_threshold;
  size_t occurrence_threshold = 1;
  PerformanceAlert::Severity from_severity;
  PerformanceAlert::Severity to_severity;
};

/**
 * @brief Intelligent alert manager with aggregation, correlation, and escalation
 */
class SOLAR_CORE_API AlertIntelligence {
 public:
  static AlertIntelligence& instance();

  // Alert processing
  /**
   * @brief Process an incoming alert with intelligence
   * @return true if alert should be forwarded to notification channels
   */
  bool process_alert(const PerformanceAlert& alert);

  // Aggregation
  void set_aggregation_config(const AggregationConfig& config);
  const AggregationConfig& aggregation_config() const { return aggregation_config_; }

  // Escalation
  void add_escalation_rule(const EscalationRule& rule);
  void clear_escalation_rules();

  // Alert tracking
  std::vector<TrackedAlert> get_active_alerts() const;
  std::vector<TrackedAlert> get_all_alerts() const;

  /**
   * @brief Acknowledge an alert
   */
  bool acknowledge_alert(const std::string& metric_name, const std::string& acknowledged_by);

  /**
   * @brief Resolve an alert
   */
  bool resolve_alert(const std::string& metric_name, const std::string& resolution_notes);

  // Correlation
  /**
   * @brief Find alerts that may be related to the given metric
   */
  std::vector<TrackedAlert> find_correlated_alerts(const std::string& metric_name) const;

  // Maintenance
  void clear_resolved_alerts();
  void clear_all();

  // Statistics
  size_t suppressed_count() const { return suppressed_count_; }
  size_t escalated_count() const { return escalated_count_; }

 private:
  AlertIntelligence() = default;

  bool should_suppress(const PerformanceAlert& alert);
  void check_escalation(TrackedAlert& tracked);
  bool are_correlated(const std::string& metric1, const std::string& metric2) const;

  mutable std::mutex mutex_;
  std::map<std::string, TrackedAlert> alerts_;
  AggregationConfig aggregation_config_;
  std::vector<EscalationRule> escalation_rules_;
  size_t suppressed_count_ = 0;
  size_t escalated_count_ = 0;
};

}  // namespace SolarSystem::Performance
