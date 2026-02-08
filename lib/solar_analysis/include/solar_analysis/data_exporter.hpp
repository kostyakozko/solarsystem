#pragma once

/**
 * @file data_exporter.hpp
 * @brief Comprehensive data export system for analysis results
 */

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <solar_analysis/data_models.hpp>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>
#include <solar_analysis/statistical_analyzer.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Export format types
 */
enum class ExportFormat { CSV, JSON, XML, Binary };

/**
 * @brief Export metadata for scientific data
 */
struct SOLAR_ANALYSIS_API ExportMetadata {
  std::string title;
  std::string description;
  std::string author;
  std::chrono::system_clock::time_point created_at;
  std::string version = "1.0";

  // Scientific metadata
  std::string reference_frame = "J2000_Ecliptic";
  std::string coordinate_system = "Cartesian";
  std::string position_units = "km";
  std::string velocity_units = "km/s";
  std::string time_units = "seconds";

  // Data provenance
  std::string data_source;
  std::chrono::system_clock::time_point data_epoch;
  std::vector<std::string> processing_steps;

  [[nodiscard]] std::string to_json() const;
};

/**
 * @brief Export configuration
 */
struct SOLAR_ANALYSIS_API ExportConfig {
  ExportFormat format = ExportFormat::CSV;
  bool include_metadata = true;
  bool include_header = true;
  bool compress = false;
  int precision = 10;
  std::string delimiter = ",";
  std::vector<std::string> fields;  // Empty = all fields
};

/**
 * @brief Export progress callback
 */
using ExportProgressCallback = std::function<void(size_t current, size_t total)>;

/**
 * @brief Export result
 */
struct SOLAR_ANALYSIS_API ExportResult {
  bool success = false;
  std::string message;
  std::filesystem::path output_path;
  size_t records_exported = 0;
  size_t bytes_written = 0;
  std::chrono::milliseconds duration{0};
};

/**
 * @brief Data exporter for analysis results
 */
class SOLAR_ANALYSIS_API DataExporter {
 public:
  DataExporter();
  explicit DataExporter(const ExportConfig& config);

  // Configuration
  void set_config(const ExportConfig& config);
  void set_metadata(const ExportMetadata& metadata);
  void set_progress_callback(ExportProgressCallback callback);

  // Export state vectors
  [[nodiscard]] ExportResult export_data(const std::vector<StateVector>& data,
                                         const std::filesystem::path& path);

  // Export orbital elements
  [[nodiscard]] ExportResult export_data(const std::vector<OrbitalElements>& data,
                                         const std::filesystem::path& path);

  // Export statistics
  [[nodiscard]] ExportResult export_data(const SummaryStatistics& stats,
                                         const std::filesystem::path& path);

  // Export to string (for in-memory use)
  [[nodiscard]] std::string export_to_string(const std::vector<StateVector>& data);
  [[nodiscard]] std::string export_to_string(const std::vector<OrbitalElements>& data);

  // Batch export
  [[nodiscard]] std::vector<ExportResult> export_batch(
      const std::map<std::string, std::vector<StateVector>>& datasets,
      const std::filesystem::path& output_dir);

 private:
  ExportConfig config_;
  ExportMetadata metadata_;
  ExportProgressCallback progress_callback_;

  [[nodiscard]] std::string format_csv(const std::vector<StateVector>& data);
  [[nodiscard]] std::string format_json(const std::vector<StateVector>& data);
  [[nodiscard]] std::string format_xml(const std::vector<StateVector>& data);
  [[nodiscard]] std::string format_csv(const std::vector<OrbitalElements>& data);
  [[nodiscard]] std::string format_json(const std::vector<OrbitalElements>& data);
};

}  // namespace SolarSystem::Analysis
