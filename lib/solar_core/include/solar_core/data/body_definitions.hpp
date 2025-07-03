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
constexpr int BODY_COUNT = 27;  // Need better approach, but good for now
constexpr std::array<BodyDefinition, BODY_COUNT> FALLBACK_SOLAR_SYSTEM = {
    {// Sun
     {.name = "Sun",
      .mass = 1.9885E30,
      .position = {2.328883776117662E+08, 9.469803236734810E+08, -1.709434366728627E+07},
      .velocity = {-1.063013245364047E+01, 8.172330033873488, 2.552304032541071E-01},
      .type = Bodies::BodyType::Star,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "10"},

     // Inner Planets
     {.name = "Mercury",
      .mass = 3.3020E23,
      .position = {3.734433664391050E+10, -5.097307167786794E+10, -7.664223240497768E+09},
      .velocity = {2.990912540620261E+04, 3.070765623019404E+04, -2.360226338489824E+02},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "199"},

     {.name = "Venus",
      .mass = 4.8685E24,
      .position = {1.026512847456508E+11, -3.535504613638300E+10, -6.425264817168800E+09},
      .velocity = {1.149840617938256E+04, 3.286370322494047E+04, -2.131977283742419E+02},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "299"},

     {.name = "Earth",
      .mass = 5.9722E24,
      .position = {-1.160377810399021E+11, 9.194319994670375E+10, -2.079208395353705E+07},
      .velocity = {-1.886526891120289E+04, -2.356160384405198E+04, 2.068517017564275},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "399"},

     // Earth's Moon
     {.name = "Moon",
      .mass = 7.3490E22,
      .position = {-1.160477699219823E+11, 9.153871284686904E+10, 6.264042621906847E+06},
      .velocity = {-1.790099667658844E+04, -2.359686044260280E+04, -5.917605837625040E+01},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "301"},

     {.name = "Mars",
      .mass = 6.4185E23,
      .position = {-1.997158552109526E+11, -1.284001748932029E+11, 2.179584290657185E+09},
      .velocity = {1.405732581815473E+04, -1.826487350913149E+04, -7.278868840699015E+02},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "499"},

     // Gas Giants
     {.name = "Jupiter",
      .mass = 1.8981E27,
      .position = {-6.081047685906553E+11, -5.361611236768045E+11, 1.582567053933117E+10},
      .velocity = {8.487735250388265E+03, -9.177396693450438E+03, -1.516657044377685E+02},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "599"},

     {.name = "Io/JI",
      .mass = 8.933000000000001e22,
      .position = {-6.084974965844570E+11, -5.360111750387562E+11, 1.582552548899150E+10},
      .velocity = {2.328291016125176E+03, -2.543081464677284E+04, -8.211168351407796E+02},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "501"},

     {.name = "Europa/JII",
      .mass = 4.797000000000000e22,
      .position = {-6.087813968408828E+11, -5.361324438212947E+11, 1.581409264479351E+10},
      .velocity = {7.883772836169378E+03, -2.276578831920047E+04, -7.451258380221084E+02},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "502"},

     {.name = "Ganymede/JIII",
      .mass = 1.4819e23,
      .position = {-6.082216753596034E+11, -5.350962496309689E+11, 1.586440200642738E+10},
      .velocity = {-2.312762729858076E+03, -1.034694085458423E+04, -3.394028524866970E+02},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "503"},

     {.name = "Callisto/JIV",
      .mass = 1.0759e23,
      .position = {-6.095083629252676E+11, -5.374307967735521E+11, 1.576675819592145E+10},
      .velocity = {1.398744019609489E+04, -1.520046486173449E+04, -2.686989877498887E+02},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "504"},

     {.name = "Saturn",
      .mass = 5.6832E26,
      .position = {3.949858217561845E+10, -1.504088201486316E+12, 2.457957210066664E+10},
      .velocity = {9.126752354196418E+03, 2.225161021121407E+02, -3.672361515973047E+02},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "699"},

     {.name = "Titan",
      .mass = 1.3455E23,
      .position = {3.929999839392310E+10, -1.502995922959354E+12, 2.403633694487488E+10},
      .velocity = {3.690030226225586E+03, -2.204268757260238E+02, 4.010626371976086E+02},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "606"},

     {.name = "Rhea",
      .mass = 2.3090E21,
      .position = {3.996385564836078E+10, -1.504321861525067E+12, 2.465856518385583E+10},
      .velocity = {1.304481971881133E+04, 6.756162931469384E+03, -4.118892445629723E+03},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "605"},

     {.name = "Iapetus",
      .mass = 1.8059E21,
      .position = {4.289600222327930E+10, -1.504313357998975E+12, 2.394502357974273E+10},
      .velocity = {9.178004838855442E+03, 3.487938513709992E+03, -1.133243907785824E+03},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "608"},

     {.name = "Uranus",
      .mass = 8.6810E25,
      .position = {2.640122441822190E+12, 1.376116834688191E+12, -2.909225616355687E+10},
      .velocity = {-3.197620376537337E+03, 5.721473008686046E+03, 6.279238980270430E+01},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "799"},

     {.name = "Titania",
      .mass = 3.5270E21,
      .position = {2.640344653102460E+12, 1.376017606903906E+12, -2.945289537567604E+10},
      .velocity = {-6.242670751319309E+03, 6.112273349811630E+03, -1.919409510456664E+03},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "703"},

     {.name = "Oberon",
      .mass = 3.0140E21,
      .position = {2.640442757626314E+12, 1.376116355970410E+12, -2.860545565141582E+10},
      .velocity = {-6.475965355525921E+02, 4.927750287955811E+03, -1.617677264266900E+03},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "704"},

     {.name = "Neptune",
      .mass = 1.0241E26,
      .position = {4.296067448041266E+12, -1.266723310574405E+12, -7.292141797366703E+10},
      .velocity = {1.501752843205070E+03, 5.245647173615618E+03, -1.432035598510835E+02},
      .type = Bodies::BodyType::Planet,
      .priority = Bodies::BodyPriority::Essential,
      .jpl_id = "899"},

     {.name = "Triton",
      .mass = 2.1470E22,
      .position = {4.295926641253536E+12, -1.267010246039275E+12, -7.307535410367203E+10},
      .velocity = {-1.869947682690540E+03, 5.394197840398287E+03, 2.664058980099843E+03},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "801"},

     // Pluto System
     {.name = "Pluto",
      .mass = 1.3070E22,
      .position = {1.631032961466709E+12, -4.738021981443667E+12, 3.520700820841193E+10},
      .velocity = {5.263494750365237E+03, 6.474560817503934E+02, -1.572669886592717E+03},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "999"},

     {.name = "Charon",
      .mass = 1.5300E21,
      .position = {1.631046050939787E+12, -4.738016060052414E+12, 3.519368340083623E+10},
      .velocity = {5.195127356962284E+03, 4.865648017516665E+02, -1.711358032334507E+03},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "901"},

     // Dwarf Planets
     {.name = "Quaoar",
      .mass = 1.4060E21,
      .position = {3.565307408086812E+10, -6.355839539598446E+12, 8.822165908103421E+11},
      .velocity = {4.568912422957285E+03, 1.995457796801816E+02, 7.169738510189458E+01},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "2050000"},

     {.name = "Haumea",
      .mass = 4.0060E21,
      .position = {-6.081493645174928E+12, -2.745113997616651E+12, 3.546526011225002E+12},
      .velocity = {1.949707122754813E+03, -3.280386606928030E+03, 4.361012604044090E+01},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "2136108"},

     {.name = "Eris",
      .mass = 1.6600E22,
      .position = {1.293238565418191E+13, 5.525047912929693E+12, -3.017983941392108E+12},
      .velocity = {-6.717235853970278E+02, 1.549067893538111E+03, 1.591252994469163E+03},
      .type = Bodies::BodyType::DwarfPlanet,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "2136199"},

     // Spacecraft (with creation dates for historical accuracy)
     {.name = "New Horizons",
      .mass = 465,
      .position = {1.648271605409920E+12, -5.848949558074337E+12, 2.064169308072057E+11},
      .velocity = {5.495070537900303E+03, -1.305799494291895E+04, 5.114585657450750E+02},
      .type = Bodies::BodyType::Spacecraft,
      .priority = Bodies::BodyPriority::Optional,
      .jpl_id = "-98"},

     {.name = "SpaceX Roadster",
      .mass = 1250,
      .position = {-1.169077428133156E+11, 9.105691074237506E+10, -2.645012828238495E+08},
      .velocity = {-2.125352869123654E+04, -2.607176140206951E+04, -6.453526951389765E+02},
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
