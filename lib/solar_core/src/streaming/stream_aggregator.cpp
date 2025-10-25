#include "solar_core/streaming/stream_aggregator.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <unordered_set>

namespace SolarSystem::Streaming {

// AggregateSnapshot implementation
std::string AggregateSnapshot::to_string() const {
  std::ostringstream oss;
  oss << "AggregateSnapshot {\n";
  oss << "  Timestamp: " << std::chrono::duration_cast<std::chrono::seconds>(
      timestamp.time_since_epoch()).count() << "\n";
  oss << "  Total Bodies: " << total_bodies << "\n";
  oss << "  Total Samples: " << total_samples << "\n";
  oss << "  Overall Quality: " << overall_avg_quality << "\n";
  oss << "  Overall Latency: " << overall_avg_latency.count() << "ms\n";
  oss << "}";
  return oss.str();
}

// TimeWindowAggregator implementation
TimeWindowAggregator::TimeWindowAggregator(std::chrono::milliseconds window_duration)
    : window_duration_(window_duration) {}

void TimeWindowAggregator::add_snapshot(const DataSnapshot& snapshot) {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);

  snapshots_.push_back(snapshot);
  cleanup_old_snapshots();
}

AggregateSnapshot TimeWindowAggregator::get_aggregate() const {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);

  AggregateSnapshot aggregate;
  aggregate.timestamp = std::chrono::system_clock::now();

  if (snapshots_.empty()) {
    return aggregate;
  }

  aggregate.window_start = snapshots_.front().timestamp;
  aggregate.window_end = snapshots_.back().timestamp;
  aggregate.total_samples = snapshots_.size();

  // Calculate overall statistics
  double total_quality = 0.0;
  std::chrono::milliseconds total_latency{0};
  std::unordered_set<std::string> unique_bodies;

  for (const auto& snapshot : snapshots_) {
    total_quality += snapshot.overall_quality;
    total_latency += snapshot.processing_time;

    for (const auto& point : snapshot.data_points) {
      unique_bodies.insert(point.body_name);
    }
  }

  aggregate.total_bodies = unique_bodies.size();
  aggregate.overall_avg_quality = total_quality / snapshots_.size();
  aggregate.overall_avg_latency = total_latency / snapshots_.size();

  return aggregate;
}

void TimeWindowAggregator::reset() {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);
  snapshots_.clear();
}

bool TimeWindowAggregator::has_sufficient_data() const {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);
  return snapshots_.size() >= min_samples_;
}

void TimeWindowAggregator::set_window_duration(std::chrono::milliseconds duration) {
  window_duration_ = duration;
  std::lock_guard<std::mutex> lock(snapshots_mutex_);
  cleanup_old_snapshots();
}

void TimeWindowAggregator::cleanup_old_snapshots() {
  auto now = std::chrono::system_clock::now();
  auto cutoff_time = now - window_duration_;

  snapshots_.erase(
      std::remove_if(snapshots_.begin(), snapshots_.end(),
                     [cutoff_time](const DataSnapshot& snapshot) {
                       return snapshot.timestamp < cutoff_time;
                     }),
      snapshots_.end());
}

BodyAggregateData TimeWindowAggregator::calculate_body_aggregate(const std::string& body_name) const {
  BodyAggregateData aggregate;
  aggregate.body_name = body_name;

  auto data_points = get_body_data_points(body_name);
  if (data_points.empty()) {
    return aggregate;
  }

  aggregate.sample_count = data_points.size();

  // Calculate position statistics
  Math::Vector3d pos_sum{};
  for (const auto* point : data_points) {
    pos_sum = pos_sum + point->position;
  }
  aggregate.avg_position = pos_sum * (1.0 / data_points.size());

  // Calculate quality statistics
  double quality_sum = 0.0;
  for (const auto* point : data_points) {
    quality_sum += point->quality_score;
  }
  aggregate.avg_quality = quality_sum / data_points.size();

  if (!data_points.empty()) {
    aggregate.first_sample_time = data_points.front()->timestamp;
    aggregate.last_sample_time = data_points.back()->timestamp;
  }

  return aggregate;
}

std::vector<const DataPoint*> TimeWindowAggregator::get_body_data_points(const std::string& body_name) const {
  std::vector<const DataPoint*> points;

  for (const auto& snapshot : snapshots_) {
    for (const auto& point : snapshot.data_points) {
      if (point.body_name == body_name) {
        points.push_back(&point);
      }
    }
  }

  return points;
}

// SampleCountAggregator implementation
SampleCountAggregator::SampleCountAggregator(size_t max_samples) : max_samples_(max_samples) {}

