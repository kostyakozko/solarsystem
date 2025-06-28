#include "simulation.h"
#include "types.h"
#include "constants.h"
#include "model.h"
#include <iostream>
#include <cstdlib>

void print_simulation_info(const SimulationArgs& args, time_t start_date) {
    std::cout << "Solar System Simulation" << std::endl;
    std::cout << "Start date: " << ctime(&start_date);
    
    if (args.use_current_date) {
        std::cout << "Target date: Current time (" << ctime(&args.target_date) << ")" << std::endl;
    } else {
        std::cout << "Target date: " << args.date_string << " (" << ctime(&args.target_date) << ")" << std::endl;
    }
}

void perform_simulation_step(long double time_step) {
    for (int i = 0; i < count; ++i) {
        acceleration delta{0.0, 0.0, 0.0};
        coord& this_position = SolarSystem[i].position;
        
        // Calculate gravitational forces
        for (int j = 0; j < i; ++j) {
            attractTo(this_position, delta, j);
        }
        for (int j = i + 1; j < count; ++j) {
            attractTo(this_position, delta, j);
        }
        
        // Apply time integration
        velocity& this_speed = SolarSystem[i].speed;
        this_speed.x += delta.x * time_step;
        this_speed.y += delta.y * time_step;
        this_speed.z += delta.z * time_step;

        this_position.x += (this_speed.x - delta.x * time_step * 0.5) * time_step;
        this_position.y += (this_speed.y - delta.y * time_step * 0.5) * time_step;
        this_position.z += (this_speed.z - delta.z * time_step * 0.5) * time_step;
    }
}

void run_forward_simulation(time_t start_date, time_t target_date) {
    std::cout << "Running forward simulation" << std::endl;
    
    time_t current = start_date;
    long double time_step = dt;
    
    printBarycenter(getBarycenter());
    
    while(1) {
        perform_simulation_step(time_step);
        current += time_step;
        
        if (current >= target_date) {
            printCurrentData(current);
            exit(0);
        }
    }
}

void run_backward_simulation(time_t start_date, time_t target_date) {
    std::cout << "Running backward simulation (past date)" << std::endl;
    
    time_t current = start_date;
    long double time_step = -dt;  // Negative time step for backward simulation
    
    printBarycenter(getBarycenter());
    
    while(1) {
        perform_simulation_step(time_step);
        current += time_step;  // time_step is negative, so this goes backward
        
        if (current <= target_date) {
            printCurrentData(current);
            exit(0);
        }
    }
}
