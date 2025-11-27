/**
 * @file user_interface.cpp
 * @brief Implementation of user-friendly interface components
 */

#include "solar_core/ui/user_interface.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace SolarSystem::UI {

// ProgressIndicator implementation

ProgressIndicator::ProgressIndicator(const std::string& task_name, size_t total_steps)
    : task_name_(task_name),
      total_steps_(total_steps),
      current_step_(0),
      style_(ProgressStyle::BAR),
      completed_(false),
      failed_(false),
      start_time_(std::chrono::steady_clock::now()) {}

void ProgressIndicator::update(size_t current_step) {
  current_step_ = current_step;
  status_message_.clear();
}

void ProgressIndicator::update(size_t current_step, const std::string& status_message) {
  current_step_ = current_step;
  status_message_ = status_message;
}

void ProgressIndicator::complete() {
  completed_ = true;
  current_step_ = total_steps_;
}

void ProgressIndicator::fail(const std::string& error_message) {
  failed_ = true;
  status_message_ = error_message;
}

void ProgressIndicator::set_style(ProgressStyle style) {
  style_ = style;
}

std::string ProgressIndicator::render() const {
  std::ostringstream oss;

  switch (style_) {
    case ProgressStyle::BAR: {
      double percentage = get_percentage();
      int bar_width = 40;
      int filled = static_cast<int>(bar_width * percentage / 100.0);

      oss << task_name_ << ": [";
      for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
          oss << "=";
        } else if (i == filled) {
          oss << ">";
        } else {
          oss << " ";
        }
      }
      oss << "] " << static_cast<int>(percentage) << "%";

      if (!status_message_.empty()) {
        oss << " - " << status_message_;
      }
      break;
    }

    case ProgressStyle::SPINNER: {
      const char* spinner_chars = "|/-\\";
      int spinner_index = static_cast<int>(current_step_ % 4);
      oss << task_name_ << ": " << spinner_chars[spinner_index];
      if (!status_message_.empty()) {
        oss << " " << status_message_;
      }
      break;
    }

    case ProgressStyle::PERCENTAGE: {
      oss << task_name_ << ": " << static_cast<int>(get_percentage()) << "% complete";
      break;
    }

    case ProgressStyle::DOTS: {
      oss << task_name_ << ": ";
      for (size_t i = 0; i < current_step_ && i < 10; ++i) {
        oss << ".";
      }
      break;
    }

    case ProgressStyle::MINIMAL: {
      oss << task_name_ << ": " << current_step_ << "/" << total_steps_;
      break;
    }
  }

  if (completed_) {
    oss << " ✓ Complete";
  } else if (failed_) {
    oss << " ✗ Failed";
  }

  return oss.str();
}

double ProgressIndicator::get_percentage() const {
  if (total_steps_ == 0) return 0.0;
  return (static_cast<double>(current_step_) / static_cast<double>(total_steps_)) * 100.0;
}

bool ProgressIndicator::is_complete() const {
  return completed_;
}

// StatusFeedback implementation

StatusFeedback& StatusFeedback::instance() {
  static StatusFeedback instance;
  return instance;
}

void StatusFeedback::info(const std::string& message) {
  StatusMessage msg;
  msg.level = StatusLevel::INFO;
  msg.message = message;
  msg.timestamp = std::chrono::system_clock::now();
  messages_.push_back(msg);
}

void StatusFeedback::success(const std::string& message) {
  StatusMessage msg;
  msg.level = StatusLevel::SUCCESS;
  msg.message = message;
  msg.timestamp = std::chrono::system_clock::now();
  messages_.push_back(msg);
}

void StatusFeedback::warning(const std::string& message) {
  StatusMessage msg;
  msg.level = StatusLevel::WARNING;
  msg.message = message;
  msg.timestamp = std::chrono::system_clock::now();
  messages_.push_back(msg);
}

void StatusFeedback::error(const std::string& message) {
  StatusMessage msg;
  msg.level = StatusLevel::ERROR;
  msg.message = message;
  msg.timestamp = std::chrono::system_clock::now();
  messages_.push_back(msg);
}

void StatusFeedback::clear() {
  messages_.clear();
}

std::vector<StatusMessage> StatusFeedback::get_messages() const {
  return messages_;
}

std::string StatusFeedback::format_message(const StatusMessage& msg) const {
  std::ostringstream oss;

  switch (msg.level) {
    case StatusLevel::INFO:
      oss << "[INFO] ";
      break;
    case StatusLevel::SUCCESS:
      oss << "[SUCCESS] ✓ ";
      break;
    case StatusLevel::WARNING:
      oss << "[WARNING] ⚠ ";
      break;
    case StatusLevel::ERROR:
      oss << "[ERROR] ✗ ";
      break;
  }

  oss << msg.message;

  return oss.str();
}

