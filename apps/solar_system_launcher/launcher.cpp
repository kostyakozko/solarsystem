/**
 * Solar System Suite Launcher
 *
 * Unified command dispatcher and workflow coordinator for the Solar System Suite.
 * This application provides a single, professional interface to all specialized
 * tools while enabling complex workflows and operations.
 */

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// Include our modular libraries
#include "args.h"
#include "jpl_data.h"

// Launcher-specific argument structure
struct LauncherArgs {
  // Operation modes
  bool simulate;
  bool fetch;
  bool status;
  bool help;

  // Simulation options
  std::string target_date;
  bool use_current_date;

  // Data management options
  bool update_data;
  bool force_update;
  bool validate_cache;
  bool clean_cache;
  bool rebuild_cache;
  bool test_storage;

  // Workflow options
  bool auto_fetch;  // Automatically fetch data if needed before simulation

  // Output options
  bool verbose;
  bool quiet;

  // Constructor with defaults
  LauncherArgs()
      : simulate(false),
        fetch(false),
        status(false),
        help(false),
        use_current_date(true),
        update_data(false),
        force_update(false),
        validate_cache(false),
        clean_cache(false),
        rebuild_cache(false),
        test_storage(false),
        auto_fetch(false),
        verbose(false),
        quiet(false) {}
};

void print_launcher_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [MODE] [OPTIONS]\n";
  std::cout << "Solar System Suite - Unified Interface\n\n";

  std::cout << "OPERATION MODES:\n";
  std::cout << "  --simulate         Run N-body gravitational simulation\n";
  std::cout << "  --fetch            Manage JPL HORIZONS ephemeris data\n";
  std::cout << "  --status           Show system and cache status\n";
  std::cout << "  --help             Show this help message\n\n";

  std::cout << "SIMULATION OPTIONS:\n";
  std::cout << "  -d, --date DATE    Target date in ISO format (YYYY-MM-DD)\n";
  std::cout << "  --auto-fetch       Automatically fetch data if needed\n\n";

  std::cout << "DATA MANAGEMENT OPTIONS:\n";
  std::cout << "  -u, --update       Update ephemeris data from JPL\n";
  std::cout << "  -f, --force        Force update (bypass smart caching)\n";
  std::cout << "  --validate         Validate cache integrity\n";
  std::cout << "  --clean            Remove all cache files\n";
  std::cout << "  --rebuild          Rebuild binary cache from JSON\n";
  std::cout << "  --test-storage     Test storage system\n\n";

  std::cout << "OUTPUT OPTIONS:\n";
  std::cout << "  -v, --verbose      Enable verbose output\n";
  std::cout << "  -q, --quiet        Suppress non-essential output\n\n";

  std::cout << "EXAMPLES:\n";
  std::cout << "  # Basic simulation with current data\n";
  std::cout << "  " << program_name << " --simulate\n\n";

  std::cout << "  # Simulate to specific date with auto-fetch\n";
  std::cout << "  " << program_name << " --simulate --date 2025-07-01 --auto-fetch\n\n";

  std::cout << "  # Update JPL data\n";
  std::cout << "  " << program_name << " --fetch --update\n\n";

  std::cout << "  # Force update and then simulate\n";
  std::cout << "  " << program_name << " --fetch --force --simulate\n\n";

  std::cout << "  # Check system status\n";
  std::cout << "  " << program_name << " --status\n\n";

  std::cout << "  # Clean cache and update\n";
  std::cout << "  " << program_name << " --fetch --clean --update\n\n";
}

