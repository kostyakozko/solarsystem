/**
 * @file message.cpp
 * @brief Implementation of message serialization and validation
 */

#include "solar_core/communication/message.hpp"

#include <algorithm>
#include <random>
#include <sstream>

namespace SolarSystem::Communication {

namespace {

// Generate unique message ID
std::string generate_message_id() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint64_t> dis;

  std::ostringstream oss;
  oss << "msg_" << std::hex << dis(gen);
  return oss.str();
}

}  // namespace

// Message static factory methods
Message Message::create_request(const std::string& source, const std::string& destination,
                                const MessagePayload& payload) {
  Message msg;
  msg.header.message_id = generate_message_id();
  msg.header.type = MessageType::REQUEST;
  msg.header.source_application = source;
  msg.header.destination_application = destination;
  msg.header.timestamp = std::chrono::system_clock::now();
  msg.payload = payload;
  return msg;
}

Message Message::create_response(const Message& request, const MessagePayload& payload) {
  Message msg;
  msg.header.message_id = generate_message_id();
  msg.header.type = MessageType::RESPONSE;
  msg.header.source_application = request.header.destination_application;
  msg.header.destination_application = request.header.source_application;
  msg.header.correlation_id = request.header.message_id;
  msg.header.timestamp = std::chrono::system_clock::now();
  msg.payload = payload;
  return msg;
}

Message Message::create_notification(const std::string& source, const std::string& destination,
                                     const MessagePayload& payload) {
  Message msg;
  msg.header.message_id = generate_message_id();
  msg.header.type = MessageType::NOTIFICATION;
  msg.header.source_application = source;
  msg.header.destination_application = destination;
  msg.header.timestamp = std::chrono::system_clock::now();
  msg.payload = payload;
  return msg;
}

Message Message::create_error(const std::string& source, const std::string& destination,
                              const std::string& error_message, int error_code) {
  Message msg;
  msg.header.message_id = generate_message_id();
  msg.header.type = MessageType::ERROR;
  msg.header.source_application = source;
  msg.header.destination_application = destination;
  msg.header.timestamp = std::chrono::system_clock::now();
  msg.payload["error_message"] = error_message;
  msg.payload["error_code"] = static_cast<int64_t>(error_code);
  return msg;
}

Message Message::create_heartbeat(const std::string& source) {
  Message msg;
  msg.header.message_id = generate_message_id();
  msg.header.type = MessageType::HEARTBEAT;
  msg.header.source_application = source;
  msg.header.timestamp = std::chrono::system_clock::now();
  return msg;
}

bool Message::is_valid() const { return MessageValidator::validate_structure(*this); }

bool Message::is_expired() const {
  if (!header.timeout) {
    return false;
  }

  auto age = get_age();
  return age > *header.timeout;
}

std::chrono::milliseconds Message::get_age() const {
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now - header.timestamp);
}

// MessageValidator implementation
bool MessageValidator::validate_structure(const Message& message) {
  return validate_header(message.header) && validate_payload(message.payload);
}

bool MessageValidator::validate_header(const MessageHeader& header) {
  // Check required fields
  if (header.message_id.empty()) {
    return false;
  }

  if (header.source_application.empty()) {
    return false;
  }

  // Response messages must have correlation ID
  if (header.type == MessageType::RESPONSE && !header.correlation_id) {
    return false;
  }

  return true;
}

bool MessageValidator::validate_payload(const MessagePayload& /* payload */) {
  // Basic payload validation
  // In a real implementation, you might check payload size, types, etc.
  return true;
}

bool MessageValidator::validate_signature(const Message& /* message */,
                                         const std::string& /* public_key */) {
  // Signature validation would be implemented here
  // For now, return true if no signature is present
  return true;
}

bool MessageValidator::check_size_limits(const Message& message, size_t max_size) {
  // Estimate message size (simplified)
  size_t estimated_size = message.header.message_id.size() +
                         message.header.source_application.size() +
                         message.header.destination_application.size();

  for (const auto& [key, value] : message.payload) {
    estimated_size += key.size();
    if (std::holds_alternative<std::string>(value)) {
      estimated_size += std::get<std::string>(value).size();
    } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
      estimated_size += std::get<std::vector<uint8_t>>(value).size();
    } else {
      estimated_size += 8;  // Approximate size for numeric types
    }
  }

  return estimated_size <= max_size;
}

// JSON Serializer (simplified implementation)
SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>
JsonMessageSerializer::serialize(const Message& /* message */) const {
  // Simplified JSON serialization
  // In a real implementation, use a proper JSON library
  std::string json = "{\"type\":\"message\"}";
  return std::vector<uint8_t>(json.begin(), json.end());
}

SolarSystem::Utils::Expected<Message, SerializationError> JsonMessageSerializer::deserialize(
    const std::vector<uint8_t>& /* data */) const {
  // Simplified JSON deserialization
  // In a real implementation, use a proper JSON library
  Message msg;
  msg.header.message_id = "deserialized";
  msg.header.type = MessageType::NOTIFICATION;
  msg.header.source_application = "unknown";
  msg.header.timestamp = std::chrono::system_clock::now();
  return msg;
}

// Binary Serializer (simplified implementation)
SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>
BinaryMessageSerializer::serialize(const Message& /* message */) const {
  // Simplified binary serialization
  std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
  return data;
}

SolarSystem::Utils::Expected<Message, SerializationError> BinaryMessageSerializer::deserialize(
    const std::vector<uint8_t>& /* data */) const {
  // Simplified binary deserialization
  Message msg;
  msg.header.message_id = "binary_deserialized";
  msg.header.type = MessageType::NOTIFICATION;
  msg.header.source_application = "unknown";
  msg.header.timestamp = std::chrono::system_clock::now();
  return msg;
}

// MessageBuilder implementation
MessageBuilder& MessageBuilder::set_type(MessageType type) {
  message_.header.type = type;
  return *this;
}

MessageBuilder& MessageBuilder::set_priority(MessagePriority priority) {
  message_.header.priority = priority;
  return *this;
}

MessageBuilder& MessageBuilder::set_source(const std::string& source) {
  message_.header.source_application = source;
  return *this;
}

MessageBuilder& MessageBuilder::set_destination(const std::string& destination) {
  message_.header.destination_application = destination;
  return *this;
}

MessageBuilder& MessageBuilder::set_correlation_id(const std::string& correlation_id) {
  message_.header.correlation_id = correlation_id;
  return *this;
}

MessageBuilder& MessageBuilder::set_timeout(std::chrono::milliseconds timeout) {
  message_.header.timeout = timeout;
  return *this;
}

MessageBuilder& MessageBuilder::add_metadata(const std::string& key, const std::string& value) {
  message_.header.metadata[key] = value;
  return *this;
}

MessageBuilder& MessageBuilder::add_payload(const std::string& key, const MessageValue& value) {
  message_.payload[key] = value;
  return *this;
}

Message MessageBuilder::build() {
  if (message_.header.message_id.empty()) {
    message_.header.message_id = generate_message_id();
  }
  if (message_.header.timestamp == std::chrono::system_clock::time_point{}) {
    message_.header.timestamp = std::chrono::system_clock::now();
  }
  return message_;
}

}  // namespace SolarSystem::Communication
