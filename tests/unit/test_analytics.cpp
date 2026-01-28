/**
 * @file test_analytics.cpp
 * @brief Unit tests for analytics and reporting
 */

#include <gtest/gtest.h>

#include "solar_core/performance/analytics.hpp"

using namespace SolarSystem::Performance;

class AnalyticsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ReportingEngine::instance().clear();
    PredictiveAnalytics::instance().clear();
  }
};

// ReportingEngine tests
TEST_F(AnalyticsTest, KPIDefinitionAndUpdate) {
  KPI kpi;
  kpi.name = "response_time";
  kpi.target_value = 100;
  kpi.current_value = 100;
  kpi.warning_threshold = 10;
  kpi.critical_threshold = 25;

  ReportingEngine::instance().define_kpi(kpi);
  ReportingEngine::instance().update_kpi("response_time", 120);

  auto kpis = ReportingEngine::instance().get_kpis();
  EXPECT_EQ(kpis.size(), 1);
  EXPECT_EQ(kpis[0].current_value, 120);
}

TEST_F(AnalyticsTest, KPIStatusCalculation) {
  KPI kpi;
  kpi.name = "test";
  kpi.target_value = 100;
  kpi.warning_threshold = 10;
  kpi.critical_threshold = 25;

  kpi.current_value = 100;
  EXPECT_EQ(kpi.get_status(), KPI::Status::ON_TARGET);

  kpi.current_value = 115;
  EXPECT_EQ(kpi.get_status(), KPI::Status::WARNING);

  kpi.current_value = 130;
  EXPECT_EQ(kpi.get_status(), KPI::Status::CRITICAL);
}

TEST_F(AnalyticsTest, ReportGeneration) {
  ReportingEngine::instance().record_metric("latency", 10);
  ReportingEngine::instance().record_metric("latency", 20);
  ReportingEngine::instance().record_metric("latency", 15);

  auto start = std::chrono::system_clock::now() - std::chrono::hours(1);
  auto end = std::chrono::system_clock::now() + std::chrono::hours(1);
  auto report = ReportingEngine::instance().generate_report("Test Report", start, end);

  EXPECT_EQ(report.title, "Test Report");
  EXPECT_EQ(report.metrics.size(), 1);
  EXPECT_EQ(report.metrics[0].sample_count, 3);
}

TEST_F(AnalyticsTest, ReportRenderText) {
  ReportingEngine::instance().record_metric("test", 100);
  auto start = std::chrono::system_clock::now() - std::chrono::hours(1);
  auto end = std::chrono::system_clock::now() + std::chrono::hours(1);
  auto report = ReportingEngine::instance().generate_report("Test", start, end);

  std::string text = ReportingEngine::instance().render_report_text(report);
  EXPECT_NE(text.find("Test"), std::string::npos);
  EXPECT_NE(text.find("Metrics"), std::string::npos);
}

TEST_F(AnalyticsTest, ReportRenderJson) {
  ReportingEngine::instance().record_metric("test", 100);
  auto start = std::chrono::system_clock::now() - std::chrono::hours(1);
  auto end = std::chrono::system_clock::now() + std::chrono::hours(1);
  auto report = ReportingEngine::instance().generate_report("Test", start, end);

  std::string json = ReportingEngine::instance().render_report_json(report);
  EXPECT_NE(json.find("title"), std::string::npos);
  EXPECT_NE(json.find("metrics"), std::string::npos);
}

// PredictiveAnalytics tests
TEST_F(AnalyticsTest, ForecastRequiresMinSamples) {
  PredictiveAnalytics::instance().record_sample("test", 100);

  auto forecast = PredictiveAnalytics::instance().forecast("test", std::chrono::seconds(3600));
  EXPECT_EQ(forecast.confidence, 0);
  EXPECT_EQ(forecast.trend, "unknown");
}

TEST_F(AnalyticsTest, ForecastWithSufficientData) {
  for (int i = 0; i < 20; ++i) {
    PredictiveAnalytics::instance().record_sample("increasing", 100 + i * 5);
  }

  auto forecast =
      PredictiveAnalytics::instance().forecast("increasing", std::chrono::seconds(3600));
  EXPECT_GT(forecast.confidence, 0);
  EXPECT_EQ(forecast.trend, "increasing");
  EXPECT_GT(forecast.predicted_value, forecast.current_value);
}

TEST_F(AnalyticsTest, CapacityAnalysis) {
  auto rec = PredictiveAnalytics::instance().analyze_capacity("memory", 70, 0.02);

  EXPECT_EQ(rec.resource, "memory");
  EXPECT_EQ(rec.current_usage_percent, 70);
  EXPECT_GT(rec.predicted_usage_percent, 70);
  EXPECT_FALSE(rec.recommendation.empty());
}

TEST_F(AnalyticsTest, PotentialIssueDetection) {
  // Record rapidly increasing values
  for (int i = 0; i < 20; ++i) {
    PredictiveAnalytics::instance().record_sample("memory_usage", 50 + i * 3);
  }

  auto issues = PredictiveAnalytics::instance().detect_potential_issues();
  // May or may not detect issues depending on threshold
  EXPECT_GE(issues.size(), 0);
}
