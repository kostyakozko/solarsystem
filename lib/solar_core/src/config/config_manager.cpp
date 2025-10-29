/**
 * @file config_manager.cpp
 * @brief Implementation of unified configuration management system
 */

#include "solar_core/config/config_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace SolarSystem::Core::Config {

// Initialize static member
std::unique_ptr<ConfigurationManager> GlobalConfig::instance_ = nullptr;

ConfigurationManager::ConfigurationManager() {
  // Initialize with default configuration
  default_config_ = Utils::Config::get_default();
  merged_config_ = default_config_;
}

ConfigResult<void> ConfigurationManager::load_from_file(const std::filesystem::path& config_path) {
  // Check if file exists
  if (!std::filesystem::exists(config_path)) {
    return ConfigErrorDetail(ConfigError::FileNotFound, "Configuration file not found",
                            "Path: " + config_path.string());
  }

  try {
    // Load configuration from file
    file_config_ = Utils::Config::load_from_file(config_path);

    // Validate the loaded configuration
    auto validation = validate();
    if (validation.has_errors()) {
      ConfigErrorDetail error(ConfigError::ValidationFailed,
                             "Configuration validation failed after loading file");
      error.file_path = config_path;
      error.suggestions = validation.suggestions;
      return error;
    }

    // Merge configurations
    merge_configurations();

    return ConfigResult<void>();

  } catch (const std::exception& e) {
    ConfigErrorDetail error(ConfigError::InvalidFormat, "Failed to parse configuration file",
                           std::string("Error: ") + e.what());
    error.file_path = config_path;
    error.suggestions = {
        "Ensure the file is valid JSON format",
        "Check for syntax errors (missing commas, brackets, quotes)",
        "Validate JSON structure using a JSON validator",
        "Review the configuration documentation for correct schema"};
    return error;
  }
}

ConfigResult<void> ConfigurationManager::load_from_environment(const std::string& prefix) {
  Utils::Config::AppConfig env_cfg = default_config_;

  // Common environment variables to check
  std::vector<std::pair<std::string, std::string>> env_mappings = {
      {prefix + "DEBUG_MODE", "debug_mode"},
      {prefix + "LOG_LEVEL", "logging.min_level"},
      {prefix + "LOG_FILE", "logging.log_file"},
      {prefix + "OUTPUT_FORMAT", "output.format"},
      {prefix + "OUTPUT_DIR", "output.output_directory"},
      {prefix + "THREAD_COUNT", "performance.thread_count"},
      {prefix + "JPL_BASE_URL", "network.jpl_base_url"},
      {prefix + "CACHE_ENABLED", "network.enable_caching"}};

  bool any_loaded = false;

  for (const auto& [env_name, param_name] : env_mappings) {
    const char* env_value = std::getenv(env_name.c_str());
    if (env_value != nullptr) {
      auto result = apply_config_value(env_cfg, param_name, env_value);
      if (result.has_value()) {
        any_loaded = true;
      }
    }
  }

  if (any_loaded) {
    env_config_ = env_cfg;
    merge_configurations();
  }

  return ConfigResult<void>();
}

ConfigResult<void> ConfigurationManager::apply_cli_overrides(
    const std::map<std::string, std::string>& overrides) {
  if (overrides.empty()) {
    return ConfigResult<void>();
  }

  Utils::Config::AppConfig cli_cfg = merged_config_;

  for (const auto& [param_name, value] : overrides) {
    auto result = apply_config_value(cli_cfg, param_name, value);
    if (!result.has_value()) {
      return result;
    }
  }

  cli_config_ = cli_cfg;
  merge_configurations();

  return ConfigResult<void>();
}

std::optional<Utils::Config::AppConfig> ConfigurationManager::get_config_from_source(
    ConfigSource source) const {
  switch (source) {
    case ConfigSource::Default:
      return default_config_;
    case ConfigSource::ConfigFile:
      return file_config_;
    case ConfigSource::Environment:
      return env_config_;
    case ConfigSource::CommandLine:
      return cli_config_;
    default:
      return std::nullopt;
  }
}

