/**
 * @file advanced_config.hpp
 * @brief Advanced configuration management system for Solar System Suite
 *
 * Implements Task 7 requirements:
 * - Comprehensive parameter validation system
 * - Configuration templates and presets
 * - Configuration conflict detection and resolution
 * - Configuration migration and upgrade support
 *
 * Requirements: 3.1, 3.4, 6.1, 6.2
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "solar_core/utils/expected.hpp"
#include "solar_utils/config.hpp"
#include "solar_utils/validation/input_validator.hpp"

namespace SolarSystem::Utils::Advanced {

/**
 * @brief Configuration validation error types
 */
enum class ConfigValidationError {
  InvalidValue,
  MissingRequired,
  ConflictingValues,
  OutOfRange,
  InvalidType,
  DependencyNotMet,
  CircularDependency,
  TemplateNotFound,
  MigrationFailed,
  SchemaViolation
};

/**
 * @brief Detailed configuration validation result
 */
struct ConfigValidationResult {
  bool is_valid = false;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::vector<std::string> suggestions;
  std::map<std::string, std::string> auto_fixes;

  ConfigValidationResult() = default;
  ConfigValidationResult(bool valid) : is_valid(valid) {}

  void add_error(const std::string& error) {
    is_valid = false;
    errors.push_back(error);
  }

  void add_warning(const std::string& warning) {
    warnings.push_back(warning);
  }

  void add_suggestion(const std::string& suggestion) {
    suggestions.push_back(suggestion);
  }

  void add_auto_fix(const std::string& key, const std::string& value) {
    auto_fixes[key] = value;
  }
};

/**
 * @brief Configuration parameter definition with validation rules
 */
struct ParameterDefinition {
  std::string name;
  std::string description;
  std::string type;  // "int", "double", "string", "bool", "duration", "path"
  bool required = false;
  std::variant<int64_t, double, std::string, bool> default_value;

  // Validation constraints
  std::optional<std::variant<int64_t, double>> min_value;
  std::optional<std::variant<int64_t, double>> max_value;
  std::vector<std::string> allowed_values;
  std::string validation_pattern;  // regex pattern

  // Dependencies and conflicts
  std::vector<std::string> depends_on;
  std::vector<std::string> conflicts_with;
  std::function<bool(const Config::ConfigMap&)> custom_validator;

  // Migration support
  std::vector<std::string> deprecated_names;  // Old parameter names
  std::string migration_note;

  ParameterDefinition() = default;
  ParameterDefinition(const std::string& n, const std::string& desc, const std::string& t)
      : name(n), description(desc), type(t) {}
};

/**
 * @brief Configuration template for common setups
 */
struct ConfigTemplate {
  std::string name;
  std::string description;
  std::string category;  // "simulation", "data", "web", "development", "production"
  Config::AppConfig base_config;
  std::map<std::string, std::string> template_variables;  // Placeholders to fill
  std::vector<std::string> required_parameters;
  std::string usage_example;

  ConfigTemplate() = default;
  ConfigTemplate(const std::string& n, const std::string& desc, const std::string& cat)
      : name(n), description(desc), category(cat) {}
};

/**
 * @brief Configuration preset for specific use cases
 */
struct ConfigPreset {
  std::string name;
  std::string description;
  std::string use_case;  // "fast_simulation", "high_accuracy", "web_server", "development"
  Config::AppConfig config;
  std::vector<std::string> tags;
  bool is_system_preset = false;  // System vs user-defined presets

  ConfigPreset() = default;
  ConfigPreset(const std::string& n, const std::string& desc, const std::string& uc)
      : name(n), description(desc), use_case(uc) {}
};

/**
 * @brief Configuration conflict information
 */
struct ConfigConflict {
  std::string type;  // "mutual_exclusion", "dependency_missing", "value_conflict"
  std::vector<std::string> conflicting_parameters;
  std::string description;
  std::vector<std::string> resolution_options;
  std::string recommended_resolution;

  ConfigConflict() = default;
  ConfigConflict(const std::string& t, const std::vector<std::string>& params, const std::string& desc)
      : type(t), conflicting_parameters(params), description(desc) {}
};

/**
 * @brief Configuration migration rule
 */
struct MigrationRule {
  std::string from_version;
  std::string to_version;
  std::string parameter_name;
  std::string old_name;
  std::string new_name;
  std::function<std::string(const std::string&)> value_transformer;
  std::string migration_note;
  bool is_breaking_change = false;

  MigrationRule() = default;
  MigrationRule(const std::string& from, const std::string& to, const std::string& param)
      : from_version(from), to_version(to), parameter_name(param) {}
};

