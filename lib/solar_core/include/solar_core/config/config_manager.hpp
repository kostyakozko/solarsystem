/**
 * @file config_manager.hpp
 * @brief Unified configuration management system for Solar System Suite
 *
 * This module provides centralized configuration management with:
 * - Multiple configuration sources (CLI, files, environment, defaults)
 * - Clear precedence rules (CLI > Environment > File > Defaults)
 * - Hot-reloading support for safe configuration updates
 * - Comprehensive validation and error reporting
 * - Configuration backup and versioning
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_utils/expected.hpp"
#include "solar_utils/config.hpp"

namespace SolarSystem::Core::Config {

/**
 * @brief Configuration source types with clear precedence
 */
enum class ConfigSource {
  Default = 0,      // Lowest priority: Built-in defaults
  ConfigFile = 1,   // File-based configuration
  Environment = 2,  // Environment variables
  CommandLine = 3   // Highest priority: Command-line arguments
};

/**
 * @brief Configuration error types
 */
enum class ConfigError {
  FileNotFound,
  InvalidFormat,
  ValidationFailed,
  ConflictDetected,
  HotReloadFailed,
  BackupFailed,
  RestoreFailed,
  SourceNotAvailable
};

/**
 * @brief Detailed configuration error information
 */
struct ConfigErrorDetail {
  ConfigError error_code;
  std::string message;
  std::string context;
  std::vector<std::string> suggestions;
  std::optional<std::filesystem::path> file_path;

  ConfigErrorDetail(ConfigError code, const std::string& msg)
      : error_code(code), message(msg) {}

  ConfigErrorDetail(ConfigError code, const std::string& msg, const std::string& ctx)
      : error_code(code), message(msg), context(ctx) {}
};

/**
 * @brief Configuration validation result
 */
struct ValidationResult {
  bool is_valid = false;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::vector<std::string> suggestions;

  [[nodiscard]] bool has_errors() const { return !errors.empty(); }
  [[nodiscard]] bool has_warnings() const { return !warnings.empty(); }
};

/**
 * @brief Configuration change notification
 */
struct ConfigChange {
  std::string parameter_name;
  std::string old_value;
  std::string new_value;
  ConfigSource source;
  std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Configuration snapshot for backup/restore
 */
struct ConfigSnapshot {
  Utils::Config::AppConfig config;
  std::chrono::system_clock::time_point timestamp;
  std::string description;
  std::map<std::string, ConfigSource> source_map;
};

/**
 * @brief Configuration change callback type
 */
using ConfigChangeCallback = std::function<void(const ConfigChange&)>;

/**
 * @brief Result type for configuration operations
 */
template <typename T>
using ConfigResult = SolarSystem::Utils::Expected<T, ConfigErrorDetail>;

/**
 * @brief Configuration dependency information
 */
struct ConfigDependency {
  std::string parameter;
  std::vector<std::string> depends_on;
  std::string description;
  std::function<bool(const Utils::Config::AppConfig&)> validation_func;
};

/**
 * @brief Configuration impact analysis result
 */
struct ImpactAnalysis {
  std::string changed_parameter;
  std::vector<std::string> affected_parameters;
  std::vector<std::string> affected_applications;
  std::vector<std::string> warnings;
  std::vector<std::string> recommendations;
};

/**
 * @brief Cross-application conflict information
 */
struct CrossAppConflict {
  std::string application1;
  std::string application2;
  std::string parameter;
  std::string conflict_description;
  std::vector<std::string> resolution_suggestions;
};

/**
 * @brief Unified configuration manager
 *
 * Manages configuration from multiple sources with clear precedence rules:
 * 1. Command-line arguments (highest priority)
 * 2. Environment variables
 * 3. Configuration files
 * 4. Built-in defaults (lowest priority)
 */
class ConfigurationManager {
 public:
  ConfigurationManager();
  ~ConfigurationManager() = default;

  // Prevent copying, allow moving
  ConfigurationManager(const ConfigurationManager&) = delete;
  ConfigurationManager& operator=(const ConfigurationManager&) = delete;
  ConfigurationManager(ConfigurationManager&&) = default;
  ConfigurationManager& operator=(ConfigurationManager&&) = default;

