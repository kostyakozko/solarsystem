/**
 * @file enhanced_test_reporter.hpp
 * @brief Enhanced test reporter base class with robust file handling (Task 15)
 *
 * Implements requirements 7.1, 7.3, 7.4, and 7.5:
 * - Handle file system errors gracefully with retry mechanisms
 * - Provide fallback mechanisms or clear error reporting
 * - Manage memory efficiently and handle streaming output
 * - Attempt alternative locations or provide clear error messages
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../utils/robust_file_handler.hpp"
#include "test_reporter.hpp"

namespace SolarSystem::Testing {

/**
 * @brief Enhanced test reporter with robust file handling capabilities
 */
class EnhancedTestReporter : public TestReporter {
 public:
  /**
   * @brief Configuration for enhanced reporting
   */
  struct EnhancedConfiguration {
    // File handling configuration
    Utils::FileOperationConfig file_config;

    // Output configuration
    std::string output_file;
    bool use_streaming_output = true;
    size_t buffer_size = 8192;
    bool auto_flush = true;

    // Error handling configuration
    bool continue_on_write_errors = true;
    bool log_file_operations = false;
    std::vector<std::string> notification_emails;  // For critical failures

    // Performance configuration
    bool use_memory_mapping = false;             // For large outputs
    size_t memory_threshold = 10 * 1024 * 1024;  // 10MB
  };

  explicit EnhancedTestReporter(const EnhancedConfiguration& config);
  virtual ~EnhancedTestReporter();

  // Enhanced file operations
  bool write_content(const std::string& content);
  bool append_content(const std::string& content);
  bool flush_output();
  bool ensure_output_ready();

  // Stream management
  Utils::RobustFileStream* get_output_stream();
  bool is_output_available() const;

  // Error handling and recovery
  virtual void handle_write_error(const std::string& error_message);
  virtual void attempt_error_recovery();
  virtual std::vector<std::string> get_fallback_locations() const;

  // Configuration management
  void update_configuration(const EnhancedConfiguration& config);
  const EnhancedConfiguration& configuration() const { return config_; }

  // Statistics and monitoring
  struct OutputStats {
    size_t bytes_written = 0;
    size_t write_operations = 0;
    size_t failed_writes = 0;
    size_t retry_attempts = 0;
    std::chrono::milliseconds total_write_time{0};
    std::string current_output_file;
    bool using_fallback_location = false;
  };

  const OutputStats& output_statistics() const { return stats_; }
  void reset_statistics() { stats_ = OutputStats{}; }

 protected:
  // Template method for derived classes
  virtual void on_output_stream_ready() {}
  virtual void on_output_stream_error(const std::string& error) { (void)error; }
  virtual void on_fallback_location_used(const std::string& new_location) { (void)new_location; }

  // Utility methods for derived classes
  void write_with_error_handling(const std::string& content);
  void write_formatted(const char* format, ...);
  void write_line(const std::string& line);
  void write_separator(char separator = '-', size_t length = 80);

  // Memory management for large outputs
  void check_memory_usage();
  void optimize_for_large_output();

  // Streaming utilities
  class StreamBuffer {
   public:
    explicit StreamBuffer(size_t buffer_size);
    ~StreamBuffer();

    void write(const std::string& content);
    void flush(Utils::RobustFileStream* stream);
    bool needs_flush() const;
    size_t size() const { return buffer_.size(); }
    void clear() { buffer_.clear(); }

   private:
    std::string buffer_;
    size_t max_size_;
  };

 private:
  EnhancedConfiguration config_;
  std::unique_ptr<Utils::RobustFileHandler> file_handler_;
  std::unique_ptr<Utils::RobustFileStream> output_stream_;
  std::unique_ptr<StreamBuffer> stream_buffer_;
  OutputStats stats_;
  bool output_ready_ = false;
  bool recovery_attempted_ = false;

  // Internal methods
  bool initialize_output();
  void cleanup_output();
  void update_statistics(bool success, size_t bytes_written, std::chrono::milliseconds write_time);
  void log_operation(const std::string& operation, bool success, const std::string& details = "");
};

