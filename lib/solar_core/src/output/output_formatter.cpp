#include "solar_core/output/output_formatter.hpp"

#include <zlib.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

namespace SolarSystem::Output {

// ============================================================================
// OutputMetadata Implementation
// ============================================================================

std::string OutputMetadata::to_json() const {
  nlohmann::json j;
  j["format_version"] = format_version;
  j["generated_at"] = OutputFormatter::format_timestamp(generated_at);
  j["generator"] = generator;
  j["generator_version"] = generator_version;

  j["simulation"] = {{"start_time", OutputFormatter::format_timestamp(simulation_start_time)},
                     {"end_time", OutputFormatter::format_timestamp(simulation_end_time)},
                     {"duration_seconds", simulation_duration_seconds},
                     {"iteration_count", iteration_count},
                     {"integration_method", integration_method},
                     {"time_step", time_step}};

  j["data"] = {{"body_count", body_count},
               {"quality_level", to_string(quality_level)},
               {"validated", validated}};

  return j.dump(2);
}

Utils::Expected<OutputMetadata, std::string> OutputMetadata::from_json(
    const std::string& json_str) {
  try {
    auto j = nlohmann::json::parse(json_str);
    OutputMetadata metadata;

    if (j.contains("format_version")) {
      metadata.format_version = j["format_version"].get<std::string>();
    }
    if (j.contains("generator")) {
      metadata.generator = j["generator"].get<std::string>();
    }
    if (j.contains("generator_version")) {
      metadata.generator_version = j["generator_version"].get<std::string>();
    }

    if (j.contains("simulation")) {
      auto& sim = j["simulation"];
      if (sim.contains("duration_seconds")) {
        metadata.simulation_duration_seconds = sim["duration_seconds"].get<double>();
      }
      if (sim.contains("iteration_count")) {
        metadata.iteration_count = sim["iteration_count"].get<size_t>();
      }
      if (sim.contains("integration_method")) {
        metadata.integration_method = sim["integration_method"].get<std::string>();
      }
      if (sim.contains("time_step")) {
        metadata.time_step = sim["time_step"].get<double>();
      }
    }

    if (j.contains("data")) {
      auto& data = j["data"];
      if (data.contains("body_count")) {
        metadata.body_count = data["body_count"].get<size_t>();
      }
      if (data.contains("validated")) {
        metadata.validated = data["validated"].get<bool>();
      }
    }

    return metadata;
  } catch (const nlohmann::json::exception& e) {
    return Utils::Expected<OutputMetadata, std::string>(std::string("JSON parsing error: ") +
                                                        e.what());
  }
}

// ============================================================================
// OutputFilter Implementation
// ============================================================================

bool OutputFilter::should_include_body(std::string_view body_name) const {
  // If include list is specified, body must be in it
  if (!include_bodies.empty()) {
    bool found = false;
    for (const auto& name : include_bodies) {
      if (name == body_name) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }

  // If exclude list is specified, body must not be in it
  if (!exclude_bodies.empty()) {
    for (const auto& name : exclude_bodies) {
      if (name == body_name) {
        return false;
      }
    }
  }

  return true;
}

// ============================================================================
// OutputOptions Implementation
// ============================================================================

Utils::Expected<void, std::string> OutputOptions::validate() const {
  // Validate compression level
  if (compression != CompressionType::NONE) {
    if (compression_level < 1 || compression_level > 9) {
      return Utils::make_unexpected(std::string("Compression level must be between 1 and 9"));
    }
  }

  // Validate precision
  if (filter.position_precision < 1 || filter.position_precision > 20) {
    return Utils::make_unexpected(std::string("Position precision must be between 1 and 20"));
  }

  // Validate output file if specified
  if (output_file.has_value()) {
    const auto& path = output_file.value();
    if (path.empty()) {
      return Utils::make_unexpected(std::string("Output file path cannot be empty"));
    }

    // Check if parent directory exists
    if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
      return Utils::make_unexpected(std::string("Output directory does not exist: ") +
                                    path.parent_path().string());
    }
  }

  return {};
}

// ============================================================================
// ValidationResult Implementation
// ============================================================================

void ValidationResult::add_error(const std::string& error) {
  errors.push_back(error);
  is_valid = false;
}

void ValidationResult::add_warning(const std::string& warning) { warnings.push_back(warning); }

void ValidationResult::add_suggestion(const std::string& suggestion) {
  suggestions.push_back(suggestion);
}

std::string ValidationResult::summary() const {
  std::ostringstream oss;
  oss << "Validation Summary:\n";
  oss << "  Status: " << (is_valid ? "VALID" : "INVALID") << "\n";
  oss << "  Data Completeness: " << (data_completeness * 100.0) << "%\n";
  oss << "  Data Consistency: " << (data_consistency * 100.0) << "%\n";

  if (!errors.empty()) {
    oss << "  Errors (" << errors.size() << "):\n";
    for (const auto& error : errors) {
      oss << "    - " << error << "\n";
    }
  }

  if (!warnings.empty()) {
    oss << "  Warnings (" << warnings.size() << "):\n";
    for (const auto& warning : warnings) {
      oss << "    - " << warning << "\n";
    }
  }

  return oss.str();
}

// ============================================================================
// FormattedOutput Implementation
// ============================================================================

Utils::Expected<void, std::string> FormattedOutput::write_to_file(const std::filesystem::path& path,
                                                                  bool create_backup) const {
  // Create backup if requested and file exists
  if (create_backup && std::filesystem::exists(path)) {
    auto backup_path = path;
    backup_path += ".backup";
    try {
      std::filesystem::copy_file(path, backup_path,
                                 std::filesystem::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
      return Utils::make_unexpected("Failed to create backup: " + std::string(e.what()));
    }
  }

  // Write content to file
  std::ofstream ofs(path, std::ios::binary);
  if (!ofs) {
    return Utils::make_unexpected("Failed to open file for writing: " + path.string());
  }

  ofs << content;
  if (!ofs) {
    return Utils::make_unexpected("Failed to write content to file: " + path.string());
  }

  return {};
}

// ============================================================================
// OutputFormatter Implementation
// ============================================================================

Utils::Expected<FormattedOutput, std::string> OutputFormatter::format(
    const Simulation::SimulationEngine& engine, const OutputOptions& options) {
  // Validate options
  auto validation = options.validate();
  if (!validation.has_value()) {
    return Utils::Expected<FormattedOutput, std::string>(validation.error());
  }

  return format_bodies(engine.get_bodies(), engine.get_current_date(), options);
}

Utils::Expected<FormattedOutput, std::string> OutputFormatter::format_bodies(
    const Bodies::BodyCollection& bodies, std::chrono::system_clock::time_point time_point,
    const OutputOptions& options) {
  FormattedOutput output;

  // Validate options
  auto validation = options.validate();
  if (!validation.has_value()) {
    return Utils::Expected<FormattedOutput, std::string>(validation.error());
  }

  // Create simulation state for formatting
  Simulation::SimulationState state;
  state.reference_time = time_point;
  state.center_of_mass = bodies.center_of_mass();
  state.current_time = 0.0;
  state.iteration_count = 0;

  // Create metadata
  output.metadata.generated_at = std::chrono::system_clock::now();
  output.metadata.simulation_start_time = state.reference_time;
  output.metadata.simulation_end_time = state.reference_time;
  output.metadata.simulation_duration_seconds = state.current_time;
  output.metadata.iteration_count = state.iteration_count;
  output.metadata.quality_level = options.quality;
  output.metadata.body_count = bodies.size();

  // Format based on selected format
  std::string formatted_content;

  switch (options.format) {
    case OutputFormat::TEXT:
      formatted_content = format_as_text(bodies, state, options);
      break;
    case OutputFormat::JSON:
      formatted_content = format_as_json(bodies, state, options);
      break;
    case OutputFormat::CSV:
      formatted_content = format_as_csv(bodies, state, options);
      break;
    case OutputFormat::XML:
      formatted_content = format_as_xml(bodies, state, options);
      break;
    case OutputFormat::MARKDOWN:
      formatted_content = format_as_markdown(bodies, state, options);
      break;
    default:
      return Utils::Expected<FormattedOutput, std::string>(
          std::string("Unsupported output format"));
  }

  output.content = formatted_content;

  // Compress if requested
  if (options.compression != CompressionType::NONE) {
    auto compressed = compress(output.content, options.compression, options.compression_level);
    if (!compressed.empty()) {
      output.metadata.uncompressed_size = output.content.size();
      output.content = compressed;
      output.metadata.compressed_size = output.content.size();
      output.metadata.compression = options.compression;
    }
  }

  // Validate output
  output.validation = validate_output(bodies, state);
  output.metadata.validated = output.validation.is_valid;

  return output;
}

Utils::Expected<FormattedOutput, std::string> OutputFormatter::format_state(
    const Simulation::SimulationState& state, const OutputOptions& options) {
  // Create empty body collection for state-only formatting
  Bodies::BodyCollection empty_bodies;

  FormattedOutput output;

  // Validate options
  auto validation = options.validate();
  if (!validation.has_value()) {
    return Utils::Expected<FormattedOutput, std::string>(validation.error());
  }

  // Create metadata
  output.metadata.generated_at = std::chrono::system_clock::now();
  output.metadata.simulation_start_time = state.reference_time;
  output.metadata.simulation_end_time = state.reference_time;
  output.metadata.simulation_duration_seconds = state.current_time;
  output.metadata.iteration_count = state.iteration_count;
  output.metadata.quality_level = options.quality;

  // Format based on selected format (with empty bodies)
  std::string formatted_content;

  switch (options.format) {
    case OutputFormat::TEXT:
      formatted_content = format_as_text(empty_bodies, state, options);
      break;
    case OutputFormat::JSON:
      formatted_content = format_as_json(empty_bodies, state, options);
      break;
    case OutputFormat::CSV:
      formatted_content = format_as_csv(empty_bodies, state, options);
      break;
    case OutputFormat::XML:
      formatted_content = format_as_xml(empty_bodies, state, options);
      break;
    case OutputFormat::MARKDOWN:
      formatted_content = format_as_markdown(empty_bodies, state, options);
      break;
    default:
      return Utils::Expected<FormattedOutput, std::string>(
          std::string("Unsupported output format"));
  }

  output.content = formatted_content;

  // Compress if requested
  if (options.compression != CompressionType::NONE) {
    auto compressed = compress(output.content, options.compression, options.compression_level);
    if (!compressed.empty()) {
      output.metadata.uncompressed_size = output.content.size();
      output.content = compressed;
      output.metadata.compressed_size = output.content.size();
      output.metadata.compression = options.compression;
    }
  }

  // Validate output
  output.validation = validate_output(empty_bodies, state);
  output.metadata.validated = output.validation.is_valid;

  return output;
}

ValidationResult OutputFormatter::validate_output(const Bodies::BodyCollection& bodies,
                                                  const Simulation::SimulationState& state) {
  ValidationResult result;

  // Check for NaN or infinite values in state
  if (std::isnan(state.current_time) || std::isinf(state.current_time)) {
    result.add_error("Invalid simulation time");
  }

  if (std::isnan(state.total_energy) || std::isinf(state.total_energy)) {
    result.add_warning("Invalid total energy value");
  }

  // Check body data
  for (const auto& body : bodies) {
    const auto& pos = body.position();
    const auto& vel = body.velocity();

    if (std::isnan(pos.x()) || std::isnan(pos.y()) || std::isnan(pos.z())) {
      result.add_error(std::string("Invalid position for body: ") + std::string(body.name()));
      result.missing_data_points++;
    }

    if (std::isnan(vel.x()) || std::isnan(vel.y()) || std::isnan(vel.z())) {
      result.add_error(std::string("Invalid velocity for body: ") + std::string(body.name()));
      result.missing_data_points++;
    }
  }

  // Calculate data completeness
  size_t total_data_points = bodies.size() * 6;  // 3 position + 3 velocity per body
  if (total_data_points > 0) {
    result.data_completeness = 1.0 - (static_cast<double>(result.missing_data_points) /
                                      static_cast<double>(total_data_points));
  }

  // Calculate data consistency
  result.data_consistency = 1.0 - (static_cast<double>(result.inconsistent_data_points) /
                                   static_cast<double>(std::max(size_t(1), total_data_points)));

  return result;
}

std::string OutputFormatter::compress(const std::string& data, CompressionType type, int level) {
  if (type == CompressionType::NONE || data.empty()) {
    return data;
  }

  // Use zlib for compression
  uLongf compressed_size = compressBound(data.size());
  std::vector<uint8_t> compressed_data(compressed_size);

  // Map compression level (0-9 for zlib)
  int zlib_level = Z_DEFAULT_COMPRESSION;
  if (level >= 0 && level <= 9) {
    zlib_level = level;
  }

  int result = compress2(compressed_data.data(), &compressed_size,
                         reinterpret_cast<const uint8_t*>(data.data()), data.size(), zlib_level);

  if (result != Z_OK) {
    // Compression failed, return original data
    return data;
  }

  // Resize to actual compressed size
  compressed_data.resize(compressed_size);

  // Convert to string
  return std::string(reinterpret_cast<char*>(compressed_data.data()), compressed_size);
}

std::string OutputFormatter::decompress(const std::string& compressed_data, CompressionType type) {
  if (type == CompressionType::NONE || compressed_data.empty()) {
    return compressed_data;
  }

  // Use zlib for decompression
  // Start with a reasonable buffer size (4x compressed size)
  uLongf uncompressed_size = compressed_data.size() * 4;
  std::vector<uint8_t> uncompressed_data(uncompressed_size);

  int result =
      uncompress(uncompressed_data.data(), &uncompressed_size,
                 reinterpret_cast<const uint8_t*>(compressed_data.data()), compressed_data.size());

  // If buffer was too small, try with larger buffer
  if (result == Z_BUF_ERROR) {
    uncompressed_size = compressed_data.size() * 10;
    uncompressed_data.resize(uncompressed_size);
    result = uncompress(uncompressed_data.data(), &uncompressed_size,
                        reinterpret_cast<const uint8_t*>(compressed_data.data()),
                        compressed_data.size());
  }

  if (result != Z_OK) {
    // Decompression failed, return empty string
    return "";
  }

  // Resize to actual uncompressed size
  uncompressed_data.resize(uncompressed_size);

  // Convert to string
  return std::string(reinterpret_cast<char*>(uncompressed_data.data()), uncompressed_size);
}

std::vector<OutputFormat> OutputFormatter::supported_formats() {
  return {OutputFormat::TEXT, OutputFormat::JSON, OutputFormat::CSV, OutputFormat::XML,
          OutputFormat::MARKDOWN};
}

std::string OutputFormatter::format_name(OutputFormat format) { return to_string(format); }

Utils::Expected<OutputFormat, std::string> OutputFormatter::parse_format(
    const std::string& format_str) {
  std::string lower = format_str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == "text" || lower == "txt") return OutputFormat::TEXT;
  if (lower == "json") return OutputFormat::JSON;
  if (lower == "csv") return OutputFormat::CSV;
  if (lower == "xml") return OutputFormat::XML;
  if (lower == "markdown" || lower == "md") return OutputFormat::MARKDOWN;
  if (lower == "binary" || lower == "bin") return OutputFormat::BINARY;

  return Utils::Expected<OutputFormat, std::string>(std::string("Unknown output format: ") +
                                                    format_str);
}

// ============================================================================
// Format-specific implementations
// ============================================================================

std::string OutputFormatter::format_as_text(const Bodies::BodyCollection& bodies,
                                            const Simulation::SimulationState& state,
                                            const OutputOptions& options) {
  std::ostringstream oss;

  // Set precision
  oss << std::scientific << std::setprecision(options.filter.position_precision);

  // Header
  if (options.include_header) {
    oss << "Solar System Simulation Output\n";
    oss << "Generated: " << format_timestamp(std::chrono::system_clock::now()) << "\n";
    oss << "Simulation Time: " << format_timestamp(state.reference_time) << "\n";
    oss << "\n";
  }

  // Barycenter
  if (options.filter.relative_to_barycenter) {
    oss << "Barycenter: " << format_vector(state.center_of_mass, options.filter.position_precision)
        << " m\n";
    oss << "\n";
  }

  // Bodies
  oss << std::setw(15) << "Body" << std::setw(21) << "X (m)" << std::setw(21) << "Y (m)"
      << std::setw(21) << "Z (m)" << std::setw(21) << "Distance (m)";

  if (options.filter.include_velocities) {
    oss << std::setw(21) << "VX (m/s)" << std::setw(21) << "VY (m/s)" << std::setw(21)
        << "VZ (m/s)";
  }
  oss << "\n";

  // Apply filter
  auto filtered_bodies = apply_filter(bodies, options.filter);

  for (const auto& body : filtered_bodies) {
    auto pos = body.position();
    if (options.filter.relative_to_barycenter) {
      pos = pos - state.center_of_mass;
    }

    long double distance = pos.magnitude();

    oss << std::setw(15) << body.name() << std::setw(21) << pos.x() << std::setw(21) << pos.y()
        << std::setw(21) << pos.z() << std::setw(21) << distance;

    if (options.filter.include_velocities) {
      const auto& vel = body.velocity();
      oss << std::setw(21) << vel.x() << std::setw(21) << vel.y() << std::setw(21) << vel.z();
    }

    oss << "\n";
  }

  // Metadata footer
  if (options.include_metadata && options.include_footer) {
    oss << "\n";
    oss << "Metadata:\n";
    oss << "  Bodies: " << filtered_bodies.size() << "\n";
    oss << "  Iterations: " << state.iteration_count << "\n";
    oss << "  Total Energy: " << state.total_energy << " J\n";
  }

  return oss.str();
}

std::string OutputFormatter::format_as_json(const Bodies::BodyCollection& bodies,
                                            const Simulation::SimulationState& state,
                                            const OutputOptions& options) {
  nlohmann::json doc;

  // Metadata
  if (options.include_metadata) {
    doc["metadata"] = {{"generated_at", format_timestamp(std::chrono::system_clock::now())},
                       {"simulation_time", format_timestamp(state.reference_time)},
                       {"iteration_count", state.iteration_count},
                       {"total_energy", state.total_energy}};
  }

  // Barycenter
  doc["barycenter"] = {{"x", static_cast<double>(state.center_of_mass.x())},
                       {"y", static_cast<double>(state.center_of_mass.y())},
                       {"z", static_cast<double>(state.center_of_mass.z())}};

  // Bodies
  nlohmann::json bodies_array = nlohmann::json::array();

  auto filtered_bodies = apply_filter(bodies, options.filter);
  for (const auto& body : filtered_bodies) {
    auto pos = body.position();
    if (options.filter.relative_to_barycenter) {
      pos = pos - state.center_of_mass;
    }

    nlohmann::json body_obj;
    body_obj["name"] = std::string(body.name());
    body_obj["mass"] = static_cast<double>(body.mass());
    body_obj["position"] = {{"x", static_cast<double>(pos.x())},
                            {"y", static_cast<double>(pos.y())},
                            {"z", static_cast<double>(pos.z())}};

    if (options.filter.include_velocities) {
      const auto& vel = body.velocity();
      body_obj["velocity"] = {{"x", static_cast<double>(vel.x())},
                              {"y", static_cast<double>(vel.y())},
                              {"z", static_cast<double>(vel.z())}};
    }

    bodies_array.push_back(body_obj);
  }

  doc["bodies"] = bodies_array;

  return doc.dump(2) + "\n";
}

std::string OutputFormatter::format_as_csv(const Bodies::BodyCollection& bodies,
                                           const Simulation::SimulationState& state,
                                           const OutputOptions& options) {
  std::ostringstream oss;

  // Header
  if (options.include_header) {
    oss << "Body,Mass,X,Y,Z,Distance";
    if (options.filter.include_velocities) {
      oss << ",VX,VY,VZ";
    }
    oss << "\n";
  }

  // Set precision
  oss << std::scientific << std::setprecision(options.filter.position_precision);

  // Bodies
  auto filtered_bodies = apply_filter(bodies, options.filter);
  for (const auto& body : filtered_bodies) {
    auto pos = body.position();
    if (options.filter.relative_to_barycenter) {
      pos = pos - state.center_of_mass;
    }

    long double distance = pos.magnitude();

    oss << body.name() << "," << body.mass() << "," << pos.x() << "," << pos.y() << "," << pos.z()
        << "," << distance;

    if (options.filter.include_velocities) {
      const auto& vel = body.velocity();
      oss << "," << vel.x() << "," << vel.y() << "," << vel.z();
    }

    oss << "\n";
  }

  return oss.str();
}

std::string OutputFormatter::format_as_xml(const Bodies::BodyCollection& bodies,
                                           const Simulation::SimulationState& state,
                                           const OutputOptions& options) {
  std::ostringstream oss;

  oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  oss << "<simulation>\n";

  // Metadata
  if (options.include_metadata) {
    oss << "  <metadata>\n";
    oss << "    <generated_at>" << format_timestamp(std::chrono::system_clock::now())
        << "</generated_at>\n";
    oss << "    <simulation_time>" << format_timestamp(state.reference_time)
        << "</simulation_time>\n";
    oss << "    <iteration_count>" << state.iteration_count << "</iteration_count>\n";
    oss << "    <total_energy>" << state.total_energy << "</total_energy>\n";
    oss << "  </metadata>\n";
  }

  // Barycenter
  oss << "  <barycenter>\n";
  oss << "    <x>" << state.center_of_mass.x() << "</x>\n";
  oss << "    <y>" << state.center_of_mass.y() << "</y>\n";
  oss << "    <z>" << state.center_of_mass.z() << "</z>\n";
  oss << "  </barycenter>\n";

  // Bodies
  oss << "  <bodies>\n";

  auto filtered_bodies = apply_filter(bodies, options.filter);
  for (const auto& body : filtered_bodies) {
    auto pos = body.position();
    if (options.filter.relative_to_barycenter) {
      pos = pos - state.center_of_mass;
    }

    oss << "    <body>\n";
    oss << "      <name>" << body.name() << "</name>\n";
    oss << "      <mass>" << body.mass() << "</mass>\n";
    oss << "      <position>\n";
    oss << "        <x>" << pos.x() << "</x>\n";
    oss << "        <y>" << pos.y() << "</y>\n";
    oss << "        <z>" << pos.z() << "</z>\n";
    oss << "      </position>\n";

    if (options.filter.include_velocities) {
      const auto& vel = body.velocity();
      oss << "      <velocity>\n";
      oss << "        <x>" << vel.x() << "</x>\n";
      oss << "        <y>" << vel.y() << "</y>\n";
      oss << "        <z>" << vel.z() << "</z>\n";
      oss << "      </velocity>\n";
    }

    oss << "    </body>\n";
  }

  oss << "  </bodies>\n";
  oss << "</simulation>\n";

  return oss.str();
}

std::string OutputFormatter::format_as_markdown(const Bodies::BodyCollection& bodies,
                                                const Simulation::SimulationState& state,
                                                const OutputOptions& options) {
  std::ostringstream oss;

  // Header
  if (options.include_header) {
    oss << "# Solar System Simulation Output\n\n";
    oss << "**Generated:** " << format_timestamp(std::chrono::system_clock::now()) << "\n\n";
    oss << "**Simulation Time:** " << format_timestamp(state.reference_time) << "\n\n";
  }

  // Metadata
  if (options.include_metadata) {
    oss << "## Simulation Metadata\n\n";
    oss << "- **Iterations:** " << state.iteration_count << "\n";
    oss << "- **Total Energy:** " << state.total_energy << " J\n";
    oss << "- **Barycenter:** " << format_vector(state.center_of_mass, 6) << " m\n\n";
  }

  // Bodies table
  oss << "## Celestial Bodies\n\n";
  oss << "| Body | X (m) | Y (m) | Z (m) | Distance (m) |";
  if (options.filter.include_velocities) {
    oss << " VX (m/s) | VY (m/s) | VZ (m/s) |";
  }
  oss << "\n";

  oss << "|------|-------|-------|-------|--------------|";
  if (options.filter.include_velocities) {
    oss << "----------|----------|----------|";
  }
  oss << "\n";

  // Set precision
  oss << std::scientific << std::setprecision(options.filter.position_precision);

  auto filtered_bodies = apply_filter(bodies, options.filter);
  for (const auto& body : filtered_bodies) {
    auto pos = body.position();
    if (options.filter.relative_to_barycenter) {
      pos = pos - state.center_of_mass;
    }

    long double distance = pos.magnitude();

    oss << "| " << body.name() << " | " << pos.x() << " | " << pos.y() << " | " << pos.z() << " | "
        << distance << " |";

    if (options.filter.include_velocities) {
      const auto& vel = body.velocity();
      oss << " " << vel.x() << " | " << vel.y() << " | " << vel.z() << " |";
    }

    oss << "\n";
  }

  return oss.str();
}

// ============================================================================
// Helper functions
// ============================================================================

std::string OutputFormatter::format_timestamp(std::chrono::system_clock::time_point tp) {
  auto time_t_val = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

std::string OutputFormatter::format_vector(const Math::Vector3d& vec, int precision) {
  std::ostringstream oss;
  oss << std::scientific << std::setprecision(precision);
  oss << "(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
  return oss.str();
}

Bodies::BodyCollection OutputFormatter::apply_filter(const Bodies::BodyCollection& bodies,
                                                     const OutputFilter& filter) {
  Bodies::BodyCollection filtered;

  for (const auto& body : bodies) {
    if (filter.should_include_body(body.name())) {
      filtered.add_body(body);
    }
  }

  return filtered;
}

// ============================================================================
// OutputArchiver Implementation
// ============================================================================

Utils::Expected<void, std::string> OutputArchiver::create_archive(
    const std::vector<std::filesystem::path>& files, const std::filesystem::path& archive_path,
    CompressionType compression) {
  try {
    std::ofstream archive(archive_path, std::ios::binary);
    if (!archive.is_open()) {
      return Utils::Expected<void, std::string>("Failed to create archive file");
    }

    // Write archive header: "SOLAR_ARCHIVE" + version
    const char* header = "SOLAR_ARCHIVE_V1";
    archive.write(header, 16);

    // Write number of files
    uint32_t file_count = static_cast<uint32_t>(files.size());
    archive.write(reinterpret_cast<const char*>(&file_count), sizeof(file_count));

    // Write each file
    for (const auto& file_path : files) {
      if (!std::filesystem::exists(file_path)) {
        return Utils::Expected<void, std::string>("File not found: " + file_path.string());
      }

      // Read file content
      std::ifstream file(file_path, std::ios::binary);
      if (!file.is_open()) {
        return Utils::Expected<void, std::string>("Failed to open file: " + file_path.string());
      }

      std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      file.close();

      // Compress if requested
      if (compression != CompressionType::NONE) {
        OutputFormatter formatter;
        content = formatter.compress(content, compression, 6);
      }

      // Write file entry: name_length + name + content_length + content
      std::string filename = file_path.filename().string();
      uint32_t name_length = static_cast<uint32_t>(filename.size());
      uint32_t content_length = static_cast<uint32_t>(content.size());

      archive.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
      archive.write(filename.c_str(), static_cast<std::streamsize>(name_length));
      archive.write(reinterpret_cast<const char*>(&content_length), sizeof(content_length));
      archive.write(content.c_str(), static_cast<std::streamsize>(content_length));
    }

    archive.close();
    return Utils::Expected<void, std::string>();

  } catch (const std::exception& ex) {
    return Utils::Expected<void, std::string>(std::string("Archive creation failed: ") + ex.what());
  }
}

Utils::Expected<std::vector<std::filesystem::path>, std::string> OutputArchiver::extract_archive(
    const std::filesystem::path& archive_path, const std::filesystem::path& destination) {
  try {
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive.is_open()) {
      return Utils::Expected<std::vector<std::filesystem::path>, std::string>(
          "Failed to open archive file");
    }

    // Read and verify header
    char header[16];
    archive.read(header, 16);
    if (std::string(header, 16) != "SOLAR_ARCHIVE_V1") {
      return Utils::Expected<std::vector<std::filesystem::path>, std::string>(
          "Invalid archive format");
    }

    // Read number of files
    uint32_t file_count;
    archive.read(reinterpret_cast<char*>(&file_count), sizeof(file_count));

    std::vector<std::filesystem::path> extracted_files;

    // Extract each file
    for (uint32_t i = 0; i < file_count; ++i) {
      // Read file name
      uint32_t name_length;
      archive.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));

      std::vector<char> name_buffer(name_length);
      archive.read(name_buffer.data(), static_cast<std::streamsize>(name_length));
      std::string filename(name_buffer.begin(), name_buffer.end());

      // Read file content
      uint32_t content_length;
      archive.read(reinterpret_cast<char*>(&content_length), sizeof(content_length));

      std::vector<char> content_buffer(content_length);
      archive.read(content_buffer.data(), static_cast<std::streamsize>(content_length));
      std::string content(content_buffer.begin(), content_buffer.end());

      // Write extracted file
      std::filesystem::path output_path = destination / filename;
      std::ofstream output(output_path, std::ios::binary);
      if (!output.is_open()) {
        return Utils::Expected<std::vector<std::filesystem::path>, std::string>(
            "Failed to create output file: " + output_path.string());
      }

      output.write(content.c_str(), static_cast<std::streamsize>(content.size()));
      output.close();

      extracted_files.push_back(output_path);
    }

