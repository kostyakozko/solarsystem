/**
 * @file help_system.cpp
 * @brief Implementation of comprehensive help system
 */

#include "solar_core/help/help_system.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace SolarSystem::Help {

HelpSystem& HelpSystem::instance() {
  static HelpSystem instance;
  return instance;
}

void HelpSystem::register_topic(const HelpTopic& topic) { topics_[topic.id] = topic; }

std::optional<HelpTopic> HelpSystem::get_topic(const std::string& topic_id) const {
  auto it = topics_.find(topic_id);
  if (it != topics_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<HelpTopic> HelpSystem::get_topics_by_category(HelpCategory category) const {
  std::vector<HelpTopic> result;
  for (const auto& [id, topic] : topics_) {
    if (topic.category == category) {
      result.push_back(topic);
    }
  }
  return result;
}

std::vector<HelpTopic> HelpSystem::get_all_topics() const {
  std::vector<HelpTopic> result;
  for (const auto& [id, topic] : topics_) {
    result.push_back(topic);
  }
  return result;
}

std::vector<std::string> HelpSystem::tokenize(const std::string& text) const {
  std::vector<std::string> tokens;
  std::string current_token;

  for (char c : text) {
    if (std::isalnum(c) || c == '_') {
      current_token += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    } else if (!current_token.empty()) {
      tokens.push_back(current_token);
      current_token.clear();
    }
  }

  if (!current_token.empty()) {
    tokens.push_back(current_token);
  }

  return tokens;
}

double HelpSystem::calculate_relevance(const HelpTopic& topic, const std::string& query) const {
  auto query_tokens = tokenize(query);
  if (query_tokens.empty()) {
    return 0.0;
  }

  double score = 0.0;

  // Check title (highest weight)
  auto title_tokens = tokenize(topic.title);
  for (const auto& qt : query_tokens) {
    for (const auto& tt : title_tokens) {
      if (tt.find(qt) != std::string::npos) {
        score += 10.0;
      }
    }
  }

  // Check keywords (high weight)
  for (const auto& keyword : topic.keywords) {
    auto keyword_tokens = tokenize(keyword);
    for (const auto& qt : query_tokens) {
      for (const auto& kt : keyword_tokens) {
        if (kt.find(qt) != std::string::npos) {
          score += 5.0;
        }
      }
    }
  }

  // Check content (lower weight)
  auto content_tokens = tokenize(topic.content);
  for (const auto& qt : query_tokens) {
    for (const auto& ct : content_tokens) {
      if (ct.find(qt) != std::string::npos) {
        score += 1.0;
      }
    }
  }

  return score;
}

std::vector<SearchResult> HelpSystem::search(const std::string& query) const {
  std::vector<SearchResult> results;

  for (const auto& [id, topic] : topics_) {
    double relevance = calculate_relevance(topic, query);
    if (relevance > 0.0) {
      SearchResult result;
      result.topic_id = id;
      result.title = topic.title;
      result.relevance_score = relevance;

      // Create snippet from content
      std::string content = topic.content;
      if (content.length() > 150) {
        content = content.substr(0, 147) + "...";
      }
      result.snippet = content;

      results.push_back(result);
    }
  }

  // Sort by relevance
  std::sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b) {
    return a.relevance_score > b.relevance_score;
  });

  // Limit to top 10 results
  if (results.size() > 10) {
    results.resize(10);
  }

  return results;
}

std::vector<SearchResult> HelpSystem::search_by_keyword(const std::string& keyword) const {
  std::vector<SearchResult> results;

  for (const auto& [id, topic] : topics_) {
    for (const auto& kw : topic.keywords) {
      if (kw.find(keyword) != std::string::npos) {
        SearchResult result;
        result.topic_id = id;
        result.title = topic.title;
        result.relevance_score = 10.0;
        result.snippet = topic.content.substr(0, std::min(size_t(150), topic.content.length()));
        results.push_back(result);
        break;
      }
    }
  }

  return results;
}

std::vector<std::string> HelpSystem::suggest_topics(const std::string& partial_query) const {
  std::vector<std::string> suggestions;

  for (const auto& [id, topic] : topics_) {
    if (topic.title.find(partial_query) != std::string::npos) {
      suggestions.push_back(topic.title);
    }
  }

  // Limit to 5 suggestions
  if (suggestions.size() > 5) {
    suggestions.resize(5);
  }

  return suggestions;
}

void HelpSystem::set_context(const std::string& context) { current_context_ = context; }

std::string HelpSystem::get_context() const { return current_context_; }

