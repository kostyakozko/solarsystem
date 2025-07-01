#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/constants.hpp"

namespace SolarSystem::Data {

/**
 * @brief Modern replacement for constants.cpp hardcoded data
 *
 * Provides fallback celestial body data when JPL HORIZONS is unavailable.
 * This data serves as a backup and for offline operation.
 */

struct BodyDefinition {
  std::string_view name;
  double mass;              // kg
  Math::Vector3d position;  // meters (J2000 ecliptic)
  Math::Vector3d velocity;  // m/s (J2000 ecliptic)
  Bodies::BodyType type;
  Bodies::BodyPriority priority;
  std::string_view jpl_id;

  // Convert to modern CelestialBody
  [[nodiscard]] Bodies::CelestialBody to_celestial_body() const;
};

/**
 * @brief Fallback solar system data (equivalent to old constants.cpp)
 *
 * This data is used when JPL HORIZONS is unavailable or for testing.
 * Positions and velocities are approximate values for a reference epoch.
 */
constexpr std::array<BodyDefinition, 27> FALLBACK_SOLAR_SYSTEM = {
    {// Sun
     {.name = "Sun",
      .mass = 1.98847e30,
      .position = {2.328883776117662e8, 9.469803236734810e8, -1.709434366728627e7},
      .velocity = {-1.063013245364047e1, 8.172330033873488, 2.552304032541071e-1},
      .type = Bodies::BodyType::Star,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "10"},

     // Inner Planets
     {.name = "Mercury",
      .mass = 3.30104e23,
      .position = {3.734433664391050e10, -5.097307167786794e10, -7.664223240497768e9},
      .velocity = {2.990912540620261e4, 3.070765623019404e4, -2.360226338489824e2},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "199"},

     {.name = "Venus",
      .mass = 4.86732e24,
      .position = {1.026512847456508e11, -3.535504613638300e10, -6.425264817168800e9},
      .velocity = {1.149840617938256e4, 3.286370322494047e4, -2.131977283742419e2},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "299"},

     {.name = "Earth",
      .mass = 5.97219e24,
      .position = {-1.160377810399021e11, 9.194319994670375e10, -2.079208395353705e7},
      .velocity = {-1.886526891120289e4, -2.356160384405198e4, 2.068517017564275},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "399"},

     {.name = "Mars",
      .mass = 6.41693e23,
      .position = {-1.980529538506177e11, -1.316265300753012e11, 1.999310309506287e9},
      .velocity = {1.439273906982806e4, -1.805004074562709e4, -7.309579840728315e2},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "499"},

     // Earth's Moon
     {.name = "Moon",
      .mass = 7.342e22,
      .position = {-1.156637810399021e11, 9.230319994670375e10, -2.079208395353705e7},
      .velocity = {-1.886526891120289e4, -2.356160384405198e4, 2.068517017564275},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "301"},

     // Gas Giants
     {.name = "Jupiter",
      .mass = 1.89813e27,
      .position = {5.989091595721393e11, 4.391225931434156e11, -1.523227264560251e10},
      .velocity = {-7.901937797405488e3, 1.116317697140190e4, 1.306729060094249e2},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "599"},

     {.name = "Saturn",
      .mass = 5.68319e26,
      .position = {9.587063371179731e11, 9.825652108563659e11, -5.522136347173799e10},
      .velocity = {-7.429475412158143e3, 6.738522586094912e3, 1.775951263495906e2},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "699"},

     {.name = "Uranus",
      .mass = 8.68103e25,
      .position = {2.158774703477780e12, -2.054825231253915e12, -3.562157203239586e10},
      .velocity = {4.637622247366138e3, 4.627225806404547e3, -4.295689514239251e1},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "799"},

     {.name = "Neptune",
      .mass = 1.02410e26,
      .position = {2.514853477780e12, -3.738825231253915e12, 1.937842796760414e10},
      .velocity = {4.474622247366138e3, 3.067225806404547e3, -1.675689514239251e2},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "899"},

     // Major Moons
     {.name = "Io",
      .mass = 8.9319e22,
      .position = {5.989091595721393e11 + 4.217e8, 4.391225931434156e11, -1.523227264560251e10},
      .velocity = {-7.901937797405488e3, 1.116317697140190e4 + 1.73e4, 1.306729060094249e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "501"},

     {.name = "Europa",
      .mass = 4.7998e22,
      .position = {5.989091595721393e11 + 6.709e8, 4.391225931434156e11, -1.523227264560251e10},
      .velocity = {-7.901937797405488e3, 1.116317697140190e4 + 1.37e4, 1.306729060094249e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "502"},

     {.name = "Ganymede",
      .mass = 1.4819e23,
      .position = {5.989091595721393e11 + 1.070e9, 4.391225931434156e11, -1.523227264560251e10},
      .velocity = {-7.901937797405488e3, 1.116317697140190e4 + 1.09e4, 1.306729060094249e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "503"},

     {.name = "Callisto",
      .mass = 1.0759e23,
      .position = {5.989091595721393e11 + 1.883e9, 4.391225931434156e11, -1.523227264560251e10},
      .velocity = {-7.901937797405488e3, 1.116317697140190e4 + 8.2e3, 1.306729060094249e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "504"},

     {.name = "Titan",
      .mass = 1.3452e23,
      .position = {9.587063371179731e11 + 1.222e9, 9.825652108563659e11, -5.522136347173799e10},
      .velocity = {-7.429475412158143e3, 6.738522586094912e3 + 5.57e3, 1.775951263495906e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "606"},

     {.name = "Rhea",
      .mass = 2.3065e21,
      .position = {9.587063371179731e11 + 5.271e8, 9.825652108563659e11, -5.522136347173799e10},
      .velocity = {-7.429475412158143e3, 6.738522586094912e3 + 8.48e3, 1.775951263495906e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "605"},

     {.name = "Iapetus",
      .mass = 1.8056e21,
      .position = {9.587063371179731e11 + 3.561e9, 9.825652108563659e11, -5.522136347173799e10},
      .velocity = {-7.429475412158143e3, 6.738522586094912e3 + 3.26e3, 1.775951263495906e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "608"},

     {.name = "Titania",
      .mass = 3.527e21,
      .position = {2.158774703477780e12 + 4.358e8, -2.054825231253915e12, -3.562157203239586e10},
      .velocity = {4.637622247366138e3, 4.627225806404547e3 + 3.64e3, -4.295689514239251e1},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "703"},

     {.name = "Oberon",
      .mass = 3.014e21,
      .position = {2.158774703477780e12 + 5.835e8, -2.054825231253915e12, -3.562157203239586e10},
      .velocity = {4.637622247366138e3, 4.627225806404547e3 + 3.15e3, -4.295689514239251e1},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "704"},

     {.name = "Triton",
      .mass = 2.139e22,
      .position = {2.514853477780e12 + 3.548e8, -3.738825231253915e12, 1.937842796760414e10},
      .velocity = {4.474622247366138e3, 3.067225806404547e3 - 4.39e3, -1.675689514239251e2},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "801"},

     // Pluto System
     {.name = "Pluto",
      .mass = 1.307e22,
      .position = {1.928374703477780e12, -4.738825231253915e12, 8.637842796760414e8},
      .velocity = {5.174622247366138e3, 2.267225806404547e3, -8.675689514239251e1},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "999"},

     {.name = "Charon",
      .mass = 1.586e21,
      .position = {1.928374703477780e12 + 1.96e7, -4.738825231253915e12, 8.637842796760414e8},
      .velocity = {5.174622247366138e3, 2.267225806404547e3 + 2.1e2, -8.675689514239251e1},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "901"},

     // Dwarf Planets
     {.name = "Quaoar",
      .mass = 1.4e21,
      .position = {6.4e12, 2.1e12, -1.5e11},
      .velocity = {-1.5e3, 4.2e3, 2.1e2},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "2050000"},

     {.name = "Haumea",
      .mass = 4.006e21,
      .position = {7.2e12, -1.8e12, 3.2e11},
      .velocity = {1.2e3, 3.8e3, -1.8e2},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "2136108"},

     {.name = "Eris",
      .mass = 1.66e22,
      .position = {1.4e13, 3.8e12, -4.2e11},
      .velocity = {-8.5e2, 1.2e3, 4.2e1},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "2136199"},

     // Spacecraft (with creation dates for historical accuracy)
     {.name = "New Horizons",
      .mass = 478.0,
      .position = {6.5e12, -2.1e12, 8.4e11},
      .velocity = {1.4e4, 8.2e3, -2.1e3},
      .type = Bodies::BodyType::Spacecraft,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "-98"},

     {.name = "SpaceX Roadster",
      .mass = 1350.0,
      .position = {2.1e11, 1.8e11, -1.2e9},
      .velocity = {-1.8e4, 2.1e4, 4.2e2},
      .type = Bodies::BodyType::Spacecraft,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "-143205"}}};

/**
 * @brief Get fallback body definition by name
 */
[[nodiscard]] std::optional<BodyDefinition> get_fallback_body(std::string_view name);

/**
 * @brief Get all essential bodies from fallback data
 */
[[nodiscard]] std::vector<BodyDefinition> get_essential_bodies();

/**
 * @brief Get ALL bodies from fallback data (all 27 bodies)
 */
[[nodiscard]] std::vector<BodyDefinition> get_all_body_definitions();

/**
 * @brief Get all bodies of a specific type from fallback data
 */
[[nodiscard]] std::vector<BodyDefinition> get_bodies_by_type(Bodies::BodyType type);

/**
 * @brief Get all bodies of a specific priority from fallback data
 */
[[nodiscard]] std::vector<BodyDefinition> get_bodies_by_priority(Bodies::BodyPriority priority);

}  // namespace SolarSystem::Data
