/**
 * @file dashboard.hpp
 * @brief Performance monitoring dashboard server and API
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "solar_core/export.hpp"
#include "solar_core/performance/component_monitors.hpp"
#include "solar_core/performance/performance_monitor.hpp"

namespace SolarSystem::Performance {

/**
 * @brief Dashboard configuration
 */
struct DashboardConfig {
  uint16_t port = 9090;
  std::string bind_address = "0.0.0.0";
  bool enable_auth = false;
  std::string api_key;
  size_t rate_limit_per_minute = 60;
  std::chrono::seconds refresh_interval{5};
};

/**
 * @brief Metrics snapshot for API responses
 */
struct MetricsSnapshot {
  std::chrono::system_clock::time_point timestamp;
  std::map<std::string, double> counters;
  std::map<std::string, double> gauges;
  std::map<std::string, Histogram::Statistics> histograms;
  JPLMonitor::Stats jpl_stats;
  SimulationMonitor::Stats sim_stats;
  CacheMonitor::Stats cache_stats;
  WebMonitor::Stats web_stats;
};

/**
 * @brief Rate limiter for API requests
 */
class SOLAR_CORE_API RateLimiter {
 public:
  explicit RateLimiter(size_t max_requests_per_minute);
  bool allow_request(const std::string& client_id);
  void reset();

 private:
  size_t max_requests_;
  std::mutex mutex_;
  std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> requests_;
};

/**
 * @brief Metrics API for querying performance data
 */
class SOLAR_CORE_API MetricsAPI {
 public:
  explicit MetricsAPI(const DashboardConfig& config);

  // Authentication
  bool authenticate(const std::string& api_key) const;

  // Rate limiting
  bool check_rate_limit(const std::string& client_id);

  // Metrics queries
  MetricsSnapshot get_current_metrics() const;
  std::string get_metrics_json() const;
  std::string get_component_metrics_json(const std::string& component) const;
  std::string get_alerts_json() const;

  // Query specific metrics
  std::string query_metric(const std::string& name) const;
  std::string query_metrics_by_prefix(const std::string& prefix) const;

 private:
  DashboardConfig config_;
  std::unique_ptr<RateLimiter> rate_limiter_;
};

/**
 * @brief Dashboard data for visualization
 */
struct DashboardData {
  // Time series for charts (last N samples)
  struct TimeSeries {
    std::string name;
    std::vector<std::pair<std::chrono::system_clock::time_point, double>> points;
  };

  std::vector<TimeSeries> series;
  std::vector<PerformanceAlert> recent_alerts;
  MetricsSnapshot current;
};

/**
 * @brief Dashboard server for web-based visualization
 */
class SOLAR_CORE_API DashboardServer {
 public:
  explicit DashboardServer(const DashboardConfig& config);
  ~DashboardServer();

  // Server control
  bool start();
  void stop();
  bool is_running() const;

  // Data access
  DashboardData get_dashboard_data() const;
  MetricsAPI& api();

  // Callbacks for custom endpoints
  using RequestHandler = std::function<std::string(const std::string&)>;
  void register_endpoint(const std::string& path, RequestHandler handler);

 private:
  void collect_metrics();
  void add_sample(const std::string& name, double value);

  DashboardConfig config_;
  std::unique_ptr<MetricsAPI> api_;
  std::atomic<bool> running_{false};
  mutable std::mutex mutex_;
  std::map<std::string, RequestHandler> endpoints_;

  // Time series storage (circular buffer)
  static constexpr size_t MAX_SAMPLES = 100;
  std::map<std::string, std::vector<std::pair<std::chrono::system_clock::time_point, double>>>
      time_series_;
};

/**
 * @brief HTML/JSON generators for dashboard visualization
 */
class SOLAR_CORE_API DashboardRenderer {
 public:
  // Generate JSON for charts
  static std::string render_chart_data(const DashboardData& data);
  static std::string render_alerts_panel(const std::vector<PerformanceAlert>& alerts);
  static std::string render_summary_cards(const MetricsSnapshot& snapshot);

  // Generate simple HTML dashboard
  static std::string render_html_dashboard(const DashboardData& data);
};

}  // namespace SolarSystem::Performance