  /**
   * @brief Load configuration from file
   * @param config_path Path to configuration file (JSON format)
   * @return Success or error with details
   */
  [[nodiscard]] ConfigResult<void> load_from_file(const std::filesystem::path& config_path);

  /**
   * @brief Load configuration from environment variables
   * @param prefix Environment variable prefix (e.g., "SOLAR_")
   * @return Success or error with details
   */
  [[nodiscard]] ConfigResult<void> load_from_environment(const std::string& prefix = "SOLAR_");

  /**
   * @brief Apply command-line overrides
   * @param overrides Map of parameter names to values
   * @return Success or error with details
   */
  [[nodiscard]] ConfigResult<void> apply_cli_overrides(
      const std::map<std::string, std::string>& overrides);

  /**
   * @brief Get current merged configuration
   * @return Current configuration with all sources merged according to precedence
   */
  [[nodiscard]] const Utils::Config::AppConfig& get_config() const { return merged_config_; }

  /**
   * @brief Get configuration from specific source
   * @param source Configuration source to query
   * @return Configuration from that source, or nullopt if not available
   */
  [[nodiscard]] std::optional<Utils::Config::AppConfig> get_config_from_source(
      ConfigSource source) const;

  /**
   * @brief Validate current configuration
   * @return Validation result with errors, warnings, and suggestions
   */
  [[nodiscard]] ValidationResult validate() const;

  /**
   * @brief Save current configuration to file
   * @param config_path Path where to save configuration
   * @param include_defaults Whether to include default values
   * @return Success or error with details
   */
  [[nodiscard]] ConfigResult<void> save_to_file(const std::filesystem::path& config_path,
                                                bool include_defaults = false) const;

  /**
   * @brief Enable hot-reloading for configuration file
   * @param config_path Path to configuration file to watch
   * @param callback Optional callback to invoke on configuration changes
   * @return Success or error with details
   */
  [[nodiscard]] ConfigResult<void> enable_hot_reload(
      const std::filesystem::path& config_path,
      const ConfigChangeCallback& callback = nullptr);

  /**
   * @brief Disable hot-reloading
   */
  void disable_hot_reload();

  /**
   * @brief Check if hot-reloading is enabled
   */
  [[nodiscard]] bool is_hot_reload_enabled() const { return hot_reload_enabled_; }

  /**
   * @brief Create configuration backup
   * @param description Optional description for the backup
   * @return Backup snapshot
   */
  [[nodiscard]] ConfigSnapshot create_backup(const std::string& description = "") const;

  /**
   * @brief Restore configuration from backup
   * @param snapshot Backup snapshot to restore
   * @return Success or error with details
   */
  [[nodiscard]] ConfigResult<void> restore_from_backup(const ConfigSnapshot& snapshot);

  /**
   * @brief Get configuration change history
   * @param since Optional time point to filter changes
   * @return List of configuration changes
   */
  [[nodiscard]] std::vector<ConfigChange> get_change_history(
      std::optional<std::chrono::system_clock::time_point> since = std::nullopt) const;

  /**
   * @brief Get configuration source for a specific parameter
   * @param parameter_name Name of the parameter
   * @return Source that provided the current value
   */
  [[nodiscard]] ConfigSource get_parameter_source(const std::string& parameter_name) const;

  /**
   * @brief Get precedence documentation
   * @return Human-readable explanation of precedence rules
   */
  [[nodiscard]] static std::string get_precedence_documentation();

  /**
   * @brief Register configuration change callback
   * @param callback Function to call when configuration changes
   */
  void register_change_callback(const ConfigChangeCallback& callback);

  /**
   * @brief Clear all configuration change callbacks
   */
  void clear_change_callbacks();

  /**
   * @brief Reset to default configuration
   */
  void reset_to_defaults();

  /**
   * @brief Detect cross-application configuration conflicts
   * @param app_configs Map of application names to their configurations
   * @return List of detected conflicts
   */
  [[nodiscard]] std::vector<CrossAppConflict> detect_cross_app_conflicts(
      const std::map<std::string, Utils::Config::AppConfig>& app_configs) const;

  /**
   * @brief Register configuration dependency
   * @param dependency Dependency information
   */
  void register_dependency(const ConfigDependency& dependency);

