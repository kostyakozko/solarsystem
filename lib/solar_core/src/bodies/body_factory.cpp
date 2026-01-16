#include "solar_core/bodies/body_factory.hpp"

#include "solar_core/data/body_definitions.hpp"

// Include existing JPL system
#include <algorithm>
#include <charconv>
#include <sstream>

namespace SolarSystem::Bodies {

BodyFactory::BodyFactory(CreationOptions options)
    : default_options_(std::move(options)),
      current_source_("UNINITIALIZED"),
      data_initialized_(false) {
  // Only initialize JPL data system if we're going to use JPL sources
  if (default_options_.preferred_source == DataSource::JPL_HORIZONS ||
      default_options_.preferred_source == DataSource::CACHED_DATA) {
    initialize_internal_data();
  }
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_body(std::string_view name) const {
  return create_body(name, default_options_);
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_body(
    std::string_view name, const CreationOptions& options) const {
  const auto& opts =
      options.preferred_source != default_options_.preferred_source ? options : default_options_;

  // Use intelligent fallback strategies if specified
  if (opts.fallback_strategy != FallbackStrategy::GRACEFUL ||
      opts.minimum_quality != DataQuality::ACCEPTABLE || opts.allow_partial_data ||
      !opts.prefer_recent_data) {
    return create_body_with_intelligent_fallback(name, opts);
  }

  // Original fallback logic for backward compatibility
  switch (opts.preferred_source) {
    case DataSource::JPL_HORIZONS:
      if (auto result = create_from_jpl(name, opts.reference_time); result.has_value()) {
        return result;
      }
      if (!opts.allow_fallback) {
        return Utils::Expected<CelestialBody, std::string>{
            "Failed to fetch JPL data for " + std::string(name) + " and fallback disabled"};
      }
      [[fallthrough]];

    case DataSource::CACHED_DATA:
      if (auto result = create_from_cache(name, opts.reference_time); result.has_value()) {
        return result;
      }
      if (!opts.allow_fallback) {
        return Utils::Expected<CelestialBody, std::string>{
            "Failed to load cached data for " + std::string(name) + " and fallback disabled"};
      }
      [[fallthrough]];

    case DataSource::FALLBACK_DATA:
      return create_from_fallback(name);
  }

  return Utils::Expected<CelestialBody, std::string>{"Failed to create body " + std::string(name) +
                                                     " from any data source"};
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_collection(
    const std::vector<std::string>& body_names) const {
  return create_collection(body_names, default_options_);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_collection(
    const std::vector<std::string>& body_names, const CreationOptions& options) const {
  BodyCollection collection;
  std::vector<std::string> errors;
  std::vector<CelestialBody> created_bodies;  // For relationship validation

  for (const auto& name : body_names) {
    auto body_result = create_body(name, options);
    if (body_result.has_value()) {
      auto body = body_result.value();

      // Apply relationship validation if enabled
      if (options.validate_data) {
        CelestialBody::Properties props{.name = std::string(body.name()),
                                        .mass = body.mass(),
                                        .position = body.position(),
                                        .velocity = body.velocity(),
                                        .type = body.type(),
                                        .priority = body.priority(),
                                        .jpl_id = body.jpl_id(),
                                        .creation_date = body.creation_date()};

        auto relationship_validation = validate_body_relationships(props, created_bodies);
        if (!relationship_validation.has_value()) {
          errors.push_back("Relationship validation failed for " + name + ": " +
                           relationship_validation.error());
          if (!options.allow_fallback) {
            continue;  // Skip this body if validation fails and fallback not allowed
          }
        }
      }

      try {
        collection.add_body(body);
        created_bodies.push_back(std::move(body));  // Track for relationship validation
      } catch (const std::exception& e) {
        errors.push_back("Failed to add " + name + ": " + e.what());
      }
    } else {
      errors.push_back("Failed to create " + name + ": " + body_result.error());
    }
  }

  if (!errors.empty() && !options.allow_fallback) {
    std::ostringstream oss;
    oss << "Failed to create collection with errors:\n";
    for (const auto& error : errors) {
      oss << "  - " << error << "\n";
    }
    return Utils::Expected<BodyCollection, std::string>{oss.str()};
  }

  return Utils::Expected<BodyCollection, std::string>{std::move(collection)};
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_solar_system() const {
  return create_solar_system(default_options_);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_solar_system(
    const CreationOptions& options) const {
  // Get ALL bodies from our modern definitions (not just essential)
  std::vector<std::string> body_names;

  // Add all available bodies from our definitions
  for (const auto& body_def : Data::get_all_body_definitions()) {
    body_names.emplace_back(body_def.name);
  }

  return create_collection(body_names, options);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_inner_planets() const {
  return create_inner_planets(default_options_);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_inner_planets(
    const CreationOptions& options) const {
  std::vector<std::string> inner_planets = {"Sun", "Mercury", "Venus", "Earth", "Mars"};

  return create_collection(inner_planets, options);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_essential_bodies() const {
  return create_essential_bodies(default_options_);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_essential_bodies(
    const CreationOptions& options) const {
  return create_bodies_for_set(DefaultBodySet::ESSENTIAL, options);
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_from_legacy_data(
    std::string_view name) const {
  // First try to use cached ephemeris data if available
  if (current_source_ != "ORIGINAL_DATA" && !cached_ephemeris_.empty()) {
    for (const auto& data : cached_ephemeris_) {
      if (data.body_name == name) {
        auto celestial_body = data.to_celestial_body();

        // Apply validation if enabled
        if (default_options_.validate_data) {
          CelestialBody::Properties props{.name = std::string(celestial_body.name()),
                                          .mass = celestial_body.mass(),
                                          .position = celestial_body.position(),
                                          .velocity = celestial_body.velocity(),
                                          .type = celestial_body.type(),
                                          .priority = celestial_body.priority(),
                                          .jpl_id = celestial_body.jpl_id(),
                                          .creation_date = celestial_body.creation_date()};

          auto validation_result = validate_physical_properties(props);
          if (!validation_result.has_value()) {
            return Utils::Expected<CelestialBody, std::string>{
                "Validation failed for " + std::string(name) +
                " from cached data: " + validation_result.error()};
          }
        }

        return Utils::Expected<CelestialBody, std::string>{celestial_body};
      }
    }
  }

  // Fall back to hardcoded fallback data
  return create_from_fallback(name);
}

std::vector<std::string> BodyFactory::get_available_bodies() const {
  std::vector<std::string> bodies;

  // Get bodies from our fallback definitions
  for (const auto& body_def : Data::FALLBACK_SOLAR_SYSTEM) {
    bodies.emplace_back(body_def.name);
  }

  return bodies;
}

bool BodyFactory::is_body_available(std::string_view name,
                                    std::chrono::system_clock::time_point time) const {
  // Check if it's a spacecraft with creation date restrictions
  auto fallback_body = Data::get_fallback_body(name);
  if (fallback_body.has_value()) {
    auto celestial_body = fallback_body->to_celestial_body();
    return celestial_body.is_available_at(time);
  }

  // For other bodies, assume always available
  return true;
}

// Private implementation methods

Utils::Expected<CelestialBody, std::string> BodyFactory::create_from_jpl(
    std::string_view name, std::chrono::system_clock::time_point time) const {
  if (!jpl_client_) {
    return Utils::Expected<CelestialBody, std::string>{"JPL client not initialized"};
  }

  // Get JPL ID for the body name
  auto jpl_id_opt = Bodies::get_jpl_id_for_body_name(name);
  if (!jpl_id_opt) {
    return Utils::Expected<CelestialBody, std::string>{"Unknown body name: " + std::string(name)};
  }

  // Fetch single body data
  auto future = jpl_client_->fetch_body_async(jpl_id_opt.value(), time);
  auto result = future.get();

  if (!SolarSystem::JPL::is_success(result)) {
    auto error = SolarSystem::JPL::get_error(result);
    std::string error_msg = "Failed to fetch JPL data: ";
    switch (error) {
      case SolarSystem::JPL::JPLError::NetworkError:
        error_msg += "Network error";
        break;
      case SolarSystem::JPL::JPLError::ParseError:
        error_msg += "Parse error";
        break;
      case SolarSystem::JPL::JPLError::InvalidBody:
        error_msg += "Invalid body";
        break;
      case SolarSystem::JPL::JPLError::InvalidDate:
        error_msg += "Invalid date";
        break;
      case SolarSystem::JPL::JPLError::RateLimited:
        error_msg += "Rate limited";
        break;
      case SolarSystem::JPL::JPLError::ServerError:
        error_msg += "Server error";
        break;
      case SolarSystem::JPL::JPLError::CacheError:
        error_msg += "Cache error";
        break;
      case SolarSystem::JPL::JPLError::ValidationError:
        error_msg += "Validation error";
        break;
    }
    return Utils::Expected<CelestialBody, std::string>{error_msg};
  }

  auto ephemeris_data = SolarSystem::JPL::get_value(result);

  // Convert EphemerisData to CelestialBody
  auto celestial_body = ephemeris_data.to_celestial_body();

  // Apply validation if enabled
  if (default_options_.validate_data) {
    // Extract properties for validation
    CelestialBody::Properties props{.name = std::string(celestial_body.name()),
                                    .mass = celestial_body.mass(),
                                    .position = celestial_body.position(),
                                    .velocity = celestial_body.velocity(),
                                    .type = celestial_body.type(),
                                    .priority = celestial_body.priority(),
                                    .jpl_id = celestial_body.jpl_id(),
                                    .creation_date = celestial_body.creation_date()};

    auto validation_result = validate_physical_properties(props);
    if (!validation_result.has_value()) {
      return Utils::Expected<CelestialBody, std::string>{
          "Validation failed for " + std::string(name) +
          " from JPL data: " + validation_result.error()};
    }
  }

  return Utils::Expected<CelestialBody, std::string>{celestial_body};
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_from_cache(
    std::string_view name, std::chrono::system_clock::time_point) const {
  if (!jpl_client_) {
    return Utils::Expected<CelestialBody, std::string>{"JPL client not initialized"};
  }

  // Try to load from cache
  auto cache_result = jpl_client_->load_from_cache();
  if (!SolarSystem::JPL::is_success(cache_result)) {
    return Utils::Expected<CelestialBody, std::string>{"No cached ephemeris data available"};
  }

  auto cached_data = SolarSystem::JPL::get_value(cache_result);

  // Find the requested body by name
  for (const auto& data : cached_data) {
    if (data.body_name == name) {
      return Utils::Expected<CelestialBody, std::string>{data.to_celestial_body()};
    }
  }

  // Try alternative name matching (case-insensitive)
  std::string lower_name = std::string(name);
  std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

  for (const auto& data : cached_data) {
    std::string lower_body_name = data.body_name;
    std::transform(lower_body_name.begin(), lower_body_name.end(), lower_body_name.begin(),
                   ::tolower);

    if (lower_body_name == lower_name) {
      return Utils::Expected<CelestialBody, std::string>{data.to_celestial_body()};
    }
  }

  return Utils::Expected<CelestialBody, std::string>{"Body " + std::string(name) +
                                                     " not found in cached data"};
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_from_fallback(
    std::string_view name) const {
  // Use our modern fallback data
  auto fallback_body = Data::get_fallback_body(name);
  if (fallback_body.has_value()) {
    auto celestial_body = fallback_body->to_celestial_body();

    // Apply validation if enabled
    if (default_options_.validate_data) {
      CelestialBody::Properties props{.name = std::string(celestial_body.name()),
                                      .mass = celestial_body.mass(),
                                      .position = celestial_body.position(),
                                      .velocity = celestial_body.velocity(),
                                      .type = celestial_body.type(),
                                      .priority = celestial_body.priority(),
                                      .jpl_id = celestial_body.jpl_id(),
                                      .creation_date = celestial_body.creation_date()};

      auto validation_result = validate_physical_properties(props);
      if (!validation_result.has_value()) {
        return Utils::Expected<CelestialBody, std::string>{
            "Validation failed for " + std::string(name) +
            " from fallback data: " + validation_result.error()};
      }
    }

    return Utils::Expected<CelestialBody, std::string>{celestial_body};
  }

  return Utils::Expected<CelestialBody, std::string>{"Body " + std::string(name) +
                                                     " not found in fallback data"};
}

BodyType BodyFactory::determine_body_type(std::string_view name) const {
  auto jpl_id = get_jpl_id(name);
  if (jpl_id) {
    return get_body_type_for_jpl_id(jpl_id.value());
  } else {
    return BodyType::Unknown;
  }
}

BodyPriority BodyFactory::determine_body_priority(std::string_view name) const {
  return get_priority_for_body_type(determine_body_type(name));
}

std::optional<int> BodyFactory::get_jpl_id(std::string_view name) const {
  // Find in JPL body map
  for (size_t i = 0; i < SolarSystem::Data::BODY_COUNT; ++i) {
    if (name == Data::FALLBACK_SOLAR_SYSTEM[i].name) {
      int jpl_id = 0;
      auto sv = Data::FALLBACK_SOLAR_SYSTEM[i].jpl_id;
      auto result = std::from_chars(sv.data(), sv.data() + sv.size(), jpl_id);
      if (result.ec != std::errc{}) {
        return std::nullopt;
      }
      return jpl_id;
    }
  }
  return std::nullopt;
}

void BodyFactory::initialize_internal_data() {
  if (data_initialized_) {
    return;
  }

  using namespace std::chrono;
  using namespace SolarSystem::JPL;

  // Create JPL client
  jpl_client_ = JPLClientFactory::create_default();

  // Try to load cached data or use fallback
  auto result = jpl_client_->load_from_cache();

  if (is_success(result)) {
    cached_ephemeris_ = get_value(result);
    if (!cached_ephemeris_.empty()) {
      current_epoch_ = cached_ephemeris_[0].epoch;
    } else {
      current_epoch_ = system_clock::now();  // or from cache metadata
    }
    current_source_ = "CACHED_DATA";
  } else {
    // Fallback to hardcoded data equivalent
    year_month_day ymd{year{2018}, month{2}, day{11}};
    current_epoch_ = sys_days{ymd};
    current_source_ = "ORIGINAL_DATA";
  }

  data_initialized_ = true;
}

[[nodiscard]] bool BodyFactory::has_current_year_ephemeris_data() const noexcept {
  if (!has_current_ephemeris_data()) {
    return false;
  }

  // Get current time
  auto now = std::chrono::system_clock::now();

  // Convert both times to year-month-day for comparison
  auto now_days = std::chrono::floor<std::chrono::days>(now);
  auto epoch_days = std::chrono::floor<std::chrono::days>(current_epoch_);

  auto now_ymd = std::chrono::year_month_day{now_days};
  auto epoch_ymd = std::chrono::year_month_day{epoch_days};

  // Compare years
  return now_ymd.year() == epoch_ymd.year();
}

SolarSystem::Utils::Expected<void, std::string> BodyFactory::fetch_current_ephemeris_data(
    std::chrono::system_clock::time_point time) {
  auto future = jpl_client_->fetch_all_bodies_async(time);
  auto result = future.get();

  if (!SolarSystem::JPL::is_success(result)) {
    auto error = SolarSystem::JPL::get_error(result);
    return SolarSystem::Utils::Expected<void, std::string>{
        SolarSystem::JPL::Utils::to_string(error)};
  }

  // Get the fetched data
  auto ephemeris_data = SolarSystem::JPL::get_value(result);

  // Save to cache for persistence
  auto save_result = jpl_client_->save_to_cache(ephemeris_data);
  if (!SolarSystem::JPL::is_success(save_result)) {
    auto error = save_result.value();
    return SolarSystem::Utils::Expected<void, std::string>{
        "Failed to save ephemeris data to cache: " + SolarSystem::JPL::Utils::to_string(error)};
  }

  // Update internal state
  cached_ephemeris_ = ephemeris_data;
  current_epoch_ = time;
  current_source_ = "JPL_DATA";

  return SolarSystem::Utils::Expected<void, std::string>{};
}

SolarSystem::Utils::Expected<void, std::string> BodyFactory::rebuild_cache() {
  auto result = jpl_client_->rebuild_cache();

  if (!SolarSystem::JPL::is_success(result)) {
    // For JPLVoidResult (std::optional<JPLError>), access the error directly:
    auto error = result.value();  // Get the JPLError from optional
    return SolarSystem::Utils::Expected<void, std::string>{
        SolarSystem::JPL::Utils::to_string(error)};
  }

  return SolarSystem::Utils::Expected<void, std::string>{};
}

SolarSystem::Utils::Expected<void, std::string> BodyFactory::test_storage_system() {
  // Use JPL client to test storage system
  auto result = jpl_client_->test_storage();

  if (!SolarSystem::JPL::is_success(result)) {
    auto error = result.value();  // JPLVoidResult uses .value()
    return SolarSystem::Utils::Expected<void, std::string>{
        SolarSystem::JPL::Utils::to_string(error)};
  }

  // Update internal state for testing
  current_epoch_ = std::chrono::system_clock::now();
  current_source_ = "TEST_DATA";

  return SolarSystem::Utils::Expected<void, std::string>{};
}

std::chrono::system_clock::time_point BodyFactory::get_current_year_epoch() noexcept {
  auto now = std::chrono::system_clock::now();
  auto now_days = std::chrono::floor<std::chrono::days>(now);
  auto now_ymd = std::chrono::year_month_day{now_days};
  auto current_year_start = std::chrono::sys_days{now_ymd.year() / std::chrono::January / 1};
  return std::chrono::system_clock::time_point{current_year_start};
}

// Comprehensive Body Validation Implementation

/**
 * @brief Validate all physical properties against realistic bounds
 */
Utils::Expected<void, std::string> BodyFactory::validate_physical_properties(
    const CelestialBody::Properties& props) const {
  // Validate mass bounds
  auto mass_validation = validate_mass_bounds(props.mass, props.type, props.name);
  if (!mass_validation.has_value()) {
    return mass_validation;
  }

  // Validate orbital parameters
  auto orbital_validation =
      validate_orbital_parameters(props.position, props.velocity, props.type, props.name);
  if (!orbital_validation.has_value()) {
    return orbital_validation;
  }

  // Validate cross-property relationships
  auto cross_validation = validate_cross_properties(props);
  if (!cross_validation.has_value()) {
    return cross_validation;
  }

  return Utils::Expected<void, std::string>{};
}

/**
 * @brief Validate mass against realistic bounds for body type
 */
Utils::Expected<void, std::string> BodyFactory::validate_mass_bounds(long double mass,
                                                                     BodyType type,
                                                                     std::string_view name) const {
  // Check for non-positive mass
  if (mass <= 0.0L) {
    return Utils::Expected<void, std::string>{"Invalid mass for " + std::string(name) +
                                              ": mass must be positive, got " +
                                              std::to_string(static_cast<double>(mass)) + " kg"};
  }

  // Check for NaN or infinite mass
  if (!std::isfinite(static_cast<double>(mass))) {
    return Utils::Expected<void, std::string>{"Invalid mass for " + std::string(name) +
                                              ": mass must be finite, got non-finite value"};
  }

  // Define realistic mass bounds for different body types (in kg)
  struct MassBounds {
    long double min_mass;
    long double max_mass;
    std::string description;
  };

  MassBounds bounds;
  switch (type) {
    case BodyType::Star:
      bounds = {1.0e29L, 1.0e32L, "stellar mass (0.05 to 50 solar masses)"};
      break;
    case BodyType::Planet:
      bounds = {3.0e23L, 2.0e30L, "planetary mass (Mercury to 10 Jupiter masses)"};
      break;
    case BodyType::DwarfPlanet:
      bounds = {1.0e20L, 1.5e23L, "dwarf planet mass (Ceres to Pluto range)"};
      break;
    case BodyType::Moon:
      bounds = {1.0e19L, 1.5e23L,
                "moon mass (small asteroids to Ganymede)"};  // Increased upper bound for Ganymede
      break;
    case BodyType::Asteroid:
      bounds = {1.0e12L, 1.0e22L, "asteroid mass (small rocks to Vesta)"};
      break;
    case BodyType::Spacecraft:
      bounds = {1.0e2L, 1.0e6L, "spacecraft mass (100 kg to 1000 tons)"};
      break;
    case BodyType::Unknown:
      bounds = {1.0e12L, 1.0e32L, "unknown body mass (very broad range)"};
      break;
  }

  if (mass < bounds.min_mass || mass > bounds.max_mass) {
    return Utils::Expected<void, std::string>{
        "Mass validation failed for " + std::string(name) +
        " (type: " + std::to_string(static_cast<int>(type)) + "): " + "mass " +
        std::to_string(static_cast<double>(mass)) + " kg is outside realistic " +
        bounds.description + " range [" + std::to_string(static_cast<double>(bounds.min_mass)) +
        ", " + std::to_string(static_cast<double>(bounds.max_mass)) + "] kg"};
  }

  return Utils::Expected<void, std::string>{};
}

/**
 * @brief Validate orbital parameters against realistic bounds
 */
Utils::Expected<void, std::string> BodyFactory::validate_orbital_parameters(
    const Math::Vector3d& position, const Math::Vector3d& velocity, BodyType type,
    std::string_view name) const {
  // Calculate orbital characteristics
  long double pos_magnitude = position.magnitude();
  long double vel_magnitude = velocity.magnitude();

  // Check for NaN or infinite values
  if (!std::isfinite(static_cast<double>(pos_magnitude)) ||
      !std::isfinite(static_cast<double>(vel_magnitude))) {
    return Utils::Expected<void, std::string>{"Invalid orbital parameters for " +
                                              std::string(name) +
                                              ": position or velocity contains non-finite values"};
  }

  // Define realistic orbital bounds based on body type
  struct OrbitalBounds {
    long double min_distance;  // meters
    long double max_distance;  // meters
    long double max_velocity;  // m/s
    std::string description;
  };

  OrbitalBounds bounds;
  switch (type) {
    case BodyType::Star:
      bounds = {0.0L, 1.0e18L, 1.0e6L, "stellar system (galactic scale)"};
      break;
    case BodyType::Planet:
      bounds = {1.0e10L, 1.0e13L, 1.0e5L, "planetary orbit (0.1 to 100 AU)"};
      break;
    case BodyType::DwarfPlanet:
      bounds = {1.0e11L, 1.0e14L, 5.0e4L, "dwarf planet orbit (1 to 1000 AU)"};
      break;
    case BodyType::Moon:
      bounds = {1.0e8L, 1.0e13L, 5.0e4L,
                "moon orbit (100 km to solar system scale)"};  // Allow higher velocities for moons
                                                               // in solar system reference frame
      break;
    case BodyType::Asteroid:
      bounds = {1.0e10L, 1.0e13L, 1.0e5L, "asteroid orbit (similar to planets)"};
      break;
    case BodyType::Spacecraft:
      bounds = {1.0e8L, 1.0e13L, 2.0e5L, "spacecraft orbit (Earth orbit to interplanetary)"};
      break;
    case BodyType::Unknown:
      bounds = {1.0e8L, 1.0e18L, 1.0e6L, "unknown body orbit (very broad range)"};
      break;
  }

  // Validate position bounds
  if (pos_magnitude < bounds.min_distance || pos_magnitude > bounds.max_distance) {
    return Utils::Expected<void, std::string>{
        "Orbital distance validation failed for " + std::string(name) +
        " (type: " + std::to_string(static_cast<int>(type)) + "): " + "distance " +
        std::to_string(static_cast<double>(pos_magnitude)) + " m is outside realistic " +
        bounds.description + " range [" + std::to_string(static_cast<double>(bounds.min_distance)) +
        ", " + std::to_string(static_cast<double>(bounds.max_distance)) + "] m"};
  }

  // Validate velocity bounds
  if (vel_magnitude > bounds.max_velocity) {
    return Utils::Expected<void, std::string>{
        "Orbital velocity validation failed for " + std::string(name) +
        " (type: " + std::to_string(static_cast<int>(type)) + "): " + "velocity " +
        std::to_string(static_cast<double>(vel_magnitude)) + " m/s exceeds realistic maximum " +
        std::to_string(static_cast<double>(bounds.max_velocity)) + " m/s for " +
        bounds.description};
  }

  // Check for zero position (bodies at origin are suspicious)
  if (pos_magnitude < 1.0e6L && type != BodyType::Star) {  // 1000 km minimum for non-stars
    return Utils::Expected<void, std::string>{
        "Suspicious orbital position for " + std::string(name) +
        ": body is very close to origin (" + std::to_string(static_cast<double>(pos_magnitude)) +
        " m), which may indicate invalid data"};
  }

  return Utils::Expected<void, std::string>{};
}

/**
 * @brief Validate cross-relationships between properties
 */
Utils::Expected<void, std::string> BodyFactory::validate_cross_properties(
    const CelestialBody::Properties& props) const {
  long double pos_magnitude = props.position.magnitude();
  long double vel_magnitude = props.velocity.magnitude();

  // Calculate specific orbital energy (energy per unit mass)
  // E = v²/2 - GM/r (simplified, assuming central body mass >> test body mass)
  // For bound orbits, E should be negative

  // Use approximate solar mass for central body (this is a rough validation)
  const long double solar_mass = 1.989e30L;  // kg
  const long double G = 6.67430e-11L;        // m³/kg/s²

  long double kinetic_energy_per_mass = vel_magnitude * vel_magnitude / 2.0L;
  long double potential_energy_per_mass = G * solar_mass / pos_magnitude;
  long double specific_energy = kinetic_energy_per_mass - potential_energy_per_mass;

  // For most solar system bodies, orbits should be bound (negative energy)
  // Allow some tolerance for measurement errors and non-solar-centric orbits
  // Be more lenient for moons which may be in solar system reference frame
  long double energy_tolerance = (props.type == BodyType::Moon) ? 2.0e8L : 1.0e8L;
  if (specific_energy > energy_tolerance && props.type != BodyType::Spacecraft) {
    return Utils::Expected<void, std::string>{
        "Cross-validation failed for " + props.name +
        ": orbital energy suggests unbound trajectory (" +
        std::to_string(static_cast<double>(specific_energy)) + " J/kg), which is unusual for " +
        std::to_string(static_cast<int>(props.type)) + " type bodies"};
  }

  // Validate mass-to-size relationship (rough check)
  // Larger bodies should generally have more mass, but this is very approximate
  if (props.type == BodyType::Planet || props.type == BodyType::DwarfPlanet) {
    // Very rough density check - most rocky/icy bodies have density 1000-8000 kg/m³
    // Assume spherical body: mass = density * (4/3) * π * r³
    // Estimate radius from orbital distance (very rough approximation)
    long double estimated_radius =
        std::pow(static_cast<double>(props.mass) / (5000.0 * 4.0 / 3.0 * M_PI), 1.0 / 3.0);

    // This is a very loose check - just catch obviously wrong values
    if (estimated_radius >
        pos_magnitude / 10.0L) {  // Body can't be larger than 1/10 its orbital distance
      return Utils::Expected<void, std::string>{
          "Cross-validation warning for " + props.name +
          ": mass-to-orbital-distance ratio suggests " +
          "unrealistic body size (estimated radius " +
          std::to_string(static_cast<double>(estimated_radius)) + " m vs orbital distance " +
          std::to_string(static_cast<double>(pos_magnitude)) + " m)"};
    }
  }

  return Utils::Expected<void, std::string>{};
}

/**
 * @brief Validate body relationships and dependencies
 */
Utils::Expected<void, std::string> BodyFactory::validate_body_relationships(
    const CelestialBody::Properties& props,
    const std::vector<CelestialBody>& existing_bodies) const {
  // Check for duplicate names
  for (const auto& existing_body : existing_bodies) {
    if (existing_body.name() == props.name) {
      return Utils::Expected<void, std::string>{
          "Body relationship validation failed: duplicate body name '" + props.name +
          "' already exists in collection"};
    }
  }

  // Validate moon-planet relationships
  if (props.type == BodyType::Moon) {
    // Moons should be relatively close to a planet
    bool found_nearby_planet = false;
    long double min_planet_distance = std::numeric_limits<long double>::max();

    for (const auto& existing_body : existing_bodies) {
      if (existing_body.type() == BodyType::Planet) {
        long double distance = (props.position - existing_body.position()).magnitude();
        min_planet_distance = std::min(min_planet_distance, distance);

        // Hill sphere approximation: a_hill ≈ a * (m_planet / 3*m_star)^(1/3)
        // For rough validation, assume moon should be within reasonable distance from planet
        // Allow larger distances for moons positioned in solar system reference frame
        if (distance <
            1.0e12L) {  // Within 1 million km to 1 million km (very lenient for solar system scale)
          found_nearby_planet = true;
          break;
        }
      }
    }

    if (!found_nearby_planet && !existing_bodies.empty()) {
      // Check if there's a dwarf planet nearby (for moons like Charon)
      for (const auto& existing_body : existing_bodies) {
        if (existing_body.type() == BodyType::DwarfPlanet) {
          long double distance = (props.position - existing_body.position()).magnitude();
          if (distance < 1.0e12L) {  // Within reasonable distance of dwarf planet
            found_nearby_planet = true;
            break;
          }
        }
      }

      if (!found_nearby_planet) {
        return Utils::Expected<void, std::string>{
            "Body relationship validation failed for moon '" + props.name +
            "': no nearby planet found " + "(closest planet at " +
            std::to_string(static_cast<double>(min_planet_distance)) + " m, " +
            "expected < 1e12 m for typical moon orbits)"};
      }
    }
  }

  // Validate spacecraft temporal relationships
  if (props.type == BodyType::Spacecraft && props.creation_date.has_value()) {
    auto creation_time = props.creation_date.value();
    auto now = std::chrono::system_clock::now();

    // Spacecraft shouldn't be created in the future
    if (creation_time > now) {
      return Utils::Expected<void, std::string>{
          "Body relationship validation failed for spacecraft '" + props.name +
          "': " + "creation date is in the future"};
    }

    // Spacecraft shouldn't be too old (before space age ~1957)
    struct tm space_age_tm = {};
    space_age_tm.tm_year = 57;  // 1957
    space_age_tm.tm_mon = 9;    // October (0-based)
    space_age_tm.tm_mday = 4;   // 4th
    auto space_age_start = std::chrono::system_clock::from_time_t(std::mktime(&space_age_tm));

    if (creation_time < space_age_start) {
      return Utils::Expected<void, std::string>{
          "Body relationship validation failed for spacecraft '" + props.name +
          "': " + "creation date predates the space age (before 1957)"};
    }
  }

  // Check for mass hierarchy violations (very rough check)
  if (props.type == BodyType::Star) {
    // Stars should be the most massive objects in the system
    for (const auto& existing_body : existing_bodies) {
      if (existing_body.mass() > props.mass && existing_body.type() != BodyType::Star) {
        return Utils::Expected<void, std::string>{
            "Body relationship validation warning for star '" + props.name +
            "': " + "non-stellar body '" + std::string(existing_body.name()) +
            "' has greater mass (" + std::to_string(static_cast<double>(existing_body.mass())) +
            " kg vs " + std::to_string(static_cast<double>(props.mass)) + " kg)"};
      }
    }
  }

  return Utils::Expected<void, std::string>{};
}

// Intelligent fallback strategies implementation

Utils::Expected<CelestialBody, std::string> BodyFactory::create_body_with_intelligent_fallback(
    std::string_view name, const CreationOptions& options) const {
  // Execute the fallback strategy
  auto fallback_result = execute_fallback_strategy(name, options);

  if (!fallback_result.success) {
    return Utils::Expected<CelestialBody, std::string>{"Intelligent fallback failed for '" +
                                                       std::string(name) +
                                                       "': no suitable data source found"};
  }

  // Try to create the body using the determined strategy
  switch (options.fallback_strategy) {
    case FallbackStrategy::STRICT: {
      // Only use preferred source, no fallback - avoid recursion
      switch (options.preferred_source) {
        case DataSource::JPL_HORIZONS:
          return create_from_jpl(name, options.reference_time);
        case DataSource::CACHED_DATA:
          return create_from_cache(name, options.reference_time);
        case DataSource::FALLBACK_DATA:
          return create_from_fallback(name);
      }
      break;
    }

    case FallbackStrategy::GRACEFUL:
      return create_with_graceful_fallback(name, options);

    case FallbackStrategy::INTELLIGENT:
      return create_with_quality_assessment(name, options);

    case FallbackStrategy::PARTIAL_ALLOWED:
      return create_with_partial_data(name, options);

    case FallbackStrategy::HYBRID:
      return create_with_hybrid_approach(name, options);
  }

  return Utils::Expected<CelestialBody, std::string>{"Unknown fallback strategy for '" +
                                                     std::string(name) + "'"};
}

std::vector<BodyFactory::DataSourceInfo> BodyFactory::assess_data_sources(
    std::string_view name, const CreationOptions& options) const {
  std::vector<DataSourceInfo> sources;

  // Assess JPL HORIZONS
  if (is_data_source_available(DataSource::JPL_HORIZONS, name)) {
    DataSourceInfo jpl_info;
    jpl_info.source = DataSource::JPL_HORIZONS;
    jpl_info.last_updated = std::chrono::system_clock::now();  // Real-time data
    jpl_info.is_complete = true;
    jpl_info.quality = DataQuality::EXCELLENT;
    jpl_info.quality_reason = "Real-time JPL HORIZONS data";
    sources.push_back(jpl_info);
  }

  // Assess cached data
  if (is_data_source_available(DataSource::CACHED_DATA, name)) {
    DataSourceInfo cache_info;
    cache_info.source = DataSource::CACHED_DATA;
    cache_info.last_updated = get_data_source_timestamp(DataSource::CACHED_DATA, name);
    cache_info.is_complete = true;

    // Assess cache quality based on age
    auto age = std::chrono::system_clock::now() - cache_info.last_updated;
    if (age < std::chrono::hours(24)) {
      cache_info.quality = DataQuality::EXCELLENT;
      cache_info.quality_reason = "Recent cached JPL data (< 24 hours)";
    } else if (age < std::chrono::hours(24 * 7)) {
      cache_info.quality = DataQuality::GOOD;
      cache_info.quality_reason = "Cached JPL data (< 1 week)";
    } else if (age < options.max_data_age) {
      cache_info.quality = DataQuality::ACCEPTABLE;
      cache_info.quality_reason = "Older cached JPL data (within max age)";
    } else {
      cache_info.quality = DataQuality::POOR;
      cache_info.quality_reason = "Very old cached data (exceeds max age)";
    }

    sources.push_back(cache_info);
  }

  // Assess fallback data
  if (is_data_source_available(DataSource::FALLBACK_DATA, name)) {
    DataSourceInfo fallback_info;
    fallback_info.source = DataSource::FALLBACK_DATA;
    fallback_info.last_updated = get_data_source_timestamp(DataSource::FALLBACK_DATA, name);
    fallback_info.is_complete = true;
    fallback_info.quality = DataQuality::ACCEPTABLE;
    fallback_info.quality_reason = "Hardcoded fallback data (static reference epoch)";
    sources.push_back(fallback_info);
  }

  // Sort by quality (best first)
  std::sort(sources.begin(), sources.end(), [](const DataSourceInfo& a, const DataSourceInfo& b) {
    return static_cast<int>(a.quality) < static_cast<int>(b.quality);
  });

  return sources;
}

BodyFactory::DataQuality BodyFactory::assess_data_quality(
    const CelestialBody& body, DataSource source, std::chrono::system_clock::time_point) const {
  // Check for basic data validity
  if (body.mass() <= 0.0L) {
    return DataQuality::POOR;
  }

  const auto& pos = body.position();
  const auto& vel = body.velocity();

  // Check for non-finite values
  if (!std::isfinite(static_cast<double>(pos.x())) ||
      !std::isfinite(static_cast<double>(pos.y())) ||
      !std::isfinite(static_cast<double>(pos.z())) ||
      !std::isfinite(static_cast<double>(vel.x())) ||
      !std::isfinite(static_cast<double>(vel.y())) ||
      !std::isfinite(static_cast<double>(vel.z()))) {
    return DataQuality::POOR;
  }

  // Assess based on source type
  switch (source) {
    case DataSource::JPL_HORIZONS:
      return DataQuality::EXCELLENT;

    case DataSource::CACHED_DATA: {
      // Quality depends on cache age (would need to check actual cache timestamp)
      return DataQuality::GOOD;
    }

    case DataSource::FALLBACK_DATA:
      // Check if fallback data seems reasonable
      long double pos_magnitude = pos.magnitude();
      long double vel_magnitude = vel.magnitude();

      // Very rough sanity checks for solar system scale
      if (pos_magnitude > 1.0e15L || vel_magnitude > 1.0e6L) {
        return DataQuality::POOR;
      }

      return DataQuality::ACCEPTABLE;
  }

  return DataQuality::UNKNOWN;
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_with_hybrid_approach(
    std::string_view name, const CreationOptions& options) const {
  // Try to get the best available data from multiple sources
  auto sources = assess_data_sources(name, options);

  if (sources.empty()) {
    return Utils::Expected<CelestialBody, std::string>{"No data sources available for '" +
                                                       std::string(name) + "'"};
  }

  // Use the highest quality source that meets minimum requirements
  for (const auto& source_info : sources) {
    if (source_info.quality >= options.minimum_quality) {
      CreationOptions source_options = options;
      source_options.preferred_source = source_info.source;
      source_options.allow_fallback = false;  // Don't fallback within hybrid approach

      auto result = create_body(name, source_options);
      if (result.has_value()) {
        return result;
      }
    }
  }

  return Utils::Expected<CelestialBody, std::string>{
      "No data source met minimum quality requirements for '" + std::string(name) + "'"};
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_with_partial_data(
    std::string_view name, const CreationOptions& options) const {
  // Try each source and accept partial data if necessary
  auto sources = get_prioritized_sources(name, options);

  for (DataSource source : sources) {
    CreationOptions source_options = options;
    source_options.preferred_source = source;
    source_options.allow_fallback = false;
    source_options.validate_data = false;  // Allow partial/invalid data

    auto result = create_body(name, source_options);
    if (result.has_value()) {
      // Assess the quality and warn if poor
      auto quality = assess_data_quality(result.value(), source, options.reference_time);
      if (quality == DataQuality::POOR) {
        // Could log warning here, but for now just return the body
      }
      return result;
    }
  }

  return Utils::Expected<CelestialBody, std::string>{"Failed to create '" + std::string(name) +
                                                     "' even with partial data allowed"};
}

BodyFactory::FallbackResult BodyFactory::execute_fallback_strategy(
    std::string_view name, const CreationOptions& options) const {
  auto start_time = std::chrono::steady_clock::now();
  FallbackResult result;
  result.success = false;

  // Assess available data sources
  auto sources = assess_data_sources(name, options);

  if (sources.empty()) {
    result.fallback_chain.push_back("No data sources available");
    auto end_time = std::chrono::steady_clock::now();
    result.total_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return result;
  }

  // Execute strategy based on type
  switch (options.fallback_strategy) {
    case FallbackStrategy::STRICT:
      // Only use preferred source
      for (const auto& source_info : sources) {
        if (source_info.source == options.preferred_source) {
          result.success = true;
          result.source_used = source_info.source;
          result.data_quality = source_info.quality;
          result.fallback_chain.push_back("Using preferred source: " + source_info.quality_reason);
          break;
        }
      }
      break;

    case FallbackStrategy::GRACEFUL:
    case FallbackStrategy::INTELLIGENT:
      // Use best available source that meets quality requirements
      for (const auto& source_info : sources) {
        if (source_info.quality >= options.minimum_quality) {
          result.success = true;
          result.source_used = source_info.source;
          result.data_quality = source_info.quality;
          result.fallback_chain.push_back("Selected source: " + source_info.quality_reason);
          break;
        }
      }
      break;

    case FallbackStrategy::PARTIAL_ALLOWED:
      // Accept any available source
      if (!sources.empty()) {
        result.success = true;
        result.source_used = sources[0].source;
        result.data_quality = sources[0].quality;
        result.fallback_chain.push_back("Using best available: " + sources[0].quality_reason);
        if (sources[0].quality < options.minimum_quality) {
          result.warnings.push_back("Data quality below minimum requirements");
        }
      }
      break;

    case FallbackStrategy::HYBRID:
      // Complex logic handled in create_with_hybrid_approach
      result.success = !sources.empty();
      if (result.success) {
        result.source_used = sources[0].source;
        result.data_quality = sources[0].quality;
        result.fallback_chain.push_back("Hybrid approach with multiple sources");
      }
      break;
  }

  auto end_time = std::chrono::steady_clock::now();
  result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return result;
}

std::vector<BodyFactory::DataSource> BodyFactory::get_prioritized_sources(
    std::string_view name, const CreationOptions& options) const {
  std::vector<DataSource> sources;

  if (options.prefer_recent_data) {
    // Prioritize by data freshness
    if (is_data_source_available(DataSource::JPL_HORIZONS, name)) {
      sources.push_back(DataSource::JPL_HORIZONS);
    }
    if (is_data_source_available(DataSource::CACHED_DATA, name)) {
      sources.push_back(DataSource::CACHED_DATA);
    }
    if (is_data_source_available(DataSource::FALLBACK_DATA, name)) {
      sources.push_back(DataSource::FALLBACK_DATA);
    }
  } else {
    // Use original preference order
    sources.push_back(options.preferred_source);

    // Add other sources
    for (auto source :
         {DataSource::JPL_HORIZONS, DataSource::CACHED_DATA, DataSource::FALLBACK_DATA}) {
      if (source != options.preferred_source && is_data_source_available(source, name)) {
        sources.push_back(source);
      }
    }
  }

  return sources;
}

bool BodyFactory::is_data_source_available(DataSource source, std::string_view name) const {
  switch (source) {
    case DataSource::JPL_HORIZONS:
      // Check if JPL client is available and network accessible
      return jpl_client_ != nullptr;

    case DataSource::CACHED_DATA:
      // Check if we have cached data
      return has_current_ephemeris_data();

    case DataSource::FALLBACK_DATA:
      // Check if body exists in fallback data
      return Data::get_fallback_body(name).has_value();
  }

  return false;
}

std::chrono::system_clock::time_point BodyFactory::get_data_source_timestamp(
    DataSource source, std::string_view) const {
  switch (source) {
    case DataSource::JPL_HORIZONS:
      // JPL data is always current
      return std::chrono::system_clock::now();

    case DataSource::CACHED_DATA:
      // Return cache timestamp (use current epoch as approximation)
      return current_epoch_;

    case DataSource::FALLBACK_DATA:
      // Fallback data has a fixed reference epoch
      return get_current_year_epoch();
  }

  return std::chrono::system_clock::time_point{};
}

// Helper methods for graceful fallback and quality assessment

Utils::Expected<CelestialBody, std::string> BodyFactory::create_with_graceful_fallback(
    std::string_view name, const CreationOptions& options) const {
  auto sources = get_prioritized_sources(name, options);
  std::vector<std::string> errors;

  for (DataSource source : sources) {
    CreationOptions source_options = options;
    source_options.preferred_source = source;
    source_options.allow_fallback = false;

    auto result = create_body(name, source_options);
    if (result.has_value()) {
      return result;
    } else {
      errors.push_back("Source " + std::to_string(static_cast<int>(source)) + ": " +
                       result.error());
    }
  }

  // All sources failed
  std::string combined_error = "All data sources failed for '" + std::string(name) + "':\n";
  for (const auto& error : errors) {
    combined_error += "  - " + error + "\n";
  }

  return Utils::Expected<CelestialBody, std::string>{combined_error};
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_with_quality_assessment(
    std::string_view name, const CreationOptions& options) const {
  auto sources = assess_data_sources(name, options);

  // Filter sources by minimum quality
  auto suitable_sources = sources;
  suitable_sources.erase(std::remove_if(suitable_sources.begin(), suitable_sources.end(),
                                        [&options](const DataSourceInfo& info) {
                                          return info.quality < options.minimum_quality;
                                        }),
                         suitable_sources.end());

  if (suitable_sources.empty()) {
    return Utils::Expected<CelestialBody, std::string>{
        "No data sources meet minimum quality requirements for '" + std::string(name) + "'"};
  }

  // Try the best quality source first
  for (const auto& source_info : suitable_sources) {
    CreationOptions source_options = options;
    source_options.preferred_source = source_info.source;
    source_options.allow_fallback = false;

    auto result = create_body(name, source_options);
    if (result.has_value()) {
      return result;
    }
  }

  return Utils::Expected<CelestialBody, std::string>{"Failed to create '" + std::string(name) +
                                                     "' from quality-assessed sources"};
}

// New standardized default body selection methods
Utils::Expected<BodyCollection, std::string> BodyFactory::create_default_bodies() const {
  return create_default_bodies(default_options_);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_default_bodies(
    const CreationOptions& options) const {
  return create_bodies_for_set(options.default_body_set, options);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_bodies_for_set(
    DefaultBodySet body_set) const {
  return create_bodies_for_set(body_set, default_options_);
}

Utils::Expected<BodyCollection, std::string> BodyFactory::create_bodies_for_set(
    DefaultBodySet body_set, const CreationOptions& options) const {
  std::vector<std::string> body_names = get_bodies_for_set(body_set);
  return create_collection(body_names, options);
}

// Body set information utilities
std::vector<std::string> BodyFactory::get_bodies_for_set(DefaultBodySet body_set) const {
  switch (body_set) {
    case DefaultBodySet::ESSENTIAL:
      // Sun + 8 planets (9 bodies total)
      return {"Sun", "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"};

    case DefaultBodySet::IMPORTANT:
      // Essential + major moons + dwarf planets (18 bodies total)
      return {"Sun",      "Mercury",  "Venus",   "Earth", "Mars",   "Jupiter",
              "Saturn",   "Uranus",   "Neptune", "Moon",  "Io",     "Europa",
              "Ganymede", "Callisto", "Titan",   "Pluto", "Charon", "Triton"};

    case DefaultBodySet::COMPLETE:
      // All 27 bodies - get from body definitions
      {
        std::vector<std::string> all_bodies;
        for (const auto& body_def : Data::get_all_body_definitions()) {
          all_bodies.emplace_back(body_def.name);
        }
        return all_bodies;
      }

    default:
      // Fallback to important set
      return get_bodies_for_set(DefaultBodySet::IMPORTANT);
  }
}

size_t BodyFactory::get_body_count_for_set(DefaultBodySet body_set) const {
  return get_bodies_for_set(body_set).size();
}

std::string BodyFactory::get_body_set_description(DefaultBodySet body_set) const {
  switch (body_set) {
    case DefaultBodySet::ESSENTIAL:
      return "Essential bodies (Sun + 8 planets) - 9 bodies total. Fast performance, suitable for "
             "basic simulations.";
    case DefaultBodySet::IMPORTANT:
      return "Important bodies (Essential + major moons + dwarf planets) - 18 bodies total. "
             "Balanced performance and completeness.";
    case DefaultBodySet::COMPLETE:
      return "Complete solar system (all available bodies) - 27 bodies total. Comprehensive but "
             "slower performance.";
    default:
      return "Unknown body set";
  }
}

BodyFactory::DefaultBodySet BodyFactory::get_recommended_body_set_for_application(
    const std::string& application_name) {
  // Recommend body sets based on application characteristics
  if (application_name == "solar_system_realtime" || application_name == "solar_system_web") {
    // Real-time applications need fast performance
    return DefaultBodySet::ESSENTIAL;
  } else if (application_name == "solar_system_launcher") {
    // Launcher should use balanced set for demonstrations
    return DefaultBodySet::IMPORTANT;
  } else if (application_name == "solar_system") {
    // Main simulation application can handle complete set
    return DefaultBodySet::COMPLETE;
  } else {
    // Default to balanced set for unknown applications
    return DefaultBodySet::IMPORTANT;
  }
}

}  // namespace SolarSystem::Bodies
