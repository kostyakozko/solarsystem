/**
 * @file dashboard.cpp
 * @brief Performance monitoring dashboard implementation
 */

#include "solar_core/performance/dashboard.hpp"

#include <algorithm>

using json = nlohmann::json;

namespace SolarSystem::Performance {

// RateLimiter implementation
RateLimiter::RateLimiter(size_t max_requests_per_minute) : max_requests_(max_requests_per_minute) {}

bool RateLimiter::allow_request(const std::string& client_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::steady_clock::now();
  auto& client_requests = requests_[client_id];

  auto cutoff = now - std::chrono::minutes(1);
  client_requests.erase(std::remove_if(client_requests.begin(), client_requests.end(),
                                       [cutoff](const auto& t) { return t < cutoff; }),
                        client_requests.end());

  if (client_requests.size() >= max_requests_) return false;
  client_requests.push_back(now);
  return true;
}

void RateLimiter::reset() {
  std::lock_guard<std::mutex> lock(mutex_);
  requests_.clear();
}

// MetricsAPI implementation
MetricsAPI::MetricsAPI(const DashboardConfig& config)
    : config_(config), rate_limiter_(std::make_unique<RateLimiter>(config.rate_limit_per_minute)) {}

bool MetricsAPI::authenticate(const std::string& api_key) const {
  if (!config_.enable_auth) return true;
  return api_key == config_.api_key;
}

bool MetricsAPI::check_rate_limit(const std::string& client_id) {
  return rate_limiter_->allow_request(client_id);
}

MetricsSnapshot MetricsAPI::get_current_metrics() const {
  MetricsSnapshot snapshot;
  snapshot.timestamp = std::chrono::system_clock::now();

  auto& pm = PerformanceMonitor::instance();
  for (const auto& metric : pm.get_all_metrics()) {
    if (metric.type == MetricType::COUNTER || metric.type == MetricType::GAUGE) {
      snapshot.counters[metric.name] = metric.value;
    }
  }

  snapshot.jpl_stats = JPLMonitor::instance().get_stats();
  snapshot.sim_stats = SimulationMonitor::instance().get_stats();
  snapshot.cache_stats = CacheMonitor::instance().get_stats();
  snapshot.web_stats = WebMonitor::instance().get_stats();
  return snapshot;
}

std::string MetricsAPI::get_metrics_json() const {
  auto snapshot = get_current_metrics();
  json j;
  j["timestamp"] = std::chrono::system_clock::to_time_t(snapshot.timestamp);
  j["metrics"] = snapshot.counters;
  j["jpl"] = {{"avg_api_time", snapshot.jpl_stats.avg_api_response_time},
              {"cache_hit_rate", snapshot.jpl_stats.cache_hit_rate},
              {"total_requests", snapshot.jpl_stats.total_requests}};
  j["simulation"] = {{"avg_timestep", snapshot.sim_stats.avg_timestep_duration},
                     {"peak_memory", snapshot.sim_stats.peak_memory_bytes},
                     {"total_timesteps", snapshot.sim_stats.total_timesteps}};
  j["cache"] = {{"hit_rate", snapshot.cache_stats.hit_rate},
                {"compression_ratio", snapshot.cache_stats.compression_ratio},
                {"total_reads", snapshot.cache_stats.total_reads}};
  j["web"] = {{"avg_response_time", snapshot.web_stats.avg_http_response_time},
              {"fps", snapshot.web_stats.fps},
              {"error_count", snapshot.web_stats.error_count}};
  return j.dump();
}

std::string MetricsAPI::get_component_metrics_json(const std::string& component) const {
  json j;
  if (component == "jpl") {
    auto s = JPLMonitor::instance().get_stats();
    j = {{"avg_api_response_time", s.avg_api_response_time},
         {"avg_parse_time", s.avg_parse_time},
         {"cache_hit_rate", s.cache_hit_rate},
         {"avg_network_latency", s.avg_network_latency},
         {"total_requests", s.total_requests},
         {"successful_requests", s.successful_requests},
         {"retry_count", s.retry_count}};
  } else if (component == "simulation") {
    auto s = SimulationMonitor::instance().get_stats();
    j = {{"avg_timestep_duration", s.avg_timestep_duration},
         {"avg_body_calc_time", s.avg_body_calc_time},
         {"peak_memory_bytes", s.peak_memory_bytes},
         {"current_memory_bytes", s.current_memory_bytes},
         {"avg_convergence", s.avg_convergence},
         {"total_timesteps", s.total_timesteps}};
  } else if (component == "cache") {
    auto s = CacheMonitor::instance().get_stats();
    j = {{"avg_read_time", s.avg_read_time},
         {"avg_write_time", s.avg_write_time},
         {"compression_ratio", s.compression_ratio},
         {"hit_rate", s.hit_rate},
         {"total_reads", s.total_reads},
         {"total_writes", s.total_writes},
         {"invalidations", s.invalidations}};
  } else if (component == "web") {
    auto s = WebMonitor::instance().get_stats();
    j = {{"avg_http_response_time", s.avg_http_response_time},
         {"avg_frame_time", s.avg_frame_time},
         {"fps", s.fps},
         {"avg_interaction_latency", s.avg_interaction_latency},
         {"total_requests", s.total_requests},
         {"error_count", s.error_count}};
  } else {
    j = {{"error", "Unknown component: " + component}};
  }
  return j.dump();
}

std::string MetricsAPI::get_alerts_json() const {
  auto alerts = PerformanceMonitor::instance().get_alerts();
  json j;
  j["alerts"] = json::array();
  for (const auto& alert : alerts) {
    j["alerts"].push_back({{"metric", alert.metric_name},
                           {"severity", static_cast<int>(alert.severity)},
                           {"current_value", alert.current_value},
                           {"threshold", alert.threshold_value},
                           {"message", alert.message}});
  }
  return j.dump();
}

std::string MetricsAPI::query_metric(const std::string& name) const {
  auto& pm = PerformanceMonitor::instance();
  json j;

  if (auto counter = pm.get_counter(name)) {
    j = {{"name", name}, {"type", "counter"}, {"value", counter->get()}};
  } else if (auto gauge = pm.get_gauge(name)) {
    j = {{"name", name}, {"type", "gauge"}, {"value", gauge->get()}};
  } else if (auto timer = pm.get_timer(name)) {
    auto stats = timer->get_statistics();
    j = {{"name", name},     {"type", "timer"},  {"mean", stats.mean},
         {"p95", stats.p95}, {"p99", stats.p99}, {"count", stats.count}};
  } else {
    j = {{"error", "Metric not found: " + name}};
  }
  return j.dump();
}

std::string MetricsAPI::query_metrics_by_prefix(const std::string& prefix) const {
  auto metrics = PerformanceMonitor::instance().get_all_metrics();
  json j;
  j["metrics"] = json::array();
  for (const auto& m : metrics) {
    if (m.name.find(prefix) == 0) {
      j["metrics"].push_back({{"name", m.name}, {"value", m.value}});
    }
  }
  return j.dump();
}

// DashboardServer implementation
DashboardServer::DashboardServer(const DashboardConfig& config)
    : config_(config), api_(std::make_unique<MetricsAPI>(config)) {}

DashboardServer::~DashboardServer() { stop(); }

bool DashboardServer::start() {
  if (running_.load()) return true;
  running_.store(true);
  collect_metrics();
  return true;
}

void DashboardServer::stop() { running_.store(false); }

bool DashboardServer::is_running() const { return running_.load(); }

void DashboardServer::collect_metrics() {
  auto snapshot = api_->get_current_metrics();

  std::lock_guard<std::mutex> lock(mutex_);
  add_sample("jpl.api_time", snapshot.jpl_stats.avg_api_response_time);
  add_sample("jpl.cache_hit_rate", snapshot.jpl_stats.cache_hit_rate);
  add_sample("sim.timestep", snapshot.sim_stats.avg_timestep_duration);
  add_sample("sim.memory", static_cast<double>(snapshot.sim_stats.current_memory_bytes));
  add_sample("cache.hit_rate", snapshot.cache_stats.hit_rate);
  add_sample("web.response_time", snapshot.web_stats.avg_http_response_time);
  add_sample("web.fps", snapshot.web_stats.fps);
}

void DashboardServer::add_sample(const std::string& name, double value) {
  auto& series = time_series_[name];
  series.emplace_back(std::chrono::system_clock::now(), value);
  if (series.size() > MAX_SAMPLES) {
    series.erase(series.begin());
  }
}

DashboardData DashboardServer::get_dashboard_data() const {
  std::lock_guard<std::mutex> lock(mutex_);
  DashboardData data;
  data.current = api_->get_current_metrics();
  data.recent_alerts = PerformanceMonitor::instance().get_alerts();

  for (const auto& [name, points] : time_series_) {
    DashboardData::TimeSeries ts;
    ts.name = name;
    ts.points = points;
    data.series.push_back(std::move(ts));
  }

  return data;
}

MetricsAPI& DashboardServer::api() { return *api_; }

void DashboardServer::register_endpoint(const std::string& path, RequestHandler handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  endpoints_[path] = std::move(handler);
}

// DashboardRenderer implementation
std::string DashboardRenderer::render_chart_data(const DashboardData& data) {
  json j;
  j["series"] = json::array();
  for (const auto& ts : data.series) {
    json series_obj;
    series_obj["name"] = ts.name;
    series_obj["data"] = json::array();
    for (const auto& [time, value] : ts.points) {
      series_obj["data"].push_back({std::chrono::system_clock::to_time_t(time), value});
    }
    j["series"].push_back(series_obj);
  }
  return j.dump();
}

std::string DashboardRenderer::render_alerts_panel(const std::vector<PerformanceAlert>& alerts) {
  json j;
  j["count"] = alerts.size();
  j["alerts"] = json::array();
  for (const auto& alert : alerts) {
    const char* severity_str =
        alert.severity == PerformanceAlert::Severity::CRITICAL
            ? "critical"
            : (alert.severity == PerformanceAlert::Severity::WARNING ? "warning" : "info");
    j["alerts"].push_back(
        {{"metric", alert.metric_name}, {"severity", severity_str}, {"message", alert.message}});
  }
  return j.dump();
}

std::string DashboardRenderer::render_summary_cards(const MetricsSnapshot& snapshot) {
  json j;
  j["cards"] = json::array();
  j["cards"].push_back({{"title", "JPL API"},
                        {"value", snapshot.jpl_stats.avg_api_response_time * 1000},
                        {"unit", "ms"},
                        {"subtitle", "Avg Response Time"}});
  j["cards"].push_back({{"title", "Cache Hit Rate"},
                        {"value", snapshot.cache_stats.hit_rate * 100},
                        {"unit", "%"},
                        {"subtitle", "Cache Efficiency"}});
  j["cards"].push_back({{"title", "Web FPS"},
                        {"value", snapshot.web_stats.fps},
                        {"unit", "fps"},
                        {"subtitle", "Render Performance"}});
  j["cards"].push_back({{"title", "Simulation"},
                        {"value", snapshot.sim_stats.total_timesteps},
                        {"unit", "steps"},
                        {"subtitle", "Total Timesteps"}});
  return j.dump();
}

std::string DashboardRenderer::render_html_dashboard(const DashboardData& /* data */) {
  // HTML dashboard is served from the dashboard.html template file via the web server.
  // This API method returns a JSON redirect to the static HTML resource.
  return R"({"redirect":"/dashboard.html"})";
}

}  // namespace SolarSystem::Performance
