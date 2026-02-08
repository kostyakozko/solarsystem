/**
 * @file data_exporter.cpp
 * @brief Implementation of data export system
 */

#include "solar_analysis/data_exporter.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace SolarSystem::Analysis {

std::string ExportMetadata::to_json() const {
  std::ostringstream oss;
  oss << "{\"title\":\"" << title << "\",\"description\":\"" << description
      << "\",\"reference_frame\":\"" << reference_frame << "\",\"coordinate_system\":\""
      << coordinate_system << "\",\"position_units\":\"" << position_units
      << "\",\"velocity_units\":\"" << velocity_units << "\",\"data_source\":\"" << data_source
      << "\",\"version\":\"" << version << "\"}";
  return oss.str();
}

DataExporter::DataExporter() = default;

DataExporter::DataExporter(const ExportConfig& config) : config_(config) {}

void DataExporter::set_config(const ExportConfig& config) { config_ = config; }

void DataExporter::set_metadata(const ExportMetadata& metadata) { metadata_ = metadata; }

void DataExporter::set_progress_callback(ExportProgressCallback callback) {
  progress_callback_ = std::move(callback);
}

ExportResult DataExporter::export_data(const std::vector<StateVector>& data,
                                       const std::filesystem::path& path) {
  ExportResult result;
  auto start = std::chrono::steady_clock::now();

  std::string content;
  switch (config_.format) {
    case ExportFormat::CSV:
      content = format_csv(data);
      break;
    case ExportFormat::JSON:
      content = format_json(data);
      break;
    case ExportFormat::XML:
      content = format_xml(data);
      break;
    default:
      content = format_csv(data);
  }

  std::ofstream file(path);
  if (!file) {
    result.message = "Failed to open file: " + path.string();
    return result;
  }

  file << content;
  file.close();

  result.success = true;
  result.output_path = path;
  result.records_exported = data.size();
  result.bytes_written = content.size();
  result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  return result;
}

ExportResult DataExporter::export_data(const std::vector<OrbitalElements>& data,
                                       const std::filesystem::path& path) {
  ExportResult result;
  auto start = std::chrono::steady_clock::now();

  std::string content;
  switch (config_.format) {
    case ExportFormat::JSON:
      content = format_json(data);
      break;
    default:
      content = format_csv(data);
  }

  std::ofstream file(path);
  if (!file) {
    result.message = "Failed to open file: " + path.string();
    return result;
  }

  file << content;
  file.close();

  result.success = true;
  result.output_path = path;
  result.records_exported = data.size();
  result.bytes_written = content.size();
  result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  return result;
}

ExportResult DataExporter::export_data(const SummaryStatistics& stats,
                                       const std::filesystem::path& path) {
  ExportResult result;
  auto start = std::chrono::steady_clock::now();

  std::ostringstream oss;
  if (config_.format == ExportFormat::JSON) {
    oss << "{\"mean\":" << stats.mean << ",\"median\":" << stats.median
        << ",\"variance\":" << stats.variance << ",\"std_dev\":" << stats.std_dev
        << ",\"min\":" << stats.min << ",\"max\":" << stats.max
        << ",\"skewness\":" << stats.skewness << ",\"kurtosis\":" << stats.kurtosis
        << ",\"count\":" << stats.count << "}";
  } else {
    oss << "mean,median,variance,std_dev,min,max,skewness,kurtosis,count\n"
        << stats.mean << "," << stats.median << "," << stats.variance << "," << stats.std_dev << ","
        << stats.min << "," << stats.max << "," << stats.skewness << "," << stats.kurtosis << ","
        << stats.count;
  }

  std::ofstream file(path);
  if (!file) {
    result.message = "Failed to open file: " + path.string();
    return result;
  }

  std::string content = oss.str();
  file << content;
  file.close();

  result.success = true;
  result.output_path = path;
  result.records_exported = 1;
  result.bytes_written = content.size();
  result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  return result;
}

std::string DataExporter::export_to_string(const std::vector<StateVector>& data) {
  switch (config_.format) {
    case ExportFormat::JSON:
      return format_json(data);
    case ExportFormat::XML:
      return format_xml(data);
    default:
      return format_csv(data);
  }
}

