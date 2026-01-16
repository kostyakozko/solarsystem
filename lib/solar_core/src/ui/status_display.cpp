/**
 * @file status_display.cpp
 * @brief Implementation of status display system
 */

#include "solar_core/ui/status_display.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace SolarSystem::UI {

namespace {
// ANSI color codes
constexpr const char* RESET = "\033[0m";
constexpr const char* GREEN = "\033[32m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* RED = "\033[31m";
constexpr const char* BLUE = "\033[34m";
constexpr const char* GRAY = "\033[90m";
constexpr const char* BOLD = "\033[1m";

}  // namespace

StatusDisplay::StatusDisplay(const StatusConfig& config) : config_(config) {}

void StatusDisplay::success(const std::string& message) { status(StatusLevel::SUCCESS, message); }

void StatusDisplay::info(const std::string& message) { status(StatusLevel::INFO, message); }

void StatusDisplay::warning(const std::string& message) { status(StatusLevel::WARNING, message); }

void StatusDisplay::error(const std::string& message) { status(StatusLevel::ERROR, message); }

void StatusDisplay::debug(const std::string& message) { status(StatusLevel::DEBUG, message); }

void StatusDisplay::status(StatusLevel level, const std::string& message) {
  std::ostringstream oss;

  // Timestamp
  if (config_.show_timestamp) {
    oss << format_timestamp() << " ";
  }

  if (config_.accessible_mode) {
    // Screen reader friendly format
    oss << get_level_name(level) << ": " << message;
  } else {
    // Visual format with colors and icons
    if (config_.colored_output) {
      oss << get_color(level);
    }

    if (config_.show_icons) {
      oss << get_icon(level) << " ";
    }

    if (!config_.compact_mode) {
      oss << get_level_name(level) << ": ";
    }

    oss << message;

    if (config_.colored_output) {
      oss << RESET;
    }
  }

  std::cout << oss.str() << std::endl;
}

void StatusDisplay::section(const std::string& title) {
  if (config_.accessible_mode) {
    std::cout << "\n" << title << "\n";
    return;
  }

  std::cout << "\n";
  if (config_.colored_output) {
    std::cout << BOLD << BLUE << "╭─ " << title << " ─╮" << RESET << "\n";
  } else {
    std::cout << "=== " << title << " ===" << "\n";
  }
}

void StatusDisplay::field(const std::string& key, const std::string& value) {
  if (config_.accessible_mode) {
    std::cout << key << ": " << value << std::endl;
    return;
  }

  if (config_.colored_output) {
    std::cout << BOLD << key << RESET << ": " << value << std::endl;
  } else {
    std::cout << key << ": " << value << std::endl;
  }
}

void StatusDisplay::list(const std::vector<std::string>& items) {
  for (const auto& item : items) {
    if (config_.accessible_mode) {
      std::cout << "- " << item << std::endl;
    } else if (config_.colored_output) {
      std::cout << BLUE << "  • " << RESET << item << std::endl;
    } else {
      std::cout << "  - " << item << std::endl;
    }
  }
}

void StatusDisplay::table_header(const std::vector<std::string>& columns) {
  if (config_.accessible_mode) {
    for (size_t i = 0; i < columns.size(); ++i) {
      std::cout << columns[i];
      if (i < columns.size() - 1) std::cout << " | ";
    }
    std::cout << std::endl;
    return;
  }

  if (config_.colored_output) {
    std::cout << BOLD;
  }

  for (size_t i = 0; i < columns.size(); ++i) {
    std::cout << std::setw(20) << std::left << columns[i];
  }

  if (config_.colored_output) {
    std::cout << RESET;
  }

  std::cout << std::endl;

  // Separator line
  if (!config_.accessible_mode) {
    std::cout << std::string(columns.size() * 20, '-') << std::endl;
  }
}

void StatusDisplay::table_row(const std::vector<std::string>& values) {
  if (config_.accessible_mode) {
    for (size_t i = 0; i < values.size(); ++i) {
      std::cout << values[i];
      if (i < values.size() - 1) std::cout << " | ";
    }
    std::cout << std::endl;
    return;
  }

  for (const auto& value : values) {
    std::cout << std::setw(20) << std::left << value;
  }
  std::cout << std::endl;
}

void StatusDisplay::separator() {
  if (config_.accessible_mode) {
    std::cout << std::endl;
  } else {
    std::cout << std::string(60, '-') << std::endl;
  }
}

void StatusDisplay::blank_line() { std::cout << std::endl; }

std::string StatusDisplay::get_icon(StatusLevel level) const {
  switch (level) {
    case StatusLevel::SUCCESS:
      return "✓";
    case StatusLevel::INFO:
      return "ℹ";
    case StatusLevel::WARNING:
      return "⚠";
    case StatusLevel::ERROR:
      return "✗";
    case StatusLevel::DEBUG:
      return "🔍";
    default:
      return "•";
  }
}

std::string StatusDisplay::get_color(StatusLevel level) const {
  switch (level) {
    case StatusLevel::SUCCESS:
      return GREEN;
    case StatusLevel::INFO:
      return BLUE;
    case StatusLevel::WARNING:
      return YELLOW;
    case StatusLevel::ERROR:
      return RED;
    case StatusLevel::DEBUG:
      return GRAY;
    default:
      return RESET;
  }
}

std::string StatusDisplay::get_level_name(StatusLevel level) const {
  switch (level) {
    case StatusLevel::SUCCESS:
      return "Success";
    case StatusLevel::INFO:
      return "Info";
    case StatusLevel::WARNING:
      return "Warning";
    case StatusLevel::ERROR:
      return "Error";
    case StatusLevel::DEBUG:
      return "Debug";
    default:
      return "Status";
  }
}

std::string StatusDisplay::format_timestamp() const {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
  return oss.str();
}

// BoxDisplay implementation

void BoxDisplay::display(const std::string& title, const std::vector<std::string>& lines,
                         bool colored) {
  // Calculate box width
  size_t max_width = title.length();
  for (const auto& line : lines) {
    max_width = std::max(max_width, line.length());
  }
  max_width += 4;  // Padding

  // Top border
  std::cout << "╭─" << std::string(max_width, '-') << "─╮" << std::endl;

  // Title
  if (colored) {
    std::cout << "│ " << BOLD << std::setw(static_cast<int>(max_width)) << std::left << title
              << RESET << " │" << std::endl;
  } else {
    std::cout << "│ " << std::setw(static_cast<int>(max_width)) << std::left << title << " │"
              << std::endl;
  }

  // Separator
  std::cout << "├─" << std::string(max_width, '-') << "─┤" << std::endl;

  // Content lines
  for (const auto& line : lines) {
    std::cout << "│ " << std::setw(static_cast<int>(max_width)) << std::left << line << " │"
              << std::endl;
  }

  // Bottom border
  std::cout << "╰─" << std::string(max_width, '-') << "─╯" << std::endl;
}

void BoxDisplay::success(const std::string& title, const std::vector<std::string>& lines) {
  std::cout << GREEN;
  display(title, lines, true);
  std::cout << RESET;
}

void BoxDisplay::error(const std::string& title, const std::vector<std::string>& lines) {
  std::cout << RED;
  display(title, lines, true);
  std::cout << RESET;
}

void BoxDisplay::info(const std::string& title, const std::vector<std::string>& lines) {
  std::cout << BLUE;
  display(title, lines, true);
  std::cout << RESET;
}

}  // namespace SolarSystem::UI
