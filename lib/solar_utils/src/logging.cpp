#include "solar_utils/logging.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <regex>
#include <thread>

namespace SolarSystem::Utils {

// LogEntry implementation
std::string LogEntry::to_string() const {
  std::ostringstream oss;
  auto time_t = std::chrono::system_clock::to_time_t(timestamp);
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

  oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0')
      << std::setw(3) << ms.count() << " [" << static_cast<int>(level) << "]"
      << " [" << component << "]"
      << " " << message;

  if (!metadata.empty()) {
    oss << " {";
    bool first = true;
    for (const auto& [key, value] : metadata) {
      if (!first) oss << ", ";
      oss << key << "=" << value;
      first = false;
    }
    oss << "}";
  }

  return oss.str();
}

std::string LogEntry::to_json() const {
  auto time_t = std::chrono::system_clock::to_time_t(timestamp);
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

  std::ostringstream timestamp_oss;
  timestamp_oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S") << '.'
                << std::setfill('0') << std::setw(3) << ms.count() << "Z";

  std::ostringstream thread_oss;
  thread_oss << thread_id;

  nlohmann::json j;
  j["timestamp"] = timestamp_oss.str();
  j["level"] = static_cast<int>(level);
  j["component"] = component;
  j["message"] = message;
  j["thread_id"] = thread_oss.str();
  j["sequence_number"] = sequence_number;

  if (!metadata.empty()) {
    j["metadata"] = metadata;
  }

  return j.dump();
}

std::string LogEntry::to_xml() const {
  std::ostringstream oss;
  auto time_t = std::chrono::system_clock::to_time_t(timestamp);

  oss << "<log>"
      << "<timestamp>" << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ")
      << "</timestamp>"
      << "<level>" << static_cast<int>(level) << "</level>"
      << "<component>" << component << "</component>"
      << "<message>" << message << "</message>"
      << "<thread_id>" << thread_id << "</thread_id>"
      << "<sequence_number>" << sequence_number << "</sequence_number>";

  if (!metadata.empty()) {
    oss << "<metadata>";
    for (const auto& [key, value] : metadata) {
      oss << "<" << key << ">" << value << "</" << key << ">";
    }
    oss << "</metadata>";
  }

  oss << "</log>";
  return oss.str();
}

std::string LogEntry::to_csv() const {
  std::ostringstream oss;
  auto time_t = std::chrono::system_clock::to_time_t(timestamp);

  oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S") << "," << static_cast<int>(level)
      << "," << component << ","
      << "\"" << message << "\"," << thread_id << "," << sequence_number;

  return oss.str();
}

// ConsoleAppender implementation
ConsoleAppender::ConsoleAppender(bool use_colors) : use_colors_(use_colors) { name_ = "console"; }

void ConsoleAppender::append(const LogEntry& entry) {
  if (!should_log(entry.level)) {
    stats_.messages_filtered++;
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  std::string formatted = entry.to_string();

  if (use_colors_) {
    std::cout << get_color_code(entry.level) << formatted << get_reset_code() << std::endl;
  } else {
    std::cout << formatted << std::endl;
  }

  update_stats(formatted.length());
}

void ConsoleAppender::flush() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::cout.flush();
}

void ConsoleAppender::close() {
  // Nothing to close for console
}

std::string ConsoleAppender::get_color_code(LogLevel level) const {
  switch (level) {
    case LogLevel::TRACE:
      return "\033[37m";  // White
    case LogLevel::DEBUG:
      return "\033[36m";  // Cyan
    case LogLevel::INFO:
      return "\033[32m";  // Green
    case LogLevel::WARN:
      return "\033[33m";  // Yellow
    case LogLevel::ERROR:
      return "\033[31m";  // Red
    case LogLevel::FATAL:
      return "\033[35m";  // Magenta
    default:
      return "";
  }
}

std::string ConsoleAppender::get_reset_code() const { return "\033[0m"; }

// FileAppender implementation
FileAppender::FileAppender(const std::string& filename, size_t max_size, int max_files)
    : filename_(filename),
      base_filename_(filename),
      max_size_(max_size),
      max_files_(max_files),
      current_size_(0) {
  name_ = "file_" + std::filesystem::path(filename).filename().string();

  // Create directory if it doesn't exist
  std::filesystem::path file_path(filename);
  if (file_path.has_parent_path()) {
    std::filesystem::create_directories(file_path.parent_path());
  }

  file_.open(filename_, std::ios::app);
  if (file_.is_open()) {
    file_.seekp(0, std::ios::end);
    current_size_ = static_cast<size_t>(file_.tellp());
  }
}

FileAppender::~FileAppender() { close(); }

void FileAppender::append(const LogEntry& entry) {
  if (!should_log(entry.level)) {
    stats_.messages_filtered++;
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  if (!file_.is_open()) {
    file_.open(filename_, std::ios::app);
    if (!file_.is_open()) {
      stats_.error_count++;
      return;
    }
  }

  std::string formatted = entry.to_string();
  file_ << formatted << std::endl;
  current_size_ += formatted.length() + 1;

  update_stats(formatted.length() + 1);
  rotate_if_needed();
}

void FileAppender::flush() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_.is_open()) {
    file_.flush();
  }
}