std::string DataExporter::export_to_string(const std::vector<OrbitalElements>& data) {
  switch (config_.format) {
    case ExportFormat::JSON:
      return format_json(data);
    default:
      return format_csv(data);
  }
}

std::vector<ExportResult> DataExporter::export_batch(
    const std::map<std::string, std::vector<StateVector>>& datasets,
    const std::filesystem::path& output_dir) {
  std::vector<ExportResult> results;
  std::filesystem::create_directories(output_dir);

  size_t i = 0;
  for (const auto& [name, data] : datasets) {
    std::string ext = (config_.format == ExportFormat::JSON) ? ".json" : ".csv";
    auto path = output_dir / (name + ext);
    results.push_back(export_data(data, path));

    if (progress_callback_) {
      progress_callback_(++i, datasets.size());
    }
  }

  return results;
}

std::string DataExporter::format_csv(const std::vector<StateVector>& data) {
  std::ostringstream oss;
  oss << std::setprecision(config_.precision);

  if (config_.include_metadata) {
    oss << "# " << metadata_.to_json() << "\n";
  }

  if (config_.include_header) {
    oss << "timestamp,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z\n";
  }

  for (size_t i = 0; i < data.size(); ++i) {
    const auto& sv = data[i];
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(sv.timestamp.time_since_epoch());
    oss << epoch.count() << config_.delimiter << sv.position.x() << config_.delimiter
        << sv.position.y() << config_.delimiter << sv.position.z() << config_.delimiter
        << sv.velocity.x() << config_.delimiter << sv.velocity.y() << config_.delimiter
        << sv.velocity.z() << "\n";

    if (progress_callback_ && i % 1000 == 0) {
      progress_callback_(i, data.size());
    }
  }

  return oss.str();
}

std::string DataExporter::format_json(const std::vector<StateVector>& data) {
  std::ostringstream oss;
  oss << std::setprecision(config_.precision);

  oss << "{";
  if (config_.include_metadata) {
    oss << "\"metadata\":" << metadata_.to_json() << ",";
  }
  oss << "\"data\":[";

  for (size_t i = 0; i < data.size(); ++i) {
    if (i > 0) oss << ",";
    const auto& sv = data[i];
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(sv.timestamp.time_since_epoch());
    oss << "{\"t\":" << epoch.count() << ",\"pos\":[" << sv.position.x() << "," << sv.position.y()
        << "," << sv.position.z() << "],\"vel\":[" << sv.velocity.x() << "," << sv.velocity.y()
        << "," << sv.velocity.z() << "]}";
  }

  oss << "]}";
  return oss.str();
}

std::string DataExporter::format_xml(const std::vector<StateVector>& data) {
  std::ostringstream oss;
  oss << std::setprecision(config_.precision);

  oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<dataset>\n";
  if (config_.include_metadata) {
    oss << "  <metadata>\n    <reference_frame>" << metadata_.reference_frame
        << "</reference_frame>\n    <position_units>" << metadata_.position_units
        << "</position_units>\n  </metadata>\n";
  }
  oss << "  <states>\n";

  for (const auto& sv : data) {
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(sv.timestamp.time_since_epoch());
    oss << "    <state t=\"" << epoch.count() << "\">\n"
        << "      <position x=\"" << sv.position.x() << "\" y=\"" << sv.position.y() << "\" z=\""
        << sv.position.z() << "\"/>\n"
        << "      <velocity x=\"" << sv.velocity.x() << "\" y=\"" << sv.velocity.y() << "\" z=\""
        << sv.velocity.z() << "\"/>\n"
        << "    </state>\n";
  }

  oss << "  </states>\n</dataset>";
  return oss.str();
}

std::string DataExporter::format_csv(const std::vector<OrbitalElements>& data) {
  std::ostringstream oss;
  oss << std::setprecision(config_.precision);

  if (config_.include_header) {
    oss << DataSerializer::to_csv_header() << "\n";
  }

  for (const auto& e : data) {
    oss << DataSerializer::to_csv(e) << "\n";
  }

  return oss.str();
}

std::string DataExporter::format_json(const std::vector<OrbitalElements>& data) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < data.size(); ++i) {
    if (i > 0) oss << ",";
    oss << DataSerializer::to_json(data[i]);
  }
  oss << "]";
  return oss.str();
}

}  // namespace SolarSystem::Analysis
