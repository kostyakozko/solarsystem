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

  // Merge simulation config - only override non-default values
  AppConfig defaults = get_default();

  if (override.simulation.timestep != defaults.simulation.timestep) {
    result.simulation.timestep = override.simulation.timestep;
  }
  if (override.simulation.max_iterations != defaults.simulation.max_iterations) {
    result.simulation.max_iterations = override.simulation.max_iterations;
  }
  if (override.simulation.enable_progress != defaults.simulation.enable_progress) {
    result.simulation.enable_progress = override.simulation.enable_progress;
  }
  if (override.simulation.verbose_output != defaults.simulation.verbose_output) {
    result.simulation.verbose_output = override.simulation.verbose_output;
  }
  if (override.simulation.output_format != defaults.simulation.output_format) {
    result.simulation.output_format = override.simulation.output_format;
  }

  // Merge data config
  if (override.data.jpl_api_url != defaults.data.jpl_api_url) {
    result.data.jpl_api_url = override.data.jpl_api_url;
  }
  if (override.data.cache_directory != defaults.data.cache_directory) {
    result.data.cache_directory = override.data.cache_directory;
  }
  if (override.data.cache_max_age_days != defaults.data.cache_max_age_days) {
    result.data.cache_max_age_days = override.data.cache_max_age_days;
  }

  // Merge logging config
  if (override.logging.min_level != defaults.logging.min_level) {
    result.logging.min_level = override.logging.min_level;
  }
  if (override.logging.output != defaults.logging.output) {
    result.logging.output = override.logging.output;
  }
  if (override.logging.log_file != defaults.logging.log_file) {
    result.logging.log_file = override.logging.log_file;
  }
  if (override.logging.colored_output != defaults.logging.colored_output) {
    result.logging.colored_output = override.logging.colored_output;
  }

  // Merge web config
  if (override.web.port != defaults.web.port) {
    result.web.port = override.web.port;
  }
  if (override.web.host != defaults.web.host) {
    result.web.host = override.web.host;
  }
  if (override.web.web_root != defaults.web.web_root) {
    result.web.web_root = override.web.web_root;
  }

  // Merge global config
  if (override.debug_mode != defaults.debug_mode) {
    result.debug_mode = override.debug_mode;
  }

  return result;
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

  // Load environment variables with SOLAR_SYSTEM_ prefix - COMPLETE MAPPING
  const struct {
    const char* env_name;
    const char* config_key;
  } env_mappings[] = {{"SOLAR_SYSTEM_TIMESTEP", "simulation.timestep"},
                      {"SOLAR_SYSTEM_MAX_ITERATIONS", "simulation.max_iterations"},
                      {"SOLAR_SYSTEM_VERBOSE", "simulation.verbose_output"},
                      {"SOLAR_SYSTEM_OUTPUT_FORMAT", "simulation.output_format"},
                      {"SOLAR_SYSTEM_JPL_API_URL", "data.jpl_api_url"},
                      {"SOLAR_SYSTEM_CACHE_DIR", "data.cache_directory"},
                      {"SOLAR_SYSTEM_LOG_LEVEL", "logging.min_level"},
                      {"SOLAR_SYSTEM_LOG_FILE", "logging.log_file"},
                      {"SOLAR_SYSTEM_LOG_COLORED", "logging.colored_output"},
                      {"SOLAR_SYSTEM_WEB_PORT", "web.port"},
                      {"SOLAR_SYSTEM_WEB_HOST", "web.host"},
                      {"SOLAR_SYSTEM_DEBUG", "global.debug_mode"},
                      {nullptr, nullptr}};

  for (const auto* mapping = env_mappings; mapping->env_name != nullptr; ++mapping) {
    const char* value = std::getenv(mapping->env_name);
    if (value != nullptr) {
      std::string str_value(value);

      // Parse value based on expected type
      if (str_value == "true" || str_value == "1") {
        map[mapping->config_key] = true;
      } else if (str_value == "false" || str_value == "0") {
        map[mapping->config_key] = false;
      } else if (str_value.find('.') != std::string::npos) {
        try {
          map[mapping->config_key] = std::stod(str_value);
        } catch (...) {
          map[mapping->config_key] = str_value;
        }
      } else {
        try {
          map[mapping->config_key] = static_cast<int64_t>(std::stoll(str_value));
        } catch (...) {
          map[mapping->config_key] = str_value;
        }
      }
    }
  }

  return map;
}

