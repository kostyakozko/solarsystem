#pragma once

/**
 * @file visualization_modes.hpp
 * @brief Multiple visualization modes for real-time monitoring
 *
 * Provides various display formats, layouts, and interactive controls
 * for visualizing solar system data in different ways.
 */

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/streaming/data_stream.hpp"
#include "solar_core/utils/expected.hpp"

namespace SolarSystem::Visualization {

/**
 * @brief Available visualization modes
 */
enum class VisualizationMode {
  TABLE,          // Tabular display with columns
  GRID,           // Grid layout with body cards
  LIST,           // Simple list format
  TREE,           // Hierarchical tree view
  CHART,          // Chart/graph visualization
  DASHBOARD,      // Multi-panel dashboard
  MINIMAL,        // Minimal compact view
  DETAILED,       // Detailed verbose view
  CUSTOM          // User-defined custom layout
};

/**
 * @brief Display layout options
 */
enum class DisplayLayout {
  SINGLE_COLUMN,    // Single column layout
  MULTI_COLUMN,     // Multiple columns
  HORIZONTAL,       // Horizontal layout
  VERTICAL,         // Vertical layout
  GRID_2X2,         // 2x2 grid
  GRID_3X3,         // 3x3 grid
  ADAPTIVE,         // Adaptive based on terminal size
  CUSTOM            // Custom layout
};

/**
 * @brief Color schemes for visualization
 */
enum class ColorScheme {
  DEFAULT,          // Default terminal colors
  DARK,             // Dark theme
  LIGHT,            // Light theme
  HIGH_CONTRAST,    // High contrast for accessibility
  MONOCHROME,       // Monochrome/grayscale
  RAINBOW,          // Rainbow colors
  SCIENTIFIC,       // Scientific color palette
  CUSTOM            // User-defined colors
};

/**
 * @brief Data display options
 */
struct DataDisplayOptions {
  // Position display
  bool show_positions = true;
  bool show_velocities = false;
  bool show_accelerations = false;
  bool show_distances = false;
  bool show_relative_positions = false;

  // Units and formatting
  std::string position_units = "km";      // km, AU, m
  std::string velocity_units = "km/s";    // km/s, m/s, AU/day
  std::string distance_units = "km";      // km, AU, m
  int precision = 3;                      // Decimal places

  // Coordinate systems
  bool use_heliocentric = true;           // Heliocentric vs barycentric
  std::optional<std::string> reference_body; // Reference body for relative coords

  // Filtering
  std::vector<std::string> visible_bodies;
  std::vector<std::string> hidden_bodies;
  bool show_only_planets = false;
  bool show_only_moons = false;
};

/**
 * @brief Interactive control options
 */
struct InteractiveControls {
  // Navigation controls
  bool enable_scrolling = true;
  bool enable_paging = true;
  bool enable_sorting = true;
  bool enable_filtering = true;
  bool enable_search = true;

  // Display controls
  bool enable_zoom = true;
  bool enable_pan = false;
  bool enable_rotation = false;
  bool enable_time_control = true;

  // Export controls
  bool enable_export = true;
  bool enable_screenshot = true;
  bool enable_data_export = true;
  bool enable_sharing = true;

  // Keyboard shortcuts
  std::map<char, std::string> keyboard_shortcuts;
};

/**
 * @brief Export format options
 */
enum class ExportFormat {
  TEXT,             // Plain text
  CSV,              // Comma-separated values
  JSON,             // JSON format
  XML,              // XML format
  HTML,             // HTML table
  MARKDOWN,         // Markdown table
  PNG,              // PNG image (if supported)
  SVG,              // SVG vector graphics
  PDF               // PDF document
};

/**
 * @brief Sharing options
 */
struct SharingOptions {
  bool enable_url_sharing = true;
  bool enable_file_sharing = true;
  bool enable_clipboard = true;
  bool include_metadata = true;
  bool include_timestamp = true;
  std::chrono::seconds share_expiry{3600}; // 1 hour default
};

/**
 * @brief Complete visualization configuration
 */
struct VisualizationConfig {
  VisualizationMode mode = VisualizationMode::TABLE;
  DisplayLayout layout = DisplayLayout::ADAPTIVE;
  ColorScheme color_scheme = ColorScheme::DEFAULT;

  DataDisplayOptions data_options;
  InteractiveControls controls;
  SharingOptions sharing;

  // Update behavior
  std::chrono::milliseconds refresh_interval{1000};
  bool auto_refresh = true;
  bool smooth_transitions = true;

  // Terminal/display settings
  int max_width = 120;
  int max_height = 40;
  bool use_unicode = true;
  bool use_colors = true;

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] Utils::Expected<void, std::string> validate() const;

  /**
   * @brief Create default configuration for mode
   */
  [[nodiscard]] static VisualizationConfig create_default(VisualizationMode mode);
};

/**
 * @brief Rendered visualization frame
 */
struct VisualizationFrame {
  std::string content;
  std::chrono::system_clock::time_point timestamp;
  VisualizationMode mode;
  size_t body_count = 0;

