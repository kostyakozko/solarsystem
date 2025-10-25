/**
 * @file visualization_modes.cpp
 * @brief Implementation of multiple visualization modes for real-time monitoring
 */

#include "solar_core/visualization/visualization_modes.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace SolarSystem::Visualization {

// VisualizationConfig implementation
Utils::Expected<void, std::string> VisualizationConfig::validate() const {
  if (refresh_interval < std::chrono::milliseconds{100}) {
    return Utils::Expected<void, std::string>(
        std::string("Refresh interval must be at least 100ms"));
  }

  if (max_width < 40 || max_width > 200) {
    return Utils::Expected<void, std::string>(
        std::string("Max width must be between 40 and 200 characters"));
  }

  if (max_height < 10 || max_height > 100) {
    return Utils::Expected<void, std::string>(
        std::string("Max height must be between 10 and 100 lines"));
  }

  if (data_options.precision < 0 || data_options.precision > 15) {
    return Utils::Expected<void, std::string>(
        std::string("Precision must be between 0 and 15 decimal places"));
  }

  return Utils::Expected<void, std::string>();
}

VisualizationConfig VisualizationConfig::create_default(VisualizationMode mode) {
  VisualizationConfig config;
  config.mode = mode;

  switch (mode) {
    case VisualizationMode::TABLE:
      config.layout = DisplayLayout::SINGLE_COLUMN;
      config.data_options.show_positions = true;
      config.data_options.show_velocities = false;
      config.data_options.precision = 2;
      break;

    case VisualizationMode::GRID:
      config.layout = DisplayLayout::GRID_3X3;
      config.data_options.show_positions = true;
      config.data_options.show_distances = true;
      config.data_options.precision = 1;
      break;

    case VisualizationMode::LIST:
      config.layout = DisplayLayout::VERTICAL;
      config.data_options.show_positions = true;
      config.data_options.precision = 3;
      break;

    case VisualizationMode::MINIMAL:
      config.layout = DisplayLayout::SINGLE_COLUMN;
      config.data_options.show_positions = true;
      config.data_options.precision = 0;
      config.max_height = 20;
      break;

    case VisualizationMode::DETAILED:
      config.layout = DisplayLayout::MULTI_COLUMN;
      config.data_options.show_positions = true;
      config.data_options.show_velocities = true;
      config.data_options.show_accelerations = true;
      config.data_options.show_distances = true;
      config.data_options.precision = 6;
      break;

    case VisualizationMode::DASHBOARD:
      config.layout = DisplayLayout::GRID_2X2;
      config.data_options.show_positions = true;
      config.data_options.show_velocities = true;
      config.data_options.precision = 2;
      break;

    default:
      // Use table defaults
      break;
  }

  return config;
}

// VisualizationFrame implementation
Utils::Expected<std::string, VisualizationFrame::ExportError> VisualizationFrame::export_to(
    ExportFormat format,
    const std::map<std::string, std::string>& /*options*/) const {

  switch (format) {
    case ExportFormat::TEXT:
      return Utils::Expected<std::string, ExportError>(content);

    case ExportFormat::CSV: {
      // Convert content to CSV format
      std::ostringstream csv;
      csv << "timestamp,body_name,x,y,z,vx,vy,vz\n";

      // Parse content and extract data (simplified implementation)
      // In a real implementation, this would parse the actual content
      csv << "# Exported from Solar System Suite at "
          << std::chrono::duration_cast<std::chrono::seconds>(
               timestamp.time_since_epoch()).count() << "\n";
      csv << "# Body count: " << body_count << "\n";
      csv << content; // Placeholder - would need proper CSV conversion

      return Utils::Expected<std::string, ExportError>(csv.str());
    }

    case ExportFormat::JSON: {
      std::ostringstream json;
      json << "{\n";
      json << "  \"timestamp\": " << std::chrono::duration_cast<std::chrono::seconds>(
                timestamp.time_since_epoch()).count() << ",\n";
      json << "  \"mode\": \"" << to_string(mode) << "\",\n";
      json << "  \"body_count\": " << body_count << ",\n";
      json << "  \"content\": " << std::quoted(content) << ",\n";
      json << "  \"metadata\": {\n";

      bool first = true;
      for (const auto& [key, value] : metadata) {
        if (!first) json << ",\n";
        json << "    " << std::quoted(key) << ": " << std::quoted(value);
        first = false;
      }

      json << "\n  }\n";
      json << "}\n";

      return Utils::Expected<std::string, ExportError>(json.str());
    }

    case ExportFormat::HTML: {
      std::ostringstream html;
      html << "<!DOCTYPE html>\n<html>\n<head>\n";
      html << "<title>Solar System Visualization</title>\n";
      html << "<style>body{font-family:monospace;background:#000;color:#fff;}</style>\n";
      html << "</head>\n<body>\n";
      html << "<h1>Solar System Visualization</h1>\n";
      html << "<p>Generated: " << std::chrono::duration_cast<std::chrono::seconds>(
                timestamp.time_since_epoch()).count() << "</p>\n";
      html << "<pre>" << content << "</pre>\n";
      html << "</body>\n</html>\n";

      return Utils::Expected<std::string, ExportError>(html.str());
    }

    case ExportFormat::MARKDOWN: {
      std::ostringstream md;
      md << "# Solar System Visualization\n\n";
      md << "**Mode:** " << to_string(mode) << "\n";
      md << "**Bodies:** " << body_count << "\n";
      md << "**Generated:** " << std::chrono::duration_cast<std::chrono::seconds>(
              timestamp.time_since_epoch()).count() << "\n\n";
      md << "```\n" << content << "\n```\n";

      return Utils::Expected<std::string, ExportError>(md.str());
    }

    default:
      return Utils::Expected<std::string, ExportError>(
          ExportError("Unsupported export format: " + to_string(format)));
  }
}