bool parse_launcher_args(int argc, char* argv[], LauncherArgs& args) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    // Operation modes
    if (arg == "--simulate") {
      args.simulate = true;
    } else if (arg == "--fetch") {
      args.fetch = true;
    } else if (arg == "--status") {
      args.status = true;
    } else if (arg == "--help" || arg == "-h") {
      args.help = true;
    }

    // Simulation options
    else if (arg == "-d" || arg == "--date") {
      if (i + 1 < argc) {
        args.target_date = argv[++i];
        args.use_current_date = false;
      } else {
        std::cerr << "Error: --date requires a date value\n";
        return false;
      }
    } else if (arg == "--auto-fetch") {
      args.auto_fetch = true;
    }

    // Data management options
    else if (arg == "-u" || arg == "--update") {
      args.update_data = true;
    } else if (arg == "-f" || arg == "--force") {
      args.force_update = true;
      args.update_data = true;  // Force implies update
    } else if (arg == "--validate") {
      args.validate_cache = true;
    } else if (arg == "--clean") {
      args.clean_cache = true;
    } else if (arg == "--rebuild") {
      args.rebuild_cache = true;
    } else if (arg == "--test-storage") {
      args.test_storage = true;
    }

    // Output options
    else if (arg == "-v" || arg == "--verbose") {
      args.verbose = true;
    } else if (arg == "-q" || arg == "--quiet") {
      args.quiet = true;
    }

    else {
      std::cerr << "Error: Unknown option: " << arg << "\n";
      return false;
    }
  }

  return true;
}

std::string get_executable_path() {
  // Get the directory where the launcher is located
  char path[1024];

#ifdef __APPLE__
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) != 0) {
    return "./";  // Fallback to current directory
  }
#else
  ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (len == -1) {
    return "./";  // Fallback to current directory
  }
  path[len] = '\0';
#endif

  // Extract directory
  std::string exe_path(path);
  size_t last_slash = exe_path.find_last_of('/');
  if (last_slash != std::string::npos) {
    std::string exe_dir = exe_path.substr(0, last_slash + 1);

    // Check if we're in an installed environment (launcher in root, tools in bin/)
    std::string bin_dir = exe_dir + "bin/";
    std::string test_file = bin_dir + "solar_system";

    // Test if installed layout exists
    if (access(test_file.c_str(), F_OK) == 0) {
      return bin_dir;  // Use installed bin/ directory
    } else {
      // Development environment - check if we're in build directory
      std::string build_apps_dir = exe_dir + "../apps/";
      std::string build_test_file = build_apps_dir + "solar_system/solar_system";
      if (access(build_test_file.c_str(), F_OK) == 0) {
        return build_apps_dir;  // Use build apps/ directory
      } else {
        // Fallback to relative paths
        return exe_dir + "../";
      }
    }
  }

  return "./";
}

int execute_command(const std::string& command, bool quiet = false) {
  if (!quiet) {
    std::cout << "Executing: " << command << std::endl;
  }

  int result = system(command.c_str());
  return WEXITSTATUS(result);
}

void print_system_status() {
  std::cout << "=== Solar System Suite Status ===\n";

  // Initialize JPL data system to check status
  initialize_jpl_data();

  if (has_current_ephemeris_data()) {
    time_t epoch = get_ephemeris_epoch();
    const char* source = get_ephemeris_source();
    struct tm* tm_epoch = localtime(&epoch);
    int cached_year = tm_epoch->tm_year + 1900;

    std::cout << "✓ Data Status: READY\n";
    std::cout << "  Source: " << source << "\n";
    std::cout << "  Year: " << cached_year << "\n";
    std::cout << "  Updated: " << ctime(&epoch);

    // Check if current year
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int current_year = tm_now->tm_year + 1900;

    if (cached_year == current_year) {
      std::cout << "  Status: ✓ CURRENT\n";
    } else {
      std::cout << "  Status: ⚠ OUTDATED\n";
    }
  } else {
    std::cout << "⚠ Data Status: USING HARDCODED DATA\n";
    std::cout << "  Recommendation: Run --fetch --update\n";
  }

  std::cout << "\n✓ Available Applications:\n";
  std::cout << "  • solar_system (High-performance simulation)\n";
  std::cout << "  • solar_system_fetch (Data management)\n";
  std::cout << "  • solar_system_launcher (This unified interface)\n";
  std::cout << "  • solar_system_realtime (Live real-time tracking)\n";
  std::cout << "  • solar_system_web (Browser-based visualization)\n";
  std::cout << "\n";
}

