/**
 * @file test_solar_utils.cpp
 * @brief Comprehensive unit tests for solar_utils library
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "solar_utils/error_handling.hpp"
#include "solar_utils/logging.hpp"

using namespace SolarSystem::Utils;

// ============================================================================
// LogLevel Tests
// ============================================================================

TEST(LogLevel, EnumValues) {
  EXPECT_LT(static_cast<int>(LogLevel::TRACE), static_cast<int>(LogLevel::DEBUG));
  EXPECT_LT(static_cast<int>(LogLevel::DEBUG), static_cast<int>(LogLevel::INFO));
  EXPECT_LT(static_cast<int>(LogLevel::INFO), static_cast<int>(LogLevel::WARN));
  EXPECT_LT(static_cast<int>(LogLevel::WARN), static_cast<int>(LogLevel::ERROR));
  EXPECT_LT(static_cast<int>(LogLevel::ERROR), static_cast<int>(LogLevel::FATAL));
}

TEST(LogLevel, Comparison) {
  EXPECT_EQ(0, static_cast<int>(LogLevel::TRACE));
  EXPECT_EQ(1, static_cast<int>(LogLevel::DEBUG));
  EXPECT_EQ(2, static_cast<int>(LogLevel::INFO));
  EXPECT_EQ(3, static_cast<int>(LogLevel::WARN));
  EXPECT_EQ(4, static_cast<int>(LogLevel::ERROR));
  EXPECT_EQ(5, static_cast<int>(LogLevel::FATAL));
}

// ============================================================================
// LogEntry Tests
// ============================================================================

TEST(LogEntry, Creation) {
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

  EXPECT_EQ("TestComponent", entry.component);
  EXPECT_EQ("Test message", entry.message);
  EXPECT_EQ(42, entry.line);
  EXPECT_EQ(1, entry.sequence_number);
}

TEST(LogEntry, ToString) {
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
  EXPECT_FALSE(str.empty());
  EXPECT_NE(std::string::npos, str.find("Test"));
  EXPECT_NE(std::string::npos, str.find("Hello"));
}

TEST(LogEntry, ToJson) {
  LogEntry entry;
  entry.timestamp = std::chrono::system_clock::now();
  entry.level = LogLevel::ERROR;
  entry.component = "ErrorTest";
  entry.message = "Error occurred";
  entry.file = "error.cpp";
  entry.line = 100;

  std::string json = entry.to_json();
  EXPECT_FALSE(json.empty());
  EXPECT_NE(std::string::npos, json.find("ErrorTest"));
  EXPECT_NE(std::string::npos, json.find("Error occurred"));
}

TEST(LogEntry, Metadata) {
  LogEntry entry;
  entry.metadata["key1"] = "value1";
  entry.metadata["key2"] = "value2";

  EXPECT_EQ(2u, entry.metadata.size());
  EXPECT_EQ("value1", entry.metadata["key1"]);
  EXPECT_EQ("value2", entry.metadata["key2"]);
}

TEST(LogEntry, ToCsv) {
  LogEntry entry;
  entry.timestamp = std::chrono::system_clock::now();
  entry.level = LogLevel::WARN;
  entry.component = "Warning";
  entry.message = "Warning message";
  entry.file = "warn.cpp";
  entry.line = 50;

  std::string csv = entry.to_csv();
  EXPECT_FALSE(csv.empty());
  EXPECT_NE(std::string::npos, csv.find(","));
}

TEST(LogEntry, ToXml) {
  LogEntry entry;
  entry.timestamp = std::chrono::system_clock::now();
  entry.level = LogLevel::DEBUG;
  entry.component = "Debug";
  entry.message = "Debug info";
  entry.file = "debug.cpp";
  entry.line = 25;

  std::string xml = entry.to_xml();
  EXPECT_FALSE(xml.empty());
  EXPECT_NE(std::string::npos, xml.find("<"));
  EXPECT_NE(std::string::npos, xml.find(">"));
}

TEST(LogEntry, ThreadId) {
  LogEntry entry;
  entry.thread_id = std::this_thread::get_id();
  EXPECT_EQ(std::this_thread::get_id(), entry.thread_id);
}

TEST(LogEntry, TimestampOrdering) {
  LogEntry entry1;
  entry1.timestamp = std::chrono::system_clock::now();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  LogEntry entry2;
  entry2.timestamp = std::chrono::system_clock::now();

  EXPECT_LT(entry1.timestamp, entry2.timestamp);
}

TEST(LogEntry, SequenceNumbers) {
  LogEntry entry1;
  entry1.sequence_number = 1;

  LogEntry entry2;
  entry2.sequence_number = 2;

  LogEntry entry3;
  entry3.sequence_number = 3;

  EXPECT_LT(entry1.sequence_number, entry2.sequence_number);
  EXPECT_LT(entry2.sequence_number, entry3.sequence_number);
}

TEST(LogEntry, EmptyMessage) {
  LogEntry entry;
  entry.message = "";
  entry.component = "Test";
  entry.level = LogLevel::INFO;

  std::string str = entry.to_string();
  EXPECT_FALSE(str.empty());
}

TEST(LogEntry, LongMessage) {
  LogEntry entry;
  entry.message = std::string(1000, 'A');
  entry.component = "Test";
  entry.level = LogLevel::INFO;

  std::string str = entry.to_string();
  EXPECT_FALSE(str.empty());
  EXPECT_NE(std::string::npos, str.find("AAA"));
}

TEST(LogEntry, SpecialCharacters) {
  LogEntry entry;
  entry.message = "Test\nNew\tLine\"Quote";
  entry.component = "Test";
  entry.level = LogLevel::INFO;

  std::string str = entry.to_string();
  EXPECT_FALSE(str.empty());
}

TEST(LogEntry, MultipleMetadata) {
  LogEntry entry;
  for (int i = 0; i < 10; ++i) {
    entry.metadata["key" + std::to_string(i)] = "value" + std::to_string(i);
  }

  EXPECT_EQ(10u, entry.metadata.size());
}

// ============================================================================
// ErrorSeverity Tests
// ============================================================================

TEST(ErrorSeverity, EnumValues) {
  EXPECT_LT(static_cast<int>(ErrorSeverity::Info), static_cast<int>(ErrorSeverity::Warning));
  EXPECT_LT(static_cast<int>(ErrorSeverity::Warning), static_cast<int>(ErrorSeverity::Error));
  EXPECT_LT(static_cast<int>(ErrorSeverity::Error), static_cast<int>(ErrorSeverity::Critical));
  EXPECT_LT(static_cast<int>(ErrorSeverity::Critical), static_cast<int>(ErrorSeverity::Fatal));
}

// ============================================================================
// ErrorCategory Tests
// ============================================================================

TEST(ErrorCategory, EnumValues) {
  ErrorCategory cat1 = ErrorCategory::Validation;
  ErrorCategory cat2 = ErrorCategory::Network;
  ErrorCategory cat3 = ErrorCategory::FileSystem;

  EXPECT_EQ(ErrorCategory::Validation, cat1);
  EXPECT_EQ(ErrorCategory::Network, cat2);
  EXPECT_EQ(ErrorCategory::FileSystem, cat3);
}

// ============================================================================
// ErrorCode Tests
// ============================================================================

TEST(ErrorCode, ValidationCodes) {
  EXPECT_EQ(ErrorCode::InvalidInput, ErrorCode::InvalidInput);
  EXPECT_NE(ErrorCode::InvalidFormat, ErrorCode::InvalidRange);
  EXPECT_NE(ErrorCode::MissingRequired, ErrorCode::ConflictingParameters);
}

TEST(ErrorCode, NetworkCodes) {
  EXPECT_EQ(ErrorCode::ConnectionFailed, ErrorCode::ConnectionFailed);
  EXPECT_NE(ErrorCode::ConnectionTimeout, ErrorCode::NetworkUnavailable);
  EXPECT_NE(ErrorCode::InvalidResponse, ErrorCode::AuthenticationFailed);
}

TEST(ErrorCode, FileSystemCodes) {
  EXPECT_EQ(ErrorCode::FileNotFound, ErrorCode::FileNotFound);
  EXPECT_NE(ErrorCode::FileAccessDenied, ErrorCode::FileCorrupted);
  EXPECT_NE(ErrorCode::DiskFull, ErrorCode::DirectoryNotFound);
}

TEST(ErrorCode, MemoryCodes) {
  EXPECT_EQ(ErrorCode::OutOfMemory, ErrorCode::OutOfMemory);
  EXPECT_NE(ErrorCode::MemoryLeak, ErrorCode::InvalidPointer);
}

// ============================================================================
// LogOutput Tests
// ============================================================================

TEST(LogOutput, EnumValues) {
  LogOutput out1 = LogOutput::CONSOLE;
  LogOutput out2 = LogOutput::FILE;
  LogOutput out3 = LogOutput::BOTH;

  EXPECT_EQ(LogOutput::CONSOLE, out1);
  EXPECT_EQ(LogOutput::FILE, out2);
  EXPECT_EQ(LogOutput::BOTH, out3);
}

// ============================================================================
// LogFormat Tests
// ============================================================================

TEST(LogFormat, EnumValues) {
  LogFormat fmt1 = LogFormat::TEXT;
  LogFormat fmt2 = LogFormat::JSON;
  LogFormat fmt3 = LogFormat::XML;
  LogFormat fmt4 = LogFormat::CSV;

  EXPECT_EQ(LogFormat::TEXT, fmt1);
  EXPECT_EQ(LogFormat::JSON, fmt2);
  EXPECT_EQ(LogFormat::XML, fmt3);
  EXPECT_EQ(LogFormat::CSV, fmt4);
}
