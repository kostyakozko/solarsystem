/**
 * @file test_logging.cpp
 * @brief Unit tests for comprehensive logging system
 */

#include "../utils/test_framework.h"

#include "solar_utils/logging.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

using namespace SolarSystem::Utils;

int main() {
  TestSuite suite("Logging System Tests");

  // Basic logging tests
  suite.run_test("Logger Singleton", []() {
    auto& logger1 = Logger::instance();
    auto& logger2 = Logger::instance();

    // Should be same instance
    if (&logger1 != &logger2) throw std::runtime_error("Logger should be singleton");
  });

  suite.run_test("Basic Log Levels", []() {
    auto& logger = Logger::instance();

    // Should not throw
    logger.debug("test", "Debug message");
    logger.info("test", "Info message");
    logger.warn("test", "Warning message");
    logger.error("test", "Error message");
  });

  suite.run_test("Logger Configuration", []() {
    auto& logger = Logger::instance();

    Logger::Config config;
    config.min_level = Logger::Level::DEBUG;
    config.output = Logger::Output::CONSOLE;
    config.colored_output = true;

    // Should not throw
    logger.configure(config);
  });

  // Console appender tests
  suite.run_test("Console Appender Creation", []() {
    ConsoleAppender appender(true);

    if (!appender.is_open()) throw std::runtime_error("Console appender should be open");
  });

  suite.run_test("Console Appender Logging", []() {
    ConsoleAppender appender(false);  // No colors for testing

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "test";
    entry.message = "Test message";
    entry.thread_id = std::this_thread::get_id();

    // Should not throw
    appender.append(entry);
    appender.flush();
  });

  // File appender tests
  suite.run_test("File Appender Creation", []() {
    const std::string test_file = "test_log.txt";

    // Clean up if exists
    std::filesystem::remove(test_file);

    FileAppender appender(test_file, 1024, 3);

    if (!appender.is_open()) throw std::runtime_error("File appender should be open");

    // Clean up
    appender.close();
    std::filesystem::remove(test_file);
  });

  suite.run_test("File Appender Logging", []() {
    const std::string test_file = "test_log.txt";
    std::filesystem::remove(test_file);

    {
      FileAppender appender(test_file);

      LogEntry entry;
      entry.timestamp = std::chrono::system_clock::now();
      entry.level = LogLevel::INFO;
      entry.component = "test";
      entry.message = "Test message";

      appender.append(entry);
      appender.flush();
      appender.close();
    }

    // Verify file was created and has content
    if (!std::filesystem::exists(test_file)) {
      throw std::runtime_error("Log file should exist");
    }

    std::ifstream file(test_file);
    std::string content;
    std::getline(file, content);

    if (content.empty()) throw std::runtime_error("Log file should have content");

    // Clean up
    std::filesystem::remove(test_file);
  });

  // Memory appender tests
  suite.run_test("Memory Appender Creation", []() {
    MemoryAppender appender(100);

    if (!appender.is_open()) throw std::runtime_error("Memory appender should be open");
    if (appender.size() != 0) throw std::runtime_error("Memory appender should start empty");
  });

  suite.run_test("Memory Appender Storage", []() {
    MemoryAppender appender(10);

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "test";
    entry.message = "Test message";

    // Add entries
    for (int i = 0; i < 5; ++i) {
      entry.message = "Message " + std::to_string(i);
      appender.append(entry);
    }

    if (appender.size() != 5) throw std::runtime_error("Should have 5 entries");

    auto entries = appender.get_entries();
    if (entries.size() != 5) throw std::runtime_error("Should retrieve 5 entries");
  });

  suite.run_test("Memory Appender Circular Buffer", []() {
    MemoryAppender appender(5);  // Small buffer

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "test";

    // Add more entries than buffer size
    for (int i = 0; i < 10; ++i) {
      entry.message = "Message " + std::to_string(i);
      appender.append(entry);
    }

    // Should only keep last 5
    if (appender.size() != 5) throw std::runtime_error("Should have 5 entries (circular buffer)");
  });

  suite.run_test("Memory Appender Clear", []() {
    MemoryAppender appender(10);

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "test";
    entry.message = "Test";

    appender.append(entry);
    appender.append(entry);

    if (appender.size() != 2) throw std::runtime_error("Should have 2 entries");

    appender.clear();

    if (appender.size() != 0) throw std::runtime_error("Should be empty after clear");
  });

  // Log entry formatting tests
  suite.run_test("LogEntry to_string", []() {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "test";
    entry.message = "Test message";

    std::string str = entry.to_string();

    if (str.empty()) throw std::runtime_error("String representation should not be empty");
    // Level is output as number, e.g. [2] for INFO
    if (str.find("[2]") == std::string::npos) {
      throw std::runtime_error("Should contain log level");
    }
    if (str.find("test") == std::string::npos) {
      throw std::runtime_error("Should contain component");
    }
  });

  suite.run_test("LogEntry to_json", []() {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "test";
    entry.message = "Test message";

    std::string json = entry.to_json();

    if (json.empty()) throw std::runtime_error("JSON representation should not be empty");
    if (json.find("\"level\"") == std::string::npos) {
      throw std::runtime_error("JSON should contain level field");
    }
  });

  // Appender statistics tests
  suite.run_test("Appender Statistics", []() {
    ConsoleAppender appender(false);

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "test";
    entry.message = "Test";

    // Log some messages
    for (int i = 0; i < 5; ++i) {
      appender.append(entry);
    }

    const auto& stats = appender.get_statistics();
    if (stats.messages_processed < 5) {
      throw std::runtime_error("Should have processed at least 5 messages");
    }
  });

  // Log level filtering tests
  suite.run_test("Appender Level Filtering", []() {
    ConsoleAppender appender(false);
    appender.set_min_level(LogLevel::WARN);

    if (appender.get_min_level() != LogLevel::WARN) {
      throw std::runtime_error("Min level should be WARN");
    }
  });

  // Enhanced logging tests
  suite.run_test("Enhanced Logging Methods", []() {
    auto& logger = Logger::instance();

    // Should not throw
    logger.trace("test", "Trace message");
    logger.debug_enhanced("test", "Debug message");
    logger.info_enhanced("test", "Info message");
    logger.warn_enhanced("test", "Warning message");
    logger.error_enhanced("test", "Error message");
    logger.fatal_enhanced("test", "Fatal message");
  });

  // Appender management tests
  suite.run_test("Add and Remove Appenders", []() {
    auto& logger = Logger::instance();

    logger.add_console_appender("test_console");
    logger.add_memory_appender("test_memory", 100);

    auto* console = logger.get_appender("test_console");
    if (!console) throw std::runtime_error("Should find console appender");

    auto* memory = logger.get_appender("test_memory");
    if (!memory) throw std::runtime_error("Should find memory appender");

    logger.remove_appender("test_console");
    logger.remove_appender("test_memory");

    auto* removed = logger.get_appender("test_console");
    if (removed) throw std::runtime_error("Appender should be removed");
  });

  suite.run_test("Flush All Appenders", []() {
    auto& logger = Logger::instance();

    logger.add_console_appender("flush_test");

    // Should not throw
    logger.flush();

    logger.remove_appender("flush_test");
  });

  // Logging macros tests
  suite.run_test("Logging Macros", []() {
    // Should not throw
    LOG_DEBUG("test", "Debug via macro");
    LOG_INFO("test", "Info via macro");
    LOG_WARN("test", "Warning via macro");
    LOG_ERROR("test", "Error via macro");
  });

  suite.print_summary();
  return suite.all_passed() ? 0 : 1;
}
