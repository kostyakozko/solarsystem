#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/export.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_utils/expected.hpp"

namespace SolarSystem::Streaming {

/**
 * @brief Data point representing a single celestial body state at a specific time
 */
struct DataPoint {
  std::string body_name;
  std::chrono::system_clock::time_point timestamp;
  Math::Vector3d position;
  Math::Vector3d velocity;
  Math::Vector3d acceleration;
  long double mass;
  Bodies::BodyType type;

  // Quality metrics
  double quality_score = 1.0;  // 0.0 to 1.0, where 1.0 is perfect quality
  std::string data_source;
  std::chrono::milliseconds latency{0};

  DataPoint() = default;
  explicit DataPoint(const Bodies::CelestialBody& body,
                     std::chrono::system_clock::time_point time = std::chrono::system_clock::now());
};

/**
 * @brief Collection of data points for multiple bodies at a specific time
 */
struct DataSnapshot {
  std::chrono::system_clock::time_point timestamp;
  std::vector<DataPoint> data_points;
  size_t sequence_number = 0;
  std::chrono::milliseconds processing_time{0};

  // Quality metrics for the entire snapshot
  double overall_quality = 1.0;
  size_t missing_bodies = 0;
  std::vector<std::string> warnings;

  DataSnapshot() = default;
  explicit DataSnapshot(
      const Bodies::BodyCollection& bodies,
      std::chrono::system_clock::time_point time = std::chrono::system_clock::now());
};

/**
 * @brief Configuration for data streaming
 */
struct StreamConfig {
  std::chrono::milliseconds update_interval{1000};  // How often to generate new data
  std::chrono::milliseconds buffer_timeout{5000};   // Max time to buffer data
  size_t max_buffer_size = 1000;                    // Max buffered snapshots
  size_t max_queue_size = 100;                      // Max queued snapshots for consumers

  // Quality thresholds
  double min_quality_threshold = 0.7;           // Minimum acceptable quality
  std::chrono::milliseconds max_latency{2000};  // Maximum acceptable latency

  // Filtering options
  bool enable_filtering = true;
  bool enable_aggregation = true;
  bool enable_quality_monitoring = true;

  // Performance options
  bool use_background_thread = true;
  size_t worker_thread_count = 1;
  bool enable_compression = false;
};

/**
 * @brief Statistics about streaming performance
 */
struct StreamStats {
  std::atomic<size_t> total_snapshots_generated{0};
  std::atomic<size_t> total_snapshots_delivered{0};
  std::atomic<size_t> total_snapshots_dropped{0};
  std::atomic<size_t> quality_failures{0};
  std::atomic<size_t> latency_violations{0};

  std::chrono::milliseconds avg_generation_time{0};
  std::chrono::milliseconds avg_delivery_time{0};
  std::chrono::milliseconds max_latency{0};

  double avg_quality_score = 1.0;
  double min_quality_score = 1.0;

  std::chrono::system_clock::time_point last_update;

  void reset();
  [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Callback function types for streaming events
 */
using DataCallback = std::function<void(const DataSnapshot&)>;
using ErrorCallback = std::function<void(const std::string&)>;
using QualityCallback = std::function<void(const StreamStats&)>;

/**
 * @brief Main data streaming interface
 */
class SOLAR_CORE_API DataStream {
 public:
  explicit DataStream(StreamConfig config = {});
  virtual ~DataStream();

  // Non-copyable, non-movable (due to atomic members)
  DataStream(const DataStream&) = delete;
  DataStream& operator=(const DataStream&) = delete;
  DataStream(DataStream&&) = delete;
  DataStream& operator=(DataStream&&) = delete;

  // Stream control
  [[nodiscard]] Utils::Expected<void, std::string> start();
  [[nodiscard]] Utils::Expected<void, std::string> stop();
  [[nodiscard]] Utils::Expected<void, std::string> pause();
  [[nodiscard]] Utils::Expected<void, std::string> resume();

  [[nodiscard]] bool is_running() const noexcept { return running_.load(); }
  [[nodiscard]] bool is_paused() const noexcept { return paused_.load(); }

  // Configuration
  void set_config(const StreamConfig& config);
  [[nodiscard]] const StreamConfig& get_config() const noexcept { return config_; }

  // Callbacks
  void set_data_callback(DataCallback callback) { data_callback_ = std::move(callback); }
  void set_error_callback(ErrorCallback callback) { error_callback_ = std::move(callback); }
  void set_quality_callback(QualityCallback callback) { quality_callback_ = std::move(callback); }

  // Statistics
  [[nodiscard]] const StreamStats& get_stats() const noexcept { return stats_; }
  void reset_stats() { stats_.reset(); }

  // Data source management
  [[nodiscard]] virtual Utils::Expected<void, std::string> set_data_source(
      std::shared_ptr<Bodies::BodyCollection> bodies) = 0;

  // Manual data push (for testing or external data sources)
  [[nodiscard]] Utils::Expected<void, std::string> push_snapshot(const DataSnapshot& snapshot);

 protected:
  // Virtual methods for derived classes to implement
  [[nodiscard]] virtual Utils::Expected<DataSnapshot, std::string> generate_snapshot() = 0;
  [[nodiscard]] virtual Utils::Expected<void, std::string> initialize_stream() = 0;
  [[nodiscard]] virtual Utils::Expected<void, std::string> cleanup_stream() = 0;

  // Utility methods for derived classes
  void notify_data_callback(const DataSnapshot& snapshot);
  void notify_error_callback(const std::string& error);
  void notify_quality_callback();

  [[nodiscard]] bool should_drop_snapshot(const DataSnapshot& snapshot) const;
  void update_stats(const DataSnapshot& snapshot);

 private:
  StreamConfig config_;
  std::atomic<bool> running_{false};
  std::atomic<bool> paused_{false};
  std::atomic<bool> stop_requested_{false};

  // Threading
  std::unique_ptr<std::thread> worker_thread_;
  std::mutex stream_mutex_;
  std::condition_variable stream_cv_;

  // Buffering
  std::queue<DataSnapshot> buffer_;
  std::mutex buffer_mutex_;
  std::condition_variable buffer_cv_;

  // Callbacks
  DataCallback data_callback_;
  ErrorCallback error_callback_;
  QualityCallback quality_callback_;

  // Statistics
  mutable StreamStats stats_;
  std::mutex stats_mutex_;

  // Worker thread function
  void worker_loop();
  void process_buffer();

  // Quality monitoring
  void monitor_quality(const DataSnapshot& snapshot);
  [[nodiscard]] bool validate_snapshot_quality(const DataSnapshot& snapshot) const;
};

}  // namespace SolarSystem::Streaming