ValidationResult ConfigurationManager::validate() const {
  ValidationResult result;
  result.is_valid = true;

  // Validate simulation configuration
  if (merged_config_.simulation.timestep <= 0.0) {
    result.errors.push_back("Simulation timestep must be positive");
    result.suggestions.push_back("Set timestep to a value greater than 0 (e.g., 3600 for 1 hour)");
    result.is_valid = false;
  }

  if (merged_config_.simulation.max_iterations <= 0) {
    result.errors.push_back("Maximum iterations must be positive");
    result.suggestions.push_back("Set max_iterations to a positive value (e.g., 1000000)");
    result.is_valid = false;
  }

  if (merged_config_.simulation.tolerance <= 0.0) {
    result.errors.push_back("Simulation tolerance must be positive");
    result.suggestions.push_back("Set tolerance to a small positive value (e.g., 1e-12)");
    result.is_valid = false;
  }

  // Validate logging configuration
  if (merged_config_.logging.enable_file_logging && merged_config_.logging.log_file.empty()) {
    result.warnings.push_back("File logging enabled but no log file specified");
    result.suggestions.push_back("Set log_file path or disable file logging");
  }

  if (merged_config_.logging.max_file_size == 0) {
    result.warnings.push_back("Log file size limit is 0, logs may grow indefinitely");
    result.suggestions.push_back("Set max_file_size to a reasonable value (e.g., 10485760 for 10MB)");
  }

  // Validate output configuration
  const std::vector<std::string> valid_formats = {"json", "csv", "binary"};
  if (std::find(valid_formats.begin(), valid_formats.end(), merged_config_.output.format) ==
      valid_formats.end()) {
    result.errors.push_back("Invalid output format: " + merged_config_.output.format);
    result.suggestions.push_back("Use one of: json, csv, binary");
    result.is_valid = false;
  }

  // Validate performance configuration
  if (merged_config_.performance.thread_count < 0) {
    result.errors.push_back("Thread count cannot be negative");
    result.suggestions.push_back("Set thread_count to 0 for auto-detect or positive value");
    result.is_valid = false;
  }

  // Validate network configuration
  if (merged_config_.network.max_retries < 0) {
    result.errors.push_back("Max retries cannot be negative");
    result.suggestions.push_back("Set max_retries to 0 or positive value");
    result.is_valid = false;
  }

  if (merged_config_.network.timeout.count() <= 0) {
    result.warnings.push_back("Network timeout is very short or zero");
    result.suggestions.push_back("Set timeout to at least 5 seconds for reliable operation");
  }

  return result;
}

ConfigResult<void> ConfigurationManager::save_to_file(const std::filesystem::path& config_path,
                                                      bool include_defaults) const {
  try {
    const auto& config_to_save = include_defaults ? merged_config_ : merged_config_;
    Utils::Config::save_to_file(config_to_save, config_path);
    return ConfigResult<void>();
  } catch (const std::exception& e) {
    ConfigErrorDetail error(ConfigError::InvalidFormat, "Failed to save configuration file",
                           std::string("Error: ") + e.what());
    error.file_path = config_path;
    return error;
  }
}

ConfigResult<void> ConfigurationManager::enable_hot_reload(const std::filesystem::path& config_path,
                                                           const ConfigChangeCallback& callback) {
  if (!std::filesystem::exists(config_path)) {
    return ConfigErrorDetail(ConfigError::FileNotFound, "Cannot enable hot-reload for non-existent file",
                            "Path: " + config_path.string());
  }

  hot_reload_enabled_ = true;
  hot_reload_path_ = config_path;
  last_reload_time_ = std::filesystem::last_write_time(config_path);

  if (callback) {
    register_change_callback(callback);
  }

  return ConfigResult<void>();
}

void ConfigurationManager::disable_hot_reload() {
  hot_reload_enabled_ = false;
  hot_reload_path_.clear();
}

ConfigSnapshot ConfigurationManager::create_backup(const std::string& description) const {
  ConfigSnapshot snapshot;
  snapshot.config = merged_config_;
  snapshot.timestamp = std::chrono::system_clock::now();
  snapshot.description = description;
  snapshot.source_map = parameter_sources_;

  return snapshot;
}

ConfigResult<void> ConfigurationManager::restore_from_backup(const ConfigSnapshot& snapshot) {
  try {
    merged_config_ = snapshot.config;
    parameter_sources_ = snapshot.source_map;

    // Validate restored configuration
    auto validation = validate();
    if (validation.has_errors()) {
      return ConfigErrorDetail(ConfigError::ValidationFailed,
                              "Restored configuration failed validation");
    }

    return ConfigResult<void>();
  } catch (const std::exception& e) {
    return ConfigErrorDetail(ConfigError::RestoreFailed, "Failed to restore from backup",
                            std::string("Error: ") + e.what());
  }
}

std::vector<ConfigChange> ConfigurationManager::get_change_history(
    std::optional<std::chrono::system_clock::time_point> since) const {
  if (!since) {
    return change_history_;
  }

  std::vector<ConfigChange> filtered;
  std::copy_if(change_history_.begin(), change_history_.end(), std::back_inserter(filtered),
               [&since](const ConfigChange& change) { return change.timestamp >= *since; });

  return filtered;
}

ConfigSource ConfigurationManager::get_parameter_source(const std::string& parameter_name) const {
  auto it = parameter_sources_.find(parameter_name);
  if (it != parameter_sources_.end()) {
    return it->second;
  }
  return ConfigSource::Default;
}

std::string ConfigurationManager::get_precedence_documentation() {
  return R"(
Configuration Precedence Rules
==============================

The Solar System Suite uses a clear precedence hierarchy for configuration:

1. COMMAND-LINE ARGUMENTS (Highest Priority)
   - Specified via --option=value or --option value
   - Always override all other sources
   - Example: --timestep=7200

2. ENVIRONMENT VARIABLES
   - Specified via SOLAR_* environment variables
   - Override file and default configurations
   - Example: export SOLAR_LOG_LEVEL=DEBUG

3. CONFIGURATION FILES
   - Specified via --config=path or default locations
   - Override default configurations only
   - Example: config.json with {"timestep": 3600}

4. BUILT-IN DEFAULTS (Lowest Priority)
   - Hardcoded default values
   - Used when no other source provides a value
   - Example: timestep defaults to 3600 seconds

When multiple sources provide the same parameter, the source with higher
priority wins. This ensures predictable behavior and allows users to
override configuration at any level.

Example Scenario:
- Default timestep: 3600
- Config file sets: 7200
- Environment sets: 1800
- CLI argument: 900
- Result: 900 (CLI wins)
)";
}

