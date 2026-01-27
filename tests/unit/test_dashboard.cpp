/**
 * @file test_dashboard.cpp
 * @brief Unit tests for performance dashboard
 */

#include <gtest/gtest.h>

#include "solar_core/performance/dashboard.hpp"

using namespace SolarSystem::Performance;

class DashboardTest : public ::testing::Test {
 protected:
  void SetUp() override {
    JPLMonitor::instance().reset();
    SimulationMonitor::instance().reset();
    CacheMonitor::instance().reset();
    WebMonitor::instance().reset();
    PerformanceMonitor::instance().reset_all();
  }
};

// RateLimiter tests
TEST_F(DashboardTest, RateLimiterAllowsWithinLimit) {
  RateLimiter limiter(5);
  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(limiter.allow_request("client1"));
  }
}

TEST_F(DashboardTest, RateLimiterBlocksOverLimit) {
  RateLimiter limiter(3);
  EXPECT_TRUE(limiter.allow_request("client1"));
  EXPECT_TRUE(limiter.allow_request("client1"));
  EXPECT_TRUE(limiter.allow_request("client1"));
  EXPECT_FALSE(limiter.allow_request("client1"));
}

TEST_F(DashboardTest, RateLimiterTracksClientsIndependently) {
  RateLimiter limiter(2);
  EXPECT_TRUE(limiter.allow_request("client1"));
  EXPECT_TRUE(limiter.allow_request("client1"));
  EXPECT_FALSE(limiter.allow_request("client1"));
  EXPECT_TRUE(limiter.allow_request("client2"));
}

// MetricsAPI tests
TEST_F(DashboardTest, MetricsAPIAuthDisabled) {
  DashboardConfig config;
  config.enable_auth = false;
  MetricsAPI api(config);
  EXPECT_TRUE(api.authenticate("any"));
}

TEST_F(DashboardTest, MetricsAPIAuthEnabled) {
  DashboardConfig config;
  config.enable_auth = true;
  config.api_key = "secret";
  MetricsAPI api(config);
  EXPECT_TRUE(api.authenticate("secret"));
  EXPECT_FALSE(api.authenticate("wrong"));
}

TEST_F(DashboardTest, MetricsAPIReturnsSnapshot) {
  JPLMonitor::instance().record_api_request(0.5, true);

  DashboardConfig config;
  MetricsAPI api(config);
  auto snapshot = api.get_current_metrics();

  EXPECT_EQ(snapshot.jpl_stats.total_requests, 1);
}

TEST_F(DashboardTest, MetricsAPIGeneratesJSON) {
  DashboardConfig config;
  MetricsAPI api(config);
  std::string json = api.get_metrics_json();

  EXPECT_NE(json.find("timestamp"), std::string::npos);
  EXPECT_NE(json.find("jpl"), std::string::npos);
}

TEST_F(DashboardTest, MetricsAPIQueryComponent) {
  JPLMonitor::instance().record_api_request(0.1, true);

  DashboardConfig config;
  MetricsAPI api(config);
  std::string json = api.get_component_metrics_json("jpl");

  EXPECT_NE(json.find("total_requests"), std::string::npos);
}

// DashboardServer tests
TEST_F(DashboardTest, ServerStartStop) {
  DashboardConfig config;
  DashboardServer server(config);

  EXPECT_FALSE(server.is_running());
  EXPECT_TRUE(server.start());
  EXPECT_TRUE(server.is_running());
  server.stop();
  EXPECT_FALSE(server.is_running());
}

TEST_F(DashboardTest, ServerGetsDashboardData) {
  DashboardConfig config;
  DashboardServer server(config);
  server.start();

  auto data = server.get_dashboard_data();
  EXPECT_FALSE(data.series.empty());
}

// DashboardRenderer tests
TEST_F(DashboardTest, RendererChartData) {
  DashboardData data;
  DashboardData::TimeSeries ts;
  ts.name = "test";
  ts.points.emplace_back(std::chrono::system_clock::now(), 42.0);
  data.series.push_back(ts);

  std::string json = DashboardRenderer::render_chart_data(data);
  EXPECT_NE(json.find("test"), std::string::npos);
}

TEST_F(DashboardTest, RendererAlertsPanel) {
  std::vector<PerformanceAlert> alerts;
  PerformanceAlert alert;
  alert.metric_name = "test_metric";
  alert.severity = PerformanceAlert::Severity::WARNING;
  alert.message = "Test";
  alerts.push_back(alert);

  std::string json = DashboardRenderer::render_alerts_panel(alerts);
  EXPECT_NE(json.find("test_metric"), std::string::npos);
  EXPECT_NE(json.find("warning"), std::string::npos);
}

TEST_F(DashboardTest, RendererSummaryCards) {
  MetricsSnapshot snapshot{};
  std::string json = DashboardRenderer::render_summary_cards(snapshot);
  EXPECT_NE(json.find("cards"), std::string::npos);
  EXPECT_NE(json.find("JPL API"), std::string::npos);
}