/**
 * @brief Enhanced XML reporter with robust file handling
 */
class EnhancedXmlReporter : public EnhancedTestReporter {
 public:
  struct XmlConfiguration {
    EnhancedConfiguration base_config;
    bool pretty_print = true;
    bool include_system_out = true;
    bool include_system_err = true;
    bool validate_xml = false;
    std::string xml_schema_path;
  };

  explicit EnhancedXmlReporter(const XmlConfiguration& config);
  explicit EnhancedXmlReporter(const std::string& output_file);

  // TestReporter interface implementation
  void on_suite_started(const std::string& suite_name, size_t total_tests) override;
  void on_suite_finished(const TestSuiteResult& result) override;
  void on_test_started(const std::string& test_name) override;
  void on_test_finished(const TestResult& result) override;
  void on_progress(const std::string& message, double percentage) override;
  void on_error(const std::string& error_message) override;

 protected:
  void on_output_stream_ready() override;
  void on_fallback_location_used(const std::string& new_location) override;

 private:
  XmlConfiguration xml_config_;
  std::string current_suite_name_;
  std::vector<TestResult> current_suite_results_;
  std::chrono::steady_clock::time_point suite_start_time_;
  bool xml_header_written_ = false;

  // XML generation methods
  void write_xml_header();
  void write_xml_footer();
  void write_testsuite_element(const TestSuiteResult& result);
  void write_testcase_element(const TestResult& result);
  std::string xml_escape(const std::string& text) const;
  bool validate_xml_output() const;
};

/**
 * @brief Enhanced JSON reporter with robust file handling
 */
class EnhancedJsonReporter : public EnhancedTestReporter {
 public:
  struct JsonConfiguration {
    EnhancedConfiguration base_config;
    bool pretty_print = true;
    bool include_metadata = true;
    bool compress_output = false;
    int indent_size = 2;
  };

  explicit EnhancedJsonReporter(const JsonConfiguration& config);
  explicit EnhancedJsonReporter(const std::string& output_file);

  // TestReporter interface implementation
  void on_suite_started(const std::string& suite_name, size_t total_tests) override;
  void on_suite_finished(const TestSuiteResult& result) override;
  void on_test_started(const std::string& test_name) override;
  void on_test_finished(const TestResult& result) override;
  void on_progress(const std::string& message, double percentage) override;
  void on_error(const std::string& error_message) override;

 protected:
  void on_output_stream_ready() override;

 private:
  JsonConfiguration json_config_;
  std::string current_suite_name_;
  std::vector<TestResult> current_suite_results_;
  std::vector<std::string> progress_messages_;
  std::vector<std::string> error_messages_;
  std::chrono::steady_clock::time_point suite_start_time_;
  bool json_header_written_ = false;

  // JSON generation methods
  void write_json_header();
  void write_json_footer();
  void write_test_suite(const TestSuiteResult& result);
  std::string json_escape(const std::string& text) const;
  std::string format_json_value(const std::string& key, const std::string& value,
                                bool is_last = false) const;
};

/**
 * @brief Factory for creating enhanced test reporters
 */
class EnhancedTestReporterFactory {
 public:
  static std::unique_ptr<EnhancedTestReporter> create_xml_reporter(
      const std::string& output_file,
      const EnhancedTestReporter::EnhancedConfiguration& config = {});

  static std::unique_ptr<EnhancedTestReporter> create_json_reporter(
      const std::string& output_file,
      const EnhancedTestReporter::EnhancedConfiguration& config = {});

  static std::unique_ptr<EnhancedTestReporter> create_robust_reporter(
      const std::string& format, const std::string& output_file,
      const EnhancedTestReporter::EnhancedConfiguration& config = {});

  // Configuration helpers
  static EnhancedTestReporter::EnhancedConfiguration create_high_reliability_config();
  static EnhancedTestReporter::EnhancedConfiguration create_performance_optimized_config();
  static EnhancedTestReporter::EnhancedConfiguration create_minimal_config();
};

}  // namespace SolarSystem::Testing
