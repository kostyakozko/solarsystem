#include "solar_utils/config.hpp"

#include <fstream>
#include <sstream>

namespace SolarSystem::Utils::Config {

AppConfig get_default() {
  AppConfig config;
  // All defaults are set in the struct definitions
  return config;
}

AppConfig load_from_file(const std::filesystem::path& config_file) {
  // Simple implementation - in a real system you'd use JSON/YAML parsing
  AppConfig config = get_default();

  if (!std::filesystem::exists(config_file)) {
    return config;
  }

  std::ifstream file(config_file);
  if (!file.is_open()) {
    return config;
  }

  std::string line;
  while (std::getline(file, line)) {
    // Simple key=value parsing
    size_t eq_pos = line.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = line.substr(0, eq_pos);
      std::string value = line.substr(eq_pos + 1);

      // Remove whitespace
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      // Apply configuration values
      if (key == "simulation.timestep") {
        config.simulation.timestep = std::stod(value);
      } else if (key == "simulation.max_iterations") {
        config.simulation.max_iterations = std::stoi(value);
      } else if (key == "debug_mode") {
        config.debug_mode = (value == "true" || value == "1");
      } else if (key == "output.format") {
        config.output.format = value;
      }
      // Add more configuration mappings as needed
    }
  }

  return config;
}

void save_to_file(const AppConfig& config, const std::filesystem::path& config_file) {
  std::ofstream file(config_file);
  if (!file.is_open()) {
    return;
  }

  file << "# Solar System Suite Configuration\n";
  file << "version=" << config.version << "\n";
  file << "debug_mode=" << (config.debug_mode ? "true" : "false") << "\n";
  file << "\n";

  file << "# Simulation Configuration\n";
  file << "simulation.timestep=" << config.simulation.timestep << "\n";
  file << "simulation.max_iterations=" << config.simulation.max_iterations << "\n";
  file << "simulation.enable_adaptive_timestep=" << (config.simulation.enable_adaptive_timestep ? "true" : "false") << "\n";
  file << "\n";

  file << "# Output Configuration\n";
  file << "output.format=" << config.output.format << "\n";
  file << "output.compress_output=" << (config.output.compress_output ? "true" : "false") << "\n";
  file << "\n";

  // Add more configuration sections as needed
}

}  // namespace SolarSystem::Utils::Config
