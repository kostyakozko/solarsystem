/**
 * @file profiling.cpp
 * @brief Debugging and profiling tools implementation
 */

#include "solar_core/performance/profiling.hpp"

#include <algorithm>
#include <sstream>

namespace SolarSystem::Performance {

// ExecutionTracer implementation
ExecutionTracer& ExecutionTracer::instance() {
  static ExecutionTracer instance;
  return instance;
}

std::string ExecutionTracer::generate_id() {
  std::ostringstream oss;
  oss << std::hex << id_counter_.fetch_add(1);
  return oss.str();
}

std::string ExecutionTracer::start_trace(const std::string& operation,
                                         const std::string& component) {
  if (!enabled_.load()) return "";

  std::string trace_id = generate_id();
  std::string span_id = trace_id;

  TraceSpan span;
  span.trace_id = trace_id;
  span.span_id = span_id;
  span.operation_name = operation;
  span.component = component;
  span.start_time = std::chrono::steady_clock::now();

  std::lock_guard<std::mutex> lock(mutex_);
  if (spans_.size() >= MAX_SPANS) {
    auto it = spans_.begin();
    spans_.erase(it);
  }
  spans_[span_id] = span;
  return trace_id;
}

std::string ExecutionTracer::start_span(const std::string& trace_id, const std::string& operation,
                                        const std::string& parent_span_id) {
  if (!enabled_.load()) return "";

  std::string span_id = generate_id();

  TraceSpan span;
  span.trace_id = trace_id;
  span.span_id = span_id;
  span.parent_span_id = parent_span_id;
  span.operation_name = operation;
  span.start_time = std::chrono::steady_clock::now();

  std::lock_guard<std::mutex> lock(mutex_);
  if (spans_.size() >= MAX_SPANS) {
    auto it = spans_.begin();
    spans_.erase(it);
  }
  spans_[span_id] = span;
  return span_id;
}

void ExecutionTracer::end_span(const std::string& span_id, bool success) {
  if (!enabled_.load() || span_id.empty()) return;

  std::lock_guard<std::mutex> lock(mutex_);
  auto it = spans_.find(span_id);
  if (it != spans_.end()) {
    it->second.end_time = std::chrono::steady_clock::now();
    it->second.success = success;
  }
}

void ExecutionTracer::add_tag(const std::string& span_id, const std::string& key,
                              const std::string& value) {
  if (span_id.empty()) return;

  std::lock_guard<std::mutex> lock(mutex_);
  auto it = spans_.find(span_id);
  if (it != spans_.end()) {
    it->second.tags[key] = value;
  }
}

std::vector<TraceSpan> ExecutionTracer::get_trace(const std::string& trace_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<TraceSpan> result;
  for (const auto& [id, span] : spans_) {
    if (span.trace_id == trace_id) {
      result.push_back(span);
    }
  }
  return result;
}

std::vector<TraceSpan> ExecutionTracer::get_recent_traces(size_t limit) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<TraceSpan> result;
  result.reserve(std::min(limit, spans_.size()));

  for (auto it = spans_.rbegin(); it != spans_.rend() && result.size() < limit; ++it) {
    result.push_back(it->second);
  }
  return result;
}

std::vector<TraceSpan> ExecutionTracer::filter_by_component(const std::string& component) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<TraceSpan> result;
  for (const auto& [id, span] : spans_) {
    if (span.component == component) {
      result.push_back(span);
    }
  }
  return result;
}

std::vector<TraceSpan> ExecutionTracer::filter_by_duration(double min_ms) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<TraceSpan> result;
  for (const auto& [id, span] : spans_) {
    if (span.duration_ms() >= min_ms) {
      result.push_back(span);
    }
  }
  return result;
}

std::vector<TraceSpan> ExecutionTracer::get_slow_spans(double threshold_ms) const {
  return filter_by_duration(threshold_ms);
}

void ExecutionTracer::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  spans_.clear();
}

void ExecutionTracer::set_enabled(bool enabled) { enabled_.store(enabled); }

bool ExecutionTracer::is_enabled() const { return enabled_.load(); }

// ScopedSpan implementation
ScopedSpan::ScopedSpan(const std::string& trace_id, const std::string& operation,
                       const std::string& parent_span_id) {
  span_id_ = ExecutionTracer::instance().start_span(trace_id, operation, parent_span_id);
}

ScopedSpan::~ScopedSpan() { ExecutionTracer::instance().end_span(span_id_, success_); }

void ScopedSpan::add_tag(const std::string& key, const std::string& value) {
  ExecutionTracer::instance().add_tag(span_id_, key, value);
}

