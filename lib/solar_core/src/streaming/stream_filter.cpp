#include "solar_core/streaming/stream_filter.hpp"

#include <algorithm>
#include <sstream>

namespace SolarSystem::Streaming {

// BodySelectionFilter implementation
BodySelectionFilter::BodySelectionFilter(std::vector<std::string> selected_bodies)
    : selected_bodies_(selected_bodies.begin(), selected_bodies.end()) {}

BodySelectionFilter::BodySelectionFilter(std::unordered_set<std::string> selected_bodies)
    : selected_bodies_(std::move(selected_bodies)) {}

std::optional<DataSnapshot> BodySelectionFilter::apply(const DataSnapshot& snapshot) {
  if (!enabled_ || selected_bodies_.empty()) {
    return snapshot;
  }

  DataSnapshot filtered_snapshot = snapshot;
  filtered_snapshot.data_points.clear();

  // Filter data points based on selected bodies
  for (const auto& point : snapshot.data_points) {
    if (selected_bodies_.find(point.body_name) != selected_bodies_.end()) {
      filtered_snapshot.data_points.push_back(point);
    }
  }

  // Update snapshot metadata
  if (filtered_snapshot.data_points.empty()) {
    return std::nullopt;  // Drop snapshot if no bodies match
  }

  // Recalculate overall quality
  double total_quality = 0.0;
  for (const auto& point : filtered_snapshot.data_points) {
    total_quality += point.quality_score;
  }
  filtered_snapshot.overall_quality =
      total_quality / static_cast<double>(filtered_snapshot.data_points.size());

  return filtered_snapshot;
}

void BodySelectionFilter::add_body(const std::string& body_name) {
  selected_bodies_.insert(body_name);
}

void BodySelectionFilter::remove_body(const std::string& body_name) {
  selected_bodies_.erase(body_name);
}

void BodySelectionFilter::clear_bodies() { selected_bodies_.clear(); }

const std::unordered_set<std::string>& BodySelectionFilter::get_selected_bodies() const noexcept {
  return selected_bodies_;
}

// QualityFilter implementation
QualityFilter::QualityFilter(double min_quality) : min_quality_(min_quality) {}

std::optional<DataSnapshot> QualityFilter::apply(const DataSnapshot& snapshot) {
  if (!enabled_) {
    return snapshot;
  }

  // Check overall quality first
  if (drop_entire_snapshot_ && snapshot.overall_quality < min_quality_) {
    return std::nullopt;
  }

  DataSnapshot filtered_snapshot = snapshot;
  filtered_snapshot.data_points.clear();

  // Filter individual data points
  for (const auto& point : snapshot.data_points) {
    if (point.quality_score >= min_quality_) {
      filtered_snapshot.data_points.push_back(point);
    }
  }

  if (filtered_snapshot.data_points.empty()) {
    return std::nullopt;
  }

  // Recalculate overall quality
  double total_quality = 0.0;
  for (const auto& point : filtered_snapshot.data_points) {
    total_quality += point.quality_score;
  }
  filtered_snapshot.overall_quality =
      total_quality / static_cast<double>(filtered_snapshot.data_points.size());

  return filtered_snapshot;
}

// LatencyFilter implementation
LatencyFilter::LatencyFilter(std::chrono::milliseconds max_latency) : max_latency_(max_latency) {}

std::optional<DataSnapshot> LatencyFilter::apply(const DataSnapshot& snapshot) {
  if (!enabled_) {
    return snapshot;
  }

  DataSnapshot filtered_snapshot = snapshot;
  filtered_snapshot.data_points.clear();

  // Filter data points based on latency
  for (const auto& point : snapshot.data_points) {
    if (point.latency <= max_latency_) {
      filtered_snapshot.data_points.push_back(point);
    }
  }

  if (filtered_snapshot.data_points.empty()) {
    return std::nullopt;
  }

  // Recalculate overall quality
  double total_quality = 0.0;
  for (const auto& point : filtered_snapshot.data_points) {
    total_quality += point.quality_score;
  }
  filtered_snapshot.overall_quality =
      total_quality / static_cast<double>(filtered_snapshot.data_points.size());

  return filtered_snapshot;
}

// RateLimitFilter implementation
RateLimitFilter::RateLimitFilter(std::chrono::milliseconds min_interval)
    : min_interval_(min_interval) {}

std::optional<DataSnapshot> RateLimitFilter::apply(const DataSnapshot& snapshot) {
  if (!enabled_) {
    return snapshot;
  }

  auto now = snapshot.timestamp;

  // Check if enough time has passed since last snapshot
  if (last_passed_time_ != std::chrono::system_clock::time_point{}) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_passed_time_);
    if (elapsed < min_interval_) {
      return std::nullopt;  // Drop snapshot due to rate limiting
    }
  }

  last_passed_time_ = now;
  return snapshot;
}

