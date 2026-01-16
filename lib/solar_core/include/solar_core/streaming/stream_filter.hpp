#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/export.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_core/streaming/data_stream.hpp"

namespace SolarSystem::Streaming {

/**
 * @brief Base class for data stream filters
 */
class SOLAR_CORE_API StreamFilter {
 public:
  virtual ~StreamFilter() = default;

  /**
   * @brief Apply filter to a data snapshot
   * @param snapshot Input snapshot to filter
   * @return Filtered snapshot, or nullopt if snapshot should be dropped
   */
  [[nodiscard]] virtual std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) = 0;

  /**
   * @brief Get filter name for debugging/logging
   */
  [[nodiscard]] virtual std::string get_name() const = 0;

  /**
   * @brief Check if filter is enabled
   */
  [[nodiscard]] virtual bool is_enabled() const noexcept { return enabled_; }

  /**
   * @brief Enable or disable filter
   */
  virtual void set_enabled(bool enabled) noexcept { enabled_ = enabled; }

 protected:
  bool enabled_ = true;
};

/**
 * @brief Filter that selects only specific celestial bodies
 */
class SOLAR_CORE_API BodySelectionFilter : public StreamFilter {
 public:
  explicit BodySelectionFilter(std::vector<std::string> selected_bodies);
  explicit BodySelectionFilter(std::unordered_set<std::string> selected_bodies);

  [[nodiscard]] std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) override;
  [[nodiscard]] std::string get_name() const override { return "BodySelectionFilter"; }

  // Configuration
  void add_body(const std::string& body_name);
  void remove_body(const std::string& body_name);
  void clear_bodies();
  [[nodiscard]] const std::unordered_set<std::string>& get_selected_bodies() const noexcept;

 private:
  std::unordered_set<std::string> selected_bodies_;
};

/**
 * @brief Filter that removes data points below a quality threshold
 */
class SOLAR_CORE_API QualityFilter : public StreamFilter {
 public:
  explicit QualityFilter(double min_quality = 0.7);

  [[nodiscard]] std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) override;
  [[nodiscard]] std::string get_name() const override { return "QualityFilter"; }

  // Configuration
  void set_min_quality(double quality) noexcept { min_quality_ = quality; }
  [[nodiscard]] double get_min_quality() const noexcept { return min_quality_; }

  void set_drop_entire_snapshot(bool drop) noexcept { drop_entire_snapshot_ = drop; }
  [[nodiscard]] bool get_drop_entire_snapshot() const noexcept { return drop_entire_snapshot_; }

 private:
  double min_quality_;
  bool drop_entire_snapshot_ = false;  // If true, drop entire snapshot if any body fails quality
};

/**
 * @brief Filter that removes data points with excessive latency
 */
class SOLAR_CORE_API LatencyFilter : public StreamFilter {
 public:
  explicit LatencyFilter(std::chrono::milliseconds max_latency = std::chrono::milliseconds{2000});

  [[nodiscard]] std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) override;
  [[nodiscard]] std::string get_name() const override { return "LatencyFilter"; }

  // Configuration
  void set_max_latency(std::chrono::milliseconds latency) noexcept { max_latency_ = latency; }
  [[nodiscard]] std::chrono::milliseconds get_max_latency() const noexcept { return max_latency_; }

 private:
  std::chrono::milliseconds max_latency_;
};

/**
 * @brief Filter that applies rate limiting to reduce update frequency
 */
class SOLAR_CORE_API RateLimitFilter : public StreamFilter {
 public:
  explicit RateLimitFilter(std::chrono::milliseconds min_interval = std::chrono::milliseconds{100});

  [[nodiscard]] std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) override;
  [[nodiscard]] std::string get_name() const override { return "RateLimitFilter"; }

  // Configuration
  void set_min_interval(std::chrono::milliseconds interval) noexcept { min_interval_ = interval; }
  [[nodiscard]] std::chrono::milliseconds get_min_interval() const noexcept {
    return min_interval_;
  }

  void reset() noexcept { last_passed_time_ = std::chrono::system_clock::time_point{}; }

 private:
  std::chrono::milliseconds min_interval_;
  std::chrono::system_clock::time_point last_passed_time_;
};

/**
 * @brief Filter that removes duplicate or nearly identical data points
 */
