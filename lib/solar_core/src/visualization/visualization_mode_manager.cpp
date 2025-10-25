/**
 * @file visualization_mode_manager.cpp
 * @brief Implementation of visualization mode manager
 */

#include "solar_core/visualization/visualization_modes.hpp"

#include <fstream>
#include <sstream>

namespace SolarSystem::Visualization {

// VisualizationModeManager implementation
Utils::Expected<void, std::string> VisualizationModeManager::register_mode(
    VisualizationMode mode,
    std::unique_ptr<VisualizationRenderer> renderer) {

  if (!renderer) {
    return Utils::Expected<void, std::string>(std::string("Renderer cannot be null"));
  }

  renderers_[mode] = std::move(renderer);
  return Utils::Expected<void, std::string>();
}

Utils::Expected<void, std::string> VisualizationModeManager::set_active_mode(VisualizationMode mode) {
  auto it = renderers_.find(mode);
  if (it == renderers_.end()) {
    return Utils::Expected<void, std::string>(
        std::string("Visualization mode not registered: " + to_string(mode)));
  }

  active_mode_ = mode;
  return Utils::Expected<void, std::string>();
}

Utils::Expected<VisualizationFrame, std::string> VisualizationModeManager::render(
    const Streaming::DataSnapshot& snapshot) {

  auto it = renderers_.find(active_mode_);
  if (it == renderers_.end()) {
    return Utils::Expected<VisualizationFrame, std::string>(
        std::string("No renderer available for active mode: " + to_string(active_mode_)));
  }

  return it->second->render(snapshot);
}

std::vector<VisualizationMode> VisualizationModeManager::get_available_modes() const {
  std::vector<VisualizationMode> modes;
  for (const auto& [mode, renderer] : renderers_) {
    modes.push_back(mode);
  }
  return modes;
}

std::string VisualizationModeManager::get_mode_description(VisualizationMode mode) const {
  switch (mode) {
    case VisualizationMode::TABLE:
      return "Tabular display with columns for organized data viewing";
    case VisualizationMode::GRID:
      return "Grid layout with body cards for visual organization";
    case VisualizationMode::LIST:
      return "Simple list format for sequential data display";
    case VisualizationMode::TREE:
      return "Hierarchical tree view showing body relationships";
    case VisualizationMode::CHART:
      return "Chart/graph visualization for data analysis";
    case VisualizationMode::DASHBOARD:
      return "Multi-panel dashboard with comprehensive overview";
    case VisualizationMode::MINIMAL:
      return "Minimal compact view for essential information";
    case VisualizationMode::DETAILED:
      return "Detailed verbose view with comprehensive data";
    case VisualizationMode::CUSTOM:
      return "User-defined custom layout";
    default:
      return "Unknown visualization mode";
  }
}

Utils::Expected<void, std::string> VisualizationModeManager::save_preset(
    const std::string& name,
    const VisualizationConfig& config) {

  if (name.empty()) {
    return Utils::Expected<void, std::string>(std::string("Preset name cannot be empty"));
  }

  auto validation_result = config.validate();
  if (!validation_result) {
    return Utils::Expected<void, std::string>(
        std::string("Invalid configuration: " + validation_result.error()));
  }

  presets_[name] = config;
  return Utils::Expected<void, std::string>();
}

Utils::Expected<VisualizationConfig, std::string> VisualizationModeManager::load_preset(
    const std::string& name) {

  auto it = presets_.find(name);
  if (it == presets_.end()) {
    return Utils::Expected<VisualizationConfig, std::string>(
        std::string("Preset not found: " + name));
  }

  return Utils::Expected<VisualizationConfig, std::string>(it->second);
}

std::vector<std::string> VisualizationModeManager::get_available_presets() const {
  std::vector<std::string> preset_names;
  for (const auto& [name, config] : presets_) {
    preset_names.push_back(name);
  }
  return preset_names;
}

// Additional VisualizationRenderer methods
Utils::Expected<VisualizationFrame, std::string> VisualizationRenderer::render_bodies(
    const Bodies::BodyCollection& bodies,
    std::chrono::system_clock::time_point timestamp) {

  // Convert BodyCollection to DataSnapshot
  Streaming::DataSnapshot snapshot(bodies, timestamp);
  return render(snapshot);
}

Utils::Expected<void, std::string> VisualizationRenderer::handle_input(
    char key, int /*x*/, int /*y*/) {

  auto it = config_.controls.keyboard_shortcuts.find(key);
  if (it == config_.controls.keyboard_shortcuts.end()) {
    return Utils::Expected<void, std::string>(
        std::string("Unknown keyboard shortcut: " + std::string(1, key)));
  }

  const std::string& action = it->second;

  if (action == "help") {
    // Show help - would be handled by calling application
    return Utils::Expected<void, std::string>();
  } else if (action == "quit") {
    // Quit - would be handled by calling application
    return Utils::Expected<void, std::string>();
  } else if (action == "refresh") {
    // Refresh - would trigger data reload
    return Utils::Expected<void, std::string>();
  } else if (action == "sort") {
    // Toggle sort order
    sort_ascending_ = !sort_ascending_;
    return Utils::Expected<void, std::string>();
  } else if (action == "filter") {
    // Toggle filter - would open filter dialog
    return Utils::Expected<void, std::string>();
  } else if (action == "export") {
    // Export current view
    if (current_frame_) {
      auto export_result = export_current_view(ExportFormat::TEXT);
      if (!export_result) {
        return Utils::Expected<void, std::string>(
            std::string("Export failed: " + export_result.error().message));
      }
    }
    return Utils::Expected<void, std::string>();
  } else if (action == "pause") {
    // Pause/resume - would be handled by calling application
    return Utils::Expected<void, std::string>();
  } else if (action == "next_page") {
    current_page_++;
    return Utils::Expected<void, std::string>();
  } else if (action == "prev_page") {
    if (current_page_ > 0) {
      current_page_--;
    }
    return Utils::Expected<void, std::string>();
  }

  return Utils::Expected<void, std::string>(std::string("Unhandled action: " + action));
}

std::vector<std::string> VisualizationRenderer::get_available_actions() const {
  std::vector<std::string> actions;
  for (const auto& [key, action] : config_.controls.keyboard_shortcuts) {
    actions.push_back(std::string(1, key) + ": " + action);
  }
  return actions;
}

Utils::Expected<std::string, VisualizationFrame::ExportError> VisualizationRenderer::export_current_view(
    ExportFormat format,
    const std::map<std::string, std::string>& options) {

  if (!current_frame_) {
    return Utils::Expected<std::string, VisualizationFrame::ExportError>(
        VisualizationFrame::ExportError("No current frame to export"));
  }

  return current_frame_->export_to(format, options);
}

Utils::Expected<std::string, VisualizationFrame::ExportError> VisualizationRenderer::create_share_url(
    const VisualizationFrame& frame,
    const SharingOptions& options) {

  // Create a simple share URL (in a real implementation, this would
  // upload the data to a sharing service and return a URL)
  std::ostringstream url;
  url << "https://solarsystem.share/view?";
  url << "mode=" << to_string(frame.mode);
  url << "&timestamp=" << std::chrono::duration_cast<std::chrono::seconds>(
           frame.timestamp.time_since_epoch()).count();
  url << "&bodies=" << frame.body_count;

  if (options.include_metadata) {
    for (const auto& [key, value] : frame.metadata) {
      url << "&" << key << "=" << value;
    }
  }

  // Add expiry if specified
  if (options.share_expiry.count() > 0) {
    auto expiry_time = std::chrono::system_clock::now() + options.share_expiry;
    url << "&expires=" << std::chrono::duration_cast<std::chrono::seconds>(
             expiry_time.time_since_epoch()).count();
  }

  return Utils::Expected<std::string, VisualizationFrame::ExportError>(url.str());
}

}  // namespace SolarSystem::Visualization
