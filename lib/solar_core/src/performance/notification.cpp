/**
 * @file notification.cpp
 * @brief Performance alert notification system implementation
 */

#include "solar_core/performance/notification.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace SolarSystem::Performance {

namespace {
std::string severity_to_string(PerformanceAlert::Severity severity) {
  switch (severity) {
    case PerformanceAlert::Severity::INFO:
      return "INFO";
    case PerformanceAlert::Severity::WARNING:
      return "WARNING";
    case PerformanceAlert::Severity::CRITICAL:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

std::string format_timestamp(std::chrono::system_clock::time_point tp) {
  auto time = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return oss.str();
}
}  // namespace

// ConsoleNotificationChannel
bool ConsoleNotificationChannel::send(const PerformanceAlert& alert) {
  if (!enabled_) return false;

  std::cout << "[PERF ALERT] " << severity_to_string(alert.severity) << " - " << alert.metric_name
            << ": " << alert.message << " (value: " << alert.current_value
            << ", threshold: " << alert.threshold_value << ")" << std::endl;
  return true;
}

// WebhookNotificationChannel
WebhookNotificationChannel::WebhookNotificationChannel(WebhookConfig config)
    : config_(std::move(config)) {}

bool WebhookNotificationChannel::send(const PerformanceAlert& alert) {
  if (!enabled_ || config_.url.empty()) return false;

  std::string payload = format_payload(alert);

  // Build curl command
  std::ostringstream cmd;
  cmd << "curl -s -X " << config_.method << " -H 'Content-Type: application/json'";

  for (const auto& [key, value] : config_.headers) {
    cmd << " -H '" << key << ": " << value << "'";
  }

  cmd << " --max-time " << config_.timeout_seconds;
  cmd << " -d '" << payload << "'";
  cmd << " '" << config_.url << "' > /dev/null 2>&1";

  int result = std::system(cmd.str().c_str());
  return result == 0;
}

std::string WebhookNotificationChannel::format_payload(const PerformanceAlert& alert) const {
  std::ostringstream json;
  json << "{";
  json << "\"metric\":\"" << alert.metric_name << "\",";
  json << "\"severity\":\"" << severity_to_string(alert.severity) << "\",";
  json << "\"value\":" << alert.current_value << ",";
  json << "\"threshold\":" << alert.threshold_value << ",";
  json << "\"message\":\"" << alert.message << "\",";
  json << "\"timestamp\":\"" << format_timestamp(alert.timestamp) << "\"";
  json << "}";
  return json.str();
}

// CallbackNotificationChannel
CallbackNotificationChannel::CallbackNotificationChannel(Callback callback,
                                                         std::string channel_name)
    : callback_(std::move(callback)), name_(std::move(channel_name)) {}

bool CallbackNotificationChannel::send(const PerformanceAlert& alert) {
  if (!enabled_ || !callback_) return false;

  try {
    callback_(alert);
    return true;
  } catch (...) {
    return false;
  }
}

// FileNotificationChannel
FileNotificationChannel::FileNotificationChannel(std::string filepath)
    : filepath_(std::move(filepath)) {}

bool FileNotificationChannel::send(const PerformanceAlert& alert) {
  if (!enabled_ || filepath_.empty()) return false;

  std::ofstream file(filepath_, std::ios::app);
  if (!file.is_open()) return false;

  file << format_timestamp(alert.timestamp) << " [" << severity_to_string(alert.severity) << "] "
       << alert.metric_name << ": " << alert.message << " (value: " << alert.current_value
       << ", threshold: " << alert.threshold_value << ")\n";

  return file.good();
}

// NotificationManager
NotificationManager& NotificationManager::instance() {
  static NotificationManager instance;
  return instance;
}

void NotificationManager::register_channel(std::shared_ptr<NotificationChannel> channel) {
  if (!channel) return;

  std::lock_guard<std::mutex> lock(mutex_);

  // Remove existing channel with same name
  auto it = std::remove_if(channels_.begin(), channels_.end(),
                           [&](const auto& ch) { return ch && ch->name() == channel->name(); });
  channels_.erase(it, channels_.end());

  channels_.push_back(std::move(channel));
}

void NotificationManager::remove_channel(const std::string& channel_name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = std::remove_if(channels_.begin(), channels_.end(),
                           [&](const auto& ch) { return ch && ch->name() == channel_name; });
  channels_.erase(it, channels_.end());
}

std::shared_ptr<NotificationChannel> NotificationManager::get_channel(
    const std::string& channel_name) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto& channel : channels_) {
    if (channel && channel->name() == channel_name) {
      return channel;
    }
  }
  return nullptr;
}

size_t NotificationManager::notify(const PerformanceAlert& alert) {
  std::lock_guard<std::mutex> lock(mutex_);

  size_t success_count = 0;
  for (const auto& channel : channels_) {
    if (channel && channel->is_enabled()) {
      if (channel->send(alert)) {
        ++success_count;
      }
    }
  }
  return success_count;
}

std::vector<std::shared_ptr<NotificationChannel>> NotificationManager::get_channels() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return channels_;
}

void NotificationManager::clear_channels() {
  std::lock_guard<std::mutex> lock(mutex_);
  channels_.clear();
}

}  // namespace SolarSystem::Performance
