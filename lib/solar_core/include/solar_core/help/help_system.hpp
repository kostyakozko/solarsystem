/**
 * @file help_system.hpp
 * @brief Comprehensive help and documentation system
 */

#ifndef SOLAR_CORE_HELP_HELP_SYSTEM_HPP
#define SOLAR_CORE_HELP_HELP_SYSTEM_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>

namespace SolarSystem::Help {

/**
 * @brief Help topic category
 */
enum class HelpCategory {
  GETTING_STARTED,
  COMMANDS,
  CONFIGURATION,
  TROUBLESHOOTING,
  EXAMPLES,
  API_REFERENCE,
  TUTORIALS
};

/**
 * @brief Help content format
 */
enum class HelpFormat {
  PLAIN_TEXT,
  MARKDOWN,
  HTML,
  INTERACTIVE
};

/**
 * @brief Help topic information
 */
struct HelpTopic {
  std::string id;
  std::string title;
  std::string content;
  HelpCategory category;
  std::vector<std::string> keywords;
  std::vector<std::string> related_topics;
  std::vector<std::string> examples;
  HelpFormat format;
};

/**
 * @brief Search result
 */
struct SearchResult {
  std::string topic_id;
  std::string title;
  std::string snippet;
  double relevance_score;
};

/**
 * @brief Tutorial step
 */
struct TutorialStep {
  std::string title;
  std::string description;
  std::string example_command;
  std::string expected_output;
  std::vector<std::string> tips;
};

/**
 * @brief Interactive tutorial
 */
struct Tutorial {
  std::string id;
  std::string title;
  std::string description;
  std::vector<TutorialStep> steps;
  std::string difficulty_level;
};

/**
 * @brief Comprehensive help system
 */
class HelpSystem {
 public:
  static HelpSystem& instance();

  // Topic management
  void register_topic(const HelpTopic& topic);
  std::optional<HelpTopic> get_topic(const std::string& topic_id) const;
  std::vector<HelpTopic> get_topics_by_category(HelpCategory category) const;
  std::vector<HelpTopic> get_all_topics() const;

  // Search functionality
  std::vector<SearchResult> search(const std::string& query) const;
  std::vector<SearchResult> search_by_keyword(const std::string& keyword) const;
  std::vector<std::string> suggest_topics(const std::string& partial_query) const;

  // Contextual help
  void set_context(const std::string& context);
  std::string get_context() const;
  std::vector<HelpTopic> get_contextual_help() const;

  // Tutorials
  void register_tutorial(const Tutorial& tutorial);
  std::optional<Tutorial> get_tutorial(const std::string& tutorial_id) const;
  std::vector<Tutorial> get_all_tutorials() const;
  std::vector<Tutorial> get_tutorials_by_difficulty(const std::string& level) const;

  // Help display
  std::string format_topic(const HelpTopic& topic, HelpFormat format) const;
  std::string format_search_results(const std::vector<SearchResult>& results) const;
  std::string format_tutorial(const Tutorial& tutorial) const;

  // Quick help
  std::string get_quick_help(const std::string& command) const;
  std::string get_usage_example(const std::string& command) const;
  std::vector<std::string> get_common_commands() const;

  // Help navigation
  std::vector<std::string> get_table_of_contents() const;
  std::string get_category_description(HelpCategory category) const;

 private:
  HelpSystem() = default;
  HelpSystem(const HelpSystem&) = delete;
  HelpSystem& operator=(const HelpSystem&) = delete;

  double calculate_relevance(const HelpTopic& topic, const std::string& query) const;
  std::vector<std::string> tokenize(const std::string& text) const;

  std::map<std::string, HelpTopic> topics_;
  std::map<std::string, Tutorial> tutorials_;
  std::string current_context_;
};

/**
 * @brief Interactive help assistant
 */
class HelpAssistant {
 public:
  static HelpAssistant& instance();

  // Interactive guidance
  void start_guided_workflow(const std::string& workflow_name);
  std::string get_next_step() const;
  bool is_workflow_complete() const;
  void complete_current_step();

  // Error assistance
  std::string suggest_solution(const std::string& error_message) const;
  std::vector<std::string> get_troubleshooting_steps(const std::string& problem) const;

  // Learning assistance
  void track_user_progress(const std::string& topic_id);
  std::vector<std::string> get_recommended_topics() const;
  double get_topic_completion_rate() const;

 private:
  HelpAssistant() = default;
  HelpAssistant(const HelpAssistant&) = delete;
  HelpAssistant& operator=(const HelpAssistant&) = delete;

  std::string current_workflow_;
  size_t current_step_;
  std::vector<std::string> completed_topics_;
};

}  // namespace SolarSystem::Help

#endif  // SOLAR_CORE_HELP_HELP_SYSTEM_HPP