// InteractiveInput implementation

InteractiveInput& InteractiveInput::instance() {
  static InteractiveInput instance;
  return instance;
}

std::string InteractiveInput::prompt(const InputPrompt& config) {
  if (!interactive_mode_) {
    return config.default_value;
  }

  // Display prompt
  std::cout << config.prompt_text;

  // Show default value if provided
  if (!config.default_value.empty()) {
    std::cout << " [" << config.default_value << "]";
  }

  std::cout << ": ";
  std::cout.flush();

  // Read input from stdin
  std::string input;
  if (!std::getline(std::cin, input)) {
    // EOF or error - return default
    return config.default_value;
  }

  // Trim whitespace
  input.erase(0, input.find_first_not_of(" \t\n\r"));
  input.erase(input.find_last_not_of(" \t\n\r") + 1);

  // If empty input, use default
  if (input.empty()) {
    return config.default_value;
  }

  // Validate input if validator provided
  if (config.validator && !config.validator(input)) {
    std::cout << "Invalid input. ";
    if (!config.validation_message.empty()) {
      std::cout << config.validation_message;
    }
    std::cout << std::endl;

    // Retry
    return prompt(config);
  }

  return input;
}

bool InteractiveInput::confirm(const std::string& question, bool default_yes) {
  if (!interactive_mode_) {
    return default_yes;
  }

  // Display question with default
  std::cout << question << " [" << (default_yes ? "Y/n" : "y/N") << "]: ";
  std::cout.flush();

  // Read input
  std::string input;
  if (!std::getline(std::cin, input)) {
    // EOF or error - return default
    return default_yes;
  }

  // Trim and convert to lowercase
  input.erase(0, input.find_first_not_of(" \t\n\r"));
  input.erase(input.find_last_not_of(" \t\n\r") + 1);
  std::transform(input.begin(), input.end(), input.begin(), ::tolower);

  // Empty input uses default
  if (input.empty()) {
    return default_yes;
  }

  // Check for yes/no
  if (input == "y" || input == "yes") {
    return true;
  } else if (input == "n" || input == "no") {
    return false;
  }

  // Invalid input, use default
  return default_yes;
}

std::string InteractiveInput::select(const std::string& prompt_text,
                                    const std::vector<std::string>& options) {
  if (!interactive_mode_ || options.empty()) {
    return options.empty() ? "" : options[0];
  }

  // Display prompt and options
  std::cout << prompt_text << std::endl;
  for (size_t i = 0; i < options.size(); ++i) {
    std::cout << "  " << (i + 1) << ". " << options[i] << std::endl;
  }
  std::cout << "Select (1-" << options.size() << "): ";
  std::cout.flush();

  // Read selection
  std::string input;
  if (!std::getline(std::cin, input)) {
    // EOF or error - return first option
    return options[0];
  }

  // Trim whitespace
  input.erase(0, input.find_first_not_of(" \t\n\r"));
  input.erase(input.find_last_not_of(" \t\n\r") + 1);

  // Try to parse as number
  try {
    size_t selection = std::stoull(input);
    if (selection >= 1 && selection <= options.size()) {
      return options[selection - 1];
    }
  } catch (...) {
    // Not a valid number, check if it matches an option text
    for (const auto& option : options) {
      if (input == option) {
        return option;
      }
    }
  }

  // Invalid selection, return first option
  std::cout << "Invalid selection. Using default." << std::endl;
  return options[0];
}

void InteractiveInput::set_interactive_mode(bool enabled) {
  interactive_mode_ = enabled;
}

bool InteractiveInput::is_interactive() const {
  return interactive_mode_;
}

// AccessibilitySupport implementation

AccessibilitySupport& AccessibilitySupport::instance() {
  static AccessibilitySupport instance;
  return instance;
}

void AccessibilitySupport::enable_screen_reader_mode(bool enabled) {
  screen_reader_mode_ = enabled;
}

bool AccessibilitySupport::is_screen_reader_enabled() const {
  return screen_reader_mode_;
}

std::string AccessibilitySupport::format_for_screen_reader(const std::string& text) const {
  if (!screen_reader_mode_) {
    return text;
  }

  // Remove special characters and formatting for screen readers
  std::string result = text;

  // Replace common symbols with words
  size_t pos = 0;
  while ((pos = result.find("✓", pos)) != std::string::npos) {
    result.replace(pos, 3, "checkmark");  // UTF-8 checkmark is 3 bytes
    pos += 9;
  }

  pos = 0;
  while ((pos = result.find("✗", pos)) != std::string::npos) {
    result.replace(pos, 3, "cross");
    pos += 5;
  }

  pos = 0;
  while ((pos = result.find("⚠", pos)) != std::string::npos) {
    result.replace(pos, 3, "warning");
    pos += 7;
  }

  return result;
}

