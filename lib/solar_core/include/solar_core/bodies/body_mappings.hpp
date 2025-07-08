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
 * @brief Body name to JPL ID mapping
 * Centralized mapping for all supported celestial bodies
 */
inline const std::unordered_map<std::string, int> BODY_NAME_TO_JPL_ID = {
    // Stars
    {"Sun", 10},

    // Planets
    {"Mercury", 199},
    {"Venus", 299},
    {"Earth", 399},
    {"Mars", 499},
    {"Jupiter", 599},
    {"Saturn", 699},
    {"Uranus", 799},
    {"Neptune", 899},

    // Moons
    {"Moon", 301},
    {"Io", 501},
    {"Europa", 502},
    {"Ganymede", 503},
    {"Callisto", 504},
    {"Titan", 606},
    {"Rhea", 605},
    {"Iapetus", 608},
    {"Titania", 703},
    {"Oberon", 704},
    {"Triton", 801},
    {"Charon", 901},

    // Dwarf Planets
    {"Pluto", 999},
    {"Quaoar", 50000},
    {"Haumea", 136108},
    {"Eris", 136199},

    // Spacecraft
    {"New Horizons", -98},
    {"SpaceX Roadster", -143205},

    // Alternative names for compatibility
    {"Io/JI", 501},
    {"Europa/JII", 502},
    {"Ganymede/JIII", 503},
    {"Callisto/JIV", 504},
};

/**
 * @brief Get JPL ID for body name
 */
[[nodiscard]] inline std::optional<int> get_jpl_id_for_body_name(std::string_view name) noexcept {
  auto it = BODY_NAME_TO_JPL_ID.find(std::string(name));
  return (it != BODY_NAME_TO_JPL_ID.end()) ? std::make_optional(it->second) : std::nullopt;
}

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
