#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace SolarSystem::Utils {

/**
 * @brief Enhanced log levels for comprehensive logging
 */
enum class LogLevel { TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };

/**
 * @brief Log output destinations
 */
enum class LogOutput { CONSOLE, FILE, NETWORK, SYSLOG, MEMORY, BOTH };

/**
 * @brief Log format types
 */
enum class LogFormat { TEXT, JSON, XML, CSV };

/**
 * @brief Enhanced log entry with comprehensive metadata
 */
struct LogEntry {
  std::chrono::system_clock::time_point timestamp;
  LogLevel level;
  std::string component;
  std::string message;
  std::string file;
  int line = 0;
  std::string function;
  std::thread::id thread_id;
  std::unordered_map<std::string, std::string> metadata;
  uint64_t sequence_number = 0;

  std::string to_string() const;
  std::string to_json() const;
  std::string to_xml() const;
  std::string to_csv() const;
};

/**
 * @brief Log appender interface for different output destinations
 */
class LogAppender {
 public:
  virtual ~LogAppender() = default;
  virtual void append(const LogEntry& entry) = 0;
  virtual void flush() = 0;
  virtual void close() = 0;
  virtual bool is_open() const = 0;

  void set_name(const std::string& name) { name_ = name; }
  const std::string& get_name() const { return name_; }

  void set_min_level(LogLevel level) { min_level_ = level; }
  LogLevel get_min_level() const { return min_level_; }

  // Statistics
  struct Statistics {
    std::atomic<uint64_t> messages_processed{0};
    std::atomic<uint64_t> messages_filtered{0};
    std::atomic<uint64_t> bytes_written{0};
    std::atomic<uint64_t> error_count{0};
  };

  const Statistics& get_statistics() const { return stats_; }

 protected:
  std::string name_;
  LogLevel min_level_ = LogLevel::TRACE;
  mutable Statistics stats_;

  bool should_log(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(min_level_);
  }

  void update_stats(size_t bytes_written) {
    stats_.messages_processed++;
    stats_.bytes_written += bytes_written;
  }
};

/**
 * @brief Console appender with color support
 */
class ConsoleAppender : public LogAppender {
 public:
  explicit ConsoleAppender(bool use_colors = true);
  void append(const LogEntry& entry) override;
  void flush() override;
  void close() override;
  bool is_open() const override { return true; }

 private:
  bool use_colors_;
  std::mutex mutex_;

  std::string get_color_code(LogLevel level) const;
  std::string get_reset_code() const;
};

/**
 * @brief File appender with rotation support
 */
class FileAppender : public LogAppender {
 public:
  FileAppender(const std::string& filename, size_t max_size = 10 * 1024 * 1024, int max_files = 5);
  ~FileAppender();

  void append(const LogEntry& entry) override;
  void flush() override;
  void close() override;
  bool is_open() const override;

  void force_rotation();

 private:
  std::string filename_;
  std::string base_filename_;
  std::ofstream file_;
  size_t max_size_;
  int max_files_;
  size_t current_size_;
  std::mutex mutex_;

  void rotate_if_needed();
  void rotate_files();
  void cleanup_old_files();
};

/**
 * @brief Memory appender for in-memory log storage
 */
class MemoryAppender : public LogAppender {
 public:
  explicit MemoryAppender(size_t max_entries = 1000);

  void append(const LogEntry& entry) override;
  void flush() override;
  void close() override;
  bool is_open() const override { return true; }

  std::vector<LogEntry> get_entries() const;
  void clear();
  size_t size() const;

 private:
  mutable std::mutex mutex_;
  std::vector<LogEntry> entries_;
  size_t max_entries_;
  size_t current_index_;
  bool is_full_;
};

/**
 * @brief Asynchronous logger for high-performance logging
 */
class AsyncLogger {
 public:
  AsyncLogger();
  ~AsyncLogger();

  void add_appender(std::unique_ptr<LogAppender> appender);
  void remove_appender(const std::string& name);
  LogAppender* get_appender(const std::string& name);

  void log(const LogEntry& entry);
  void flush();
  void shutdown();

  void set_level(LogLevel level) { min_level_ = level; }
  LogLevel get_level() const { return min_level_; }

  // Performance metrics
  struct Metrics {
    std::atomic<uint64_t> messages_logged{0};
    std::atomic<uint64_t> messages_dropped{0};
    std::atomic<uint64_t> total_processing_time_us{0};
    std::atomic<uint64_t> queue_size{0};
    std::atomic<uint64_t> peak_queue_size{0};
  };

  const Metrics& get_metrics() const { return metrics_; }
  std::string get_performance_report() const;

 private:
  std::vector<std::unique_ptr<LogAppender>> appenders_;
  std::queue<LogEntry> log_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::thread worker_thread_;
  std::atomic<bool> running_;
  LogLevel min_level_;
  Metrics metrics_;
  std::atomic<uint64_t> sequence_counter_{0};
  static constexpr size_t MAX_QUEUE_SIZE = 10000;

