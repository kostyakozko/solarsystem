/**
 * @file config_manager.cpp
 * @brief Implementation of unified configuration management system
 */

#include "solar_core/config/config_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <thread>

namespace SolarSystem::Core::Config {

// Initialize static member
std::unique_ptr<ConfigurationManager> GlobalConfig::instance_ = nullptr;

ConfigurationManager::ConfigurationManager() {
  // Initialize with default configuration
  default_config_ = Utils::Config::get_default();
  merged_config_ = default_config_;

  // Initialize default dependencies
  initialize_default_dependencies();
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
    error.suggestions = {"Ensure the file is valid JSON format",
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
    result.suggestions.push_back(
        "Set max_file_size to a reasonable value (e.g., 10485760 for 10MB)");
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
    // Use merged_config_ for both cases - include_defaults parameter reserved for future use
    (void)include_defaults;  // Suppress unused parameter warning
    Utils::Config::save_to_file(merged_config_, config_path);
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
    return ConfigErrorDetail(ConfigError::FileNotFound,
                             "Cannot enable hot-reload for non-existent file",
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

  // Helper lambda to merge a config into result
  auto merge_into = [](Utils::Config::AppConfig& target, const Utils::Config::AppConfig& source) {
    // Merge simulation config
    if (source.simulation.timestep != target.simulation.timestep) {
      target.simulation.timestep = source.simulation.timestep;
    }
    if (source.simulation.max_iterations != target.simulation.max_iterations) {
      target.simulation.max_iterations = source.simulation.max_iterations;
    }
    if (source.simulation.tolerance != target.simulation.tolerance) {
      target.simulation.tolerance = source.simulation.tolerance;
    }
    target.simulation.enable_adaptive_timestep = source.simulation.enable_adaptive_timestep;

    // Merge logging config
    target.logging.console_output = source.logging.console_output;
    target.logging.colored_output = source.logging.colored_output;
    target.logging.enable_file_logging = source.logging.enable_file_logging;
    target.logging.min_level = source.logging.min_level;
    if (!source.logging.log_file.empty()) {
      target.logging.log_file = source.logging.log_file;
    }
    if (source.logging.max_file_size != 0) {
      target.logging.max_file_size = source.logging.max_file_size;
    }
    if (source.logging.max_backup_files > 0) {
      target.logging.max_backup_files = source.logging.max_backup_files;
    }

    // Merge output config
    if (!source.output.format.empty()) {
      target.output.format = source.output.format;
    }
    if (!source.output.output_directory.empty()) {
      target.output.output_directory = source.output.output_directory;
    }
    target.output.compress_output = source.output.compress_output;

    // Merge performance config
    if (source.performance.thread_count >= 0) {
      target.performance.thread_count = source.performance.thread_count;
    }
    target.performance.enable_gpu_acceleration = source.performance.enable_gpu_acceleration;

    // Merge network config
    if (!source.network.jpl_base_url.empty()) {
      target.network.jpl_base_url = source.network.jpl_base_url;
    }
    target.network.enable_caching = source.network.enable_caching;
    if (source.network.max_retries >= 0) {
      target.network.max_retries = source.network.max_retries;
    }
    if (source.network.timeout.count() > 0) {
      target.network.timeout = source.network.timeout;
    }

    // Merge debug mode
    target.debug_mode = source.debug_mode;
  };

  // Apply file configuration
  if (file_config_) {
    merge_into(result, *file_config_);
    // Track source for each parameter
    parameter_sources_["simulation.timestep"] = ConfigSource::ConfigFile;
    parameter_sources_["logging.log_file"] = ConfigSource::ConfigFile;
    parameter_sources_["output.format"] = ConfigSource::ConfigFile;
  }

  // Apply environment configuration (overrides file)
  if (env_config_) {
    merge_into(result, *env_config_);
    // Update sources for env-provided parameters
    parameter_sources_["debug_mode"] = ConfigSource::Environment;
  }

  // Apply CLI configuration (highest priority, overrides everything)
  if (cli_config_) {
    merge_into(result, *cli_config_);
    // Update sources for CLI-provided parameters
    parameter_sources_["simulation.timestep"] = ConfigSource::CommandLine;
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
  try {
    // Debug mode
    if (param_name == "debug_mode") {
      config.debug_mode = (value == "true" || value == "1" || value == "yes");
    }
    // Simulation parameters
    else if (param_name == "simulation.timestep") {
      config.simulation.timestep = std::stod(value);
    } else if (param_name == "simulation.max_iterations") {
      config.simulation.max_iterations = static_cast<int>(std::stoull(value));
    } else if (param_name == "simulation.tolerance") {
      config.simulation.tolerance = std::stod(value);
    } else if (param_name == "simulation.enable_adaptive_timestep") {
      config.simulation.enable_adaptive_timestep =
          (value == "true" || value == "1" || value == "yes");
    }
    // Logging parameters
    else if (param_name == "logging.console_output") {
      config.logging.console_output = (value == "true" || value == "1" || value == "yes");
    } else if (param_name == "logging.colored_output") {
      config.logging.colored_output = (value == "true" || value == "1" || value == "yes");
    } else if (param_name == "logging.enable_file_logging") {
      config.logging.enable_file_logging = (value == "true" || value == "1" || value == "yes");
    } else if (param_name == "logging.log_file") {
      config.logging.log_file = value;
    } else if (param_name == "logging.max_file_size") {
      config.logging.max_file_size = static_cast<size_t>(std::stoull(value));
    } else if (param_name == "logging.max_backup_files") {
      config.logging.max_backup_files = std::stoi(value);
    } else if (param_name == "logging.min_level") {
      // Parse log level string to enum
      std::string level_lower = value;
      std::transform(level_lower.begin(), level_lower.end(), level_lower.begin(), ::tolower);

      if (level_lower == "debug")
        config.logging.min_level = SolarSystem::Utils::ConfigLogLevel::DEBUG;
      else if (level_lower == "info")
        config.logging.min_level = SolarSystem::Utils::ConfigLogLevel::INFO;
      else if (level_lower == "warn" || level_lower == "warning")
        config.logging.min_level = SolarSystem::Utils::ConfigLogLevel::WARNING;
      else if (level_lower == "error")
        config.logging.min_level = SolarSystem::Utils::ConfigLogLevel::ERROR;
      else {
        return ConfigErrorDetail(ConfigError::ValidationFailed, "Invalid log level: " + value,
                                 "Valid values: DEBUG, INFO, WARN, ERROR");
      }
    }
    // Output parameters
    else if (param_name == "output.format") {
      config.output.format = value;
    } else if (param_name == "output.output_directory") {
      config.output.output_directory = value;
    } else if (param_name == "output.compress_output") {
      config.output.compress_output = (value == "true" || value == "1" || value == "yes");
    }
    // Performance parameters
    else if (param_name == "performance.thread_count") {
      config.performance.thread_count = std::stoi(value);
    } else if (param_name == "performance.enable_gpu_acceleration") {
      config.performance.enable_gpu_acceleration =
          (value == "true" || value == "1" || value == "yes");
    }
    // Network parameters
    else if (param_name == "network.jpl_base_url") {
      config.network.jpl_base_url = value;
    } else if (param_name == "network.enable_caching") {
      config.network.enable_caching = (value == "true" || value == "1" || value == "yes");
    } else if (param_name == "network.max_retries") {
      config.network.max_retries = std::stoi(value);
    } else if (param_name == "network.timeout") {
      config.network.timeout = std::chrono::seconds(std::stoi(value));
    } else {
      return ConfigErrorDetail(ConfigError::ValidationFailed,
                               "Unknown configuration parameter: " + param_name,
                               "Check parameter name spelling and documentation");
    }

    return ConfigResult<void>();
  } catch (const std::invalid_argument& e) {
    return ConfigErrorDetail(
        ConfigError::ValidationFailed, "Invalid value for parameter: " + param_name,
        std::string("Value '") + value + "' cannot be parsed. Error: " + e.what());
  } catch (const std::out_of_range& e) {
    return ConfigErrorDetail(ConfigError::ValidationFailed,
                             "Value out of range for parameter: " + param_name,
                             std::string("Value '") + value + "' is too large. Error: " + e.what());
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

ConfigResult<void> GlobalConfig::initialize(
    const std::filesystem::path& config_path,
    const std::map<std::string, std::string>& cli_overrides) {
  instance_ = std::make_unique<ConfigurationManager>();

  auto file_result = instance_->load_from_file(config_path);
  if (!file_result.has_value()) {
    return file_result;
  }

  return instance_->apply_cli_overrides(cli_overrides);
}

// Cross-application conflict detection
std::vector<CrossAppConflict> ConfigurationManager::detect_cross_app_conflicts(
    const std::map<std::string, Utils::Config::AppConfig>& app_configs) const {
  std::vector<CrossAppConflict> conflicts;

  // Check for conflicting port assig
  std::map<int, std::string> port_assignments;
  for (const auto& [app_name, config] : app_configs) {
    int port = config.network.default_port;
    if (port > 0) {
      auto it = port_assignments.find(port);
      if (it != port_assignments.end()) {
        CrossAppConflict conflict;
        conflict.application1 = it->second;
        conflict.application2 = app_name;
        conflict.parameter = "network.port";
        conflict.conflict_description =
            "Port " + std::to_string(port) + " is used by both " + it->second + " and " + app_name;
        conflict.resolution_suggestions.push_back("Use different ports for each application");
        conflicts.push_back(conflict);
      } else {
        port_assignments[port] = app_name;
      }
    }
  }

  // Check for conflicting output directories
  std::map<std::string, std::string> output_dirs;
  for (const auto& [app_name, config] : app_configs) {
    std::string output_dir = config.output.output_directory.string();
    auto it = output_dirs.find(output_dir);
    if (it != output_dirs.end()) {
      CrossAppConflict conflict;
      conflict.application1 = it->second;
      conflict.application2 = app_name;
      conflict.parameter = "output.output_directory";
      conflict.conflict_description =
          "Both applications writing to the same output directory may cause file conflicts";
      conflict.resolution_suggestions.push_back(
          "Use different output directories for each application");
      conflict.resolution_suggestions.push_back("Use application-specific subdirectories");
      conflicts.push_back(conflict);
    } else {
      output_dirs[output_dir] = app_name;
    }
  }

  // Check for conflicting log files
  std::map<std::string, std::string> log_files;
  for (const auto& [app_name, config] : app_configs) {
    if (!config.logging.log_file.empty()) {
      auto it = log_files.find(config.logging.log_file);
      if (it != log_files.end()) {
        CrossAppConflict conflict;
        conflict.application1 = it->second;
        conflict.application2 = app_name;
        conflict.parameter = "logging.log_file";
        conflict.conflict_description =
            "Both applications writing to the same log file may cause log corruption";
        conflict.resolution_suggestions.push_back("Use different log files for each application");
        conflict.resolution_suggestions.push_back("Use application-specific log file names");
        conflicts.push_back(conflict);
      } else {
        log_files[config.logging.log_file] = app_name;
      }
    }
  }

  // Check for resource conflicts (thread count)
  int total_threads = 0;
  for (const auto& [app_name, config] : app_configs) {
    if (config.performance.thread_count > 0) {
      total_threads += config.performance.thread_count;
    }
  }

  // Warn if total threads exceed reasonable limits
  unsigned int hardware_threads_unsigned = std::thread::hardware_concurrency();
  int hardware_threads = static_cast<int>(hardware_threads_unsigned);
  if (total_threads > hardware_threads * 2) {
    CrossAppConflict conflict;
    conflict.application1 = "system";
    conflict.application2 = "all_applications";
    conflict.parameter = "performance.thread_count";
    conflict.conflict_description = "Total thread count (" + std::to_string(total_threads) +
                                    ") significantly exceeds hardware threads (" +
                                    std::to_string(hardware_threads) + ")";
    conflict.resolution_suggestions.push_back("Reduce thread count in individual applications");
    conflict.resolution_suggestions.push_back(
        "Use thread_count=0 for auto-detection in some applications");
    conflicts.push_back(conflict);
  }

  return conflicts;
}

// Dependency management
void ConfigurationManager::register_dependency(const ConfigDependency& dependency) {
  dependencies_.push_back(dependency);

  // Update dependency map
  for (const auto& dep : dependency.depends_on) {
    parameter_dependencies_[dependency.parameter].push_back(dep);
  }
}

ValidationResult ConfigurationManager::validate_dependencies() const {
  ValidationResult result;
  result.is_valid = true;

  for (const auto& dependency : dependencies_) {
    // Run custom validation function if provided
    if (dependency.validation_func) {
      if (!dependency.validation_func(merged_config_)) {
        result.is_valid = false;
        result.errors.push_back("Dependency validation failed for " + dependency.parameter + ": " +
                                dependency.description);
      }
    }
  }

  // Check for circular dependencies
  for (const auto& [param, deps] : parameter_dependencies_) {
    if (has_circular_dependency(param)) {
      result.is_valid = false;
      result.errors.push_back("Circular dependency detected for parameter: " + param);
      result.suggestions.push_back("Review parameter dependencies and remove circular references");
    }
  }

  return result;
}

ImpactAnalysis ConfigurationManager::analyze_impact(const std::string& parameter_name,
                                                    const std::string& new_value) const {
  ImpactAnalysis analysis;
  analysis.changed_parameter = parameter_name;

  // Find parameters that depend on this one
  analysis.affected_parameters = get_dependent_parameters(parameter_name);

  // Analyze impact on applications
  if (parameter_name.find("simulation.") == 0) {
    analysis.affected_applications.push_back("solar_system");
    analysis.affected_applications.push_back("solar_system_launcher");
  } else if (parameter_name.find("logging.") == 0) {
    analysis.affected_applications.push_back("all_applications");
  } else if (parameter_name.find("output.") == 0) {
    analysis.affected_applications.push_back("solar_system");
    analysis.affected_applications.push_back("solar_system_fetch");
  } else if (parameter_name.find("network.") == 0) {
    analysis.affected_applications.push_back("solar_system_fetch");
    analysis.affected_applications.push_back("solar_system_web");
  }

  // Generate warnings based on the change
  if (parameter_name == "simulation.timestep") {
    try {
      double timestep = std::stod(new_value);
      if (timestep < 1.0) {
        analysis.warnings.push_back(
            "Very small timestep may significantly increase simulation time");
      } else if (timestep > 86400.0) {
        analysis.warnings.push_back("Large timestep may reduce simulation accuracy");
      }
    } catch (...) {
      analysis.warnings.push_back("Invalid timestep value");
    }
  }

  if (parameter_name == "performance.thread_count") {
    try {
      int threads = std::stoi(new_value);
      unsigned int hardware_threads_unsigned = std::thread::hardware_concurrency();
      int hardware_threads = static_cast<int>(hardware_threads_unsigned);
      if (threads > hardware_threads) {
        analysis.warnings.push_back("Thread count exceeds hardware threads, may cause contention");
        analysis.recommendations.push_back("Consider using thread_count=0 for automatic detection");
      }
    } catch (...) {
      analysis.warnings.push_back("Invalid thread count value");
    }
  }

  if (parameter_name == "logging.min_level") {
    analysis.recommendations.push_back(
        "Changing log level affects all logging output across applications");
    analysis.recommendations.push_back(
        "Consider restarting applications for change to take effect");
  }

  return analysis;
}

std::vector<ConfigDependency> ConfigurationManager::get_dependencies() const {
  return dependencies_;
}

bool ConfigurationManager::would_cause_conflict(const std::string& parameter_name,
                                                const std::string& new_value) const {
  // Create a temporary config with the proposed change
  Utils::Config::AppConfig temp_config = merged_config_;

  // Apply the change (simplified)
  auto result = const_cast<ConfigurationManager*>(this)->apply_config_value(
      temp_config, parameter_name, new_value);

  if (!result.has_value()) {
    return true;  // Invalid value would cause a conflict
  }

  // Check dependencies
  for (const auto& dependency : dependencies_) {
    if (dependency.validation_func && !dependency.validation_func(temp_config)) {
      return true;  // Dependency validation failed
    }
  }

  return false;
}

void ConfigurationManager::initialize_default_dependencies() {
  // Dependency: adaptive timestep requires tolerance to be set
  ConfigDependency adaptive_dep;
  adaptive_dep.parameter = "simulation.enable_adaptive_timestep";
  adaptive_dep.depends_on = {"simulation.tolerance"};
  adaptive_dep.description = "Adaptive timestep requires tolerance parameter";
  adaptive_dep.validation_func = [](const Utils::Config::AppConfig& config) {
    if (config.simulation.enable_adaptive_timestep) {
      return config.simulation.tolerance > 0.0;
    }
    return true;
  };
  register_dependency(adaptive_dep);

  // Dependency: file logging requires log file path
  ConfigDependency file_logging_dep;
  file_logging_dep.parameter = "logging.enable_file_logging";
  file_logging_dep.depends_on = {"logging.log_file"};
  file_logging_dep.description = "File logging requires log file path";
  file_logging_dep.validation_func = [](const Utils::Config::AppConfig& config) {
    if (config.logging.enable_file_logging) {
      return !config.logging.log_file.empty();
    }
    return true;
  };
  register_dependency(file_logging_dep);

  // Dependency: compression requires output format that supports it
  ConfigDependency compression_dep;
  compression_dep.parameter = "output.compress_output";
  compression_dep.depends_on = {"output.format"};
  compression_dep.description = "Output compression depends on format";
  compression_dep.validation_func = [](const Utils::Config::AppConfig& config) {
    if (config.output.compress_output) {
      // Binary format supports compression best
      return config.output.format == "binary" || config.output.format == "json";
    }
    return true;
  };
  register_dependency(compression_dep);

  // Dependency: GPU acceleration requires appropriate thread count
  ConfigDependency gpu_dep;
  gpu_dep.parameter = "performance.enable_gpu_acceleration";
  gpu_dep.depends_on = {"performance.thread_count"};
  gpu_dep.description = "GPU acceleration may conflict with high thread counts";
  gpu_dep.validation_func = [](const Utils::Config::AppConfig& config) {
    if (config.performance.enable_gpu_acceleration && config.performance.thread_count > 0) {
      // Warn if using both GPU and many CPU threads
      return config.performance.thread_count <= 4;
    }
    return true;
  };
  register_dependency(gpu_dep);
}

bool ConfigurationManager::has_circular_dependency(const std::string& param) const {
  std::set<std::string> visited;
  std::set<std::string> recursion_stack;

  std::function<bool(const std::string&)> check_circular = [&](const std::string& current) -> bool {
    if (recursion_stack.find(current) != recursion_stack.end()) {
      return true;  // Circular dependency found
    }

    if (visited.find(current) != visited.end()) {
      return false;  // Already checked this path
    }

    visited.insert(current);
    recursion_stack.insert(current);

    auto it = parameter_dependencies_.find(current);
    if (it != parameter_dependencies_.end()) {
      for (const auto& dep : it->second) {
        if (check_circular(dep)) {
          return true;
        }
      }
    }

    recursion_stack.erase(current);
    return false;
  };

  return check_circular(param);
}

std::vector<std::string> ConfigurationManager::get_dependent_parameters(
    const std::string& param) const {
  std::vector<std::string> dependents;

  for (const auto& [dependent_param, dependencies] : parameter_dependencies_) {
    if (std::find(dependencies.begin(), dependencies.end(), param) != dependencies.end()) {
      dependents.push_back(dependent_param);
    }
  }

  return dependents;
}

}  // namespace SolarSystem::Core::Config
