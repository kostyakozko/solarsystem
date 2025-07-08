#include "simulation.h"

#include <iostream>

// Legacy C functions - minimal stubs for compatibility
// These are not used by the modern C++ applications

void print_simulation_info_legacy(time_t start_date, time_t target_date) {
  std::cout << "Solar System Simulation" << std::endl;
  std::cout << "Start date: " << ctime(&start_date);
  std::cout << "Target date: " << ctime(&target_date);
}

void run_forward_simulation(time_t start_date, time_t target_date) {
  // Legacy stub - not implemented
  std::cerr << "Legacy simulation functions are deprecated. Use modern C++ simulation engine."
            << std::endl;
}

void run_backward_simulation(time_t start_date, time_t target_date) {
  // Legacy stub - not implemented
  std::cerr << "Legacy simulation functions are deprecated. Use modern C++ simulation engine."
            << std::endl;
}

bool run_web_forward_simulation(time_t start_date, time_t target_date) {
  // Legacy stub - not implemented
  std::cerr << "Legacy simulation functions are deprecated. Use modern C++ simulation engine."
            << std::endl;
  return false;
}

bool run_web_backward_simulation(time_t start_date, time_t target_date) {
  // Legacy stub - not implemented
  std::cerr << "Legacy simulation functions are deprecated. Use modern C++ simulation engine."
            << std::endl;
  return false;
}

void update_simulation_to_current_time() {
  // Legacy stub - not implemented
  std::cerr << "Legacy simulation functions are deprecated. Use modern C++ simulation engine."
            << std::endl;
}
