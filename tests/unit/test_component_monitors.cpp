/**
 * @file test_component_monitors.cpp
 * @brief Unit tests for component-specific performance monitors
 */

#include <gtest/gtest.h>

#include "solar_core/performance/component_monitors.hpp"

using namespace SolarSystem::Performance;

class ComponentMonitorsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    JPLMonitor::instance().reset();
    SimulationMonitor::instance().reset();
    CacheMonitor::instance().reset();
    WebMonitor::instance().reset();
  }
};

// JPLMonitor tests
TEST_F(ComponentMonitorsTest, JPLMonitorRecordsAPIRequests) {
  auto& monitor = JPLMonitor::instance();
  monitor.record_api_request(0.5, true);
  monitor.record_api_request(0.3, true);
  monitor.record_api_request(0.4, false);

  auto stats = monitor.get_stats();
  EXPECT_EQ(stats.total_requests, 3);
  EXPECT_EQ(stats.successful_requests, 2);
  EXPECT_GT(stats.avg_api_response_time, 0);
}

TEST_F(ComponentMonitorsTest, JPLMonitorTracksCacheHitRate) {
  auto& monitor = JPLMonitor::instance();
  monitor.record_cache_access(true);
  monitor.record_cache_access(true);
  monitor.record_cache_access(false);

  auto stats = monitor.get_stats();
  EXPECT_NEAR(stats.cache_hit_rate, 0.666, 0.01);
}

TEST_F(ComponentMonitorsTest, JPLMonitorTracksRetries) {
  auto& monitor = JPLMonitor::instance();
  monitor.record_retry();
  monitor.record_retry();

  auto stats = monitor.get_stats();
  EXPECT_EQ(stats.retry_count, 2);
}

// SimulationMonitor tests
TEST_F(ComponentMonitorsTest, SimulationMonitorRecordsTimesteps) {
  auto& monitor = SimulationMonitor::instance();
  monitor.record_timestep(0.001);
  monitor.record_timestep(0.002);

  auto stats = monitor.get_stats();
  EXPECT_EQ(stats.total_timesteps, 2);
  EXPECT_GT(stats.avg_timestep_duration, 0);
}

TEST_F(ComponentMonitorsTest, SimulationMonitorTracksPeakMemory) {
  auto& monitor = SimulationMonitor::instance();
  monitor.record_memory_usage(1000);
  monitor.record_memory_usage(5000);
  monitor.record_memory_usage(3000);

  auto stats = monitor.get_stats();
  EXPECT_EQ(stats.peak_memory_bytes, 5000);
  EXPECT_EQ(stats.current_memory_bytes, 3000);
}

// CacheMonitor tests
TEST_F(ComponentMonitorsTest, CacheMonitorRecordsReadWrite) {
  auto& monitor = CacheMonitor::instance();
  monitor.record_read(0.01, 1024);
  monitor.record_write(0.02, 2048);

  auto stats = monitor.get_stats();
  EXPECT_EQ(stats.total_reads, 1);
  EXPECT_EQ(stats.total_writes, 1);
}

TEST_F(ComponentMonitorsTest, CacheMonitorTracksCompressionRatio) {
  auto& monitor = CacheMonitor::instance();
  monitor.record_compression(1000, 500);
  monitor.record_compression(2000, 1000);

  auto stats = monitor.get_stats();
  EXPECT_NEAR(stats.compression_ratio, 0.5, 0.01);
}

TEST_F(ComponentMonitorsTest, CacheMonitorTracksHitRate) {
  auto& monitor = CacheMonitor::instance();
  monitor.record_hit();
  monitor.record_hit();
  monitor.record_hit();
  monitor.record_miss();

  auto stats = monitor.get_stats();
  EXPECT_NEAR(stats.hit_rate, 0.75, 0.01);
}

// WebMonitor tests
TEST_F(ComponentMonitorsTest, WebMonitorRecordsHTTPRequests) {
  auto& monitor = WebMonitor::instance();
  monitor.record_http_request(0.1, 200);
  monitor.record_http_request(0.2, 404);
  monitor.record_http_request(0.15, 500);

  auto stats = monitor.get_stats();
  EXPECT_EQ(stats.total_requests, 3);
  EXPECT_EQ(stats.error_count, 2);
}

TEST_F(ComponentMonitorsTest, WebMonitorCalculatesFPS) {
  auto& monitor = WebMonitor::instance();
  // 60 FPS = 16.67ms per frame
  monitor.record_render_frame(0.01667);
  monitor.record_render_frame(0.01667);

  auto stats = monitor.get_stats();
  EXPECT_NEAR(stats.fps, 60.0, 1.0);
}

// Macro tests
TEST_F(ComponentMonitorsTest, MacrosWork) {
  JPL_MONITOR_API_REQUEST(0.5, true);
  JPL_MONITOR_CACHE_HIT();
  JPL_MONITOR_CACHE_MISS();

  SIM_MONITOR_TIMESTEP(0.001);
  SIM_MONITOR_MEMORY(4096);

  CACHE_MONITOR_READ(0.01, 1024);
  CACHE_MONITOR_WRITE(0.02, 2048);

  WEB_MONITOR_REQUEST(0.1, 200);
  WEB_MONITOR_FRAME(0.016);

  // Verify macros recorded data
  EXPECT_EQ(JPLMonitor::instance().get_stats().total_requests, 1);
  EXPECT_EQ(SimulationMonitor::instance().get_stats().total_timesteps, 1);
  EXPECT_EQ(CacheMonitor::instance().get_stats().total_reads, 1);
  EXPECT_EQ(WebMonitor::instance().get_stats().total_requests, 1);
}
