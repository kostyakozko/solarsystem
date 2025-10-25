/**
 * @file realtime_modern.cpp
 * @brief Modern C++20 Solar System Real-Time Monitoring with Live Data Streaming
 *
 * Enhanced live simulation application with:
 * - Phase 0.3: Fluent interfaces and builder patterns
 * - Modern C++20: Async operations, coroutines-ready design
 * - Beautiful UI: Rich terminal interface with live updates
 * - RAII: Automatic resource management and graceful shutdown
 * - Type safety: Compile-time validation and structured error handling
 * - Live Data Streaming: Efficient real-time data updates with quality monitoring
 * - Data Filtering: Advanced filtering and aggregation capabilities
 * - Quality Monitoring: Comprehensive data quality assessment and reporting
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
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/builders/simulation_builder.hpp"
#include "solar_core/streaming/realtime_stream.hpp"
#include "solar_core/streaming/stream_filter.hpp"
#include "solar_core/streaming/stream_aggregator.hpp"
#include "solar_core/streaming/quality_monitor.hpp"
#include "solar_utils/argument_parser.hpp"
#include "solar_utils/logging.hpp"

using namespace SolarSystem::Core::Builders;
using namespace SolarSystem::Utils;
using namespace SolarSystem::Streaming;
using namespace std::chrono_literals;
using namespace SolarSystem::Utils;

// Use modern RealtimeConfig from argument parser
using MonitorConfig = RealtimeConfig;

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
    auto current_time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&current_time_t);

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
 * @brief Modern real-time solar system monitor with live data streaming
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

    // Initialize streaming components
    initialize_streaming_system();
  }

  /**
   * @brief Destructor - ensure proper cleanup order
   */
  ~RealtimeMonitor() {
    try {
      // Ensure streaming system is stopped before destruction
      if (realtime_stream_ && realtime_stream_->is_running()) {
        stop();
      } else if (quality_monitor_ && quality_monitor_->is_running()) {
        stop();
      }
    } catch (const std::exception& e) {
      // Log error but don't throw from destructor
      LOG_ERROR("RealtimeMonitor", "Error during destruction: " + std::string(e.what()));
    }
  }

  /**
   * @brief Start real-time monitoring
   */
  [[nodiscard]] bool start(SolarSystem::Bodies::BodyFactory& factory) {
    try {
      LOG_INFO("Monitor", "Starting real-time solar system monitoring");

      // Initialize JPL data system
      if (!factory.is_initialized()) {
        LOG_ERROR("Monitor", "Failed to initialize JPL data system");
        return false;
      }

      // Auto-fetch data if requested
      if (config_.auto_fetch_data && !factory.has_current_year_ephemeris_data()) {
        if (!config_.quiet_mode) {
          std::cout << "🌐 Auto-fetching current year JPL data...\n";
        }

        auto result = factory.fetch_current_ephemeris_data();  // Uses default current year
        if (!result.has_value()) {
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

      // Start streaming system
      if (!start_streaming()) {
        LOG_ERROR("Monitor", "Failed to start streaming system");
        return false;
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

    // Stop streaming system in proper order
    // 1. Stop quality monitor first (it depends on stream data)
    if (quality_monitor_) {
      quality_monitor_->stop();
    }

    // 2. Stop the stream (this will stop worker threads)
    if (realtime_stream_) {
      auto stop_result = realtime_stream_->stop();
      (void)stop_result;  // Suppress unused result warning
    }

    // 3. Give threads time to finish cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

 private:
  MonitorConfig config_;
  std::optional<SolarSystem::Bodies::BodyCollection> bodies_;

  // Streaming system components
  std::unique_ptr<RealtimeStream> realtime_stream_;
  std::unique_ptr<FilterChain> filter_chain_;
  std::unique_ptr<StreamAggregator> aggregator_;
  std::unique_ptr<QualityMonitor> quality_monitor_;

  // Streaming statistics
  std::atomic<size_t> total_snapshots_received_{0};
  std::atomic<size_t> total_snapshots_filtered_{0};
  std::atomic<double> current_quality_score_{1.0};
  std::chrono::system_clock::time_point last_data_update_;

  // Display state
  mutable std::mutex display_mutex_;
  DataSnapshot latest_snapshot_;
  AggregateSnapshot latest_aggregate_;
  bool has_streaming_data_ = false;

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
      // Use standardized body set based on configuration
      SolarSystem::Bodies::BodyFactory::DefaultBodySet body_set;
      if (config_.body_set == "essential") {
        body_set = SolarSystem::Bodies::BodyFactory::DefaultBodySet::ESSENTIAL;
      } else if (config_.body_set == "important") {
        body_set = SolarSystem::Bodies::BodyFactory::DefaultBodySet::IMPORTANT;
      } else if (config_.body_set == "complete") {
        body_set = SolarSystem::Bodies::BodyFactory::DefaultBodySet::COMPLETE;
      } else {
        // Default to important set
        body_set = SolarSystem::Bodies::BodyFactory::DefaultBodySet::IMPORTANT;
      }

      auto bodies = BodySelector().body_set(body_set).build(&error);

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
   * @brief Main monitoring loop with streaming data updates
   */
  [[nodiscard]] bool run_monitoring_loop() {
    TerminalStateGuard terminal_guard;

    auto start_time = std::chrono::steady_clock::now();
    auto last_display = start_time;

    size_t display_count = 0;

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

      // Check if it's time to update display
      auto display_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_display);
      if (display_elapsed >= config_.display_interval) {
        display_streaming_state(display_count);
        last_display = now;
        display_count++;
      }

      // Sleep briefly to avoid busy waiting
      std::this_thread::sleep_for(100ms);

    } while (config_.continuous_mode && !g_shutdown_requested.load());

    if (!config_.quiet_mode) {
      std::cout << "\n✨ Monitoring completed successfully!\n";
      std::cout << "📊 Total displays: " << display_count << "\n";

      // Show final streaming statistics
      if (has_streaming_data_) {
        std::cout << "📈 Final Streaming Statistics:\n";
        std::cout << "  Snapshots Received: " << total_snapshots_received_.load() << "\n";
        std::cout << "  Snapshots Filtered: " << total_snapshots_filtered_.load() << "\n";
        std::cout << "  Final Quality Score: " << std::fixed << std::setprecision(3)
                  << current_quality_score_.load() << "\n";

        if (quality_monitor_) {
          std::cout << "  Quality Report:\n";
          auto report = quality_monitor_->get_quality_report();
          std::cout << "    " << report << "\n";
        }
      }
    }

    return true;
  }

  /**
   * @brief Display current streaming state
   */
  void display_streaming_state(size_t display_count) {
    try {
      if (config_.continuous_mode && !config_.quiet_mode) {
        TerminalUI::clear_screen();
        TerminalUI::print_header("Solar System Real-Time Monitor (Streaming)");
      }

      // Status line with streaming information
      std::string status = "Display #" + std::to_string(display_count);

      if (has_streaming_data_) {
        std::lock_guard<std::mutex> lock(display_mutex_);
        status += " │ Bodies: " + std::to_string(latest_snapshot_.data_points.size());
        status += " │ Quality: " + std::to_string(static_cast<int>(latest_snapshot_.overall_quality * 100)) + "%";
        status += " │ Latency: " + std::to_string(latest_snapshot_.processing_time.count()) + "ms";
      } else {
        status += " │ Waiting for streaming data...";
      }

      if (!config_.quiet_mode) {
        TerminalUI::print_status_line(status);
      }

      // Display streaming body information
      if (has_streaming_data_) {
        display_streaming_bodies();

        if (config_.show_summary && !config_.quiet_mode) {
          display_streaming_summary();
        }
      } else {
        if (!config_.quiet_mode) {
          std::cout << "⏳ Initializing streaming data...\n";
          std::cout << "   This may take a few moments while the system\n";
          std::cout << "   establishes connections and begins data flow.\n\n";
        }
      }

    } catch (const std::exception& e) {
      LOG_ERROR("Monitor", "Failed to display streaming state: " + std::string(e.what()));
    }
  }

  /**
   * @brief Display celestial bodies with streaming data
   */
  void display_streaming_bodies() const {
    if (config_.quiet_mode) return;

    std::lock_guard<std::mutex> lock(display_mutex_);

    if (latest_snapshot_.data_points.empty()) {
      std::cout << "⏳ No streaming data available yet...\n\n";
      return;
    }

    std::cout << "🌌 Celestial Bodies (Live Stream):\n";
    std::cout << "┌─────────────────┬─────────────────────────────────────────────┬─────────┬─────────┐\n";
    std::cout << "│ Body            │ Position (km)                               │ Quality │ Latency │";

    if (config_.show_velocities) {
      std::cout << " Velocity (km/s)                         │";
    }
    std::cout << "\n";
    std::cout << "├─────────────────┼─────────────────────────────────────────────┼─────────┼─────────┤\n";

    // Display streaming data points
    for (const auto& point : latest_snapshot_.data_points) {
      std::cout << "│ " << std::setw(15) << std::left << point.body_name << " │ ";

      // Position display (convert from meters to kilometers)
      std::cout << std::fixed << std::setprecision(0);
      std::cout << "(" << std::setw(12) << point.position.x() / 1000.0
                << ", " << std::setw(12) << point.position.y() / 1000.0
                << ", " << std::setw(12) << point.position.z() / 1000.0
                << ")";

      // Quality and latency
      std::cout << " │ " << std::setw(5) << std::setprecision(1)
                << (point.quality_score * 100.0) << "% │ ";
      std::cout << std::setw(5) << point.latency.count() << "ms │";

      if (config_.show_velocities) {
        std::cout << " (" << std::setprecision(2);
        std::cout << std::setw(8) << point.velocity.x() / 1000.0
                  << ", " << std::setw(8) << point.velocity.y() / 1000.0
                  << ", " << std::setw(8) << point.velocity.z() / 1000.0
                  << ") │";
      }

      std::cout << "\n";
    }

    std::cout << "└─────────────────┴─────────────────────────────────────────────┴─────────┴─────────┘\n\n";
  }

  /**
   * @brief Display streaming summary with aggregated data
   */
  void display_streaming_summary() const {
    if (config_.quiet_mode) return;

    std::lock_guard<std::mutex> lock(display_mutex_);

    auto now = std::chrono::system_clock::now();
    auto current_time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&current_time_t);

    std::cout << "📈 Streaming Summary:\n";
    std::cout << "  Current Time: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S %Z") << "\n";

    if (!latest_snapshot_.data_points.empty()) {
      auto snapshot_time_t = std::chrono::system_clock::to_time_t(latest_snapshot_.timestamp);
      auto snapshot_tm = *std::localtime(&snapshot_time_t);
      std::cout << "  Data Time: " << std::put_time(&snapshot_tm, "%Y-%m-%d %H:%M:%S %Z") << "\n";
    }

    std::cout << "  Bodies Tracked: " << latest_snapshot_.data_points.size() << "\n";
    std::cout << "  Overall Quality: " << std::fixed << std::setprecision(1)
              << (latest_snapshot_.overall_quality * 100.0) << "%\n";
    std::cout << "  Processing Time: " << latest_snapshot_.processing_time.count() << "ms\n";

    // Aggregated statistics
    if (aggregator_ && aggregator_->has_sufficient_data()) {
      std::cout << "  📊 Aggregated Statistics:\n";
      std::cout << "    Total Samples: " << latest_aggregate_.total_samples << "\n";
      std::cout << "    Avg Quality: " << std::fixed << std::setprecision(1)
                << (latest_aggregate_.overall_avg_quality * 100.0) << "%\n";
      std::cout << "    Avg Latency: " << latest_aggregate_.overall_avg_latency.count() << "ms\n";

      // Note: system_warnings not implemented in simplified version
      if (latest_aggregate_.total_samples > 0) {
        std::cout << "    📊 Total Samples: " << latest_aggregate_.total_samples << "\n";
      }
    }

    // Quality monitoring status
    if (quality_monitor_ && quality_monitor_->is_running()) {
      auto current_quality = quality_monitor_->get_current_quality_score();
      auto current_latency = quality_monitor_->get_current_latency();

      std::cout << "  🔍 Quality Monitor:\n";
      std::cout << "    Current Score: " << std::fixed << std::setprecision(1)
                << (current_quality * 100.0) << "%\n";
      std::cout << "    Current Latency: " << current_latency.count() << "ms\n";
    }

    if (config_.continuous_mode) {
      std::cout << "  Press Ctrl+C to stop monitoring\n";
    }

    std::cout << "\n";
  }

  /**
   * @brief Display monitoring summary with streaming statistics
   */
  void display_summary() const {
    auto now = std::chrono::system_clock::now();
    auto current_time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&current_time_t);

    std::cout << "📈 Summary:\n";
    std::cout << "  Current Time: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S %Z") << "\n";
    std::cout << "  Bodies Tracked: " << bodies_->size() << "\n";
    std::cout << "  Update Interval: " << config_.update_interval.count() << "s\n";
    std::cout << "  Display Interval: " << config_.display_interval.count() << "s\n";

    // Streaming statistics
    if (has_streaming_data_) {
      std::cout << "  📊 Streaming Statistics:\n";
      std::cout << "    Snapshots Received: " << total_snapshots_received_.load() << "\n";
      std::cout << "    Snapshots Filtered: " << total_snapshots_filtered_.load() << "\n";
      std::cout << "    Current Quality: " << std::fixed << std::setprecision(3)
                << current_quality_score_.load() << "\n";

      if (realtime_stream_) {
        const auto& stats = realtime_stream_->get_stats();
        std::cout << "    Generated: " << stats.total_snapshots_generated.load() << "\n";
        std::cout << "    Delivered: " << stats.total_snapshots_delivered.load() << "\n";
        std::cout << "    Dropped: " << stats.total_snapshots_dropped.load() << "\n";
        std::cout << "    Avg Quality: " << std::fixed << std::setprecision(3)
                  << stats.avg_quality_score << "\n";
      }

      // Time since last update
      if (last_data_update_ != std::chrono::system_clock::time_point{}) {
        auto time_since_update = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_data_update_);
        std::cout << "    Last Update: " << time_since_update.count() << "s ago\n";
      }
    }

    if (config_.continuous_mode) {
      std::cout << "  Press Ctrl+C to stop monitoring\n";
    }

    std::cout << "\n";
  }

  /**
   * @brief Initialize the streaming system components
   */
  void initialize_streaming_system() {
    // Configure streaming
    StreamConfig stream_config;
    stream_config.update_interval = config_.update_interval;
    stream_config.max_buffer_size = 100;
    stream_config.enable_quality_monitoring = true;
    stream_config.min_quality_threshold = 0.7;
    stream_config.use_background_thread = true;

    // Configure real-time streaming
    RealtimeStream::RealtimeConfig realtime_config;
    realtime_config.sync_with_system_time = true;
    realtime_config.enable_prediction = true;
    realtime_config.auto_correct_drift = true;

    // Create streaming components
    realtime_stream_ = std::make_unique<RealtimeStream>(stream_config, realtime_config);
    filter_chain_ = FilterFactory::create_realtime_filter_chain();
    aggregator_ = AggregatorFactory::create_realtime_aggregator(config_.update_interval);
    quality_monitor_ = QualityMonitorFactory::create_realtime_monitor();

    // Set up callbacks
    realtime_stream_->set_data_callback([this](const DataSnapshot& snapshot) {
      handle_data_snapshot(snapshot);
    });

    realtime_stream_->set_error_callback([this](const std::string& error) {
      handle_streaming_error(error);
    });

    realtime_stream_->set_quality_callback([this](const StreamStats& stats) {
      handle_quality_update(stats);
    });

    // Set up quality monitoring callbacks
    quality_monitor_->set_quality_alert_callback([this](const std::string& alert,
                                                        const SnapshotQuality& quality) {
      handle_quality_alert(alert, quality);
    });

    LOG_INFO("RealtimeMonitor", "Streaming system initialized");
  }

  /**
   * @brief Handle incoming data snapshots from the stream
   */
  void handle_data_snapshot(const DataSnapshot& snapshot) {
    total_snapshots_received_.fetch_add(1);

    // Apply filters
    auto filtered_snapshot = filter_chain_->apply(snapshot);
    if (!filtered_snapshot) {
      total_snapshots_filtered_.fetch_add(1);
      return;
    }

    // Update aggregator
    aggregator_->add_snapshot(*filtered_snapshot);

    // Update quality monitor
    quality_monitor_->process_snapshot(*filtered_snapshot);

    // Update display state
    {
      std::lock_guard<std::mutex> lock(display_mutex_);
      latest_snapshot_ = *filtered_snapshot;
      latest_aggregate_ = aggregator_->get_aggregate();
      has_streaming_data_ = true;
      last_data_update_ = std::chrono::system_clock::now();
    }

    current_quality_score_.store(filtered_snapshot->overall_quality);
  }

  /**
   * @brief Handle streaming errors
   */
  void handle_streaming_error(const std::string& error) {
    LOG_ERROR("RealtimeMonitor", "Streaming error: " + error);
    if (config_.verbose_output && !config_.quiet_mode) {
      std::cout << "⚠️  Streaming error: " << error << "\n";
    }
  }

  /**
   * @brief Handle quality updates
   */
  void handle_quality_update(const StreamStats& stats) {
    if (config_.verbose_output) {
      LOG_DEBUG("RealtimeMonitor", "Quality update: avg=" +
                std::to_string(stats.avg_quality_score) +
                ", generated=" + std::to_string(stats.total_snapshots_generated.load()));
    }
  }

  /**
   * @brief Handle quality alerts
   */
  void handle_quality_alert(const std::string& alert, const SnapshotQuality& quality) {
    LOG_WARN("RealtimeMonitor", "Quality alert: " + alert);
    if (!config_.quiet_mode) {
      std::cout << "🚨 Quality Alert: " << alert << " (Score: "
                << std::fixed << std::setprecision(3) << quality.overall_score << ")\n";
    }
  }

  /**
   * @brief Start the streaming system
   */
  [[nodiscard]] bool start_streaming() {
    if (!bodies_.has_value()) {
      LOG_ERROR("RealtimeMonitor", "Cannot start streaming without bodies");
      return false;
    }

    // Set data source for streaming
    auto bodies_ptr = std::make_shared<SolarSystem::Bodies::BodyCollection>(*bodies_);
    auto set_result = realtime_stream_->set_data_source(bodies_ptr);
    if (!set_result) {
      LOG_ERROR("RealtimeMonitor", "Failed to set streaming data source: " + set_result.error());
      return false;
    }

    // Start streaming components
    auto stream_result = realtime_stream_->start();
    if (!stream_result) {
      LOG_ERROR("RealtimeMonitor", "Failed to start stream: " + stream_result.error());
      return false;
    }

    if (!quality_monitor_->start()) {
      LOG_ERROR("RealtimeMonitor", "Failed to start quality monitor");
      return false;
    }

    LOG_INFO("RealtimeMonitor", "Streaming system started successfully");
    return true;
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

    SolarSystem::Bodies::BodyFactory factory;

    // Parse command-line arguments using modern parser
    RealtimeArgumentParser parser(argv[0]);
    auto result = parser.parse(argc, const_cast<const char* const*>(argv));
    if (!result) {
      std::cerr << "Error parsing arguments: " << to_string(result.error()) << std::endl;
      parser.print_usage();
      return 1;
    }

    auto config = result.value();

    // Initialize logging system
    Logger::Config log_config;
    log_config.min_level = config.verbose_output ? Logger::Level::DEBUG : Logger::Level::INFO;
    log_config.colored_output = true;
    log_config.include_timestamp = true;
    Logger::instance().configure(log_config);

    LOG_INFO("Main", "Solar System Real-Time Monitor (Modern) starting");

    // Create and start monitor
    RealtimeMonitor monitor(config);
    bool success = monitor.start(factory);

    if (success) {
      LOG_INFO("Main", "Real-time monitoring completed successfully");
      if (!config.quiet_mode) {
        std::cout << "\n🎉 Real-time monitoring session completed!\n";
      }
    } else {
      LOG_ERROR("Main", "Real-time monitoring failed");
      if (!config.quiet_mode) {
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
