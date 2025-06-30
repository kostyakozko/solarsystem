#include "jpl_bodies.h"

#include "constants.h"
#include "jpl_data.h"

// Body ID lookup table - must match the exact order in your SolarSystem array
const JPLBodyInfo JPL_BODY_MAP[] = {
    {10, "Sun", "Sun", true, BODY_ESSENTIAL},
    {199, "Mercury", "Mercury", true, BODY_ESSENTIAL},
    {299, "Venus", "Venus", true, BODY_ESSENTIAL},
    {399, "Earth", "Earth", true, BODY_ESSENTIAL},
    {301, "Moon", "Moon", true, BODY_IMPORTANT},
    {499, "Mars", "Mars", true, BODY_ESSENTIAL},
    {599, "Jupiter", "Jupiter", true, BODY_ESSENTIAL},
    {501, "Io/JI", "Io", false, BODY_IMPORTANT},
    {502, "Europa/JII", "Europa", false, BODY_IMPORTANT},
    {503, "Ganymede/JIII", "Ganymede", false, BODY_IMPORTANT},
    {504, "Callisto/JIV", "Callisto", false, BODY_IMPORTANT},
    {699, "Saturn", "Saturn", true, BODY_ESSENTIAL},
    {606, "Titan", "Titan", false, BODY_IMPORTANT},
    {605, "Rhea", "Rhea", false, BODY_IMPORTANT},
    {608, "Iapetus", "Iapetus", false, BODY_IMPORTANT},
    {799, "Uranus", "Uranus", true, BODY_ESSENTIAL},
    {703, "Titania", "Titania", false, BODY_IMPORTANT},
    {704, "Oberon", "Oberon", false, BODY_IMPORTANT},
    {899, "Neptune", "Neptune", true, BODY_ESSENTIAL},
    {801, "Triton", "Triton", false, BODY_IMPORTANT},
    {999, "Pluto", "Pluto", true, BODY_IMPORTANT},  // Dwarf planet - important but not essential
    {901, "Charon", "Charon", false, BODY_IMPORTANT},
    {50000, "Quaoar", "50000 Quaoar", false, BODY_IMPORTANT},
    {136108, "Haumea", "136108 Haumea", false, BODY_IMPORTANT},
    {136199, "Eris", "136199 Eris", false, BODY_IMPORTANT},
    {-98, "New Horizons", "New Horizons", false,
     BODY_OPTIONAL},  // Spacecraft - can fail for historical dates
    {-143205, "SpaceX Roadster", "SpaceX Roadster", false,
     BODY_OPTIONAL}  // Spacecraft - can fail for historical dates
};

const int JPL_BODY_COUNT = sizeof(JPL_BODY_MAP) / sizeof(JPL_BODY_MAP[0]);

int get_jpl_id_by_index(int body_index) {
  if (body_index < 0 || body_index >= BODY_COUNT) {
    return 0;  // Invalid index, return 0 to skip
  }
  return JPL_BODY_MAP[body_index].horizons_id;
}

int get_jpl_id_for_body(int body_index) {
  return get_jpl_id_by_index(body_index);  // Alias for compatibility
}

const char* get_jpl_name_by_index(int body_index) {
  if (body_index < 0 || body_index >= BODY_COUNT) {
    return "Unknown";
  }
  return JPL_BODY_MAP[body_index].horizons_name;
}

bool validate_all_jpl_bodies() {
  // Basic validation - ensure we have the right number of bodies
  return JPL_BODY_COUNT == BODY_COUNT;
}
