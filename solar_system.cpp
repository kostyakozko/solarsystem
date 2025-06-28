#include <iostream>
#include <iomanip>
#include <ctime>
#include "types.h"
#include "constants.h"
#include "model.h"
#include "args.h"
#include "simulation.h"

int main(int argc, char* argv[])
{
  // Parse command line arguments
  SimulationArgs args = parse_arguments(argc, argv);
  
  // Set output precision
  std::cout.precision(12);
  
  // Define starting date (your original data point)
  struct tm start_timeinfo = {0, 0, 0, 11, 1, 2018 - 1900};
  time_t start_date = mktime(&start_timeinfo);
  
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