// BottleneckAnalyzer implementation
BottleneckAnalyzer& BottleneckAnalyzer::instance() {
  static BottleneckAnalyzer instance;
  return instance;
}

void BottleneckAnalyzer::record_operation(const std::string& operation,
                                          const std::string& component, double duration_ms,
                                          bool cpu_intensive, bool io_intensive) {
  std::string key = component + "::" + operation;

  std::lock_guard<std::mutex> lock(mutex_);
  auto& stats = operations_[key];
  stats.operation = operation;
  stats.component = component;
  stats.total_time_ms += duration_ms;
  stats.count++;
  stats.avg_time_ms = stats.total_time_ms / stats.count;
  if (duration_ms > stats.max_time_ms) stats.max_time_ms = duration_ms;
  if (cpu_intensive) stats.cpu_intensive = true;
  if (io_intensive) stats.io_intensive = true;
}

void BottleneckAnalyzer::record_contention(const std::string& resource, double wait_time_ms) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto& [total, count] = contentions_[resource];
  total += wait_time_ms;
  count++;
}

std::vector<Bottleneck> BottleneckAnalyzer::identify_bottlenecks() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Bottleneck> result;

  // Calculate total time for impact scoring
  double total_time = 0;
  for (const auto& [key, stats] : operations_) {
    total_time += stats.total_time_ms;
  }
  if (total_time == 0) return result;

  // Identify operation bottlenecks
  for (const auto& [key, stats] : operations_) {
    double impact = (stats.total_time_ms / total_time) * 100;
    if (impact < 5) continue;  // Skip low-impact operations

    Bottleneck b;
    b.operation = stats.operation;
    b.component = stats.component;
    b.impact_score = impact;
    b.avg_duration_ms = stats.avg_time_ms;
    b.occurrence_count = stats.count;

    if (stats.cpu_intensive) {
      b.type = Bottleneck::Type::CPU_BOUND;
      b.recommendation = "Consider parallelization or algorithm optimization";
    } else if (stats.io_intensive) {
      b.type = Bottleneck::Type::IO_BOUND;
      b.recommendation = "Consider async I/O, caching, or batching";
    } else if (stats.avg_time_ms > 100) {
      b.type = Bottleneck::Type::SLOW_OPERATION;
      b.recommendation = "Profile and optimize this operation";
    } else {
      b.type = Bottleneck::Type::CPU_BOUND;
      b.recommendation = "High call frequency - consider caching results";
    }

    result.push_back(b);
  }

  // Identify contention bottlenecks
  for (const auto& [resource, data] : contentions_) {
    auto [total_wait, count] = data;
    double avg_wait = total_wait / count;
    if (avg_wait < 1) continue;

    Bottleneck b;
    b.operation = resource;
    b.component = "contention";
    b.type = Bottleneck::Type::CONTENTION;
    b.impact_score = std::min(100.0, (total_wait / total_time) * 100);
    b.avg_duration_ms = avg_wait;
    b.occurrence_count = count;
    b.recommendation = "Reduce lock scope or use lock-free data structures";
    result.push_back(b);
  }

  // Sort by impact
  std::sort(result.begin(), result.end(), [](const Bottleneck& a, const Bottleneck& b) {
    return a.impact_score > b.impact_score;
  });

  return result;
}

std::vector<Bottleneck> BottleneckAnalyzer::get_hotspots(size_t limit) const {
  auto bottlenecks = identify_bottlenecks();
  if (bottlenecks.size() > limit) {
    bottlenecks.resize(limit);
  }
  return bottlenecks;
}

std::vector<std::string> BottleneckAnalyzer::get_recommendations() const {
  auto bottlenecks = identify_bottlenecks();
  std::vector<std::string> recommendations;
  for (const auto& b : bottlenecks) {
    if (!b.recommendation.empty()) {
      recommendations.push_back(b.component + "::" + b.operation + ": " + b.recommendation);
    }
  }
  return recommendations;
}

std::vector<BottleneckAnalyzer::OperationStats> BottleneckAnalyzer::get_operation_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<OperationStats> result;
  result.reserve(operations_.size());
  for (const auto& [key, stats] : operations_) {
    result.push_back(stats);
  }
  std::sort(result.begin(), result.end(), [](const OperationStats& a, const OperationStats& b) {
    return a.total_time_ms > b.total_time_ms;
  });
  return result;
}

void BottleneckAnalyzer::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  operations_.clear();
  contentions_.clear();
}

}  // namespace SolarSystem::Performance
