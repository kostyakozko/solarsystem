/**
 * @file notification.hpp
 * @brief Performance alert notification system
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "solar_core/export.hpp"
#include "solar_core/performance/performance_monitor.hpp"

namespace SolarSystem::Performance {

/**
 * @brief Notification channel interface
 */
class SOLAR_CORE_API NotificationChannel {
 public:
  virtual ~NotificationChannel() = default;

  /**
   * @brief Send a notification for an alert
   * @param alert The performance alert to notify about
   * @return true if notification was sent successfully
   */
  virtual bool send(const PerformanceAlert& alert) = 0;

  /**
   * @brief Get the channel name
   */
  virtual std::string name() const = 0;

  /**
   * @brief Check if channel is enabled
   */
  virtual bool is_enabled() const = 0;

  /**
   * @brief Enable or disable the channel
   */
  virtual void set_enabled(bool enabled) = 0;
};

/**
 * @brief Console notification channel - logs alerts to stdout
 */
class SOLAR_CORE_API ConsoleNotificationChannel : public NotificationChannel {
 public:
  bool send(const PerformanceAlert& alert) override;
  std::string name() const override { return "console"; }
  bool is_enabled() const override { return enabled_; }
  void set_enabled(bool enabled) override { enabled_ = enabled; }

 private:
  bool enabled_ = true;
};

/**
 * @brief Webhook notification channel configuration
 */
struct WebhookConfig {
  std::string url;
  std::string method = "POST";
  std::map<std::string, std::string> headers;
  int timeout_seconds = 10;
};

/**
 * @brief Webhook notification channel - sends alerts via HTTP
 */
class SOLAR_CORE_API WebhookNotificationChannel : public NotificationChannel {
 public:
  explicit WebhookNotificationChannel(WebhookConfig config);

  bool send(const PerformanceAlert& alert) override;
  std::string name() const override { return "webhook"; }
  bool is_enabled() const override { return enabled_; }
  void set_enabled(bool enabled) override { enabled_ = enabled; }

  const WebhookConfig& config() const { return config_; }

 private:
  std::string format_payload(const PerformanceAlert& alert) const;

  WebhookConfig config_;
  bool enabled_ = true;
};

/**
 * @brief Callback notification channel - invokes a user-provided function
 */
class SOLAR_CORE_API CallbackNotificationChannel : public NotificationChannel {
 public:
  using Callback = std::function<void(const PerformanceAlert&)>;

  explicit CallbackNotificationChannel(Callback callback, std::string channel_name = "callback");

  bool send(const PerformanceAlert& alert) override;
  std::string name() const override { return name_; }
  bool is_enabled() const override { return enabled_; }
  void set_enabled(bool enabled) override { enabled_ = enabled; }

 private:
  Callback callback_;
  std::string name_;
  bool enabled_ = true;
};

/**
 * @brief File notification channel - appends alerts to a log file
 */
class SOLAR_CORE_API FileNotificationChannel : public NotificationChannel {
 public:
  explicit FileNotificationChannel(std::string filepath);

  bool send(const PerformanceAlert& alert) override;
  std::string name() const override { return "file"; }
  bool is_enabled() const override { return enabled_; }
  void set_enabled(bool enabled) override { enabled_ = enabled; }

  const std::string& filepath() const { return filepath_; }

 private:
  std::string filepath_;
  bool enabled_ = true;
};

/**
 * @brief Notification manager - routes alerts to registered channels
 */
class SOLAR_CORE_API NotificationManager {
 public:
  static NotificationManager& instance();

  /**
   * @brief Register a notification channel
   */
  void register_channel(std::shared_ptr<NotificationChannel> channel);

  /**
   * @brief Remove a notification channel by name
   */
  void remove_channel(const std::string& channel_name);

  /**
   * @brief Get a channel by name
   */
  std::shared_ptr<NotificationChannel> get_channel(const std::string& channel_name);

  /**
   * @brief Send alert to all enabled channels
   * @return Number of channels that successfully sent the notification
   */
  size_t notify(const PerformanceAlert& alert);

  /**
   * @brief Get all registered channels
   */
  std::vector<std::shared_ptr<NotificationChannel>> get_channels() const;

  /**
   * @brief Clear all channels
   */
  void clear_channels();

 private:
  NotificationManager() = default;

  mutable std::mutex mutex_;
  std::vector<std::shared_ptr<NotificationChannel>> channels_;
};

}  // namespace SolarSystem::Performance
