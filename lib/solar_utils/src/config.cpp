#include "solar_utils/config.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace SolarSystem::Utils {

Expected<Config::AppConfig, std::string> Config::load(const std::optional<std::string>& config_file,
                                                      const std::vector<std::string>& cli_args) {
  // Start with default configuration
  AppConfig config = get_default();

  // Load from config file if provided
  if (config_file.has_value()) {
    auto file_result = load_from_file(*config_file);
    if (!file_result.has_value()) {
      return Expected<AppConfig, std::string>::error("Failed to load config file: " +
                                                     file_result.error());
    }

    auto file_config_result = map_to_config(*file_result);
    if (!file_config_result.has_value()) {
      return Expected<AppConfig, std::string>::error("Invalid config file format: " +
                                                     file_config_result.error());
    }

    config = merge(config, *file_config_result);
  }

  // Load from environment variables
  auto env_map = load_from_environment();
  if (!env_map.empty()) {
    auto env_config_result = map_to_config(env_map);
    if (env_config_result.has_value()) {
      config = merge(config, *env_config_result);
    }
  }

  // Load from command line (highest priority)
  auto cli_map = load_from_cli(cli_args);
  if (!cli_map.empty()) {
    auto cli_config_result = map_to_config(cli_map);
    if (cli_config_result.has_value()) {
      config = merge(config, *cli_config_result);
    }
  }

  // Validate final configuration
  auto validation_result = validate(config);
  if (!validation_result.has_value()) {
    return Expected<AppConfig, std::string>::error("Configuration validation failed: " +
                                                   validation_result.error());
  }

  return Expected<AppConfig, std::string>::success(config);
}

Expected<void, std::string> Config::save(const AppConfig& config, const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    return Expected<void, std::string>::error("Failed to open file for writing: " + filename);
  }

  // Simple JSON-like format (could be enhanced with proper JSON library)
  file << "# Solar System Suite Configuration\n";
  file << "# Generated automatically - edit with care\n\n";

  file << "[simulation]\n";
  file << "timestep = " << config.simulation.timestep << "\n";
  file << "max_iterations = " << config.simulation.max_iterations << "\n";
  file << "enable_progress = " << (config.simulation.enable_progress ? "true" : "false") << "\n";
  file << "verbose_output = " << (config.simulation.verbose_output ? "true" : "false") << "\n";
  file << "output_format = \"" << config.simulation.output_format << "\"\n";
  file << "gravitational_constant = " << config.simulation.gravitational_constant << "\n";
  file << "convergence_threshold = " << config.simulation.convergence_threshold << "\n";
  file << "thread_count = " << config.simulation.thread_count << "\n";
  file << "enable_simd = " << (config.simulation.enable_simd ? "true" : "false") << "\n";
  file << "enable_lto = " << (config.simulation.enable_lto ? "true" : "false") << "\n\n";

  file << "[data]\n";
  file << "jpl_api_url = \"" << config.data.jpl_api_url << "\"\n";
  file << "cache_directory = \"" << config.data.cache_directory << "\"\n";
  file << "cache_max_age_days = " << config.data.cache_max_age_days << "\n";
  file << "allow_fallback_data = " << (config.data.allow_fallback_data ? "true" : "false") << "\n";
  file << "auto_update_cache = " << (config.data.auto_update_cache ? "true" : "false") << "\n";
  file << "network_timeout = " << config.data.network_timeout.count() << "\n";
  file << "max_retries = " << config.data.max_retries << "\n";
  file << "verify_ssl = " << (config.data.verify_ssl ? "true" : "false") << "\n\n";

  file << "[logging]\n";
  file << "min_level = " << static_cast<int>(config.logging.min_level) << "\n";
  file << "output = " << static_cast<int>(config.logging.output) << "\n";
  file << "log_file = \"" << config.logging.log_file << "\"\n";
  file << "include_timestamp = " << (config.logging.include_timestamp ? "true" : "false") << "\n";
  file << "include_thread_id = " << (config.logging.include_thread_id ? "true" : "false") << "\n";
  file << "colored_output = " << (config.logging.colored_output ? "true" : "false") << "\n";
  file << "max_log_file_size_mb = " << config.logging.max_log_file_size_mb << "\n";
  file << "max_log_files = " << config.logging.max_log_files << "\n\n";

  file << "[web]\n";
  file << "port = " << config.web.port << "\n";
  file << "host = \"" << config.web.host << "\"\n";
  file << "web_root = \"" << config.web.web_root << "\"\n";
  file << "enable_cors = " << (config.web.enable_cors ? "true" : "false") << "\n";
  file << "enable_compression = " << (config.web.enable_compression ? "true" : "false") << "\n";
  file << "max_connections = " << config.web.max_connections << "\n";
  file << "request_timeout = " << config.web.request_timeout.count() << "\n\n";

  file << "[global]\n";
  file << "app_name = \"" << config.app_name << "\"\n";
  file << "version = \"" << config.version << "\"\n";
  file << "debug_mode = " << (config.debug_mode ? "true" : "false") << "\n";

  return Expected<void, std::string>::success();
}

Config::AppConfig Config::get_default() {
  return AppConfig{};  // Uses default member initializers
}

