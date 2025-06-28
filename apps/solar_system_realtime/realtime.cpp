/**
 * Solar System Real-Time Simulation
 *
 * Live simulation application that continuously tracks the current solar system
 * state using real system time. Perfect for demonstrations, live displays,
 * and real-time solar system monitoring.
 */

#include <signal.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

// Include our modular libraries
#include "args.h"
#include "jpl_data.h"
#include "model.h"
#include "simulation.h"

// Global flag for graceful shutdown
volatile bool running = true;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    std::cout << "\n\nReceived shutdown signal. Stopping real-time simulation...\n";
    running = false;
  }
}

// Real-time specific argument structure
struct RealtimeArgs {
  // Display options
  bool show_positions;
  bool show_velocities;
  bool show_summary;
  bool continuous_mode;

  // Update intervals
  int update_interval_seconds;
  int display_interval_seconds;

  // Output options
  bool verbose;
  bool quiet;
  bool help;

  // Data management
  bool auto_fetch;

  // Constructor with defaults
  RealtimeArgs()
      : show_positions(true),
        show_velocities(false),
        show_summary(true),
        continuous_mode(true),
        update_interval_seconds(60),
        display_interval_seconds(10),
        verbose(false),
        quiet(false),
        help(false),
        auto_fetch(true) {}
};

void print_realtime_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [OPTIONS]\n";
  std::cout << "Solar System Real-Time Simulation - Live Solar System Tracking\n\n";

  std::cout << "DISPLAY OPTIONS:\n";
  std::cout << "  --positions        Show celestial body positions (default)\n";
  std::cout << "  --velocities       Show celestial body velocities\n";
  std::cout << "  --summary          Show system summary (default)\n";
  std::cout << "  --no-continuous    Run once instead of continuous updates\n\n";

  std::cout << "TIMING OPTIONS:\n";
  std::cout << "  --update-interval SEC    Simulation update interval (default: 60s)\n";
  std::cout << "  --display-interval SEC   Display refresh interval (default: 10s)\n\n";

  std::cout << "DATA OPTIONS:\n";
  std::cout << "  --auto-fetch       Automatically fetch current JPL data (default)\n";
  std::cout << "  --no-auto-fetch    Use existing cached data only\n\n";

  std::cout << "OUTPUT OPTIONS:\n";
  std::cout << "  -v, --verbose      Enable verbose output\n";
  std::cout << "  -q, --quiet        Minimal output (positions only)\n";
  std::cout << "  -h, --help         Show this help message\n\n";

  std::cout << "EXAMPLES:\n";
  std::cout << "  # Basic real-time tracking\n";
  std::cout << "  " << program_name << "\n\n";

  std::cout << "  # Real-time with velocities, fast updates\n";
  std::cout << "  " << program_name << " --velocities --display-interval 5\n\n";

  std::cout << "  # Single snapshot (no continuous mode)\n";
  std::cout << "  " << program_name << " --no-continuous\n\n";

  std::cout << "  # Quiet mode for data logging\n";
  std::cout << "  " << program_name << " --quiet --display-interval 30\n\n";

  std::cout << "CONTROLS:\n";
  std::cout << "  Ctrl+C             Graceful shutdown\n";
  std::cout << "  SIGTERM            Graceful shutdown\n\n";
}

bool parse_realtime_args(int argc, char* argv[], RealtimeArgs& args) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      args.help = true;
    } else if (arg == "--positions") {
      args.show_positions = true;
    } else if (arg == "--velocities") {
      args.show_velocities = true;
    } else if (arg == "--summary") {
      args.show_summary = true;
    } else if (arg == "--no-continuous") {
      args.continuous_mode = false;
    } else if (arg == "--update-interval") {
      if (i + 1 < argc) {
        try {
          args.update_interval_seconds = std::stoi(argv[++i]);
          if (args.update_interval_seconds < 1) {
            std::cerr << "Error: Update interval must be at least 1 second\n";
            return false;
          }
        } catch (const std::exception& e) {
          std::cerr << "Error: Invalid update interval: " << argv[i] << "\n";
          return false;
        }
      } else {
        std::cerr << "Error: --update-interval requires a value\n";
        return false;
      }
    } else if (arg == "--display-interval") {
      if (i + 1 < argc) {
        try {
          args.display_interval_seconds = std::stoi(argv[++i]);
          if (args.display_interval_seconds < 1) {
            std::cerr << "Error: Display interval must be at least 1 second\n";
            return false;
          }
        } catch (const std::exception& e) {
          std::cerr << "Error: Invalid display interval: " << argv[i] << "\n";
          return false;
        }
      } else {
        std::cerr << "Error: --display-interval requires a value\n";
        return false;
      }
    } else if (arg == "--auto-fetch") {
      args.auto_fetch = true;
    } else if (arg == "--no-auto-fetch") {
      args.auto_fetch = false;
    } else if (arg == "-v" || arg == "--verbose") {
      args.verbose = true;
    } else if (arg == "-q" || arg == "--quiet") {
      args.quiet = true;
    } else {
      std::cerr << "Error: Unknown option: " << arg << "\n";
      return false;
    }
  }

  return true;
}

void print_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::cout << "🕐 " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
            << " (Local Time)\n";
}