// VisualizationRenderer implementation
VisualizationRenderer::VisualizationRenderer(VisualizationConfig config)
    : config_(std::move(config)) {
  // Initialize default keyboard shortcuts
  config_.controls.keyboard_shortcuts['h'] = "help";
  config_.controls.keyboard_shortcuts['q'] = "quit";
  config_.controls.keyboard_shortcuts['r'] = "refresh";
  config_.controls.keyboard_shortcuts['s'] = "sort";
  config_.controls.keyboard_shortcuts['f'] = "filter";
  config_.controls.keyboard_shortcuts['e'] = "export";
  config_.controls.keyboard_shortcuts[' '] = "pause";
  config_.controls.keyboard_shortcuts['n'] = "next_page";
  config_.controls.keyboard_shortcuts['p'] = "prev_page";
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render(
    const Streaming::DataSnapshot& snapshot) {

  switch (config_.mode) {
    case VisualizationMode::TABLE:
      return render_table(snapshot);
    case VisualizationMode::GRID:
      return render_grid(snapshot);
    case VisualizationMode::LIST:
      return render_list(snapshot);
    case VisualizationMode::TREE:
      return render_tree(snapshot);
    case VisualizationMode::CHART:
      return render_chart(snapshot);
    case VisualizationMode::DASHBOARD:
      return render_dashboard(snapshot);
    case VisualizationMode::MINIMAL:
      return render_minimal(snapshot);
    case VisualizationMode::DETAILED:
      return render_detailed(snapshot);
    default:
      return Utils::Expected<VisualizationFrame, std::string>(
          std::string("Unsupported visualization mode"));
  }
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_table(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  // Filter and sort data
  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  // Create table header
  output << create_table_header() << "\n";

  // Add separator line
  output << std::string(static_cast<size_t>(config_.max_width - 2), '-') << "\n";

  // Add data rows
  for (const auto& point : filtered_data) {
    output << create_table_row(point) << "\n";
  }

  // Add footer with statistics
  if (config_.data_options.show_distances) {
    output << std::string(static_cast<size_t>(config_.max_width - 2), '-') << "\n";
    output << "Total bodies: " << filtered_data.size()
           << " | Quality: " << std::fixed << std::setprecision(1)
           << (snapshot.overall_quality * 100.0) << "%\n";
  }

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::TABLE;
  frame.body_count = filtered_data.size();
  frame.metadata["quality"] = std::to_string(snapshot.overall_quality);
  frame.metadata["processing_time"] = std::to_string(snapshot.processing_time.count());

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_grid(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  // Determine grid dimensions
  int cols = 3; // Default for 3x3 grid
  int rows = static_cast<int>((filtered_data.size() + static_cast<size_t>(cols) - 1) / static_cast<size_t>(cols));

  if (config_.layout == DisplayLayout::GRID_2X2) {
    cols = 2;
    rows = static_cast<int>((filtered_data.size() + static_cast<size_t>(cols) - 1) / static_cast<size_t>(cols));
  }

  // Calculate cell width
  int cell_width = (config_.max_width - cols - 1) / cols;

  for (int row = 0; row < rows && row * cols < static_cast<int>(filtered_data.size()); ++row) {
    for (int col = 0; col < cols; ++col) {
      int index = row * cols + col;
      if (index < static_cast<int>(filtered_data.size())) {
        std::string cell = create_grid_cell(filtered_data[static_cast<size_t>(index)]);
        // Truncate or pad to cell width
        if (cell.length() > static_cast<size_t>(cell_width)) {
          cell = cell.substr(0, static_cast<size_t>(cell_width - 3)) + "...";
        } else {
          cell.resize(static_cast<size_t>(cell_width), ' ');
        }
        output << cell;
      } else {
        output << std::string(static_cast<size_t>(cell_width), ' ');
      }

      if (col < cols - 1) {
        output << "|";
      }
    }
    output << "\n";

    // Add separator line between rows
    if (row < rows - 1) {
      for (int col = 0; col < cols; ++col) {
        output << std::string(static_cast<size_t>(cell_width), '-');
        if (col < cols - 1) {
          output << "+";
        }
      }
      output << "\n";
    }
  }

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::GRID;
  frame.body_count = filtered_data.size();

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_list(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  for (size_t i = 0; i < filtered_data.size(); ++i) {
    output << (i + 1) << ". " << create_list_item(filtered_data[i]) << "\n";
  }

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::LIST;
  frame.body_count = filtered_data.size();

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_minimal(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  // Show only essential information in compact format
  for (const auto& point : filtered_data) {
    output << std::setw(12) << std::left << point.body_name.substr(0, 11) << " ";
    output << format_position(point.position) << "\n";
  }

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::MINIMAL;
  frame.body_count = filtered_data.size();

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_detailed(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  for (const auto& point : filtered_data) {
    output << "=== " << point.body_name << " ===\n";
    output << "Position: " << format_position(point.position) << "\n";

    if (config_.data_options.show_velocities) {
      output << "Velocity: " << format_velocity(point.velocity) << "\n";
    }

    if (config_.data_options.show_accelerations) {
      output << "Acceleration: " << format_velocity(point.acceleration) << "\n";
    }

    if (config_.data_options.show_distances) {
      double distance = std::sqrt(
          point.position.x() * point.position.x() +
          point.position.y() * point.position.y() +
          point.position.z() * point.position.z());
      output << "Distance from origin: " << format_distance(distance) << "\n";
    }

    output << "Quality: " << std::fixed << std::setprecision(1)
           << (point.quality_score * 100.0) << "%\n";
    output << "Latency: " << point.latency.count() << "ms\n";
    output << "\n";
  }

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::DETAILED;
  frame.body_count = filtered_data.size();

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_dashboard(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  // Dashboard layout with multiple panels
  int panel_width = config_.max_width / 2 - 2;

  // Top panels: Summary and Quality
  output << "+" << std::string(static_cast<size_t>(panel_width), '-') << "+"
         << std::string(static_cast<size_t>(panel_width), '-') << "+\n";

  output << "| SUMMARY" << std::string(static_cast<size_t>(panel_width - 8), ' ') << "|";
  output << " QUALITY" << std::string(static_cast<size_t>(panel_width - 8), ' ') << "|\n";

  output << "| Bodies: " << std::setw(panel_width - 9) << std::left
         << filtered_data.size() << "|";
  output << " Overall: " << std::setw(panel_width - 10) << std::left
         << (std::to_string(static_cast<int>(snapshot.overall_quality * 100)) + "%") << "|\n";

  output << "+" << std::string(static_cast<size_t>(panel_width), '-') << "+"
         << std::string(static_cast<size_t>(panel_width), '-') << "+\n";

  // Bottom panels: Recent data
  output << "| RECENT DATA" << std::string(static_cast<size_t>(panel_width - 12), ' ') << "|";
  output << " STATISTICS" << std::string(static_cast<size_t>(panel_width - 12), ' ') << "|\n";

  // Show first few bodies in left panel
  int max_bodies = std::min(5, static_cast<int>(filtered_data.size()));
  for (int i = 0; i < max_bodies; ++i) {
    std::string body_line = filtered_data[static_cast<size_t>(i)].body_name.substr(0, static_cast<size_t>(panel_width - 2));
    body_line.resize(static_cast<size_t>(panel_width - 1), ' ');
    output << "|" << body_line << "|";

    if (i == 0) {
      output << " Avg Latency: " << std::setw(panel_width - 14) << std::left
             << (std::to_string(snapshot.processing_time.count()) + "ms") << "|\n";
    } else if (i == 1) {
      output << " Missing: " << std::setw(panel_width - 10) << std::left
             << snapshot.missing_bodies << "|\n";
    } else {
      output << std::string(static_cast<size_t>(panel_width), ' ') << "|\n";
    }
  }

  output << "+" << std::string(static_cast<size_t>(panel_width), '-') << "+"
         << std::string(static_cast<size_t>(panel_width), '-') << "+\n";

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::DASHBOARD;
  frame.body_count = filtered_data.size();

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_tree(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  // Group bodies by type for tree structure
  std::map<std::string, std::vector<Streaming::DataPoint>> grouped_bodies;

  for (const auto& point : filtered_data) {
    std::string category;
    if (point.body_name == "Sun") {
      category = "Star";
    } else if (point.body_name.find("Moon") != std::string::npos ||
               point.body_name == "Phobos" || point.body_name == "Deimos" ||
               point.body_name == "Io" || point.body_name == "Europa" ||
               point.body_name == "Ganymede" || point.body_name == "Callisto") {
      category = "Moons";
    } else {
      category = "Planets";
    }
    grouped_bodies[category].push_back(point);
  }

  // Render tree structure
  for (const auto& [category, bodies] : grouped_bodies) {
    output << "+- " << category << " (" << bodies.size() << ")\n";

    for (size_t i = 0; i < bodies.size(); ++i) {
      bool is_last = (i == bodies.size() - 1);
      output << (is_last ? "+- " : "+- ");
      output << bodies[i].body_name << " - " << format_position(bodies[i].position) << "\n";
    }
  }

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::TREE;
  frame.body_count = filtered_data.size();

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_chart(
    const Streaming::DataSnapshot& snapshot) {

  std::ostringstream output;
  VisualizationFrame frame;

  auto filtered_data = filter_and_sort_data(snapshot.data_points);

  // Simple ASCII chart showing distances from origin
  output << "Distance Chart (relative scale)\n";
  output << std::string(static_cast<size_t>(config_.max_width - 2), '=') << "\n";

  // Calculate distances and find max for scaling
  std::vector<std::pair<std::string, double>> distances;
  double max_distance = 0.0;

  for (const auto& point : filtered_data) {
    double distance = std::sqrt(
        point.position.x() * point.position.x() +
        point.position.y() * point.position.y() +
        point.position.z() * point.position.z());
    distances.emplace_back(point.body_name, distance);
    max_distance = std::max(max_distance, distance);
  }

  // Sort by distance
  std::sort(distances.begin(), distances.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

  // Render chart bars
  int max_bar_width = config_.max_width - 20; // Leave space for labels

  for (const auto& [name, distance] : distances) {
    int bar_width = max_distance > 0 ?
        static_cast<int>((distance / max_distance) * max_bar_width) : 0;

    output << std::setw(12) << std::left << name.substr(0, 11) << " |";
    output << std::string(static_cast<size_t>(bar_width), '#');
    output << " " << format_distance(distance) << "\n";
  }

  frame.content = output.str();
  frame.timestamp = snapshot.timestamp;
  frame.mode = VisualizationMode::CHART;
  frame.body_count = filtered_data.size();

  current_frame_ = frame;
  return Utils::Expected<VisualizationFrame, std::string>(frame);
}

// Helper methods implementation
std::string VisualizationRenderer::format_position(const Math::Vector3d& pos) const {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(config_.data_options.precision);

  if (config_.data_options.position_units == "km") {
    ss << "(" << (pos.x() / 1000.0) << ", " << (pos.y() / 1000.0) << ", " << (pos.z() / 1000.0) << ")";
  } else if (config_.data_options.position_units == "AU") {
    const double AU = 149597870.7; // km
    ss << "(" << (pos.x() / 1000.0 / AU) << ", " << (pos.y() / 1000.0 / AU) << ", " << (pos.z() / 1000.0 / AU) << ")";
  } else {
    ss << "(" << pos.x() << ", " << pos.y() << ", " << pos.z() << ")";
  }

  return ss.str();
}

std::string VisualizationRenderer::format_velocity(const Math::Vector3d& vel) const {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(config_.data_options.precision);

  if (config_.data_options.velocity_units == "km/s") {
    ss << "(" << (vel.x() / 1000.0) << ", " << (vel.y() / 1000.0) << ", " << (vel.z() / 1000.0) << ")";
  } else {
    ss << "(" << vel.x() << ", " << vel.y() << ", " << vel.z() << ")";
  }

  return ss.str();
}

std::string VisualizationRenderer::format_distance(double distance) const {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(config_.data_options.precision);

  if (config_.data_options.distance_units == "km") {
    ss << (distance / 1000.0) << " km";
  } else if (config_.data_options.distance_units == "AU") {
    const double AU = 149597870.7; // km
    ss << (distance / 1000.0 / AU) << " AU";
  } else {
    ss << distance << " m";
  }

  return ss.str();
}

std::vector<Streaming::DataPoint> VisualizationRenderer::filter_and_sort_data(
    const std::vector<Streaming::DataPoint>& data) const {

  std::vector<Streaming::DataPoint> filtered_data;

  // Apply filters
  for (const auto& point : data) {
    bool include = true;

    // Check visible bodies filter
    if (!config_.data_options.visible_bodies.empty()) {
      include = std::find(config_.data_options.visible_bodies.begin(),
                         config_.data_options.visible_bodies.end(),
                         point.body_name) != config_.data_options.visible_bodies.end();
    }

    // Check hidden bodies filter
    if (include && !config_.data_options.hidden_bodies.empty()) {
      include = std::find(config_.data_options.hidden_bodies.begin(),
                         config_.data_options.hidden_bodies.end(),
                         point.body_name) == config_.data_options.hidden_bodies.end();
    }

    // Apply search filter
    if (include && !search_filter_.empty()) {
      include = point.body_name.find(search_filter_) != std::string::npos;
    }

    if (include) {
      filtered_data.push_back(point);
    }
  }

  // Apply sorting
  if (current_sort_field_ == "name") {
    std::sort(filtered_data.begin(), filtered_data.end(),
              [this](const auto& a, const auto& b) {
                return sort_ascending_ ? a.body_name < b.body_name : a.body_name > b.body_name;
              });
  } else if (current_sort_field_ == "distance") {
    std::sort(filtered_data.begin(), filtered_data.end(),
              [this](const auto& a, const auto& b) {
                double dist_a = std::sqrt(a.position.x() * a.position.x() +
                                        a.position.y() * a.position.y() +
                                        a.position.z() * a.position.z());
                double dist_b = std::sqrt(b.position.x() * b.position.x() +
                                        b.position.y() * b.position.y() +
                                        b.position.z() * b.position.z());
                return sort_ascending_ ? dist_a < dist_b : dist_a > dist_b;
              });
  }

  return filtered_data;
}

std::string VisualizationRenderer::create_table_header() const {
  std::ostringstream header;

  header << std::setw(15) << std::left << "Body";

  if (config_.data_options.show_positions) {
    header << std::setw(45) << std::left << "Position (" + config_.data_options.position_units + ")";
  }

  if (config_.data_options.show_velocities) {
    header << std::setw(35) << std::left << "Velocity (" + config_.data_options.velocity_units + ")";
  }

  if (config_.data_options.show_distances) {
    header << std::setw(15) << std::left << "Distance";
  }

  return header.str();
}

std::string VisualizationRenderer::create_table_row(const Streaming::DataPoint& point) const {
  std::ostringstream row;

  row << std::setw(15) << std::left << point.body_name.substr(0, 14);

  if (config_.data_options.show_positions) {
    row << std::setw(45) << std::left << format_position(point.position);
  }

  if (config_.data_options.show_velocities) {
    row << std::setw(35) << std::left << format_velocity(point.velocity);
  }

  if (config_.data_options.show_distances) {
    double distance = std::sqrt(
        point.position.x() * point.position.x() +
        point.position.y() * point.position.y() +
        point.position.z() * point.position.z());
    row << std::setw(15) << std::left << format_distance(distance);
  }

  return row.str();
}

std::string VisualizationRenderer::create_grid_cell(const Streaming::DataPoint& point) const {
  std::ostringstream cell;

  cell << point.body_name.substr(0, 10) << "\n";
  cell << format_position(point.position);

  return cell.str();
}

std::string VisualizationRenderer::create_list_item(const Streaming::DataPoint& point) const {
  std::ostringstream item;

  item << point.body_name << ": " << format_position(point.position);

  if (config_.data_options.show_velocities) {
    item << " | v=" << format_velocity(point.velocity);
  }

  return item.str();
}

// Utility functions
std::string to_string(VisualizationMode mode) {
  switch (mode) {
    case VisualizationMode::TABLE: return "table";
    case VisualizationMode::GRID: return "grid";
    case VisualizationMode::LIST: return "list";
    case VisualizationMode::TREE: return "tree";
    case VisualizationMode::CHART: return "chart";
    case VisualizationMode::DASHBOARD: return "dashboard";
    case VisualizationMode::MINIMAL: return "minimal";
    case VisualizationMode::DETAILED: return "detailed";
    case VisualizationMode::CUSTOM: return "custom";
    default: return "unknown";
  }
}

std::string to_string(DisplayLayout layout) {
  switch (layout) {
    case DisplayLayout::SINGLE_COLUMN: return "single_column";
    case DisplayLayout::MULTI_COLUMN: return "multi_column";
    case DisplayLayout::HORIZONTAL: return "horizontal";
    case DisplayLayout::VERTICAL: return "vertical";
    case DisplayLayout::GRID_2X2: return "grid_2x2";
    case DisplayLayout::GRID_3X3: return "grid_3x3";
    case DisplayLayout::ADAPTIVE: return "adaptive";
    case DisplayLayout::CUSTOM: return "custom";
    default: return "unknown";
  }
}

std::string to_string(ColorScheme scheme) {
  switch (scheme) {
    case ColorScheme::DEFAULT: return "default";
    case ColorScheme::DARK: return "dark";
    case ColorScheme::LIGHT: return "light";
    case ColorScheme::HIGH_CONTRAST: return "high_contrast";
    case ColorScheme::MONOCHROME: return "monochrome";
    case ColorScheme::RAINBOW: return "rainbow";
    case ColorScheme::SCIENTIFIC: return "scientific";
    case ColorScheme::CUSTOM: return "custom";
    default: return "unknown";
  }
}

std::string to_string(ExportFormat format) {
  switch (format) {
    case ExportFormat::TEXT: return "text";
    case ExportFormat::CSV: return "csv";
    case ExportFormat::JSON: return "json";
    case ExportFormat::XML: return "xml";
    case ExportFormat::HTML: return "html";
    case ExportFormat::MARKDOWN: return "markdown";
    case ExportFormat::PNG: return "png";
    case ExportFormat::SVG: return "svg";
    case ExportFormat::PDF: return "pdf";
    default: return "unknown";
  }
}

Utils::Expected<VisualizationMode, std::string> parse_visualization_mode(const std::string& mode_str) {
  if (mode_str == "table") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::TABLE);
  if (mode_str == "grid") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::GRID);
  if (mode_str == "list") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::LIST);
  if (mode_str == "tree") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::TREE);
  if (mode_str == "chart") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::CHART);
  if (mode_str == "dashboard") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::DASHBOARD);
  if (mode_str == "minimal") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::MINIMAL);
  if (mode_str == "detailed") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::DETAILED);
  if (mode_str == "custom") return Utils::Expected<VisualizationMode, std::string>(VisualizationMode::CUSTOM);

  return Utils::Expected<VisualizationMode, std::string>(std::string("Unknown visualization mode: " + mode_str));
}

Utils::Expected<ExportFormat, std::string> parse_export_format(const std::string& format_str) {
  if (format_str == "text") return Utils::Expected<ExportFormat, std::string>(ExportFormat::TEXT);
  if (format_str == "csv") return Utils::Expected<ExportFormat, std::string>(ExportFormat::CSV);
  if (format_str == "json") return Utils::Expected<ExportFormat, std::string>(ExportFormat::JSON);
  if (format_str == "xml") return Utils::Expected<ExportFormat, std::string>(ExportFormat::XML);
  if (format_str == "html") return Utils::Expected<ExportFormat, std::string>(ExportFormat::HTML);
  if (format_str == "markdown") return Utils::Expected<ExportFormat, std::string>(ExportFormat::MARKDOWN);
  if (format_str == "png") return Utils::Expected<ExportFormat, std::string>(ExportFormat::PNG);
  if (format_str == "svg") return Utils::Expected<ExportFormat, std::string>(ExportFormat::SVG);
  if (format_str == "pdf") return Utils::Expected<ExportFormat, std::string>(ExportFormat::PDF);

  return Utils::Expected<ExportFormat, std::string>(std::string("Unknown export format: " + format_str));
}

}  // namespace SolarSystem::Visualization
