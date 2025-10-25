#include "solar_core/streaming/data_stream.hpp"

#include <algorithm>
#include <sstream>

#include "solar_utils/logging.hpp"

namespace SolarSystem::Streaming {

using namespace SolarSystem::Utils;

// DataPoint implementation
DataPoint::DataPoint(const Bodies::CelestialBody& body, std::chrono::system_clock::time_point time)
    : body_name(body.name()),
      timestamp(time),
      position(body.position()),
      velocity(body.velocity()),
      acceleration(body.acceleration()),
      mass(body.mass()),
      type(body.type()),
      data_source("simulation") {
  // Calculate quality score based on data completeness and consistency
  quality_score = 1.0;  // Start with perfect score

  // Check for invalid values
  if (std::isnan(position.x()) || std::isnan(position.y()) || std::isnan(position.z())) {
    quality_score *= 0.5;
    data_source += "_invalid_position";
  }

  if (std::isnan(velocity.x()) || std::isnan(velocity.y()) || std::isnan(velocity.z())) {
    quality_score *= 0.5;
    data_source += "_invalid_velocity";
  }

  if (mass <= 0.0) {
    quality_score *= 0.8;
    data_source += "_invalid_mass";
  }

  // Calculate latency (time since data was generated)
  auto now = std::chrono::system_clock::now();
  if (now >= timestamp) {
    latency = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);
  }
}

// DataSnapshot implementation
DataSnapshot::DataSnapshot(const Bodies::BodyCollection& bodies, std::chrono::system_clock::time_point time)
    : timestamp(time) {
  auto start_time = std::chrono::steady_clock::now();

  data_points.reserve(bodies.size());

  for (const auto& body : bodies) {
    data_points.emplace_back(body, time);
  }

  auto end_time = std::chrono::steady_clock::now();
  processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Calculate overall quality
  if (!data_points.empty()) {
    double total_quality = 0.0;
    for (const auto& point : data_points) {
      total_quality += point.quality_score;
    }
    overall_quality = total_quality / data_points.size();
  }
}

// StreamStats implementation
void StreamStats::reset() {
  total_snapshots_generated.store(0);
  total_snapshots_delivered.store(0);
  total_snapshots_dropped.store(0);
  quality_failures.store(0);
  latency_violations.store(0);

  avg_generation_time = std::chrono::milliseconds{0};
  avg_delivery_time = std::chrono::milliseconds{0};
  max_latency = std::chrono::milliseconds{0};

  avg_quality_score = 1.0;
  min_quality_score = 1.0;

  last_update = std::chrono::system_clock::now();
}

std::string StreamStats::to_string() const {
  std::ostringstream oss;
  oss << "StreamStats {\n";
  oss << "  Generated: " << total_snapshots_generated.load() << "\n";
  oss << "  Delivered: " << total_snapshots_delivered.load() << "\n";
  oss << "  Dropped: " << total_snapshots_dropped.load() << "\n";
  oss << "  Quality Failures: " << quality_failures.load() << "\n";
  oss << "  Latency Violations: " << latency_violations.load() << "\n";
  oss << "  Avg Generation Time: " << avg_generation_time.count() << "ms\n";
  oss << "  Avg Delivery Time: " << avg_delivery_time.count() << "ms\n";
  oss << "  Max Latency: " << max_latency.count() << "ms\n";
  oss << "  Avg Quality: " << avg_quality_score << "\n";
  oss << "  Min Quality: " << min_quality_score << "\n";
  oss << "}";
  return oss.str();
}

// DataStream implementation
DataStream::DataStream(StreamConfig config) : config_(std::move(config)) {
  stats_.reset();
}

DataStream::~DataStream() {
  if (running_.load()) {
    auto result = stop();
    (void)result;  // Suppress unused result warning
  }
}

