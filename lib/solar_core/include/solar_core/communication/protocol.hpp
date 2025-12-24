/**
 * @file protocol.hpp
 * @brief Communication protocol interface and implementations
 *
 * Provides standardized communication protocols for inter-application
 * communication with retry logic, error handling, and monitoring.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/communication/message.hpp"
#include "solar_core/export.hpp"
#include "solar_utils/expected.hpp"

namespace SolarSystem::Communication {

/**
 * @brief Protocol error types
 */
enum class ProtocolError {
  CONNECTION_FAILED,
  SEND_FAILED,
  RECEIVE_FAILED,
  TIMEOUT,
  INVALID_MESSAGE,
  AUTHENTICATION_FAILED,
  ENCRYPTION_FAILED,
  PROTOCOL_VIOLATION
};

/**
 * @brief Protocol statistics
 */
struct ProtocolStats {
  size_t messages_sent = 0;
  size_t messages_received = 0;
  size_t messages_failed = 0;
  size_t bytes_sent = 0;
  size_t bytes_received = 0;
  std::chrono::milliseconds average_latency{0};
  std::chrono::system_clock::time_point last_activity;
};

/**
 * @brief Communication protocol interface
 */
class IProtocol {
 public:
  virtual ~IProtocol() = default;

  /**
   * @brief Initialize protocol
   */
  [[nodiscard]] virtual SolarSystem::Utils::Expected<void, ProtocolError> initialize() = 0;

  /**
   * @brief Shutdown protocol
   */
  virtual void shutdown() = 0;

  /**
   * @brief Send message
   */
  [[nodiscard]] virtual SolarSystem::Utils::Expected<void, ProtocolError> send(
      const Message& message) = 0;

  /**
   * @brief Receive message (blocking with timeout)
   */
  [[nodiscard]] virtual SolarSystem::Utils::Expected<Message, ProtocolError> receive(
      std::chrono::milliseconds timeout = std::chrono::seconds(30)) = 0;

  /**
   * @brief Send request and wait for response
   */
  [[nodiscard]] virtual SolarSystem::Utils::Expected<Message, ProtocolError> request(
      const Message& request, std::chrono::milliseconds timeout = std::chrono::seconds(30)) = 0;

  /**
   * @brief Check if protocol is ready
   */
  [[nodiscard]] virtual bool is_ready() const = 0;

  /**
   * @brief Get protocol statistics
   */
  [[nodiscard]] virtual ProtocolStats get_stats() const = 0;

  /**
   * @brief Get protocol name
   */
  [[nodiscard]] virtual std::string get_name() const = 0;
};

/**
 * @brief In-process protocol (for same-process communication)
 */
class SOLAR_CORE_API InProcessProtocol : public IProtocol {
 public:
  InProcessProtocol();
  ~InProcessProtocol() override;

  [[nodiscard]] SolarSystem::Utils::Expected<void, ProtocolError> initialize() override;
  void shutdown() override;
  [[nodiscard]] SolarSystem::Utils::Expected<void, ProtocolError> send(
      const Message& message) override;
  [[nodiscard]] SolarSystem::Utils::Expected<Message, ProtocolError> receive(
      std::chrono::milliseconds timeout) override;
  [[nodiscard]] SolarSystem::Utils::Expected<Message, ProtocolError> request(
      const Message& request, std::chrono::milliseconds timeout) override;
  [[nodiscard]] bool is_ready() const override;
  [[nodiscard]] ProtocolStats get_stats() const override;
  [[nodiscard]] std::string get_name() const override { return "InProcess"; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief File-based protocol (for inter-process communication via files)
 */
class SOLAR_CORE_API FileProtocol : public IProtocol {
 public:
  explicit FileProtocol(const std::string& directory);
  ~FileProtocol() override;

  [[nodiscard]] SolarSystem::Utils::Expected<void, ProtocolError> initialize() override;
  void shutdown() override;
  [[nodiscard]] SolarSystem::Utils::Expected<void, ProtocolError> send(
      const Message& message) override;
  [[nodiscard]] SolarSystem::Utils::Expected<Message, ProtocolError> receive(
      std::chrono::milliseconds timeout) override;
  [[nodiscard]] SolarSystem::Utils::Expected<Message, ProtocolError> request(
      const Message& request, std::chrono::milliseconds timeout) override;
  [[nodiscard]] bool is_ready() const override;
  [[nodiscard]] ProtocolStats get_stats() const override;
  [[nodiscard]] std::string get_name() const override { return "File"; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Protocol with retry logic
 */
class RetryProtocol : public IProtocol {
 public:
  struct RetryConfig {
    size_t max_retries;
    std::chrono::milliseconds initial_delay;
    std::chrono::milliseconds max_delay;
    double backoff_multiplier;
    bool retry_on_timeout;
    bool retry_on_connection_failure;

    // Default constructor
    RetryConfig()
        : max_retries(3),
          initial_delay(100),
          max_delay(5000),
          backoff_multiplier(2.0),
          retry_on_timeout(true),
          retry_on_connection_failure(true) {}
  };

  explicit RetryProtocol(std::shared_ptr<IProtocol> underlying_protocol,
                        RetryConfig config = RetryConfig());
  ~RetryProtocol() override;

  [[nodiscard]] SolarSystem::Utils::Expected<void, ProtocolError> initialize() override;
  void shutdown() override;
  [[nodiscard]] SolarSystem::Utils::Expected<void, ProtocolError> send(
      const Message& message) override;
  [[nodiscard]] SolarSystem::Utils::Expected<Message, ProtocolError> receive(
      std::chrono::milliseconds timeout) override;
  [[nodiscard]] SolarSystem::Utils::Expected<Message, ProtocolError> request(
      const Message& request, std::chrono::milliseconds timeout) override;
  [[nodiscard]] bool is_ready() const override;
  [[nodiscard]] ProtocolStats get_stats() const override;
  [[nodiscard]] std::string get_name() const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Protocol monitor for diagnostics
 */
class ProtocolMonitor {
 public:
  using MessageCallback = std::function<void(const Message&, bool /* is_outgoing */)>;
  using ErrorCallback = std::function<void(ProtocolError, const std::string& /* context */)>;

  explicit ProtocolMonitor(std::shared_ptr<IProtocol> protocol);

  /**
   * @brief Set message callback
   */
  void set_message_callback(MessageCallback callback);

  /**
   * @brief Set error callback
   */
  void set_error_callback(ErrorCallback callback);

  /**
   * @brief Get monitored protocol
   */
  [[nodiscard]] std::shared_ptr<IProtocol> get_protocol() const;

  /**
   * @brief Get message history
   */
  [[nodiscard]] std::vector<Message> get_message_history(size_t max_count = 100) const;

  /**
   * @brief Get error history
   */
  [[nodiscard]] std::vector<std::pair<ProtocolError, std::string>> get_error_history(
      size_t max_count = 100) const;

  /**
   * @brief Clear history
   */
  void clear_history();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Communication
