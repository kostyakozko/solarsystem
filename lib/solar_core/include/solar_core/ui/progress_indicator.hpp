/**
 * @file progress_indicator.hpp
 * @brief Modern progress indication system for CLI applications
 *
 * Provides visual feedback for long-running operations with:
 * - Multiple progress bar styles
 * - Percentage and time estimates
 * - Accessibility support
 * - Thread-safe updates
 */

#pragma once

#include "solar_core/export.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>

namespace SolarSystem::UI {

/**
 * @brief Progress indicator styles
 */
enum class ProgressStyle {
  BAR,           // [=====>    ] 50%
  SPINNER,       // ⠋ Processing...
  DOTS,          // ... Processing
  PERCENTAGE,    // 50% Complete
  MINIMAL        // Accessible text-only
};

/**
 * @brief Progress indicator configuration
 */
struct ProgressConfig {
  ProgressStyle style = ProgressStyle::BAR;
  size_t bar_width = 40;
  bool show_percentage = true;
  bool show_time_estimate = true;
  bool accessible_mode = false;  // Screen reader friendly
  bool colored_output = true;
  std::chrono::milliseconds update_interval = std::chrono::milliseconds(100);
};

/**
 * @brief Thread-safe progress indicator for CLI applications
 */
class SOLAR_CORE_API ProgressIndicator {
 public:
  /**
   * @brief Construct progress indicator with configuration
   */
  explicit ProgressIndicator(const ProgressConfig& config = {});

  /**
   * @brief Start progress indication
   */
  void start(const std::string& task_name);

  /**
   * @brief Update progress (0.0 to 1.0)
   */
  void update(double progress);

  /**
   * @brief Update with custom message
   */
  void update(double progress, const std::string& message);

  /**
   * @brief Complete progress indication
   */
  void complete(const std::string& message = "Complete");

  /**
   * @brief Fail progress indication
   */
  void fail(const std::string& message = "Failed");

  /**
   * @brief Check if progress is active
   */
  [[nodiscard]] bool is_active() const { return active_; }

  /**
   * @brief Get current progress (0.0 to 1.0)
   */
  [[nodiscard]] double get_progress() const { return progress_; }

 private:
  void render_bar(double progress, const std::string& message);
  void render_spinner(const std::string& message);
  void render_dots(const std::string& message);
  void render_percentage(double progress, const std::string& message);
  void render_minimal(double progress, const std::string& message);

  std::string get_time_estimate() const;
  void clear_line();

  ProgressConfig config_;
  std::atomic<double> progress_{0.0};
  std::atomic<bool> active_{false};
  std::chrono::steady_clock::time_point start_time_;
  std::string current_task_;
  size_t spinner_frame_{0};
  mutable std::mutex mutex_;
};

/**
 * @brief RAII progress indicator that auto-completes
 */
class SOLAR_CORE_API ScopedProgress {
 public:
  ScopedProgress(ProgressIndicator& indicator, const std::string& task_name)
      : indicator_(indicator) {
    indicator_.start(task_name);
  }

  ~ScopedProgress() {
    if (indicator_.is_active()) {
      indicator_.complete();
    }
  }

  void update(double progress) { indicator_.update(progress); }

  void update(double progress, const std::string& message) {
    indicator_.update(progress, message);
  }

  // Non-copyable, non-movable
  ScopedProgress(const ScopedProgress&) = delete;
  ScopedProgress& operator=(const ScopedProgress&) = delete;
  ScopedProgress(ScopedProgress&&) = delete;
  ScopedProgress& operator=(ScopedProgress&&) = delete;

 private:
  ProgressIndicator& indicator_;
};

}  // namespace SolarSystem::UI
