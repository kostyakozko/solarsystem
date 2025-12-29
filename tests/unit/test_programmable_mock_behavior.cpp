/**
 * @file test_programmable_mock_behavior.cpp
 * @brief Programmable mock behavior system tests (Task 12)
 * @note Migrated to Google Test
 *
 * Tests programmable mock behavior:
 * - Flexible mock configuration and response programming
 * - State-based mock behavior with transitions
 * - Conditional mock responses based on input
 * - Mock interaction recording and playback
 *
 * Requirements: 4.3, 4.5
 */

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

/**
 * @brief Programmable mock behavior system
 */
class ProgrammableMock {
 public:
  // Mock state
  enum class State { Initial, Active, Error, Complete };

  // Response configuration
  struct Response {
    std::string data;
    int status_code = 200;
    int delay_ms = 0;
  };

  // Behavior rule
  using BehaviorRule = std::function<Response(const std::string&)>;

  // Constructor
  ProgrammableMock() : current_state_(State::Initial) {}

  // Configure behavior
  void set_behavior(const std::string& key, BehaviorRule rule) {
    behaviors_[key] = rule;
  }

  // Execute with behavior
  Response execute(const std::string& input) {
    record_interaction(input);

    // Check for configured behavior
    for (const auto& [key, rule] : behaviors_) {
      if (input.find(key) != std::string::npos) {
        return rule(input);
      }
    }

    // Default response
    return Response{"default", 200, 0};
  }

  // State management
  void set_state(State state) { current_state_ = state; }
  State get_state() const { return current_state_; }

  // State-based behavior
  Response execute_stateful(const std::string& input) {
    record_interaction(input);

    Response resp;
    switch (current_state_) {
      case State::Initial:
        resp = Response{"initialized", 200, 0};
        current_state_ = State::Active;
        break;
      case State::Active:
        resp = Response{"processing", 200, 0};
        break;
      case State::Error:
        resp = Response{"error", 500, 0};
        break;
      case State::Complete:
        resp = Response{"done", 200, 0};
        break;
    }
    return resp;
  }

  // Recording
  void record_interaction(const std::string& input) {
    interactions_.push_back(input);
  }

  const std::vector<std::string>& get_interactions() const {
    return interactions_;
  }

  void clear_interactions() { interactions_.clear(); }

  // Playback
  void record_response(const Response& resp) {
    recorded_responses_.push_back(resp);
  }

  Response playback(size_t index) {
    if (index < recorded_responses_.size()) {
      return recorded_responses_[index];
    }
    return Response{"", 404, 0};
  }