/**
 * @brief Advanced configuration manager
 */
class AdvancedConfigManager {
public:
  /**
   * @brief Initialize the advanced configuration manager
   */
  AdvancedConfigManager();

  /**
   * @brief Load configuration with aidation
   */
  Expected<Config::AppConfig, std::string> load_configuration(
      const std::optional<std::string>& config_file = std::nullopt,
      const std::vector<std::string>& cli_args = {},
      const std::string& preset_name = "",
      const std::string& template_name = ""
  );

  /**
   * @brief Validate configuration comprehensively
   */
  ConfigValidationResult validate_configuration(const Config::AppConfig& config) const;

  /**
   * @brief Validate individual parameter
   */
  Validation::ValidationResult validate_parameter(
      const std::string& name,
      const std::string& value
  ) const;

  /**
   * @brief Detect configuration conflicts
   */
  std::vector<ConfigConflict> detect_conflicts(const Config::AppConfig& config) const;

  /**
   * @brief Resolve configuration conflicts automatically where possible
   */
  Expected<Config::AppConfig, std::string> resolve_conflicts(
      const Config::AppConfig& config,
      const std::vector<std::string>& resolution_preferences = {}
  ) const;

  /**
   * @brief Apply configuration template
   */
  Expected<Config::AppConfig, std::string> apply_template(
      const std::string& template_name,
      const std::map<std::string, std::string>& variables = {}
  ) const;

  /**
   * @brief Load configuration preset
   */
  Expected<Config::AppConfig, std::string> load_preset(const std::string& preset_name) const;

  /**
   * @brief Save configuration as preset
   */
  Expected<void, std::string> save_preset(
      const std::string& name,
      const Config::AppConfig& config,
      const std::string& description = "",
      const std::string& use_case = ""
  );

  /**
   * @brief Migrate configuration from older version
   */
  Expected<Config::AppConfig, std::string> migrate_configuration(
      const Config::AppConfig& old_config,
      const std::string& from_version,
      const std::string& to_version
  ) const;

  /**
   * @brief Get available templates
   */
  std::vector<ConfigTemplate> get_available_templates() const;

  /**
   * @brief Get available presets
   */
  std::vector<ConfigPreset> get_available_presets() const;

  /**
   * @brief Get parameter definitions
   */
  std::vector<ParameterDefinition> get_parameter_definitions() const;

  /**
   * @brief Generate configuration documentation
   */
  std::string generate_documentation() const;

  /**
   * @brief Export configuration with metadata
   */
  Expected<void, std::string> export_configuration(
      const Config::AppConfig& config,
      const std::filesystem::path& output_path,
      const std::string& format = "json"  // "json", "yaml", "ini"
  ) const;

  /**
   * @brief Import configuration with validation
   */
  Expected<Config::AppConfig, std::string> import_configuration(
      const std::filesystem::path& input_path
  ) const;

  /**
   * @brief Register custom parameter definition
   */
  void register_parameter(const ParameterDefinition& param_def);

  /**
   * @brief Register custom template
   */
  void register_template(const ConfigTemplate& template_def);

  /**
   * @brief Register custom preset
   */
  void register_preset(const ConfigPreset& preset_def);

  /**
   * @brief Register migration rule
   */
  void register_migration_rule(const MigrationRule& rule);

private:
  std::map<std::string, ParameterDefinition> parameter_definitions_;
  std::map<std::string, ConfigTemplate> templates_;
  std::map<std::string, ConfigPreset> presets_;
  std::vector<MigrationRule> migration_rules_;

  /**
   * @brief Initialize built-in parameter definitions
   */
  void initialize_parameter_definitions();

  /**
   * @brief Initialize built-in templates
   */
  void initialize_templates();

  /**
   * @brief Initialize built-in presets
   */
  void initialize_presets();

  /**
   * @brief Initialize migration rules
   */
  void initialize_migration_rules();

  /**
   * @brief Validate parameter against definition
   */
  Validation::ValidationResult validate_against_definition(
      const ParameterDefinition& def,
      const std::string& value
  ) const;

  /**
   * @brief Check parameter dependencies
   */
  std::vector<std::string> check_dependencies(
      const std::string& param_name,
      const Config::ConfigMap& config_map
  ) const;

  /**
   * @brief Check parameter conflicts
   */
  std::vector<std::string> check_conflicts(
      const std::string& param_name,
      const Config::ConfigMap& config_map
  ) const;

  /**
   * @brief Apply migration rules
   */
  Config::ConfigMap apply_migration_rules(
      const Config::ConfigMap& old_config,
      const std::string& from_version,
      const std::string& to_version
  ) const;

