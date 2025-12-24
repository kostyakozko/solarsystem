/**
 * @file status_display.hpp
 * @brief Status display and feedback system for CLI applications
 *
 * Provides consistent status messages with:
 * - Color-coded severity levels
 * - Icons and visual indicators
 * - Accessibility support
 * - Structured formatting
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "solar_core/export.hpp"

namespace SolarSystem::UI {

/**
 * @brief Status message severity levels
 */
enum class StatusLevel {
  SUCCESS,    // ✓ Green - Operation successful
  INFO,       // ℹ Blue - Informational message
  WARNING,    // ⚠ Yellow - Warning condition
  ERROR,      // ✗ Red - Error condition
  DEBUG       // 🔍 Gray - Debug information
};

/**
 * @brief Status display configuration
 */
struct StatusConfig {
  bool colored_output = true;
  bool show_icons = true;
  bool accessible_mode = false;  // Screen reader friendly
  bool show_timestamp = false;
  bool compact_mode = false;
};

/**
 * @brief Status display system for consistent user feedback
 */
class SOLAR_CORE_API StatusDisplay {
 public:
  /**
   * @brief Construct status display with configuration
   */
  explicit StatusDisplay(const StatusConfig& config = {});

  /**
   * @brief Display success message
   */
  void success(const std::string& message);

  /**
   * @brief Display info message
   */
  void info(const std::string& message);

  /**
   * @brief Display warning message
   */
  void warning(const std::string& message);

  /**
   * @brief Display error message
   */
  void error(const std::string& message);

  /**
   * @brief Display debug message
   */
  void debug(const std::string& message);

  /**
   * @brief Display status with custom level
   */
  void status(StatusLevel level, const std::string& message);

  /**
   * @brief Display section header
   */
  void section(const std::string& title);

  /**
   * @brief Display key-value pair
   */
  void field(const std::string& key, const std::string& value);

  /**
   * @brief Display list of items
   */
  void list(const std::vector<std::string>& items);

  /**
   * @brief Display table header
   */
  void table_header(const std::vector<std::string>& columns);

  /**
   * @brief Display table row
   */
  void table_row(const std::vector<std::string>& values);

  /**
   * @brief Display horizontal separator
   */
  void separator();

  /**
   * @brief Display blank line
   */
  void blank_line();

 private:
  std::string get_icon(StatusLevel level) const;
  std::string get_color(StatusLevel level) const;
  std::string get_level_name(StatusLevel level) const;
  std::string format_timestamp() const;

  StatusConfig config_;
};

/**
 * @brief Formatted box display for important messages
 */
class SOLAR_CORE_API BoxDisplay {
 public:
  /**
   * @brief Display message in a box
   */
  static void display(const std::string& title, const std::vector<std::string>& lines,
                      bool colored = true);

  /**
   * @brief Display success box
   */
  static void success(const std::string& title, const std::vector<std::string>& lines);

  /**
   * @brief Display error box
   */
  static void error(const std::string& title, const std::vector<std::string>& lines);

  /**
   * @brief Display info box
   */
  static void info(const std::string& title, const std::vector<std::string>& lines);

  /**
   * @brief Display warning box
   */
  static void warning(const std::string& title, const std::vector<std::string>& lines);
};

}  // namespace SolarSystem::UI
