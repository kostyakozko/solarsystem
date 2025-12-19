/**
 * @file message.hpp
 * @brief Standardized message format for inter-application communication
 *
 * Provides a type-safe, serializable message format for communication
 * between Solar System Suite applications with validation and security.
 */

#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "solar_utils/expected.hpp"

namespace SolarSystem::Communication {

/**
 * @brief Message type enumeration
 */
enum class MessageType {
  REQUEST,       ///< Request message
  RESPONSE,      ///< Response message
  NOTIFICATION,  ///< One-way notification
  ERROR,         ///< Error message
  HEARTBEAT      ///< Keep-alive heartbeat
};

/**
 * @brief Message priority levels
 */
enum class MessagePriority {
  LOW = 0,
  NORMAL = 1,
  HIGH = 2,
  CRITICAL = 3
};

/**
 * @brief Message payload value types
 */
using MessageValue = std::variant<std::string, int64_t, double, bool, std::vector<uint8_t>>;

/**
 * @brief Message payload
 */
using MessagePayload = std::map<std::string, MessageValue>;

/**
 * @brief Message header information
 */
struct MessageHeader {
  std::string message_id;                                      ///< Unique message identifier
  MessageType type = MessageType::REQUEST;                     ///< Message type
  MessagePriority priority = MessagePriority::NORMAL;          ///< Message priority
  std::string source_application;                              ///< Source application name
  std::string destination_application;                         ///< Destination application name
  std::chrono::system_clock::time_point timestamp;             ///< Message timestamp
  std::optional<std::string> correlation_id;                   ///< For request/response correlation
  std::optional<std::chrono::milliseconds> timeout;            ///< Message timeout
  std::map<std::string, std::string> metadata;                 ///< Additional metadata
};

/**
 * @brief Complete message structure
 */
struct Message {
  MessageHeader header;
  MessagePayload payload;
  std::optional<std::string> signature;  ///< Optional message signature for security

  /**
   * @brief Create a request message
   */
  static Message create_request(const std::string& source, const std::string& destination,
                                const MessagePayload& payload);

  /**
   * @brief Create a response message
   */
  static Message create_response(const Message& request, const MessagePayload& payload);

  /**
   * @brief Create a notification message
   */
  static Message create_notification(const std::string& source, const std::string& destination,
                                     const MessagePayload& payload);

  /**
   * @brief Create an error message
   */
  static Message create_error(const std::string& source, const std::string& destination,
                              const std::string& error_message, int error_code = -1);

  /**
   * @brief Create a heartbeat message
   */
  static Message create_heartbeat(const std::string& source);

  /**
   * @brief Validate message structure
   */
  [[nodiscard]] bool is_valid() const;

  /**
   * @brief Check if message has expired
   */
  [[nodiscard]] bool is_expired() const;

  /**
   * @brief Get message age
   */
  [[nodiscard]] std::chrono::milliseconds get_age() const;
};

/**
 * @brief Message serialization error types
 */
enum class SerializationError {
  INVALID_FORMAT,
  UNSUPPORTED_TYPE,
  BUFFER_TOO_SMALL,
  CORRUPTED_DATA,
  VALIDATION_FAILED
};

/**
 * @brief Message serializer interface
 */
class IMessageSerializer {
 public:
  virtual ~IMessageSerializer() = default;

  /**
   * @brief Serialize message to bytes
   */
  [[nodiscard]] virtual SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>
  serialize(const Message& message) const = 0;

  /**
   * @brief Deserialize message from bytes
   */
  [[nodiscard]] virtual SolarSystem::Utils::Expected<Message, SerializationError> deserialize(
      const std::vector<uint8_t>& data) const = 0;

  /**
   * @brief Get serializer name
   */
  [[nodiscard]] virtual std::string get_name() const = 0;
};

/**
 * @brief JSON message serializer
 */
class JsonMessageSerializer : public IMessageSerializer {
 public:
  [[nodiscard]] SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError> serialize(
      const Message& message) const override;

  [[nodiscard]] SolarSystem::Utils::Expected<Message, SerializationError> deserialize(
      const std::vector<uint8_t>& data) const override;

  [[nodiscard]] std::string get_name() const override { return "JSON"; }
};

/**
 * @brief Binary message serializer (more efficient)
 */
class BinaryMessageSerializer : public IMessageSerializer {
 public:
  [[nodiscard]] SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError> serialize(
      const Message& message) const override;

  [[nodiscard]] SolarSystem::Utils::Expected<Message, SerializationError> deserialize(
      const std::vector<uint8_t>& data) const override;

  [[nodiscard]] std::string get_name() const override { return "Binary"; }
};

/**
 * @brief Message validator
 */
class MessageValidator {
 public:
  /**
   * @brief Validate message structure
   */
  [[nodiscard]] static bool validate_structure(const Message& message);

  /**
   * @brief Validate message header
   */
  [[nodiscard]] static bool validate_header(const MessageHeader& header);

  /**
   * @brief Validate message payload
   */
  [[nodiscard]] static bool validate_payload(const MessagePayload& payload);

  /**
   * @brief Validate message signature
   */
  [[nodiscard]] static bool validate_signature(const Message& message,
                                               const std::string& public_key);

  /**
   * @brief Check if message size is within limits
   */
  [[nodiscard]] static bool check_size_limits(const Message& message, size_t max_size = 1048576);
};

/**
 * @brief Message builder for fluent API
 */
class MessageBuilder {
 public:
  MessageBuilder() = default;

  MessageBuilder& set_type(MessageType type);
  MessageBuilder& set_priority(MessagePriority priority);
  MessageBuilder& set_source(const std::string& source);
  MessageBuilder& set_destination(const std::string& destination);
  MessageBuilder& set_correlation_id(const std::string& correlation_id);
  MessageBuilder& set_timeout(std::chrono::milliseconds timeout);
  MessageBuilder& add_metadata(const std::string& key, const std::string& value);
  MessageBuilder& add_payload(const std::string& key, const MessageValue& value);

  [[nodiscard]] Message build();

 private:
  Message message_;
};

}  // namespace SolarSystem::Communication
