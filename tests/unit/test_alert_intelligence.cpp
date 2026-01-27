/**
 * @file test_alert_intelligence.cpp
 * @brief Tests for alert intelligence system
 */

#include <gtest/gtest.h>

#include "solar_core/performance/alert_intelligence.hpp"

using namespace SolarSystem::Performance;

class AlertIntelligenceTest : public ::testing::Test {
 protected:
  void SetUp() override { AlertIntelligence::instance().clear_all(); }

  void TearDown() override { AlertIntelligence::instance().clear_all(); }

  PerformanceAlert create_alert(const std::string& name, PerformanceAlert::Severity severity =
                                                             PerformanceAlert::Severity::WARNING) {
    PerformanceAlert alert;
    alert.metric_name = name;
    alert.severity = severity;
    alert.current_value = 100.0;
    alert.threshold_value = 50.0;
    alert.message = "Test alert";
    alert.timestamp = std::chrono::system_clock::now();
    return alert;
  }
};

TEST_F(AlertIntelligenceTest, ProcessNewAlert) {
  auto& ai = AlertIntelligence::instance();
  auto alert = create_alert("cpu_usage");

  EXPECT_TRUE(ai.process_alert(alert));
  EXPECT_EQ(ai.get_active_alerts().size(), 1);
}

TEST_F(AlertIntelligenceTest, AggregationSuppressesDuplicates) {
  auto& ai = AlertIntelligence::instance();

  AggregationConfig config;
  config.window = std::chrono::seconds(60);
  config.max_alerts_per_metric = 3;
  ai.set_aggregation_config(config);

  auto alert = create_alert("cpu_usage");

  // First 3 should pass
  EXPECT_TRUE(ai.process_alert(alert));
  EXPECT_TRUE(ai.process_alert(alert));
  EXPECT_TRUE(ai.process_alert(alert));

  // 4th and beyond should be suppressed
  EXPECT_FALSE(ai.process_alert(alert));
  EXPECT_FALSE(ai.process_alert(alert));

  EXPECT_EQ(ai.suppressed_count(), 2);
}

TEST_F(AlertIntelligenceTest, AcknowledgeAlert) {
  auto& ai = AlertIntelligence::instance();
  ai.process_alert(create_alert("memory_usage"));

  EXPECT_TRUE(ai.acknowledge_alert("memory_usage", "admin"));

  auto alerts = ai.get_all_alerts();
  EXPECT_EQ(alerts.size(), 1);
  EXPECT_EQ(alerts[0].state, AlertState::ACKNOWLEDGED);
  EXPECT_EQ(alerts[0].acknowledged_by, "admin");
}

TEST_F(AlertIntelligenceTest, ResolveAlert) {
  auto& ai = AlertIntelligence::instance();
  ai.process_alert(create_alert("disk_usage"));

  EXPECT_TRUE(ai.resolve_alert("disk_usage", "Increased disk space"));

  auto alerts = ai.get_all_alerts();
  EXPECT_EQ(alerts.size(), 1);
  EXPECT_EQ(alerts[0].state, AlertState::RESOLVED);
  EXPECT_EQ(alerts[0].resolution_notes, "Increased disk space");

  // Active alerts should be empty
  EXPECT_EQ(ai.get_active_alerts().size(), 0);
}

TEST_F(AlertIntelligenceTest, ClearResolvedAlerts) {
  auto& ai = AlertIntelligence::instance();

  ai.process_alert(create_alert("metric1"));
  ai.process_alert(create_alert("metric2"));
  ai.resolve_alert("metric1", "Fixed");

  EXPECT_EQ(ai.get_all_alerts().size(), 2);

  ai.clear_resolved_alerts();

  EXPECT_EQ(ai.get_all_alerts().size(), 1);
  EXPECT_EQ(ai.get_all_alerts()[0].alert.metric_name, "metric2");
}

TEST_F(AlertIntelligenceTest, FindCorrelatedAlerts) {
  auto& ai = AlertIntelligence::instance();

  ai.process_alert(create_alert("jpl_response_time"));
  ai.process_alert(create_alert("jpl_error_rate"));
  ai.process_alert(create_alert("cache_hit_rate"));

  auto correlated = ai.find_correlated_alerts("jpl_response_time");

  EXPECT_EQ(correlated.size(), 1);
  EXPECT_EQ(correlated[0].alert.metric_name, "jpl_error_rate");
}

TEST_F(AlertIntelligenceTest, EscalationRule) {
  auto& ai = AlertIntelligence::instance();

  EscalationRule rule;
  rule.from_severity = PerformanceAlert::Severity::WARNING;
  rule.to_severity = PerformanceAlert::Severity::CRITICAL;
  rule.duration_threshold = std::chrono::seconds(0);  // Immediate for testing
  rule.occurrence_threshold = 3;
  ai.add_escalation_rule(rule);

  auto alert = create_alert("test_metric", PerformanceAlert::Severity::WARNING);

  ai.process_alert(alert);
  ai.process_alert(alert);
  ai.process_alert(alert);  // Should trigger escalation

  auto alerts = ai.get_all_alerts();
  EXPECT_EQ(alerts.size(), 1);
  EXPECT_EQ(alerts[0].alert.severity, PerformanceAlert::Severity::CRITICAL);
  EXPECT_EQ(ai.escalated_count(), 1);
}

TEST_F(AlertIntelligenceTest, OccurrenceTracking) {
  auto& ai = AlertIntelligence::instance();

  // Disable suppression for this test
  AggregationConfig config;
  config.max_alerts_per_metric = 100;
  ai.set_aggregation_config(config);

  auto alert = create_alert("test_metric");
  ai.process_alert(alert);
  ai.process_alert(alert);
  ai.process_alert(alert);

  auto alerts = ai.get_all_alerts();
  EXPECT_EQ(alerts.size(), 1);
  EXPECT_EQ(alerts[0].occurrence_count, 3);
}
