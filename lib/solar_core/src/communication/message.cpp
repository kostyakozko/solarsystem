/**
 * @file message.cpp
 * @brief Implementation of message serialization and validation
 */

#include "solar_core/communication/message.hpp"

#include <algorithm>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>

// OpenSSL for cryptographic signature validation
#ifdef OPENSSL_VERSION_NUMBER
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#endif

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

bool MessageValidator::validate_signature(const Message& message,
                                          const std::string& public_key_pem) {
  // If no signature is present, validation passes (unsigned message)
  if (!message.signature || message.signature->empty()) {
    return true;
  }

  // If signature is present but no public key provided, validation fails
  if (public_key_pem.empty()) {
    return false;
  }

#ifdef OPENSSL_VERSION_NUMBER
  // Use OpenSSL for signature verification

  // Create a message digest of the message content
  std::string message_content = message.header.message_id + message.header.source_application +
                                message.header.destination_application;

  // Add payload to message content
  for (const auto& [key, value] : message.payload) {
    message_content += key;
    if (std::holds_alternative<std::string>(value)) {
      message_content += std::get<std::string>(value);
    }
  }

  // Load public key from PEM format
  BIO* bio = BIO_new_mem_buf(public_key_pem.data(), static_cast<int>(public_key_pem.size()));
  if (!bio) {
    return false;
  }

  EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);

  if (!pkey) {
    return false;
  }

  // Create verification context
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    EVP_PKEY_free(pkey);
    return false;
  }

  // Initialize verification with SHA256
  if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) != 1) {
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return false;
  }

  // Update with message content
  if (EVP_DigestVerifyUpdate(ctx, message_content.data(), message_content.size()) != 1) {
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return false;
  }

  // Verify signature
  int result =
      EVP_DigestVerifyFinal(ctx, reinterpret_cast<const unsigned char*>(message.signature->data()),
                            message.signature->size());

  // Cleanup
  EVP_MD_CTX_free(ctx);
  EVP_PKEY_free(pkey);

  return result == 1;
#else
  // OpenSSL not available, cannot verify signatures
  return false;
#endif
}

bool MessageValidator::check_size_limits(const Message& message, size_t max_size) {
  // Calculate accurate message size including all fields and overhead
  size_t estimated_size = 0;

  // Header fields
  estimated_size += message.header.message_id.size();
  estimated_size += message.header.source_application.size();
  estimated_size += message.header.destination_application.size();
  estimated_size += sizeof(MessageType);                            // type enum
  estimated_size += sizeof(MessagePriority);                        // priority enum
  estimated_size += sizeof(std::chrono::system_clock::time_point);  // timestamp

  // Optional correlation_id
  if (message.header.correlation_id) {
    estimated_size += message.header.correlation_id->size();
  }

  // Optional timeout
  if (message.header.timeout) {
    estimated_size += sizeof(std::chrono::milliseconds);
  }

  // Metadata
  for (const auto& [key, value] : message.header.metadata) {
    estimated_size += key.size() + value.size();
  }

  // Payload
  for (const auto& [key, value] : message.payload) {
    estimated_size += key.size();

    if (std::holds_alternative<std::string>(value)) {
      estimated_size += std::get<std::string>(value).size();
    } else if (std::holds_alternative<int64_t>(value)) {
      estimated_size += sizeof(int64_t);
    } else if (std::holds_alternative<double>(value)) {
      estimated_size += sizeof(double);
    } else if (std::holds_alternative<bool>(value)) {
      estimated_size += sizeof(bool);
    } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
      estimated_size += std::get<std::vector<uint8_t>>(value).size();
    }
  }

  // Optional signature
  if (message.signature) {
    estimated_size += message.signature->size();
  }

  // Add overhead for serialization format (approximately 20% for JSON, 10% for binary)
  estimated_size = static_cast<size_t>(static_cast<double>(estimated_size) * 1.2);

  return estimated_size <= max_size;
}

// Helper function to convert MessageType to string
static std::string message_type_to_string(MessageType type) {
  switch (type) {
    case MessageType::REQUEST:
      return "REQUEST";
    case MessageType::RESPONSE:
      return "RESPONSE";
    case MessageType::NOTIFICATION:
      return "NOTIFICATION";
    case MessageType::ERROR:
      return "ERROR";
    case MessageType::HEARTBEAT:
      return "HEARTBEAT";
    default:
      return "UNKNOWN";
  }
}