void FileAppender::close() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_.is_open()) {
    file_.close();
  }
}

bool FileAppender::is_open() const {
  std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
  return file_.is_open();
}

void FileAppender::force_rotation() {
  std::lock_guard<std::mutex> lock(mutex_);
  rotate_files();
}

void FileAppender::rotate_if_needed() {
  if (current_size_ >= max_size_) {
    rotate_files();
  }
}

void FileAppender::rotate_files() {
  file_.close();

  // Generate timestamped filename for current file
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestamp_oss;
  timestamp_oss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");

  std::string timestamped_name = base_filename_ + "." + timestamp_oss.str();

  // Move current file to timestamped name
  if (std::filesystem::exists(filename_)) {
    std::filesystem::rename(filename_, timestamped_name);
  }

  // Clean up old files
  cleanup_old_files();

  // Open new file
  file_.open(filename_, std::ios::out | std::ios::trunc);
  current_size_ = 0;
}

void FileAppender::cleanup_old_files() {
  std::filesystem::path dir = std::filesystem::path(base_filename_).parent_path();
  if (dir.empty()) dir = ".";

  std::string base_name = std::filesystem::path(base_filename_).filename().string();

  std::vector<std::filesystem::path> log_files;

  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().filename().string().find(base_name + ".") == 0) {
      log_files.push_back(entry.path());
    }
  }

  // Sort by modification time (newest first)
  std::sort(log_files.begin(), log_files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });

  // Remove excess files
  for (size_t i = static_cast<size_t>(max_files_); i < log_files.size(); ++i) {
    std::filesystem::remove(log_files[i]);
  }
}

// MemoryAppender implementation
MemoryAppender::MemoryAppender(size_t max_entries)
    : max_entries_(max_entries), current_index_(0), is_full_(false) {
  name_ = "memory";
  entries_.reserve(max_entries_);
}

void MemoryAppender::append(const LogEntry& entry) {
  if (!should_log(entry.level)) {
    stats_.messages_filtered++;
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  if (entries_.size() < max_entries_) {
    entries_.push_back(entry);
  } else {
    entries_[current_index_] = entry;
    current_index_ = (current_index_ + 1) % max_entries_;
    is_full_ = true;
  }

  update_stats(entry.message.length());
}

void MemoryAppender::flush() {
  // Nothing to flush for memory appender
}

void MemoryAppender::close() {
  // Nothing to close for memory appender
}

std::vector<LogEntry> MemoryAppender::get_entries() const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!is_full_) {
    return entries_;
  }

  std::vector<LogEntry> result;
  result.reserve(max_entries_);

  // Add entries from current_index_ to end
  for (size_t i = current_index_; i < entries_.size(); ++i) {
    result.push_back(entries_[i]);
  }

  // Add entries from beginning to current_index_
  for (size_t i = 0; i < current_index_; ++i) {
    result.push_back(entries_[i]);
  }

  return result;
}

void MemoryAppender::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  entries_.clear();
  current_index_ = 0;
  is_full_ = false;
}

