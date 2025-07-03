#pragma once

#include <unordered_map>

#include "solar_core/bodies/celestial_body.hpp"

namespace SolarSystem::Bodies {

/**
 * @brief JPL ID to BodyType mapping
 * Centralized mapping used throughout the system
 */
inline const std::unordered_map<int, BodyType> JPL_ID_TO_TYPE = {
    // Stars
    {10, BodyType::Star},  // Sun

    // Planets
    {199, BodyType::Planet},  // Mercury
    {299, BodyType::Planet},  // Venus
    {399, BodyType::Planet},  // Earth
    {499, BodyType::Planet},  // Mars
    {599, BodyType::Planet},  // Jupiter
    {699, BodyType::Planet},  // Saturn
    {799, BodyType::Planet},  // Uranus
    {899, BodyType::Planet},  // Neptune

    // Moons
    {301, BodyType::Moon},  // Moon
    {501, BodyType::Moon},  // Io
    {502, BodyType::Moon},  // Europa
    {503, BodyType::Moon},  // Ganymede
    {504, BodyType::Moon},  // Callisto
    {606, BodyType::Moon},  // Titan
    {605, BodyType::Moon},  // Rhea
    {608, BodyType::Moon},  // Iapetus
    {703, BodyType::Moon},  // Titania
    {704, BodyType::Moon},  // Oberon
    {801, BodyType::Moon},  // Triton
    {901, BodyType::Moon},  // Charon

    // Dwarf Planets
    {999, BodyType::DwarfPlanet},     // Pluto
    {50000, BodyType::DwarfPlanet},   // Quaoar
    {136108, BodyType::DwarfPlanet},  // Haumea
    {136199, BodyType::DwarfPlanet},  // Eris

    // Spacecraft
    {-98, BodyType::Spacecraft},      // New Horizons
    {-143205, BodyType::Spacecraft},  // SpaceX Roadster
};

/**
 * @brief Get BodyType for JPL ID
 */
[[nodiscard]] inline BodyType get_body_type_for_jpl_id(int jpl_id) noexcept {
  auto it = JPL_ID_TO_TYPE.find(jpl_id);
  return (it != JPL_ID_TO_TYPE.end()) ? it->second : BodyType::Planet;  // Default fallback
}

/**
 * @brief BodyType to BodyPriority mapping
 * Defines default priority levels for each body type
 */
inline const std::unordered_map<BodyType, BodyPriority> TYPE_TO_PRIORITY = {
    {BodyType::Star, BodyPriority::Essential},         // Sun - always needed
    {BodyType::Planet, BodyPriority::Essential},       // Planets - always needed
    {BodyType::Moon, BodyPriority::Important},         // Moons - usually needed
    {BodyType::DwarfPlanet, BodyPriority::Important},  // Dwarf planets - usually needed
    {BodyType::Asteroid, BodyPriority::Optional},      // Asteroids - optional
    {BodyType::Spacecraft, BodyPriority::Optional},    // Spacecraft - optional (historical issues)
    {BodyType::Unknown, BodyPriority::Optional}        // Unknown objects - optional
};

/**
 * @brief Get BodyPriority for BodyType
 */
[[nodiscard]] inline BodyPriority get_priority_for_body_type(BodyType type) noexcept {
  auto it = TYPE_TO_PRIORITY.find(type);
  return (it != TYPE_TO_PRIORITY.end()) ? it->second : BodyPriority::Important;  // Default fallback
}

}  // namespace SolarSystem::Bodies
