#include "solar_core/bodies/body_factory.hpp"

#include "solar_core/data/body_definitions.hpp"

// Include existing JPL system
#include <algorithm>
#include <sstream>

#include "jpl_bodies.h"
#include "jpl_data.h"

namespace SolarSystem::Bodies {

BodyFactory::BodyFactory(CreationOptions options) : default_options_(std::move(options)) {
  // Only initialize JPL data system if we're going to use JPL sources
  if (default_options_.preferred_source == DataSource::JPL_HORIZONS ||
      default_options_.preferred_source == DataSource::CACHED_DATA) {
    initialize_jpl_data();
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
  // For now, just delegate to fallback data to avoid namespace conflicts
  // TODO: Implement proper legacy data integration later
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
  // Convert time to date string for JPL system
  auto time_t_val = std::chrono::system_clock::to_time_t(time);
  auto* tm_val = std::gmtime(&time_t_val);

  char date_str[32];
  std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_val);

  // Try to fetch JPL data for this date
  if (!fetch_jpl_data_for_date(date_str)) {
    return Utils::Expected<CelestialBody, std::string>{"Failed to fetch JPL data for date " +
                                                       std::string(date_str)};
  }

  // For now, fall back to our modern data since legacy integration is complex
  // TODO: Implement proper JPL data integration later
  return create_from_fallback(name);
}

Utils::Expected<CelestialBody, std::string> BodyFactory::create_from_cache(
    std::string_view name, std::chrono::system_clock::time_point time) const {
  // Check if we have current ephemeris data
  if (!has_current_ephemeris_data()) {
    return Utils::Expected<CelestialBody, std::string>{"No cached ephemeris data available"};
  }

  // For now, fall back to our modern data since legacy integration is complex
  // TODO: Implement proper cached data integration later
  return create_from_fallback(name);
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
  // Map to our modern enum based on name patterns
  if (name == "Sun") return BodyType::Star;
  if (name.find("Moon") != std::string_view::npos || name == "Io" || name == "Europa" ||
      name == "Ganymede" || name == "Callisto" || name == "Titan" || name == "Rhea" ||
      name == "Iapetus" || name == "Titania" || name == "Oberon" || name == "Triton" ||
      name == "Charon") {
    return BodyType::Moon;
  }
  if (name == "Mercury" || name == "Venus" || name == "Earth" || name == "Mars" ||
      name == "Jupiter" || name == "Saturn" || name == "Uranus" || name == "Neptune") {
    return BodyType::Planet;
  }
  if (name == "Pluto" || name == "Quaoar" || name == "Haumea" || name == "Eris") {
    return BodyType::DwarfPlanet;
  }
  if (name.find("Horizons") != std::string_view::npos ||
      name.find("Roadster") != std::string_view::npos) {
    return BodyType::Spacecraft;
  }

  return BodyType::Planet;  // Default
}

BodyPriority BodyFactory::determine_body_priority(std::string_view name) const {
  // Essential: Sun and major planets
  if (name == "Sun" || name == "Mercury" || name == "Venus" || name == "Earth" || name == "Mars" ||
      name == "Jupiter" || name == "Saturn" || name == "Uranus" || name == "Neptune") {
    return BodyPriority::Essential;
  }

  // Important: Major moons and dwarf planets
  if (name == "Moon" || name == "Io" || name == "Europa" || name == "Ganymede" ||
      name == "Callisto" || name == "Titan" || name == "Pluto") {
    return BodyPriority::Important;
  }

  // Optional: Everything else
  return BodyPriority::Optional;
}

std::optional<std::string> BodyFactory::get_jpl_id(std::string_view name) const {
  // Find in JPL body map
  for (int i = 0; i < JPL_BODY_COUNT; ++i) {
    if (name == JPL_BODY_MAP[i].name) {
      return std::to_string(JPL_BODY_MAP[i].horizons_id);
    }
  }
  return std::nullopt;
}

// Helper function to convert legacy BodyType to modern enum
BodyType BodyFactory::convert_legacy_body_type(int legacy_type) const {
  switch (legacy_type) {
    case BODY_ESSENTIAL:
      return BodyType::Planet;  // Most essential bodies are planets
    case BODY_IMPORTANT:
      return BodyType::Moon;  // Most important bodies are moons
    case BODY_OPTIONAL:
      return BodyType::Spacecraft;  // Most optional bodies are spacecraft
    case BODY_UNKNOWN:
    default:
      return BodyType::Planet;  // Safe default
  }
}

}  // namespace SolarSystem::Bodies
