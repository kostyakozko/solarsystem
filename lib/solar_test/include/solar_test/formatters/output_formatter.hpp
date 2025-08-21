/**
 * @file output_formatter.hpp
 * @brief Enhanced output formatting system for test reporters (Task 16)
 *
 * Implements requirements 7.2 and 7.4:
 * - Ensure valid output format regardless of test content
 * - Manage memory efficiently and handle streaming output
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../framework/test_result.hpp"

namespace SolarSystem::Testing::Formatters {

/**
 * @brief Output format types supported by the system
 */
enum class OutputFormat {
  XML,
  JSON,
  TAP,
  JUnit,
  HTML,
  CSV,
  Plain,
  Markdown
};

/**
 * @brief Format validation result
 */
struct FormatValidationResult {
  bool is_valid = false;
  std::string error_message;
  std::vector<std::string> warnings;
  std::vector<std::string> suggestions;
  size_t error_line = 0;
  size_t error_column = 0;

  FormatValidationResult() = default;
  FormatValidationResult(bool valid) : is_valid(valid) {}

  explicit operator bool() const { return is_valid; }
};

/**
 * @brief Streaming configuration for large result sets
 */
struct StreamingConfig {
  size_t buffer_size = 64 * 1024;  // 64KB default
  size_t flush_threshold = 32 * 1024;  // Flush when buffer is half full
  bool auto_flush = true;
  bool compress_output = false;
  std::string compression_algorithm = "gzip";  // gzip, lz4, zstd
  size_t compression_threshold = 1024 * 1024;  // 1MB

  // Memory management
  size_t max_memory_usage = 100 * 1024 * 1024;  // 100MB
  bool use_memory_mapping = false;
  bool enable_chunked_processing = true;
  size_t chunk_size = 10000;  // Number of test results per chunk
};

/**
 * @brief Format-specific optimization settings
 */
struct FormatOptimization {
  bool minimize_whitespace = false;
  bool escape_special_chars = true;
  bool validate_during_write = true;
  bool use_streaming_json = false;  // For JSON arrays
  bool pretty_print = true;
  int indent_size = 2;

  // XML-specific
  bool include_xml_declaration = true;
  bool include_schema_reference = false;
  std::string xml_encoding = "UTF-8";

  // JSON-specific
  bool include_metadata = true;
  bool use_compact_arrays = false;

  // Performance optimizations
  bool cache_escaped_strings = true;
  bool use_string_interning = false;
  size_t string_cache_size = 1000;
};

/**
 * @brief Base interface for output formatters
 */
class OutputFormatter {
public:
  virtual ~OutputFormatter() = default;

  // Core formatting methods
  virtual std::string format_test_suite(const TestSuiteResult& result) = 0;
  virtual std::string format_test_result(const TestResult& result) = 0;
  virtual std::string format_header() = 0;
  virtual std::string format_footer() = 0;

  // Streaming methods
  virtual void start_streaming(std::ostream& output) = 0;
  virtual void stream_test_result(const TestResult& result, std::ostream& output) = 0;
  virtual void end_streaming(std::ostream& output) = 0;

  // Validation and error recovery
  virtual FormatValidationResult validate_output(const std::string& output) = 0;
  virtual std::string recover_from_error(const std::string& invalid_output,
                                        const std::string& error_context) = 0;

  // Configuration
  virtual void set_optimization(const FormatOptimization& optimization) = 0;
  virtual void set_streaming_config(const StreamingConfig& config) = 0;

  // Utility methods
  virtual OutputFormat get_format() const = 0;
  virtual std::string get_file_extension() const = 0;
  virtual std::string get_mime_type() const = 0;

protected:
  FormatOptimization optimization_;
  StreamingConfig streaming_config_;
};

/**
 * @brief Enhanced XML formatter with validation and streaming
 */
class EnhancedXmlFormatter : public OutputFormatter {
public:
  explicit EnhancedXmlFormatter(const FormatOptimization& optimization = FormatOptimization{});

  // OutputFormatter interface
  std::string format_test_suite(const TestSuiteResult& result) override;
  std::string format_test_result(const TestResult& result) override;
  std::string format_header() override;
  std::string format_footer() override;

  void start_streaming(std::ostream& output) override;
  void stream_test_result(const TestResult& result, std::ostream& output) override;
  void end_streaming(std::ostream& output) override;

  FormatValidationResult validate_output(const std::string& output) override;
  std::string recover_from_error(const std::string& invalid_output,
                                const std::string& error_context) override;

  void set_optimization(const FormatOptimization& optimization) override;
  void set_streaming_config(const StreamingConfig& config) override;

  OutputFormat get_format() const override { return OutputFormat::XML; }
  std::string get_file_extension() const override { return ".xml"; }
  std::string get_mime_type() const override { return "application/xml"; }

private:
  std::unordered_map<std::string, std::string> escaped_string_cache_;
  bool streaming_active_ = false;
  size_t test_count_ = 0;

  // XML utility methods
  std::string xml_escape(const std::string& text);
  std::string format_xml_element(const std::string& name,
                                const std::unordered_map<std::string, std::string>& attributes,
                                const std::string& content = "",
                                bool self_closing = false);
  std::string format_duration(std::chrono::milliseconds duration);
  void validate_xml_structure(const std::string& xml);
};

/**
 * @brief Enhanced JSON formatter with streaming and compression
 */
class EnhancedJsonFormatter : public OutputFormatter {
public:
  explicit EnhancedJsonFormatter(const FormatOptimization& optimization = FormatOptimization{});

  // OutputFormatter interface
  std::string format_test_suite(const TestSuiteResult& result) override;
  std::string format_test_result(const TestResult& result) override;
  std::string format_header() override;
  std::string format_footer() override;

