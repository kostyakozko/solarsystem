#include "simulation.h"

#include <cstdlib>
#include <iostream>

#include "constants.h"
#include "model.h"
#include "types.h"

void print_simulation_info(const SimulationArgs& args, time_t start_date) {
  std::cout << "Solar System Simulation" << std::endl;
  std::cout << "Start date: " << ctime(&start_date);

  if (args.use_current_date) {
    std::cout << "Target date: Current time (" << ctime(&args.target_date) << ")" << std::endl;
  } else {
    std::cout << "Target date: " << args.date_string << " (" << ctime(&args.target_date) << ")"
              << std::endl;
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

  while (1) {
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

  while (1) {
    perform_simulation_step(time_step);
    current += time_step;  // time_step is negative, so this goes backward

    if (current <= target_date) {
      printCurrentData(current);
      exit(0);
    }
  }
}

// Global variable to track current simulation time
static time_t current_simulation_time = 0;

void initialize_simulation_to_current_time() {
  current_simulation_time = time(NULL);
  // The simulation state is already initialized with current ephemeris data
}

void update_simulation_to_current_time() {
  time_t now = time(NULL);

  if (now > current_simulation_time) {
    // Run simulation forward to catch up to current time
    time_t time_diff = now - current_simulation_time;

    // Use appropriate time step based on time difference
    long double time_step = (time_diff > 3600) ? 3600.0 : 60.0;  // 1 hour or 1 minute steps

    while (current_simulation_time < now) {
      perform_simulation_step(time_step);
      current_simulation_time += (time_t)time_step;

      if (current_simulation_time > now) {
        current_simulation_time = now;
        break;
      }
    }
  }
}

time_t get_simulation_time() { return current_simulation_time; }

// Web server specific simulation functions (non-blocking, no exit)
bool run_web_forward_simulation(time_t start_date, time_t target_date) {
  std::cout << "Running web forward simulation from " << start_date << " to " << target_date
            << std::endl;

  time_t current = start_date;
  time_t time_diff = target_date - start_date;

  // Adaptive time step based on simulation length
  long double web_time_step;
  if (time_diff <= 86400) {              // <= 1 day: use small steps for accuracy
    web_time_step = dt;                  // 30 seconds
  } else if (time_diff <= 86400 * 7) {   // <= 1 week: medium steps
    web_time_step = dt * 10;             // 5 minutes (300 seconds)
  } else if (time_diff <= 86400 * 30) {  // <= 1 month: larger steps
    web_time_step = dt * 60;             // 30 minutes (1800 seconds)
  } else {                               // > 1 month: use optimal step for accuracy
    web_time_step = dt;                  // 30 seconds (optimal for accuracy)
  }

  // Safety check: maximum iterations based on time step
  const int max_iterations = std::min(1000000, (int)(time_diff / web_time_step) + 1000);
  int iterations = 0;

  std::cout << "Using adaptive web time step: " << web_time_step << " seconds ("
            << (web_time_step / 86400.0) << " days)" << std::endl;
  std::cout << "Expected iterations: " << (time_diff / web_time_step)
            << ", Max allowed: " << max_iterations << std::endl;

  while (current < target_date && iterations < max_iterations) {
    perform_simulation_step(web_time_step);
    current += web_time_step;
    iterations++;

    // Progress logging every 1000 iterations
    if (iterations % 1000 == 0) {
      std::cout << "Web simulation progress: " << iterations << " steps, "
                << ((double)(current - start_date) / time_diff * 100.0) << "% complete"
                << std::endl;
    }
  }

  if (iterations >= max_iterations) {
    std::cout << "Warning: Web simulation reached maximum iterations limit" << std::endl;
    return false;
  }

  std::cout << "Web forward simulation completed in " << iterations << " steps" << std::endl;
  return true;
}

bool run_web_backward_simulation(time_t start_date, time_t target_date) {
  std::cout << "Running web backward simulation from " << start_date << " to " << target_date
            << std::endl;

  time_t current = start_date;
  time_t time_diff = start_date - target_date;

  // Adaptive time step based on simulation length
  long double web_time_step;
  if (time_diff <= 86400) {              // <= 1 day: use small steps for accuracy
    web_time_step = -dt;                 // -30 seconds
  } else if (time_diff <= 86400 * 7) {   // <= 1 week: medium steps
    web_time_step = -dt * 10;            // -5 minutes (-300 seconds)
  } else if (time_diff <= 86400 * 30) {  // <= 1 month: larger steps
    web_time_step = -dt * 60;            // -30 minutes (-1800 seconds)
  } else {                               // > 1 month: use optimal step for accuracy
    web_time_step = -dt;                 // -30 seconds (optimal for accuracy)
  }

  // Safety check: maximum iterations based on time step
  const int max_iterations = std::min(1000000, (int)(time_diff / (-web_time_step)) + 1000);
  int iterations = 0;

  std::cout << "Using adaptive web time step: " << web_time_step << " seconds ("
            << (web_time_step / 86400.0) << " days)" << std::endl;
  std::cout << "Expected iterations: " << (time_diff / (-web_time_step))
            << ", Max allowed: " << max_iterations << std::endl;

  while (current > target_date && iterations < max_iterations) {
    perform_simulation_step(web_time_step);
    current += web_time_step;
    iterations++;

    // Progress logging every 1000 iterations
    if (iterations % 1000 == 0) {
      std::cout << "Web simulation progress: " << iterations << " steps, "
                << ((double)(start_date - current) / time_diff * 100.0) << "% complete"
                << std::endl;
    }
  }

  if (iterations >= max_iterations) {
    std::cout << "Warning: Web simulation reached maximum iterations limit" << std::endl;
    return false;
  }

  std::cout << "Web backward simulation completed in " << iterations << " steps" << std::endl;
  return true;
}
