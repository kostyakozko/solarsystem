/**
 * @file enhanced_test_reporter.cpp
 * @brief Implementation of enhanced test reporter with robust file handling
 */

#include "solar_test/reporters/enhanced_test_reporter.hpp"

#include <algorithm>
#include <cstdarg>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace SolarSystem::Testing {

// EnhancedTestReporter implementation
EnhancedTestReporter::EnhancedTestReporter(const EnhancedConfiguration& config) : config_(config) {
  file_handler_ = std::make_unique<Utils::RobustFileHandler>(config_.file_config);

  if (config_.use_streaming_output) {
    stream_buffer_ = std::make_unique<StreamBuffer>(config_.buffer_size);
  }

  initialize_output();
}

EnhancedTestReporter::~EnhancedTestReporter() {
  try {
    cleanup_output();
  } catch (...) {
    // Suppress exceptions in destructor
  }
}

bool EnhancedTestReporter::write_content(const std::string& content) {
  auto start_time = std::chrono::steady_clock::now();
  bool success = false;

  try {
    if (!ensure_output_ready()) {
      handle_write_error("Output stream not ready");
      return false;
    }

    if (config_.use_streaming_output && stream_buffer_) {
      stream_buffer_->write(content);
      if (stream_buffer_->needs_flush() || config_.auto_flush) {
        stream_buffer_->flush(output_stream_.get());
      }
      success = output_stream_->good();
    } else {
      *output_stream_ << content;
      if (config_.auto_flush) {
        output_stream_->flush();
      }
      success = output_stream_->good();
    }

    if (!success) {
      handle_write_error("Stream write failed");
    }

  } catch (const std::exception& e) {
    handle_write_error("Exception during write: " + std::string(e.what()));
  }

  auto end_time = std::chrono::steady_clock::now();
  auto write_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  update_statistics(success, content.size(), write_time);

  return success;
}

bool EnhancedTestReporter::append_content(const std::string& content) {
  return write_content(content);
}

bool EnhancedTestReporter::flush_output() {
  if (!output_stream_) {
    return false;
  }

  try {
    if (stream_buffer_) {
      stream_buffer_->flush(output_stream_.get());
    }

    output_stream_->flush();
    return output_stream_->good();
  } catch (...) {
    return false;
  }
}

bool EnhancedTestReporter::ensure_output_ready() {
  if (output_ready_ && output_stream_ && output_stream_->is_open()) {
    return true;
  }

  if (!recovery_attempted_) {
    attempt_error_recovery();
    recovery_attempted_ = true;
  }

  return output_ready_ && output_stream_ && output_stream_->is_open();
}

Utils::RobustFileStream* EnhancedTestReporter::get_output_stream() { return output_stream_.get(); }

bool EnhancedTestReporter::is_output_available() const {
  return output_ready_ && output_stream_ && output_stream_->is_open();
}

void EnhancedTestReporter::handle_write_error(const std::string& error_message) {
  stats_.failed_writes++;

  if (config_.log_file_operations) {
    log_operation("write_error", false, error_message);
  }

  if (config_.continue_on_write_errors) {
    // Try to recover
    if (!recovery_attempted_) {
      attempt_error_recovery();
    }
  } else {
    // Propagate error to derived class
    on_output_stream_error(error_message);
  }
}

void EnhancedTestReporter::attempt_error_recovery() {
  recovery_attempted_ = true;

  // Try to reinitialize output
  cleanup_output();

  // Try fallback locations
  auto fallback_locations = get_fallback_locations();
  for (const auto& location : fallback_locations) {
    config_.output_file = location;
    if (initialize_output()) {
      stats_.using_fallback_location = true;
      stats_.current_output_file = location;
      on_fallback_location_used(location);
      break;
    }
  }
}

std::vector<std::string> EnhancedTestReporter::get_fallback_locations() const {
  std::vector<std::string> locations;

  // Generate fallback file names
  std::string base_name = config_.output_file;
  std::string extension;

  size_t dot_pos = base_name.find_last_of('.');
  if (dot_pos != std::string::npos) {
    extension = base_name.substr(dot_pos);
    base_name = base_name.substr(0, dot_pos);
  }

  // Add timestamp-based fallbacks
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestamp;
  timestamp << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");

  locations.push_back(base_name + "_" + timestamp.str() + extension);
  locations.push_back("./test_output/" + base_name + extension);
  locations.push_back("/tmp/solar_test_" + base_name + extension);

  return locations;
}

void EnhancedTestReporter::write_with_error_handling(const std::string& content) {
  if (!write_content(content)) {
    // Handle error based on configuration
    if (!config_.continue_on_write_errors) {
      throw std::runtime_error("Failed to write content: " + content.substr(0, 100));
    }
  }
}