void AccessibilitySupport::enable_high_contrast(bool enabled) {
  high_contrast_ = enabled;
}

bool AccessibilitySupport::is_high_contrast_enabled() const {
  return high_contrast_;
}

void AccessibilitySupport::set_text_scale(double scale) {
  text_scale_ = std::max(0.5, std::min(3.0, scale));
}

double AccessibilitySupport::get_text_scale() const {
  return text_scale_;
}

void AccessibilitySupport::enable_keyboard_shortcuts(bool enabled) {
  keyboard_shortcuts_ = enabled;
}

bool AccessibilitySupport::are_keyboard_shortcuts_enabled() const {
  return keyboard_shortcuts_;
}

void AccessibilitySupport::set_alt_text(const std::string& element_id, const std::string& alt_text) {
  alt_texts_[element_id] = alt_text;
}

std::string AccessibilitySupport::get_alt_text(const std::string& element_id) const {
  auto it = alt_texts_.find(element_id);
  if (it != alt_texts_.end()) {
    return it->second;
  }
  return "";
}

// FormattingUtils implementation

std::string FormattingUtils::format_table(const std::vector<std::vector<std::string>>& data,
                                         const std::vector<std::string>& headers) {
  if (data.empty()) {
    return "";
  }

  // Calculate column widths
  std::vector<size_t> widths(headers.size(), 0);
  for (size_t i = 0; i < headers.size(); ++i) {
    widths[i] = headers[i].length();
  }

  for (const auto& row : data) {
    for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
      widths[i] = std::max(widths[i], row[i].length());
    }
  }

  std::ostringstream oss;

  // Header
  for (size_t i = 0; i < headers.size(); ++i) {
    oss << std::left << std::setw(static_cast<int>(widths[i] + 2)) << headers[i];
  }
  oss << "\n";

  // Separator
  for (size_t i = 0; i < headers.size(); ++i) {
    oss << std::string(widths[i] + 2, '-');
  }
  oss << "\n";

  // Data rows
  for (const auto& row : data) {
    for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
      oss << std::left << std::setw(static_cast<int>(widths[i] + 2)) << row[i];
    }
    oss << "\n";
  }

  return oss.str();
}

std::string FormattingUtils::format_list(const std::vector<std::string>& items, bool numbered) {
  std::ostringstream oss;

  for (size_t i = 0; i < items.size(); ++i) {
    if (numbered) {
      oss << (i + 1) << ". " << items[i] << "\n";
    } else {
      oss << "• " << items[i] << "\n";
    }
  }

  return oss.str();
}

std::string FormattingUtils::format_key_value(const std::string& key, const std::string& value) {
  std::ostringstream oss;
  oss << std::left << std::setw(20) << key << ": " << value;
  return oss.str();
}

std::string FormattingUtils::format_section(const std::string& title, const std::string& content) {
  std::ostringstream oss;
  oss << "\n=== " << title << " ===\n\n";
  oss << content << "\n";
  return oss.str();
}

std::string FormattingUtils::format_box(const std::string& content, const std::string& title) {
  std::ostringstream oss;

  size_t width = 60;

  // Top border
  oss << "┌" << std::string(width - 2, '-') << "┐\n";

  // Title if provided
  if (!title.empty()) {
    oss << "│ " << std::left << std::setw(static_cast<int>(width - 4)) << title << " │\n";
    oss << "├" << std::string(width - 2, '-') << "┤\n";
  }

  // Content
  oss << "│ " << std::left << std::setw(static_cast<int>(width - 4)) << content << " │\n";

  // Bottom border
  oss << "└" << std::string(width - 2, '-') << "┘\n";

  return oss.str();
}

std::string FormattingUtils::colorize(const std::string& text, const std::string& color) {
  // ANSI color codes
  std::string color_code;
  if (color == "red") color_code = "\033[31m";
  else if (color == "green") color_code = "\033[32m";
  else if (color == "yellow") color_code = "\033[33m";
  else if (color == "blue") color_code = "\033[34m";
  else if (color == "magenta") color_code = "\033[35m";
  else if (color == "cyan") color_code = "\033[36m";
  else return text;

  return color_code + text + "\033[0m";
}

std::string FormattingUtils::bold(const std::string& text) {
  return "\033[1m" + text + "\033[0m";
}

std::string FormattingUtils::italic(const std::string& text) {
  return "\033[3m" + text + "\033[0m";
}

std::string FormattingUtils::underline(const std::string& text) {
  return "\033[4m" + text + "\033[0m";
}

}  // namespace SolarSystem::UI
