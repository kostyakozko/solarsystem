#ifndef __ARGS_H__
#define __ARGS_H__

#include <string>
#include <ctime>

struct SimulationArgs {
    time_t target_date;
    bool use_current_date;
    std::string date_string;
};

// Parse command line arguments
SimulationArgs parse_arguments(int argc, char* argv[]);

// Parse ISO date string (YYYY-MM-DD) to time_t
time_t parse_iso_date(const std::string& date_str);

// Print usage information
void print_usage(const char* program_name);

#endif //__ARGS_H__