 private:
  State current_state_;
  std::map<std::string, BehaviorRule> behaviors_;
  std::vector<std::string> interactions_;
  std::vector<Response> recorded_responses_;
};
  TEST_SUITE("Programmable Mock Behavior Tests");

  // Test 1: Flexible mock configuration
  TEST_CASE("Flexible Mock Configuration") {
    ProgrammableMock mock;

    // Test 1.1: Simple behavior
    {
      mock.set_behavior("test", [](const std::string&) {
        return ProgrammableMock::Response{"test_response", 200, 0};
      });

      auto resp = mock.execute("test_input");
      ASSERT_EQ(resp.data, "test_response");
      ASSERT_EQ(resp.status_code, 200);
    }

    // Test 1.2: Multiple behaviors
    {
      mock.set_behavior("success", [](const std::string&) {
        return ProgrammableMock::Response{"ok", 200, 0};
      });

      mock.set_behavior("error", [](const std::string&) {
        return ProgrammableMock::Response{"failed", 500, 0};
      });

      auto resp1 = mock.execute("success_case");
      ASSERT_EQ(resp1.status_code, 200);

      auto resp2 = mock.execute("error_case");
      ASSERT_EQ(resp2.status_code, 500);
    }

    // Test 1.3: Input-based behavior
    {
      mock.set_behavior("echo", [](const std::string& input) {
        return ProgrammableMock::Response{input, 200, 0};
      });

      auto resp = mock.execute("echo_hello");
      EXPECT_NE(std::string::npos, resp.data.find("echo_hello"));
    }
  });

  // Test 2: State-based mock behavior
  TEST_CASE("State-Based Mock Behavior") {
    ProgrammableMock mock;

    // Test 2.1: Initial state
    {
      ASSERT_TRUE(mock.get_state() == ProgrammableMock::State::Initial);

      auto resp = mock.execute_stateful("start");
      ASSERT_EQ(resp.data, "initialized");
      ASSERT_TRUE(mock.get_state() == ProgrammableMock::State::Active);
    }

    // Test 2.2: State transitions
    {
      auto resp = mock.execute_stateful("process");
      ASSERT_EQ(resp.data, "processing");
      ASSERT_TRUE(mock.get_state() == ProgrammableMock::State::Active);
    }

    // Test 2.3: Error state
    {
      mock.set_state(ProgrammableMock::State::Error);
      auto resp = mock.execute_stateful("retry");
      ASSERT_EQ(resp.status_code, 500);
    }

    // Test 2.4: Complete state
    {
      mock.set_state(ProgrammableMock::State::Complete);
      auto resp = mock.execute_stateful("finish");
      ASSERT_EQ(resp.data, "done");
    }
  });

  // Test 3: Conditional responses
  TEST_CASE("Conditional Mock Responses") {
    ProgrammableMock mock;

    // Test 3.1: Condition based on input length
    {
      mock.set_behavior("length", [](const std::string& input) {
        if (input.length() > 20) {
          return ProgrammableMock::Response{"too_long", 400, 0};
        }
        return ProgrammableMock::Response{"ok", 200, 0};
      });

      auto resp1 = mock.execute("length_short");
      ASSERT_EQ(resp1.status_code, 200);

      auto resp2 = mock.execute("length_this_is_a_very_long_input_string");
      ASSERT_EQ(resp2.status_code, 400);
    }

    // Test 3.2: Condition based on content
    {
      mock.set_behavior("validate", [](const std::string& input) {
        if (input.find("invalid") != std::string::npos) {
          return ProgrammableMock::Response{"rejected", 400, 0};
        }
        if (input.find("valid") != std::string::npos) {
          return ProgrammableMock::Response{"accepted", 200, 0};
        }
        return ProgrammableMock::Response{"unknown", 400, 0};
      });

      auto resp1 = mock.execute("validate_valid_data");
      ASSERT_EQ(resp1.data, "accepted");

      auto resp2 = mock.execute("validate_invalid_data");
      ASSERT_EQ(resp2.data, "rejected");
    }

    // Test 3.3: Multiple conditions
    {
      mock.set_behavior("complex", [](const std::string& input) {
        if (input.empty()) {
          return ProgrammableMock::Response{"empty", 400, 0};
        }
        if (input.find("admin") != std::string::npos) {
          return ProgrammableMock::Response{"authorized", 200, 0};
        }
        return ProgrammableMock::Response{"unauthorized", 403, 0};
      });

      auto resp1 = mock.execute("complex_admin_request");
      ASSERT_EQ(resp1.status_code, 200);

      auto resp2 = mock.execute("complex_user_request");
      ASSERT_EQ(resp2.status_code, 403);
    }
  });

  // Test 4: Interaction recording
  TEST_CASE("Mock Interaction Recording") {
    ProgrammableMock mock;

    // Test 4.1: Record interactions
    {
      mock.execute("request1");
      mock.execute("request2");
      mock.execute("request3");

      const auto& interactions = mock.get_interactions();
      ASSERT_EQ(interactions.size(), 3);
      ASSERT_EQ(interactions[0], "request1");
      ASSERT_EQ(interactions[1], "request2");
      ASSERT_EQ(interactions[2], "request3");
    }

    // Test 4.2: Clear interactions
    {
      mock.clear_interactions();
      ASSERT_EQ(mock.get_interactions().size(), 0);
    }

    // Test 4.3: Multiple recordings
    {
      for (int i = 0; i < 10; ++i) {
        mock.execute("request_" + std::to_string(i));
      }

      ASSERT_EQ(mock.get_interactions().size(), 10);
    }
  });

  // Test 5: Response playback
  TEST_CASE("Mock Response Playback") {
    ProgrammableMock mock;

    // Test 5.1: Record and playback
    {
      ProgrammableMock::Response resp1{"data1", 200, 0};
      ProgrammableMock::Response resp2{"data2", 201, 0};

      mock.record_response(resp1);
      mock.record_response(resp2);

      auto played1 = mock.playback(0);
      ASSERT_EQ(played1.data, "data1");
      ASSERT_EQ(played1.status_code, 200);

      auto played2 = mock.playback(1);
      ASSERT_EQ(played2.data, "data2");
      ASSERT_EQ(played2.status_code, 201);
    }

    // Test 5.2: Invalid playback index
    {
      auto played = mock.playback(999);
      ASSERT_EQ(played.status_code, 404);
    }

    // Test 5.3: Sequential playback
    {
      ProgrammableMock mock2;  // Use fresh mock to avoid interference
      for (size_t i = 0; i < 5; ++i) {
        mock2.record_response(
            ProgrammableMock::Response{"seq_" + std::to_string(i), 200, 0});
      }

      for (size_t i = 0; i < 5; ++i) {
        auto played = mock2.playback(i);
        EXPECT_NE(std::string::npos, played.data.find("seq_"));
      }
    }
  });

  // Test 6: Integrated scenarios
  TEST_CASE("Integrated Programmable Mock Scenarios") {
    ProgrammableMock mock;

    // Test 6.1: Stateful with recording
    {
      mock.clear_interactions();
      mock.set_state(ProgrammableMock::State::Initial);

      mock.execute_stateful("init");
      mock.execute_stateful("process");
      mock.execute_stateful("complete");

      ASSERT_EQ(mock.get_interactions().size(), 3);
    }

    // Test 6.2: Conditional with state
    {
      mock.set_behavior("state_check", [&mock](const std::string&) {
        if (mock.get_state() == ProgrammableMock::State::Active) {
          return ProgrammableMock::Response{"active_response", 200, 0};
        }
        return ProgrammableMock::Response{"inactive", 503, 0};
      });

      mock.set_state(ProgrammableMock::State::Active);
      auto resp1 = mock.execute("state_check");
      ASSERT_EQ(resp1.status_code, 200);

      mock.set_state(ProgrammableMock::State::Error);
      auto resp2 = mock.execute("state_check");
      ASSERT_EQ(resp2.status_code, 503);
    }

    // Test 6.3: Record, playback, and verify
    {
      mock.clear_interactions();

      // Execute and record
      mock.set_behavior("record", [&mock](const std::string& input) {
        auto resp = ProgrammableMock::Response{input + "_processed", 200, 0};
        mock.record_response(resp);
        return resp;
      });

      mock.execute("record_test1");
      mock.execute("record_test2");

      // Verify interactions
      ASSERT_EQ(mock.get_interactions().size(), 2);

      // Playback
      auto played = mock.playback(0);
      EXPECT_NE(std::string::npos, played.data.find("test1"));
    }
  });

  return current_suite->all_passed() ? 0 : 1;