  /**
   * @brief Convert AppConfig to ConfigMap for validation
   */
  Config::ConfigMap config_to_map(const Config::AppConfig& config) const;

  /**
   * @brief Convert ConfigMap to AppConfig after validation
   */
  Expected<Config::AppConfig, std::string> map_to_config(const Config::ConfigMap& map) const;

  /**
   * @brief Load presets from file system
   */
  void load_user_presets();

  /**
   * @brief Save preset to file system
   */
  Expected<void, std::string> save_preset_to_file(const ConfigPreset& preset) const;

  /**
   * @brief Get preset file path
   */
  std::filesystem::path get_preset_file_path(const std::string& preset_name) const;

  /**
   * @brief Get presets directory
   */
  std::filesystem::path get_presets_directory() const;
};

/**
 * @brief Configuration builder for fluent API
 */
class ConfigurationBuilder {
public:
  ConfigurationBuilder() = default;

  /**
   * @brief Start with a template
   */
  ConfigurationBuilder& from_template(const std::string& template_name);

  /**
   * @brief Start with a preset
   */
  ConfigurationBuilder& from_preset(const std::string& preset_name);

  /**
   * @brief Set simulation parameters
   */
  ConfigurationBuilder& simulation_timestep(double timestep);
  ConfigurationBuilder& simulation_max_iterations(size_t max_iterations);
  ConfigurationBuilder& simulation_output_format(const std::string& format);

  /**
   * @brief Set data parameters
   */
  ConfigurationBuilder& data_cache_directory(const std::string& directory);
  ConfigurationBuilder& data_jpl_api_url(const std::string& url);
  ConfigurationBuilder& data_cache_max_age_days(size_t days);

  /**
   * @brief Set logging parameters
   */
  ConfigurationBuilder& logging_level(Logger::Level level);
  ConfigurationBuilder& logging_file(const std::string& file);
  ConfigurationBuilder& logging_colored_output(bool enabled);

  /**
   * @brief Set web parameters
   */
  ConfigurationBuilder& web_port(uint16_t port);
  ConfigurationBuilder& web_host(const std::string& host);
  ConfigurationBuilder& web_enable_cors(bool enabled);

  /**
   * @brief Set custom parameter
   */
  ConfigurationBuilder& set_parameter(const std::string& key, const std::string& value);

  /**
   * @brief Validate and build configuration
   */
  Expected<Config::AppConfig, std::string> build();

  /**
   * @brief Build with advanced manager
   */
  Expected<Config::AppConfig, std::string> build_with_manager(
      const AdvancedConfigManager& manager
  );

private:
  Config::AppConfig config_;
  std::map<std::string, std::string> custom_parameters_;
  std::string template_name_;
  std::string preset_name_;

  /**
   * @brief Apply template if specified
   */
  void apply_template_if_specified();

  /**
   * @brief Apply preset if specified
   */
  void apply_preset_if_specified();
};

/**
 * @brief Configuration validator with comprehensive rules
 */
class ConfigurationValidator {
public:
  /**
   * @brief Validate configuration with detailed reporting
   */
  static ConfigValidationResult validate_comprehensive(
      const Config::AppConfig& config,
      const std::map<std::string, ParameterDefinition>& parameter_definitions
  );

  /**
   * @brief Validate simulation configuration
   */
  static ConfigValidationResult validate_simulation_config(
      const Config::SimulationConfig& config
  );

  /**
   * @brief Validate data configuration
   */
  static ConfigValidationResult validate_data_config(
      const Config::DataConfig& config
  );

  /**
   * @brief Validate logging configuration
   */
  static ConfigValidationResult validate_logging_config(
      const Config::LoggingConfig& config
  );

  /**
   * @brief Validate web configuration
   */
  static ConfigValidationResult validate_web_config(
      const Config::WebConfig& config
  );

  /**
   * @brief Check for common configuration issues
   */
  static std::vector<std::string> check_common_issues(
      const Config::AppConfig& config
  );

  /**
   * @brief Generate configuration recommendations
   */
  static std::vector<std::string> generate_recommendations(
      const Config::AppConfig& config
  );

private:
  /**
   * @brief Validate numeric range
   */
  template<typename T>
  static bool validate_range(T value, T min_val, T max_val);

  /**
   * @brief Validate file path accessibility
   */
  static bool validate_file_path(const std::string& path, bool must_exist = false);

  /**
   * @brief Validate network configuration
   */
  static bool validate_network_config(const std::string& host, uint16_t port);
};

}  // namespace SolarSystem::Utils::Advanced
