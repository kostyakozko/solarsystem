/**
 * @file test_help_system.cpp
 * @brief Unit tests for comprehensive help system
 */

#include "../utils/test_framework.h"
#include "solar_core/help/help_system.hpp"

using namespace SolarSystem::Help;

int main() {
  TestSuite suite("Help System Tests");

  // Test 1: Register and retrieve topic
  suite.run_test("Register and Retrieve Topic", []() {
    auto& help = HelpSystem::instance();

    HelpTopic topic;
    topic.id = "test_topic";
    topic.title = "Test Topic";
    topic.content = "This is test content";
    topic.category = HelpCategory::GETTING_STARTED;
    topic.keywords = {"test", "example"};
    topic.format = HelpFormat::PLAIN_TEXT;

    help.register_topic(topic);

    auto retrieved = help.get_topic("test_topic");
    if (!retrieved) throw std::runtime_error("Topic not found");
    if (retrieved->title != "Test Topic") throw std::runtime_error("Title mismatch");
  });

  // Test 2: Search functionality
  suite.run_test("Search Functionality", []() {
    auto& help = HelpSystem::instance();

    HelpTopic topic;
    topic.id = "search_test";
    topic.title = "Search Test Topic";
    topic.content = "Content about searching and finding information";
    topic.category = HelpCategory::COMMANDS;
    topic.keywords = {"search", "find", "query"};
    topic.format = HelpFormat::PLAIN_TEXT;

    help.register_topic(topic);

    auto results = help.search("search");
    if (results.empty()) throw std::runtime_error("No search results found");
    if (results[0].relevance_score <= 0.0) throw std::runtime_error("Invalid relevance score");
  });

  // Test 3: Get topics by category
  suite.run_test("Get Topics by Category", []() {
    auto& help = HelpSystem::instance();

    auto topics = help.get_topics_by_category(HelpCategory::GETTING_STARTED);
    if (topics.empty()) throw std::runtime_error("No topics found in category");
  });

  // Test 4: Tutorial registration
  suite.run_test("Tutorial Registration", []() {
    auto& help = HelpSystem::instance();

    Tutorial tutorial;
    tutorial.id = "test_tutorial";
    tutorial.title = "Test Tutorial";
    tutorial.description = "A test tutorial";
    tutorial.difficulty_level = "beginner";

    TutorialStep step;
    step.title = "Step 1";
    step.description = "First step";
    step.example_command = "test command";
    tutorial.steps.push_back(step);

    help.register_tutorial(tutorial);

    auto retrieved = help.get_tutorial("test_tutorial");
    if (!retrieved) throw std::runtime_error("Tutorial not found");
    if (retrieved->steps.size() != 1) throw std::runtime_error("Step count mismatch");
  });

  // Test 5: Format topic
  suite.run_test("Format Topic", []() {
    auto& help = HelpSystem::instance();

    HelpTopic topic;
    topic.id = "format_test";
    topic.title = "Format Test";
    topic.content = "Test content";
    topic.category = HelpCategory::EXAMPLES;
    topic.examples = {"example 1", "example 2"};
    topic.format = HelpFormat::PLAIN_TEXT;

    help.register_topic(topic);

    std::string formatted = help.format_topic(topic, HelpFormat::PLAIN_TEXT);
    if (formatted.empty()) throw std::runtime_error("Formatted output is empty");
    if (formatted.find("Format Test") == std::string::npos) throw std::runtime_error("Title not in output");
  });

  // Test 6: Quick help
  suite.run_test("Quick Help", []() {
    auto& help = HelpSystem::instance();

    HelpTopic topic;
    topic.id = "quick_help_test";
    topic.title = "Quick Help Command";
    topic.content = "Quick help content";
    topic.category = HelpCategory::COMMANDS;
    topic.keywords = {"quick", "help"};
    topic.format = HelpFormat::PLAIN_TEXT;

    help.register_topic(topic);

    std::string quick_help = help.get_quick_help("quick");
    if (quick_help.empty()) throw std::runtime_error("Quick help is empty");
  });

  // Test 7: Help assistant workflow
  suite.run_test("Help Assistant Workflow", []() {
    auto& help = HelpSystem::instance();
    auto& assistant = HelpAssistant::instance();

    Tutorial tutorial;
    tutorial.id = "workflow_test";
    tutorial.title = "Workflow Test";
    tutorial.description = "Test workflow";
    tutorial.difficulty_level = "beginner";

    TutorialStep step1;
    step1.title = "Step 1";
    step1.description = "First step";
    tutorial.steps.push_back(step1);

    TutorialStep step2;
    step2.title = "Step 2";
    step2.description = "Second step";
    tutorial.steps.push_back(step2);

    help.register_tutorial(tutorial);

    assistant.start_guided_workflow("workflow_test");
    if (assistant.is_workflow_complete()) throw std::runtime_error("Workflow should not be complete");

    std::string next_step = assistant.get_next_step();
    if (next_step.empty()) throw std::runtime_error("Next step is empty");

    assistant.complete_current_step();
    assistant.complete_current_step();

    if (!assistant.is_workflow_complete()) throw std::runtime_error("Workflow should be complete");
  });

  // Test 8: Search by keyword
  suite.run_test("Search by Keyword", []() {
    auto& help = HelpSystem::instance();

    HelpTopic topic;
    topic.id = "keyword_test";
    topic.title = "Keyword Test";
    topic.content = "Content for keyword testing";
    topic.category = HelpCategory::COMMANDS;
    topic.keywords = {"special_keyword", "test"};
    topic.format = HelpFormat::PLAIN_TEXT;

    help.register_topic(topic);

    auto results = help.search_by_keyword("special_keyword");
    if (results.empty()) throw std::runtime_error("No results for keyword search");
  });

  // Test 9: Topic suggestions
  suite.run_test("Topic Suggestions", []() {
    auto& help = HelpSystem::instance();

    HelpTopic topic;
    topic.id = "suggestion_test";
    topic.title = "Suggestion Test Topic";
    topic.content = "Content for suggestions";
    topic.category = HelpCategory::COMMANDS;
    topic.format = HelpFormat::PLAIN_TEXT;

    help.register_topic(topic);

    auto suggestions = help.suggest_topics("Suggestion");
    if (suggestions.empty()) throw std::runtime_error("No suggestions found");
  });

  // Test 10: Troubleshooting assistance
  suite.run_test("Troubleshooting Assistance", []() {
    auto& assistant = HelpAssistant::instance();

    auto steps = assistant.get_troubleshooting_steps("test problem");
    if (steps.empty()) throw std::runtime_error("No troubleshooting steps");
  });

  suite.print_summary();
  return suite.get_failed_count() > 0 ? 1 : 0;
}
