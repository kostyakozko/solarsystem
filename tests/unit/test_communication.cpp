/**
 * @file test_communication.cpp
 * @brief Unit tests for communication protocols
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/communication/message.hpp"
#include "solar_core/communication/protocol.hpp"

using namespace SolarSystem::Communication;
TEST(CommunicationProtocolTests, Message_Creation) {
    MessagePayload payload;
    payload["key1"] = std::string("value1");
    payload["key2"] = int64_t(42);

    auto msg = Message::create_request("app1", "app2", payload);

    if (msg.header.type != MessageType::REQUEST) throw std::runtime_error("Wrong message type");
    if (msg.header.source_application != "app1") throw std::runtime_error("Wrong source");
    if (msg.header.destination_application != "app2") throw std::runtime_error("Wrong destination");
    if (msg.header.message_id.empty()) throw std::runtime_error("Empty message ID");
    if (msg.payload.size() != 2) throw std::runtime_error("Wrong payload size");
}

TEST(CommunicationProtocolTests, Message_Validation) {
    auto msg = Message::create_request("source", "dest", {});
    if (!msg.is_valid()) throw std::runtime_error("Valid message failed validation");

    Message invalid_msg;
    invalid_msg.header.message_id = "test";
    invalid_msg.header.source_application = "";
    if (invalid_msg.is_valid()) throw std::runtime_error("Invalid message passed validation");
}

TEST(CommunicationProtocolTests, Response_Creation) {
    auto request = Message::create_request("app1", "app2", {});
    auto response = Message::create_response(request, {});

    if (response.header.type != MessageType::RESPONSE) throw std::runtime_error("Wrong type");
    if (response.header.source_application != "app2") throw std::runtime_error("Wrong source");
    if (!response.header.correlation_id.has_value()) throw std::runtime_error("No correlation ID");
    if (*response.header.correlation_id != request.header.message_id)
      throw std::runtime_error("Wrong correlation ID");
}

TEST(CommunicationProtocolTests, InProcess_Protocol) {
    auto protocol = std::make_shared<InProcessProtocol>();

    auto init_result = protocol->initialize();
    if (!init_result.has_value()) throw std::runtime_error("Init failed");
    if (!protocol->is_ready()) throw std::runtime_error("Protocol not ready");

    auto msg = Message::create_notification("sender", "receiver", {});
    auto send_result = protocol->send(msg);
    if (!send_result.has_value()) throw std::runtime_error("Send failed");

    auto receive_result = protocol->receive(std::chrono::seconds(1));
    if (!receive_result.has_value()) throw std::runtime_error("Receive failed");
    if (receive_result.value().header.message_id != msg.header.message_id)
      throw std::runtime_error("Message ID mismatch");

    protocol->shutdown();
}

TEST(CommunicationProtocolTests, Protocol_Statistics) {
    auto protocol = std::make_shared<InProcessProtocol>();
    auto init = protocol->initialize();
    if (!init.has_value()) throw std::runtime_error("Init failed");

    auto stats_before = protocol->get_stats();
    if (stats_before.messages_sent != 0) throw std::runtime_error("Initial sent count wrong");

    auto msg = Message::create_notification("sender", "receiver", {});
    auto send_res = protocol->send(msg);
    if (!send_res.has_value()) throw std::runtime_error("Send failed");
    auto recv_res = protocol->receive(std::chrono::seconds(1));
    if (!recv_res.has_value()) throw std::runtime_error("Receive failed");

    auto stats_after = protocol->get_stats();
    if (stats_after.messages_sent != 1) throw std::runtime_error("Sent count wrong");
    if (stats_after.messages_received != 1) throw std::runtime_error("Received count wrong");

    protocol->shutdown();
}

