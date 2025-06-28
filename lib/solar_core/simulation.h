#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include <ctime>

#include "args.h"

// Print simulation information
void print_simulation_info(const SimulationArgs& args, time_t start_date);

// Run forward simulation from start_date to target_date
void run_forward_simulation(time_t start_date, time_t target_date);

// Run backward simulation from start_date to target_date
void run_backward_simulation(time_t start_date, time_t target_date);

// Perform one simulation step (physics calculations)
void perform_simulation_step(long double time_step);

// Real-time simulation functions
void initialize_simulation_to_current_time();
void update_simulation_to_current_time();
time_t get_simulation_time();

#endif  //__SIMULATION_H__
