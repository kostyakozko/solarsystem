/**
 * @file security_monitor.hpp
 * @brief Security monitoring and threat detection
 *
 * Provides real-time security monitoring with:
 * - Threat detection
 * - Anomaly detection
 * - Audit logging
 * - Alert generation
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "solar_core/security/security_manager.hpp"

namespace SolarSystem::Security {

/**
 * @brief Threat level
 */
enum class ThreatLevel { LOW, MEDIUM, HIGH, CRITICAL };

/**
 * @brief Convert threat level to string
 */
[[nodiscard]] std::string to_string(ThreatLevel level);

/**
 * @brief Security alert
 */
struct SecurityAlert {
  ThreatLevel level;
  std::string title;
  std::string description;
  std::string source_ip;
  std::string user_id;
  std::chrono::system_clock::time_point timestamp;
  std::vector<std::string> indicators;
};

/**
 * @brief Security metrics
 */
struct SecurityMetrics {
  size_t total_requests = 0;
  size_t failed_auth_attempts = 0;
  size_t successful_auth = 0;
  size_t blocked_requests = 0;
  size_t suspicious_activities = 0;
  size_t active_sessions = 0;
  std::chrono::system_clock::time_point last_updated;
};

/**
 * @brief Security monitor configuration
 */
struct SecurityMonitorConfig {
  size_t max_failed_attempts = 5;
  std::chrono::minutes failed_attempt_window{15};
  size_t suspicious_activity_threshold = 10;
  bool enable_anomaly_detection = true;
  bool enable_geo_blocking = false;
  std::vector<std::string> blocked_countries;
  std::vector<std::string> blocked_ips;
};

/**
 * @brief Security monitor
 */
class SecurityMonitor {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit SecurityMonitor(SecurityMonitorConfig config = {});

  /**
   * @brief Destructor
   */
  ~SecurityMonitor();

  // Non-copyable, movable
  SecurityMonitor(const SecurityMonitor&) = delete;
  SecurityMonitor& operator=(const SecurityMonitor&) = delete;
  SecurityMonitor(SecurityMonitor&&) noexcept;
  SecurityMonitor& operator=(SecurityMonitor&&) noexcept;

  /**
   * @brief Record security event
   */
  void record_event(const SecurityEvent& event);

  /**
   * @brief Check for threats
   */
  [[nodiscard]] std::vector<SecurityAlert> check_threats(const std::string& identifier);

  /**
   * @brief Get security metrics
   */
  [[nodiscard]] SecurityMetrics get_metrics() const;

  /**
   * @brief Check if IP is blocked
   */
  [[nodiscard]] bool is_ip_blocked(const std::string& ip) const;

  /**
   * @brief Block IP address
   */
  void block_ip(const std::string& ip, std::chrono::seconds duration = {});

  /**
   * @brief Unblock IP address
   */
  void unblock_ip(const std::string& ip);

  /**
   * @brief Get recent alerts
   */
  [[nodiscard]] std::vector<SecurityAlert> get_recent_alerts(
      std::chrono::minutes window = std::chrono::minutes{60}) const;

  /**
   * @brief Set alert callback
   */
  using AlertCallback = std::function<void(const SecurityAlert&)>;
  void set_alert_callback(AlertCallback callback);

  /**
   * @brief Start monitoring
   */
  void start();

  /**
   * @brief Stop monitoring
   */
  void stop();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Audit logger
 */
class AuditLogger {
 public:
  /**
   * @brief Log audit event
   */
  static void log(const SecurityEvent& event);

  /**
   * @brief Get audit log
   */
  [[nodiscard]] static std::vector<SecurityEvent> get_log(
      std::chrono::system_clock::time_point since = {},
      std::chrono::system_clock::time_point until = {});

  /**
   * @brief Clear old audit logs
   */
  static void cleanup(std::chrono::hours retention = std::chrono::hours{720});  // 30 days
};

}  // namespace SolarSystem::Security