void EnhancedTestReporter::write_formatted(const char* format, ...) {
  va_list args;
  va_start(args, format);

  // Calculate required buffer size
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(nullptr, 0, format, args_copy);
  va_end(args_copy);

  if (size > 0) {
    std::vector<char> buffer(size + 1);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    write_with_error_handling(std::string(buffer.data()));
  }

  va_end(args);
}

void EnhancedTestReporter::write_line(const std::string& line) {
  write_with_error_handling(line + "\n");
}

void EnhancedTestReporter::write_separator(char separator, size_t length) {
  write_with_error_handling(std::string(length, separator) + "\n");
}

bool EnhancedTestReporter::initialize_output() {
  try {
    output_stream_ = file_handler_->create_output_stream(config_.output_file);

    if (output_stream_ && output_stream_->is_open()) {
      output_ready_ = true;
      stats_.current_output_file = config_.output_file;
      on_output_stream_ready();

      if (config_.log_file_operations) {
        log_operation("initialize_output", true, config_.output_file);
      }

      return true;
    }
  } catch (const std::exception& e) {
    if (config_.log_file_operations) {
      log_operation("initialize_output", false, e.what());
    }
  }

  output_ready_ = false;
  return false;
}

void EnhancedTestReporter::cleanup_output() {
  if (stream_buffer_) {
    try {
      stream_buffer_->flush(output_stream_.get());
    } catch (...) {
      // Ignore flush errors during cleanup
    }
  }

  if (output_stream_) {
    try {
      output_stream_->close();
    } catch (...) {
      // Ignore close errors during cleanup
    }
    output_stream_.reset();
  }

  output_ready_ = false;
}

void EnhancedTestReporter::update_statistics(bool success, size_t bytes_written,
                                             std::chrono::milliseconds write_time) {
  stats_.write_operations++;
  if (success) {
    stats_.bytes_written += bytes_written;
  } else {
    stats_.failed_writes++;
  }
  stats_.total_write_time += write_time;
}

void EnhancedTestReporter::update_configuration(const EnhancedConfiguration& config) {
  config_ = config;

  // Update file handler configuration
  if (file_handler_) {
    file_handler_->set_config(config_.file_config);
  }

  // Reinitialize if output file changed
  if (output_stream_ && config_.output_file != stats_.current_output_file) {
    cleanup_output();
    initialize_output();
  }
}

void EnhancedTestReporter::log_operation(const std::string& operation, bool success,
                                         const std::string& details) {
  if (config_.log_file_operations) {
    std::cerr << "[EnhancedTestReporter] " << operation << ": " << (success ? "SUCCESS" : "FAILED");
    if (!details.empty()) {
      std::cerr << " - " << details;
    }
    std::cerr << std::endl;
  }
}

// StreamBuffer implementation
EnhancedTestReporter::StreamBuffer::StreamBuffer(size_t buffer_size) : max_size_(buffer_size) {
  buffer_.reserve(buffer_size);
}

EnhancedTestReporter::StreamBuffer::~StreamBuffer() = default;

void EnhancedTestReporter::StreamBuffer::write(const std::string& content) { buffer_ += content; }

void EnhancedTestReporter::StreamBuffer::flush(Utils::RobustFileStream* stream) {
  if (!buffer_.empty() && stream) {
    *stream << buffer_;
    stream->flush();
    buffer_.clear();
  }
}

bool EnhancedTestReporter::StreamBuffer::needs_flush() const { return buffer_.size() >= max_size_; }

// EnhancedXmlReporter implementation
EnhancedXmlReporter::EnhancedXmlReporter(const XmlConfiguration& config)
    : EnhancedTestReporter(config.base_config), xml_config_(config) {}

EnhancedXmlReporter::EnhancedXmlReporter(const std::string& output_file)
    : EnhancedTestReporter(EnhancedConfiguration{}) {
  XmlConfiguration config;
  config.base_config.output_file = output_file;
  xml_config_ = config;

  // Update base configuration
  update_configuration(config.base_config);
}

void EnhancedXmlReporter::on_suite_started(const std::string& suite_name, size_t total_tests) {
  current_suite_name_ = suite_name;
  current_suite_results_.clear();
  current_suite_results_.reserve(total_tests);
  suite_start_time_ = std::chrono::steady_clock::now();

  if (!xml_header_written_) {
    write_xml_header();
    xml_header_written_ = true;
  }
}

void EnhancedXmlReporter::on_suite_finished(const TestSuiteResult& result) {
  write_testsuite_element(result);
  write_xml_footer();
  flush_output();
}

