/**
 * @file alert_intelligence.cpp
 * @brief Intelligent alert management implementation
 */

#include "solar_core/performance/alert_intelligence.hpp"

#include <algorithm>

namespace SolarSystem::Performance {

AlertIntelligence& AlertIntelligence::instance() {
  static AlertIntelligence instance;
  return instance;
}

bool AlertIntelligence::process_alert(const PerformanceAlert& alert) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto now = std::chrono::system_clock::now();
  auto it = alerts_.find(alert.metric_name);

  if (it != alerts_.end()) {
    // Existing alert - update tracking
    auto& tracked = it->second;
    tracked.last_seen = now;
    tracked.occurrence_count++;
    tracked.alert = alert;
    tracked.alert.severity = tracked.escalated ? tracked.alert.severity : alert.severity;

    // Check if should suppress
    if (should_suppress(alert)) {
      ++suppressed_count_;
      return false;
    }

    // Check escalation
    check_escalation(tracked);
  } else {
    // New alert
    TrackedAlert tracked;
    tracked.alert = alert;
    tracked.original_severity = alert.severity;
    tracked.first_seen = now;
    tracked.last_seen = now;
    tracked.state = AlertState::ACTIVE;
    alerts_[alert.metric_name] = tracked;
  }

  return true;
}

void AlertIntelligence::set_aggregation_config(const AggregationConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  aggregation_config_ = config;
}

void AlertIntelligence::add_escalation_rule(const EscalationRule& rule) {
  std::lock_guard<std::mutex> lock(mutex_);
  escalation_rules_.push_back(rule);
}

void AlertIntelligence::clear_escalation_rules() {
  std::lock_guard<std::mutex> lock(mutex_);
  escalation_rules_.clear();
}

std::vector<TrackedAlert> AlertIntelligence::get_active_alerts() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<TrackedAlert> result;
  for (const auto& [name, tracked] : alerts_) {
    if (tracked.state == AlertState::ACTIVE) {
      result.push_back(tracked);
    }
  }
  return result;
}

std::vector<TrackedAlert> AlertIntelligence::get_all_alerts() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<TrackedAlert> result;
  for (const auto& [name, tracked] : alerts_) {
    result.push_back(tracked);
  }
  return result;
}

bool AlertIntelligence::acknowledge_alert(const std::string& metric_name,
                                          const std::string& acknowledged_by) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = alerts_.find(metric_name);
  if (it == alerts_.end()) return false;

  it->second.state = AlertState::ACKNOWLEDGED;
  it->second.acknowledged_by = acknowledged_by;
  return true;
}

bool AlertIntelligence::resolve_alert(const std::string& metric_name,
                                      const std::string& resolution_notes) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = alerts_.find(metric_name);
  if (it == alerts_.end()) return false;

  it->second.state = AlertState::RESOLVED;
  it->second.resolution_notes = resolution_notes;
  return true;
}

std::vector<TrackedAlert> AlertIntelligence::find_correlated_alerts(
    const std::string& metric_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<TrackedAlert> result;

  for (const auto& [name, tracked] : alerts_) {
    if (name != metric_name && are_correlated(metric_name, name)) {
      result.push_back(tracked);
    }
  }
  return result;
}

void AlertIntelligence::clear_resolved_alerts() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = alerts_.begin(); it != alerts_.end();) {
    if (it->second.state == AlertState::RESOLVED) {
      it = alerts_.erase(it);
    } else {
      ++it;
    }
  }
}

void AlertIntelligence::clear_all() {
  std::lock_guard<std::mutex> lock(mutex_);
  alerts_.clear();
  suppressed_count_ = 0;
  escalated_count_ = 0;
}

bool AlertIntelligence::should_suppress(const PerformanceAlert& alert) {
  auto it = alerts_.find(alert.metric_name);
  if (it == alerts_.end()) return false;

  const auto& tracked = it->second;
  auto now = std::chrono::system_clock::now();

  // Suppress if within aggregation window and over limit
  auto time_since_first =
      std::chrono::duration_cast<std::chrono::seconds>(now - tracked.first_seen);
  if (time_since_first < aggregation_config_.window) {
    // occurrence_count is already incremented, so check against limit
    if (tracked.occurrence_count > aggregation_config_.max_alerts_per_metric) {
      return true;
    }
  }

  return false;
}

void AlertIntelligence::check_escalation(TrackedAlert& tracked) {
  if (tracked.escalated) return;  // Already escalated

  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - tracked.first_seen);

  for (const auto& rule : escalation_rules_) {
    if (tracked.original_severity == rule.from_severity && duration >= rule.duration_threshold &&
        tracked.occurrence_count >= rule.occurrence_threshold) {
      tracked.alert.severity = rule.to_severity;
      tracked.escalated = true;
      ++escalated_count_;
      return;
    }
  }
}

bool AlertIntelligence::are_correlated(const std::string& metric1,
                                       const std::string& metric2) const {
  // Simple correlation: metrics with common prefix are related
  auto find_prefix = [](const std::string& s) {
    auto pos = s.find('_');
    return pos != std::string::npos ? s.substr(0, pos) : s;
  };

  return find_prefix(metric1) == find_prefix(metric2);
}

}  // namespace SolarSystem::Performance
