#include "solar_core/data/body_definitions.hpp"

#include <algorithm>

namespace SolarSystem::Data {

Bodies::CelestialBody BodyDefinition::to_celestial_body() const {
  Bodies::CelestialBody::Properties props{.name = std::string(name),
                                          .mass = mass,
                                          .position = position,
                                          .velocity = velocity,
                                          .type = type,
                                          .priority = priority,
                                          .jpl_id = std::string(jpl_id)};

  // Set creation dates for spacecraft for historical accuracy
  if (type == Bodies::BodyType::Spacecraft) {
    if (name == "New Horizons") {
      // Launched January 19, 2006
      auto launch_time = std::chrono::system_clock::from_time_t(1137628800);  // 2006-01-19
      props.creation_date = launch_time;
    } else if (name == "SpaceX Roadster") {
      // Launched February 6, 2018
      auto launch_time = std::chrono::system_clock::from_time_t(1517875200);  // 2018-02-06
      props.creation_date = launch_time;
    }
  }

  return Bodies::CelestialBody{props};
}

std::optional<BodyDefinition> get_fallback_body(std::string_view name) {
  auto it = std::find_if(FALLBACK_SOLAR_SYSTEM.begin(), FALLBACK_SOLAR_SYSTEM.end(),
                         [name](const BodyDefinition& body) { return body.name == name; });

  if (it != FALLBACK_SOLAR_SYSTEM.end()) {
    return *it;
  }
  return std::nullopt;
}

std::vector<BodyDefinition> get_essential_bodies() {
  return get_bodies_by_priority(Bodies::BodyPriority::Essential);
}

std::vector<BodyDefinition> get_bodies_by_type(Bodies::BodyType type) {
  std::vector<BodyDefinition> result;
  std::copy_if(FALLBACK_SOLAR_SYSTEM.begin(), FALLBACK_SOLAR_SYSTEM.end(),
               std::back_inserter(result),
               [type](const BodyDefinition& body) { return body.type == type; });
  return result;
}

std::vector<BodyDefinition> get_bodies_by_priority(Bodies::BodyPriority priority) {
  std::vector<BodyDefinition> result;
  std::copy_if(FALLBACK_SOLAR_SYSTEM.begin(), FALLBACK_SOLAR_SYSTEM.end(),
               std::back_inserter(result),
               [priority](const BodyDefinition& body) { return body.priority == priority; });
  return result;
}

}  // namespace SolarSystem::Data