size_t MemoryAppender::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_.size();
}

// AsyncLogger implementation
AsyncLogger::AsyncLogger() : running_(true), min_level_(LogLevel::INFO) {
  worker_thread_ = std::thread(&AsyncLogger::worker_loop, this);
}

AsyncLogger::~AsyncLogger() { shutdown(); }

void AsyncLogger::add_appender(std::unique_ptr<LogAppender> appender) {
  appenders_.push_back(std::move(appender));
}

void AsyncLogger::remove_appender(const std::string& name) {
  appenders_.erase(std::remove_if(appenders_.begin(), appenders_.end(),
                                  [&name](const std::unique_ptr<LogAppender>& appender) {
                                    return appender->get_name() == name;
                                  }),
                   appenders_.end());
}

LogAppender* AsyncLogger::get_appender(const std::string& name) {
  auto it = std::find_if(appenders_.begin(), appenders_.end(),
                         [&name](const std::unique_ptr<LogAppender>& appender) {
                           return appender->get_name() == name;
                         });

  return (it != appenders_.end()) ? it->get() : nullptr;
}

void AsyncLogger::log(const LogEntry& entry) {
  if (static_cast<int>(entry.level) < static_cast<int>(min_level_)) {
    return;
  }

  std::unique_lock<std::mutex> lock(queue_mutex_);

  if (log_queue_.size() >= MAX_QUEUE_SIZE) {
    metrics_.messages_dropped++;
    return;
  }

  LogEntry enhanced_entry = entry;
  enhanced_entry.sequence_number = sequence_counter_++;

  log_queue_.push(enhanced_entry);
  size_t current_queue_size = log_queue_.size();
  metrics_.queue_size = current_queue_size;

  if (current_queue_size > metrics_.peak_queue_size) {
    metrics_.peak_queue_size = current_queue_size;
  }

  lock.unlock();
  queue_cv_.notify_one();
}

void AsyncLogger::flush() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  queue_cv_.wait(lock, [this] { return log_queue_.empty(); });

  for (auto& appender : appenders_) {
    appender->flush();
  }
}

void AsyncLogger::shutdown() {
  running_ = false;
  queue_cv_.notify_all();

  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }

  for (auto& appender : appenders_) {
    appender->close();
  }
}

std::string AsyncLogger::get_performance_report() const {
  const auto& m = metrics_;

  std::ostringstream oss;
  oss << "=== AsyncLogger Performance Report ===\n";
  oss << "Messages logged: " << m.messages_logged << "\n";
  oss << "Messages dropped: " << m.messages_dropped << "\n";
  oss << "Current queue size: " << m.queue_size << "\n";
  oss << "Peak queue size: " << m.peak_queue_size << "\n";

  if (m.messages_logged > 0) {
    double avg_processing_time =
        static_cast<double>(m.total_processing_time_us) / static_cast<double>(m.messages_logged);
    oss << "Average processing time: " << avg_processing_time << " microseconds\n";
  }

  return oss.str();
}

void AsyncLogger::worker_loop() {
  while (running_) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_cv_.wait(lock, [this] { return !log_queue_.empty() || !running_; });

    while (!log_queue_.empty() && running_) {
      LogEntry entry = log_queue_.front();
      log_queue_.pop();
      metrics_.queue_size = log_queue_.size();
      lock.unlock();

      process_entry(entry);

      lock.lock();
    }
  }
}

