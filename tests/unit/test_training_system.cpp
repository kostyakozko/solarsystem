/**
 * @file test_training_system.cpp
 * @brief Test training system (Task 35)
 * @note Migrated to Google Test
 *
 * Tests the HelpSystem's tutorial and topic management capabilities:
 * - Topic registration and retrieval
 * - Search functionality
 * - Tutorial management
 *
 * Requirements: All requirements
 */

#include <gtest/gtest.h>

#include <solar_core/help/help_system.hpp>

using namespace SolarSystem::Help;

class TrainingSystemTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& help = HelpSystem::instance();

    HelpTopic topic1;
    topic1.id = "test_dev";
    topic1.title = "Test Development";
    topic1.content = "Guide for developing unit tests in the solar system suite.";
    topic1.category = HelpCategory::TUTORIALS;
    topic1.keywords = {"testing", "development", "unit"};
    topic1.format = HelpFormat::PLAIN_TEXT;
    help.register_topic(topic1);

    HelpTopic topic2;
    topic2.id = "test_debug";
    topic2.title = "Test Debugging";
    topic2.content = "How to debug failing tests and diagnose issues.";
    topic2.category = HelpCategory::TROUBLESHOOTING;
    topic2.keywords = {"debugging", "troubleshooting", "tests"};
    topic2.format = HelpFormat::PLAIN_TEXT;
    help.register_topic(topic2);

    Tutorial tutorial;
    tutorial.id = "intro_testing";
    tutorial.title = "Introduction to Testing";
    tutorial.description = "A beginner tutorial for writing tests.";
    tutorial.difficulty_level = "beginner";

    TutorialStep step;
    step.title = "Write your first test";
    step.description = "Create a simple assertion.";
    step.example_command = "ASSERT_EQ(1, 1);";
    step.expected_output = "Test passed";
    tutorial.steps.push_back(step);

    help.register_tutorial(tutorial);
  }
};

TEST_F(TrainingSystemTest, TopicRegistrationAndRetrieval) {
  auto& help = HelpSystem::instance();
  auto topic = help.get_topic("test_dev");
  ASSERT_TRUE(topic.has_value());
  EXPECT_EQ(topic->title, "Test Development");
  EXPECT_EQ(topic->category, HelpCategory::TUTORIALS);
}

TEST_F(TrainingSystemTest, TopicNotFound) {
  auto& help = HelpSystem::instance();
  auto topic = help.get_topic("nonexistent_topic");
  EXPECT_FALSE(topic.has_value());
}

TEST_F(TrainingSystemTest, SearchFindsRelevantTopics) {
  auto& help = HelpSystem::instance();
  auto results = help.search("debugging");
  ASSERT_FALSE(results.empty());
  bool found_debug_topic = false;
  for (const auto& result : results) {
    if (result.topic_id == "test_debug") {
      found_debug_topic = true;
      break;
    }
  }
  EXPECT_TRUE(found_debug_topic);
}

TEST_F(TrainingSystemTest, TutorialRetrieval) {
  auto& help = HelpSystem::instance();
  auto tutorial = help.get_tutorial("intro_testing");
  ASSERT_TRUE(tutorial.has_value());
  EXPECT_EQ(tutorial->title, "Introduction to Testing");
  EXPECT_EQ(tutorial->difficulty_level, "beginner");
  EXPECT_FALSE(tutorial->steps.empty());
}

TEST_F(TrainingSystemTest, TutorialListing) {
  auto& help = HelpSystem::instance();
  auto tutorials = help.get_all_tutorials();
  ASSERT_GE(tutorials.size(), 1u);
  bool found = false;
  for (const auto& t : tutorials) {
    if (t.id == "intro_testing") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}
