#ifndef __ARGS_H__
#define __ARGS_H__

#include <ctime>
#include <string>

struct SimulationArgs {
  time_t target_date;
  bool use_current_date;
  std::string date_string;
  bool update_data;    // -u, --update-data
  bool rebuild_cache;  // --rebuild
  bool test_storage;   // --test-storage
};

// Extended args structure for specialized applications
struct Args {
  // Simulation options
  std::string target_date;
  bool use_current_date;

  // Data management options
  bool update_data;
  bool force_update;
  int target_year;
  bool rebuild_cache;
  bool test_storage;
  bool validate_cache;
  bool clean_cache;
  bool show_status;

  // Output options
  bool verbose;
  bool show_help;

  // Constructor with defaults
  Args()
      : target_date(""),
        use_current_date(true),
        update_data(false),
        force_update(false),
        target_year(0),
        rebuild_cache(false),
        test_storage(false),
        validate_cache(false),
        clean_cache(false),
        show_status(false),
        verbose(false),
        show_help(false) {}
};

// Parse command line arguments (original function for main simulation)
SimulationArgs parse_arguments(int argc, char* argv[]);

// Parse command line arguments (extended function for specialized apps)
bool parse_args(int argc, char* argv[], Args& args);

// Parse ISO date string (YYYY-MM-DD) to time_t
time_t parse_iso_date(const std::string& date_str);

// Print usage information
void print_usage(const char* program_name);

#endif  //__ARGS_H__