  void start_streaming(std::ostream& output) override;
  void stream_test_result(const TestResult& result, std::ostream& output) override;
  void end_streaming(std::ostream& output) override;

  FormatValidationResult validate_output(const std::string& output) override;
  std::string recover_from_error(const std::string& invalid_output,
                                const std::string& error_context) override;

  void set_optimization(const FormatOptimization& optimization) override;
  void set_streaming_config(const StreamingConfig& config) override;

  OutputFormat get_format() const override { return OutputFormat::JSON; }
  std::string get_file_extension() const override { return ".json"; }
  std::string get_mime_type() const override { return "application/json"; }

private:
  bool streaming_active_ = false;
  bool first_result_ = true;

  // JSON utility methods
  std::string json_escape(const std::string& text);
  std::string format_json_object(const std::unordered_map<std::string, std::string>& fields,
                                int indent_level = 0);
  std::string format_json_array(const std::vector<std::string>& items,
                               int indent_level = 0);
  void validate_json_syntax(const std::string& json);
};

/**
 * @brief Streaming output manager for large result sets
 */
class StreamingOutputManager {
public:
  explicit StreamingOutputManager(std::unique_ptr<OutputFormatter> formatter,
                                 const StreamingConfig& config = StreamingConfig{});

  // Streaming operations
  void start_output(std::ostream& output);
  void add_test_result(const TestResult& result);
  void add_test_suite(const TestSuiteResult& suite);
  void finish_output();

  // Memory management
  void flush_buffer();
  void optimize_memory_usage();
  size_t get_memory_usage() const;

  // Statistics
  struct StreamingStats {
    size_t total_results_processed = 0;
    size_t bytes_written = 0;
    size_t flush_operations = 0;
    size_t compression_ratio = 100;  // Percentage
    std::chrono::milliseconds total_time{0};
    size_t peak_memory_usage = 0;
  };

  const StreamingStats& get_statistics() const { return stats_; }
  void reset_statistics() { stats_ = StreamingStats{}; }

private:
  std::unique_ptr<OutputFormatter> formatter_;
  StreamingConfig config_;
  std::ostringstream buffer_;
  std::ostream* output_stream_ = nullptr;
  StreamingStats stats_;
  std::chrono::steady_clock::time_point start_time_;

  // Memory management
  void check_memory_limits();
  void compress_buffer_if_needed();
  std::string compress_data(const std::string& data);
};

/**
 * @brief Format converter for migrating between output formats
 */
class FormatConverter {
public:
  // Conversion methods
  static std::string convert(const std::string& input,
                           OutputFormat from_format,
                           OutputFormat to_format);

  static std::string xml_to_json(const std::string& xml);
  static std::string json_to_xml(const std::string& json);
  static std::string xml_to_html(const std::string& xml);
  static std::string json_to_csv(const std::string& json);

  // Validation during conversion
  static FormatValidationResult validate_conversion(const std::string& input,
                                                   const std::string& output,
                                                   OutputFormat from_format,
                                                   OutputFormat to_format);

  // Batch conversion
  struct ConversionJob {
    std::string input_file;
    std::string output_file;
    OutputFormat from_format;
    OutputFormat to_format;
    FormatOptimization optimization;
  };

  static std::vector<FormatValidationResult> convert_batch(
    const std::vector<ConversionJob>& jobs);

private:
  // Internal parsing helpers
  struct ParsedTestResult {
    std::string name;
    std::string status;
    std::string error_message;
    std::chrono::milliseconds duration{0};
    std::unordered_map<std::string, std::string> metadata;
  };

  static std::vector<ParsedTestResult> parse_xml_results(const std::string& xml);
  static std::vector<ParsedTestResult> parse_json_results(const std::string& json);
};

/**
 * @brief Factory for creating output formatters
 */
class OutputFormatterFactory {
public:
  static std::unique_ptr<OutputFormatter> create_formatter(
    OutputFormat format,
    const FormatOptimization& optimization = FormatOptimization{});

  static std::unique_ptr<StreamingOutputManager> create_streaming_manager(
    OutputFormat format,
    const StreamingConfig& streaming_config = StreamingConfig{},
    const FormatOptimization& optimization = FormatOptimization{});

  // Format detection
  static OutputFormat detect_format(const std::string& content);
  static OutputFormat detect_format_from_extension(const std::string& filename);

  // Supported formats
  static std::vector<OutputFormat> get_supported_formats();
  static std::string format_to_string(OutputFormat format);
  static OutputFormat string_to_format(const std::string& format_str);
};

/**
 * @brief Output format validator with comprehensive error checking
 */
class OutputValidator {
public:
  // Validation methods
  static FormatValidationResult validate_xml(const std::string& xml);
  static FormatValidationResult validate_json(const std::string& json);
  static FormatValidationResult validate_junit_xml(const std::string& xml);

  // Content validation
  static FormatValidationResult validate_test_content(const std::string& content,
                                                     OutputFormat format);

  // Schema validation
  static FormatValidationResult validate_against_schema(const std::string& content,
                                                        const std::string& schema,
                                                        OutputFormat format);

  // Recovery suggestions
  static std::vector<std::string> suggest_fixes(const FormatValidationResult& validation_result);

private:
  // Internal validation helpers
  static bool is_valid_xml_name(const std::string& name);
  static bool is_valid_json_string(const std::string& str);
  static std::vector<std::string> find_xml_errors(const std::string& xml);
  static std::vector<std::string> find_json_errors(const std::string& json);
};

} // namespace SolarSystem::Testing::Formatters
