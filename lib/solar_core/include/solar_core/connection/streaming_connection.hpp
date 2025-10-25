/**
 * @file streaming_connection.hpp
 * @brief Streaming data connection implementation
 */

#pragma once

#include "solar_core/connection/connection_manager.hpp"
#include "solar_core/streaming/realtime_stream.hpp"

#include <memory>

namespace SolarSystem::Connection {

/**
 * @brief Connection for streaming data sources
 */
class StreamingConnection : public IConnection {
 public:
  /**
   * @brief Construct streaming connection
   */
  explicit StreamingConnection(
      std::string id,
      std::shared_ptr<SolarSystem::Streaming::RealtimeStream> stream);

  /**
   * @brief Destructor
   */
  ~StreamingConnection() override;

  // IConnection interface
  [[nodiscard]] bool connect() override;
  void disconnect() override;
  [[nodiscard]] bool is_connected() const override;
  [[nodiscard]] bool health_check() override;
  [[nodiscard]] ConnectionHealth get_health() const override;
  [[nodiscard]] std::string get_id() const override;

 private:
  std::string id_;
  std::shared_ptr<SolarSystem::Streaming::RealtimeStream> stream_;
  mutable ConnectionHealth health_;
  mutable std::mutex health_mutex_;
};

}  // namespace SolarSystem::Connection
