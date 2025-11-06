/**
 * @file test_solar_utils.cpp
 * @brief Comprehensive unit tests for solar_utils library
 */

#include "../utils/test_framework.h"
#include "solar_utils/logging.hpp"
#include "solar_utils/error_handling.hpp"
#include <chrono>
#include <thread>

using namespace SolarSystem::Utils;

int main() {
  TestSuite suite("Solar Utils Library Tests");

  // Test LogLevel enum
  suite.run_test("LogLevel Enum Values", []() {
    ASSERT_TRUE(LogLevel::TRACE < LogLevel::DEBUG);
    ASSERT_TRUE(LogLevel::DEBUG < LogLevel::INFO);
    ASSERT_TRUE(LogLevel::INFO < LogLevel::WARN);
    ASSERT_TRUE(LogLevel::WARN < LogLevel::ERROR);
    ASSERT_TRUE(LogLevel::ERROR < LogLevel::FATAL);
  });

  // Test LogEntry creation
  suite.run_test("LogEntry Creation", []() {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "TestComponent";
    entry.message = "Test message";
    entry.file = "test.cpp";
    entry.line = 42;
    entry.function = "test_function";
    entry.thread_id = std::this_thread::get_id();
    entry.sequence_number = 1;

    ASSERT_EQ("TestComponent", entry.component);
    ASSERT_EQ("Test message", entry.message);
    ASSERT_EQ(42, entry.line);
    ASSERT_EQ(1, entry.sequence_number);
  });

  // Test LogEntry to_string
  suite.run_test("LogEntry to_string", []() {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::INFO;
    entry.component = "Test";
    entry.message = "Hello";
    entry.file = "test.cpp";
    entry.line = 10;
    entry.function = "main";
    entry.thread_id = std::this_thread::get_id();

    std::string str = entry.to_string();
    ASSERT_FALSE(str.empty());
    ASSERT_TRUE(str.find("Test") != std::string::npos);
    ASSERT_TRUE(str.find("Hello") != std::string::npos);
  });

  // Test LogEntry to_json
  suite.run_test("LogEntry to_json", []() {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::ERROR;
    entry.component = "ErrorTest";
    entry.message = "Error occurred";
    entry.file = "error.cpp";
    entry.line = 100;

    std::string json = entry.to_json();
    ASSERT_FALSE(json.empty());
    ASSERT_TRUE(json.find("ErrorTest") != std::string::npos);
    ASSERT_TRUE(json.find("Error occurred") != std::string::npos);
  });

  // Test LogEntry metadata
  suite.run_test("LogEntry Metadata", []() {
    LogEntry entry;
    entry.metadata["key1"] = "value1";
    entry.metadata["key2"] = "value2";

    ASSERT_EQ(2, entry.metadata.size());
    ASSERT_EQ("value1", entry.metadata["key1"]);
    ASSERT_EQ("value2", entry.metadata["key2"]);
  });

  // Test ErrorSeverity enum
  suite.run_test("ErrorSeverity Enum Values", []() {
    ASSERT_TRUE(ErrorSeverity::Info < ErrorSeverity::Warning);
    ASSERT_TRUE(ErrorSeverity::Warning < ErrorSeverity::Error);
    ASSERT_TRUE(ErrorSeverity::Error < ErrorSeverity::Critical);
    ASSERT_TRUE(ErrorSeverity::Critical < ErrorSeverity::Fatal);
  });

  // Test ErrorCategory enum
  suite.run_test("ErrorCategory Enum", []() {
    ErrorCategory cat1 = ErrorCategory::Validation;
    ErrorCategory cat2 = ErrorCategory::Network;
    ErrorCategory cat3 = ErrorCategory::FileSystem;

    ASSERT_TRUE(cat1 == ErrorCategory::Validation);
    ASSERT_TRUE(cat2 == ErrorCategory::Network);
    ASSERT_TRUE(cat3 == ErrorCategory::FileSystem);
  });

  // Test ErrorCode enum values
  suite.run_test("ErrorCode Validation Codes", []() {
    ASSERT_TRUE(ErrorCode::InvalidInput == ErrorCode::InvalidInput);
    ASSERT_TRUE(ErrorCode::InvalidFormat != ErrorCode::InvalidRange);
    ASSERT_TRUE(ErrorCode::MissingRequired != ErrorCode::ConflictingParameters);
  });

  suite.run_test("ErrorCode Network Codes", []() {
    ASSERT_TRUE(ErrorCode::ConnectionFailed == ErrorCode::ConnectionFailed);
    ASSERT_TRUE(ErrorCode::ConnectionTimeout != ErrorCode::NetworkUnavailable);
    ASSERT_TRUE(ErrorCode::InvalidResponse != ErrorCode::AuthenticationFailed);
  });

  suite.run_test("ErrorCode FileSystem Codes", []() {
    ASSERT_TRUE(ErrorCode::FileNotFound == ErrorCode::FileNotFound);
    ASSERT_TRUE(ErrorCode::FileAccessDenied != ErrorCode::FileCorrupted);
    ASSERT_TRUE(ErrorCode::DiskFull != ErrorCode::DirectoryNotFound);
  });

  suite.run_test("ErrorCode Memory Codes", []() {
    ASSERT_TRUE(ErrorCode::OutOfMemory == ErrorCode::OutOfMemory);
    ASSERT_TRUE(ErrorCode::MemoryLeak != ErrorCode::InvalidPointer);
  });

  // Test LogOutput enum
  suite.run_test("LogOutput Enum", []() {
    LogOutput out1 = LogOutput::CONSOLE;
    LogOutput out2 = LogOutput::FILE;
    LogOutput out3 = LogOutput::BOTH;

    ASSERT_TRUE(out1 == LogOutput::CONSOLE);
    ASSERT_TRUE(out2 == LogOutput::FILE);
    ASSERT_TRUE(out3 == LogOutput::BOTH);
  });

  // Test LogFormat enum
  suite.run_test("LogFormat Enum", []() {
    LogFormat fmt1 = LogFormat::TEXT;
    LogFormat fmt2 = LogFormat::JSON;
    LogFormat fmt3 = LogFormat::XML;
    LogFormat fmt4 = LogFormat::CSV;

    ASSERT_TRUE(fmt1 == LogFormat::TEXT);
    ASSERT_TRUE(fmt2 == LogFormat::JSON);
    ASSERT_TRUE(fmt3 == LogFormat::XML);
    ASSERT_TRUE(fmt4 == LogFormat::CSV);
  });

  // Test LogEntry CSV format
  suite.run_test("LogEntry to_csv", []() {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::WARN;
    entry.component = "Warning";
    entry.message = "Warning message";
    entry.file = "warn.cpp";
    entry.line = 50;

    std::string csv = entry.to_csv();
    ASSERT_FALSE(csv.empty());
    // CSV should contain comma-separated values
    ASSERT_TRUE(csv.find(",") != std::string::npos);
  });

  // Test LogEntry XML format
  suite.run_test("LogEntry to_xml", []() {
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::DEBUG;
    entry.component = "Debug";
    entry.message = "Debug info";
    entry.file = "debug.cpp";
    entry.line = 25;

    std::string xml = entry.to_xml();
    ASSERT_FALSE(xml.empty());
    // XML should contain tags
    ASSERT_TRUE(xml.find("<") != std::string::npos);
    ASSERT_TRUE(xml.find(">") != std::string::npos);
  });

  // Test thread ID capture
  suite.run_test("LogEntry Thread ID", []() {
    LogEntry entry;
    entry.thread_id = std::this_thread::get_id();

    // Thread ID should be captured
    ASSERT_TRUE(entry.thread_id == std::this_thread::get_id());
  });

  // Test timestamp ordering
  suite.run_test("LogEntry Timestamp Ordering", []() {
    LogEntry entry1;
    entry1.timestamp = std::chrono::system_clock::now();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    LogEntry entry2;
    entry2.timestamp = std::chrono::system_clock::now();

    ASSERT_TRUE(entry1.timestamp < entry2.timestamp);
  });

  // Test sequence numbers
  suite.run_test("LogEntry Sequence Numbers", []() {
    LogEntry entry1;
    entry1.sequence_number = 1;

    LogEntry entry2;
    entry2.sequence_number = 2;

    LogEntry entry3;
    entry3.sequence_number = 3;

    ASSERT_TRUE(entry1.sequence_number < entry2.sequence_number);
    ASSERT_TRUE(entry2.sequence_number < entry3.sequence_number);
  });

  // Test empty message handling
  suite.run_test("LogEntry Empty Message", []() {
    LogEntry entry;
    entry.message = "";
    entry.component = "Test";
    entry.level = LogLevel::INFO;

    std::string str = entry.to_string();
    ASSERT_FALSE(str.empty());  // Should still produce output
  });

  // Test long message handling
  suite.run_test("LogEntry Long Message", []() {
    LogEntry entry;
    entry.message = std::string(1000, 'A');  // 1000 character message
    entry.component = "Test";
    entry.level = LogLevel::INFO;

    std::string str = entry.to_string();
    ASSERT_FALSE(str.empty());
    ASSERT_TRUE(str.find("AAA") != std::string::npos);
  });

  // Test special characters in message
  suite.run_test("LogEntry Special Characters", []() {
    LogEntry entry;
    entry.message = "Test\nNew\tLine\"Quote";
    entry.component = "Test";
    entry.level = LogLevel::INFO;

    std::string str = entry.to_string();
    ASSERT_FALSE(str.empty());
  });

  // Test multiple metadata entries
  suite.run_test("LogEntry Multiple Metadata", []() {
    LogEntry entry;
    for (int i = 0; i < 10; ++i) {
      entry.metadata["key" + std::to_string(i)] = "value" + std::to_string(i);
    }

    ASSERT_EQ(10, entry.metadata.size());
  });

  // Test LogLevel comparison
  suite.run_test("LogLevel Comparison", []() {
    ASSERT_TRUE(static_cast<int>(LogLevel::TRACE) == 0);
    ASSERT_TRUE(static_cast<int>(LogLevel::DEBUG) == 1);
    ASSERT_TRUE(static_cast<int>(LogLevel::INFO) == 2);
    ASSERT_TRUE(static_cast<int>(LogLevel::WARN) == 3);
    ASSERT_TRUE(static_cast<int>(LogLevel::ERROR) == 4);
    ASSERT_TRUE(static_cast<int>(LogLevel::FATAL) == 5);
  });

  return suite.all_passed() ? 0 : suite.get_failed_count();
}