void EnhancedXmlReporter::on_test_started(const std::string& test_name) {
  // XML reporter doesn't need to do anything when test starts
  (void)test_name;
}

void EnhancedXmlReporter::on_test_finished(const TestResult& result) {
  current_suite_results_.push_back(result);
}

void EnhancedXmlReporter::on_progress(const std::string& message, double percentage) {
  // XML reporter doesn't output progress information
  (void)message;
  (void)percentage;
}

void EnhancedXmlReporter::on_error(const std::string& error_message) {
  write_with_error_handling("<!-- ERROR: " + xml_escape(error_message) + " -->\n");
}

void EnhancedXmlReporter::on_output_stream_ready() {
  // Reset state when output stream is ready
  xml_header_written_ = false;
}

void EnhancedXmlReporter::on_fallback_location_used(const std::string& new_location) {
  std::cerr << "[EnhancedXmlReporter] Using fallback location: " << new_location << std::endl;
}

void EnhancedXmlReporter::write_xml_header() {
  write_line("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  write_line("<testsuites>");
}

void EnhancedXmlReporter::write_xml_footer() { write_line("</testsuites>"); }

void EnhancedXmlReporter::write_testsuite_element(const TestSuiteResult& result) {
  auto suite_end_time = std::chrono::steady_clock::now();
  auto suite_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(suite_end_time - suite_start_time_);

  write_formatted(
      "  <testsuite name=\"%s\" tests=\"%zu\" failures=\"%zu\" errors=\"0\" skipped=\"%zu\" "
      "time=\"%.3f\">\n",
      xml_escape(result.suite_name).c_str(), result.test_results.size(), result.failed_count,
      result.skipped_count, static_cast<double>(suite_duration.count()) / 1000.0);

  for (const auto& test_result : result.test_results) {
    write_testcase_element(test_result);
  }

  write_line("  </testsuite>");
}

void EnhancedXmlReporter::write_testcase_element(const TestResult& result) {
  write_formatted("    <testcase name=\"%s\" classname=\"%s\" time=\"%.3f\"",
                  xml_escape(result.test_name).c_str(), xml_escape(current_suite_name_).c_str(),
                  static_cast<double>(result.execution_time.count()) / 1000.0);

  if (result.status == TestResult::Status::Failed || result.status == TestResult::Status::Error ||
      result.status == TestResult::Status::Timeout) {
    write_line(">");
    write_formatted(
        "      <failure type=\"%s\" message=\"%s\"/>\n",
        result.status == TestResult::Status::Timeout ? "TestTimeout" : "AssertionFailure",
        xml_escape(result.error_message).c_str());
    write_line("    </testcase>");
  } else if (result.status == TestResult::Status::Skipped) {
    write_line(">");
    write_line("      <skipped/>");
    write_line("    </testcase>");
  } else {
    write_line("/>");
  }
}

std::string EnhancedXmlReporter::xml_escape(const std::string& text) const {
  std::string escaped;
  escaped.reserve(text.length() * 1.2);

  for (char c : text) {
    switch (c) {
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '&':
        escaped += "&amp;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&apos;";
        break;
      default:
        escaped += c;
        break;
    }
  }

  return escaped;
}

// EnhancedTestReporterFactory implementation
std::unique_ptr<EnhancedTestReporter> EnhancedTestReporterFactory::create_xml_reporter(
    const std::string& output_file, const EnhancedTestReporter::EnhancedConfiguration& config) {
  EnhancedXmlReporter::XmlConfiguration xml_config;
  xml_config.base_config = config;
  xml_config.base_config.output_file = output_file;

  return std::make_unique<EnhancedXmlReporter>(xml_config);
}

EnhancedTestReporter::EnhancedConfiguration
EnhancedTestReporterFactory::create_high_reliability_config() {
  EnhancedTestReporter::EnhancedConfiguration config;

  // High reliability file operations
  config.file_config.max_retries = 5;
  config.file_config.retry_delay = std::chrono::milliseconds(200);
  config.file_config.retry_backoff_multiplier = 2.0;
  config.file_config.use_atomic_writes = true;
  config.file_config.backup_existing_files = true;
  config.file_config.create_directories = true;

  // Multiple fallback directories
  config.file_config.fallback_directories = {"./test_output", "./backup_test_output",
                                             "/tmp/solar_system_tests", "."};

  // Robust output configuration
  config.use_streaming_output = true;
  config.buffer_size = 4096;
  config.auto_flush = true;
  config.continue_on_write_errors = true;
  config.log_file_operations = true;

  return config;
}

}  // namespace SolarSystem::Testing
