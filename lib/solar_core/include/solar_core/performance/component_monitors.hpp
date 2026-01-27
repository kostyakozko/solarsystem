/**
 * @file component_monitors.hpp
 * @brief Component-specific performance monitoring
 */

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "solar_core/export.hpp"
#include "solar_core/performance/performance_monitor.hpp"

namespace SolarSystem::Performance {

/**
 * @brief JPL operations monitoring (Task 6.1)
 */
class SOLAR_CORE_API JPLMonitor {
 public:
  static JPLMonitor& instance();

  // API metrics
  void record_api_request(double duration_seconds, bool success);
  void record_parse_duration(double duration_seconds);
  void record_cache_access(bool hit);
  void record_network_latency(double latency_seconds);
  void record_retry();

  // Statistics
  struct Stats {
    double avg_api_response_time = 0;
    double avg_parse_time = 0;
    double cache_hit_rate = 0;
    double avg_network_latency = 0;
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t retry_count = 0;
  };
  Stats get_stats() const;
  void reset();

 private:
  JPLMonitor();
  std::shared_ptr<Timer> api_timer_;
  std::shared_ptr<Timer> parse_timer_;
  std::shared_ptr<Timer> latency_timer_;
  std::shared_ptr<Counter> request_counter_;
  std::shared_ptr<Counter> success_counter_;
  std::shared_ptr<Counter> cache_hit_counter_;
  std::shared_ptr<Counter> cache_miss_counter_;
  std::shared_ptr<Counter> retry_counter_;
};

/**
 * @brief Simulation performance monitoring (Task 6.2)
 */
class SOLAR_CORE_API SimulationMonitor {
 public:
  static SimulationMonitor& instance();

  // Simulation metrics
  void record_timestep(double duration_seconds);
  void record_body_calculation(double duration_seconds, size_t body_count);
  void record_memory_usage(size_t bytes);
  void record_convergence_metric(double value);

  // Statistics
  struct Stats {
    double avg_timestep_duration = 0;
    double avg_body_calc_time = 0;
    size_t peak_memory_bytes = 0;
    size_t current_memory_bytes = 0;
    double avg_convergence = 0;
    size_t total_timesteps = 0;
  };
  Stats get_stats() const;
  void reset();

 private:
  SimulationMonitor();
  std::shared_ptr<Timer> timestep_timer_;
  std::shared_ptr<Timer> body_calc_timer_;
  std::shared_ptr<Gauge> memory_gauge_;
  std::shared_ptr<Histogram> convergence_histogram_;
  std::shared_ptr<Counter> timestep_counter_;
  std::atomic<size_t> peak_memory_{0};
};

/**
 * @brief Cache performance monitoring (Task 6.3)
 */
class SOLAR_CORE_API CacheMonitor {
 public:
  static CacheMonitor& instance();

  // Cache metrics
  void record_read(double duration_seconds, size_t bytes);
  void record_write(double duration_seconds, size_t bytes);
  void record_compression(size_t original_size, size_t compressed_size);
  void record_hit();
  void record_miss();
  void record_invalidation();

  // Statistics
  struct Stats {
    double avg_read_time = 0;
    double avg_write_time = 0;
    double compression_ratio = 0;
    double hit_rate = 0;
    size_t total_reads = 0;
    size_t total_writes = 0;
    size_t invalidations = 0;
  };
  Stats get_stats() const;
  void reset();

 private:
  CacheMonitor();
  std::shared_ptr<Timer> read_timer_;
  std::shared_ptr<Timer> write_timer_;
  std::shared_ptr<Counter> read_counter_;
  std::shared_ptr<Counter> write_counter_;
  std::shared_ptr<Counter> hit_counter_;
  std::shared_ptr<Counter> miss_counter_;
  std::shared_ptr<Counter> invalidation_counter_;
  std::atomic<size_t> total_original_{0};
  std::atomic<size_t> total_compressed_{0};
};

/**
 * @brief Web interface monitoring (Task 6.4)
 */
class SOLAR_CORE_API WebMonitor {
 public:
  static WebMonitor& instance();

  // HTTP metrics
  void record_http_request(double duration_seconds, int status_code);
  void record_render_frame(double duration_seconds);
  void record_user_interaction(double latency_seconds);
  void record_api_call(const std::string& endpoint, double duration_seconds);

  // Statistics
  struct Stats {
    double avg_http_response_time = 0;
    double avg_frame_time = 0;
    double fps = 0;
    double avg_interaction_latency = 0;
    size_t total_requests = 0;
    size_t error_count = 0;
  };
  Stats get_stats() const;
  void reset();

 private:
  WebMonitor();
  std::shared_ptr<Timer> http_timer_;
  std::shared_ptr<Timer> render_timer_;
  std::shared_ptr<Timer> interaction_timer_;
  std::shared_ptr<Counter> request_counter_;
  std::shared_ptr<Counter> error_counter_;
};

// Convenience macros
#define JPL_MONITOR_API_REQUEST(duration, success) \
  SolarSystem::Performance::JPLMonitor::instance().record_api_request(duration, success)
#define JPL_MONITOR_CACHE_HIT() \
  SolarSystem::Performance::JPLMonitor::instance().record_cache_access(true)
#define JPL_MONITOR_CACHE_MISS() \
  SolarSystem::Performance::JPLMonitor::instance().record_cache_access(false)

#define SIM_MONITOR_TIMESTEP(duration) \
  SolarSystem::Performance::SimulationMonitor::instance().record_timestep(duration)
#define SIM_MONITOR_MEMORY(bytes) \
  SolarSystem::Performance::SimulationMonitor::instance().record_memory_usage(bytes)

#define CACHE_MONITOR_READ(duration, bytes) \
  SolarSystem::Performance::CacheMonitor::instance().record_read(duration, bytes)
#define CACHE_MONITOR_WRITE(duration, bytes) \
  SolarSystem::Performance::CacheMonitor::instance().record_write(duration, bytes)

#define WEB_MONITOR_REQUEST(duration, status) \
  SolarSystem::Performance::WebMonitor::instance().record_http_request(duration, status)
#define WEB_MONITOR_FRAME(duration) \
  SolarSystem::Performance::WebMonitor::instance().record_render_frame(duration)

}  // namespace SolarSystem::Performance