// Helper function to convert string to MessageType
static MessageType string_to_message_type(const std::string& type_str) {
  if (type_str == "REQUEST") return MessageType::REQUEST;
  if (type_str == "RESPONSE") return MessageType::RESPONSE;
  if (type_str == "NOTIFICATION") return MessageType::NOTIFICATION;
  if (type_str == "ERROR") return MessageType::ERROR;
  if (type_str == "HEARTBEAT") return MessageType::HEARTBEAT;
  return MessageType::NOTIFICATION;  // Default
}

// Helper function to convert MessagePriority to int
static int message_priority_to_int(MessagePriority priority) { return static_cast<int>(priority); }

// Helper function to convert int to MessagePriority
static MessagePriority int_to_message_priority(int priority_int) {
  switch (priority_int) {
    case 0:
      return MessagePriority::LOW;
    case 1:
      return MessagePriority::NORMAL;
    case 2:
      return MessagePriority::HIGH;
    case 3:
      return MessagePriority::CRITICAL;
    default:
      return MessagePriority::NORMAL;
  }
}

// Helper function to convert MessageValue to JSON
static nlohmann::json message_value_to_json(const MessageValue& value) {
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<int64_t>(value)) {
    return std::get<int64_t>(value);
  } else if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  } else if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value);
  } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
    const auto& vec = std::get<std::vector<uint8_t>>(value);
    return nlohmann::json::binary(vec);
  }
  return nullptr;
}

// Helper function to convert JSON to MessageValue
static MessageValue json_to_message_value(const nlohmann::json& j) {
  if (j.is_string()) {
    return j.get<std::string>();
  } else if (j.is_number_integer()) {
    return j.get<int64_t>();
  } else if (j.is_number_float()) {
    return j.get<double>();
  } else if (j.is_boolean()) {
    return j.get<bool>();
  } else if (j.is_binary()) {
    return j.get_binary();
  }
  return std::string("");  // Default to empty string
}

// JSON Serializer implementation
SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>
JsonMessageSerializer::serialize(const Message& message) const {
  try {
    nlohmann::json j;

    // Serialize header
    j["header"]["message_id"] = message.header.message_id;
    j["header"]["type"] = message_type_to_string(message.header.type);
    j["header"]["priority"] = message_priority_to_int(message.header.priority);
    j["header"]["source_application"] = message.header.source_application;
    j["header"]["destination_application"] = message.header.destination_application;

    // Serialize timestamp as milliseconds since epoch
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            message.header.timestamp.time_since_epoch())
                            .count();
    j["header"]["timestamp"] = timestamp_ms;

    // Serialize optional fields
    if (message.header.correlation_id) {
      j["header"]["correlation_id"] = *message.header.correlation_id;
    }

    if (message.header.timeout) {
      j["header"]["timeout"] = message.header.timeout->count();
    }

    // Serialize metadata
    if (!message.header.metadata.empty()) {
      j["header"]["metadata"] = message.header.metadata;
    }

    // Serialize payload
    nlohmann::json payload_json;
    for (const auto& [key, value] : message.payload) {
      payload_json[key] = message_value_to_json(value);
    }
    j["payload"] = payload_json;

    // Serialize optional signature
    if (message.signature) {
      j["signature"] = *message.signature;
    }

    // Convert to bytes
    std::string json_str = j.dump();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());

  } catch (const nlohmann::json::exception& e) {
    return SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>(
        SerializationError::INVALID_FORMAT);
  } catch (...) {
    return SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>(
        SerializationError::CORRUPTED_DATA);
  }
}

SolarSystem::Utils::Expected<Message, SerializationError> JsonMessageSerializer::deserialize(
    const std::vector<uint8_t>& data) const {
  try {
    // Convert bytes to string
    std::string json_str(data.begin(), data.end());

    // Parse JSON
    nlohmann::json j = nlohmann::json::parse(json_str);

    Message msg;

    // Deserialize header
    msg.header.message_id = j["header"]["message_id"].get<std::string>();
    msg.header.type = string_to_message_type(j["header"]["type"].get<std::string>());
    msg.header.priority = int_to_message_priority(j["header"]["priority"].get<int>());
    msg.header.source_application = j["header"]["source_application"].get<std::string>();
    msg.header.destination_application = j["header"]["destination_application"].get<std::string>();

    // Deserialize timestamp
    int64_t timestamp_ms = j["header"]["timestamp"].get<int64_t>();
    msg.header.timestamp =
        std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp_ms));

    // Deserialize optional fields
    if (j["header"].contains("correlation_id")) {
      msg.header.correlation_id = j["header"]["correlation_id"].get<std::string>();
    }

    if (j["header"].contains("timeout")) {
      msg.header.timeout = std::chrono::milliseconds(j["header"]["timeout"].get<int64_t>());
    }

    // Deserialize metadata
    if (j["header"].contains("metadata")) {
      msg.header.metadata = j["header"]["metadata"].get<std::map<std::string, std::string>>();
    }

    // Deserialize payload
    if (j.contains("payload")) {
      for (auto& [key, value] : j["payload"].items()) {
        msg.payload[key] = json_to_message_value(value);
      }
    }

    // Deserialize optional signature
    if (j.contains("signature")) {
      msg.signature = j["signature"].get<std::string>();
    }

    return msg;

  } catch (const nlohmann::json::parse_error& e) {
    return SolarSystem::Utils::Expected<Message, SerializationError>(
        SerializationError::INVALID_FORMAT);
  } catch (const nlohmann::json::exception& e) {
    return SolarSystem::Utils::Expected<Message, SerializationError>(
        SerializationError::CORRUPTED_DATA);
  } catch (...) {
    return SolarSystem::Utils::Expected<Message, SerializationError>(
        SerializationError::VALIDATION_FAILED);
  }
}

