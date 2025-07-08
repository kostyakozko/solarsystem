#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include <ctime>

// Legacy C functions - these are kept for compatibility with existing C code
// but should not be used in new C++ code

// Print simulation information
void print_simulation_info_legacy(time_t start_date, time_t target_date);

// Run forward simulation from start_date to target_date (for command-line tools)
void run_forward_simulation(time_t start_date, time_t target_date);

// Run backward simulation from start_date to target_date (for command-line tools)
void run_backward_simulation(time_t start_date, time_t target_date);

// Web server specific simulation functions (non-blocking, no exit)
bool run_web_forward_simulation(time_t start_date, time_t target_date);
bool run_web_backward_simulation(time_t start_date, time_t target_date);

// Update simulation to current time (legacy stub)
void update_simulation_to_current_time();

// Perform one simulation step (physics calculations)
void perform_simulation_step(long double time_step);

// Real-time simulation functions
void initialize_simulation_to_current_time();
void update_simulation_to_current_time();
time_t get_simulation_time();

#endif  //__SIMULATION_H__
