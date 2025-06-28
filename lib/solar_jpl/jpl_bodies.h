#ifndef __JPL_BODIES_H__
#define __JPL_BODIES_H__

// JPL HORIZONS body ID mapping
// Reference: https://ssd.jpl.nasa.gov/horizons/app.html#/
struct JPLBodyInfo {
  int horizons_id;
  const char* name;
  const char* horizons_name;  // Exact name used in JPL system
  bool is_major_body;         // Major planets vs small bodies
};

// Body ID lookup table - matching your existing body order
extern const JPLBodyInfo JPL_BODY_MAP[];
extern const int JPL_BODY_COUNT;

// Function to get JPL ID for a body by index (matching SolarSystem array)
int get_jpl_id_by_index(int body_index);
int get_jpl_id_for_body(int body_index);  // Alias for compatibility

// Function to get body name for validation
const char* get_jpl_name_by_index(int body_index);

// Function to validate all bodies exist in JPL system
bool validate_all_jpl_bodies();

#endif  //__JPL_BODIES_H__
