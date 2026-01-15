/**
 * @file progress_indicator.cpp
 * @brief Implementation of progress indication system
 */

#include "solar_core/ui/progress_indicator.hpp"

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

// Spinner frames (Braille patterns for smooth animation)
constexpr const char* SPINNER_FRAMES[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
constexpr size_t SPINNER_FRAME_COUNT = 10;

}  // namespace

ProgressIndicator::ProgressIndicator(const ProgressConfig& config) : config_(config) {}

void ProgressIndicator::start(const std::string& task_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  current_task_ = task_name;
  progress_ = 0.0;
  active_ = true;
  start_time_ = std::chrono::steady_clock::now();
  spinner_frame_ = 0;

  if (config_.accessible_mode) {
    std::cout << task_name << ": Started" << std::endl;
  }
}

void ProgressIndicator::update(double progress) {
  update(progress, current_task_);
}

void ProgressIndicator::update(double progress, const std::string& message) {
  if (!active_) return;

  std::lock_guard<std::mutex> lock(mutex_);
  progress_ = std::clamp(progress, 0.0, 1.0);

  if (config_.accessible_mode) {
    // Only output at significant milestones for screen readers
    int percent = static_cast<int>(progress_ * 100);
    if (percent % 25 == 0) {
      std::cout << message << ": " << percent << "% complete" << std::endl;
    }
    return;
  }

  switch (config_.style) {
    case ProgressStyle::BAR:
      render_bar(progress_, message);
      break;
    case ProgressStyle::SPINNER:
      render_spinner(message);
      break;
    case ProgressStyle::DOTS:
      render_dots(message);
      break;
    case ProgressStyle::PERCENTAGE:
      render_percentage(progress_, message);
      break;
    case ProgressStyle::MINIMAL:
      render_minimal(progress_, message);
      break;
  }

  std::cout << std::flush;
}

void ProgressIndicator::complete(const std::string& message) {
  if (!active_) return;

  std::lock_guard<std::mutex> lock(mutex_);
  progress_ = 1.0;
  active_ = false;

  if (!config_.accessible_mode) {
    clear_line();
  }

  if (config_.colored_output && !config_.accessible_mode) {
    std::cout << GREEN << "✓ " << RESET << message << std::endl;
  } else {
    std::cout << message << std::endl;
  }
}

void ProgressIndicator::fail(const std::string& message) {
  if (!active_) return;

  std::lock_guard<std::mutex> lock(mutex_);
  active_ = false;

  if (!config_.accessible_mode) {
    clear_line();
  }

  if (config_.colored_output && !config_.accessible_mode) {
    std::cout << RED << "✗ " << RESET << message << std::endl;
  } else {
    std::cout << "Failed: " << message << std::endl;
  }
}

void ProgressIndicator::render_bar(double progress, const std::string& message) {
  clear_line();

  std::ostringstream oss;

  // Progress bar
  size_t filled = static_cast<size_t>(progress * static_cast<double>(config_.bar_width));
  size_t empty = config_.bar_width - filled;

  if (config_.colored_output) {
    oss << BLUE << "[" << RESET;
    oss << GREEN << std::string(filled, '=');
    if (filled < config_.bar_width) {
      oss << ">";
    }
    oss << RESET << std::string(empty > 0 ? empty - 1 : 0, ' ');
    oss << BLUE << "]" << RESET;
  } else {
    oss << "[" << std::string(filled, '=');
    if (filled < config_.bar_width) {
      oss << ">";
    }
    oss << std::string(empty > 0 ? empty - 1 : 0, ' ') << "]";
  }

  // Percentage
  if (config_.show_percentage) {
    oss << " " << std::setw(3) << static_cast<int>(progress * 100) << "%";
  }

  // Message
  oss << " " << message;

  // Time estimate
  if (config_.show_time_estimate && progress > 0.01) {
    oss << " " << get_time_estimate();
  }

  std::cout << "\r" << oss.str();
}

void ProgressIndicator::render_spinner(const std::string& message) {
  clear_line();

  std::ostringstream oss;

  if (config_.colored_output) {
    oss << YELLOW << SPINNER_FRAMES[spinner_frame_] << RESET << " " << message;
  } else {
    oss << SPINNER_FRAMES[spinner_frame_] << " " << message;
  }

  spinner_frame_ = (spinner_frame_ + 1) % SPINNER_FRAME_COUNT;

  std::cout << "\r" << oss.str();
}

void ProgressIndicator::render_dots(const std::string& message) {
  clear_line();

  size_t dot_count = (spinner_frame_ % 4);
  std::string dots(dot_count, '.');

  std::ostringstream oss;
  oss << message << dots << std::string(3 - dot_count, ' ');

  spinner_frame_++;

  std::cout << "\r" << oss.str();
}

void ProgressIndicator::render_percentage(double progress, const std::string& message) {
  clear_line();

  std::ostringstream oss;
  oss << std::setw(3) << static_cast<int>(progress * 100) << "% " << message;

  if (config_.show_time_estimate && progress > 0.01) {
    oss << " " << get_time_estimate();
  }

  std::cout << "\r" << oss.str();
}

void ProgressIndicator::render_minimal(double progress, const std::string& message) {
  // Text-only, no special characters
  std::ostringstream oss;
  oss << message << ": " << static_cast<int>(progress * 100) << "% complete";

  std::cout << "\r" << oss.str();
}

std::string ProgressIndicator::get_time_estimate() const {
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);

  if (progress_ < 0.01) {
    return "";
  }

  auto total_estimated = static_cast<double>(elapsed.count()) / progress_;
  auto remaining = total_estimated - static_cast<double>(elapsed.count());

  if (remaining < 0) remaining = 0;

  std::ostringstream oss;
  if (remaining < 60) {
    oss << "(" << remaining << "s remaining)";
  } else if (remaining < 3600) {
    auto minutes = static_cast<long>(remaining / 60);
    auto seconds = static_cast<long>(remaining) % 60;
    oss << "(" << minutes << "m " << seconds << "s remaining)";
  } else {
    auto hours = static_cast<long>(remaining / 3600);
    auto minutes = (static_cast<long>(remaining) % 3600) / 60;
    oss << "(" << hours << "h " << minutes << "m remaining)";
  }

  return oss.str();
}

void ProgressIndicator::clear_line() {
  // Move cursor to beginning and clear line
  std::cout << "\r" << std::string(80, ' ') << "\r";
}

}  // namespace SolarSystem::UI