  // Metadata
  std::map<std::string, std::string> metadata;

  // Interactive elements (for terminal UI)
  struct InteractiveElement {
    int x, y, width, height;
    std::string action;
    std::string tooltip;
  };
  std::vector<InteractiveElement> interactive_elements;

  /**
   * @brief Export frame to specified format
   */
  struct ExportError {
    std::string message;
    ExportError(std::string msg) : message(std::move(msg)) {}
  };

  [[nodiscard]] Utils::Expected<std::string, ExportError> export_to(
      ExportFormat format,
      const std::map<std::string, std::string>& options = {}) const;
};

/**
 * @brief Main visualization renderer
 */
class VisualizationRenderer {
public:
  explicit VisualizationRenderer(VisualizationConfig config = {});
  ~VisualizationRenderer() = default;

  // Configuration management
  void set_config(const VisualizationConfig& config) { config_ = config; }
  [[nodiscard]] const VisualizationConfig& get_config() const { return config_; }

  // Rendering
  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_bodies(
      const Bodies::BodyCollection& bodies,
      std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now());

  // Mode-specific rendering
  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_table(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_grid(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_list(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_tree(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_chart(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_dashboard(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_minimal(
      const Streaming::DataSnapshot& snapshot);

  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render_detailed(
      const Streaming::DataSnapshot& snapshot);

  // Interactive controls
  [[nodiscard]] Utils::Expected<void, std::string> handle_input(
      char key, int x = -1, int y = -1);

  [[nodiscard]] std::vector<std::string> get_available_actions() const;

  // Export and sharing
  [[nodiscard]] Utils::Expected<std::string, VisualizationFrame::ExportError> export_current_view(
      ExportFormat format,
      const std::map<std::string, std::string>& options = {});

  [[nodiscard]] Utils::Expected<std::string, VisualizationFrame::ExportError> create_share_url(
      const VisualizationFrame& frame,
      const SharingOptions& options = {});

private:
  VisualizationConfig config_;
  std::optional<VisualizationFrame> current_frame_;

  // State management
  int current_page_ = 0;
  std::string current_sort_field_ = "name";
  bool sort_ascending_ = true;
  std::string search_filter_;

  // Helper methods
  [[nodiscard]] std::string apply_color_scheme(const std::string& text, const std::string& color_key) const;
  [[nodiscard]] std::string format_position(const Math::Vector3d& pos) const;
  [[nodiscard]] std::string format_velocity(const Math::Vector3d& vel) const;
  [[nodiscard]] std::string format_distance(double distance) const;
  [[nodiscard]] std::vector<Streaming::DataPoint> filter_and_sort_data(
      const std::vector<Streaming::DataPoint>& data) const;

  // Layout helpers
  [[nodiscard]] std::string create_table_header() const;
  [[nodiscard]] std::string create_table_row(const Streaming::DataPoint& point) const;
  [[nodiscard]] std::string create_grid_cell(const Streaming::DataPoint& point) const;
  [[nodiscard]] std::string create_list_item(const Streaming::DataPoint& point) const;
};

/**
 * @brief Visualization mode manager
 */
class VisualizationModeManager {
public:
  VisualizationModeManager() = default;
  ~VisualizationModeManager() = default;

  // Mode management
  [[nodiscard]] Utils::Expected<void, std::string> register_mode(
      VisualizationMode mode,
      std::unique_ptr<VisualizationRenderer> renderer);

  [[nodiscard]] Utils::Expected<void, std::string> set_active_mode(VisualizationMode mode);
  [[nodiscard]] VisualizationMode get_active_mode() const { return active_mode_; }

  // Rendering with active mode
  [[nodiscard]] Utils::Expected<VisualizationFrame, std::string> render(
      const Streaming::DataSnapshot& snapshot);

  // Available modes
  [[nodiscard]] std::vector<VisualizationMode> get_available_modes() const;
  [[nodiscard]] std::string get_mode_description(VisualizationMode mode) const;

  // Presets
  [[nodiscard]] Utils::Expected<void, std::string> save_preset(
      const std::string& name,
      const VisualizationConfig& config);

  [[nodiscard]] Utils::Expected<VisualizationConfig, std::string> load_preset(
      const std::string& name);

  [[nodiscard]] std::vector<std::string> get_available_presets() const;

private:
  VisualizationMode active_mode_ = VisualizationMode::TABLE;
  std::map<VisualizationMode, std::unique_ptr<VisualizationRenderer>> renderers_;
  std::map<std::string, VisualizationConfig> presets_;
};

// Utility functions
[[nodiscard]] std::string to_string(VisualizationMode mode);
[[nodiscard]] std::string to_string(DisplayLayout layout);
[[nodiscard]] std::string to_string(ColorScheme scheme);
[[nodiscard]] std::string to_string(ExportFormat format);

[[nodiscard]] Utils::Expected<VisualizationMode, std::string> parse_visualization_mode(
    const std::string& mode_str);

[[nodiscard]] Utils::Expected<ExportFormat, std::string> parse_export_format(
    const std::string& format_str);

}  // namespace SolarSystem::Visualization
