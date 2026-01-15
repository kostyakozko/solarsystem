/**
 * @file security_monitor.cpp
 * @brief Implementation of security monitoring
 */

#include "solar_core/security/security_monitor.hpp"

#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <vector>

namespace SolarSystem::Security {

std::string to_string(ThreatLevel level) {
  switch (level) {
    case ThreatLevel::LOW: return "LOW";
    case ThreatLevel::MEDIUM: return "MEDIUM";
    case ThreatLevel::HIGH: return "HIGH";
    case ThreatLevel::CRITICAL: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

struct SecurityMonitor::Impl {
  SecurityMonitorConfig config;
  std::vector<SecurityEvent> events;
  std::vector<SecurityAlert> alerts;
  std::map<std::string, size_t> failed_attempts;
  std::vector<std::string> blocked_ips;
  SecurityMetrics metrics;
  mutable std::mutex mutex;
  AlertCallback alert_callback;
  std::atomic<bool> running{false};

  explicit Impl(SecurityMonitorConfig cfg) : config(std::move(cfg)) {}
};

SecurityMonitor::SecurityMonitor(SecurityMonitorConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

SecurityMonitor::~SecurityMonitor() = default;

SecurityMonitor::SecurityMonitor(SecurityMonitor&&) noexcept = default;
SecurityMonitor& SecurityMonitor::operator=(SecurityMonitor&&) noexcept = default;

void SecurityMonitor::record_event(const SecurityEvent& event) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->events.push_back(event);

  // Update metrics
  impl_->metrics.total_requests++;

  if (event.type == SecurityEventType::LOGIN_FAILURE) {
    impl_->metrics.failed_auth_attempts++;
    impl_->failed_attempts[event.user_id]++;

    // Check for brute force
    if (impl_->failed_attempts[event.user_id] >= impl_->config.max_failed_attempts) {
      SecurityAlert alert;
      alert.level = ThreatLevel::HIGH;
      alert.title = "Brute Force Attack Detected";
      alert.description = "Multiple failed login attempts";
      alert.user_id = event.user_id;
      alert.source_ip = event.ip_address;
      alert.timestamp = std::chrono::system_clock::now();

      impl_->alerts.push_back(alert);

      if (impl_->alert_callback) {
        impl_->alert_callback(alert);
      }
    }
  } else if (event.type == SecurityEventType::LOGIN_SUCCESS) {
    impl_->metrics.successful_auth++;
    impl_->failed_attempts.erase(event.user_id);
  }

  impl_->metrics.last_updated = std::chrono::system_clock::now();
}

std::vector<SecurityAlert> SecurityMonitor::check_threats(const std::string& identifier) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  std::vector<SecurityAlert> threats;

  // Check for excessive failed attempts
  if (impl_->failed_attempts.count(identifier) &&
      impl_->failed_attempts[identifier] >= impl_->config.max_failed_attempts) {
    SecurityAlert alert;
    alert.level = ThreatLevel::HIGH;
    alert.title = "Excessive Failed Attempts";
    alert.description = "User has exceeded maximum failed login attempts";
    alert.user_id = identifier;
    alert.timestamp = std::chrono::system_clock::now();
    threats.push_back(alert);
  }

  return threats;
}

SecurityMetrics SecurityMonitor::get_metrics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->metrics;
}

bool SecurityMonitor::is_ip_blocked(const std::string& ip) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return std::find(impl_->blocked_ips.begin(), impl_->blocked_ips.end(), ip) !=
         impl_->blocked_ips.end();
}

void SecurityMonitor::block_ip(const std::string& ip, std::chrono::seconds /* duration */) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->blocked_ips.push_back(ip);
  impl_->metrics.blocked_requests++;
}

void SecurityMonitor::unblock_ip(const std::string& ip) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->blocked_ips.erase(
      std::remove(impl_->blocked_ips.begin(), impl_->blocked_ips.end(), ip),
      impl_->blocked_ips.end());
}

std::vector<SecurityAlert> SecurityMonitor::get_recent_alerts(
    std::chrono::minutes window) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto cutoff = std::chrono::system_clock::now() - window;

  std::vector<SecurityAlert> recent;
  std::copy_if(impl_->alerts.begin(), impl_->alerts.end(),
               std::back_inserter(recent),
               [cutoff](const SecurityAlert& alert) {
                 return alert.timestamp >= cutoff;
               });

  return recent;
}

void SecurityMonitor::set_alert_callback(AlertCallback callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->alert_callback = std::move(callback);
}

void SecurityMonitor::start() {
  impl_->running.store(true);
}

void SecurityMonitor::stop() {
  impl_->running.store(false);
}

// AuditLogger implementation
static std::vector<SecurityEvent> audit_log;
static std::mutex audit_mutex;

void AuditLogger::log(const SecurityEvent& event) {
  std::lock_guard<std::mutex> lock(audit_mutex);
  audit_log.push_back(event);
}

std::vector<SecurityEvent> AuditLogger::get_log(
    std::chrono::system_clock::time_point since,
    std::chrono::system_clock::time_point until) {
  std::lock_guard<std::mutex> lock(audit_mutex);

  if (since == std::chrono::system_clock::time_point{} &&
      until == std::chrono::system_clock::time_point{}) {
    return audit_log;
  }

  std::vector<SecurityEvent> filtered;
  std::copy_if(audit_log.begin(), audit_log.end(),
               std::back_inserter(filtered),
               [since, until](const SecurityEvent& event) {
                 return (since == std::chrono::system_clock::time_point{} || event.timestamp >= since) &&
                        (until == std::chrono::system_clock::time_point{} || event.timestamp <= until);
               });

  return filtered;
}

void AuditLogger::cleanup(std::chrono::hours retention) {
  std::lock_guard<std::mutex> lock(audit_mutex);

  auto cutoff = std::chrono::system_clock::now() - retention;

  audit_log.erase(
      std::remove_if(audit_log.begin(), audit_log.end(),
                     [cutoff](const SecurityEvent& event) {
                       return event.timestamp < cutoff;
                     }),
      audit_log.end());
}

}  // namespace SolarSystem::Security
