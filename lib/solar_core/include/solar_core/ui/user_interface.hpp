/**
 * @file user_interface.hpp
 * @brief User-friendly interface components
 */

#ifndef SOLAR_CORE_UI_USER_INTERFACE_HPP
#define SOLAR_CORE_UI_USER_INTERFACE_HPP

#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/ui/status_display.hpp"

namespace SolarSystem::UI {

// StatusLevel is defined in status_display.hpp

/**
 * @brief Status feedback message
 */
struct StatusMessage {
  StatusLevel level;
  std::string message;
  std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Status feedback system
 */
class StatusFeedback {
 public:
  static StatusFeedback& instance();

  void info(const std::string& message);
  void success(const std::string& message);
  void warning(const std::string& message);
  void error(const std::string& message);

  void clear();
  std::vector<StatusMessage> get_messages() const;
  std::string format_message(const StatusMessage& msg) const;

 private:
  StatusFeedback() = default;
  StatusFeedback(const StatusFeedback&) = delete;
  StatusFeedback& operator=(const StatusFeedback&) = delete;

  std::vector<StatusMessage> messages_;
};

/**
 * @brief Input prompt configuration
 */
struct InputPrompt {
  std::string prompt_text;
  std::string default_value;
  std::vector<std::string> examples;
  std::function<bool(const std::string&)> validator;
  std::string validation_message;
  bool required;
};

/**
 * @brief Interactive input system
 */
class InteractiveInput {
 public:
  static InteractiveInput& instance();

  std::string prompt(const InputPrompt& config);
  bool confirm(const std::string& question, bool default_yes = true);
  std::string select(const std::string& prompt, const std::vector<std::string>& options);

  void set_interactive_mode(bool enabled);
  bool is_interactive() const;

 private:
  InteractiveInput() = default;
  InteractiveInput(const InteractiveInput&) = delete;
  InteractiveInput& operator=(const InteractiveInput&) = delete;

  bool interactive_mode_{true};
};

/**
 * @brief Accessibility features
 */
class AccessibilitySupport {
 public:
  static AccessibilitySupport& instance();

  // Screen reader support
  void enable_screen_reader_mode(bool enabled);
  bool is_screen_reader_enabled() const;
  std::string format_for_screen_reader(const std::string& text) const;

  // High contrast mode
  void enable_high_contrast(bool enabled);
  bool is_high_contrast_enabled() const;

  // Text size
  void set_text_scale(double scale);
  double get_text_scale() const;

  // Keyboard navigation
  void enable_keyboard_shortcuts(bool enabled);
  bool are_keyboard_shortcuts_enabled() const;

  // Alternative text
  void set_alt_text(const std::string& element_id, const std::string& alt_text);
  std::string get_alt_text(const std::string& element_id) const;

 private:
  AccessibilitySupport() = default;
  AccessibilitySupport(const AccessibilitySupport&) = delete;
  AccessibilitySupport& operator=(const AccessibilitySupport&) = delete;

  bool screen_reader_mode_{false};
  bool high_contrast_{false};
  double text_scale_{1.0};
  bool keyboard_shortcuts_{true};
  std::map<std::string, std::string> alt_texts_;
};

/**
 * @brief Consistent formatting utilities
 */
class FormattingUtils {
 public:
  static std::string format_table(const std::vector<std::vector<std::string>>& data,
                                  const std::vector<std::string>& headers);
  static std::string format_list(const std::vector<std::string>& items, bool numbered = false);
  static std::string format_key_value(const std::string& key, const std::string& value);
  static std::string format_section(const std::string& title, const std::string& content);
  static std::string format_box(const std::string& content, const std::string& title = "");

  static std::string colorize(const std::string& text, const std::string& color);
  static std::string bold(const std::string& text);
  static std::string italic(const std::string& text);
  static std::string underline(const std::string& text);
};

/**
 * @brief Feedback entry from a user
 */
struct FeedbackEntry {
  int rating;
  std::string comment;
  std::string feature_area;
  std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Collects, stores, and persists user feedback
 */
class UserFeedbackCollector {
 public:
  void submit_feedback(int rating, const std::string& comment, const std::string& area);
  size_t get_feedback_count() const;
  const std::vector<FeedbackEntry>& get_entries() const;

  bool save_to_file(const std::string& path) const;
  bool load_from_file(const std::string& path);

 private:
  std::vector<FeedbackEntry> entries_;
};

}  // namespace SolarSystem::UI

#endif  // SOLAR_CORE_UI_USER_INTERFACE_HPP
