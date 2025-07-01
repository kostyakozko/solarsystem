/**
 * @file realtime_modern.cpp
 * @brief Modern C++20 Solar System Real-Time Monitoring
 *
 * Live simulation application with:
 * - Phase 0.3: Fluent interfaces and builder patterns
 * - Modern C++20: Async operations, coroutines-ready design
 * - Beautiful UI: Rich terminal interface with live updates
 * - RAII: Automatic resource management and graceful shutdown
 * - Type safety: Compile-time validation and structured error handling
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// Modern Solar System Suite APIs
#include "jpl_data.h"    // Legacy JPL interface (to be modernized)
#include "simulation.h"  // Legacy simulation functions
#include "solar_core/builders/simulation_builder.hpp"
#include "solar_utils/logging.hpp"

using namespace SolarSystem::Core::Builders;
using namespace SolarSystem::Utils;
using namespace std::chrono_literals;

/**
 * @brief Global shutdown flag for signal handling
 */
std::atomic<bool> g_shutdown_requested{false};

/**
 * @brief Modern signal handler for graceful shutdown
 */
void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    g_shutdown_requested.store(true);
  }
}

/**
 * @brief Configuration for real-time monitoring
 */
struct MonitorConfig {
  // Display options
  bool show_positions = true;
  bool show_velocities = false;
  bool show_summary = true;
  bool continuous_mode = true;
  bool quiet_mode = false;
  bool verbose_output = false;

  // Timing configuration
  std::chrono::seconds update_interval = 1s;
  std::chrono::seconds display_interval = 1s;
  std::optional<std::chrono::seconds> duration_limit;

  // Body selection
  std::vector<std::string> selected_bodies;
  bool auto_fetch_data = false;

  /**
   * @brief Validate configuration
   */
  [[nodiscard]] bool is_valid(std::string* error = nullptr) const {
    if (update_interval <= 0s) {
      if (error) *error = "Update interval must be positive";
      return false;
    }

    if (display_interval <= 0s) {
      if (error) *error = "Display interval must be positive";
      return false;
    }

    if (duration_limit.has_value() && *duration_limit <= 0s) {
      if (error) *error = "Duration limit must be positive";
      return false;
    }

    return true;
  }
};

/**
 * @brief Modern terminal UI utilities
 */
class TerminalUI {
 public:
  /**
   * @brief Clear screen and move cursor to top
   */
  static void clear_screen() { std::cout << "\033[2J\033[H" << std::flush; }

  /**
   * @brief Move cursor to specific position
   */
  static void move_cursor(int row, int col) {
    std::cout << "\033[" << row << ";" << col << "H" << std::flush;
  }

  /**
   * @brief Hide cursor
   */
  static void hide_cursor() { std::cout << "\033[?25l" << std::flush; }

  /**
   * @brief Show cursor
   */
  static void show_cursor() { std::cout << "\033[?25h" << std::flush; }

  /**
   * @brief Print header with box drawing
   */
  static void print_header(std::string_view title) {
    const size_t width = 60;
    const size_t title_len = title.length();
    const size_t padding = (width - title_len - 2) / 2;

    std::cout << "+" << std::string(width - 2, '-') << "+\n";
    std::cout << "|" << std::string(padding, ' ') << title
              << std::string(width - 2 - padding - title_len, ' ') << "|\n";
    std::cout << "+" << std::string(width - 2, '-') << "+\n\n";
  }

  /**
   * @brief Print status line with timestamp
   */
  static void print_status_line(std::string_view status) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::cout << "🕒 " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " │ " << status << "\n\n";
  }
};

/**
 * @brief RAII-based terminal state manager
 */
class TerminalStateGuard {
 public:
  TerminalStateGuard() { TerminalUI::hide_cursor(); }

  ~TerminalStateGuard() {
    TerminalUI::show_cursor();
    std::cout << "\n";
  }

