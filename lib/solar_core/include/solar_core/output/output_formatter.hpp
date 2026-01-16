#pragma once

/**
 * @file output_formatter.hpp
 * @brief Comprehensive output formatting system for simulation results
 *
 * Provides multiple output formats with metadata, validation, customization,
 * filtering, compression, and archiving capabilities.
 */

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/export.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_utils/expected.hpp"

namespace SolarSystem::Output {

/**
 * @brief Supported output formats
 */
enum class OutputFormat {
  TEXT,     // Human-readable text format (default/legacy)
  JSON,     // JSON format for programmatic access
  CSV,      // CSV format for spreadsheet analysis
  XML,      // XML format for structured data exchange
  BINARY,   // Binary format for efficient storage
  MARKDOWN  // Markdown format for documentation
};

/**
 * @brief Output quality levels
 */
enum class QualityLevel {
  MINIMAL,       // Minimal output (positions only)
  STANDARD,      // Standard output (positions + velocities)
  DETAILED,      // Detailed output (+ energies, momentum)
  COMPREHENSIVE  // Comprehensive output (all available data)
};

/**
 * @brief Output compression types
 */
enum class CompressionType {
  NONE,   // No compression
  GZIP,   // GZIP compression
  ZLIB,   // ZLIB compression
  CUSTOM  // Custom compression
};

/**
 * @brief Output metadata
 */
struct SOLAR_CORE_API OutputMetadata {
  std::string format_version = "1.0";
  std::chrono::system_clock::time_point generated_at;
  std::string generator = "Solar System Suite";
  std::string generator_version = "4.0.0";

  // Simulation metadata
  std::chrono::system_clock::time_point simulation_start_time;
  std::chrono::system_clock::time_point simulation_end_time;
  double simulation_duration_seconds = 0.0;
  size_t iteration_count = 0;

  // Data metadata
  size_t body_count = 0;
  std::vector<std::string> body_names;
  std::string integration_method;
  double time_step = 0.0;

  // Quality metadata
  QualityLevel quality_level = QualityLevel::STANDARD;
  bool validated = false;
  std::vector<std::string> validation_warnings;

  // File metadata
  std::optional<std::filesystem::path> output_path;
  size_t uncompressed_size = 0;
  size_t compressed_size = 0;
  CompressionType compression = CompressionType::NONE;

  /**
   * @brief Convert metadata to JSON string
   */
  [[nodiscard]] std::string to_json() const;

  /**
   * @brief Parse metadata from JSON string
   */
  [[nodiscard]] static Utils::Expected<OutputMetadata, std::string> from_json(
      const std::string& json);
};

/**
 * @brief Output filter configuration
 */
struct SOLAR_CORE_API OutputFilter {
  // Body filtering
  std::vector<std::string> include_bodies;
  std::vector<std::string> exclude_bodies;

  // Data filtering
  bool include_positions = true;
  bool include_velocities = true;
  bool include_accelerations = false;
  bool include_energies = false;
  bool include_momentum = false;

  // Coordinate system
  bool relative_to_barycenter = true;
  std::optional<std::string> relative_to_body;

  // Precision control
  int position_precision = 12;
  int velocity_precision = 12;
  int energy_precision = 15;

  /**
   * @brief Check if a body should be included
   */
  [[nodiscard]] bool should_include_body(std::string_view body_name) const;
};

/**
 * @brief Output formatting options
 */
struct SOLAR_CORE_API OutputOptions {
  OutputFormat format = OutputFormat::TEXT;
  QualityLevel quality = QualityLevel::STANDARD;
  OutputFilter filter;

  // Formatting options
  bool pretty_print = true;
  bool include_metadata = true;
  bool include_header = true;
  bool include_footer = false;

  // Compression options
  CompressionType compression = CompressionType::NONE;
  int compression_level = 6;  // 1-9 for gzip/zlib

  // Output destination
  std::optional<std::filesystem::path> output_file;
  bool append_mode = false;
  bool create_backup = false;

  /**
   * @brief Validate options
   */
  [[nodiscard]] Utils::Expected<void, std::string> validate() const;
};

/**
 * @brief Validation result for output data
 */
struct ValidationResult {
  bool is_valid = true;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::vector<std::string> suggestions;

  // Quality metrics
  double data_completeness = 1.0;  // 0.0 to 1.0
  double data_consistency = 1.0;   // 0.0 to 1.0
  size_t missing_data_points = 0;
  size_t inconsistent_data_points = 0;

  /**
   * @brief Add error message
   */
  void add_error(const std::string& error);

  /**
   * @brief Add warning message
   */
  void add_warning(const std::string& warning);

  /**
   * @brief Add suggestion
   */
  void add_suggestion(const std::string& suggestion);

  /**
   * @brief Get summary string
   */
  [[nodiscard]] std::string summary() const;
};

/**
 * @brief Formatted output result
 */
struct FormattedOutput {
  std::string content;
  OutputMetadata metadata;
  ValidationResult validation;