std::vector<HelpTopic> HelpSystem::get_contextual_help() const {
  if (current_context_.empty()) {
    return {};
  }

  return search_by_keyword(current_context_).size() > 0
             ? get_topics_by_category(HelpCategory::COMMANDS)
             : std::vector<HelpTopic>{};
}

void HelpSystem::register_tutorial(const Tutorial& tutorial) { tutorials_[tutorial.id] = tutorial; }

std::optional<Tutorial> HelpSystem::get_tutorial(const std::string& tutorial_id) const {
  auto it = tutorials_.find(tutorial_id);
  if (it != tutorials_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<Tutorial> HelpSystem::get_all_tutorials() const {
  std::vector<Tutorial> result;
  for (const auto& [id, tutorial] : tutorials_) {
    result.push_back(tutorial);
  }
  return result;
}

std::vector<Tutorial> HelpSystem::get_tutorials_by_difficulty(const std::string& level) const {
  std::vector<Tutorial> result;
  for (const auto& [id, tutorial] : tutorials_) {
    if (tutorial.difficulty_level == level) {
      result.push_back(tutorial);
    }
  }
  return result;
}

std::string HelpSystem::format_topic(const HelpTopic& topic, HelpFormat format) const {
  std::ostringstream oss;

  switch (format) {
    case HelpFormat::PLAIN_TEXT:
      oss << "=== " << topic.title << " ===\n\n";
      oss << topic.content << "\n\n";

      if (!topic.examples.empty()) {
        oss << "Examples:\n";
        for (const auto& example : topic.examples) {
          oss << "  " << example << "\n";
        }
        oss << "\n";
      }

      if (!topic.related_topics.empty()) {
        oss << "Related Topics: ";
        for (size_t i = 0; i < topic.related_topics.size(); ++i) {
          oss << topic.related_topics[i];
          if (i < topic.related_topics.size() - 1) {
            oss << ", ";
          }
        }
        oss << "\n";
      }
      break;

    case HelpFormat::MARKDOWN:
      oss << "# " << topic.title << "\n\n";
      oss << topic.content << "\n\n";

      if (!topic.examples.empty()) {
        oss << "## Examples\n\n";
        for (const auto& example : topic.examples) {
          oss << "```\n" << example << "\n```\n\n";
        }
      }

      if (!topic.related_topics.empty()) {
        oss << "## Related Topics\n\n";
        for (const auto& related : topic.related_topics) {
          oss << "- " << related << "\n";
        }
      }
      break;

    default:
      oss << topic.title << "\n" << topic.content;
      break;
  }

  return oss.str();
}

std::string HelpSystem::format_search_results(const std::vector<SearchResult>& results) const {
  std::ostringstream oss;

  oss << "Search Results (" << results.size() << " found):\n\n";

  for (size_t i = 0; i < results.size(); ++i) {
    oss << (i + 1) << ". " << results[i].title << "\n";
    oss << "   " << results[i].snippet << "\n";
    oss << "   Topic ID: " << results[i].topic_id << "\n\n";
  }

  return oss.str();
}

std::string HelpSystem::format_tutorial(const Tutorial& tutorial) const {
  std::ostringstream oss;

  oss << "=== Tutorial: " << tutorial.title << " ===\n";
  oss << "Difficulty: " << tutorial.difficulty_level << "\n\n";
  oss << tutorial.description << "\n\n";

  for (size_t i = 0; i < tutorial.steps.size(); ++i) {
    const auto& step = tutorial.steps[i];
    oss << "Step " << (i + 1) << ": " << step.title << "\n";
    oss << step.description << "\n\n";

    if (!step.example_command.empty()) {
      oss << "Command: " << step.example_command << "\n";
    }

    if (!step.expected_output.empty()) {
      oss << "Expected Output: " << step.expected_output << "\n";
    }

    if (!step.tips.empty()) {
      oss << "Tips:\n";
      for (const auto& tip : step.tips) {
        oss << "  - " << tip << "\n";
      }
    }

    oss << "\n";
  }

  return oss.str();
}

std::string HelpSystem::get_quick_help(const std::string& command) const {
  auto results = search(command);
  if (!results.empty()) {
    auto topic = get_topic(results[0].topic_id);
    if (topic) {
      return format_topic(*topic, HelpFormat::PLAIN_TEXT);
    }
  }
  return "No help available for: " + command;
}

std::string HelpSystem::get_usage_example(const std::string& command) const {
  auto results = search(command);
  if (!results.empty()) {
    auto topic = get_topic(results[0].topic_id);
    if (topic && !topic->examples.empty()) {
      return topic->examples[0];
    }
  }
  return "";
}

std::vector<std::string> HelpSystem::get_common_commands() const {
  std::vector<std::string> commands;

  auto command_topics = get_topics_by_category(HelpCategory::COMMANDS);
  for (const auto& topic : command_topics) {
    commands.push_back(topic.title);
  }

  return commands;
}

std::vector<std::string> HelpSystem::get_table_of_contents() const {
  std::vector<std::string> toc;

  for (const auto& [id, topic] : topics_) {
    toc.push_back(topic.title + " (" + id + ")");
  }

  std::sort(toc.begin(), toc.end());

  return toc;
}

std::string HelpSystem::get_category_description(HelpCategory category) const {
  switch (category) {
    case HelpCategory::GETTING_STARTED:
      return "Introduction and basic concepts for new users";
    case HelpCategory::COMMANDS:
      return "Command reference and usage information";
    case HelpCategory::CONFIGURATION:
      return "Configuration options and settings";
    case HelpCategory::TROUBLESHOOTING:
      return "Common problems and solutions";
    case HelpCategory::EXAMPLES:
      return "Practical examples and use cases";
    case HelpCategory::API_REFERENCE:
      return "API documentation and reference";
    case HelpCategory::TUTORIALS:
      return "Step-by-step tutorials and guides";
    default:
      return "Help topics";
  }
}

// HelpAssistant implementation

HelpAssistant& HelpAssistant::instance() {
  static HelpAssistant instance;
  return instance;
}

void HelpAssistant::start_guided_workflow(const std::string& workflow_name) {
  current_workflow_ = workflow_name;
  current_step_ = 0;
}

std::string HelpAssistant::get_next_step() const {
  auto& help = HelpSystem::instance();
  auto tutorial = help.get_tutorial(current_workflow_);

  if (tutorial && current_step_ < tutorial->steps.size()) {
    const auto& step = tutorial->steps[current_step_];
    std::ostringstream oss;
    oss << "Step " << (current_step_ + 1) << ": " << step.title << "\n";
    oss << step.description << "\n";
    if (!step.example_command.empty()) {
      oss << "Try: " << step.example_command << "\n";
    }
    return oss.str();
  }

  return "No more steps in this workflow.";
}

bool HelpAssistant::is_workflow_complete() const {
  auto& help = HelpSystem::instance();
  auto tutorial = help.get_tutorial(current_workflow_);

  return !tutorial || current_step_ >= tutorial->steps.size();
}

void HelpAssistant::complete_current_step() { ++current_step_; }

std::string HelpAssistant::suggest_solution(const std::string& error_message) const {
  auto& help = HelpSystem::instance();
  auto results = help.search(error_message);

  if (!results.empty()) {
    auto topic = help.get_topic(results[0].topic_id);
    if (topic) {
      return "Suggested solution: " + topic->content;
    }
  }

  return "No specific solution found. Try checking the troubleshooting guide.";
}

std::vector<std::string> HelpAssistant::get_troubleshooting_steps(
    const std::string& problem) const {
  std::vector<std::string> steps;

  auto& help = HelpSystem::instance();
  auto topics = help.get_topics_by_category(HelpCategory::TROUBLESHOOTING);

  for (const auto& topic : topics) {
    if (topic.content.find(problem) != std::string::npos) {
      for (const auto& example : topic.examples) {
        steps.push_back(example);
      }
    }
  }

  if (steps.empty()) {
    steps.push_back("1. Check the documentation");
    steps.push_back("2. Verify your configuration");
    steps.push_back("3. Check the logs for errors");
    steps.push_back("4. Try running with verbose output");
  }

  return steps;
}

void HelpAssistant::track_user_progress(const std::string& topic_id) {
  if (std::find(completed_topics_.begin(), completed_topics_.end(), topic_id) ==
      completed_topics_.end()) {
    completed_topics_.push_back(topic_id);
  }
}

std::vector<std::string> HelpAssistant::get_recommended_topics() const {
  auto& help = HelpSystem::instance();
  auto all_topics = help.get_all_topics();

  std::vector<std::string> recommendations;

  for (const auto& topic : all_topics) {
    if (std::find(completed_topics_.begin(), completed_topics_.end(), topic.id) ==
        completed_topics_.end()) {
      recommendations.push_back(topic.id);
      if (recommendations.size() >= 5) {
        break;
      }
    }
  }

  return recommendations;
}

double HelpAssistant::get_topic_completion_rate() const {
  auto& help = HelpSystem::instance();
  auto all_topics = help.get_all_topics();

  if (all_topics.empty()) {
    return 0.0;
  }

  return static_cast<double>(completed_topics_.size()) / static_cast<double>(all_topics.size()) *
         100.0;
}

}  // namespace SolarSystem::Help