void SampleCountAggregator::add_snapshot(const DataSnapshot& snapshot) {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);

  snapshots_.push_back(snapshot);
  maintain_sample_limit();
}

AggregateSnapshot SampleCountAggregator::get_aggregate() const {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);

  AggregateSnapshot aggregate;
  aggregate.timestamp = std::chrono::system_clock::now();

  if (snapshots_.empty()) {
    return aggregate;
  }

  aggregate.window_start = snapshots_.front().timestamp;
  aggregate.window_end = snapshots_.back().timestamp;
  aggregate.total_samples = snapshots_.size();

  // Calculate overall statistics (simplified)
  double total_quality = 0.0;
  std::unordered_set<std::string> unique_bodies;

  for (const auto& snapshot : snapshots_) {
    total_quality += snapshot.overall_quality;
    for (const auto& point : snapshot.data_points) {
      unique_bodies.insert(point.body_name);
    }
  }

  aggregate.total_bodies = unique_bodies.size();
  aggregate.overall_avg_quality = total_quality / snapshots_.size();

  return aggregate;
}

void SampleCountAggregator::reset() {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);
  snapshots_.clear();
}

bool SampleCountAggregator::has_sufficient_data() const {
  std::lock_guard<std::mutex> lock(snapshots_mutex_);
  return snapshots_.size() >= min_samples_;
}

void SampleCountAggregator::set_max_samples(size_t max_samples) {
  max_samples_ = max_samples;
  std::lock_guard<std::mutex> lock(snapshots_mutex_);
  maintain_sample_limit();
}

void SampleCountAggregator::maintain_sample_limit() {
  while (snapshots_.size() > max_samples_) {
    snapshots_.pop_front();
  }
}

BodyAggregateData SampleCountAggregator::calculate_body_aggregate(const std::string& body_name) const {
  // Simplified implementation
  BodyAggregateData aggregate;
  aggregate.body_name = body_name;
  return aggregate;
}

std::vector<const DataPoint*> SampleCountAggregator::get_body_data_points(const std::string& ) const {
  // Simplified implementation
  return {};
}

// RealtimeAggregator implementation
RealtimeAggregator::RealtimeAggregator(std::chrono::milliseconds update_interval)
    : update_interval_(update_interval) {}

void RealtimeAggregator::add_snapshot(const DataSnapshot& snapshot) {
  std::lock_guard<std::mutex> lock(stats_mutex_);

  // Update running statistics for each body
  for (const auto& point : snapshot.data_points) {
    auto& stats = body_stats_[point.body_name];
    stats.add_data_point(point);
  }

  aggregate_dirty_ = true;
}

AggregateSnapshot RealtimeAggregator::get_aggregate() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);

  if (aggregate_dirty_ || is_aggregate_stale()) {
    update_cached_aggregate();
  }

  return cached_aggregate_;
}

void RealtimeAggregator::reset() {
  std::lock_guard<std::mutex> lock(stats_mutex_);

  for (auto& [name, stats] : body_stats_) {
    stats.reset();
  }

  aggregate_dirty_ = true;
}

bool RealtimeAggregator::has_sufficient_data() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  return !body_stats_.empty();
}

bool RealtimeAggregator::is_aggregate_stale() const {
  auto now = std::chrono::system_clock::now();
  return (now - last_update_time_) > update_interval_;
}

void RealtimeAggregator::force_update() {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  update_cached_aggregate();
}

void RealtimeAggregator::update_cached_aggregate() const {
  cached_aggregate_ = AggregateSnapshot{};
  cached_aggregate_.timestamp = std::chrono::system_clock::now();
  cached_aggregate_.total_bodies = body_stats_.size();

  // Calculate aggregate statistics
  if (!body_stats_.empty()) {
    double total_quality = 0.0;
    size_t total_samples = 0;

    for (const auto& [name, stats] : body_stats_) {
      if (stats.count > 0) {
        total_quality += stats.quality_sum / stats.count;
        total_samples += stats.count;
      }
    }

    cached_aggregate_.overall_avg_quality = total_quality / body_stats_.size();
    cached_aggregate_.total_samples = total_samples;
  }

  last_update_time_ = cached_aggregate_.timestamp;
  aggregate_dirty_ = false;
}

// RunningStats implementation
void RealtimeAggregator::RunningStats::add_data_point(const DataPoint& point) {
  count++;
  position_sum = position_sum + point.position;
  velocity_sum = velocity_sum + point.velocity;
  quality_sum += point.quality_score;
  latency_sum += point.latency;

  if (count == 1) {
    first_time = point.timestamp;
    min_position = point.position;
    max_position = point.position;
    min_velocity = point.velocity;
    max_velocity = point.velocity;
    min_quality = point.quality_score;
  } else {
    // Update min/max values (simplified)
    if (point.quality_score < min_quality) {
      min_quality = point.quality_score;
    }
  }

  last_time = point.timestamp;

  if (point.latency > max_latency) {
    max_latency = point.latency;
  }
}

