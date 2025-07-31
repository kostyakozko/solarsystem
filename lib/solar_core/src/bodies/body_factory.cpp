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

  // Try preferred source first
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

  for (const auto& name : body_names) {
    auto body_result = create_body(name, options);
    if (body_result.has_value()) {
      try {
        collection.add_body(std::move(body_result.value()));
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
  return create_solar_system(options);  // Same as solar system for now
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_from_legacy_data(
    std::string_view name) const {
  // First try to use cached ephemeris data if available
  if (current_source_ != "ORIGINAL_DATA" && !cached_ephemeris_.empty()) {
    for (const auto& data : cached_ephemeris_) {
      if (data.body_name == name) {
        return Utils::Expected<CelestialBody, std::string>{data.to_celestial_body()};
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
  return Utils::Expected<CelestialBody, std::string>{ephemeris_data.to_celestial_body()};
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
    return Utils::Expected<CelestialBody, std::string>{fallback_body->to_celestial_body()};
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
      int jpl_id;
      auto sv = Data::FALLBACK_SOLAR_SYSTEM[i].jpl_id;
      auto result = std::from_chars(sv.data(), sv.data() + sv.size(), jpl_id);
      if (result.ec == std::errc::invalid_argument) {
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

  // Update internal state
  cached_ephemeris_ = SolarSystem::JPL::get_value(result);
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

}  // namespace SolarSystem::Bodies