Utils::Expected<void, std::string> DataStream::start() {
  std::lock_guard<std::mutex> lock(stream_mutex_);

  if (running_.load()) {
    return Utils::Expected<void, std::string>("Stream is already running");
  }

  // Initialize the stream
  auto init_result = initialize_stream();
  if (!init_result) {
    return init_result;
  }

  // Reset state
  stop_requested_.store(false);
  paused_.store(false);
  stats_.reset();

  // Start worker thread if configured
  if (config_.use_background_thread) {
    worker_thread_ = std::make_unique<std::thread>(&DataStream::worker_loop, this);
  }

  running_.store(true);

  LOG_INFO("DataStream", "Stream started successfully");
  return Utils::Expected<void, std::string>();
}

Utils::Expected<void, std::string> DataStream::stop() {
  std::lock_guard<std::mutex> lock(stream_mutex_);

  if (!running_.load()) {
    return Utils::Expected<void, std::string>();
  }

  // Signal stop
  stop_requested_.store(true);
  running_.store(false);

  // Wake up worker thread
  stream_cv_.notify_all();
  buffer_cv_.notify_all();

  // Wait for worker thread to finish
  if (worker_thread_ && worker_thread_->joinable()) {
    worker_thread_->join();
    worker_thread_.reset();
  }

  // Cleanup
  auto cleanup_result = cleanup_stream();
  if (!cleanup_result) {
    LOG_WARN("DataStream", "Cleanup failed: " + cleanup_result.error());
  }

  LOG_INFO("DataStream", "Stream stopped successfully");
  return Utils::Expected<void, std::string>();
}

Utils::Expected<void, std::string> DataStream::pause() {
  if (!running_.load()) {
    return Utils::Expected<void, std::string>("Stream is not running");
  }

  paused_.store(true);
  LOG_INFO("DataStream", "Stream paused");
  return Utils::Expected<void, std::string>();
}

Utils::Expected<void, std::string> DataStream::resume() {
  if (!running_.load()) {
    return Utils::Expected<void, std::string>("Stream is not running");
  }

  paused_.store(false);
  stream_cv_.notify_all();
  LOG_INFO("DataStream", "Stream resumed");
  return Utils::Expected<void, std::string>();
}

void DataStream::set_config(const StreamConfig& config) {
  std::lock_guard<std::mutex> lock(stream_mutex_);
  config_ = config;
}

Utils::Expected<void, std::string> DataStream::push_snapshot(const DataSnapshot& snapshot) {
  if (!running_.load()) {
    return Utils::Expected<void, std::string>("Stream is not running");
  }

  // Check quality
  if (should_drop_snapshot(snapshot)) {
    stats_.total_snapshots_dropped.fetch_add(1);
    return Utils::Expected<void, std::string>("Snapshot dropped due to quality issues");
  }

  // Add to buffer
  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    // Check buffer size
    if (buffer_.size() >= config_.max_buffer_size) {
      // Drop oldest snapshot
      buffer_.pop();
      stats_.total_snapshots_dropped.fetch_add(1);
    }

    buffer_.push(snapshot);
  }

  buffer_cv_.notify_one();
  update_stats(snapshot);

  return Utils::Expected<void, std::string>();
}

void DataStream::notify_data_callback(const DataSnapshot& snapshot) {
  if (data_callback_) {
    try {
      data_callback_(snapshot);
      stats_.total_snapshots_delivered.fetch_add(1);
    } catch (const std::exception& e) {
      notify_error_callback("Data callback failed: " + std::string(e.what()));
    }
  }
}

void DataStream::notify_error_callback(const std::string& error) {
  if (error_callback_) {
    try {
      error_callback_(error);
    } catch (const std::exception& e) {
      LOG_ERROR("DataStream", "Error callback failed: " + std::string(e.what()));
    }
  }
  LOG_ERROR("DataStream", error);
}

void DataStream::notify_quality_callback() {
  if (quality_callback_) {
    try {
      quality_callback_(stats_);
    } catch (const std::exception& e) {
      LOG_ERROR("DataStream", "Quality callback failed: " + std::string(e.what()));
    }
  }
}

bool DataStream::should_drop_snapshot(const DataSnapshot& snapshot) const {
  // Check quality threshold
  if (config_.enable_quality_monitoring &&
      snapshot.overall_quality < config_.min_quality_threshold) {
    return true;
  }

  // Check latency
  if (snapshot.processing_time > config_.max_latency) {
    return true;
  }

  return false;
}