// DuplicationFilter implementation
DuplicationFilter::DuplicationFilter(double position_tolerance, double velocity_tolerance)
    : position_tolerance_(position_tolerance), velocity_tolerance_(velocity_tolerance) {}

std::optional<DataSnapshot> DuplicationFilter::apply(const DataSnapshot& snapshot) {
  if (!enabled_ || !last_snapshot_) {
    last_snapshot_ = snapshot;
    return snapshot;
  }

  // Check if current snapshot is significantly different from the last one
  if (is_significantly_different(snapshot, *last_snapshot_)) {
    last_snapshot_ = snapshot;
    return snapshot;
  }

  // Snapshot is too similar, drop it
  return std::nullopt;
}

bool DuplicationFilter::is_significantly_different(const DataSnapshot& current,
                                                   const DataSnapshot& previous) const {
  // If different number of bodies, it's definitely different
  if (current.data_points.size() != previous.data_points.size()) {
    return true;
  }

  // Check each body for significant changes
  for (const auto& current_point : current.data_points) {
    // Find corresponding body in previous snapshot
    auto prev_it = std::find_if(previous.data_points.begin(), previous.data_points.end(),
                                [&current_point](const DataPoint& prev_point) {
                                  return prev_point.body_name == current_point.body_name;
                                });

    if (prev_it == previous.data_points.end()) {
      return true;  // Body not found in previous snapshot
    }

    if (is_body_different(current_point, *prev_it)) {
      return true;
    }
  }

  return false;  // No significant differences found
}

bool DuplicationFilter::is_body_different(const DataPoint& current,
                                          const DataPoint& previous) const {
  // Check position difference
  auto pos_diff = current.position - previous.position;
  double pos_magnitude = static_cast<double>(std::sqrt(
      pos_diff.x() * pos_diff.x() + pos_diff.y() * pos_diff.y() + pos_diff.z() * pos_diff.z()));

  if (pos_magnitude > position_tolerance_) {
    return true;
  }

  // Check velocity difference
  auto vel_diff = current.velocity - previous.velocity;
  double vel_magnitude = static_cast<double>(std::sqrt(
      vel_diff.x() * vel_diff.x() + vel_diff.y() * vel_diff.y() + vel_diff.z() * vel_diff.z()));

  if (vel_magnitude > velocity_tolerance_) {
    return true;
  }

  return false;
}

// PredicateFilter implementation
PredicateFilter::PredicateFilter(SnapshotPredicate predicate, std::string name)
    : snapshot_predicate_(std::move(predicate)), name_(std::move(name)) {}

PredicateFilter::PredicateFilter(DataPointPredicate predicate, std::string name)
    : datapoint_predicate_(std::move(predicate)), name_(std::move(name)) {}

std::optional<DataSnapshot> PredicateFilter::apply(const DataSnapshot& snapshot) {
  if (!enabled_) {
    return snapshot;
  }

  if (snapshot_predicate_) {
    // Apply snapshot-level predicate
    return snapshot_predicate_(snapshot) ? std::make_optional(snapshot) : std::nullopt;
  }

  if (datapoint_predicate_) {
    // Apply data point-level predicate
    DataSnapshot filtered_snapshot = snapshot;
    filtered_snapshot.data_points.clear();

    for (const auto& point : snapshot.data_points) {
      if (datapoint_predicate_(point)) {
        filtered_snapshot.data_points.push_back(point);
      }
    }

    if (filtered_snapshot.data_points.empty()) {
      return std::nullopt;
    }

    // Recalculate overall quality
    double total_quality = 0.0;
    for (const auto& point : filtered_snapshot.data_points) {
      total_quality += point.quality_score;
    }
    filtered_snapshot.overall_quality =
        total_quality / static_cast<double>(filtered_snapshot.data_points.size());

    return filtered_snapshot;
  }

  // No predicate set, pass through
  return snapshot;
}

// FilterChain implementation
FilterChain::FilterChain(std::vector<std::unique_ptr<StreamFilter>> filters)
    : filters_(std::move(filters)) {}

