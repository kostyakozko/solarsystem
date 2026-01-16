#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "solar_core/export.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_core/streaming/data_stream.hpp"

namespace SolarSystem::Streaming {

/**
 * @brief Aggregated statistics for a celestial body over time
 */
struct BodyAggregateData {
  std::string body_name;
  size_t sample_count = 0;

  // Position statistics
  Math::Vector3d avg_position{};
  Math::Vector3d min_position{};
  Math::Vector3d max_position{};
  Math::Vector3d position_variance{};

  // Velocity statistics
  Math::Vector3d avg_velocity{};
  Math::Vector3d min_velocity{};
  Math::Vector3d max_velocity{};
  Math::Vector3d velocity_variance{};

  // Derived statistics
  double avg_speed = 0.0;
  double max_speed = 0.0;
  double min_speed = 0.0;
  double avg_distance_from_origin = 0.0;

  // Quality statistics
  double avg_quality = 1.0;
  double min_quality = 1.0;
  std::chrono::milliseconds avg_latency{0};
  std::chrono::milliseconds max_latency{0};

  // Time range
  std::chrono::system_clock::time_point first_sample_time;
  std::chrono::system_clock::time_point last_sample_time;
  std::chrono::milliseconds time_span{0};
};

/**
 * @brief Aggregated data for an entire snapshot collection
 */
struct AggregateSnapshot {
  std::chrono::system_clock::time_point timestamp;
  std::chrono::system_clock::time_point window_start;
  std::chrono::system_clock::time_point window_end;

  std::vector<BodyAggregateData> body_aggregates;

  // Overall statistics
  size_t total_samples = 0;
  size_t total_bodies = 0;
  double overall_avg_quality = 1.0;
  std::chrono::milliseconds overall_avg_latency{0};

  // System-wide derived statistics
  Math::Vector3d system_center_of_mass{};
  Math::Vector3d system_total_momentum{};
  double system_total_kinetic_energy = 0.0;