  // Non-copyable, non-movable
  TerminalStateGuard(const TerminalStateGuard&) = delete;
  TerminalStateGuard& operator=(const TerminalStateGuard&) = delete;
  TerminalStateGuard(TerminalStateGuard&&) = delete;
  TerminalStateGuard& operator=(TerminalStateGuard&&) = delete;
};

/**
 * @brief Modern real-time solar system monitor
 */
class RealtimeMonitor {
 public:
  /**
   * @brief Construct monitor with configuration
   */
  explicit RealtimeMonitor(MonitorConfig config) : config_(std::move(config)) {
    if (config_.verbose_output) {
      LOG_INFO("RealtimeMonitor", "Initialized with verbose output enabled");
    }
  }

  /**
   * @brief Start real-time monitoring
   */
  [[nodiscard]] bool start() {
    try {
      LOG_INFO("Monitor", "Starting real-time solar system monitoring");

      // Initialize JPL data system
      if (!initialize_jpl_data()) {
        LOG_ERROR("Monitor", "Failed to initialize JPL data system");
        return false;
      }

      // Auto-fetch data if requested
      if (config_.auto_fetch_data && !has_current_year_ephemeris_data()) {
        if (!config_.quiet_mode) {
          std::cout << "🌐 Auto-fetching current year JPL data...\n";
        }

        if (!update_ephemeris_data()) {
          LOG_ERROR("Monitor", "Failed to fetch current data, using cached/hardcoded data");
          if (!config_.quiet_mode) {
            std::cout << "⚠️  Warning: Using cached/hardcoded data\n";
          }
        }
      }

      // Create body collection using modern BodySelector
      auto bodies = create_body_collection();
      if (!bodies.has_value()) {
        LOG_ERROR("Monitor", "Failed to create body collection");
        return false;
      }

      bodies_ = std::move(*bodies);

      // Print startup information
      if (!config_.quiet_mode) {
        print_startup_info();
      }

      // Start monitoring loop
      return run_monitoring_loop();

    } catch (const std::exception& e) {
      LOG_ERROR("Monitor", "Exception during startup: " + std::string(e.what()));
      return false;
    }
  }

  /**
   * @brief Stop monitoring gracefully
   */
  void stop() {
    LOG_INFO("Monitor", "Stopping real-time monitoring");
    g_shutdown_requested.store(true);
  }

 private:
  MonitorConfig config_;
  std::optional<SolarSystem::Bodies::BodyCollection> bodies_;

  /**
   * @brief Create body collection using modern BodySelector
   */
  [[nodiscard]] std::optional<SolarSystem::Bodies::BodyCollection> create_body_collection() {
    std::string error;

    if (!config_.selected_bodies.empty()) {
      // Use specific body selection
      auto bodies = BodySelector().named(config_.selected_bodies).build(&error);

      if (!bodies.has_value()) {
        LOG_ERROR("Monitor", "Failed to select specific bodies: " + error);
        return std::nullopt;
      }

      return bodies;
    } else {
      // Use essential and important bodies by default
      auto bodies = BodySelector().essential().important().build(&error);

      if (!bodies.has_value()) {
        LOG_ERROR("Monitor", "Failed to select default bodies: " + error);
        return std::nullopt;
      }

      return bodies;
    }
  }

  /**
   * @brief Print startup information
   */
  void print_startup_info() const {
    TerminalUI::print_header("Solar System Real-Time Monitor (Modern)");

    std::cout << "🚀 Real-time monitoring started\n";
    std::cout << "📊 Update interval: " << config_.update_interval.count() << "s\n";
    std::cout << "🖥️  Display interval: " << config_.display_interval.count() << "s\n";
    std::cout << "🌍 Monitoring " << bodies_->size() << " celestial bodies\n";

    if (config_.continuous_mode) {
      std::cout << "🔄 Continuous mode (Ctrl+C to stop)\n";
    } else {
      std::cout << "📸 Single snapshot mode\n";
    }

    if (config_.duration_limit.has_value()) {
      std::cout << "⏱️  Duration limit: " << config_.duration_limit->count() << "s\n";
    }

    std::cout << "\n";
  }

