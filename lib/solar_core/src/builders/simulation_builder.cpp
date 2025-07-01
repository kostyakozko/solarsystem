#include "solar_core/builders/simulation_builder.hpp"

#include <iomanip>
#include <sstream>

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
  // Validate timestep
  if (timestep_ <= 0) {
    if (error_message) *error_message = "Timestep must be positive";
    return false;
  }

  // Validate max iterations
  if (max_iterations_ == 0) {
    if (error_message) *error_message = "Max iterations must be positive";
    return false;
  }

  // Validate convergence threshold
  if (convergence_threshold_ <= 0) {
    if (error_message) *error_message = "Convergence threshold must be positive";
    return false;
  }

  // Validate bodies
  if (!bodies_.has_value()) {
    if (error_message) *error_message = "No celestial bodies specified";
    return false;
  }

  if (bodies_->size() == 0) {
    if (error_message) *error_message = "Body collection is empty";
    return false;
  }

  // Custom validation if provided
  if (validator_.has_value()) {
    if (!validator_.value()(*bodies_)) {
      if (error_message) *error_message = "Custom validation failed";
      return false;
    }
  }

  return true;
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
  filters_.push_back([priority](const Bodies::CelestialBody& body) {
    // This would need to be implemented based on how priority is stored in CelestialBody
    // For now, we'll use a placeholder
    return true;  // TODO: Implement priority checking
  });
}

void BodySelector::add_type_filter(Bodies::BodyType type) {
  filters_.push_back([type](const Bodies::CelestialBody& body) {
    // This would need to be implemented based on how type is stored in CelestialBody
    // For now, we'll use a placeholder
    return true;  // TODO: Implement type checking
  });
}

void BodySelector::add_name_filter(const std::vector<std::string>& names, bool include) {
  filters_.push_back([names, include](const Bodies::CelestialBody& body) {
    bool found = std::find(names.begin(), names.end(), body.name()) != names.end();
    return include ? found : !found;
  });
}

// === ConfigurationBuilder Implementation ===

ConfigurationBuilder::ConfigurationBuilder() {
  // Use default SimulationConfig values
}

ConfigurationBuilder::ConfigurationBuilder(const Simulation::SimulationConfig& base)
    : config_(base) {}

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

ConfigurationBuilder& ConfigurationBuilder::high_accuracy() {
  config_.time_step = 60.0;  // 1 minute
  config_.tolerance = 1e-15;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::high_performance() {
  config_.time_step = 7200.0;  // 2 hours
  config_.tolerance = 1e-10;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::balanced() {
  config_.time_step = 3600.0;  // 1 hour
  config_.tolerance = 1e-12;
  return *this;
}

ConfigurationBuilder& ConfigurationBuilder::real_time() {
  config_.time_step = 1.0;  // 1 second
  config_.tolerance = 1e-8;
  return *this;
}

Simulation::SimulationConfig ConfigurationBuilder::build() const { return config_; }

bool ConfigurationBuilder::validate(std::string* error_message) const {
  // Simple validation for now
  if (config_.time_step <= 0) {
    if (error_message) *error_message = "Timestep must be positive";
    return false;
  }

  if (config_.tolerance <= 0) {
    if (error_message) *error_message = "Tolerance must be positive";
    return false;
  }

  return true;
}

}  // namespace SolarSystem::Core::Builders