// Binary Serializer using MessagePack
SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>
BinaryMessageSerializer::serialize(const Message& message) const {
  try {
    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(buffer);

    // Pack as a map with 3 keys: header, payload, signature
    packer.pack_map(message.signature ? 3 : 2);

    // Pack header
    packer.pack("header");
    packer.pack_map(9);  // 9 header fields

    packer.pack("message_id");
    packer.pack(message.header.message_id);

    packer.pack("type");
    packer.pack(static_cast<int>(message.header.type));

    packer.pack("priority");
    packer.pack(static_cast<int>(message.header.priority));

    packer.pack("source_application");
    packer.pack(message.header.source_application);

    packer.pack("destination_application");
    packer.pack(message.header.destination_application);

    packer.pack("timestamp");
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            message.header.timestamp.time_since_epoch())
                            .count();
    packer.pack(timestamp_ms);

    packer.pack("correlation_id");
    if (message.header.correlation_id) {
      packer.pack(*message.header.correlation_id);
    } else {
      packer.pack_nil();
    }

    packer.pack("timeout");
    if (message.header.timeout) {
      packer.pack(message.header.timeout->count());
    } else {
      packer.pack_nil();
    }

    packer.pack("metadata");
    packer.pack_map(static_cast<uint32_t>(message.header.metadata.size()));
    for (const auto& [key, value] : message.header.metadata) {
      packer.pack(key);
      packer.pack(value);
    }

    // Pack payload
    packer.pack("payload");
    packer.pack_map(static_cast<uint32_t>(message.payload.size()));
    for (const auto& [key, value] : message.payload) {
      packer.pack(key);

      if (std::holds_alternative<std::string>(value)) {
        packer.pack(std::get<std::string>(value));
      } else if (std::holds_alternative<int64_t>(value)) {
        packer.pack(std::get<int64_t>(value));
      } else if (std::holds_alternative<double>(value)) {
        packer.pack(std::get<double>(value));
      } else if (std::holds_alternative<bool>(value)) {
        packer.pack(std::get<bool>(value));
      } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
        const auto& vec = std::get<std::vector<uint8_t>>(value);
        packer.pack_bin(static_cast<uint32_t>(vec.size()));
        packer.pack_bin_body(reinterpret_cast<const char*>(vec.data()),
                             static_cast<uint32_t>(vec.size()));
      }
    }

    // Pack signature if present
    if (message.signature) {
      packer.pack("signature");
      packer.pack(*message.signature);
    }

    // Convert buffer to vector
    return std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size());

  } catch (const std::exception& e) {
    return SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>(
        SerializationError::INVALID_FORMAT);
  } catch (...) {
    return SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>(
        SerializationError::CORRUPTED_DATA);
  }
}

