/**
 * @file test_documentation.cpp
 * @brief Test documentation system (Task 34)
 * @note Migrated to Google Test
 *
 * Tests the HelpSystem's documentation capabilities:
 * - Topic registration and retrieval
 * - Category-based topic listing
 * - Search functionality
 * - Help formatting
 *
 * Requirements: All requirements
 */

#include <gtest/gtest.h>

#include <solar_core/help/help_system.hpp>

using namespace SolarSystem::Help;

class DocumentationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& help = HelpSystem::instance();

    HelpTopic getting_started;
    getting_started.id = "doc_getting_started";
    getting_started.title = "Getting Started Guide";
    getting_started.content =
        "Welcome to the Solar System Suite. This guide covers installation and first run.";
    getting_started.category = HelpCategory::GETTING_STARTED;
    getting_started.keywords = {"install", "setup", "first", "guide"};
    getting_started.format = HelpFormat::PLAIN_TEXT;
    help.register_topic(getting_started);

    HelpTopic config_doc;
    config_doc.id = "doc_configuration";
    config_doc.title = "Configuration Reference";
    config_doc.content =
        "Detailed reference for all configuration parameters and environment variables.";
    config_doc.category = HelpCategory::CONFIGURATION;
    config_doc.keywords = {"configuration", "settings", "parameters", "environment"};
    config_doc.format = HelpFormat::PLAIN_TEXT;
    help.register_topic(config_doc);

    HelpTopic api_doc;
    api_doc.id = "doc_api_reference";
    api_doc.title = "API Reference";
    api_doc.content = "Complete API documentation for the solar system simulation library.";
    api_doc.category = HelpCategory::API_REFERENCE;
    api_doc.keywords = {"api", "reference", "library", "functions"};
    api_doc.format = HelpFormat::MARKDOWN;
    help.register_topic(api_doc);
  }
};

TEST_F(DocumentationTest, RetrieveTopicByName) {
  auto& help = HelpSystem::instance();
  auto topic = help.get_topic("doc_getting_started");
  ASSERT_TRUE(topic.has_value());
  EXPECT_EQ(topic->title, "Getting Started Guide");
  EXPECT_FALSE(topic->content.empty());
}

TEST_F(DocumentationTest, TopicHasCorrectCategory) {
  auto& help = HelpSystem::instance();
  auto topic = help.get_topic("doc_configuration");
  ASSERT_TRUE(topic.has_value());
  EXPECT_EQ(topic->category, HelpCategory::CONFIGURATION);
}

TEST_F(DocumentationTest, SearchFindsConfigDocs) {
  auto& help = HelpSystem::instance();
  auto results = help.search("configuration");
  ASSERT_FALSE(results.empty());
  bool found_config = false;
  for (const auto& r : results) {
    if (r.topic_id == "doc_configuration") {
      found_config = true;
      break;
    }
  }
  EXPECT_TRUE(found_config);
}

TEST_F(DocumentationTest, SearchByKeyword) {
  auto& help = HelpSystem::instance();
  auto results = help.search_by_keyword("api");
  ASSERT_FALSE(results.empty());
  bool found_api = false;
  for (const auto& r : results) {
    if (r.topic_id == "doc_api_reference") {
      found_api = true;
      break;
    }
  }
  EXPECT_TRUE(found_api);
}

TEST_F(DocumentationTest, FormatTopicProducesOutput) {
  auto& help = HelpSystem::instance();
  auto topic = help.get_topic("doc_getting_started");
  ASSERT_TRUE(topic.has_value());
  std::string formatted = help.format_topic(*topic, HelpFormat::PLAIN_TEXT);
  EXPECT_FALSE(formatted.empty());
}

TEST_F(DocumentationTest, NonexistentTopicReturnsEmpty) {
  auto& help = HelpSystem::instance();
  auto topic = help.get_topic("doc_nonexistent");
  EXPECT_FALSE(topic.has_value());
}