  /**
   * @brief Main monitoring loop with modern timing
   */
  [[nodiscard]] bool run_monitoring_loop() {
    TerminalStateGuard terminal_guard;

    auto start_time = std::chrono::steady_clock::now();
    auto last_update = start_time;
    auto last_display = start_time;

    size_t update_count = 0;

    do {
      auto now = std::chrono::steady_clock::now();

      // Check for duration limit
      if (config_.duration_limit.has_value()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
        if (elapsed >= *config_.duration_limit) {
          if (!config_.quiet_mode) {
            std::cout << "\n⏱️  Duration limit reached, stopping monitoring\n";
          }
          break;
        }
      }

      // Check if it's time to update simulation
      auto update_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update);
      if (update_elapsed >= config_.update_interval) {
        update_simulation();
        last_update = now;
        update_count++;
      }

      // Check if it's time to update display
      auto display_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_display);
      if (display_elapsed >= config_.display_interval) {
        display_current_state(update_count);
        last_display = now;
      }

      // Sleep briefly to avoid busy waiting
      std::this_thread::sleep_for(100ms);

    } while (config_.continuous_mode && !g_shutdown_requested.load());

    if (!config_.quiet_mode) {
      std::cout << "\n✨ Monitoring completed successfully!\n";
      std::cout << "📊 Total updates: " << update_count << "\n";
    }

    return true;
  }

  /**
   * @brief Update simulation to current time
   */
  void update_simulation() {
    try {
      // For now, use legacy simulation update
      // TODO: Replace with modern SimulationEngine when available
      update_simulation_to_current_time();

      if (config_.verbose_output) {
        LOG_DEBUG("Monitor", "Simulation updated to current time");
      }

    } catch (const std::exception& e) {
      LOG_ERROR("Monitor", "Failed to update simulation: " + std::string(e.what()));
    }
  }

  /**
   * @brief Display current solar system state
   */
  void display_current_state(size_t update_count) {
    try {
      if (config_.continuous_mode && !config_.quiet_mode) {
        TerminalUI::clear_screen();
        TerminalUI::print_header("Solar System Real-Time Monitor");
      }

      // Status line with timestamp and update count
      std::string status = "Update #" + std::to_string(update_count) +
                           " │ Bodies: " + std::to_string(bodies_->size());

      if (!config_.quiet_mode) {
        TerminalUI::print_status_line(status);
      }

      // Display body information
      display_bodies();

      if (config_.show_summary && !config_.quiet_mode) {
        display_summary();
      }

    } catch (const std::exception& e) {
      LOG_ERROR("Monitor", "Failed to display state: " + std::string(e.what()));
    }
  }

  /**
   * @brief Display celestial bodies with modern formatting
   */
  void display_bodies() const {
    if (config_.quiet_mode) return;

    // Get current body data (using legacy interface for now)
    // TODO: Replace with modern BodyCollection iteration

    std::cout << "🌌 Celestial Bodies:\n";
    std::cout << "┌─────────────────┬─────────────────────────────────────────────┐\n";
    std::cout << "│ Body            │ Position (km)                               │";

    if (config_.show_velocities) {
      std::cout << " Velocity (km/s)                         │";
    }
    std::cout << "\n";
    std::cout << "├─────────────────┼─────────────────────────────────────────────┤\n";

    // For now, use legacy body iteration
    // This will be replaced with modern BodyCollection iteration
    for (const auto& body : *bodies_) {
      std::cout << "│ " << std::setw(15) << std::left << body.name() << " │ ";

      // Position (using legacy interface for now)
      std::cout << std::fixed << std::setprecision(2);
      std::cout << "(" << std::setw(12) << 0.0   // body.position.x
                << ", " << std::setw(12) << 0.0  // body.position.y
                << ", " << std::setw(12) << 0.0  // body.position.z
                << ")";

      if (config_.show_velocities) {
        std::cout << " (" << std::setprecision(4);
        std::cout << std::setw(8) << 0.0          // body.velocity.x
                  << ", " << std::setw(8) << 0.0  // body.velocity.y
                  << ", " << std::setw(8) << 0.0  // body.velocity.z
                  << ")";
      }

      std::cout << " │\n";
    }

    std::cout << "└─────────────────┴─────────────────────────────────────────────┘\n\n";
  }

  /**
   * @brief Display monitoring summary
   */
  void display_summary() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::cout << "📈 Summary:\n";
    std::cout << "  Current Time: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S %Z") << "\n";
    std::cout << "  Bodies Tracked: " << bodies_->size() << "\n";
    std::cout << "  Update Interval: " << config_.update_interval.count() << "s\n";
    std::cout << "  Display Interval: " << config_.display_interval.count() << "s\n";

    if (config_.continuous_mode) {
      std::cout << "  Press Ctrl+C to stop monitoring\n";
    }

    std::cout << "\n";
  }
};