SolarSystem::Utils::Expected<Message, SerializationError> BinaryMessageSerializer::deserialize(
    const std::vector<uint8_t>& data) const {
  try {
    // Unpack the data
    msgpack::object_handle oh =
        msgpack::unpack(reinterpret_cast<const char*>(data.data()), data.size());
    msgpack::object obj = oh.get();

    // Convert to map
    if (obj.type != msgpack::type::MAP) {
      return SolarSystem::Utils::Expected<Message, SerializationError>(
          SerializationError::INVALID_FORMAT);
    }

    Message msg;
    msgpack::object_kv* p = obj.via.map.ptr;
    msgpack::object_kv* const pend = obj.via.map.ptr + obj.via.map.size;

    for (; p < pend; ++p) {
      std::string key;
      p->key.convert(key);

      if (key == "header") {
        // Unpack header
        if (p->val.type != msgpack::type::MAP) {
          return SolarSystem::Utils::Expected<Message, SerializationError>(
              SerializationError::INVALID_FORMAT);
        }

        msgpack::object_kv* hp = p->val.via.map.ptr;
        msgpack::object_kv* const hpend = p->val.via.map.ptr + p->val.via.map.size;

        for (; hp < hpend; ++hp) {
          std::string hkey;
          hp->key.convert(hkey);

          if (hkey == "message_id") {
            hp->val.convert(msg.header.message_id);
          } else if (hkey == "type") {
            int type_int;
            hp->val.convert(type_int);
            msg.header.type = static_cast<MessageType>(type_int);
          } else if (hkey == "priority") {
            int priority_int;
            hp->val.convert(priority_int);
            msg.header.priority = static_cast<MessagePriority>(priority_int);
          } else if (hkey == "source_application") {
            hp->val.convert(msg.header.source_application);
          } else if (hkey == "destination_application") {
            hp->val.convert(msg.header.destination_application);
          } else if (hkey == "timestamp") {
            int64_t timestamp_ms;
            hp->val.convert(timestamp_ms);
            msg.header.timestamp =
                std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp_ms));
          } else if (hkey == "correlation_id") {
            if (hp->val.type != msgpack::type::NIL) {
              std::string corr_id;
              hp->val.convert(corr_id);
              msg.header.correlation_id = corr_id;
            }
          } else if (hkey == "timeout") {
            if (hp->val.type != msgpack::type::NIL) {
              int64_t timeout_ms;
              hp->val.convert(timeout_ms);
              msg.header.timeout = std::chrono::milliseconds(timeout_ms);
            }
          } else if (hkey == "metadata") {
            if (hp->val.type == msgpack::type::MAP) {
              msgpack::object_kv* mp = hp->val.via.map.ptr;
              msgpack::object_kv* const mpend = hp->val.via.map.ptr + hp->val.via.map.size;
              for (; mp < mpend; ++mp) {
                std::string mkey, mval;
                mp->key.convert(mkey);
                mp->val.convert(mval);
                msg.header.metadata[mkey] = mval;
              }
            }
          }
        }
      } else if (key == "payload") {
        // Unpack payload
        if (p->val.type != msgpack::type::MAP) {
          return SolarSystem::Utils::Expected<Message, SerializationError>(
              SerializationError::INVALID_FORMAT);
        }

        msgpack::object_kv* pp = p->val.via.map.ptr;
        msgpack::object_kv* const ppend = p->val.via.map.ptr + p->val.via.map.size;

        for (; pp < ppend; ++pp) {
          std::string pkey;
          pp->key.convert(pkey);

          // Determine type and convert
          if (pp->val.type == msgpack::type::STR) {
            std::string str_val;
            pp->val.convert(str_val);
            msg.payload[pkey] = str_val;
          } else if (pp->val.type == msgpack::type::POSITIVE_INTEGER ||
                     pp->val.type == msgpack::type::NEGATIVE_INTEGER) {
            int64_t int_val;
            pp->val.convert(int_val);
            msg.payload[pkey] = int_val;
          } else if (pp->val.type == msgpack::type::FLOAT32 ||
                     pp->val.type == msgpack::type::FLOAT64) {
            double dbl_val;
            pp->val.convert(dbl_val);
            msg.payload[pkey] = dbl_val;
          } else if (pp->val.type == msgpack::type::BOOLEAN) {
            bool bool_val;
            pp->val.convert(bool_val);
            msg.payload[pkey] = bool_val;
          } else if (pp->val.type == msgpack::type::BIN) {
            const char* bin_data = pp->val.via.bin.ptr;
            uint32_t bin_size = pp->val.via.bin.size;
            std::vector<uint8_t> bin_vec(bin_data, bin_data + bin_size);
            msg.payload[pkey] = bin_vec;
          }
        }
      } else if (key == "signature") {
        std::string sig;
        p->val.convert(sig);
        msg.signature = sig;
      }
    }

    return msg;

  } catch (const msgpack::type_error& e) {
    return SolarSystem::Utils::Expected<Message, SerializationError>(
        SerializationError::INVALID_FORMAT);
  } catch (const std::exception& e) {
    return SolarSystem::Utils::Expected<Message, SerializationError>(
        SerializationError::CORRUPTED_DATA);
  } catch (...) {
    return SolarSystem::Utils::Expected<Message, SerializationError>(
        SerializationError::VALIDATION_FAILED);
  }
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