  /**
   * @brief Validate configuration dependencies
   * @return Validation result with dependency violations
   */
  [[nodiscard]] ValidationResult validate_dependencies() const;

  /**
   * @brief Analyze impact of configuration change
   * @param parameter_name Parameter being changed
   * @param new_value New value for the parameter
   * @return Impact analysis result
   */
  [[nodiscard]] ImpactAnalysis analyze_impact(const std::string& parameter_name,
                                              const std::string& new_value) const;

  /**
   * @brief Get all registered dependencies
   */
  [[nodiscard]] std::vector<ConfigDependency> get_dependencies() const;

  /**
   * @brief Check if configuration change would cause conflicts
   * @param parameter_name Parameter to change
   * @param new_value New value
   * @return True if change would cause conflicts
   */
  [[nodiscard]] bool would_cause_conflict(const std::string& parameter_name,
                                          const std::string& new_value) const;

 private:
  // Configuration from different sources
  Utils::Config::AppConfig default_config_;
  std::optional<Utils::Config::AppConfig> file_config_;
  std::optional<Utils::Config::AppConfig> env_config_;
  std::optional<Utils::Config::AppConfig> cli_config_;

  // Merged configuration (result of applying precedence rules)
  Utils::Config::AppConfig merged_config_;

  // Source tracking for each parameter
  std::map<std::string, ConfigSource> parameter_sources_;

  // Hot-reload support
  bool hot_reload_enabled_ = false;
  std::filesystem::path hot_reload_path_;
  std::filesystem::file_time_type last_reload_time_;

  // Change tracking
  std::vector<ConfigChange> change_history_;
  std::vector<ConfigChangeCallback> change_callbacks_;

  // Backup management
  std::vector<ConfigSnapshot> backups_;
  static constexpr size_t MAX_BACKUPS = 10;

  // Dependency tracking
  std::vector<ConfigDependency> dependencies_;
  std::map<std::string, std::vector<std::string>> parameter_dependencies_;  // param -> depends on

  /**
   * @brief Merge configurations according to precedence rules
   */
  void merge_configurations();

  /**
   * @brief Track configuration change
   */
  void track_change(const std::string& param_name, const std::string& old_val,
                    const std::string& new_val, ConfigSource source);

  /**
   * @brief Notify callbacks of configuration change
   */
  void notify_change(const ConfigChange& change);

  /**
   * @brief Check if configuration file has been modified
   */
  [[nodiscard]] bool has_file_changed() const;

  /**
   * @brief Reload configuration from file if changed
   */
  [[nodiscard]] ConfigResult<void> reload_if_changed();

  /**
   * @brief Parse environment variable name to configuration parameter
   */
  [[nodiscard]] static std::optional<std::string> parse_env_var_name(const std::string& env_name,
                                                                     const std::string& prefix);

  /**
   * @brief Apply configuration value with type conversion
   */
  [[nodiscard]] ConfigResult<void> apply_config_value(Utils::Config::AppConfig& config,
                                                      const std::string& param_name,
                                                      const std::string& value);

  /**
   * @brief Initialize default dependencies
   */
  void initialize_default_dependencies();

  /**
   * @brief Check for circular dependencies
   */
  [[nodiscard]] bool has_circular_dependency(const std::string& param) const;

  /**
   * @brief Get parameters that depend on the given parameter
   */
  [[nodiscard]] std::vector<std::string> get_dependent_parameters(
      const std::string& param) const;
};

/**
 * @brief Global configuration manager instance
 *
 * Provides singleton access to the configuration manager for the application.
 * This ensures consistent configuration across all components.
 */
class GlobalConfig {
 public:
  /**
   * @brief Get the global configuration manager instance
   */
  static ConfigurationManager& instance();

  /**
   * @brief Initialize global configuration with file
   */
  static ConfigResult<void> initialize(const std::filesystem::path& config_path);

  /**
   * @brief Initialize global configuration with multiple sources
   */
  static ConfigResult<void> initialize(const std::filesystem::path& config_path,
                                      const std::map<std::string, std::string>& cli_overrides);

 private:
  static std::unique_ptr<ConfigurationManager> instance_;
};

}  // namespace SolarSystem::Core::Config
