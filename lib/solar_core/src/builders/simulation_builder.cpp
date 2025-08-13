#include "solar_core/builders/simulation_builder.hpp"

#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <regex>

#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/data/body_definitions.hpp"

namespace SolarSystem::Core::Builders {

// === SimulationBuilder Implementation ===

SimulationBuilder::SimulationBuilder() {
  // Use default values
}

SimulationBuilder::SimulationBuilder(double timestep) : timestep_(timestep) {}

SimulationBuilder& SimulationBuilder::with_bodies(Bodies::BodyCollection bodies) {
  bodies_ = std::move(bodies);
  return *this;
}

SimulationBuilder& SimulationBuilder::with_timestep(double timestep_seconds) {
  timestep_ = timestep_seconds;
  return *this;
}

SimulationBuilder& SimulationBuilder::with_max_iterations(size_t max_iterations) {
  max_iterations_ = max_iterations;
  return *this;
}

SimulationBuilder& SimulationBuilder::with_target_date(const std::string& iso_date) {
  try {
    target_date_ = parse_iso_date(iso_date);
  } catch (const std::exception& e) {
    LOG_ERROR("SimulationBuilder", "Failed to parse date: " + std::string(e.what()));
  }
  return *this;
}

SimulationBuilder& SimulationBuilder::with_target_date(std::time_t target_time) {
  target_date_ = std::chrono::system_clock::from_time_t(target_time);
  return *this;
}

SimulationBuilder& SimulationBuilder::with_target_date(
    std::chrono::system_clock::time_point target) {
  target_date_ = target;
  return *this;
}

SimulationBuilder& SimulationBuilder::with_progress(bool enable_progress) {
  enable_progress_ = enable_progress;
  return *this;
}

SimulationBuilder& SimulationBuilder::with_progress_callback(ProgressCallback callback) {
  progress_callback_ = std::move(callback);
  enable_progress_ = true;  // Auto-enable progress when callback is set
  return *this;
}

SimulationBuilder& SimulationBuilder::with_convergence_threshold(double threshold) {
  convergence_threshold_ = threshold;
  return *this;
}

SimulationBuilder& SimulationBuilder::with_verbose_output(bool verbose) {
  verbose_output_ = verbose;
  return *this;
}

SimulationBuilder& SimulationBuilder::with_validation(ValidationCallback validator) {
  validator_ = std::move(validator);
  return *this;
}

SimulationBuilder& SimulationBuilder::from_defaults() {
  timestep_ = 3600.0;
  max_iterations_ = 1000000;
  return *this;
}

SimulationBuilder& SimulationBuilder::from_timestep(double timestep) {
  timestep_ = timestep;
  return *this;
}

SimulationBuilder& SimulationBuilder::reset_to_defaults() {
  timestep_ = 3600.0;
  max_iterations_ = 1000000;
  enable_progress_ = true;
  verbose_output_ = false;
  convergence_threshold_ = 1e-12;
  bodies_.reset();
  target_date_.reset();
  progress_callback_.reset();
  validator_.reset();
  return *this;
}

bool SimulationBuilder::validate(std::string* error_message) const {
  auto result = validate_comprehensive();
  if (error_message && !result.is_valid && !result.errors.empty()) {
    *error_message = result.errors[0].message;
  }
  return result.is_valid;
}

ValidationResult SimulationBuilder::validate_comprehensive() const {
  ValidationResult result;

  // Update validation timestamp
  last_validation_time_ = std::chrono::system_clock::now();

  // Validate individual parameters
  auto timestep_result = validate_timestep();
  auto iterations_result = validate_max_iterations();
  auto threshold_result = validate_convergence_threshold();
  auto bodies_result = validate_bodies();
  auto date_result = validate_target_date();

  // Merge individual validation results
  for (const auto& error : timestep_result.errors) result.add_error(error);
  for (const auto& warning : timestep_result.warnings) result.add_error(warning);

  for (const auto& error : iterations_result.errors) result.add_error(error);
  for (const auto& warning : iterations_result.warnings) result.add_error(warning);

  for (const auto& error : threshold_result.errors) result.add_error(error);
  for (const auto& warning : threshold_result.warnings) result.add_error(warning);

  for (const auto& error : bodies_result.errors) result.add_error(error);
  for (const auto& warning : bodies_result.warnings) result.add_error(warning);

  for (const auto& error : date_result.errors) result.add_error(error);
  for (const auto& warning : date_result.warnings) result.add_error(warning);

  // Cross-parameter validation
  auto cross_param_result = validate_cross_parameters();
  for (const auto& error : cross_param_result.errors) result.add_error(error);
  for (const auto& warning : cross_param_result.warnings) result.add_error(warning);

  // Computational load validation
  auto load_result = validate_computational_load();
  for (const auto& error : load_result.errors) result.add_error(error);
  for (const auto& warning : load_result.warnings) result.add_error(warning);

  // Custom validation if provided
  if (validator_.has_value() && bodies_.has_value()) {
    if (!validator_.value()(*bodies_)) {
      result.add_error(ValidationErrorCode::CUSTOM_VALIDATION_FAILED,
                      ValidationSeverity::ERROR,
                      "Custom validation callback failed",
                      "User-provided validation function returned false");
    }
  }

  // Generate summary
  if (result.is_valid) {
    result.summary = "Configuration is valid and ready for simulation";
  } else {
    result.summary = "Configuration has " + std::to_string(result.errors.size()) +
                    " error(s) and " + std::to_string(result.warnings.size()) + " warning(s)";
  }

  return result;
}

ValidationResult SimulationBuilder::validate_with_level(ValidationSeverity min_severity) const {
  auto full_result = validate_comprehensive();
  ValidationResult filtered_result;

  // Filter results based on minimum severity
  for (const auto& error : full_result.errors) {
    if (error.severity >= min_severity) {
      filtered_result.add_error(error);
    }
  }

  for (const auto& warning : full_result.warnings) {
    if (warning.severity >= min_severity) {
      filtered_result.add_error(warning);
    }
  }

  filtered_result.parameter_status = full_result.parameter_status;
  return filtered_result;
}

ValidationResult SimulationBuilder::detect_conflicts() const {
  return validate_parameter_combinations();
}

ValidationResult SimulationBuilder::validate_physical_constraints() const {
  ValidationResult result;

  // Validate against physical constraints
  auto timestep_result = validate_timestep();
  auto iterations_result = validate_max_iterations();
  auto threshold_result = validate_convergence_threshold();

  // Merge results
  for (const auto& error : timestep_result.errors) result.add_error(error);
  for (const auto& error : iterations_result.errors) result.add_error(error);
  for (const auto& error : threshold_result.errors) result.add_error(error);

  return result;
}

ValidationResult SimulationBuilder::validate_cross_parameters() const {
  return validate_parameter_combinations();
}

std::unique_ptr<Simulation::SimulationEngine> SimulationBuilder::build(std::string* error_message) {
  // Validate configuration
  if (!validate(error_message)) {
    return nullptr;
  }

  // Create simulation engine configuration
  Simulation::SimulationConfig engine_config;
  engine_config.time_step = timestep_;
  engine_config.tolerance = convergence_threshold_;

  // Create simulation engine
  auto engine = std::make_unique<Simulation::SimulationEngine>(engine_config);

  // Initialize with bodies (simplified for now)
  if (bodies_.has_value()) {
    // For now, just create the engine - initialization will be done separately
    LOG_INFO("SimulationBuilder",
             "Built simulation engine with " + std::to_string(bodies_->size()) + " bodies");
  }

  if (verbose_output_) {
    LOG_INFO("SimulationBuilder", get_config_summary());
  }

  return engine;
}

std::optional<Bodies::BodyCollection> SimulationBuilder::build_and_run(std::string* error_message) {
  auto engine = build(error_message);
  if (!engine) {
    return std::nullopt;
  }

  // For now, just return the original bodies
  if (bodies_.has_value()) {
    LOG_INFO("SimulationBuilder", "Simulation completed successfully");
    return *bodies_;
  } else {
    if (error_message) *error_message = "No bodies to simulate";
    return std::nullopt;
  }
}

std::string SimulationBuilder::get_config_summary() const {
  std::ostringstream oss;
  oss << "Simulation Configuration:\n";
  oss << "  Timestep: " << timestep_ << " seconds\n";
  oss << "  Max iterations: " << max_iterations_ << "\n";
  oss << "  Convergence threshold: " << convergence_threshold_ << "\n";

  if (bodies_.has_value()) {
    oss << "  Bodies: " << bodies_->size() << " celestial objects\n";
  } else {
    oss << "  Bodies: Not specified\n";
  }

  if (target_date_.has_value()) {
    oss << "  Target date: " << format_time_point(*target_date_) << "\n";
  } else {
    oss << "  Target date: Not specified\n";
  }

  oss << "  Progress reporting: " << (enable_progress_ ? "enabled" : "disabled") << "\n";
  oss << "  Verbose output: " << (verbose_output_ ? "enabled" : "disabled");

  return oss.str();
}

bool SimulationBuilder::is_valid() const { return validate(); }

std::chrono::seconds SimulationBuilder::get_estimated_duration() const {
  // Simple estimation based on typical performance
  if (bodies_.has_value()) {
    size_t body_count = bodies_->size();
    size_t estimated_iterations =
        std::min(max_iterations_, size_t(86400 / timestep_));  // 1 day max

    // Rough estimate: 1 microsecond per body per iteration
    auto microseconds = body_count * estimated_iterations;
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::microseconds(microseconds));
  }