void print_system_summary(bool verbose) {
  if (verbose) {
    std::cout << "\n=== Solar System Real-Time Status ===\n";
  }

  print_timestamp();

  if (has_current_ephemeris_data()) {
    time_t epoch = get_ephemeris_epoch();
    const char* source = get_ephemeris_source();

    if (verbose) {
      std::cout << "📡 Data Source: " << source << "\n";
      std::cout << "📅 Data Epoch: " << ctime(&epoch);
    }

    // Check data currency
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    struct tm* tm_epoch = localtime(&epoch);
    int current_year = tm_now->tm_year + 1900;
    int cached_year = tm_epoch->tm_year + 1900;

    if (cached_year == current_year) {
      if (verbose) {
        std::cout << "✅ Data Status: CURRENT (" << current_year << ")\n";
      }
    } else {
      std::cout << "⚠️  Data Status: OUTDATED (cached: " << cached_year
                << ", current: " << current_year << ")\n";
    }
  } else {
    std::cout << "⚠️  Using hardcoded ephemeris data\n";
  }

  if (verbose) {
    std::cout << "🌌 Tracking " << get_body_count() << " celestial bodies\n";
  }
}

void print_body_positions(bool show_velocities, bool quiet) {
  if (!quiet) {
    std::cout << "\n=== Current Celestial Body Positions ===\n";
  }

  for (int i = 0; i < get_body_count(); i++) {
    const planet& body = get_body(i);

    if (quiet) {
      // Compact format for logging
      std::cout << body.name << "," << std::fixed << std::setprecision(6) << body.position.x << ","
                << body.position.y << "," << body.position.z;

      if (show_velocities) {
        std::cout << "," << body.speed.x << "," << body.speed.y << "," << body.speed.z;
      }
      std::cout << "\n";
    } else {
      // Human-readable format
      std::cout << "🪐 " << std::setw(15) << std::left << body.name << ": ";
      std::cout << "(" << std::fixed << std::setprecision(3) << std::setw(12) << body.position.x
                << ", " << std::setw(12) << body.position.y << ", " << std::setw(12)
                << body.position.z << ")";

      if (show_velocities) {
        std::cout << " v=(" << std::setprecision(6) << std::setw(10) << body.speed.x << ", "
                  << std::setw(10) << body.speed.y << ", " << std::setw(10) << body.speed.z << ")";
      }
      std::cout << "\n";
    }
  }
}

void clear_screen() {
  // Clear screen for continuous updates
  std::cout << "\033[2J\033[H";
}

int main(int argc, char* argv[]) {
  RealtimeArgs args;

  // Parse arguments
  if (!parse_realtime_args(argc, argv, args)) {
    print_realtime_usage(argv[0]);
    return 1;
  }

  // Handle help
  if (args.help) {
    print_realtime_usage(argv[0]);
    return 0;
  }

  // Set up signal handlers for graceful shutdown
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Initialize JPL data system
  if (!initialize_jpl_data()) {
    std::cerr << "Failed to initialize JPL data system\n";
    return 1;
  }

  // Auto-fetch current data if requested
  if (args.auto_fetch && !has_current_year_ephemeris_data()) {
    if (!args.quiet) {
      std::cout << "Auto-fetching current year JPL data...\n";
    }

    if (!update_ephemeris_data()) {
      std::cerr << "Warning: Failed to fetch current data, using cached/hardcoded data\n";
    }
  }

  if (!args.quiet) {
    std::cout << "🚀 Solar System Real-Time Simulation Started\n";
    std::cout << "📊 Update interval: " << args.update_interval_seconds << "s\n";
    std::cout << "🖥️  Display interval: " << args.display_interval_seconds << "s\n";

    if (args.continuous_mode) {
      std::cout << "🔄 Continuous mode (Ctrl+C to stop)\n";
    } else {
      std::cout << "📸 Single snapshot mode\n";
    }
    std::cout << "\n";
  }

  // Initialize simulation to current time
  initialize_simulation_to_current_time();

  auto last_update = std::chrono::steady_clock::now();
  auto last_display = std::chrono::steady_clock::now();

  // Main real-time loop
  do {
    auto now = std::chrono::steady_clock::now();

    // Check if it's time to update simulation
    auto update_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update);
    if (update_elapsed.count() >= args.update_interval_seconds) {
      // Update simulation to current real time
      update_simulation_to_current_time();
      last_update = now;
    }

    // Check if it's time to update display
    auto display_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_display);
    if (display_elapsed.count() >= args.display_interval_seconds) {
      if (args.continuous_mode && !args.quiet) {
        clear_screen();
      }

      if (args.show_summary) {
        print_system_summary(args.verbose);
      }

      if (args.show_positions) {
        print_body_positions(args.show_velocities, args.quiet);
      }

      if (!args.quiet && args.continuous_mode) {
        std::cout << "\n⏱️  Next update in " << args.display_interval_seconds
                  << "s (Ctrl+C to stop)\n";
      }

      last_display = now;
    }

    // Sleep for a short interval to avoid busy waiting
    if (args.continuous_mode && running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

  } while (args.continuous_mode && running);

  if (!args.quiet) {
    std::cout << "\n🛑 Real-time simulation stopped.\n";
  }

  return 0;
}