Config::ConfigMap Config::load_from_cli(const std::vector<std::string>& args) {
  ConfigMap map;

  // Parse command line arguments in --key=value format - COMPLETE MAPPING
  for (const auto& arg : args) {
    if (arg.starts_with("--")) {
      size_t eq_pos = arg.find('=');
      if (eq_pos != std::string::npos) {
        std::string key = arg.substr(2, eq_pos - 2);
        std::string value = arg.substr(eq_pos + 1);

        // Convert CLI keys to config keys - COMPREHENSIVE MAPPING
        if (key == "timestep") {
          map["simulation.timestep"] = std::stod(value);
        } else if (key == "max-iterations") {
          map["simulation.max_iterations"] = static_cast<int64_t>(std::stoll(value));
        } else if (key == "verbose") {
          map["simulation.verbose_output"] = (value == "true" || value == "1");
        } else if (key == "output-format") {
          map["simulation.output_format"] = value;
        } else if (key == "jpl-api-url") {
          map["data.jpl_api_url"] = value;
        } else if (key == "cache-dir") {
          map["data.cache_directory"] = value;
        } else if (key == "cache-max-age") {
          map["data.cache_max_age_days"] = static_cast<int64_t>(std::stoll(value));
        } else if (key == "log-level") {
          map["logging.min_level"] = static_cast<int64_t>(std::stoll(value));
        } else if (key == "log-file") {
          map["logging.log_file"] = value;
        } else if (key == "log-colored") {
          map["logging.colored_output"] = (value == "true" || value == "1");
        } else if (key == "port") {
          map["web.port"] = static_cast<int64_t>(std::stoll(value));
        } else if (key == "host") {
          map["web.host"] = value;
        } else if (key == "web-root") {
          map["web.web_root"] = value;
        } else if (key == "cors") {
          map["web.enable_cors"] = (value == "true" || value == "1");
        } else if (key == "debug") {
          map["global.debug_mode"] = (value == "true" || value == "1");
        }
        // Add more mappings as needed
      }
    }
  }

  return map;
}

Expected<Config::AppConfig, std::string> Config::map_to_config(const ConfigMap& map) {
  AppConfig config = get_default();

  // Convert map values to config structure - COMPLETE IMPLEMENTATION

  // Simulation config
  if (auto val = get_value<double>(map, "simulation.timestep")) {
    config.simulation.timestep = *val;
  }
  if (auto val = get_value<int64_t>(map, "simulation.max_iterations")) {
    config.simulation.max_iterations = static_cast<size_t>(*val);
  }
  if (auto val = get_value<bool>(map, "simulation.enable_progress")) {
    config.simulation.enable_progress = *val;
  }
  if (auto val = get_value<bool>(map, "simulation.verbose_output")) {
    config.simulation.verbose_output = *val;
  }
  if (auto val = get_value<std::string>(map, "simulation.output_format")) {
    config.simulation.output_format = *val;
  }

  // Data config
  if (auto val = get_value<std::string>(map, "data.jpl_api_url")) {
    config.data.jpl_api_url = *val;
  }
  if (auto val = get_value<std::string>(map, "data.cache_directory")) {
    config.data.cache_directory = *val;
  }
  if (auto val = get_value<int64_t>(map, "data.cache_max_age_days")) {
    config.data.cache_max_age_days = static_cast<size_t>(*val);
  }
  if (auto val = get_value<bool>(map, "data.allow_fallback_data")) {
    config.data.allow_fallback_data = *val;
  }
  if (auto val = get_value<int64_t>(map, "data.network_timeout")) {
    config.data.network_timeout = std::chrono::seconds(*val);
  }
  if (auto val = get_value<int64_t>(map, "data.max_retries")) {
    config.data.max_retries = static_cast<size_t>(*val);
  }

  // Logging config
  if (auto val = get_value<int64_t>(map, "logging.min_level")) {
    config.logging.min_level = static_cast<Logger::Level>(*val);
  }
  if (auto val = get_value<int64_t>(map, "logging.output")) {
    config.logging.output = static_cast<Logger::Output>(*val);
  }
  if (auto val = get_value<std::string>(map, "logging.log_file")) {
    config.logging.log_file = *val;
  }
  if (auto val = get_value<bool>(map, "logging.include_timestamp")) {
    config.logging.include_timestamp = *val;
  }
  if (auto val = get_value<bool>(map, "logging.colored_output")) {
    config.logging.colored_output = *val;
  }

  // Web config
  if (auto val = get_value<int64_t>(map, "web.port")) {
    config.web.port = static_cast<uint16_t>(*val);
  }
  if (auto val = get_value<std::string>(map, "web.host")) {
    config.web.host = *val;
  }
  if (auto val = get_value<std::string>(map, "web.web_root")) {
    config.web.web_root = *val;
  }
  if (auto val = get_value<bool>(map, "web.enable_cors")) {
    config.web.enable_cors = *val;
  }

  // Global config
  if (auto val = get_value<std::string>(map, "global.app_name")) {
    config.app_name = *val;
  }
  if (auto val = get_value<std::string>(map, "global.version")) {
    config.version = *val;
  }
  if (auto val = get_value<bool>(map, "global.debug_mode")) {
    config.debug_mode = *val;
  }

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

// Explicit template instantiations for commonly used types
template std::optional<bool> Config::get_value<bool>(const ConfigMap&, const std::string&);
template std::optional<int64_t> Config::get_value<int64_t>(const ConfigMap&, const std::string&);
template std::optional<double> Config::get_value<double>(const ConfigMap&, const std::string&);
template std::optional<std::string> Config::get_value<std::string>(const ConfigMap&,
                                                                   const std::string&);

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