std::optional<DataSnapshot> FilterChain::apply(const DataSnapshot& snapshot) {
  if (!enabled_) {
    return snapshot;
  }

  std::optional<DataSnapshot> current_snapshot = snapshot;

  // Apply each filter in sequence
  for (const auto& filter : filters_) {
    if (!current_snapshot) {
      break;  // Snapshot was dropped by a previous filter
    }

    if (filter && filter->is_enabled()) {
      current_snapshot = filter->apply(*current_snapshot);
    }
  }

  return current_snapshot;
}

std::string FilterChain::get_name() const {
  std::ostringstream oss;
  oss << "FilterChain[";
  for (size_t i = 0; i < filters_.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << (filters_[i] ? filters_[i]->get_name() : "null");
  }
  oss << "]";
  return oss.str();
}

void FilterChain::add_filter(std::unique_ptr<StreamFilter> filter) {
  if (filter) {
    filters_.push_back(std::move(filter));
  }
}

void FilterChain::remove_filter(const std::string& filter_name) {
  filters_.erase(std::remove_if(filters_.begin(), filters_.end(),
                                [&filter_name](const std::unique_ptr<StreamFilter>& filter) {
                                  return filter && filter->get_name() == filter_name;
                                }),
                 filters_.end());
}

void FilterChain::clear_filters() { filters_.clear(); }

const std::vector<std::unique_ptr<StreamFilter>>& FilterChain::get_filters() const noexcept {
  return filters_;
}

void FilterChain::enable_filter(const std::string& filter_name) {
  auto* filter = find_filter(filter_name);
  if (filter) {
    filter->set_enabled(true);
  }
}

void FilterChain::disable_filter(const std::string& filter_name) {
  auto* filter = find_filter(filter_name);
  if (filter) {
    filter->set_enabled(false);
  }
}

StreamFilter* FilterChain::find_filter(const std::string& name) {
  auto it = std::find_if(filters_.begin(), filters_.end(),
                         [&name](const std::unique_ptr<StreamFilter>& filter) {
                           return filter && filter->get_name() == name;
                         });
  return (it != filters_.end()) ? it->get() : nullptr;
}

// FilterFactory implementation
std::unique_ptr<FilterChain> FilterFactory::create_basic_filter_chain() {
  auto chain = std::make_unique<FilterChain>();
  chain->add_filter(create_quality_filter(0.5));
  chain->add_filter(create_latency_filter(std::chrono::seconds{5}));
  return chain;
}

std::unique_ptr<FilterChain> FilterFactory::create_high_quality_filter_chain() {
  auto chain = std::make_unique<FilterChain>();
  chain->add_filter(create_quality_filter(0.8));
  chain->add_filter(create_latency_filter(std::chrono::seconds{2}));
  chain->add_filter(create_duplication_filter(500.0, 0.5));
  return chain;
}

std::unique_ptr<FilterChain> FilterFactory::create_performance_filter_chain() {
  auto chain = std::make_unique<FilterChain>();
  chain->add_filter(create_rate_limit_filter(std::chrono::milliseconds{200}));
  chain->add_filter(create_duplication_filter(2000.0, 2.0));
  chain->add_filter(create_quality_filter(0.3));
  return chain;
}

std::unique_ptr<FilterChain> FilterFactory::create_realtime_filter_chain() {
  auto chain = std::make_unique<FilterChain>();
  chain->add_filter(create_quality_filter(0.6));
  chain->add_filter(create_latency_filter(std::chrono::seconds{3}));
  chain->add_filter(create_rate_limit_filter(std::chrono::milliseconds{100}));
  return chain;
}

std::unique_ptr<BodySelectionFilter> FilterFactory::create_body_selection_filter(
    const std::vector<std::string>& bodies) {
  return std::make_unique<BodySelectionFilter>(bodies);
}

std::unique_ptr<QualityFilter> FilterFactory::create_quality_filter(double min_quality) {
  return std::make_unique<QualityFilter>(min_quality);
}

std::unique_ptr<LatencyFilter> FilterFactory::create_latency_filter(
    std::chrono::milliseconds max_latency) {
  return std::make_unique<LatencyFilter>(max_latency);
}

std::unique_ptr<RateLimitFilter> FilterFactory::create_rate_limit_filter(
    std::chrono::milliseconds min_interval) {
  return std::make_unique<RateLimitFilter>(min_interval);
}

std::unique_ptr<DuplicationFilter> FilterFactory::create_duplication_filter(
    double position_tolerance, double velocity_tolerance) {
  return std::make_unique<DuplicationFilter>(position_tolerance, velocity_tolerance);
}

}  // namespace SolarSystem::Streaming