void AsyncLogger::process_entry(const LogEntry& entry) {
  auto start_time = std::chrono::high_resolution_clock::now();

  for (auto& appender : appenders_) {
    try {
      appender->append(entry);
    } catch (...) {
      // Ignore appender errors to prevent logging system failure
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto processing_time =
      std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  metrics_.messages_logged++;
  metrics_.total_processing_time_us += static_cast<uint64_t>(processing_time.count());
}

// Logger implementation (backward compatible with enhancements)
Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::configure(const Config& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  config_ = config;

  // Initialize comprehensive mode if requested
  if (config_.async_logging) {
    initialize_comprehensive();
  }

  // Open log file if needed (backward compatibility)
  if (config_.output == Output::FILE || config_.output == Output::BOTH) {
    log_file_ = std::make_unique<std::ofstream>(config_.log_file, std::ios::app);
    if (!log_file_->is_open()) {
      std::cerr << "Warning: Failed to open log file: " << config_.log_file << std::endl;
    }
  }
}

void Logger::initialize_comprehensive() {
  if (comprehensive_mode_) {
    return;
  }

  // Set comprehensive_mode_ early to prevent recursion
  comprehensive_mode_ = true;

  async_logger_ = std::make_unique<AsyncLogger>();
  async_logger_->set_level(convert_level(config_.min_level));

  // Add memory appender for log analysis
  auto memory_appender = std::make_unique<MemoryAppender>(1000);
  memory_appender_ = memory_appender.get();                 // Keep raw pointer for access
  async_logger_->add_appender(std::move(memory_appender));  // Transfer ownership

  // Add default appenders based on configuration
  if (config_.output == Output::CONSOLE || config_.output == Output::BOTH) {
    auto appender = std::make_unique<ConsoleAppender>(config_.colored_output);
    appender->set_name("default_console");
    appender->set_min_level(convert_level(config_.min_level));
    async_logger_->add_appender(std::move(appender));
  }

  if (config_.output == Output::FILE || config_.output == Output::BOTH) {
    auto appender = std::make_unique<FileAppender>(config_.log_file);
    appender->set_name("default_file");
    appender->set_min_level(convert_level(config_.min_level));
    async_logger_->add_appender(std::move(appender));
  }

  initialized_ = true;
}

void Logger::shutdown() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (async_logger_) {
    async_logger_->shutdown();
    async_logger_.reset();
  }

  if (log_file_) {
    log_file_->close();
    log_file_.reset();
  }

  memory_appender_ = nullptr;
  comprehensive_mode_ = false;
  initialized_ = false;
}

void Logger::log(Level level, std::string_view component, std::string_view message) {
  // Use comprehensive logging if available
  if (comprehensive_mode_ && async_logger_) {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = convert_level(level);
    entry.component = std::string(component);
    entry.message = std::string(message);
    entry.thread_id = std::this_thread::get_id();

    async_logger_->log(entry);
    return;
  }

  // Fallback to original implementation
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

void Logger::log_enhanced(LogLevel level, std::string_view component, std::string_view message,
                          const std::string& file, int line, const std::string& function,
                          const std::unordered_map<std::string, std::string>& metadata) {
  if (!comprehensive_mode_) {
    initialize_comprehensive();
  }

  LogEntry entry;
  entry.timestamp = std::chrono::system_clock::now();
  entry.level = level;
  entry.component = std::string(component);
  entry.message = std::string(message);
  entry.file = file;
  entry.line = line;
  entry.function = function;
  entry.thread_id = std::this_thread::get_id();
  entry.metadata = metadata;

  async_logger_->log(entry);
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

void Logger::trace(std::string_view component, std::string_view message) {
  log_enhanced(LogLevel::TRACE, component, message);
}

void Logger::debug_enhanced(std::string_view component, std::string_view message) {
  log_enhanced(LogLevel::DEBUG, component, message);
}

void Logger::info_enhanced(std::string_view component, std::string_view message) {
  log_enhanced(LogLevel::INFO, component, message);
}

void Logger::warn_enhanced(std::string_view component, std::string_view message) {
  log_enhanced(LogLevel::WARN, component, message);
}

void Logger::error_enhanced(std::string_view component, std::string_view message) {
  log_enhanced(LogLevel::ERROR, component, message);
}

void Logger::fatal_enhanced(std::string_view component, std::string_view message) {
  log_enhanced(LogLevel::FATAL, component, message);
}

template <typename... Args>
void Logger::log_formatted(Level level, std::string_view component, std::string_view format,
                           Args&&...) {
  if (level < config_.min_level) {
    return;
  }

  // Simple sprintf-style formatting (could be enhanced with std::format in C++20)
  std::ostringstream oss;
  oss << format;  // Basic implementation - could be improved
  log(level, component, oss.str());
}

void Logger::add_console_appender(const std::string& name, LogLevel min_level) {
  if (!comprehensive_mode_) {
    initialize_comprehensive();
  }

  auto appender = std::make_unique<ConsoleAppender>(config_.colored_output);
  appender->set_name(name);
  appender->set_min_level(min_level);
  async_logger_->add_appender(std::move(appender));
}

void Logger::add_file_appender(const std::string& name, const std::string& filename,
                               LogLevel min_level) {
  if (!comprehensive_mode_) {
    initialize_comprehensive();
  }

  auto appender =
      std::make_unique<FileAppender>(filename, config_.max_file_size, config_.max_files);
  appender->set_name(name);
  appender->set_min_level(min_level);
  async_logger_->add_appender(std::move(appender));
}

void Logger::add_memory_appender(const std::string& name, size_t max_entries, LogLevel min_level) {
  if (!comprehensive_mode_) {
    initialize_comprehensive();
  }

  auto appender = std::make_unique<MemoryAppender>(max_entries);
  appender->set_name(name);
  appender->set_min_level(min_level);
  async_logger_->add_appender(std::move(appender));
}

void Logger::remove_appender(const std::string& name) {
  if (comprehensive_mode_ && async_logger_) {
    async_logger_->remove_appender(name);
  }
}

LogAppender* Logger::get_appender(const std::string& name) {
  if (comprehensive_mode_ && async_logger_) {
    return async_logger_->get_appender(name);
  }
  return nullptr;
}

std::string Logger::get_performance_report() const {
  if (comprehensive_mode_ && async_logger_) {
    return async_logger_->get_performance_report();
  }
  return "Comprehensive logging not enabled";
}

std::unordered_map<std::string, LogAppender::Statistics> Logger::get_appender_statistics() const {
  std::unordered_map<std::string, LogAppender::Statistics> stats;

  if (comprehensive_mode_ && async_logger_) {
    // Implementation would collect statistics from all appenders
    // This is a simplified version
  }

  return stats;
}

std::vector<LogEntry> Logger::get_recent_logs(size_t count) const {
  if (memory_appender_) {
    auto all_entries = memory_appender_->get_entries();
    if (all_entries.size() <= count) {
      return all_entries;
    }

    return std::vector<LogEntry>(all_entries.end() - static_cast<std::ptrdiff_t>(count),
                                 all_entries.end());
  }

  return {};
}

std::vector<LogEntry> Logger::search_logs(const std::string& pattern, LogLevel min_level) const {
  std::vector<LogEntry> results;

  if (memory_appender_) {
    auto all_entries = memory_appender_->get_entries();
    std::regex search_regex(pattern, std::regex_constants::icase);

    for (const auto& entry : all_entries) {
      if (static_cast<int>(entry.level) >= static_cast<int>(min_level) &&
          std::regex_search(entry.message, search_regex)) {
        results.push_back(entry);
      }
    }
  }

  return results;
}

void Logger::flush() {
  if (comprehensive_mode_ && async_logger_) {
    async_logger_->flush();
  }

  if (log_file_) {
    log_file_->flush();
  }
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

LogLevel Logger::convert_level(Level level) const {
  switch (level) {
    case Level::DEBUG:
      return LogLevel::DEBUG;
    case Level::INFO:
      return LogLevel::INFO;
    case Level::WARN:
      return LogLevel::WARN;
    case Level::ERROR:
      return LogLevel::ERROR;
    case Level::FATAL:
      return LogLevel::FATAL;
    default:
      return LogLevel::INFO;
  }
}

Logger::Level Logger::convert_level_back(LogLevel level) const {
  switch (level) {
    case LogLevel::TRACE:
      return Level::DEBUG;  // Map TRACE to DEBUG for backward compatibility
    case LogLevel::DEBUG:
      return Level::DEBUG;
    case LogLevel::INFO:
      return Level::INFO;
    case LogLevel::WARN:
      return Level::WARN;
    case LogLevel::ERROR:
      return Level::ERROR;
    case LogLevel::FATAL:
      return Level::FATAL;
    default:
      return Level::INFO;
  }
}

}  // namespace SolarSystem::Utils
