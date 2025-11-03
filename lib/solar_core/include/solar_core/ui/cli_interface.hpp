/**
 * @file cli_interface.hpp
 * @brief Unified CLI interface manager for consistent UX
 *
 * Provides a high-level interface that combines:
 * - Progress indication
 * - Status display
 * - Accessibility features
 * - Consistent formatting
 */

#pragma once

#include "solar_core/ui/accessibility.hpp"
#include "solar_core/ui/progress_indicator.hpp"
#include "solar_core/ui/status_display.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace SolarSystem::UI {

/**
 * @brief CLI interface configuration
 */
struct CLIConfig {
  bool colored_output = true;
  bool show_progress = true;
  bool accessible_mode = false;
  bool verbose = false;
  bool quiet = false;
  ProgressStyle progress_style = ProgressStyle::BAR;
};

/**
 * @brief Unified CLI interface manager
 */
class CLIInterface {
 public:
  /**
   * @brief Construct CLI interface with configuration
   */
  explicit CLIInterface(const CLIConfig& config = {});

  /**
   * @brief Initialize interface (auto-detect accessibility, etc.)
   */
  void initialize();

  /**
   * @brief Display application banner
   */
  void show_banner(const std::string& app_name, const std::string& version,
                   const std::string& description = "");

  /**
   * @brief Display help information
   */
  void show_help(const std::string& usage, const std::vector<std::string>& options,
                 const std::vector<std::string>& examples = {});

  /**
   * @brief Run operation with progress indication
   */
  template <typename Func>
  bool run_with_progress(const std::string& task_name, Func&& operation);

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
   * @brief Display debug message (only if verbose)
   */
  void debug(const std::string& message);

  /**
   * @brief Display section header
   */
  void section(const std::string& title);

  /**
   * @brief Display key-value field
   */
  void field(const std::string& key, const std::string& value);

  /**
   * @brief Display list of items
   */
  void list(const std::vector<std::string>& items);

  /**
   * @brief Display table
   */
  void table(const std::vector<std::string>& headers,
             const std::vector<std::vector<std::string>>& rows);

  /**
   * @brief Prompt user for confirmation
   */
  [[nodiscard]] bool confirm(const std::string& message, bool default_yes = false);

  /**
   * @brief Prompt user for input
   */
  [[nodiscard]] std::string prompt(const std::string& message,
                                    const std::string& default_value = "");

  /**
   * @brief Display keyboard shortcuts
   */
  void show_shortcuts();

  /**
   * @brief Get progress indicator
   */
  [[nodiscard]] ProgressIndicator& progress() { return *progress_; }

  /**
   * @brief Get status display
   */
  [[nodiscard]] StatusDisplay& status() { return *status_; }

  /**
   * @brief Check if quiet mode is enabled
   */
  [[nodiscard]] bool is_quiet() const { return config_.quiet; }

  /**
   * @brief Check if verbose mode is enabled
   */
  [[nodiscard]] bool is_verbose() const { return config_.verbose; }

  [[nodiscard]] const CLIConfig& config() const { return config_; }

 private:
  CLIConfig config_;
  std::unique_ptr<ProgressIndicator> progress_;
  std::unique_ptr<StatusDisplay> status_;
};

/**
 * @brief RAII helper for CLI operations
 */
class CLIOperation {
 public:
  CLIOperation(CLIInterface& cli, const std::string& operation_name);
  ~CLIOperation();

  void update_progress(double progress);
  void update_progress(double progress, const std::string& message);
  void complete(const std::string& message = "Complete");
  void fail(const std::string& message = "Failed");

  // Non-copyable, non-movable
  CLIOperation(const CLIOperation&) = delete;
  CLIOperation& operator=(const CLIOperation&) = delete;
  CLIOperation(CLIOperation&&) = delete;
  CLIOperation& operator=(CLIOperation&&) = delete;

 private:
  friend class CLIInterface;
  CLIInterface& cli_;
  std::string operation_name_;
  bool completed_{false};
};

}  // namespace SolarSystem::UI