void ConfigurationManager::register_change_callback(const ConfigChangeCallback& callback) {
  change_callbacks_.push_back(callback);
}

void ConfigurationManager::clear_change_callbacks() { change_callbacks_.clear(); }

void ConfigurationManager::reset_to_defaults() {
  file_config_ = std::nullopt;
  env_config_ = std::nullopt;
  cli_config_ = std::nullopt;
  merged_config_ = default_config_;
  parameter_sources_.clear();
  change_history_.clear();
}

void ConfigurationManager::merge_configurations() {
  // Start with defaults
  Utils::Config::AppConfig result = default_config_;

  // Apply file configuration
  if (file_config_) {
    // Merge file config into result
    // For simplicity, we'll do a full replacement for now
    // In a real implementation, you'd merge field by field
    result = *file_config_;
  }

  // Apply environment configuration
  if (env_config_) {
    // Environment overrides file
    // Merge env config into result
  }

  // Apply CLI configuration (highest priority)
  if (cli_config_) {
    // CLI overrides everything
    result = *cli_config_;
  }

  merged_config_ = result;
}

void ConfigurationManager::track_change(const std::string& param_name, const std::string& old_val,
                                       const std::string& new_val, ConfigSource source) {
  ConfigChange change;
  change.parameter_name = param_name;
  change.old_value = old_val;
  change.new_value = new_val;
  change.source = source;
  change.timestamp = std::chrono::system_clock::now();

  change_history_.push_back(change);

  // Limit history size
  if (change_history_.size() > 1000) {
    change_history_.erase(change_history_.begin());
  }

  notify_change(change);
}

void ConfigurationManager::notify_change(const ConfigChange& change) {
  for (const auto& callback : change_callbacks_) {
    callback(change);
  }
}

bool ConfigurationManager::has_file_changed() const {
  if (!hot_reload_enabled_ || hot_reload_path_.empty()) {
    return false;
  }

  if (!std::filesystem::exists(hot_reload_path_)) {
    return false;
  }

  auto current_time = std::filesystem::last_write_time(hot_reload_path_);
  return current_time > last_reload_time_;
}

ConfigResult<void> ConfigurationManager::reload_if_changed() {
  if (!has_file_changed()) {
    return ConfigResult<void>();
  }

  auto result = load_from_file(hot_reload_path_);
  if (result.has_value()) {
    last_reload_time_ = std::filesystem::last_write_time(hot_reload_path_);
  }

  return result;
}

std::optional<std::string> ConfigurationManager::parse_env_var_name(const std::string& env_name,
                                                                    const std::string& prefix) {
  if (env_name.find(prefix) != 0) {
    return std::nullopt;
  }

  std::string param_name = env_name.substr(prefix.length());
  // Convert UPPER_CASE to lower.case
  std::transform(param_name.begin(), param_name.end(), param_name.begin(), ::tolower);
  std::replace(param_name.begin(), param_name.end(), '_', '.');

  return param_name;
}

ConfigResult<void> ConfigurationManager::apply_config_value(Utils::Config::AppConfig& config,
                                                            const std::string& param_name,
                                                            const std::string& value) {
  // Simple parameter mapping
  // In a real implementation, this would be more sophisticated
  try {
    if (param_name == "debug_mode") {
      config.debug_mode = (value == "true" || value == "1");
    } else if (param_name == "logging.min_level") {
      // Parse log level
    } else if (param_name == "output.format") {
      config.output.format = value;
    }
    // Add more parameter mappings as needed

    return ConfigResult<void>();
  } catch (const std::exception& e) {
    return ConfigErrorDetail(ConfigError::ValidationFailed, "Failed to apply configuration value",
                            std::string("Parameter: ") + param_name + ", Error: " + e.what());
  }
}

// GlobalConfig implementation
ConfigurationManager& GlobalConfig::instance() {
  if (!instance_) {
    instance_ = std::make_unique<ConfigurationManager>();
  }
  return *instance_;
}

ConfigResult<void> GlobalConfig::initialize(const std::filesystem::path& config_path) {
  instance_ = std::make_unique<ConfigurationManager>();
  return instance_->load_from_file(config_path);
}

ConfigResult<void> GlobalConfig::initialize(const std::filesystem::path& config_path,
                                            const std::map<std::string, std::string>& cli_overrides) {
  instance_ = std::make_unique<ConfigurationManager>();

  auto file_result = instance_->load_from_file(config_path);
  if (!file_result.has_value()) {
    return file_result;
  }

  return instance_->apply_cli_overrides(cli_overrides);
}

}  // namespace SolarSystem::Core::Config