void DataStream::update_stats(const DataSnapshot& snapshot) {
  std::lock_guard<std::mutex> lock(stats_mutex_);

  stats_.total_snapshots_generated.fetch_add(1);

  // Update quality stats
  if (snapshot.overall_quality < config_.min_quality_threshold) {
    stats_.quality_failures.fetch_add(1);
  }

  if (snapshot.processing_time > config_.max_latency) {
    stats_.latency_violations.fetch_add(1);
  }

  // Update timing stats
  if (snapshot.processing_time > stats_.max_latency) {
    stats_.max_latency = snapshot.processing_time;
  }

  // Update quality scores
  if (snapshot.overall_quality < stats_.min_quality_score) {
    stats_.min_quality_score = snapshot.overall_quality;
  }

  // Update average quality (simple moving average approximation)
  stats_.avg_quality_score = (stats_.avg_quality_score * 0.9) + (snapshot.overall_quality * 0.1);

  stats_.last_update = std::chrono::system_clock::now();
}

void DataStream::worker_loop() {
  LOG_INFO("DataStream", "Worker thread started");

  while (!stop_requested_.load()) {
    try {
      // Wait for work or timeout
      std::unique_lock<std::mutex> lock(stream_mutex_);
      stream_cv_.wait_for(lock, config_.update_interval, [this] {
        return stop_requested_.load() || !paused_.load();
      });

      if (stop_requested_.load()) {
        break;
      }

      if (paused_.load()) {
        continue;
      }

      lock.unlock();

      // Generate new snapshot
      auto snapshot_result = generate_snapshot();
      if (snapshot_result) {
        auto push_result = push_snapshot(snapshot_result.value());
        (void)push_result;  // Suppress unused result warning
      } else {
        notify_error_callback("Failed to generate snapshot: " + snapshot_result.error());
      }

      // Process buffer
      process_buffer();

      // Notify quality callback periodically
      static auto last_quality_notification = std::chrono::steady_clock::now();
      auto now = std::chrono::steady_clock::now();
      if (now - last_quality_notification > std::chrono::seconds{5}) {
        notify_quality_callback();
        last_quality_notification = now;
      }

    } catch (const std::exception& e) {
      notify_error_callback("Worker thread error: " + std::string(e.what()));
    }
  }

  LOG_INFO("DataStream", "Worker thread stopped");
}

void DataStream::process_buffer() {
  std::lock_guard<std::mutex> lock(buffer_mutex_);

  while (!buffer_.empty()) {
    auto snapshot = buffer_.front();
    buffer_.pop();

    // Check if snapshot is too old
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - snapshot.timestamp);

    if (age > config_.buffer_timeout) {
      stats_.total_snapshots_dropped.fetch_add(1);
      continue;
    }

    // Deliver snapshot
    notify_data_callback(snapshot);
  }
}

void DataStream::monitor_quality(const DataSnapshot& snapshot) {
  if (!config_.enable_quality_monitoring) {
    return;
  }

  // Check overall quality
  if (snapshot.overall_quality < config_.min_quality_threshold) {
    notify_error_callback("Snapshot quality below threshold: " +
                         std::to_string(snapshot.overall_quality));
  }

  // Check individual data points
  for (const auto& point : snapshot.data_points) {
    if (point.quality_score < config_.min_quality_threshold) {
      notify_error_callback("Data point quality below threshold for " + point.body_name +
                           ": " + std::to_string(point.quality_score));
    }

    if (point.latency > config_.max_latency) {
      notify_error_callback("Data point latency too high for " + point.body_name +
                           ": " + std::to_string(point.latency.count()) + "ms");
    }
  }
}

bool DataStream::validate_snapshot_quality(const DataSnapshot& snapshot) const {
  if (!config_.enable_quality_monitoring) {
    return true;
  }

  // Check overall quality
  if (snapshot.overall_quality < config_.min_quality_threshold) {
    return false;
  }

  // Check processing time
  if (snapshot.processing_time > config_.max_latency) {
    return false;
  }

  // Check for missing bodies
  if (snapshot.missing_bodies > 0) {
    return false;
  }

  return true;
}

}  // namespace SolarSystem::Streaming
