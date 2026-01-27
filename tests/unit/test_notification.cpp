/**
 * @file test_notification.cpp
 * @brief Tests for performance notification system
 */

#include <gtest/gtest.h>

#include <atomic>
#include <fstream>

#include "solar_core/performance/notification.hpp"

using namespace SolarSystem::Performance;

class NotificationTest : public ::testing::Test {
 protected:
  void SetUp() override { NotificationManager::instance().clear_channels(); }

  void TearDown() override { NotificationManager::instance().clear_channels(); }

  PerformanceAlert create_test_alert() {
    PerformanceAlert alert;
    alert.metric_name = "test_metric";
    alert.severity = PerformanceAlert::Severity::WARNING;
    alert.current_value = 150.0;
    alert.threshold_value = 100.0;
    alert.message = "Test alert message";
    alert.timestamp = std::chrono::system_clock::now();
    return alert;
  }
};

TEST_F(NotificationTest, ConsoleChannelSendsAlert) {
  auto channel = std::make_shared<ConsoleNotificationChannel>();
  EXPECT_TRUE(channel->is_enabled());
  EXPECT_EQ(channel->name(), "console");

  auto alert = create_test_alert();
  EXPECT_TRUE(channel->send(alert));
}

TEST_F(NotificationTest, ConsoleChannelDisabled) {
  auto channel = std::make_shared<ConsoleNotificationChannel>();
  channel->set_enabled(false);
  EXPECT_FALSE(channel->is_enabled());

  auto alert = create_test_alert();
  EXPECT_FALSE(channel->send(alert));
}

TEST_F(NotificationTest, CallbackChannelInvokesCallback) {
  std::atomic<int> call_count{0};
  std::string received_metric;

  auto callback = [&](const PerformanceAlert& alert) {
    ++call_count;
    received_metric = alert.metric_name;
  };

  auto channel = std::make_shared<CallbackNotificationChannel>(callback, "test_callback");
  EXPECT_EQ(channel->name(), "test_callback");

  auto alert = create_test_alert();
  EXPECT_TRUE(channel->send(alert));
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(received_metric, "test_metric");
}

TEST_F(NotificationTest, FileChannelWritesToFile) {
  std::string test_file = "/tmp/test_perf_alerts.log";
  std::remove(test_file.c_str());

  auto channel = std::make_shared<FileNotificationChannel>(test_file);
  EXPECT_EQ(channel->name(), "file");

  auto alert = create_test_alert();
  EXPECT_TRUE(channel->send(alert));

  std::ifstream file(test_file);
  EXPECT_TRUE(file.is_open());

  std::string content;
  std::getline(file, content);
  EXPECT_TRUE(content.find("test_metric") != std::string::npos);
  EXPECT_TRUE(content.find("WARNING") != std::string::npos);

  std::remove(test_file.c_str());
}

TEST_F(NotificationTest, ManagerRegistersChannels) {
  auto& manager = NotificationManager::instance();

  auto console = std::make_shared<ConsoleNotificationChannel>();
  manager.register_channel(console);

  auto channels = manager.get_channels();
  EXPECT_EQ(channels.size(), 1);

  auto retrieved = manager.get_channel("console");
  EXPECT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->name(), "console");
}

TEST_F(NotificationTest, ManagerRemovesChannels) {
  auto& manager = NotificationManager::instance();

  manager.register_channel(std::make_shared<ConsoleNotificationChannel>());
  EXPECT_EQ(manager.get_channels().size(), 1);

  manager.remove_channel("console");
  EXPECT_EQ(manager.get_channels().size(), 0);
}

TEST_F(NotificationTest, ManagerNotifiesAllChannels) {
  auto& manager = NotificationManager::instance();

  std::atomic<int> callback1_count{0};
  std::atomic<int> callback2_count{0};

  manager.register_channel(std::make_shared<CallbackNotificationChannel>(
      [&](const PerformanceAlert&) { ++callback1_count; }, "cb1"));

  manager.register_channel(std::make_shared<CallbackNotificationChannel>(
      [&](const PerformanceAlert&) { ++callback2_count; }, "cb2"));

  auto alert = create_test_alert();
  size_t sent = manager.notify(alert);

  EXPECT_EQ(sent, 2);
  EXPECT_EQ(callback1_count, 1);
  EXPECT_EQ(callback2_count, 1);
}

TEST_F(NotificationTest, ManagerSkipsDisabledChannels) {
  auto& manager = NotificationManager::instance();

  std::atomic<int> call_count{0};
  auto channel = std::make_shared<CallbackNotificationChannel>(
      [&](const PerformanceAlert&) { ++call_count; }, "test");

  channel->set_enabled(false);
  manager.register_channel(channel);

  auto alert = create_test_alert();
  size_t sent = manager.notify(alert);

  EXPECT_EQ(sent, 0);
  EXPECT_EQ(call_count, 0);
}

TEST_F(NotificationTest, WebhookChannelFormatsPayload) {
  WebhookConfig config;
  config.url = "";  // Empty URL to prevent actual HTTP call
  config.timeout_seconds = 1;

  auto channel = std::make_shared<WebhookNotificationChannel>(config);
  EXPECT_EQ(channel->name(), "webhook");

  // With empty URL, send should return false
  auto alert = create_test_alert();
  EXPECT_FALSE(channel->send(alert));
}
