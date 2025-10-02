#include "solar_utils/advanced_config.hpp"

#include <algorithm>
#include <sstream>

namespace SolarSystem::Utils::Advanced {

AdvancedConfigManager::AdvancedConfigManager() {
  initialize_parameter_definitions();
  initialize_templates();
  initialize_presets();
}

ValidationResult AdvancedConfigManager::validate_parameter(const std::string& name,
                                                           const std::string& value) const {
  auto def_it = parameter_definitions_.find(name);
  if (def_it == parameter_definitions_.end()) {
    ValidationResult result;
    result.is_valid = false;
    result.error_message = "Unknown parameter: " + name;
    return result;
  }

  const auto& def = def_it->second;

  if (def.type == "int") {
    return validate_integer_parameter(def, value);
  } else if (def.type == "double") {
    return validate_double_parameter(def, value);
  } else if (def.type == "string") {
    return validate_string_parameter(def, value);
  } else if (def.type == "bool") {
    return validate_bool_parameter(def, value);
  }

  ValidationResult result;
  result.is_valid = false;
  result.error_message = "Unknown parameter type: " + def.type;
  return result;
}

ConfigValidationResult AdvancedConfigManager::validate_configuration(
    const Config::AppConfig& config) const {
  ConfigValidationResult result;
  result.is_valid = true;

  // Validate simulation parameters
  if (config.simulation.timestep <= 0.0) {
    result.is_valid = false;
    result.errors.push_back("Simulation timestep must be positive");
  } else if (config.simulation.timestep < 1.0) {
    result.warnings.push_back("Very small timestep may cause performance issues");
  }

  if (config.simulation.max_iterations <= 0) {
    result.is_valid = false;
    result.errors.push_back("Maximum iterations must be positive");
  }

  // Validate logging configuration
  if (config.logging.log_file.empty() && !config.logging.console_output) {
    result.warnings.push_back("No logging output configured");
  }

  // Validate output configuration
  if (config.output.format != "json" && config.output.format != "csv" &&
      config.output.format != "binary") {
    result.is_valid = false;
    result.errors.push_back("Invalid output format: " + config.output.format);
    result.suggestions.push_back("Use 'json', 'csv', or 'binary'");
  }

  return result;
}

std::vector<ConfigConflict> AdvancedConfigManager::detect_conflicts(
    const Config::AppConfig& config) const {
  std::vector<ConfigConflict> conflicts;

  // Check for debug mode with production settings
  if (config.debug_mode && config.logging.min_level == Logger::Level::ERROR) {
    ConfigConflict conflict;
    conflict.parameter1 = "debug_mode";
    conflict.parameter2 = "logging.min_level";
    conflict.description = "Debug mode enabled but logging level set to ERROR";
    conflict.resolution_suggestions.push_back("Set logging level to DEBUG or INFO");
    conflict.resolution_suggestions.push_back("Disable debug mode for production");
    conflicts.push_back(conflict);
  }

  // Check for performance conflicts
  if (config.simulation.timestep < 1.0 && config.simulation.max_iterations > 1000000) {
    ConfigConflict conflict;
    conflict.parameter1 = "simulation.timestep";
    conflict.parameter2 = "simulation.max_iterations";
    conflict.description = "Small timestep with high iteration count may cause performance issues";
    conflict.resolution_suggestions.push_back("Increase timestep or reduce max iterations");
    conflicts.push_back(conflict);
  }

  return conflicts;
}

std::optional<Config::AppConfig> AdvancedConfigManager::apply_template(
    const std::string& name) const {
  auto template_it = templates_.find(name);
  if (template_it == templates_.end()) {
    return std::nullopt;
  }

  return template_it->second.config;
}

std::vector<std::string> AdvancedConfigManager::get_available_templates() const {
  std::vector<std::string> names;
  for (const auto& [name, template_] : templates_) {
    names.push_back(name);
  }
  return names;
}

std::optional<ConfigTemplate> AdvancedConfigManager::get_template(const std::string& name) const {
  auto template_it = templates_.find(name);
  if (template_it == templates_.end()) {
    return std::nullopt;
  }
  return template_it->second;
}

std::optional<Config::AppConfig> AdvancedConfigManager::apply_preset(
    const std::string& name, const Config::AppConfig& base) const {
  auto preset_it = presets_.find(name);
  if (preset_it == presets_.end()) {
    return std::nullopt;
  }

  Config::AppConfig result = base;
  const auto& preset = preset_it->second;

  // Apply preset parameters to base configuration
  for (const auto& [param_name, param_value] : preset.parameters) {
    // This is a simplified implementation - in a real system you'd have
    // a more sophisticated parameter application mechanism
    if (param_name == "simulation.timestep") {
      result.simulation.timestep = std::stod(param_value);
    } else if (param_name == "simulation.max_iterations") {
      result.simulation.max_iterations = std::stoi(param_value);
    } else if (param_name == "output.format") {
      result.output.format = param_value;
    }
    // Add more parameter mappings as needed
  }

  return result;
}

std::vector<std::string> AdvancedConfigManager::get_available_presets() const {
  std::vector<std::string> names;
  for (const auto& [name, preset] : presets_) {
    names.push_back(name);
  }
  return names;
}

std::optional<ConfigPreset> AdvancedConfigManager::get_preset(const std::string& name) const {
  auto preset_it = presets_.find(name);
  if (preset_it == presets_.end()) {
    return std::nullopt;
  }
  return preset_it->second;
}

std::vector<ParameterDefinition> AdvancedConfigManager::get_parameter_definitions() const {
  std::vector<ParameterDefinition> definitions;
  for (const auto& [name, def] : parameter_definitions_) {
    definitions.push_back(def);
  }
  return definitions;
}

std::optional<ParameterDefinition> AdvancedConfigManager::get_parameter_definition(
    const std::string& name) const {
  auto def_it = parameter_definitions_.find(name);
  if (def_it == parameter_definitions_.end()) {
    return std::nullopt;
  }
  return def_it->second;
}

SolarSystem::Utils::Expected<Config::AppConfig, std::string>
AdvancedConfigManager::migrate_configuration(const Config::AppConfig& old_config,
                                              const std::string& from_version,
                                              const std::string& to_version) const {
  // Simple migration logic - in a real system this would be more sophisticated
  Config::AppConfig migrated = old_config;

  if (from_version == "1.0" && to_version == "2.0") {
    // Example migration: convert old timestep format
    if (migrated.simulation.timestep > 86400.0) {
      migrated.simulation.timestep = 3600.0;  // Convert from days to hours
    }
  }

  return SolarSystem::Utils::Expected<Config::AppConfig, std::string>{migrated};
}

SolarSystem::Utils::Expected<Config::AppConfig, std::string>
AdvancedConfigManager::upgrade_configuration(const Config::AppConfig& config) const {
  // Upgrade to latest version
  return migrate_configuration(config, "1.0", "2.0");
}

// Private methods

void AdvancedConfigManager::initialize_parameter_definitions() {
  // Simulation parameters
  parameter_definitions_["simulation.timestep"] = {
      .name = "simulation.timestep",
      .type = "double",
      .description = "Simulation timestep in seconds",
      .default_value = "3600.0",
      .min_value = 0.1,
      .max_value = 86400.0,
      .required = false};

  parameter_definitions_["simulation.max_iterations"] = {
      .name = "simulation.max_iterations",
      .type = "int",
      .description = "Maximum number of simulation iterations",
      .default_value = "1000000",
      .min_value = 1,
      .max_value = 100000000,
      .required = false};

  parameter_definitions_["simulation.output_format"] = {
      .name = "simulation.output_format",
      .type = "string",
      .description = "Output format for simulation results",
      .default_value = "json",
      .allowed_values = {"json", "csv", "binary"},
      .required = false};

  // Add more parameter definitions as needed
}

void AdvancedConfigManager::initialize_templates() {
  // Development template
  ConfigTemplate dev_template;
  dev_template.name = "development";
  dev_template.description = "Configuration optimized for development";
  dev_template.config = Config::get_default();
  dev_template.config.debug_mode = true;
  dev_template.config.logging.min_level = Logger::Level::DEBUG;
  dev_template.config.logging.colored_output = true;
  dev_template.config.logging.console_output = true;
  templates_["development"] = dev_template;

  // Production template
  ConfigTemplate prod_template;
  prod_template.name = "production";
  prod_template.description = "Configuration optimized for production";
  prod_template.config = Config::get_default();
  prod_template.config.debug_mode = false;
  prod_template.config.logging.min_level = Logger::Level::INFO;
  prod_template.config.logging.colored_output = false;
  prod_template.config.logging.console_output = false;
  prod_template.config.logging.log_file = "solar_system.log";
  templates_["production"] = prod_template;

  // Performance template
  ConfigTemplate perf_template;
  perf_template.name = "performance";
  perf_template.description = "Configuration optimized for performance";
  perf_template.config = Config::get_default();
  perf_template.config.simulation.timestep = 86400.0;  // 1 day
  perf_template.config.logging.min_level = Logger::Level::WARNING;
  templates_["performance"] = perf_template;
}

void AdvancedConfigManager::initialize_presets() {
  // Fast simulation preset
  ConfigPreset fast_preset;
  fast_preset.name = "fast";
  fast_preset.description = "Fast simulation with reduced accuracy";
  fast_preset.parameters["simulation.timestep"] = "86400.0";
  fast_preset.parameters["simulation.max_iterations"] = "100000";
  fast_preset.parameters["output.format"] = "binary";
  presets_["fast"] = fast_preset;

  // Accurate simulation preset
  ConfigPreset accurate_preset;
  accurate_preset.name = "accurate";
  accurate_preset.description = "Accurate simulation with smaller timestep";
  accurate_preset.parameters["simulation.timestep"] = "3600.0";
  accurate_preset.parameters["simulation.max_iterations"] = "10000000";
  accurate_preset.parameters["output.format"] = "json";
  presets_["accurate"] = accurate_preset;
}

ValidationResult AdvancedConfigManager::validate_integer_parameter(
    const ParameterDefinition& def, const std::string& value) const {
  ValidationResult result;

  try {
    int int_value = std::stoi(value);

    if (def.min_value.has_value() && int_value < def.min_value.value()) {
      result.is_valid = false;
      result.error_message = "Value " + value + " is below minimum " +
                             std::to_string(static_cast<int>(def.min_value.value()));
      return result;
    }

    if (def.max_value.has_value() && int_value > def.max_value.value()) {
      result.is_valid = false;
      result.error_message = "Value " + value + " is above maximum " +
                             std::to_string(static_cast<int>(def.max_value.value()));
      return result;
    }

    result.is_valid = true;
    result.normalized_value = std::to_string(int_value);
  } catch (const std::exception&) {
    result.is_valid = false;
    result.error_message = "Invalid integer value: " + value;
  }

  return result;
}

ValidationResult AdvancedConfigManager::validate_double_parameter(
    const ParameterDefinition& def, const std::string& value) const {
  ValidationResult result;

  try {
    double double_value = std::stod(value);

    if (def.min_value.has_value() && double_value < def.min_value.value()) {
      result.is_valid = false;
      result.error_message = "Value " + value + " is below minimum " +
                             std::to_string(def.min_value.value());
      return result;
    }

    if (def.max_value.has_value() && double_value > def.max_value.value()) {
      result.is_valid = false;
      result.error_message = "Value " + value + " is above maximum " +
                             std::to_string(def.max_value.value());
      return result;
    }

    result.is_valid = true;
    result.normalized_value = std::to_string(double_value);
  } catch (const std::exception&) {
    result.is_valid = false;
    result.error_message = "Invalid double value: " + value;
  }

  return result;
}

ValidationResult AdvancedConfigManager::validate_string_parameter(
    const ParameterDefinition& def, const std::string& value) const {
  ValidationResult result;

  if (!def.allowed_values.empty()) {
    auto it = std::find(def.allowed_values.begin(), def.allowed_values.end(), value);
    if (it == def.allowed_values.end()) {
      result.is_valid = false;
      result.error_message = "Invalid value: " + value;
      result.suggestions = def.allowed_values;
      return result;
    }
  }

  result.is_valid = true;
  result.normalized_value = value;
  return result;
}

ValidationResult AdvancedConfigManager::validate_bool_parameter(
    const ParameterDefinition& def, const std::string& value) const {
  ValidationResult result;

  std::string lower_value = value;
  std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);

  if (lower_value == "true" || lower_value == "1" || lower_value == "yes" || lower_value == "on") {
    result.is_valid = true;
    result.normalized_value = "true";
  } else if (lower_value == "false" || lower_value == "0" || lower_value == "no" ||
             lower_value == "off") {
    result.is_valid = true;
    result.normalized_value = "false";
  } else {
    result.is_valid = false;
    result.error_message = "Invalid boolean value: " + value;
    result.suggestions = {"true", "false", "1", "0", "yes", "no", "on", "off"};
  }

  return result;
}

}  // namespace SolarSystem::Utils::Advanced
