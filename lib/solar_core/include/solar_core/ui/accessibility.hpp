/**
 * @file accessibility.hpp
 * @brief Accessibility features for CLI applications
 *
 * Provides support for:
 * - Screen readers
 * - High contrast modes
 * - Keyboard navigation
 * - Alternative text descriptions
 */

#pragma once

#include <optional>
#include <string>

#include "solar_core/export.hpp"

namespace SolarSystem::UI {

/**
 * @brief Accessibility configuration
 */
struct AccessibilityConfig {
  bool screen_reader_mode = false;    // Optimize for screen readers
  bool high_contrast_mode = false;    // High contrast colors
  bool no_unicode = false;            // ASCII-only characters
  bool verbose_descriptions = false;  // Detailed descriptions
  bool keyboard_shortcuts = true;     // Enable keyboard shortcuts
};

/**
 * @brief Accessibility manager for CLI applications
 */
class SOLAR_CORE_API AccessibilityManager {
 public:
  /**
   * @brief Get singleton instance
   */
  static AccessibilityManager& instance();

  /**
   * @brief Configure accessibility settings
   */
  void configure(const AccessibilityConfig& config);

  /**
   * @brief Get current configuration
   */
  [[nodiscard]] const AccessibilityConfig& config() const { return config_; }

  /**
   * @brief Check if screen reader mode is enabled
   */
  [[nodiscard]] bool is_screen_reader_mode() const { return config_.screen_reader_mode; }

  /**
   * @brief Check if high contrast mode is enabled
   */
  [[nodiscard]] bool is_high_contrast_mode() const { return config_.high_contrast_mode; }

  /**
   * @brief Auto-detect accessibility needs from environment
   */
  void auto_detect();

  /**
   * @brief Get accessible alternative for Unicode character
   */
  [[nodiscard]] std::string get_accessible_char(const std::string& unicode_char,
                                                const std::string& ascii_fallback) const;

  /**
   * @brief Get accessible color code
   */
  [[nodiscard]] std::string get_accessible_color(const std::string& color_code) const;

  /**
   * @brief Format text for accessibility
   */
  [[nodiscard]] std::string format_accessible(const std::string& text,
                                              const std::string& description = "") const;

 private:
  AccessibilityManager() = default;
  AccessibilityConfig config_;
};

/**
 * @brief Keyboard shortcut manager
 */
class SOLAR_CORE_API KeyboardShortcuts {
 public:
  /**
   * @brief Display available keyboard shortcuts
   */
  static void display_shortcuts();

  /**
   * @brief Check if keyboard shortcuts are enabled
   */
  static bool are_enabled();

  /**
   * @brief Enable/disable keyboard shortcuts
   */
  static void set_enabled(bool enabled);
};

/**
 * @brief Color scheme manager for accessibility
 */
class SOLAR_CORE_API ColorScheme {
 public:
  enum class Scheme {
    DEFAULT,        // Standard colors
    HIGH_CONTRAST,  // High contrast for visibility
    MONOCHROME,     // Black and white only
    COLORBLIND      // Colorblind-friendly palette
  };

  /**
   * @brief Set color scheme
   */
  static void set_scheme(Scheme scheme);

  /**
   * @brief Get current scheme
   */
  static Scheme get_scheme();

  /**
   * @brief Get color code for semantic meaning
   */
  static std::string get_color(const std::string& semantic_name);
};

}  // namespace SolarSystem::UI
