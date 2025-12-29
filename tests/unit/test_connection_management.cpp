/**
 * @file test_connection_management.cpp
 * @brief Unit tests for connection management system (Task 12)
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/connection/connection_manager.hpp"
#include "solar_core/streaming/realtime_stream.hpp"

using namespace SolarSystem::Connection;

// Mock connection for testing
class MockConnection : public IConnection {
 public:
  MockConnection(std::string id, bool should_fail = false)
      : id_(std::move(id)), should_fail_(should_fail), connected_(false) {}

  bool connect() override {
    if (should_fail_) return false;
    connected_ = true;
    return true;
  }

  void disconnect() override { connected_ = false; }

  bool is_connected() const override { return connected_; }

  bool health_check() override { return connected_; }

  ConnectionHealth get_health() const override {
    ConnectionHealth health;
    health.state = connected_ ? ConnectionState::CONNECTED : ConnectionState::DISCONNECTED;
    health.health_score = connected_ ? 1.0 : 0.0;
    return health;
  }

  std::string get_id() const override { return id_; }

 private:
  std::string id_;
  bool should_fail_;
  bool connected_;
};
TEST(ConnectionManagementTests, Connection_State_Conversion) {
    auto state_str = to_string(ConnectionState::CONNECTED);
    if (state_str.empty()) throw std::runtime_error("State string should not be empty");
}

TEST(ConnectionManagementTests, Connection_Manager_Initialization) {
    ConnectionConfig config;
    config.connection_timeout = std::chrono::seconds(5);
    config.max_reconnection_attempts = 3;

    ConnectionManager manager(config);
    if (manager.is_running()) throw std::runtime_error("Manager should not be running initially");
}

TEST(ConnectionManagementTests, Add_and_Remove_Connections) {
    ConnectionManager manager;
    auto conn = std::make_shared<MockConnection>("test_conn");

    manager.add_connection(conn);
    auto health = manager.get_connection_health("test_conn");
    if (!health.has_value()) throw std::runtime_error("Connection should exist");

    manager.remove_connection("test_conn");
    health = manager.get_connection_health("test_conn");
    if (health.has_value()) throw std::runtime_error("Connection should be removed");
}

TEST(ConnectionManagementTests, Exponential_Backoff) {
    ExponentialBackoff backoff(std::chrono::milliseconds(100), std::chrono::milliseconds(1000),
                               2.0);

    auto delay1 = backoff.next_delay();
    if (delay1.count() < 100) throw std::runtime_error("First delay should be at least 100ms");

    auto delay2 = backoff.next_delay();
    if (delay2.count() <= delay1.count())
      throw std::runtime_error("Second delay should be greater than first");

    auto delay3 = backoff.next_delay();
    if (delay3.count() <= delay2.count())
      throw std::runtime_error("Third delay should be greater than second");

    backoff.reset();
    auto delay_reset = backoff.next_delay();
    if (delay_reset.count() > delay3.count())
      throw std::runtime_error("Reset delay should be less than previous");
}

TEST(ConnectionManagementTests, Connection_Pool) {
    ConnectionPool pool(5);

    auto conn1 = std::make_shared<MockConnection>("conn1");
    auto conn2 = std::make_shared<MockConnection>("conn2");

    pool.add_connection(conn1);
    pool.add_connection(conn2);

    if (pool.get_total_count() != 2) throw std::runtime_error("Pool should have 2 connections");

    auto retrieved = pool.get_connection("conn1");
    if (!retrieved) throw std::runtime_error("Should retrieve connection by ID");
    if (retrieved->get_id() != "conn1") throw std::runtime_error("Wrong connection retrieved");
}

TEST(ConnectionManagementTests, Connection_Health_Monitoring) {
    auto conn = std::make_shared<MockConnection>("healthy_conn");
    conn->connect();

    auto health = conn->get_health();
    if (health.state != ConnectionState::CONNECTED)
      throw std::runtime_error("Connection should be connected");
    if (health.health_score != 1.0) throw std::runtime_error("Health score should be 1.0");

    conn->disconnect();
    health = conn->get_health();
    if (health.state != ConnectionState::DISCONNECTED)
      throw std::runtime_error("Connection should be disconnected");
}