  return std::chrono::seconds(0);
}

std::string SimulationBuilder::format_time_point(std::chrono::system_clock::time_point tp) const {
  auto time_t = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
  return oss.str();
}

std::chrono::system_clock::time_point SimulationBuilder::parse_iso_date(
    const std::string& iso_date) const {
  // Simple ISO date parsing (YYYY-MM-DD or YYYY-MM-DD HH:MM:SS)
  std::tm tm = {};
  std::istringstream ss(iso_date);

  if (iso_date.find(' ') != std::string::npos) {
    // Date and time
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  } else {
    // Date only
    ss >> std::get_time(&tm, "%Y-%m-%d");
  }

  if (ss.fail()) {
    throw std::runtime_error("Invalid date format: " + iso_date);
  }

  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// === BodySelector Implementation ===

BodySelector::BodySelector() = default;

BodySelector& BodySelector::essential() {
  add_priority_filter(Bodies::BodyPriority::Essential);
  return *this;
}

BodySelector& BodySelector::important() {
  add_priority_filter(Bodies::BodyPriority::Important);
  return *this;
}

BodySelector& BodySelector::optional() {
  add_priority_filter(Bodies::BodyPriority::Optional);
  return *this;
}

BodySelector& BodySelector::all() {
  filters_.clear();  // Remove all filters to include everything
  return *this;
}

BodySelector& BodySelector::named(const std::vector<std::string>& names) {
  add_name_filter(names, true);
  return *this;
}

BodySelector& BodySelector::of_type(Bodies::BodyType type) {
  add_type_filter(type);
  return *this;
}

BodySelector& BodySelector::with_priority(Bodies::BodyPriority priority) {
  add_priority_filter(priority);
  return *this;
}

BodySelector& BodySelector::excluding(const std::vector<std::string>& names) {
  add_name_filter(names, false);
  return *this;
}

BodySelector& BodySelector::where(std::function<bool(const Bodies::CelestialBody&)> predicate) {
  filters_.push_back(std::move(predicate));
  return *this;
}

std::optional<Bodies::BodyCollection> BodySelector::build(std::string* error_message) {
  // Get all available body definitions
  auto all_definitions = Data::get_all_body_definitions();

  // Create body factory
  Bodies::BodyFactory factory;

  // Create body collection
  Bodies::BodyCollection collection;

  for (const auto& definition : all_definitions) {
    // Convert definition to celestial body - use correct API
    Bodies::BodyFactory::CreationOptions options;
    options.preferred_source = Bodies::BodyFactory::DataSource::FALLBACK_DATA;
    auto body_result = factory.create_body(definition.name, options);

    if (body_result.has_value()) {
      auto body = body_result.value();

      // Apply all filters
      bool include = true;
      for (const auto& filter : filters_) {
        if (!filter(body)) {
          include = false;
          break;
        }
      }

      if (include) {
        collection.add_body(std::move(body));
      }
    }
  }

  if (collection.size() == 0) {
    if (error_message) *error_message = "No bodies match the selection criteria";
    return std::nullopt;
  }

  LOG_INFO("BodySelector", "Selected " + std::to_string(collection.size()) + " bodies");

  return collection;
}

size_t BodySelector::count() const {
  auto result = const_cast<BodySelector*>(this)->build();
  return result.has_value() ? result->size() : 0;
}

std::vector<std::string> BodySelector::get_selected_names() const {
  auto result = const_cast<BodySelector*>(this)->build();
  std::vector<std::string> names;

  if (result.has_value()) {
    for (const auto& body : *result) {
      names.emplace_back(body.name());  // Convert string_view to string
    }
  }

  return names;
}

void BodySelector::add_priority_filter(Bodies::BodyPriority priority) {
  filters_.push_back(
      [priority](const Bodies::CelestialBody& body) { return body.priority() == priority; });
}

void BodySelector::add_type_filter(Bodies::BodyType type) {
  filters_.push_back([type](const Bodies::CelestialBody& body) { return body.type() == type; });
}

void BodySelector::add_name_filter(const std::vector<std::string>& names, bool include) {
  filters_.push_back([names, include](const Bodies::CelestialBody& body) {
    bool found = std::find(names.begin(), names.end(), body.name()) != names.end();
    return include ? found : !found;
  });
}

// === Enhanced ConfigurationBuilder Implementation ===

// Static member initialization
std::unordered_map<std::string, ConfigurationTemplate> ConfigurationBuilder::templates_;
std::vector<ConfigurationMigration> ConfigurationBuilder::migrations_;
bool ConfigurationBuilder::templates_initialized_ = false;

ConfigurationBuilder::ConfigurationBuilder() {
  // Use default SimulationConfig values
  if (!templates_initialized_) {
    initialize_templates();
    initialize_migrations();
    templates_initialized_ = true;
  }
}

ConfigurationBuilder::ConfigurationBuilder(const Simulation::SimulationConfig& base)
    : config_(base) {
  if (!templates_initialized_) {
    initialize_templates();
    initialize_migrations();
    templates_initialized_ = true;
  }
}

ConfigurationBuilder::ConfigurationBuilder(const std::string& template_name) {
  if (!templates_initialized_) {
    initialize_templates();
    initialize_migrations();
    templates_initialized_ = true;
  }
  from_template(template_name);
}

ConfigurationBuilder& ConfigurationBuilder::timestep(double seconds) {
  config_.time_step = seconds;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::timestep_minutes(double minutes) {
  config_.time_step = minutes * 60.0;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::timestep_hours(double hours) {
  config_.time_step = hours * 3600.0;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::gravitational_constant(double G) {
  config_.gravitational_constant = G;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::convergence_threshold(double threshold) {
  config_.tolerance = threshold;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::adaptive_timestep(bool enable, double min_step, double max_step) {
  config_.use_adaptive_timestep = enable;
  config_.min_timestep = min_step;
  config_.max_timestep = max_step;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::collision_detection(bool enable, double threshold) {
  config_.enable_collision_detection = enable;
  config_.collision_threshold = threshold;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::high_accuracy() {
  config_.time_step = 60.0;  // 1 minute
  config_.tolerance = 1e-15;
  config_.use_adaptive_timestep = true;
  config_.min_timestep = 1.0;
  config_.max_timestep = 300.0;  // 5 minutes max
  metadata_["preset"] = "high_accuracy";
  metadata_["description"] = "High accuracy preset for research simulations";
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::high_performance() {
  config_.time_step = 7200.0;  // 2 hours
  config_.tolerance = 1e-10;
  config_.use_adaptive_timestep = false;
  config_.enable_collision_detection = false;
  metadata_["preset"] = "high_performance";
  metadata_["description"] = "High performance preset for large-scale simulations";
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::balanced() {
  config_.time_step = 3600.0;  // 1 hour
  config_.tolerance = 1e-12;
  config_.use_adaptive_timestep = true;
  config_.min_timestep = 60.0;
  config_.max_timestep = 7200.0;
  metadata_["preset"] = "balanced";
  metadata_["description"] = "Balanced preset for general use";
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::real_time() {
  config_.time_step = 1.0;  // 1 second
  config_.tolerance = 1e-8;
  config_.use_adaptive_timestep = false;
  config_.enable_collision_detection = true;
  metadata_["preset"] = "real_time";
  metadata_["description"] = "Real-time preset for interactive applications";
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::educational() {
  config_.time_step = 3600.0;  // 1 hour
  config_.tolerance = 1e-10;
  config_.use_adaptive_timestep = false;
  config_.enable_collision_detection = false;
  metadata_["preset"] = "educational";
  metadata_["description"] = "Educational preset for teaching scenarios";
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::research() {
  config_.time_step = 300.0;  // 5 minutes
  config_.tolerance = 1e-14;
  config_.use_adaptive_timestep = true;
  config_.min_timestep = 1.0;
  config_.max_timestep = 1800.0;  // 30 minutes max
  metadata_["preset"] = "research";
  metadata_["description"] = "Research preset for scientific studies";
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::visualization() {
  config_.time_step = 60.0;  // 1 minute
  config_.tolerance = 1e-10;
  config_.use_adaptive_timestep = false;
  config_.enable_collision_detection = true;
  config_.collision_threshold = 1e5;  // Closer collision detection for visualization
  metadata_["preset"] = "visualization";
  metadata_["description"] = "Visualization preset for rendering applications";
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::from_template(const std::string& template_name) {
  auto it = templates_.find(template_name);
  if (it != templates_.end()) {
    config_ = it->second.config;
    metadata_ = it->second.metadata;
  }
  return *this;
}

bool ConfigurationBuilder::save_as_template(const std::string& template_name, const std::string& description) {
  ConfigurationTemplate new_template(template_name, description, config_);
  new_template.metadata = metadata_;
  templates_[template_name] = new_template;
  return true;  // In a real implementation, this would save to persistent storage
}

std::vector<std::string> ConfigurationBuilder::get_available_templates() {
  if (!templates_initialized_) {
    // Force initialization of static members
    ConfigurationBuilder temp;
  }

  std::vector<std::string> names;
  for (const auto& pair : templates_) {
    names.push_back(pair.first);
  }
  return names;
}

std::optional<ConfigurationTemplate> ConfigurationBuilder::get_template(const std::string& name) {
  if (!templates_initialized_) {
    // Force initialization of static members
    ConfigurationBuilder temp;
  }

  auto it = templates_.find(name);
  if (it != templates_.end()) {
    return it->second;
  }
  return std::nullopt;
}

ValidationResult ConfigurationBuilder::validate_comprehensive() const {
  ValidationResult result;

  // Validate physics parameters
  auto physics_result = validate_physics_parameters();
  for (const auto& error : physics_result.errors) result.add_error(error);
  for (const auto& warning : physics_result.warnings) result.add_error(warning);

  // Validate adaptive timestep settings
  if (config_.use_adaptive_timestep) {
    auto adaptive_result = validate_adaptive_timestep();
    for (const auto& error : adaptive_result.errors) result.add_error(error);
    for (const auto& warning : adaptive_result.warnings) result.add_error(warning);
  }

  // Validate collision detection settings
  if (config_.enable_collision_detection) {
    auto collision_result = validate_collision_detection();
    for (const auto& error : collision_result.errors) result.add_error(error);
    for (const auto& warning : collision_result.warnings) result.add_error(warning);
  }

  // Generate summary
  if (result.is_valid) {
    result.summary = "Configuration is valid and ready for use";
  } else {
    result.summary = "Configuration has " + std::to_string(result.errors.size()) +
                    " error(s) and " + std::to_string(result.warnings.size()) + " warning(s)";
  }

  return result;
}

std::vector<ConfigurationConflict> ConfigurationBuilder::detect_conflicts() const {
  std::vector<ConfigurationConflict> conflicts;

  // Check timestep conflicts
  auto timestep_conflicts = check_timestep_conflicts();
  conflicts.insert(conflicts.end(), timestep_conflicts.begin(), timestep_conflicts.end());

  // Check adaptive timestep conflicts
  if (config_.use_adaptive_timestep) {
    auto adaptive_conflicts = check_adaptive_conflicts();
    conflicts.insert(conflicts.end(), adaptive_conflicts.begin(), adaptive_conflicts.end());
  }

  // Check performance conflicts
  auto performance_conflicts = check_performance_conflicts();
  conflicts.insert(conflicts.end(), performance_conflicts.begin(), performance_conflicts.end());

  return conflicts;
}

ConfigurationBuilder& ConfigurationBuilder::resolve_conflicts_automatically() {
  auto conflicts = detect_conflicts();

  for (const auto& conflict : conflicts) {
    if (!conflict.recommended_resolution.empty()) {
      // Apply recommended resolution
      if (conflict.parameter1 == "timestep" && conflict.parameter2 == "adaptive_timestep") {
        if (conflict.recommended_resolution == "disable_adaptive") {
          config_.use_adaptive_timestep = false;
        } else if (conflict.recommended_resolution == "adjust_timestep") {
          config_.time_step = std::max(config_.min_timestep, std::min(config_.max_timestep, config_.time_step));
        }
      }
      // Add more conflict resolution logic as needed
    }
  }

  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::resolve_conflict(const std::string& parameter1,
                                                            const std::string& parameter2,
                                                            const std::string& resolution) {
  // Apply specific conflict resolution
  if (parameter1 == "timestep" && parameter2 == "adaptive_timestep") {
    if (resolution == "disable_adaptive") {
      config_.use_adaptive_timestep = false;
    } else if (resolution == "adjust_timestep") {
      config_.time_step = std::max(config_.min_timestep, std::min(config_.max_timestep, config_.time_step));
    }
  }
  // Add more specific resolution logic as needed

  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::migrate_from_version(int version) {
  for (const auto& migration : migrations_) {
    if (migration.from_version == version && migration.to_version > version) {
      config_ = migration.migrate_function(config_);
      config_version_ = migration.to_version;
      break;
    }
  }
  return *this;
}

bool ConfigurationBuilder::needs_migration() const {
  return config_version_ < CURRENT_CONFIG_VERSION;
}

int ConfigurationBuilder::get_version() const {
  return config_version_;
}

std::string ConfigurationBuilder::to_json() const {
  // Simple JSON serialization (in a real implementation, use a JSON library)
  std::ostringstream json;
  json << "{\n";
  json << "  \"version\": " << config_version_ << ",\n";
  json << "  \"time_step\": " << config_.time_step << ",\n";
  json << "  \"gravitational_constant\": " << config_.gravitational_constant << ",\n";
  json << "  \"tolerance\": " << config_.tolerance << ",\n";
  json << "  \"use_adaptive_timestep\": " << (config_.use_adaptive_timestep ? "true" : "false") << ",\n";
  json << "  \"min_timestep\": " << config_.min_timestep << ",\n";
  json << "  \"max_timestep\": " << config_.max_timestep << ",\n";
  json << "  \"enable_collision_detection\": " << (config_.enable_collision_detection ? "true" : "false") << ",\n";
  json << "  \"collision_threshold\": " << config_.collision_threshold << "\n";
  json << "}";
  return json.str();
}

ConfigurationBuilder& ConfigurationBuilder::from_json(const std::string& json_str) {
  // Simple JSON parsing (in a real implementation, use a JSON library)
  // This is a basic implementation for demonstration

  // Extract version
  std::regex version_regex(R"("version":\s*(\d+))");
  std::smatch match;
  if (std::regex_search(json_str, match, version_regex)) {
    config_version_ = std::stoi(match[1]);
  }

  // Extract time_step
  std::regex timestep_regex(R"("time_step":\s*([\d.e+-]+))");
  if (std::regex_search(json_str, match, timestep_regex)) {
    config_.time_step = std::stod(match[1]);
  }

  // Extract tolerance
  std::regex tolerance_regex(R"("tolerance":\s*([\d.e+-]+))");
  if (std::regex_search(json_str, match, tolerance_regex)) {
    config_.tolerance = std::stod(match[1]);
  }

  // Extract adaptive timestep
  std::regex adaptive_regex(R"("use_adaptive_timestep":\s*(true|false))");
  if (std::regex_search(json_str, match, adaptive_regex)) {
    config_.use_adaptive_timestep = (match[1] == "true");
  }

  return *this;
}

bool ConfigurationBuilder::save_to_file(const std::string& filename) const {
  std::ofstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  file << to_json();
  return true;
}

ConfigurationBuilder& ConfigurationBuilder::load_from_file(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return *this;
  }

  std::string json_content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
  from_json(json_content);

  return *this;
}

Simulation::SimulationConfig ConfigurationBuilder::build() const {
  return config_;
}

bool ConfigurationBuilder::validate(std::string* error_message) const {
  auto result = validate_comprehensive();
  if (error_message && !result.is_valid && !result.errors.empty()) {
    *error_message = result.errors[0].message;
  }
  return result.is_valid;
}

std::string ConfigurationBuilder::get_summary() const {
  std::ostringstream summary;
  summary << "Configuration Summary:\n";
  summary << "  Version: " << config_version_ << "\n";
  summary << "  Timestep: " << config_.time_step << " seconds\n";
  summary << "  Tolerance: " << config_.tolerance << "\n";
  summary << "  Gravitational constant: " << config_.gravitational_constant << " m³/kg/s²\n";
  summary << "  Adaptive timestep: " << (config_.use_adaptive_timestep ? "enabled" : "disabled") << "\n";

  if (config_.use_adaptive_timestep) {
    summary << "    Min timestep: " << config_.min_timestep << " seconds\n";
    summary << "    Max timestep: " << config_.max_timestep << " seconds\n";
  }

  summary << "  Collision detection: " << (config_.enable_collision_detection ? "enabled" : "disabled") << "\n";

  if (config_.enable_collision_detection) {
    summary << "    Collision threshold: " << config_.collision_threshold << " meters\n";
  }

  auto preset_it = metadata_.find("preset");
  if (preset_it != metadata_.end()) {
    summary << "  Preset: " << preset_it->second << "\n";
  }

  return summary.str();
}

std::vector<std::string> ConfigurationBuilder::compare_with(const ConfigurationBuilder& other) const {
  std::vector<std::string> differences;

  if (config_.time_step != other.config_.time_step) {
    differences.push_back("Timestep: " + std::to_string(config_.time_step) +
                         " vs " + std::to_string(other.config_.time_step));
  }

  if (config_.tolerance != other.config_.tolerance) {
    differences.push_back("Tolerance: " + std::to_string(config_.tolerance) +
                         " vs " + std::to_string(other.config_.tolerance));
  }

  if (config_.use_adaptive_timestep != other.config_.use_adaptive_timestep) {
    differences.push_back("Adaptive timestep: " +
                         std::string(config_.use_adaptive_timestep ? "enabled" : "disabled") +
                         " vs " + std::string(other.config_.use_adaptive_timestep ? "enabled" : "disabled"));
  }

  if (config_.enable_collision_detection != other.config_.enable_collision_detection) {
    differences.push_back("Collision detection: " +
                         std::string(config_.enable_collision_detection ? "enabled" : "disabled") +
                         " vs " + std::string(other.config_.enable_collision_detection ? "enabled" : "disabled"));
  }

  return differences;
}

// === Private Helper Methods ===

void ConfigurationBuilder::initialize_templates() {
  // Initialize built-in templates

  // High Accuracy Research Template
  Simulation::SimulationConfig research_config;
  research_config.time_step = 300.0;  // 5 minutes
  research_config.tolerance = 1e-14;
  research_config.use_adaptive_timestep = true;
  research_config.min_timestep = 1.0;
  research_config.max_timestep = 1800.0;

  ConfigurationTemplate research_template("research", "High accuracy configuration for scientific research", research_config);
  research_template.recommended_bodies = {"Sun", "Earth", "Moon", "Mars", "Jupiter"};
  research_template.metadata["category"] = "scientific";
  research_template.metadata["accuracy"] = "high";
  templates_["research"] = research_template;

  // Educational Template
  Simulation::SimulationConfig edu_config;
  edu_config.time_step = 3600.0;  // 1 hour
  edu_config.tolerance = 1e-10;
  edu_config.use_adaptive_timestep = false;

  ConfigurationTemplate edu_template("educational", "Simple configuration for educational purposes", edu_config);
  edu_template.recommended_bodies = {"Sun", "Earth", "Moon"};
  edu_template.metadata["category"] = "educational";
  edu_template.metadata["complexity"] = "low";
  templates_["educational"] = edu_template;

  // Visualization Template
  Simulation::SimulationConfig viz_config;
  viz_config.time_step = 60.0;  // 1 minute
  viz_config.tolerance = 1e-10;
  viz_config.enable_collision_detection = true;
  viz_config.collision_threshold = 1e5;

  ConfigurationTemplate viz_template("visualization", "Optimized for real-time visualization", viz_config);
  viz_template.recommended_bodies = {"Sun", "Mercury", "Venus", "Earth", "Mars"};
  viz_template.metadata["category"] = "visualization";
  viz_template.metadata["real_time"] = "true";
  templates_["visualization"] = viz_template;
}

void ConfigurationBuilder::initialize_migrations() {
  // Migration from version 1 to 2
  migrations_.emplace_back(1, 2, "Add adaptive timestep and collision detection support",
    [](const Simulation::SimulationConfig& old_config) {
      Simulation::SimulationConfig new_config = old_config;
      // Set default values for new fields
      new_config.use_adaptive_timestep = false;
      new_config.min_timestep = 1.0;
      new_config.max_timestep = 3600.0;
      new_config.enable_collision_detection = false;
      new_config.collision_threshold = 1e6;
      return new_config;
    });
}

ValidationResult ConfigurationBuilder::validate_physics_parameters() const {
  ValidationResult result;

  // Validate timestep
  if (config_.time_step <= 0) {
    result.add_error(ValidationErrorCode::INVALID_TIMESTEP,
                    ValidationSeverity::FATAL,
                    "Timestep must be positive",
                    "Current value: " + std::to_string(config_.time_step));
  }

  // Validate tolerance
  if (config_.tolerance <= 0) {
    result.add_error(ValidationErrorCode::INVALID_CONVERGENCE_THRESHOLD,
                    ValidationSeverity::FATAL,
                    "Tolerance must be positive",
                    "Current value: " + std::to_string(config_.tolerance));
  }

  // Validate gravitational constant
  if (config_.gravitational_constant <= 0) {
    result.add_error(ValidationErrorCode::INVALID_GRAVITATIONAL_CONSTANT,
                    ValidationSeverity::ERROR,
                    "Gravitational constant must be positive",
                    "Current value: " + std::to_string(config_.gravitational_constant));
  }

  // Check if gravitational constant is reasonable
  constexpr double STANDARD_G = 6.67430e-11;
  if (std::abs(config_.gravitational_constant - STANDARD_G) / STANDARD_G > 0.1) {
    result.add_error(ValidationErrorCode::INVALID_GRAVITATIONAL_CONSTANT,
                    ValidationSeverity::WARNING,
                    "Gravitational constant differs significantly from standard value",
                    "Current: " + std::to_string(config_.gravitational_constant) +
                    ", standard: " + std::to_string(STANDARD_G));
  }

  return result;
}

ValidationResult ConfigurationBuilder::validate_adaptive_timestep() const {
  ValidationResult result;

  if (config_.min_timestep <= 0) {
    result.add_error(ValidationErrorCode::TIMESTEP_TOO_SMALL,
                    ValidationSeverity::ERROR,
                    "Minimum timestep must be positive",
                    "Current value: " + std::to_string(config_.min_timestep));
  }

  if (config_.max_timestep <= config_.min_timestep) {
    result.add_error(ValidationErrorCode::CONFLICTING_PARAMETERS,
                    ValidationSeverity::ERROR,
                    "Maximum timestep must be greater than minimum timestep",
                    "Min: " + std::to_string(config_.min_timestep) +
                    ", Max: " + std::to_string(config_.max_timestep));
  }

  if (config_.time_step < config_.min_timestep || config_.time_step > config_.max_timestep) {
    result.add_error(ValidationErrorCode::CONFLICTING_PARAMETERS,
                    ValidationSeverity::WARNING,
                    "Initial timestep is outside adaptive timestep range",
                    "Timestep: " + std::to_string(config_.time_step) +
                    ", Range: [" + std::to_string(config_.min_timestep) +
                    ", " + std::to_string(config_.max_timestep) + "]");
  }

  return result;
}

ValidationResult ConfigurationBuilder::validate_collision_detection() const {
  ValidationResult result;

  if (config_.collision_threshold <= 0) {
    result.add_error(ValidationErrorCode::CONFLICTING_PARAMETERS,
                    ValidationSeverity::ERROR,
                    "Collision threshold must be positive",
                    "Current value: " + std::to_string(config_.collision_threshold));
  }

  // Warn if collision threshold is very large (might miss collisions)
  if (config_.collision_threshold > 1e8) {  // 100,000 km
    result.add_error(ValidationErrorCode::CONFLICTING_PARAMETERS,
                    ValidationSeverity::WARNING,
                    "Collision threshold is very large, may miss close approaches",
                    "Current value: " + std::to_string(config_.collision_threshold) + " meters");
  }

  return result;
}

std::vector<ConfigurationConflict> ConfigurationBuilder::check_timestep_conflicts() const {
  std::vector<ConfigurationConflict> conflicts;

  // Check if timestep is too large for collision detection
  if (config_.enable_collision_detection && config_.time_step > 3600.0) {
    ConfigurationConflict conflict("timestep", "collision_detection",
                                  "Large timestep may miss collision events");
    conflict.resolution_options = {"Reduce timestep to <= 3600 seconds", "Disable collision detection"};
    conflict.recommended_resolution = "reduce_timestep";
    conflicts.push_back(conflict);
  }

  return conflicts;
}

std::vector<ConfigurationConflict> ConfigurationBuilder::check_adaptive_conflicts() const {
  std::vector<ConfigurationConflict> conflicts;

  // Check if adaptive timestep range is too narrow
  if (config_.max_timestep / config_.min_timestep < 2.0) {
    ConfigurationConflict conflict("min_timestep", "max_timestep",
                                  "Adaptive timestep range is too narrow to be effective");
    conflict.resolution_options = {"Increase max_timestep", "Decrease min_timestep", "Disable adaptive timestep"};
    conflict.recommended_resolution = "increase_range";
    conflicts.push_back(conflict);
  }

  return conflicts;
}

std::vector<ConfigurationConflict> ConfigurationBuilder::check_performance_conflicts() const {
  std::vector<ConfigurationConflict> conflicts;

  // Check if high accuracy settings might impact performance
  if (config_.tolerance < 1e-13 && config_.time_step < 60.0) {
    ConfigurationConflict conflict("tolerance", "timestep",
                                  "Very high accuracy settings may significantly impact performance");
    conflict.resolution_options = {"Increase tolerance to 1e-12", "Increase timestep to >= 60 seconds", "Keep current settings"};
    conflict.recommended_resolution = "balance_accuracy_performance";
    conflicts.push_back(conflict);
  }

  return conflicts;
}

// === Enhanced Validation Implementation ===

ValidationResult SimulationBuilder::validate_timestep() const {
  ValidationResult result;

  // Basic validation
  if (timestep_ <= 0) {
    result.add_error(ValidationErrorCode::INVALID_TIMESTEP,
                    ValidationSeverity::FATAL,
                    "Timestep must be positive",
                    "Current value: " + std::to_string(timestep_));
    return result;
  }

  // Physical constrais validation
  if (timestep_ < PhysicalConstraints::MIN_TIMESTEP) {
    result.add_error(ValidationErrorCode::TIMESTEP_TOO_SMALL,
                    ValidationSeverity::ERROR,
                    "Timestep is too small for stable simulation",
                    "Current: " + std::to_string(timestep_) + "s, minimum: " +
                    std::to_string(PhysicalConstraints::MIN_TIMESTEP) + "s");

    ValidationError& error = result.errors.back();
    error.suggestions.push_back("Use timestep >= " + std::to_string(PhysicalConstraints::RECOMMENDED_MIN_TIMESTEP) + " seconds");
    error.suggestions.push_back("Consider using adaptive timestep for very small values");
    error.recovery_action = "Set timestep to " + std::to_string(PhysicalConstraints::RECOMMENDED_MIN_TIMESTEP) + " seconds";
  }

  if (timestep_ > PhysicalConstraints::MAX_TIMESTEP) {
    result.add_error(ValidationErrorCode::TIMESTEP_TOO_LARGE,
                    ValidationSeverity::ERROR,
                    "Timestep is too large for accurate simulation",
                    "Current: " + std::to_string(timestep_) + "s, maximum: " +
                    std::to_string(PhysicalConstraints::MAX_TIMESTEP) + "s");

    ValidationError& error = result.errors.back();
    error.suggestions.push_back("Use timestep <= " + std::to_string(PhysicalConstraints::RECOMMENDED_MAX_TIMESTEP) + " seconds");
    error.suggestions.push_back("Break simulation into smaller time segments");
    error.recovery_action = "Set timestep to " + std::to_string(PhysicalConstraints::RECOMMENDED_MAX_TIMESTEP) + " seconds";
  }

  // Performance warnings
  if (timestep_ < PhysicalConstraints::RECOMMENDED_MIN_TIMESTEP) {
    result.add_error(ValidationErrorCode::TIMESTEP_TOO_SMALL,
                    ValidationSeverity::WARNING,
                    "Small timestep may impact performance",
                    "Current: " + std::to_string(timestep_) + "s, recommended minimum: " +
                    std::to_string(PhysicalConstraints::RECOMMENDED_MIN_TIMESTEP) + "s");
  }

  if (timestep_ > PhysicalConstraints::RECOMMENDED_MAX_TIMESTEP) {
    result.add_error(ValidationErrorCode::TIMESTEP_TOO_LARGE,
                    ValidationSeverity::WARNING,
                    "Large timestep may reduce accuracy",
                    "Current: " + std::to_string(timestep_) + "s, recommended maximum: " +
                    std::to_string(PhysicalConstraints::RECOMMENDED_MAX_TIMESTEP) + "s");
  }

  result.parameter_status["timestep"] = "valid";
  return result;
}

ValidationResult SimulationBuilder::validate_max_iterations() const {
  ValidationResult result;

  if (max_iterations_ < PhysicalConstraints::MIN_ITERATIONS) {
    result.add_error(ValidationErrorCode::INVALID_MAX_ITERATIONS,
                    ValidationSeverity::ERROR,
                    "Maximum iterations must be at least " + std::to_string(PhysicalConstraints::MIN_ITERATIONS),
                    "Current value: " + std::to_string(max_iterations_));

    ValidationError& error = result.errors.back();
    error.suggestions.push_back("Set max_iterations to at least " + std::to_string(PhysicalConstraints::MIN_ITERATIONS));
    error.recovery_action = "Set max_iterations to " + std::to_string(PhysicalConstraints::MIN_ITERATIONS);
  }

  if (max_iterations_ > PhysicalConstraints::MAX_ITERATIONS) {
    result.add_error(ValidationErrorCode::INVALID_MAX_ITERATIONS,
                    ValidationSeverity::ERROR,
                    "Maximum iterations exceeds reasonable limit",
                    "Current: " + std::to_string(max_iterations_) + ", maximum: " +
                    std::to_string(PhysicalConstraints::MAX_ITERATIONS));

    ValidationError& error = result.errors.back();
    error.suggestions.push_back("Reduce max_iterations to <= " + std::to_string(PhysicalConstraints::RECOMMENDED_MAX_ITERATIONS));
    error.suggestions.push_back("Consider breaking simulation into multiple runs");
    error.recovery_action = "Set max_iterations to " + std::to_string(PhysicalConstraints::RECOMMENDED_MAX_ITERATIONS);
  }

  // Performance warning
  if (max_iterations_ > PhysicalConstraints::RECOMMENDED_MAX_ITERATIONS) {
    result.add_error(ValidationErrorCode::EXCESSIVE_COMPUTATIONAL_LOAD,
                    ValidationSeverity::WARNING,
                    "High iteration count may impact performance",
                    "Current: " + std::to_string(max_iterations_) + ", recommended maximum: " +
                    std::to_string(PhysicalConstraints::RECOMMENDED_MAX_ITERATIONS));
  }

  result.parameter_status["max_iterations"] = "valid";
  return result;
}

ValidationResult SimulationBuilder::validate_convergence_threshold() const {
  ValidationResult result;

  if (convergence_threshold_ <= 0) {
    result.add_error(ValidationErrorCode::INVALID_CONVERGENCE_THRESHOLD,
                    ValidationSeverity::FATAL,
                    "Convergence threshold must be positive",
                    "Current value: " + std::to_string(convergence_threshold_));
    return result;
  }

  if (convergence_threshold_ < PhysicalConstraints::MIN_CONVERGENCE_THRESHOLD) {
    result.add_error(ValidationErrorCode::INVALID_CONVERGENCE_THRESHOLD,
                    ValidationSeverity::WARNING,
                    "Convergence threshold may be too strict",
                    "Current: " + std::to_string(convergence_threshold_) + ", minimum: " +
                    std::to_string(PhysicalConstraints::MIN_CONVERGENCE_THRESHOLD));

    ValidationError& error = result.warnings.back();
    error.suggestions.push_back("Consider using threshold >= " + std::to_string(PhysicalConstraints::RECOMMENDED_CONVERGENCE_THRESHOLD));
    error.suggestions.push_back("Very strict thresholds may prevent convergence");
  }

  if (convergence_threshold_ > PhysicalConstraints::MAX_CONVERGENCE_THRESHOLD) {
    result.add_error(ValidationErrorCode::INVALID_CONVERGENCE_THRESHOLD,
                    ValidationSeverity::WARNING,
                    "Convergence threshold may be too loose",
                    "Current: " + std::to_string(convergence_threshold_) + ", maximum: " +
                    std::to_string(PhysicalConstraints::MAX_CONVERGENCE_THRESHOLD));

    ValidationError& error = result.warnings.back();
    error.suggestions.push_back("Consider using threshold <= " + std::to_string(PhysicalConstraints::RECOMMENDED_CONVERGENCE_THRESHOLD));
    error.suggestions.push_back("Loose thresholds may reduce simulation accuracy");
  }

  result.parameter_status["convergence_threshold"] = "valid";
  return result;
}

ValidationResult SimulationBuilder::validate_bodies() const {
  ValidationResult result;

  if (!bodies_.has_value()) {
    result.add_error(ValidationErrorCode::NO_BODIES_SPECIFIED,
                    ValidationSeverity::ERROR,
                    "No celestial bodies specified for simulation",
                    "Bodies collection is not set");

    ValidationError& error = result.errors.back();
    error.suggestions.push_back("Use with_bodies() to specify celestial bodies");
    error.suggestions.push_back("Use BodySelector to create a body collection");
    error.recovery_action = "Add bodies using with_bodies(BodySelector().essential().build())";
    return result;
  }

  if (bodies_->size() == 0) {
    result.add_error(ValidationErrorCode::EMPTY_BODY_COLLECTION,
                    ValidationSeverity::ERROR,
                    "Body collection is empty",
                    "No bodies in the collection");

    ValidationError& error = result.errors.back();
    error.suggestions.push_back("Ensure body collection contains at least " +
                               std::to_string(PhysicalConstraints::MIN_BODIES_FOR_SIMULATION) + " bodies");
    error.recovery_action = "Add bodies to the collection";
    return result;
  }

  if (bodies_->size() < PhysicalConstraints::MIN_BODIES_FOR_SIMULATION) {
    result.add_error(ValidationErrorCode::INSUFFICIENT_BODIES_FOR_SIMULATION,
                    ValidationSeverity::ERROR,
                    "Insufficient bodies for meaningful simulation",
                    "Current: " + std::to_string(bodies_->size()) + " bodies, minimum: " +
                    std::to_string(PhysicalConstraints::MIN_BODIES_FOR_SIMULATION));

    ValidationError& error = result.errors.back();
    error.suggestions.push_back("Add more bodies to reach minimum of " +
                               std::to_string(PhysicalConstraints::MIN_BODIES_FOR_SIMULATION));
    error.suggestions.push_back("Use BodySelector to add essential bodies");
  }

  if (bodies_->size() > PhysicalConstraints::MAX_BODIES_FOR_PERFORMANCE) {
    result.add_error(ValidationErrorCode::EXCESSIVE_COMPUTATIONAL_LOAD,
                    ValidationSeverity::WARNING,
                    "Large number of bodies may impact performance",
                    "Current: " + std::to_string(bodies_->size()) + " bodies, recommended maximum: " +
                    std::to_string(PhysicalConstraints::RECOMMENDED_MAX_BODIES));

    ValidationError& error = result.warnings.back();
    error.suggestions.push_back("Consider reducing body count for better performance");
    error.suggestions.push_back("Use body filtering to focus on essential objects");
  }

  result.parameter_status["bodies"] = "valid";
  return result;
}

ValidationResult SimulationBuilder::validate_target_date() const {
  ValidationResult result;

  if (target_date_.has_value()) {
    auto time_t_value = std::chrono::system_clock::to_time_t(*target_date_);
    auto tm_value = *std::gmtime(&time_t_value);

    if (tm_value.tm_year + 1900 < PhysicalConstraints::MIN_YEAR) {
      result.add_error(ValidationErrorCode::DATE_OUT_OF_RANGE,
                      ValidationSeverity::ERROR,
                      "Target date is too far in the past",
                      "Year: " + std::to_string(tm_value.tm_year + 1900) +
                      ", minimum: " + std::to_string(PhysicalConstraints::MIN_YEAR));

      ValidationError& error = result.errors.back();
      error.suggestions.push_back("Use date >= " + std::to_string(PhysicalConstraints::MIN_YEAR));
      error.suggestions.push_back("Historical data may be less accurate for very old dates");
    }

    if (tm_value.tm_year + 1900 > PhysicalConstraints::MAX_YEAR) {
      result.add_error(ValidationErrorCode::DATE_OUT_OF_RANGE,
                      ValidationSeverity::ERROR,
                      "Target date is too far in the future",
                      "Year: " + std::to_string(tm_value.tm_year + 1900) +
                      ", maximum: " + std::to_string(PhysicalConstraints::MAX_YEAR));

      ValidationError& error = result.errors.back();
      error.suggestions.push_back("Use date <= " + std::to_string(PhysicalConstraints::MAX_YEAR));
      error.suggestions.push_back("Future predictions become less accurate over time");
    }
  }

  result.parameter_status["target_date"] = target_date_.has_value() ? "valid" : "not_set";
  return result;
}

ValidationResult SimulationBuilder::validate_computational_load() const {
  ValidationResult result;

  double complexity = estimate_computational_complexity();

  // Define complexity thresholds
  constexpr double HIGH_COMPLEXITY_THRESHOLD = 1e9;
  constexpr double EXTREME_COMPLEXITY_THRESHOLD = 1e12;

  if (complexity > EXTREME_COMPLEXITY_THRESHOLD) {
    result.add_error(ValidationErrorCode::EXCESSIVE_COMPUTATIONAL_LOAD,
                    ValidationSeverity::ERROR,
                    "Computational load is extremely high",
                    "Estimated complexity: " + std::to_string(complexity));

    ValidationError& error = result.errors.back();
    error.suggestions = suggest_performance_improvements();
    error.recovery_action = "Reduce timestep resolution or body count";
  } else if (complexity > HIGH_COMPLEXITY_THRESHOLD) {
    result.add_error(ValidationErrorCode::EXCESSIVE_COMPUTATIONAL_LOAD,
                    ValidationSeverity::WARNING,
                    "Computational load is high",
                    "Estimated complexity: " + std::to_string(complexity));

    ValidationError& error = result.warnings.back();
    error.suggestions = suggest_performance_improvements();
  }

  return result;
}

ValidationResult SimulationBuilder::validate_parameter_combinations() const {
  ValidationResult result;

  // Check timestep vs body count interaction
  if (bodies_.has_value() && !is_timestep_appropriate_for_bodies()) {
    result.add_error(ValidationErrorCode::CONFLICTING_PARAMETERS,
                    ValidationSeverity::WARNING,
                    "Timestep may not be appropriate for body configuration",
                    "Bodies: " + std::to_string(bodies_->size()) +
                    ", timestep: " + std::to_string(timestep_) + "s");

    ValidationError& error = result.warnings.back();
    error.suggestions = suggest_timestep_improvements();
  }

  // Check iterations vs timestep interaction
  double total_simulation_time = max_iterations_ * timestep_;
  constexpr double MAX_REASONABLE_SIMULATION_TIME = 365.25 * 24 * 3600 * 100; // 100 years

  if (total_simulation_time > MAX_REASONABLE_SIMULATION_TIME) {
    result.add_error(ValidationErrorCode::CONFLICTING_PARAMETERS,
                    ValidationSeverity::WARNING,
                    "Total simulation time is very long",
                    "Total time: " + std::to_string(total_simulation_time / (365.25 * 24 * 3600)) + " years");

    ValidationError& error = result.warnings.back();
    error.suggestions.push_back("Reduce max_iterations or increase timestep");
    error.suggestions.push_back("Consider breaking into multiple simulation runs");
  }

  return result;
}

// === Utility Methods ===

bool SimulationBuilder::is_timestep_appropriate_for_bodies() const {
  if (!bodies_.has_value()) return true;

  size_t body_count = bodies_->size();

  // For many bodies, smaller timesteps are generally better
  if (body_count > 50 && timestep_ > 1800.0) { // 30 minutes
    return false;
  }

  // For few bodies, very small timesteps may be overkill
  if (body_count < 10 && timestep_ < 60.0) { // 1 minute
    return false;
  }

  return true;
}

double SimulationBuilder::estimate_computational_complexity() const {
  double complexity = 1.0;

  if (bodies_.has_value()) {
    size_t n = bodies_->size();
    complexity *= n * n; // N-body problem is O(N²)
  }

  complexity *= max_iterations_;
  complexity /= timestep_; // Smaller timesteps = more computation

  return complexity;
}

std::vector<std::string> SimulationBuilder::suggest_timestep_improvements() const {
  std::vector<std::string> suggestions;

  if (bodies_.has_value()) {
    size_t body_count = bodies_->size();

    if (body_count > 50) {
      suggestions.push_back("For " + std::to_string(body_count) + " bodies, consider timestep 300-1800 seconds");
    } else if (body_count > 20) {
      suggestions.push_back("For " + std::to_string(body_count) + " bodies, consider timestep 600-3600 seconds");
    } else {
      suggestions.push_back("For " + std::to_string(body_count) + " bodies, consider timestep 1800-7200 seconds");
    }
  }

  suggestions.push_back("Use adaptive timestep for optimal performance");
  suggestions.push_back("Test different timesteps to find the best balance");

  return suggestions;
}

std::vector<std::string> SimulationBuilder::suggest_performance_improvements() const {
  std::vector<std::string> suggestions;

  suggestions.push_back("Increase timestep to reduce computational load");
  suggestions.push_back("Reduce number of bodies in simulation");
  suggestions.push_back("Decrease max_iterations");
  suggestions.push_back("Use body filtering to focus on essential objects");
  suggestions.push_back("Consider parallel processing for large simulations");

  return suggestions;
}

std::string SimulationBuilder::get_validation_context() const {
  std::ostringstream oss;
  oss << "SimulationBuilder validation context:\n";
  oss << "  Timestep: " << timestep_ << "s\n";
  oss << "  Max iterations: " << max_iterations_ << "\n";
  oss << "  Bodies: " << (bodies_.has_value() ? std::to_string(bodies_->size()) : "none") << "\n";
  oss << "  Convergence threshold: " << convergence_threshold_ << "\n";
  oss << "  Target date: " << (target_date_.has_value() ? "set" : "not set");
  return oss.str();
}

}  // namespace SolarSystem::Core::Builders
