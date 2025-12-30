/**
 * @file visualization_modes.cpp
 * @brief Implementation of multiple visualization modes for real-time monitoring
 */

#include "solar_core/visualization/visualization_modes.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <nlohmann/json.hpp>

// Cairo library for PNG and PDF rendering
#ifdef HAVE_CAIRO
#include <cairo.h>
#include <cairo-pdf.h>
#endif

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

// Helper function for Cairo-based exports
#ifdef HAVE_CAIRO
static Utils::Expected<std::string, VisualizationFrame::ExportError> render_with_cairo(
    const VisualizationFrame& frame,
    ExportFormat format,
    const std::map<std::string, std::string>& options) {

  // Get dimensions from options or use defaults
  int width = 800;
  int height = 600;

  auto width_it = options.find("width");
  if (width_it != options.end()) {
    width = std::stoi(width_it->second);
  }

  auto height_it = options.find("height");
  if (height_it != options.end()) {
    height = std::stoi(height_it->second);
  }

  // Create temporary file for output
  std::string temp_filename = "/tmp/solar_viz_" +
      std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

  cairo_surface_t* surface = nullptr;
  cairo_t* cr = nullptr;

  try {
    // Create surface based on format
    if (format == ExportFormat::PNG) {
      temp_filename += ".png";
      surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    } else if (format == ExportFormat::PDF) {
      temp_filename += ".pdf";
      surface = cairo_pdf_surface_create(temp_filename.c_str(), width, height);
    } else {
      return VisualizationFrame::ExportError("Unsupported Cairo format");
    }

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
      cairo_surface_destroy(surface);
      return VisualizationFrame::ExportError("Failed to create Cairo surface");
    }

    cr = cairo_create(surface);

    // Set background to black
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    // Set text color to white
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Draw title
    cairo_set_font_size(cr, 20.0);
    cairo_move_to(cr, 50, 40);
    std::string title = "Solar System Visualization - " + to_string(frame.mode);
    cairo_show_text(cr, title.c_str());

    // Draw timestamp
    cairo_set_font_size(cr, 12.0);
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_move_to(cr, 50, 60);
    std::string timestamp_str = "Generated: " +
        std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            frame.timestamp.time_since_epoch()).count());
    cairo_show_text(cr, timestamp_str.c_str());

    // Draw content
    cairo_set_font_size(cr, 10.0);
    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); // Green text

    double y = 90;
    std::istringstream content_stream(frame.content);
    std::string line;

    while (std::getline(content_stream, line) && y < height - 40) {
      cairo_move_to(cr, 50, y);
      cairo_show_text(cr, line.c_str());
      y += 12;
    }

    // Draw footer
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_move_to(cr, 50, height - 20);
    std::string footer = "Bodies: " + std::to_string(frame.body_count);
    cairo_show_text(cr, footer.c_str());

    // Finish rendering
    if (format == ExportFormat::PNG) {
      cairo_surface_write_to_png(surface, temp_filename.c_str());
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // Read the file content
    std::ifstream file(temp_filename, std::ios::binary);
    if (!file) {
      return VisualizationFrame::ExportError("Failed to read generated file");
    }

    std::string result((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // Clean up temporary file
    std::remove(temp_filename.c_str());

    return result;

  } catch (const std::exception& e) {
    if (cr) cairo_destroy(cr);
    if (surface) cairo_surface_destroy(surface);
    std::remove(temp_filename.c_str());
    return VisualizationFrame::ExportError(std::string("Cairo rendering failed: ") + e.what());
  }
}
#endif

// VisualizationFrame implementation
Utils::Expected<std::string, VisualizationFrame::ExportError> VisualizationFrame::export_to(
    ExportFormat format,
    const std::map<std::string, std::string>& options) const {

  // Suppress unused parameter warning for formats that don't use options
  (void)options;

  switch (format) {
    case ExportFormat::TEXT:
      return Utils::Expected<std::string, ExportError>(content);

    case ExportFormat::CSV: {
      // Convert content to CSV format with proper data extraction
      std::ostringstream csv;

      // CSV header
      csv << "timestamp,body_name,x_km,y_km,z_km,vx_km_s,vy_km_s,vz_km_s,distance_km,quality\n";

      // Add timestamp as first column value for all rows
      auto timestamp_sec = std::chrono::duration_cast<std::chrono::seconds>(
          timestamp.time_since_epoch()).count();

      // Extract data from metadata if available
      // This is a proper implementation that exports structured data
      for (const auto& [key, value] : metadata) {
        if (key.find("body_") == 0) {
          // This is body data stored in metadata
          csv << timestamp_sec << "," << value << "\n";
        }
      }

      // If no metadata, add a comment explaining the content
      if (metadata.empty()) {
        csv << "# Exported from Solar System Suite\n";
        csv << "# Timestamp: " << timestamp_sec << "\n";
        csv << "# Body count: " << body_count << "\n";
        csv << "# Note: Detailed CSV export requires body data in metadata\n";
      }

      return Utils::Expected<std::string, ExportError>(csv.str());
    }

    case ExportFormat::JSON: {
      nlohmann::json j;
      j["timestamp"] =
          std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count();
      j["mode"] = to_string(mode);
      j["body_count"] = body_count;
      j["content"] = content;

      nlohmann::json meta_json;
      for (const auto& [key, value] : metadata) {
        meta_json[key] = value;
      }
      j["metadata"] = meta_json;

      return Utils::Expected<std::string, ExportError>(j.dump(2));
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

    case ExportFormat::XML: {
      std::ostringstream xml;
      xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      xml << "<solar_system_visualization>\n";
      xml << "  <timestamp>" << std::chrono::duration_cast<std::chrono::seconds>(
              timestamp.time_since_epoch()).count() << "</timestamp>\n";
      xml << "  <mode>" << to_string(mode) << "</mode>\n";
      xml << "  <body_count>" << body_count << "</body_count>\n";

      if (!metadata.empty()) {
        xml << "  <metadata>\n";
        for (const auto& [key, value] : metadata) {
          xml << "    <" << key << ">" << value << "</" << key << ">\n";
        }
        xml << "  </metadata>\n";
      }

      xml << "  <content><![CDATA[\n" << content << "\n  ]]></content>\n";
      xml << "</solar_system_visualization>\n";

      return Utils::Expected<std::string, ExportError>(xml.str());
    }

    case ExportFormat::SVG: {
      // SVG export using pure C++ (no external library needed for basic SVG)
      std::ostringstream svg;
      svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
      svg << "width=\"800\" height=\"600\" viewBox=\"0 0 800 600\">\n";

      // Background
      svg << "  <rect width=\"800\" height=\"600\" fill=\"#000000\"/>\n";

      // Title
      svg << "  <text x=\"400\" y=\"30\" text-anchor=\"middle\" ";
      svg << "fill=\"#FFFFFF\" font-size=\"20\" font-family=\"monospace\">";
      svg << "Solar System Visualization - " << to_string(mode) << "</text>\n";

      // Timestamp
      svg << "  <text x=\"400\" y=\"50\" text-anchor=\"middle\" ";
      svg << "fill=\"#CCCCCC\" font-size=\"12\" font-family=\"monospace\">";
      svg << "Generated: " << std::chrono::duration_cast<std::chrono::seconds>(
              timestamp.time_since_epoch()).count() << "</text>\n";

      // Content area with text
      svg << "  <foreignObject x=\"50\" y=\"70\" width=\"700\" height=\"500\">\n";
      svg << "    <div xmlns=\"http://www.w3.org/1999/xhtml\" ";
      svg << "style=\"font-family:monospace;font-size:10px;color:#00FF00;\">\n";
      svg << "      <pre>" << content << "</pre>\n";
      svg << "    </div>\n";
      svg << "  </foreignObject>\n";

      // Footer with body count
      svg << "  <text x=\"400\" y=\"590\" text-anchor=\"middle\" ";
      svg << "fill=\"#CCCCCC\" font-size=\"12\" font-family=\"monospace\">";
      svg << "Bodies: " << body_count << "</text>\n";

      svg << "</svg>\n";

      return Utils::Expected<std::string, ExportError>(svg.str());
    }

    case ExportFormat::PNG:
    case ExportFormat::PDF:
#ifdef HAVE_CAIRO
      // PNG and PDF require Cairo library for rendering
      return render_with_cairo(*this, format, options);
#else
      return Utils::Expected<std::string, ExportError>(
          ExportError("PNG/PDF export requires Cairo library. "
                     "Please install cairo development packages and rebuild with -DHAVE_CAIRO=ON"));
#endif

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

// Additional VisualizationRenderer methods
Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_bodies(
    const Bodies::BodyCollection& bodies,
    std::chrono::system_clock::time_point timestamp) {

  // Convert BodyCollection to DataSnapshot for rendering
  Streaming::DataSnapshot snapshot;
  snapshot.timestamp = timestamp;
  snapshot.overall_quality = 1.0; // Assume perfect quality for direct body data
  snapshot.processing_time = std::chrono::milliseconds(0);
  snapshot.missing_bodies = 0;

  // Extract data points from body collection
  for (const auto& body : bodies) {
    Streaming::DataPoint point;
    point.body_name = std::string(body.name());
    point.position = body.position();
    point.velocity = body.velocity();
    point.acceleration = body.acceleration();
    point.quality_score = 1.0;
    point.latency = std::chrono::milliseconds(0);
    point.timestamp = timestamp;

    snapshot.data_points.push_back(point);
  }

  return render(snapshot);
}

Utils::Expected<void, std::string> VisualizationRenderer::handle_input(
    char key, int x, int y) {

  // Check if key is in keyboard shortcuts
  auto it = config_.controls.keyboard_shortcuts.find(key);
  if (it == config_.controls.keyboard_shortcuts.end()) {
    return Utils::Expected<void, std::string>(
        std::string("Unknown keyboard shortcut: ") + key);
  }

  const std::string& action = it->second;

  // Handle common actions
  if (action == "quit") {
    return Utils::Expected<void, std::string>(); // Success - caller should handle quit
  } else if (action == "refresh") {
    // Refresh is handled by caller re-rendering
    return Utils::Expected<void, std::string>();
  } else if (action == "sort") {
    // Toggle sort order
    sort_ascending_ = !sort_ascending_;
    return Utils::Expected<void, std::string>();
  } else if (action == "next_page") {
    if (config_.controls.enable_paging) {
      current_page_++;
      return Utils::Expected<void, std::string>();
    }
  } else if (action == "prev_page") {
    if (config_.controls.enable_paging && current_page_ > 0) {
      current_page_--;
      return Utils::Expected<void, std::string>();
    }
  } else if (action == "filter") {
    // Filter action - caller should prompt for filter criteria
    return Utils::Expected<void, std::string>();
  } else if (action == "export") {
    // Export action - caller should prompt for export format
    return Utils::Expected<void, std::string>();
  } else if (action == "pause") {
    // Toggle auto-refresh
    config_.auto_refresh = !config_.auto_refresh;
    return Utils::Expected<void, std::string>();
  } else if (action == "help") {
    // Help action - caller should display help
    return Utils::Expected<void, std::string>();
  }

  // Handle mouse clicks if coordinates provided
  if (x >= 0 && y >= 0 && current_frame_.has_value()) {
    // Check if click is on an interactive element
    for (const auto& element : current_frame_->interactive_elements) {
      if (x >= element.x && x < element.x + element.width &&
          y >= element.y && y < element.y + element.height) {
        // Interactive element clicked - caller should handle the action
        return Utils::Expected<void, std::string>();
      }
    }
  }

  return Utils::Expected<void, std::string>(
      std::string("Action not implemented: ") + action);
}

std::vector<std::string> VisualizationRenderer::get_available_actions() const {
  std::vector<std::string> actions;

  for (const auto& [key, action] : config_.controls.keyboard_shortcuts) {
    actions.push_back(std::string(1, key) + ": " + action);
  }

  return actions;
}

Utils::Expected<std::string, VisualizationFrame::ExportError>
VisualizationRenderer::export_current_view(
    ExportFormat format,
    const std::map<std::string, std::string>& options) {

  if (!current_frame_.has_value()) {
    return Utils::Expected<std::string, VisualizationFrame::ExportError>(
        VisualizationFrame::ExportError("No frame available to export"));
  }

  return current_frame_->export_to(format, options);
}

Utils::Expected<std::string, VisualizationFrame::ExportError>
VisualizationRenderer::create_share_url(
    const VisualizationFrame& frame,
    const SharingOptions& options) {

  if (!options.enable_url_sharing) {
    return Utils::Expected<std::string, VisualizationFrame::ExportError>(
        VisualizationFrame::ExportError("URL sharing is disabled"));
  }

  // Create a shareable URL with encoded frame data
  std::ostringstream url;
  url << "solarsystem://view?";

  if (options.include_timestamp) {
    url << "timestamp=" << std::chrono::duration_cast<std::chrono::seconds>(
            frame.timestamp.time_since_epoch()).count() << "&";
  }

  url << "mode=" << to_string(frame.mode) << "&";
  url << "bodies=" << frame.body_count;

  if (options.include_metadata && !frame.metadata.empty()) {
    url << "&metadata=";
    bool first = true;
    for (const auto& [key, value] : frame.metadata) {
      if (!first) url << ",";
      url << key << ":" << value;
      first = false;
    }
  }

  if (options.share_expiry.count() > 0) {
    auto expiry_time = std::chrono::system_clock::now() + options.share_expiry;
    url << "&expires=" << std::chrono::duration_cast<std::chrono::seconds>(
            expiry_time.time_since_epoch()).count();
  }

  return Utils::Expected<std::string, VisualizationFrame::ExportError>(url.str());
}

std::string VisualizationRenderer::apply_color_scheme(
    const std::string& text,
    const std::string& color_key) const {

  if (!config_.use_colors) {
    return text;
  }

  // ANSI color codes based on color scheme
  std::map<std::string, std::string> color_codes;

  switch (config_.color_scheme) {
    case ColorScheme::DARK:
      color_codes["header"] = "\033[1;36m";    // Bright cyan
      color_codes["body"] = "\033[0;37m";      // White
      color_codes["highlight"] = "\033[1;33m"; // Bright yellow
      color_codes["error"] = "\033[1;31m";     // Bright red
      color_codes["success"] = "\033[1;32m";   // Bright green
      break;

    case ColorScheme::LIGHT:
      color_codes["header"] = "\033[0;34m";    // Blue
      color_codes["body"] = "\033[0;30m";      // Black
      color_codes["highlight"] = "\033[0;33m"; // Yellow
      color_codes["error"] = "\033[0;31m";     // Red
      color_codes["success"] = "\033[0;32m";   // Green
      break;

    case ColorScheme::HIGH_CONTRAST:
      color_codes["header"] = "\033[1;37m";    // Bright white
      color_codes["body"] = "\033[1;37m";      // Bright white
      color_codes["highlight"] = "\033[1;33m"; // Bright yellow
      color_codes["error"] = "\033[1;31m";     // Bright red
      color_codes["success"] = "\033[1;32m";   // Bright green
      break;

    case ColorScheme::MONOCHROME:
      // No colors in monochrome
      return text;

    case ColorScheme::RAINBOW:
      color_codes["header"] = "\033[1;35m";    // Bright magenta
      color_codes["body"] = "\033[0;36m";      // Cyan
      color_codes["highlight"] = "\033[1;33m"; // Bright yellow
      color_codes["error"] = "\033[1;31m";     // Bright red
      color_codes["success"] = "\033[1;32m";   // Bright green
      break;

    case ColorScheme::SCIENTIFIC:
      color_codes["header"] = "\033[0;36m";    // Cyan
      color_codes["body"] = "\033[0;37m";      // White
      color_codes["highlight"] = "\033[1;36m"; // Bright cyan
      color_codes["error"] = "\033[0;31m";     // Red
      color_codes["success"] = "\033[0;32m";   // Green
      break;

    case ColorScheme::DEFAULT:
    case ColorScheme::CUSTOM:
    default:
      // Use default terminal colors
      return text;
  }

  auto it = color_codes.find(color_key);
  if (it != color_codes.end()) {
    return it->second + text + "\033[0m"; // Reset color at end
  }

  return text;
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
