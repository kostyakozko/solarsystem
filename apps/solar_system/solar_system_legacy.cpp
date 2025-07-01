/**
 * @file solar_system_legacy.cpp
 * @brief Legacy C Solar System simulation - Original
 * Implementation
 * 
 * This is the original C-based implementation kept for reference and
 * comparison.
 * The main production application is now solar_system.cpp which uses modern C++
 *
 * architecture while maintaining identical behavior and output format.
 * 
 * This legacy version
 * demonstrates the transformation from procedural C code
 * to modern C++ object-oriented design
 * with significant performance improvements.
 */

#include <ctime>
#include <iomanip>
#include <iostream>

#include "args.h"
#include "constants.h"
#include "jpl_data.h"
#include "model.h"
#include "simulation.h"
#include "types.h"

int main(int argc, char* argv[]) {
  // Parse command line arguments
  SimulationArgs args = parse_arguments(argc, argv);

  // Set output precision
  std::cout.precision(12);

  // Initialize JPL data system
  initialize_jpl_data();

  // Handle JPL data operations
  if (args.update_data) {
    if (update_ephemeris_data()) {
      std::cout << "Ephemeris data updated successfully" << std::endl;
    } else {
      std::cerr << "Failed to update ephemeris data" << std::endl;
      return 1;
    }
    return 0;  // Exit after update
  }

  if (args.rebuild_cache) {
    if (rebuild_binary_cache()) {
      std::cout << "Binary cache rebuilt successfully" << std::endl;
    } else {
      std::cerr << "Failed to rebuild binary cache" << std::endl;
      return 1;
    }
    return 0;  // Exit after rebuild
  }

  if (args.test_storage) {
    if (save_current_data_for_testing()) {
      std::cout << "Storage system test completed successfully" << std::endl;
    } else {
      std::cerr << "Storage system test failed" << std::endl;
      return 1;
    }
    return 0;  // Exit after test
  }

  // Determine starting date based on available ephemeris data
  time_t start_date;
  if (has_current_ephemeris_data()) {
    // Use JPL data epoch as starting point
    start_date = get_ephemeris_epoch();
    std::cout << "Using JPL ephemeris data: " << get_ephemeris_source() << std::endl;
  } else {
    // Fallback to original hardcoded date
    struct tm start_timeinfo = {};
    start_timeinfo.tm_sec = 0;
    start_timeinfo.tm_min = 0;
    start_timeinfo.tm_hour = 0;
    start_timeinfo.tm_mday = 11;
    start_timeinfo.tm_mon = 1;
    start_timeinfo.tm_year = 2018 - 1900;
    start_timeinfo.tm_isdst = -1;  // Let system determine DST
    start_date = mktime(&start_timeinfo);
    std::cout << "Using original ephemeris data: " << get_ephemeris_source() << std::endl;
  }

  // Show ephemeris data status
  std::cout << "Ephemeris epoch: " << ctime(&start_date);

  if (!has_current_year_ephemeris_data()) {
    std::cout << "Note: Consider updating ephemeris data with -u for current year" << std::endl;
  }

  // Print simulation information
  print_simulation_info(args, start_date);

  // Run appropriate simulation based on date direction
  if (args.target_date < start_date) {
    run_backward_simulation(start_date, args.target_date);
  } else {
    run_forward_simulation(start_date, args.target_date);
  }

  return 0;
}