  /**
   * @brief Write to file
   */
  [[nodiscard]] Utils::Expected<void, std::string> write_to_file(const std::filesystem::path& path,
                                                                 bool create_backup = false) const;

  /**
   * @brief Get content size
   */
  [[nodiscard]] size_t size() const { return content.size(); }

  /**
   * @brief Check if output is valid
   */
  [[nodiscard]] bool is_valid() const { return validation.is_valid; }
};

/**
 * @brief Main output formatter class
 */
class SOLAR_CORE_API OutputFormatter {
 public:
  /**
   * @brief Format simulation results
   */
  [[nodiscard]] static Utils::Expected<FormattedOutput, std::string> format(
      const Simulation::SimulationEngine& engine, const OutputOptions& options = {});

  /**
   * @brief Format body collection
   */
  [[nodiscard]] static Utils::Expected<FormattedOutput, std::string> format_bodies(
      const Bodies::BodyCollection& bodies, std::chrono::system_clock::time_point time_point,
      const OutputOptions& options = {});

  /**
   * @brief Format simulation state
   */
  [[nodiscard]] static Utils::Expected<FormattedOutput, std::string> format_state(
      const Simulation::SimulationState& state, const OutputOptions& options = {});

  /**
   * @brief Validate output data
   */
  [[nodiscard]] static ValidationResult validate_output(const Bodies::BodyCollection& bodies,
                                                        const Simulation::SimulationState& state);

  /**
   * @brief Compress output data (returns empty string on failure)
   */
  [[nodiscard]] static std::string compress(const std::string& data, CompressionType type,
                                            int level = 6);

  /**
   * @brief Decompress output data (returns empty string on failure)
   */
  [[nodiscard]] static std::string decompress(const std::string& compressed_data,
                                              CompressionType type);

  /**
   * @brief Get supported formats
   */
  [[nodiscard]] static std::vector<OutputFormat> supported_formats();

  /**
   * @brief Get format name
   */
  [[nodiscard]] static std::string format_name(OutputFormat format);

  /**
   * @brief Parse format from string
   */
  [[nodiscard]] static Utils::Expected<OutputFormat, std::string> parse_format(
      const std::string& format_str);

  // Helper functions (public for use by OutputMetadata)
  [[nodiscard]] static std::string format_timestamp(std::chrono::system_clock::time_point tp);

  [[nodiscard]] static std::string format_vector(const Math::Vector3d& vec, int precision);

 private:
  // Format-specific implementations (using int as error code to avoid Expected<string,string>
  // issue)
  [[nodiscard]] static std::string format_as_text(const Bodies::BodyCollection& bodies,
                                                  const Simulation::SimulationState& state,
                                                  const OutputOptions& options);

  [[nodiscard]] static std::string format_as_json(const Bodies::BodyCollection& bodies,
                                                  const Simulation::SimulationState& state,
                                                  const OutputOptions& options);

  [[nodiscard]] static std::string format_as_csv(const Bodies::BodyCollection& bodies,
                                                 const Simulation::SimulationState& state,
                                                 const OutputOptions& options);

  [[nodiscard]] static std::string format_as_xml(const Bodies::BodyCollection& bodies,
                                                 const Simulation::SimulationState& state,
                                                 const OutputOptions& options);

  [[nodiscard]] static std::string format_as_markdown(const Bodies::BodyCollection& bodies,
                                                      const Simulation::SimulationState& state,
                                                      const OutputOptions& options);

  // Private helper functions
  [[nodiscard]] static Bodies::BodyCollection apply_filter(const Bodies::BodyCollection& bodies,
                                                           const OutputFilter& filter);
};

/**
 * @brief Output archiver for managing multiple output files
 */
class SOLAR_CORE_API OutputArchiver {
 public:
  /**
   * @brief Create archive from multiple outputs
   */
  [[nodiscard]] static Utils::Expected<void, std::string> create_archive(
      const std::vector<std::filesystem::path>& files, const std::filesystem::path& archive_path,
      CompressionType compression = CompressionType::GZIP);

  /**
   * @brief Extract archive
   */
  [[nodiscard]] static Utils::Expected<std::vector<std::filesystem::path>, std::string>
  extract_archive(const std::filesystem::path& archive_path,
                  const std::filesystem::path& destination);

  /**
   * @brief List archive contents
   */
  [[nodiscard]] static Utils::Expected<std::vector<std::string>, std::string> list_archive(
      const std::filesystem::path& archive_path);
};

// Utility functions
[[nodiscard]] SOLAR_CORE_API std::string to_string(OutputFormat format);
[[nodiscard]] SOLAR_CORE_API std::string to_string(QualityLevel quality);
[[nodiscard]] SOLAR_CORE_API std::string to_string(CompressionType compression);

}  // namespace SolarSystem::Output
