# Performance Monitoring System

Comprehensive performance monitoring for the Solar System Suite with real-time metrics, alerting, analytics, and visualization.

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Core Components](#core-components)
- [API Reference](#api-reference)
- [Dashboard Guide](#dashboard-guide)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

## Overview

The Performance Monitoring System provides:

- **Real-time Metrics**: Counters, gauges, histograms, and timers
- **Component Monitors**: JPL, Simulation, Cache, and Web monitoring
- **Alerting**: Threshold-based alerts with notification channels
- **Analytics**: KPI tracking, forecasting, and capacity planning
- **Visualization**: Web dashboard with real-time updates
- **Profiling**: Execution tracing and bottleneck detection

## Quick Start

### Basic Usage

```cpp
#include "solar_core/performance/performance_monitor.hpp"

using namespace SolarSystem::Performance;

// Get the singleton instance
auto& monitor = PerformanceMonitor::instance();

// Register and use metrics
auto counter = monitor.register_counter("requests_total");
counter->increment();

auto timer = monitor.register_timer("request_duration");
{
    auto scoped = timer->time();  // Automatically records duration
    // ... operation ...
}

// Use convenience macros
PERF_COUNTER_INC("api_calls");
PERF_GAUGE_SET("active_connections", 42);
PERF_TIMER_SCOPE("operation_time");
```

### Component Monitoring

```cpp
#include "solar_core/performance/component_monitors.hpp"

// JPL operations
JPLMonitor::instance().record_api_request(0.5, true);
JPLMonitor::instance().record_cache_access(true);  // cache hit

// Simulation
SimulationMonitor::instance().record_timestep(0.001);
SimulationMonitor::instance().record_memory_usage(1024 * 1024);

// Cache
CacheMonitor::instance().record_read(0.01, 4096);
CacheMonitor::instance().record_compression(1000, 500);

// Web
WebMonitor::instance().record_http_request(0.1, 200);
WebMonitor::instance().record_render_frame(0.016);
```

## Core Components

### PerformanceMonitor

Central registry for all metrics.

| Method | Description |
|--------|-------------|
| `register_counter(name)` | Create monotonically increasing counter |
| `register_gauge(name)` | Create value that can increase/decrease |
| `register_histogram(name, buckets)` | Create distribution tracker |
| `register_timer(name)` | Create duration measurement |
| `set_threshold(threshold)` | Configure alerting threshold |
| `get_alerts()` | Retrieve triggered alerts |
| `generate_report()` | Generate text report of all metrics |

### Component Monitors

| Monitor | Metrics |
|---------|---------|
| **JPLMonitor** | API response time, parse duration, cache hit rate, network latency, retries |
| **SimulationMonitor** | Timestep duration, body calculations, memory usage, convergence |
| **CacheMonitor** | Read/write time, compression ratio, hit rate, invalidations |
| **WebMonitor** | HTTP response time, FPS, interaction latency, errors |

### Alert System

```cpp
#include "solar_core/performance/notification.hpp"
#include "solar_core/performance/alert_intelligence.hpp"

// Configure threshold
PerformanceThreshold threshold;
threshold.metric_name = "response_time";
threshold.warning_threshold = 100;
threshold.critical_threshold = 500;
threshold.above_threshold = true;
monitor.set_threshold(threshold);

// Set up notifications
auto& notifier = NotificationManager::instance();
notifier.add_channel(std::make_unique<ConsoleNotificationChannel>());
notifier.add_channel(std::make_unique<WebhookNotificationChannel>("https://hooks.example.com"));

// Alert intelligence (aggregation, correlation, escalation)
AlertIntelligence intelligence;
intelligence.configure_aggregation({std::chrono::minutes(5), 10});
intelligence.add_escalation_rule({"response_time", std::chrono::minutes(15), 5,
                                   PerformanceAlert::Severity::WARNING,
                                   PerformanceAlert::Severity::CRITICAL});
```

### Analytics

```cpp
#include "solar_core/performance/analytics.hpp"

// KPI tracking
KPI kpi;
kpi.name = "p99_latency";
kpi.target_value = 100;
kpi.warning_threshold = 10;   // 10% deviation
kpi.critical_threshold = 25;  // 25% deviation
ReportingEngine::instance().define_kpi(kpi);

// Generate reports
auto report = ReportingEngine::instance().generate_report(
    "Daily Performance", start_time, end_time);
std::string json = ReportingEngine::instance().render_report_json(report);

// Predictive analytics
PredictiveAnalytics::instance().record_sample("memory_usage", 75.0);
auto forecast = PredictiveAnalytics::instance().forecast("memory_usage", 
    std::chrono::hours(24));
auto recommendations = PredictiveAnalytics::instance().get_capacity_recommendations();
```

### Profiling

```cpp
#include "solar_core/performance/profiling.hpp"

// Distributed tracing
auto trace_id = ExecutionTracer::instance().start_trace("request", "web");
{
    ScopedSpan span(trace_id, "database_query");
    span.add_tag("query_type", "select");
    // ... operation ...
}
ExecutionTracer::instance().end_span(trace_id);

// Query traces
auto slow_spans = ExecutionTracer::instance().get_slow_spans(100.0);  // > 100ms
auto jpl_spans = ExecutionTracer::instance().filter_by_component("jpl");

// Bottleneck analysis
BottleneckAnalyzer::instance().record_operation("parse", "jpl", 50.0, true);
auto bottlenecks = BottleneckAnalyzer::instance().identify_bottlenecks();
auto recommendations = BottleneckAnalyzer::instance().get_recommendations();
```

## API Reference

### Metric Types

#### Counter
```cpp
class Counter {
    void increment(double amount = 1.0);
    double get() const;
    void reset();
};
```

#### Gauge
```cpp
class Gauge {
    void set(double value);
    void increment(double amount = 1.0);
    void decrement(double amount = 1.0);
    double get() const;
};
```

#### Histogram
```cpp
class Histogram {
    void observe(double value);
    Statistics get_statistics() const;  // min, max, mean, median, p95, p99, count, sum
};
```

#### Timer
```cpp
class Timer {
    void record(double seconds);
    ScopedTimer time();  // RAII helper
    Statistics get_statistics() const;
};
```

### Dashboard API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/perf/metrics` | GET | All current metrics as JSON |
| `/api/perf/metrics/{component}` | GET | Component-specific metrics |
| `/api/perf/alerts` | GET | Active alerts |
| `/api/perf/query?name={metric}` | GET | Query specific metric |
| `/dashboard.html` | GET | Web dashboard UI |

### JSON Response Formats

**Metrics Response:**
```json
{
  "timestamp": 1706450000,
  "metrics": {"counter_name": 42},
  "jpl": {"avg_api_time": 0.5, "cache_hit_rate": 0.85},
  "simulation": {"avg_timestep": 0.001, "peak_memory": 1048576},
  "cache": {"hit_rate": 0.9, "compression_ratio": 0.5},
  "web": {"avg_response_time": 0.1, "fps": 60}
}
```

**Alerts Response:**
```json
{
  "alerts": [
    {"metric": "response_time", "severity": 2, "current_value": 150, 
     "threshold": 100, "message": "Response time exceeded threshold"}
  ]
}
```

## Dashboard Guide

### Accessing the Dashboard

1. Start the web server with performance endpoints enabled
2. Navigate to `http://localhost:8080/dashboard.html`
3. Dashboard auto-refreshes every 5 seconds

### Dashboard Features

- **Summary Cards**: Key metrics at a glance (JPL API time, cache hit rate, FPS, timesteps)
- **Alerts Panel**: Active alerts with severity indicators
- **Auto-refresh**: Configurable refresh interval
- **Manual Refresh**: Click "Refresh" button for immediate update

### Customizing the Dashboard

Edit `apps/solar_system_web/web/dashboard.html` to:
- Add new metric cards
- Change refresh interval
- Modify styling
- Add custom visualizations

## Configuration

### DashboardConfig

```cpp
DashboardConfig config;
config.port = 9090;                    // Dashboard port
config.bind_address = "0.0.0.0";       // Bind address
config.enable_auth = true;             // Enable API authentication
config.api_key = "your-secret-key";    // API key for auth
config.rate_limit_per_minute = 60;     // Rate limiting
config.refresh_interval = std::chrono::seconds(5);
```

### Threshold Configuration

```cpp
PerformanceThreshold threshold;
threshold.metric_name = "metric_name";
threshold.warning_threshold = 100.0;
threshold.critical_threshold = 200.0;
threshold.above_threshold = true;  // Alert when value > threshold
```

### System Monitor Thresholds

```cpp
SystemMonitor::instance().set_cpu_threshold(90.0);     // 90% CPU
SystemMonitor::instance().set_memory_threshold(85.0);  // 85% memory
SystemMonitor::instance().set_disk_threshold(90.0);    // 90% disk
```

## Troubleshooting

### Common Issues

#### No Metrics Appearing

1. Ensure monitoring is enabled:
   ```cpp
   PerformanceMonitor::instance().enable_monitoring(true);
   ```

2. Check that metrics are being recorded:
   ```cpp
   auto metrics = PerformanceMonitor::instance().get_all_metrics();
   ```

#### Alerts Not Triggering

1. Verify threshold configuration:
   ```cpp
   auto thresholds = PerformanceMonitor::instance().get_thresholds();
   ```

2. Check that metric values exceed thresholds

3. Ensure `above_threshold` is set correctly

#### Dashboard Not Loading

1. Verify web server is running
2. Check that `dashboard.html` exists in web root
3. Verify API endpoints are accessible:
   ```bash
   curl http://localhost:8080/api/perf/metrics
   ```

#### High Memory Usage

1. Check metric history limits:
   - Default: 10,000 samples per metric
   - Adjust `MAX_SAMPLES` if needed

2. Clear old data periodically:
   ```cpp
   PerformanceMonitor::instance().reset_all();
   ```

### Performance Impact

The monitoring system is designed for minimal overhead:

- Metric operations: < 1μs
- Memory per metric: ~100 bytes + samples
- Dashboard refresh: ~10ms

To reduce overhead:
- Disable tracing when not needed: `ExecutionTracer::instance().set_enabled(false)`
- Use sampling for high-frequency operations
- Limit history retention

### Logging

Enable verbose logging for debugging:
```cpp
// Component monitors log to standard logging system
LOG_DEBUG("perf", "Metric recorded: " + metric_name);
```

## Best Practices

1. **Use meaningful metric names**: `component.operation.measurement`
2. **Set appropriate thresholds**: Based on baseline measurements
3. **Monitor the monitors**: Track monitoring system overhead
4. **Regular baseline updates**: Update KPI targets as system evolves
5. **Alert fatigue prevention**: Use aggregation and correlation
