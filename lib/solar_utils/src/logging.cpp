#include "solar_utils/logging.hpp"

#include <iomanip>
#include <thread>

namespace SolarSystem::Utils {

Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::configure(const Config& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  config_ = config;

  // Open log file if needed
  if (config_.output == Output::FILE || config_.output == Output::BOTH) {
    log_file_ = std::make_unique<std::ofstream>(config_.log_file, std::ios::app);
    if (!log_file_->is_open()) {
      std::cerr << "Warning: Failed to open log file: " << config_.log_file << std::endl;
    }
  }
}

void Logger::log(Level level, std::string_view component, std::string_view message) {
  if (level < config_.min_level) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  std::string formatted = format_message(level, component, message);

  // Output to console
  if (config_.output == Output::CONSOLE || config_.output == Output::BOTH) {
    if (config_.colored_output) {
      std::cout << get_color_code(level) << formatted << "\033[0m" << std::endl;
    } else {
      std::cout << formatted << std::endl;
    }
  }

  // Output to file
  if ((config_.output == Output::FILE || config_.output == Output::BOTH) && log_file_ &&
      log_file_->is_open()) {
    *log_file_ << formatted << std::endl;
    log_file_->flush();
  }
}

void Logger::debug(std::string_view component, std::string_view message) {
  log(Level::DEBUG, component, message);
}

void Logger::info(std::string_view component, std::string_view message) {
  log(Level::INFO, component, message);
}

void Logger::warn(std::string_view component, std::string_view message) {
  log(Level::WARN, component, message);
}

void Logger::error(std::string_view component, std::string_view message) {
  log(Level::ERROR, component, message);
}

void Logger::fatal(std::string_view component, std::string_view message) {
  log(Level::FATAL, component, message);
}

template <typename... Args>
void Logger::log_formatted(Level level, std::string_view component, std::string_view format,
                           Args&&... args) {
  if (level < config_.min_level) {
    return;
  }

  // Simple sprintf-style formatting (could be enhanced with std::format in C++20)
  std::ostringstream oss;
  oss << format;  // Basic implementation - could be improved
  log(level, component, oss.str());
}

std::string Logger::format_message(Level level, std::string_view component,
                                   std::string_view message) const {
  std::ostringstream oss;

  // Timestamp
  if (config_.include_timestamp) {
    oss << "[" << get_timestamp() << "] ";
  }

  // Log level
  oss << "[" << level_to_string(level) << "] ";

  // Component
  oss << "[" << component << "] ";

  // Thread ID
  if (config_.include_thread_id) {
    oss << "[" << std::this_thread::get_id() << "] ";
  }

  // Message
  oss << message;

  return oss.str();
}

std::string Logger::level_to_string(Level level) const {
  switch (level) {
    case Level::DEBUG:
      return "DEBUG";
    case Level::INFO:
      return "INFO ";
    case Level::WARN:
      return "WARN ";
    case Level::ERROR:
      return "ERROR";
    case Level::FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

std::string Logger::get_color_code(Level level) const {
  if (!config_.colored_output) {
    return "";
  }

  switch (level) {
    case Level::DEBUG:
      return "\033[36m";  // Cyan
    case Level::INFO:
      return "\033[32m";  // Green
    case Level::WARN:
      return "\033[33m";  // Yellow
    case Level::ERROR:
      return "\033[31m";  // Red
    case Level::FATAL:
      return "\033[35m";  // Magenta
    default:
      return "";
  }
}

std::string Logger::get_timestamp() const {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  oss << "." << std::setfill('0') << std::setw(3) << ms.count();

  return oss.str();
}

}  // namespace SolarSystem::Utils
