#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>

namespace SolarSystem::Utils {

/**
 * @brief Modern structured logging system
 *
 * Provides thread-safe, configurable logging with multiple output targets
 * and structured formatting for better debugging and monitoring.
 */
class Logger {
 public:
  enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4 };

  enum class Output { CONSOLE, FILE, BOTH };

  /**
   * @brief Configuration for logger behavior
   */
  struct Config {
    Level min_level = Level::INFO;
    Output output = Output::CONSOLE;
    std::string log_file = "solar_system.log";
    bool include_timestamp = true;
    bool include_thread_id = false;
    bool colored_output = true;
  };

  /**
   * @brief Get the global logger instance
   */
  static Logger& instance();

  /**
   * @brief Configure the logger
   */
  void configure(const Config& config);

  /**
   * @brief Log a message at the specified level
   */
  void log(Level level, std::string_view component, std::string_view message);

  /**
   * @brief Convenience methods for different log levels
   */
  void debug(std::string_view component, std::string_view message);
  void info(std::string_view component, std::string_view message);
  void warn(std::string_view component, std::string_view message);
  void error(std::string_view component, std::string_view message);
  void fatal(std::string_view component, std::string_view message);

  /**
   * @brief Template method for formatted logging
   */
  template <typename... Args>
  void log_formatted(Level level, std::string_view component, std::string_view format,
                     Args&&... args);

 private:
  Logger() = default;

  std::string format_message(Level level, std::string_view component,
                             std::string_view message) const;
  std::string level_to_string(Level level) const;
  std::string get_color_code(Level level) const;
  std::string get_timestamp() const;

  Config config_;
  std::unique_ptr<std::ofstream> log_file_;
  mutable std::mutex mutex_;
};

/**
 * @brief Convenient macros for logging
 */
#define LOG_DEBUG(component, message) \
  SolarSystem::Utils::Logger::instance().debug(component, message)

#define LOG_INFO(component, message) SolarSystem::Utils::Logger::instance().info(component, message)

#define LOG_WARN(component, message) SolarSystem::Utils::Logger::instance().warn(component, message)

#define LOG_ERROR(component, message) \
  SolarSystem::Utils::Logger::instance().error(component, message)

#define LOG_FATAL(component, message) \
  SolarSystem::Utils::Logger::instance().fatal(component, message)

/**
 * @brief Formatted logging macros
 */
#define LOG_DEBUG_F(component, format, ...)                                                      \
  SolarSystem::Utils::Logger::instance().log_formatted(SolarSystem::Utils::Logger::Level::DEBUG, \
                                                       component, format, __VA_ARGS__)

#define LOG_INFO_F(component, format, ...)                                                      \
  SolarSystem::Utils::Logger::instance().log_formatted(SolarSystem::Utils::Logger::Level::INFO, \
                                                       component, format, __VA_ARGS__)

#define LOG_WARN_F(component, format, ...)                                                      \
  SolarSystem::Utils::Logger::instance().log_formatted(SolarSystem::Utils::Logger::Level::WARN, \
                                                       component, format, __VA_ARGS__)

#define LOG_ERROR_F(component, format, ...)                                                      \
  SolarSystem::Utils::Logger::instance().log_formatted(SolarSystem::Utils::Logger::Level::ERROR, \
                                                       component, format, __VA_ARGS__)

}  // namespace SolarSystem::Utils
