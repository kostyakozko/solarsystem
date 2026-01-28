/**
 * @file test_monitoring_integration.cpp
 * @brief Unit tests for monitoring integration
 */

#include <gtest/gtest.h>

#include "solar_core/performance/monitoring_integration.hpp"

using namespace SolarSystem::Performance;

class MonitoringIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override { CorrelationTracker::instance().clear(); }
};

// ApplicationMonitor tests
TEST_F(MonitoringIntegrationTest, AppMonitorStartStop) {
  ApplicationMonitor monitor(ApplicationType::SIMULATION, "test_sim");
  EXPECT_FALSE(monitor.is_running());

  monitor.start();
  EXPECT_TRUE(monitor.is_running());

  monitor.stop();
  EXPECT_FALSE(monitor.is_running());
}

TEST_F(MonitoringIntegrationTest, AppMonitorRecordsStats) {
  ApplicationMonitor monitor(ApplicationType::WEB, "test_web");
  monitor.start();
  monitor.record_startup_time(1.5);
  monitor.record_operation("render", 16.0);
  monitor.record_operation("render", 18.0);
  monitor.record_error("timeout");

  auto stats = monitor.get_stats();
  EXPECT_EQ(stats.type, ApplicationType::WEB);
  EXPECT_EQ(stats.startup_time_s, 1.5);
  EXPECT_EQ(stats.total_operations, 2);
  EXPECT_EQ(stats.error_count, 1);
  EXPECT_EQ(stats.avg_operation_time_ms, 17.0);
}

TEST_F(MonitoringIntegrationTest, AppMonitorCorrelationId) {
  ApplicationMonitor monitor(ApplicationType::FETCH, "test_fetch");
  monitor.set_correlation_id("corr-123");
  EXPECT_EQ(monitor.get_correlation_id(), "corr-123");
}

// SystemMonitor tests
TEST_F(MonitoringIntegrationTest, SystemMonitorGetsResources) {
  auto res = SystemMonitor::instance().get_current_resources();
  // Memory should be non-zero on any system
  EXPECT_GT(res.memory_used_bytes, 0);
}

TEST_F(MonitoringIntegrationTest, SystemMonitorRecordsSamples) {
  SystemMonitor::instance().record_sample();
  SystemMonitor::instance().record_sample();

  auto history = SystemMonitor::instance().get_history(10);
  EXPECT_GE(history.size(), 2);
}

TEST_F(MonitoringIntegrationTest, SystemMonitorThresholds) {
  SystemMonitor::instance().set_memory_threshold(0.1);  // Very low threshold
  auto alerts = SystemMonitor::instance().check_thresholds();

  // Should trigger memory alert since usage > 0.1%
  bool found_memory = false;
  for (const auto& a : alerts) {
    if (a.resource == "memory") found_memory = true;
  }
  EXPECT_TRUE(found_memory);

  // Reset threshold
  SystemMonitor::instance().set_memory_threshold(90.0);
}

// CorrelationTracker tests
TEST_F(MonitoringIntegrationTest, CorrelationTrackerGeneratesIds) {
  auto id1 = CorrelationTracker::instance().generate_correlation_id();
  auto id2 = CorrelationTracker::instance().generate_correlation_id();
  EXPECT_NE(id1, id2);
  EXPECT_FALSE(id1.empty());
}

TEST_F(MonitoringIntegrationTest, CorrelationTrackerRegistersApps) {
  auto id = CorrelationTracker::instance().generate_correlation_id();
  CorrelationTracker::instance().register_application(id, ApplicationType::LAUNCHER);
  CorrelationTracker::instance().register_application(id, ApplicationType::FETCH);

  auto correlations = CorrelationTracker::instance().get_recent_correlations(10);
  EXPECT_EQ(correlations.size(), 1);
  EXPECT_EQ(correlations[0].app_chain.size(), 2);
}

TEST_F(MonitoringIntegrationTest, CorrelationTrackerRecordsHandoff) {
  auto id = CorrelationTracker::instance().generate_correlation_id();
  CorrelationTracker::instance().register_application(id, ApplicationType::LAUNCHER);
  CorrelationTracker::instance().record_handoff(id, ApplicationType::LAUNCHER,
                                                ApplicationType::SIMULATION);

  auto correlations = CorrelationTracker::instance().get_recent_correlations(10);
  EXPECT_EQ(correlations[0].app_chain.size(), 2);
  EXPECT_EQ(correlations[0].app_chain[1], ApplicationType::SIMULATION);
}