int main(int argc, char* argv[]) {
  LauncherArgs args;

  // Parse arguments
  if (!parse_launcher_args(argc, argv, args)) {
    print_launcher_usage(argv[0]);
    return 1;
  }

  // Handle help
  if (args.help || argc == 1) {
    print_launcher_usage(argv[0]);
    return 0;
  }

  // Get executable directory for calling other applications
  std::string exe_dir = get_executable_path();

  int exit_code = 0;
  bool performed_operation = false;

  // Handle status display
  if (args.status) {
    print_system_status();
    performed_operation = true;
  }

  // Handle data management operations
  if (args.fetch || args.update_data || args.force_update || args.validate_cache ||
      args.clean_cache || args.rebuild_cache || args.test_storage) {
    if (!args.quiet) {
      std::cout << "=== Data Management Operations ===\n";
    }

    // Build fetch command
    std::string fetch_cmd = exe_dir;
    if (exe_dir.find("apps/") != std::string::npos) {
      // Build environment
      fetch_cmd += "solar_system_fetch/solar_system_fetch";
    } else {
      // Installed environment
      fetch_cmd += "solar_system_fetch";
    }

    if (args.clean_cache) {
      fetch_cmd += " --clean";
    }
    if (args.force_update) {
      fetch_cmd += " --force";
    } else if (args.update_data) {
      fetch_cmd += " --update";
    }
    if (args.validate_cache) {
      fetch_cmd += " --validate";
    }
    if (args.rebuild_cache) {
      fetch_cmd += " --rebuild";
    }
    if (args.test_storage) {
      fetch_cmd += " --test-storage";
    }
    if (args.verbose) {
      fetch_cmd += " --verbose";
    }

    // If no specific fetch operation, show status
    if (!args.update_data && !args.force_update && !args.validate_cache && !args.clean_cache &&
        !args.rebuild_cache && !args.test_storage) {
      fetch_cmd += " --status";
    }

    int fetch_result = execute_command(fetch_cmd, args.quiet);
    if (fetch_result != 0) {
      std::cerr << "Data management operation failed\n";
      exit_code = fetch_result;
    }

    performed_operation = true;
  }

  // Handle simulation
  if (args.simulate) {
    if (!args.quiet) {
      std::cout << "=== Solar System Simulation ===\n";
    }

    // Auto-fetch if requested and no current data
    if (args.auto_fetch) {
      initialize_jpl_data();
      if (!has_current_year_ephemeris_data()) {
        if (!args.quiet) {
          std::cout << "Auto-fetching current year data...\n";
        }
        std::string auto_fetch_cmd = exe_dir;
        if (exe_dir.find("apps/") != std::string::npos) {
          // Build environment
          auto_fetch_cmd += "solar_system_fetch/solar_system_fetch --update";
        } else {
          // Installed environment
          auto_fetch_cmd += "solar_system_fetch --update";
        }
        int fetch_result = execute_command(auto_fetch_cmd, args.quiet);
        if (fetch_result != 0) {
          std::cerr << "Auto-fetch failed, continuing with available data\n";
        }
      }
    }

    // Build simulation command
    std::string sim_cmd = exe_dir;
    if (exe_dir.find("apps/") != std::string::npos) {
      // Build environment
      sim_cmd += "solar_system/solar_system";
    } else {
      // Installed environment
      sim_cmd += "solar_system";
    }

    if (!args.use_current_date && !args.target_date.empty()) {
      sim_cmd += " --date " + args.target_date;
    }

    int sim_result = execute_command(sim_cmd, args.quiet);
    if (sim_result != 0) {
      std::cerr << "Simulation failed\n";
      exit_code = sim_result;
    }

    performed_operation = true;
  }

  // If no operations were performed, show status
  if (!performed_operation) {
    print_system_status();
    std::cout << "Use --help for available options\n";
  }

  return exit_code;
}