    archive.close();
    return Utils::Expected<std::vector<std::filesystem::path>, std::string>(extracted_files);

  } catch (const std::exception& ex) {
    return Utils::Expected<std::vector<std::filesystem::path>, std::string>(
        std::string("Archive extraction failed: ") + ex.what());
  }
}

Utils::Expected<std::vector<std::string>, std::string> OutputArchiver::list_archive(
    const std::filesystem::path& archive_path) {
  try {
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive.is_open()) {
      return Utils::Expected<std::vector<std::string>, std::string>("Failed to open archive file");
    }

    // Read and verify header
    char header[16];
    archive.read(header, 16);
    if (std::string(header, 16) != "SOLAR_ARCHIVE_V1") {
      return Utils::Expected<std::vector<std::string>, std::string>("Invalid archive format");
    }

    // Read number of files
    uint32_t file_count;
    archive.read(reinterpret_cast<char*>(&file_count), sizeof(file_count));

    std::vector<std::string> file_list;

    // List each file
    for (uint32_t i = 0; i < file_count; ++i) {
      // Read file name
      uint32_t name_length;
      archive.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));

      std::vector<char> name_buffer(name_length);
      archive.read(name_buffer.data(), static_cast<std::streamsize>(name_length));
      std::string filename(name_buffer.begin(), name_buffer.end());

      file_list.push_back(filename);

      // Skip content
      uint32_t content_length;
      archive.read(reinterpret_cast<char*>(&content_length), sizeof(content_length));
      archive.seekg(content_length, std::ios::cur);
    }

    archive.close();
    return Utils::Expected<std::vector<std::string>, std::string>(file_list);

  } catch (const std::exception& ex) {
    return Utils::Expected<std::vector<std::string>, std::string>(
        std::string("Archive listing failed: ") + ex.what());
  }
}

// ============================================================================
// Utility functions
// ============================================================================

std::string to_string(OutputFormat format) {
  switch (format) {
    case OutputFormat::TEXT:
      return "text";
    case OutputFormat::JSON:
      return "json";
    case OutputFormat::CSV:
      return "csv";
    case OutputFormat::XML:
      return "xml";
    case OutputFormat::BINARY:
      return "binary";
    case OutputFormat::MARKDOWN:
      return "markdown";
    default:
      return "unknown";
  }
}

std::string to_string(QualityLevel quality) {
  switch (quality) {
    case QualityLevel::MINIMAL:
      return "minimal";
    case QualityLevel::STANDARD:
      return "standard";
    case QualityLevel::DETAILED:
      return "detailed";
    case QualityLevel::COMPREHENSIVE:
      return "comprehensive";
    default:
      return "unknown";
  }
}

std::string to_string(CompressionType compression) {
  switch (compression) {
    case CompressionType::NONE:
      return "none";
    case CompressionType::GZIP:
      return "gzip";
    case CompressionType::ZLIB:
      return "zlib";
    case CompressionType::CUSTOM:
      return "custom";
    default:
      return "unknown";
  }
}

}  // namespace SolarSystem::Output