Expected<void, std::string> Config::validate(const AppConfig& config) {
  // Validate simulation parameters
  if (config.simulation.timestep <= 0) {
    return Expected<void, std::string>::error("Timestep must be positive");
  }

  if (config.simulation.max_iterations == 0) {
    return Expected<void, std::string>::error("Max iterations must be positive");
  }

  if (config.simulation.gravitational_constant <= 0) {
    return Expected<void, std::string>::error("Gravitational constant must be positive");
  }

  // Validate data parameters
  if (config.data.cache_max_age_days == 0) {
    return Expected<void, std::string>::error("Cache max age must be positive");
  }

  if (config.data.max_retries == 0) {
    return Expected<void, std::string>::error("Max retries must be positive");
  }

  // Validate web parameters
  if (config.web.port == 0) {
    return Expected<void, std::string>::error("Web port must be positive");
  }

  if (config.web.max_connections == 0) {
    return Expected<void, std::string>::error("Max connections must be positive");
  }

  return Expected<void, std::string>::success();
}

Config::AppConfig Config::merge(const AppConfig& base, const AppConfig& override) {
  AppConfig result = base;

  // Simple merge - in a real implementation, this would be more sophisticated
  // For now, we'll just copy non-default values from override

  // This is a simplified implementation - a full version would check each field
  // and only override if the override value is different from default

  return result;  // Placeholder - needs full implementation
}

Expected<Config::ConfigMap, std::string> Config::load_from_file(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return Expected<ConfigMap, std::string>::error("Cannot open file: " + filename);
  }

  ConfigMap map;
  std::string line;
  std::string current_section;

  // Simple INI-style parser (could be enhanced with proper YAML/JSON library)
  while (std::getline(file, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Section headers
    if (line[0] == '[' && line.back() == ']') {
      current_section = line.substr(1, line.length() - 2);
      continue;
    }

    // Key-value pairs
    size_t eq_pos = line.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = line.substr(0, eq_pos);
      std::string value = line.substr(eq_pos + 1);

      // Trim whitespace
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      // Remove quotes from string values
      if (value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
      }

      std::string full_key = current_section.empty() ? key : current_section + "." + key;

      // Try to parse as different types
      if (value == "true") {
        map[full_key] = true;
      } else if (value == "false") {
        map[full_key] = false;
      } else if (value.find('.') != std::string::npos) {
        try {
          map[full_key] = std::stod(value);
        } catch (...) {
          map[full_key] = value;
        }
      } else {
        try {
          map[full_key] = static_cast<int64_t>(std::stoll(value));
        } catch (...) {
          map[full_key] = value;
        }
      }
    }
  }

  return Expected<ConfigMap, std::string>::success(map);
}

Config::ConfigMap Config::load_from_environment() {
  ConfigMap map;

  // Load environment variables with SOLAR_SYSTEM_ prefix
  const char* env_vars[] = {"SOLAR_SYSTEM_TIMESTEP", "SOLAR_SYSTEM_MAX_ITERATIONS",
                            "SOLAR_SYSTEM_VERBOSE",  "SOLAR_SYSTEM_LOG_LEVEL",
                            "SOLAR_SYSTEM_WEB_PORT", nullptr};

  for (const char** var = env_vars; *var != nullptr; ++var) {
    const char* value = std::getenv(*var);
    if (value != nullptr) {
      std::string key = *var;
      // Convert SOLAR_SYSTEM_TIMESTEP to simulation.timestep
      // This is a simplified mapping - full implementation would be more comprehensive
      map[key] = std::string(value);
    }
  }

  return map;
}

Config::ConfigMap Config::load_from_cli(const std::vector<std::string>& args) {
  ConfigMap map;

  // Parse command line arguments in --key=value format
  for (const auto& arg : args) {
    if (arg.starts_with("--")) {
      size_t eq_pos = arg.find('=');
      if (eq_pos != std::string::npos) {
        std::string key = arg.substr(2, eq_pos - 2);
        std::string value = arg.substr(eq_pos + 1);

        // Convert CLI keys to config keys
        if (key == "timestep") {
          map["simulation.timestep"] = std::stod(value);
        } else if (key == "verbose") {
          map["simulation.verbose_output"] = (value == "true" || value == "1");
        } else if (key == "port") {
          map["web.port"] = static_cast<int64_t>(std::stoll(value));
        }
        // Add more mappings as needed
      }
    }
  }

  return map;
}

Expected<Config::AppConfig, std::string> Config::map_to_config(const ConfigMap& map) {
  AppConfig config = get_default();

  // Convert map values to config structure
  // This is a simplified implementation - full version would handle all fields

  if (auto val = get_value<double>(map, "simulation.timestep")) {
    config.simulation.timestep = *val;
  }

  if (auto val = get_value<int64_t>(map, "simulation.max_iterations")) {
    config.simulation.max_iterations = static_cast<size_t>(*val);
  }

  if (auto val = get_value<bool>(map, "simulation.verbose_output")) {
    config.simulation.verbose_output = *val;
  }

  if (auto val = get_value<int64_t>(map, "web.port")) {
    config.web.port = static_cast<uint16_t>(*val);
  }

  // Add more field mappings as needed

  return Expected<AppConfig, std::string>::success(config);
}

template <typename T>
std::optional<T> Config::get_value(const ConfigMap& map, const std::string& key) {
  auto it = map.find(key);
  if (it != map.end()) {
    if (std::holds_alternative<T>(it->second)) {
      return std::get<T>(it->second);
    }
  }
  return std::nullopt;
}

// Global configuration implementation
std::optional<Config::AppConfig> GlobalConfig::config_;
bool GlobalConfig::initialized_ = false;

Config::AppConfig& GlobalConfig::instance() {
  if (!initialized_) {
    config_ = Config::get_default();
    initialized_ = true;
  }
  return *config_;
}

void GlobalConfig::initialize(const Config::AppConfig& config) {
  config_ = config;
  initialized_ = true;
}

bool GlobalConfig::is_initialized() { return initialized_; }

}  // namespace SolarSystem::Utils