BodyAggregateData RealtimeAggregator::RunningStats::to_aggregate_data(const std::string& body_name) const {
  BodyAggregateData aggregate;
  aggregate.body_name = body_name;
  aggregate.sample_count = count;

  if (count > 0) {
    aggregate.avg_position = position_sum * (1.0 / count);
    aggregate.avg_velocity = velocity_sum * (1.0 / count);
    aggregate.avg_quality = quality_sum / count;
    aggregate.min_quality = min_quality;
    aggregate.avg_latency = latency_sum / count;
    aggregate.max_latency = max_latency;
    aggregate.first_sample_time = first_time;
    aggregate.last_sample_time = last_time;
  }

  return aggregate;
}

void RealtimeAggregator::RunningStats::reset() {
  *this = RunningStats{};
}

// CallbackAggregator implementation
CallbackAggregator::CallbackAggregator(std::unique_ptr<StreamAggregator> base_aggregator)
    : base_aggregator_(std::move(base_aggregator)) {}

void CallbackAggregator::add_snapshot(const DataSnapshot& snapshot) {
  if (base_aggregator_) {
    base_aggregator_->add_snapshot(snapshot);

    if (should_trigger_callback()) {
      auto aggregate = base_aggregator_->get_aggregate();

      if (aggregate_callback_) {
        aggregate_callback_(aggregate);
      }

      check_thresholds(aggregate);
      last_callback_time_ = std::chrono::system_clock::now();
    }
  }
}

AggregateSnapshot CallbackAggregator::get_aggregate() const {
  return base_aggregator_ ? base_aggregator_->get_aggregate() : AggregateSnapshot{};
}

void CallbackAggregator::reset() {
  if (base_aggregator_) {
    base_aggregator_->reset();
  }
  last_callback_time_ = std::chrono::system_clock::time_point{};
}

std::string CallbackAggregator::get_name() const {
  return "CallbackAggregator[" + (base_aggregator_ ? base_aggregator_->get_name() : "null") + "]";
}

bool CallbackAggregator::has_sufficient_data() const {
  return base_aggregator_ ? base_aggregator_->has_sufficient_data() : false;
}

void CallbackAggregator::check_thresholds(const AggregateSnapshot& aggregate) {
  if (!threshold_callback_) {
    return;
  }

  // Check quality thresholds for each body
  for (const auto& body_aggregate : aggregate.body_aggregates) {
    if (body_aggregate.avg_quality < quality_threshold_) {
      threshold_callback_(body_aggregate.body_name, body_aggregate);
    }

    if (body_aggregate.avg_latency > latency_threshold_) {
      threshold_callback_(body_aggregate.body_name, body_aggregate);
    }
  }
}

bool CallbackAggregator::should_trigger_callback() const {
  if (last_callback_time_ == std::chrono::system_clock::time_point{}) {
    return true;  // First callback
  }

  auto now = std::chrono::system_clock::now();
  return (now - last_callback_time_) >= callback_interval_;
}

// AggregatorFactory implementation
std::unique_ptr<TimeWindowAggregator> AggregatorFactory::create_time_window_aggregator(
    std::chrono::milliseconds window_duration) {
  return std::make_unique<TimeWindowAggregator>(window_duration);
}

std::unique_ptr<SampleCountAggregator> AggregatorFactory::create_sample_count_aggregator(
    size_t max_samples) {
  return std::make_unique<SampleCountAggregator>(max_samples);
}

std::unique_ptr<RealtimeAggregator> AggregatorFactory::create_realtime_aggregator(
    std::chrono::milliseconds update_interval) {
  return std::make_unique<RealtimeAggregator>(update_interval);
}

std::unique_ptr<CallbackAggregator> AggregatorFactory::create_callback_aggregator(
    std::unique_ptr<StreamAggregator> base_aggregator) {
  return std::make_unique<CallbackAggregator>(std::move(base_aggregator));
}

std::unique_ptr<StreamAggregator> AggregatorFactory::create_performance_monitoring_aggregator() {
  return create_realtime_aggregator(std::chrono::seconds{1});
}

std::unique_ptr<StreamAggregator> AggregatorFactory::create_quality_monitoring_aggregator() {
  return create_time_window_aggregator(std::chrono::minutes{5});
}

std::unique_ptr<StreamAggregator> AggregatorFactory::create_scientific_analysis_aggregator() {
  return create_sample_count_aggregator(1000);
}

}  // namespace SolarSystem::Streaming
