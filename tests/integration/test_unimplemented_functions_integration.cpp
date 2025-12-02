/**
 * @file test_unimplemented_functions_integration.cpp
 * @brief Integration tests for previously unimplemented functions (Task 11.1)
 *
 * This test suite validates the integration of functions implemented
 * as part of the unimplemented-functions-completion specification.
 */

#include "../utils/test_framework.h"
#include "solar_core/communication/message.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace SolarSystem;
using namespace SolarSystem::Communication;

int main() {
  TEST_SUITE("Unimplemented Functions Integration Tests");

  // Test 1: JSON Serialization Round Trip
  TEST_CASE("JSON Serialization Round Trip") {
    JsonMessageSerializer serializer;

    MessageBuilder builder;
    auto msg = builder.set_type(MessageType::REQUEST)
                   .set_priority(MessagePriority::NORMAL)
                   .set_source("test-sender")
                   .set_destination("test-recipient")
                   .add_payload("test_key", std::string("test_value"))
                   .add_payload("test_number", int64_t(42))
                   .build();

    auto serialize_result = serializer.serialize(msg);
    ASSERT_TRUE(serialize_result.has_value());

    auto deserialize_result = serializer.deserialize(serialize_result.value());
    ASSERT_TRUE(deserialize_result.has_value());

    auto deserialized_msg = deserialize_result.value();
    ASSERT_TRUE(msg.header.type == deserialized_msg.header.type);
    ASSERT_EQ(msg.header.source_application, deserialized_msg.header.source_application);
  });

  // Test 2: Message Validation
  TEST_CASE("Message Validation") {
    auto msg = Message::create_request("app1", "app2", MessagePayload{});
    bool is_valid = MessageValidator::validate_structure(msg);
    ASSERT_TRUE(is_valid);
  });

  // Test 3: Create Request Message
  TEST_CASE("Create Request Message") {
    MessagePayload payload;
    payload["action"] = std::string("fetch_data");

    auto msg = Message::create_request("client", "server", payload);
    ASSERT_TRUE(msg.header.type == MessageType::REQUEST);
    ASSERT_TRUE(msg.is_valid());
  });

  // Test 4: Create Response Message
  TEST_CASE("Create Response Message") {
    auto request = Message::create_request("client", "server", MessagePayload{});
    auto response = Message::create_response(request, MessagePayload{});

    ASSERT_TRUE(response.header.type == MessageType::RESPONSE);
    ASSERT_TRUE(response.header.correlation_id.has_value());
  });

  // Test 5: Message Expiration
  TEST_CASE("Message Expiration") {
    MessageBuilder builder;
    auto msg = builder.set_type(MessageType::REQUEST)
                   .set_source("client")
                   .set_destination("server")
                   .set_timeout(std::chrono::milliseconds(100))
                   .build();

    ASSERT_FALSE(msg.is_expired());
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    ASSERT_TRUE(msg.is_expired());
  });

  current_suite->print_summary();
  return current_suite->all_passed() ? 0 : 1;
}