class SOLAR_CORE_API DuplicationFilter : public StreamFilter {
 public:
  explicit DuplicationFilter(double position_tolerance = 1000.0,  // meters
                             double velocity_tolerance = 1.0);    // m/s

  [[nodiscard]] std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) override;
  [[nodiscard]] std::string get_name() const override { return "DuplicationFilter"; }

  // Configuration
  void set_position_tolerance(double tolerance) noexcept { position_tolerance_ = tolerance; }
  void set_velocity_tolerance(double tolerance) noexcept { velocity_tolerance_ = tolerance; }
  [[nodiscard]] double get_position_tolerance() const noexcept { return position_tolerance_; }
  [[nodiscard]] double get_velocity_tolerance() const noexcept { return velocity_tolerance_; }

  void clear_cache() { last_snapshot_.reset(); }

 private:
  double position_tolerance_;
  double velocity_tolerance_;
  std::optional<DataSnapshot> last_snapshot_;

  [[nodiscard]] bool is_significantly_different(const DataSnapshot& current,
                                                const DataSnapshot& previous) const;
  [[nodiscard]] bool is_body_different(const DataPoint& current, const DataPoint& previous) const;
};

/**
 * @brief Custom filter using user-provided predicate function
 */
class SOLAR_CORE_API PredicateFilter : public StreamFilter {
 public:
  using SnapshotPredicate = std::function<bool(const DataSnapshot&)>;
  using DataPointPredicate = std::function<bool(const DataPoint&)>;

  explicit PredicateFilter(SnapshotPredicate predicate, std::string name = "PredicateFilter");
  explicit PredicateFilter(DataPointPredicate predicate, std::string name = "PredicateFilter");

  [[nodiscard]] std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) override;
  [[nodiscard]] std::string get_name() const override { return name_; }

 private:
  SnapshotPredicate snapshot_predicate_;
  DataPointPredicate datapoint_predicate_;
  std::string name_;
};

/**
 * @brief Composite filter that applies multiple filters in sequence
 */
class SOLAR_CORE_API FilterChain : public StreamFilter {
 public:
  FilterChain() = default;
  explicit FilterChain(std::vector<std::unique_ptr<StreamFilter>> filters);

  [[nodiscard]] std::optional<DataSnapshot> apply(const DataSnapshot& snapshot) override;
  [[nodiscard]] std::string get_name() const override;

  // Filter management
  void add_filter(std::unique_ptr<StreamFilter> filter);
  void remove_filter(const std::string& filter_name);
  void clear_filters();

  [[nodiscard]] size_t get_filter_count() const noexcept { return filters_.size(); }
  [[nodiscard]] const std::vector<std::unique_ptr<StreamFilter>>& get_filters() const noexcept;

  // Enable/disable specific filters
  void enable_filter(const std::string& filter_name);
  void disable_filter(const std::string& filter_name);

 private:
  std::vector<std::unique_ptr<StreamFilter>> filters_;

  [[nodiscard]] StreamFilter* find_filter(const std::string& name);
};

/**
 * @brief Factory for creating common filter configurations
 */
class SOLAR_CORE_API FilterFactory {
 public:
  // Predefined filter chains
  [[nodiscard]] static std::unique_ptr<FilterChain> create_basic_filter_chain();
  [[nodiscard]] static std::unique_ptr<FilterChain> create_high_quality_filter_chain();
  [[nodiscard]] static std::unique_ptr<FilterChain> create_performance_filter_chain();
  [[nodiscard]] static std::unique_ptr<FilterChain> create_realtime_filter_chain();

  // Individual filter creation
  [[nodiscard]] static std::unique_ptr<BodySelectionFilter> create_body_selection_filter(
      const std::vector<std::string>& bodies);
  [[nodiscard]] static std::unique_ptr<QualityFilter> create_quality_filter(
      double min_quality = 0.7);
  [[nodiscard]] static std::unique_ptr<LatencyFilter> create_latency_filter(
      std::chrono::milliseconds max_latency = std::chrono::milliseconds{2000});
  [[nodiscard]] static std::unique_ptr<RateLimitFilter> create_rate_limit_filter(
      std::chrono::milliseconds min_interval = std::chrono::milliseconds{100});
  [[nodiscard]] static std::unique_ptr<DuplicationFilter> create_duplication_filter(
      double position_tolerance = 1000.0, double velocity_tolerance = 1.0);
};

}  // namespace SolarSystem::Streaming
