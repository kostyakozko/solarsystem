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
        "Visualization mode not registered: " + to_string(mode));
  }

  active_mode_ = mode;
  return Utils::Expected<void, std::string>();
}

Utils::Expected<VisualizationFrame, std::string> VisualizationModeManager::render(
    const Streaming::DataSnapshot& snapshot) {

  auto it = renderers_.find(active_mode_);
  if (it == renderers_.end()) {
    return Utils::Expected<VisualizationFrame, std::string>(
        "No renderer available for active mode: " + to_string(active_mode_));
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
        "Invalid configuration: " + validation_result.error());
  }

  presets_[name] = config;
  return Utils::Expected<void, std::string>();
}

Utils::Expected<VisualizationConfig, std::string> VisualizationModeManager::load_preset(
    const std::string& name) {

  auto it = presets_.find(name);
  if (it == presets_.end()) {
    return Utils::Expected<VisualizationConfig, std::string>(
        "Preset not found: " + name);
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

}  // namespace SolarSystem::Visualization