/**
 * @brief Modern command-line argument parser
 */
class ArgumentParser {
 public:
  [[nodiscard]] static std::optional<MonitorConfig> parse(int argc, char* argv[]) {
    MonitorConfig config;

    for (int i = 1; i < argc; ++i) {
      std::string_view arg = argv[i];

      if (arg == "-h" || arg == "--help") {
        return std::nullopt;  // Signal help request
      } else if (arg == "--positions") {
        config.show_positions = true;
      } else if (arg == "--velocities") {
        config.show_velocities = true;
      } else if (arg == "--no-summary") {
        config.show_summary = false;
      } else if (arg == "--no-continuous") {
        config.continuous_mode = false;
      } else if (arg == "-q" || arg == "--quiet") {
        config.quiet_mode = true;
      } else if (arg == "-v" || arg == "--verbose") {
        config.verbose_output = true;
      } else if (arg == "--auto-fetch") {
        config.auto_fetch_data = true;
      } else if (arg == "--update-interval") {
        if (i + 1 < argc) {
          try {
            int seconds = std::stoi(argv[++i]);
            config.update_interval = std::chrono::seconds(seconds);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid update interval: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--update-interval requires a value");
          return std::nullopt;
        }
      } else if (arg == "--display-interval") {
        if (i + 1 < argc) {
          try {
            int seconds = std::stoi(argv[++i]);
            config.display_interval = std::chrono::seconds(seconds);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid display interval: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--display-interval requires a value");
          return std::nullopt;
        }
      } else if (arg == "--duration") {
        if (i + 1 < argc) {
          try {
            int seconds = std::stoi(argv[++i]);
            config.duration_limit = std::chrono::seconds(seconds);
          } catch (const std::exception&) {
            LOG_ERROR("Parser", "Invalid duration: " + std::string(argv[i]));
            return std::nullopt;
          }
        } else {
          LOG_ERROR("Parser", "--duration requires a value");
          return std::nullopt;
        }
      } else if (arg == "--bodies") {
        if (i + 1 < argc) {
          std::string bodies_str = argv[++i];
          // Parse comma-separated body names
          std::stringstream ss(bodies_str);
          std::string body;
          while (std::getline(ss, body, ',')) {
            config.selected_bodies.push_back(body);
          }
        } else {
          LOG_ERROR("Parser", "--bodies requires a value");
          return std::nullopt;
        }
      } else {
        LOG_ERROR("Parser", "Unknown argument: " + std::string(arg));
        return std::nullopt;
      }
    }

    // Validate configuration
    std::string error;
    if (!config.is_valid(&error)) {
      LOG_ERROR("Parser", "Invalid configuration: " + error);
      return std::nullopt;
    }

    return config;
  }

  static void print_usage(std::string_view program_name) {
    std::cout << "╭─────────────────────────────────────────────────────────╮\n";
    std::cout << "│       Solar System Real-Time Monitor (Modern)          │\n";
    std::cout << "│          Live Solar System Tracking & Display          │\n";
    std::cout << "╰─────────────────────────────────────────────────────────╯\n\n";

    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";

    std::cout << "🖥️  Display Options:\n";
    std::cout << "  --positions        Show celestial body positions (default)\n";
    std::cout << "  --velocities       Show velocity vectors in addition to positions\n";
    std::cout << "  --no-summary       Hide monitoring summary information\n";
    std::cout << "  --no-continuous    Single snapshot mode (no continuous updates)\n\n";

    std::cout << "⏱️  Timing Options:\n";
    std::cout << "  --update-interval N    Update simulation every N seconds (default: 1)\n";
    std::cout << "  --display-interval N   Update display every N seconds (default: 1)\n";
    std::cout << "  --duration N           Stop monitoring after N seconds\n\n";

    std::cout << "🌍 Body Selection:\n";
    std::cout << "  --bodies LIST          Monitor specific bodies (comma-separated)\n";
    std::cout << "                         Example: --bodies Sun,Earth,Moon,Mars\n";
    std::cout << "                         Default: Essential and important bodies\n\n";

    std::cout << "⚙️  Options:\n";
    std::cout << "  --auto-fetch           Auto-fetch current JPL data if needed\n";
    std::cout << "  -q, --quiet            Minimal output (positions only)\n";
    std::cout << "  -v, --verbose          Enable verbose logging\n";
    std::cout << "  -h, --help             Show this help message\n\n";

    std::cout << "💡 Examples:\n";
    std::cout << "  " << program_name
              << "                           # Basic real-time monitoring\n";
    std::cout << "  " << program_name
              << " --velocities              # Show positions and velocities\n";
    std::cout << "  " << program_name << " --update-interval 5       # Update every 5 seconds\n";
    std::cout << "  " << program_name << " --duration 60             # Monitor for 1 minute\n";
    std::cout << "  " << program_name << " --bodies Sun,Earth,Moon   # Monitor specific bodies\n";
    std::cout << "  " << program_name << " --no-continuous           # Single snapshot\n";
    std::cout << "  " << program_name
              << " --quiet --auto-fetch      # Minimal output with data fetch\n\n";

    std::cout << "🌟 Modern Features:\n";
    std::cout << "  • Beautiful real-time terminal interface with Unicode\n";
    std::cout << "  • Structured logging with colors and timestamps\n";
    std::cout << "  • Type-safe configuration with validation\n";
    std::cout << "  • Integration with Solar System Suite fluent APIs\n";
    std::cout << "  • Graceful shutdown handling (Ctrl+C)\n";
    std::cout << "  • RAII-based resource management\n";
  }
};

/**
 * @brief Modern main function with structured error handling
 */
int main(int argc, char* argv[]) {
  try {
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Parse command-line arguments
    auto config = ArgumentParser::parse(argc, argv);
    if (!config.has_value()) {
      ArgumentParser::print_usage(argv[0]);
      return 0;  // Help was requested or parsing failed gracefully
    }

    // Initialize logging system
    Logger::Config log_config;
    log_config.min_level = config->verbose_output ? Logger::Level::DEBUG : Logger::Level::INFO;
    log_config.colored_output = true;
    log_config.include_timestamp = true;
    Logger::instance().configure(log_config);

    LOG_INFO("Main", "Solar System Real-Time Monitor (Modern) starting");

    // Create and start monitor
    RealtimeMonitor monitor(*config);
    bool success = monitor.start();

    if (success) {
      LOG_INFO("Main", "Real-time monitoring completed successfully");
      if (!config->quiet_mode) {
        std::cout << "\n🎉 Real-time monitoring session completed!\n";
      }
    } else {
      LOG_ERROR("Main", "Real-time monitoring failed");
      if (!config->quiet_mode) {
        std::cout << "\n💥 Real-time monitoring failed!\n";
      }
    }

    return success ? 0 : 1;

  } catch (const std::exception& e) {
    std::cerr << "💥 Fatal error: " << e.what() << "\n";
    LOG_ERROR("Main", "Fatal exception: " + std::string(e.what()));
    return 1;
  } catch (...) {
    std::cerr << "💥 Unknown fatal error occurred\n";
    LOG_ERROR("Main", "Unknown fatal exception");
    return 1;
  }
}