  void worker_loop();
  void process_entry(const LogEntry& entry);
};

/**
 * @brief Enhanced comprehensive logging system
 *
 * Provides thread-safe, configurable logging with multiple output targets,
 * structured formatting, performance monitoring, and advanced features.
 */
class Logger {
 public:
  // Maintain backward compatibility with existing Level enum
  enum class Level { DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };
  enum class Output { CONSOLE, FILE, BOTH };

  /**
   * @brief Enhanced configuration for logger behavior
   */
  struct Config {
    Level min_level = Level::INFO;
    Output output = Output::CONSOLE;
    std::string log_file = "solar_system.log";
    bool include_timestamp = true;
    bool include_thread_id = false;
    bool colored_output = true;

    // Enhanced configuration options
    bool async_logging = true;
    LogFormat format = LogFormat::TEXT;
    size_t max_file_size = 10 * 1024 * 1024;
    int max_files = 5;
    size_t queue_size = 10000;
    bool enable_performance_monitoring = true;
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
   * @brief Initialize with comprehensive logging features
   */
  void initialize_comprehensive();

  /**
   * @brief Shutdown the logger
   */
  void shutdown();

  /**
   * @brief Log a message at the specified level (backward compatible)
   */
  void log(Level level, std::string_view component, std::string_view message);

  /**
   * @brief Enhanced log method with metadata
   */
  void log_enhanced(LogLevel level, std::string_view component, std::string_view message,
                    const std::string& file = "", int line = 0, const std::string& function = "",
                    const std::unordered_map<std::string, std::string>& metadata = {});

  /**
   * @brief Convenience methods for different log levels (backward compatible)
   */
  void debug(std::string_view component, std::string_view message);
  void info(std::string_view component, std::string_view message);
  void warn(std::string_view component, std::string_view message);
  void error(std::string_view component, std::string_view message);
  void fatal(std::string_view component, std::string_view message);

  /**
   * @brief Enhanced convenience methods with new log levels
   */
  void trace(std::string_view component, std::string_view message);
  void debug_enhanced(std::string_view component, std::string_view message);
  void info_enhanced(std::string_view component, std::string_view message);
  void warn_enhanced(std::string_view component, std::string_view message);
  void error_enhanced(std::string_view component, std::string_view message);
  void fatal_enhanced(std::string_view component, std::string_view message);

  /**
   * @brief Template method for formatted logging (backward compatible)
   */
  template <typename... Args>
  void log_formatted(Level level, std::string_view component, std::string_view format,
                     Args&&... args);

  /**
   * @brief Appender management
   */
  void add_console_appender(const std::string& name = "console",
                            LogLevel min_level = LogLevel::INFO);
  void add_file_appender(const std::string& name, const std::string& filename,
                         LogLevel min_level = LogLevel::DEBUG);
  void add_memory_appender(const std::string& name, size_t max_entries = 1000,
                           LogLevel min_level = LogLevel::TRACE);
  void remove_appender(const std::string& name);
  LogAppender* get_appender(const std::string& name);

  /**
   * @brief Performance monitoring
   */
  std::string get_performance_report() const;
  std::unordered_map<std::string, LogAppender::Statistics> get_appender_statistics() const;

  /**
   * @brief Log analysis
   */
  std::vector<LogEntry> get_recent_logs(size_t count = 100) const;
  std::vector<LogEntry> search_logs(const std::string& pattern,
                                    LogLevel min_level = LogLevel::TRACE) const;

  /**
   * @brief Flush all appenders
   */
  void flush();

 private:
  Logger() = default;

  std::string format_message(Level level, std::string_view component,
                             std::string_view message) const;
  std::string level_to_string(Level level) const;
  std::string get_color_code(Level level) const;
  std::string get_timestamp() const;

  // Convert between old and new level enums
  LogLevel convert_level(Level level) const;
  Level convert_level_back(LogLevel level) const;

  Config config_;
  std::unique_ptr<std::ofstream> log_file_;
  mutable std::mutex mutex_;

  // Enhanced logging components
  std::unique_ptr<AsyncLogger> async_logger_;
  MemoryAppender* memory_appender_ =
      nullptr;  // Raw pointer - ownership transferred to async_logger_
  bool comprehensive_mode_ = false;
  std::atomic<bool> initialized_{false};
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

// Stream operator for Logger::Level to support testing
inline std::ostream& operator<<(std::ostream& os, const Logger::Level& level) {
  switch (level) {
    case Logger::Level::DEBUG: return os << "DEBUG";
    case Logger::Level::INFO: return os << "INFO";
    case Logger::Level::WARN: return os << "WARN";
    case Logger::Level::ERROR: return os << "ERROR";
    case Logger::Level::FATAL: return os << "FATAL";
    default: return os << "UNKNOWN";
  }
}

}  // namespace SolarSystem::Utils
