/**
 * @file streaming_connection.cpp
 * @brief Implementation of streaming data connection
 */

#include "solar_core/connection/streaming_connection.hpp"

#include "solar_utils/logging.hpp"

namespace SolarSystem::Connection {

using namespace SolarSystem::Utils;

StreamingConnection::StreamingConnection(
    std::string id,
    std::shared_ptr<SolarSystem::Streaming::RealtimeStream> stream)
    : id_(std::move(id)), stream_(std::move(stream)) {
  health_.state = ConnectionState::DISCONNECTED;
}

StreamingConnection::~StreamingConnection() {
  if (is_connected()) {
    disconnect();
  }
}

bool StreamingConnection::connect() {
  std::lock_guard<std::mutex> lock(health_mutex_);

  LOG_INFO("StreamingConnection", "Connecting: " + id_);

  health_.state = ConnectionState::CONNECTING;
  health_.last_connection_attempt = std::chrono::system_clock::now();

  // Start the stream
  auto result = stream_->start();
  if (!result) {
    LOG_ERROR("StreamingConnection", "Failed to connect: " + result.error());
    health_.state = ConnectionState::DISCONNECTED;
    health_.last_error = result.error();
    health_.failed_operations++;
    return false;
  }

  health_.state = ConnectionState::CONNECTED;
  health_.last_successful_connection = std::chrono::system_clock::now();
  health_.successful_operations++;
  health_.health_score = 1.0;

  LOG_INFO("StreamingConnection", "Connected successfully: " + id_);
  return true;
}

void StreamingConnection::disconnect() {
  std::lock_guard<std::mutex> lock(health_mutex_);

  LOG_INFO("StreamingConnection", "Disconnecting: " + id_);

  if (stream_->is_running()) {
    auto result = stream_->stop();
    if (!result) {
      LOG_WARN("StreamingConnection", "Error during disconnect: " + result.error());
    }
  }

  health_.state = ConnectionState::DISCONNECTED;
}

bool StreamingConnection::is_connected() const {
  std::lock_guard<std::mutex> lock(health_mutex_);
  return health_.state == ConnectionState::CONNECTED &&
         stream_->is_running();
}

bool StreamingConnection::health_check() {
  std::lock_guard<std::mutex> lock(health_mutex_);

  if (!stream_->is_running()) {
    health_.health_score = 0.0;
    health_.state = ConnectionState::DISCONNECTED;
    return false;
  }

  // Get stream statistics
  const auto& stats = stream_->get_stats();

  // Calculate health score based on stream performance
  double quality_score = stats.avg_quality_score;
  double delivery_rate = stats.total_snapshots_delivered.load() > 0
                             ? static_cast<double>(stats.total_snapshots_delivered.load()) /
                                   (stats.total_snapshots_generated.load() + 1)
                             : 0.0;

  health_.health_score = (quality_score + delivery_rate) / 2.0;

  // Update latency - use actual average delivery time from stream statistics
  if (stats.total_snapshots_delivered.load() > 0) {
    // Use the actual average delivery time tracked by the stream
    // This represents the real latency of data delivery
    health_.average_latency = stats.avg_delivery_time;

    // Also track the maximum latency observed
    if (stats.max_latency > health_.average_latency) {
      // Store max latency in metadata for monitoring
      health_.last_error = "Max latency: " + std::to_string(stats.max_latency.count()) + "ms";
    }
  }

  // Update operation counts
  health_.successful_operations = stats.total_snapshots_delivered.load();
  health_.failed_operations = stats.total_snapshots_dropped.load();

  return health_.health_score > 0.5;
}

ConnectionHealth StreamingConnection::get_health() const {
  std::lock_guard<std::mutex> lock(health_mutex_);
  return health_;
}

std::string StreamingConnection::get_id() const {
  return id_;
}

}  // namespace SolarSystem::Connection
