/**
 * @file accessibility.cpp
 * @brief Implementation of accessibility features
 */

#include "solar_core/ui/accessibility.hpp"

#include <cstdlib>
#include <iostream>
#include <map>

namespace SolarSystem::UI {

namespace {
// Global color scheme
ColorScheme::Scheme g_color_scheme = ColorScheme::Scheme::DEFAULT;
bool g_shortcuts_enabled = true;

// Color mappings for different schemes
const std::map<std::string, std::string> DEFAULT_COLORS = {{"success", "\033[32m"},  // Green
                                                           {"error", "\033[31m"},    // Red
                                                           {"warning", "\033[33m"},  // Yellow
                                                           {"info", "\033[34m"},     // Blue
                                                           {"debug", "\033[90m"},    // Gray
                                                           {"reset", "\033[0m"}};

const std::map<std::string, std::string> HIGH_CONTRAST_COLORS = {
    {"success", "\033[1;32m"},  // Bold Green
    {"error", "\033[1;31m"},    // Bold Red
    {"warning", "\033[1;33m"},  // Bold Yellow
    {"info", "\033[1;36m"},     // Bold Cyan
    {"debug", "\033[1;37m"},    // Bold White
    {"reset", "\033[0m"}};

const std::map<std::string, std::string> MONOCHROME_COLORS = {{"success", "\033[1m"},  // Bold
                                                              {"error", "\033[1m"},    // Bold
                                                              {"warning", "\033[1m"},  // Bold
                                                              {"info", ""},            // Normal
                                                              {"debug", "\033[2m"},    // Dim
                                                              {"reset", "\033[0m"}};

}  // namespace

// AccessibilityManager implementation

AccessibilityManager& AccessibilityManager::instance() {
  static AccessibilityManager instance;
  return instance;
}

void AccessibilityManager::configure(const AccessibilityConfig& config) { config_ = config; }

void AccessibilityManager::auto_detect() {
  // Check environment variables for accessibility hints
  const char* term = std::getenv("TERM");
  const char* colorterm = std::getenv("COLORTERM");
  const char* screen_reader = std::getenv("SCREEN_READER");

  // Detect screen reader
  if (screen_reader && std::string(screen_reader) == "1") {
    config_.screen_reader_mode = true;
    config_.no_unicode = true;
    config_.verbose_descriptions = true;
  }

  // Detect limited terminal capabilities
  if (term && (std::string(term) == "dumb" || std::string(term) == "unknown")) {
    config_.no_unicode = true;
    config_.high_contrast_mode = false;
  }

  // Detect color support
  if (!colorterm || std::string(colorterm).empty()) {
    config_.high_contrast_mode = true;
  }
}

std::string AccessibilityManager::get_accessible_char(const std::string& unicode_char,
                                                      const std::string& ascii_fallback) const {
  if (config_.no_unicode || config_.screen_reader_mode) {
    return ascii_fallback;
  }
  return unicode_char;
}

std::string AccessibilityManager::get_accessible_color(const std::string& color_code) const {
  if (config_.screen_reader_mode || config_.high_contrast_mode) {
    return "";  // No colors in screen reader mode
  }
  return color_code;
}

std::string AccessibilityManager::format_accessible(const std::string& text,
                                                    const std::string& description) const {
  if (config_.screen_reader_mode && !description.empty()) {
    return description + ": " + text;
  }
  return text;
}

// KeyboardShortcuts implementation

void KeyboardShortcuts::display_shortcuts() {
  std::cout << "\n╭─────────────────────────────────────────╮\n";
  std::cout << "│      Keyboard Shortcuts                  │\n";
  std::cout << "╰─────────────────────────────────────────╯\n\n";

  std::cout << "General:\n";
  std::cout << "  Ctrl+C    - Cancel current operation\n";
  std::cout << "  Ctrl+D    - Exit application\n";
  std::cout << "  h or ?    - Show help\n";
  std::cout << "  q         - Quit\n\n";

  std::cout << "Navigation:\n";
  std::cout << "  ↑/↓       - Navigate options\n";
  std::cout << "  Enter     - Select/Confirm\n";
  std::cout << "  Esc       - Cancel/Back\n";
  std::cout << "  Tab       - Next field\n\n";

  std::cout << "Accessibility:\n";
  std::cout << "  Ctrl+A    - Toggle accessibility mode\n";
  std::cout << "  Ctrl+H    - Toggle high contrast\n";
  std::cout << "  Ctrl+V    - Toggle verbose descriptions\n\n";
}

bool KeyboardShortcuts::are_enabled() { return g_shortcuts_enabled; }

void KeyboardShortcuts::set_enabled(bool enabled) { g_shortcuts_enabled = enabled; }

// ColorScheme implementation

void ColorScheme::set_scheme(Scheme scheme) { g_color_scheme = scheme; }

ColorScheme::Scheme ColorScheme::get_scheme() { return g_color_scheme; }

std::string ColorScheme::get_color(const std::string& semantic_name) {
  switch (g_color_scheme) {
    case Scheme::HIGH_CONTRAST: {
      auto it = HIGH_CONTRAST_COLORS.find(semantic_name);
      return (it != HIGH_CONTRAST_COLORS.end()) ? it->second : "";
    }
    case Scheme::MONOCHROME: {
      auto it = MONOCHROME_COLORS.find(semantic_name);
      return (it != MONOCHROME_COLORS.end()) ? it->second : "";
    }
    case Scheme::COLORBLIND:
      // Use high contrast for colorblind mode (can be customized further)
      {
        auto it = HIGH_CONTRAST_COLORS.find(semantic_name);
        return (it != HIGH_CONTRAST_COLORS.end()) ? it->second : "";
      }
    case Scheme::DEFAULT:
    default: {
      auto it = DEFAULT_COLORS.find(semantic_name);
      return (it != DEFAULT_COLORS.end()) ? it->second : "";
    }
  }
}

}  // namespace SolarSystem::UI