  [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Base class for stream aggregators
 */
class SOLAR_CORE_API StreamAggregator {
 public:
  virtual ~StreamAggregator() = default;

  /**
   * @brief Add a data snapshot to the aggregator
   */
  virtual void add_snapshot(const DataSnapshot& snapshot) = 0;

  /**
   * @brief Get current aggregated data
   */
  [[nodiscard]] virtual AggregateSnapshot get_aggregate() const = 0;

  /**
   * @brief Reset aggregator state
   */
  virtual void reset() = 0;

  /**
   * @brief Get aggregator name for debugging
   */
  [[nodiscard]] virtual std::string get_name() const = 0;

  /**
   * @brief Check if aggregator has enough data for meaningful results
   */
  [[nodiscard]] virtual bool has_sufficient_data() const = 0;
};

/**
 * @brief Time-window based aggregator that maintains statistics over a sliding window
 */
class SOLAR_CORE_API TimeWindowAggregator : public StreamAggregator {
 public:
  explicit TimeWindowAggregator(std::chrono::milliseconds window_duration = std::chrono::minutes{
                                    5});

  void add_snapshot(const DataSnapshot& snapshot) override;
  [[nodiscard]] AggregateSnapshot get_aggregate() const override;
  void reset() override;
  [[nodiscard]] std::string get_name() const override { return "TimeWindowAggregator"; }
  [[nodiscard]] bool has_sufficient_data() const override;

  // Configuration
  void set_window_duration(std::chrono::milliseconds duration);
  [[nodiscard]] std::chrono::milliseconds get_window_duration() const noexcept {
    return window_duration_;
  }

  void set_min_samples(size_t min_samples) noexcept { min_samples_ = min_samples; }
  [[nodiscard]] size_t get_min_samples() const noexcept { return min_samples_; }

 private:
  std::chrono::milliseconds window_duration_;
  size_t min_samples_ = 3;

  std::deque<DataSnapshot> snapshots_;
  mutable std::mutex snapshots_mutex_;

  void cleanup_old_snapshots();
  [[nodiscard]] BodyAggregateData calculate_body_aggregate(const std::string& body_name) const;
  [[nodiscard]] std::vector<const DataPoint*> get_body_data_points(
      const std::string& body_name) const;
};

/**
 * @brief Sample-count based aggregator that maintains statistics over a fixed number of samples
 */
class SOLAR_CORE_API SampleCountAggregator : public StreamAggregator {
 public:
  explicit SampleCountAggregator(size_t max_samples = 100);

  void add_snapshot(const DataSnapshot& snapshot) override;
  [[nodiscard]] AggregateSnapshot get_aggregate() const override;
  void reset() override;
  [[nodiscard]] std::string get_name() const override { return "SampleCountAggregator"; }
  [[nodiscard]] bool has_sufficient_data() const override;

  // Configuration
  void set_max_samples(size_t max_samples);
  [[nodiscard]] size_t get_max_samples() const noexcept { return max_samples_; }

  void set_min_samples(size_t min_samples) noexcept { min_samples_ = min_samples; }
  [[nodiscard]] size_t get_min_samples() const noexcept { return min_samples_; }

 private:
  size_t max_samples_;
  size_t min_samples_ = 3;

  std::deque<DataSnapshot> snapshots_;
  mutable std::mutex snapshots_mutex_;

  void maintain_sample_limit();
  [[nodiscard]] BodyAggregateData calculate_body_aggregate(const std::string& body_name) const;
  [[nodiscard]] std::vector<const DataPoint*> get_body_data_points(
      const std::string& body_name) const;
};

/**
 * @brief Real-time aggregator that provides continuous statistics updates
 */
class SOLAR_CORE_API RealtimeAggregator : public StreamAggregator {
 public:
  explicit RealtimeAggregator(std::chrono::milliseconds update_interval = std::chrono::seconds{1});

  void add_snapshot(const DataSnapshot& snapshot) override;
  [[nodiscard]] AggregateSnapshot get_aggregate() const override;
  void reset() override;
  [[nodiscard]] std::string get_name() const override { return "RealtimeAggregator"; }
  [[nodiscard]] bool has_sufficient_data() const override;

  // Configuration
  void set_update_interval(std::chrono::milliseconds interval) { update_interval_ = interval; }
  [[nodiscard]] std::chrono::milliseconds get_update_interval() const noexcept {
    return update_interval_;
  }

  // Real-time specific methods
  [[nodiscard]] bool is_aggregate_stale() const;
  void force_update();

 private:
  std::chrono::milliseconds update_interval_;

  // Running statistics per body
  struct RunningStats {
    size_t count = 0;
    Math::Vector3d position_sum{};
    Math::Vector3d position_sum_sq{};
    Math::Vector3d velocity_sum{};
    Math::Vector3d velocity_sum_sq{};
    double quality_sum = 0.0;
    std::chrono::milliseconds latency_sum{0};

    Math::Vector3d min_position{std::numeric_limits<long double>::max(),
                                std::numeric_limits<long double>::max(),
                                std::numeric_limits<long double>::max()};
    Math::Vector3d max_position{std::numeric_limits<long double>::lowest(),
                                std::numeric_limits<long double>::lowest(),
                                std::numeric_limits<long double>::lowest()};
    Math::Vector3d min_velocity{std::numeric_limits<long double>::max(),
                                std::numeric_limits<long double>::max(),
                                std::numeric_limits<long double>::max()};
    Math::Vector3d max_velocity{std::numeric_limits<long double>::lowest(),
                                std::numeric_limits<long double>::lowest(),
                                std::numeric_limits<long double>::lowest()};
    double min_quality = std::numeric_limits<double>::max();
    std::chrono::milliseconds max_latency{0};

    std::chrono::system_clock::time_point first_time;
    std::chrono::system_clock::time_point last_time;

    void add_data_point(const DataPoint& point);
    [[nodiscard]] BodyAggregateData to_aggregate_data(const std::string& body_name) const;
    void reset();
  };

  std::unordered_map<std::string, RunningStats> body_stats_;
  mutable std::mutex stats_mutex_;

  mutable std::chrono::system_clock::time_point last_update_time_;
  mutable AggregateSnapshot cached_aggregate_;
  mutable bool aggregate_dirty_ = true;

  void update_cached_aggregate() const;
};

/**
 * @brief Callback-based aggregator that triggers user-defined functions on aggregation events
 */
class SOLAR_CORE_API CallbackAggregator : public StreamAggregator {
 public:
  using AggregateCallback = std::function<void(const AggregateSnapshot&)>;
  using ThresholdCallback = std::function<void(const std::string&, const BodyAggregateData&)>;

  explicit CallbackAggregator(std::unique_ptr<StreamAggregator> base_aggregator);

  void add_snapshot(const DataSnapshot& snapshot) override;
  [[nodiscard]] AggregateSnapshot get_aggregate() const override;
  void reset() override;
  [[nodiscard]] std::string get_name() const override;
  [[nodiscard]] bool has_sufficient_data() const override;

  // Callback management
  void set_aggregate_callback(AggregateCallback callback) {
    aggregate_callback_ = std::move(callback);
  }
  void set_threshold_callback(ThresholdCallback callback) {
    threshold_callback_ = std::move(callback);
  }

  // Threshold configuration
  void set_quality_threshold(double threshold) { quality_threshold_ = threshold; }
  void set_latency_threshold(std::chrono::milliseconds threshold) {
    latency_threshold_ = threshold;
  }
  void set_callback_interval(std::chrono::milliseconds interval) { callback_interval_ = interval; }

 private:
  std::unique_ptr<StreamAggregator> base_aggregator_;

  AggregateCallback aggregate_callback_;
  ThresholdCallback threshold_callback_;

  double quality_threshold_ = 0.5;
  std::chrono::milliseconds latency_threshold_{5000};
  std::chrono::milliseconds callback_interval_{1000};

  std::chrono::system_clock::time_point last_callback_time_;

  void check_thresholds(const AggregateSnapshot& aggregate);
  bool should_trigger_callback() const;
};

/**
 * @brief Factory for creating common aggregator configurations
 */
class SOLAR_CORE_API AggregatorFactory {
 public:
  // Predefined aggregators
  [[nodiscard]] static std::unique_ptr<TimeWindowAggregator> create_time_window_aggregator(
      std::chrono::milliseconds window_duration = std::chrono::minutes{5});

  [[nodiscard]] static std::unique_ptr<SampleCountAggregator> create_sample_count_aggregator(
      size_t max_samples = 100);

  [[nodiscard]] static std::unique_ptr<RealtimeAggregator> create_realtime_aggregator(
      std::chrono::milliseconds update_interval = std::chrono::seconds{1});

  [[nodiscard]] static std::unique_ptr<CallbackAggregator> create_callback_aggregator(
      std::unique_ptr<StreamAggregator> base_aggregator);

  // Specialized configurations
  [[nodiscard]] static std::unique_ptr<StreamAggregator> create_performance_monitoring_aggregator();
  [[nodiscard]] static std::unique_ptr<StreamAggregator> create_quality_monitoring_aggregator();
  [[nodiscard]] static std::unique_ptr<StreamAggregator> create_scientific_analysis_aggregator();
};

}  // namespace SolarSystem::Streaming
