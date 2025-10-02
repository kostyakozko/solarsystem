#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/utils/expected.hpp"
#include "solar_utils/config.hpp"

namespace SolarSystem::Utils::Advanced {

/**
 * @brief Parameter validation result
 */
struct ValidationResult {
  bool is_valid = false;
  std::string error_message;
  std::vector<std::string> warnings;
  std::vector<std::string> suggestions;
  std::string normalized_value;
};

/**
 * @brief Configuration validation result
 */
struct ConfigValidationResult {
  bool is_valid = false;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::vector<std::string> suggestions;
};

/**
 * @brief Configuration conflict information
 */
struct ConfigConflict {
  std::string parameter1;
  std::string parameter2;
  std::string description;
  std::vector<std::string> resolution_suggestions;
};

/**
 * @brief Parameter definition for validation
 */
struct ParameterDefinition {
  std::string name;
  std::string type;  // "int", "double", "string", "bool"
  std::string description;
  std::optional<std::string> default_value;
  std::vector<std::string> allowed_values;
  std::optional<double> min_value;
  std::optional<double> max_value;
  bool required = false;
};

/**
 * @brief Configuration template
 */
struct ConfigTemplate {
  std::string name;
  std::string description;
  Config::AppConfig config;
};

/**
 * @brief Configuration preset
 */
struct ConfigPreset {
  std::string name;
  std::string description;
  std::map<std::string, std::string> parameters;
};

/**
 * @brief Advanced configuration manager
 */
class AdvancedConfigManager {
 public:
  AdvancedConfigManager();
  ~AdvancedConfigManager() = default;

  // Parameter validation
  [[nodiscard]] ValidationResult validate_parameter(const std::string& name,
                                                    const std::string& value) const;

  // Configuration validation
  [[nodiscard]] ConfigValidationResult validate_configuration(
      const Config::AppConfig& config) const;

  // Conflict detection
  [[nodiscard]] std::vector<ConfigConflict> detect_conflicts(
      const Config::AppConfig& config) const;

  // Template management
  [[nodiscard]] std::optional<Config::AppConfig> apply_template(const std::string& name) const;
  [[nodiscard]] std::vector<std::string> get_available_templates() const;
  [[nodiscard]] std::optional<ConfigTemplate> get_template(const std::string& name) const;

  // Preset management
  [[nodiscard]] std::optional<Config::AppConfig> apply_preset(const std::string& name,
                                                              const Config::AppConfig& base) const;
  [[nodiscard]] std::vector<std::string> get_available_presets() const;
  [[nodiscard]] std::optional<ConfigPreset> get_preset(const std::string& name) const;

  // Parameter definitions
  [[nodiscard]] std::vector<ParameterDefinition> get_parameter_definitions() const;
  [[nodiscard]] std::optional<ParameterDefinition> get_parameter_definition(
      const std::string& name) const;

  // Configuration migration
  [[nodiscard]] SolarSystem::Utils::Expected<Config::AppConfig, std::string> migrate_configuration(
      const Config::AppConfig& old_config, const std::string& from_version,
      const std::string& to_version) const;

  // Configuration upgrade
  [[nodiscard]] SolarSystem::Utils::Expected<Config::AppConfig, std::string> upgrade_configuration(
      const Config::AppConfig& config) const;

 private:
  std::map<std::string, ParameterDefinition> parameter_definitions_;
  std::map<std::string, ConfigTemplate> templates_;
  std::map<std::string, ConfigPreset> presets_;

  void initialize_parameter_definitions();
  void initialize_templates();
  void initialize_presets();

  [[nodiscard]] ValidationResult validate_integer_parameter(const ParameterDefinition& def,
                                                            const std::string& value) const;
  [[nodiscard]] ValidationResult validate_double_parameter(const ParameterDefinition& def,
                                                           const std::string& value) const;
  [[nodiscard]] ValidationResult validate_string_parameter(const ParameterDefinition& def,
                                                           const std::string& value) const;
  [[nodiscard]] ValidationResult validate_bool_parameter(const ParameterDefinition& def,
                                                         const std::string& value) const;
};

}  // namespace SolarSystem::Utils::Advanced
