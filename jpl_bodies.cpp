#include "jpl_bodies.h"
#include "constants.h"

// Body ID lookup table - must match the exact order in your SolarSystem array
const JPLBodyInfo JPL_BODY_MAP[] = {
    {10,       "Sun",           "Sun",                    true},
    {199,      "Mercury",       "Mercury",                true},
    {299,      "Venus",         "Venus",                  true},
    {399,      "Earth",         "Earth",                  true},
    {301,      "Moon",          "Moon",                   true},
    {499,      "Mars",          "Mars",                   true},
    {599,      "Jupiter",       "Jupiter",                true},
    {501,      "Io/JI",         "Io",                     false},
    {502,      "Europa/JII",    "Europa",                 false},
    {503,      "Ganymede/JIII", "Ganymede",               false},
    {504,      "Callisto/JIV",  "Callisto",               false},
    {699,      "Saturn",        "Saturn",                 true},
    {606,      "Titan",         "Titan",                  false},
    {605,      "Rhea",          "Rhea",                   false},
    {608,      "Iapetus",       "Iapetus",                false},
    {799,      "Uranus",        "Uranus",                 true},
    {703,      "Titania",       "Titania",                false},
    {704,      "Oberon",        "Oberon",                 false},
    {899,      "Neptune",       "Neptune",                true},
    {801,      "Triton",        "Triton",                 false},
    {999,      "Pluto",         "Pluto",                  true},
    {901,      "Charon",        "Charon",                 false},
    {2050000,  "Quaoar",        "50000 Quaoar",           false},
    {2136108,  "Haumea",        "136108 Haumea",          false},
    {2136199,  "Eris",          "136199 Eris",            false},
    {-98,      "New Horizons",  "New Horizons",           false},
    {-143205,  "SpaceX Roadster", "SpaceX Roadster",      false}
};

const int JPL_BODY_COUNT = sizeof(JPL_BODY_MAP) / sizeof(JPL_BODY_MAP[0]);

int get_jpl_id_by_index(int body_index) {
    if (body_index < 0 || body_index >= count || body_index >= JPL_BODY_COUNT) {
        return -1;  // Invalid index
    }
    return JPL_BODY_MAP[body_index].horizons_id;
}

const char* get_jpl_name_by_index(int body_index) {
    if (body_index < 0 || body_index >= count || body_index >= JPL_BODY_COUNT) {
        return "Unknown";
    }
    return JPL_BODY_MAP[body_index].horizons_name;
}

bool validate_all_jpl_bodies() {
    // Check that we have JPL mapping for all bodies in our simulation
    if (JPL_BODY_COUNT != count) {
        return false;  // Mismatch in body count
    }
    
    // All JPL IDs should be valid (non-zero)
    for (int i = 0; i < count; i++) {
        if (JPL_BODY_MAP[i].horizons_id == 0) {
            return false;
        }
    }
    
    return true;
}
