/**
 * @file component_monitors.cpp
 * @brief Component-specific performance monitoring implementation
 */

#include "solar_core/performance/component_monitors.hpp"

namespace SolarSystem::Performance {

// JPLMonitor implementation
JPLMonitor& JPLMonitor::instance() {
  static JPLMonitor instance;
  return instance;
}

JPLMonitor::JPLMonitor() {
  auto& pm = PerformanceMonitor::instance();
  api_timer_ = pm.register_timer("jpl.api.response_time");
  parse_timer_ = pm.register_timer("jpl.parse.duration");
  latency_timer_ = pm.register_timer("jpl.network.latency");
  request_counter_ = pm.register_counter("jpl.requests.total");
  success_counter_ = pm.register_counter("jpl.requests.success");
  cache_hit_counter_ = pm.register_counter("jpl.cache.hits");
  cache_miss_counter_ = pm.register_counter("jpl.cache.misses");
  retry_counter_ = pm.register_counter("jpl.requests.retries");
}

void JPLMonitor::record_api_request(double duration_seconds, bool success) {
  api_timer_->record(duration_seconds);
  request_counter_->increment();
  if (success) success_counter_->increment();
}

void JPLMonitor::record_parse_duration(double duration_seconds) {
  parse_timer_->record(duration_seconds);
}

void JPLMonitor::record_cache_access(bool hit) {
  if (hit)
    cache_hit_counter_->increment();
  else
    cache_miss_counter_->increment();
}

void JPLMonitor::record_network_latency(double latency_seconds) {
  latency_timer_->record(latency_seconds);
}

void JPLMonitor::record_retry() { retry_counter_->increment(); }

JPLMonitor::Stats JPLMonitor::get_stats() const {
  Stats stats;
  auto api_stats = api_timer_->get_statistics();
  auto parse_stats = parse_timer_->get_statistics();
  auto latency_stats = latency_timer_->get_statistics();

  stats.avg_api_response_time = api_stats.mean;
  stats.avg_parse_time = parse_stats.mean;
  stats.avg_network_latency = latency_stats.mean;
  stats.total_requests = static_cast<size_t>(request_counter_->get());
  stats.successful_requests = static_cast<size_t>(success_counter_->get());
  stats.retry_count = static_cast<size_t>(retry_counter_->get());

  double hits = cache_hit_counter_->get();
  double misses = cache_miss_counter_->get();
  stats.cache_hit_rate = (hits + misses > 0) ? hits / (hits + misses) : 0;

  return stats;
}

void JPLMonitor::reset() {
  request_counter_->reset();
  success_counter_->reset();
  cache_hit_counter_->reset();
  cache_miss_counter_->reset();
  retry_counter_->reset();
}

// SimulationMonitor implementation
SimulationMonitor& SimulationMonitor::instance() {
  static SimulationMonitor instance;
  return instance;
}

SimulationMonitor::SimulationMonitor() {
  auto& pm = PerformanceMonitor::instance();
  timestep_timer_ = pm.register_timer("sim.timestep.duration");
  body_calc_timer_ = pm.register_timer("sim.body.calc_time");
  memory_gauge_ = pm.register_gauge("sim.memory.current");
  convergence_histogram_ = pm.register_histogram("sim.convergence");
  timestep_counter_ = pm.register_counter("sim.timesteps.total");
}

void SimulationMonitor::record_timestep(double duration_seconds) {
  timestep_timer_->record(duration_seconds);
  timestep_counter_->increment();
}

void SimulationMonitor::record_body_calculation(double duration_seconds, size_t /* body_count */) {
  body_calc_timer_->record(duration_seconds);
}

void SimulationMonitor::record_memory_usage(size_t bytes) {
  memory_gauge_->set(static_cast<double>(bytes));
  size_t current_peak = peak_memory_.load();
  while (bytes > current_peak && !peak_memory_.compare_exchange_weak(current_peak, bytes)) {
  }
}

void SimulationMonitor::record_convergence_metric(double value) {
  convergence_histogram_->observe(value);
}

SimulationMonitor::Stats SimulationMonitor::get_stats() const {
  Stats stats;
  auto ts_stats = timestep_timer_->get_statistics();
  auto body_stats = body_calc_timer_->get_statistics();
  auto conv_stats = convergence_histogram_->get_statistics();

  stats.avg_timestep_duration = ts_stats.mean;
  stats.avg_body_calc_time = body_stats.mean;
  stats.current_memory_bytes = static_cast<size_t>(memory_gauge_->get());
  stats.peak_memory_bytes = peak_memory_.load();
  stats.avg_convergence = conv_stats.mean;
  stats.total_timesteps = static_cast<size_t>(timestep_counter_->get());

  return stats;
}

void SimulationMonitor::reset() {
  timestep_counter_->reset();
  memory_gauge_->set(0);
  peak_memory_.store(0);
}

// CacheMonitor implementation
CacheMonitor& CacheMonitor::instance() {
  static CacheMonitor instance;
  return instance;
}

CacheMonitor::CacheMonitor() {
  auto& pm = PerformanceMonitor::instance();
  read_timer_ = pm.register_timer("cache.read.duration");
  write_timer_ = pm.register_timer("cache.write.duration");
  read_counter_ = pm.register_counter("cache.reads.total");
  write_counter_ = pm.register_counter("cache.writes.total");
  hit_counter_ = pm.register_counter("cache.hits");
  miss_counter_ = pm.register_counter("cache.misses");
  invalidation_counter_ = pm.register_counter("cache.invalidations");
}

void CacheMonitor::record_read(double duration_seconds, size_t /* bytes */) {
  read_timer_->record(duration_seconds);
  read_counter_->increment();
}

void CacheMonitor::record_write(double duration_seconds, size_t /* bytes */) {
  write_timer_->record(duration_seconds);
  write_counter_->increment();
}

void CacheMonitor::record_compression(size_t original_size, size_t compressed_size) {
  total_original_.fetch_add(original_size);
  total_compressed_.fetch_add(compressed_size);
}

void CacheMonitor::record_hit() { hit_counter_->increment(); }

void CacheMonitor::record_miss() { miss_counter_->increment(); }

void CacheMonitor::record_invalidation() { invalidation_counter_->increment(); }

CacheMonitor::Stats CacheMonitor::get_stats() const {
  Stats stats;
  auto read_stats = read_timer_->get_statistics();
  auto write_stats = write_timer_->get_statistics();

  stats.avg_read_time = read_stats.mean;
  stats.avg_write_time = write_stats.mean;
  stats.total_reads = static_cast<size_t>(read_counter_->get());
  stats.total_writes = static_cast<size_t>(write_counter_->get());
  stats.invalidations = static_cast<size_t>(invalidation_counter_->get());

  size_t orig = total_original_.load();
  size_t comp = total_compressed_.load();
  stats.compression_ratio =
      (orig > 0) ? static_cast<double>(comp) / static_cast<double>(orig) : 1.0;

  double hits = hit_counter_->get();
  double misses = miss_counter_->get();
  stats.hit_rate = (hits + misses > 0) ? hits / (hits + misses) : 0;

  return stats;
}

void CacheMonitor::reset() {
  read_counter_->reset();
  write_counter_->reset();
  hit_counter_->reset();
  miss_counter_->reset();
  invalidation_counter_->reset();
  total_original_.store(0);
  total_compressed_.store(0);
}

// WebMonitor implementation
WebMonitor& WebMonitor::instance() {
  static WebMonitor instance;
  return instance;
}

WebMonitor::WebMonitor() {
  auto& pm = PerformanceMonitor::instance();
  http_timer_ = pm.register_timer("web.http.response_time");
  render_timer_ = pm.register_timer("web.render.frame_time");
  interaction_timer_ = pm.register_timer("web.interaction.latency");
  request_counter_ = pm.register_counter("web.requests.total");
  error_counter_ = pm.register_counter("web.requests.errors");
}

void WebMonitor::record_http_request(double duration_seconds, int status_code) {
  http_timer_->record(duration_seconds);
  request_counter_->increment();
  if (status_code >= 400) error_counter_->increment();
}

void WebMonitor::record_render_frame(double duration_seconds) {
  render_timer_->record(duration_seconds);
}

void WebMonitor::record_user_interaction(double latency_seconds) {
  interaction_timer_->record(latency_seconds);
}

void WebMonitor::record_api_call(const std::string& /* endpoint */, double duration_seconds) {
  http_timer_->record(duration_seconds);
}

WebMonitor::Stats WebMonitor::get_stats() const {
  Stats stats;
  auto http_stats = http_timer_->get_statistics();
  auto render_stats = render_timer_->get_statistics();
  auto interaction_stats = interaction_timer_->get_statistics();

  stats.avg_http_response_time = http_stats.mean;
  stats.avg_frame_time = render_stats.mean;
  stats.fps = (render_stats.mean > 0) ? 1.0 / render_stats.mean : 0;
  stats.avg_interaction_latency = interaction_stats.mean;
  stats.total_requests = static_cast<size_t>(request_counter_->get());
  stats.error_count = static_cast<size_t>(error_counter_->get());

  return stats;
}

void WebMonitor::reset() {
  request_counter_->reset();
  error_counter_->reset();
}

}  // namespace SolarSystem::Performance
