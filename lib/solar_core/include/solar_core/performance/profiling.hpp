/**
 * @file profiling.hpp
 * @brief Debugging and profiling tools for performance analysis
 */

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "solar_core/export.hpp"

namespace SolarSystem::Performance {

/**
 * @brief Single trace span representing an operation
 */
struct TraceSpan {
  std::string trace_id;
  std::string span_id;
  std::string parent_span_id;
  std::string operation_name;
  std::string component;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
  std::map<std::string, std::string> tags;
  bool success = true;

  double duration_ms() const {
    return std::chrono::duration<double, std::milli>(end_time - start_time).count();
  }
};

/**
 * @brief Execution tracer for distributed tracing (Task 9.1)
 */
class SOLAR_CORE_API ExecutionTracer {
 public:
  static ExecutionTracer& instance();

  // Trace management
  std::string start_trace(const std::string& operation, const std::string& component = "");
  std::string start_span(const std::string& trace_id, const std::string& operation,
                         const std::string& parent_span_id = "");
  void end_span(const std::string& span_id, bool success = true);
  void add_tag(const std::string& span_id, const std::string& key, const std::string& value);

  // Query traces
  std::vector<TraceSpan> get_trace(const std::string& trace_id) const;
  std::vector<TraceSpan> get_recent_traces(size_t limit = 100) const;
  std::vector<TraceSpan> filter_by_component(const std::string& component) const;
  std::vector<TraceSpan> filter_by_duration(double min_ms) const;

  // Analysis
  std::vector<TraceSpan> get_slow_spans(double threshold_ms) const;

  void clear();
  void set_enabled(bool enabled);
  bool is_enabled() const;

 private:
  ExecutionTracer() = default;
  std::string generate_id();

  mutable std::mutex mutex_;
  std::map<std::string, TraceSpan> spans_;
  std::atomic<bool> enabled_{true};
  std::atomic<uint64_t> id_counter_{0};
  static constexpr size_t MAX_SPANS = 10000;
};

/**
 * @brief RAII helper for automatic span management
 */
class SOLAR_CORE_API ScopedSpan {
 public:
  ScopedSpan(const std::string& trace_id, const std::string& operation,
             const std::string& parent_span_id = "");
  ~ScopedSpan();

  const std::string& span_id() const { return span_id_; }
  void set_success(bool success) { success_ = success; }
  void add_tag(const std::string& key, const std::string& value);

 private:
  std::string span_id_;
  bool success_ = true;
};

/**
 * @brief Bottleneck information
 */
struct Bottleneck {
  enum class Type { CPU_BOUND, MEMORY_BOUND, IO_BOUND, CONTENTION, SLOW_OPERATION };

  std::string operation;
  std::string component;
  Type type;
  double impact_score;  // 0-100, higher = more impact
  double avg_duration_ms;
  size_t occurrence_count;
  std::string recommendation;
};

/**
 * @brief Bottleneck identifier and analyzer (Task 9.2)
 */
class SOLAR_CORE_API BottleneckAnalyzer {
 public:
  static BottleneckAnalyzer& instance();

  // Record operations for analysis
  void record_operation(const std::string& operation, const std::string& component,
                        double duration_ms, bool cpu_intensive = false, bool io_intensive = false);
  void record_contention(const std::string& resource, double wait_time_ms);

  // Analysis
  std::vector<Bottleneck> identify_bottlenecks() const;
  std::vector<Bottleneck> get_hotspots(size_t limit = 10) const;
  std::vector<std::string> get_recommendations() const;

  // Statistics
  struct OperationStats {
    std::string operation;
    std::string component;
    double total_time_ms = 0;
    double avg_time_ms = 0;
    double max_time_ms = 0;
    size_t count = 0;
    bool cpu_intensive = false;
    bool io_intensive = false;
  };
  std::vector<OperationStats> get_operation_stats() const;

  void clear();

 private:
  BottleneckAnalyzer() = default;

  mutable std::mutex mutex_;
  std::map<std::string, OperationStats> operations_;
  std::map<std::string, std::pair<double, size_t>> contentions_;  // resource -> (total_wait, count)
};

// Convenience macros
#define TRACE_SPAN(trace_id, operation) \
  SolarSystem::Performance::ScopedSpan __trace_span_##__LINE__(trace_id, operation)

#define TRACE_SPAN_PARENT(trace_id, operation, parent) \
  SolarSystem::Performance::ScopedSpan __trace_span_##__LINE__(trace_id, operation, parent)

#define RECORD_OPERATION(op, component, duration_ms)                                       \
  SolarSystem::Performance::BottleneckAnalyzer::instance().record_operation(op, component, \
                                                                            duration_ms)

}  // namespace SolarSystem::Performance
