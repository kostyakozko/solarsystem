#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "solar_core/utils/expected.hpp"
#include "solar_utils/logging.hpp"

namespace SolarSystem::Utils {

/**
 * @brief Modern configuration system with YAML/JSON support
 *
 * Provides centralized configuration management with type safety,
 * validation, and multiple data sources (files, environment, CLI).
 */
class Config {
 public:
  /**
   * @brief Configuration value types
   */
  using Value = std::variant<bool, int64_t, double, std::string>;
  using ConfigMap = std::unordered_map<std::string, Value>;

  /**
   * @brief Configuration sources in priority order
   */
  enum class Source {
    DEFAULT = 0,
    CONFIG_FILE = 1,
    ENVIRONMENT = 2,
    COMMAND_LINE = 3  // Highest priority
  };

  /**
   * @brief Simulation-specific configuration
   */
  struct SimulationConfig {
    double timestep = 3600.0;  // seconds
    size_t max_iterations = 1000000;
    bool enable_progress = true;
    bool verbose_output = false;
    std::string output_format = "standard";  // standard, json, csv

    // Physics parameters
    double gravitational_constant = 6.67430e-11;
    double convergence_threshold = 1e-12;

    // Performance settings
    size_t thread_count = 0;  // 0 = auto-detect
    bool enable_simd = true;
    bool enable_lto = true;
  };

  /**
   * @brief Data source configuration
   */
  struct DataConfig {
    std::string jpl_api_url = "https://ssd.jpl.nasa.gov/api/horizons.api";
    std::string cache_directory = "./cache";
    size_t cache_max_age_days = 30;
    bool allow_fallback_data = true;
    bool auto_update_cache = true;

    // Network settings
    std::chrono::seconds network_timeout{30};
    size_t max_retries = 3;
    bool verify_ssl = true;
  };

  /**
   * @brief Logging configuration
   */
  struct LoggingConfig {
    Logger::Level min_level = Logger::Level::INFO;
    Logger::Output output = Logger::Output::CONSOLE;
    std::string log_file = "solar_system.log";
    bool include_timestamp = true;
    bool include_thread_id = false;
    bool colored_output = true;
    size_t max_log_file_size_mb = 100;
    size_t max_log_files = 5;
  };

  /**
   * @brief Web server configuration
   */
  struct WebConfig {
    uint16_t port = 8080;
    std::string host = "localhost";
    std::string web_root = "./web";
    bool enable_cors = true;
    bool enable_compression = true;
    size_t max_connections = 100;
    std::chrono::seconds request_timeout{30};
  };

  /**
   * @brief Complete application configuration
   */
  struct AppConfig {
    SimulationConfig simulation;
    DataConfig data;
    LoggingConfig logging;
    WebConfig web;

    // Global settings
    std::string app_name = "Solar System Suite";
    std::string version = "4.0.0";
    bool debug_mode = false;
  };

  /**
   * @brief Load configuration from multiple sources
   */
  static Expected<AppConfig, std::string> load(
      const std::optional<std::string>& config_file = std::nullopt,
      const std::vector<std::string>& cli_args = {});

  /**
   * @brief Save configuration to file
   */
  static Expected<void, std::string> save(const AppConfig& config, const std::string& filename);

  /**
   * @brief Get default configuration
   */
  static AppConfig get_default();

  /**
   * @brief Validate configuration
   */
  static Expected<void, std::string> validate(const AppConfig& config);

  /**
   * @brief Merge configurations (higher priority overwrites lower)
   */
  static AppConfig merge(const AppConfig& base, const AppConfig& override);

 private:
  /**
   * @brief Load from JSON/YAML file
   */
  static Expected<ConfigMap, std::string> load_from_file(const std::string& filename);

  /**
   * @brief Load from environment variables
   */
  static ConfigMap load_from_environment();

  /**
   * @brief Load from command line arguments
   */
  static ConfigMap load_from_cli(const std::vector<std::string>& args);

  /**
   * @brief Convert ConfigMap to AppConfig
   */
  static Expected<AppConfig, std::string> map_to_config(const ConfigMap& map);

  /**
   * @brief Convert AppConfig to ConfigMap
   */
  static ConfigMap config_to_map(const AppConfig& config);

  /**
   * @brief Get value from map with type checking
   */
  template <typename T>
  static std::optional<T> get_value(const ConfigMap& map, const std::string& key);

  /**
   * @brief Set value in map with type conversion
   */
  template <typename T>
  static void set_value(ConfigMap& map, const std::string& key, const T& value);
};

/**
 * @brief Global configuration instance
 */
class GlobalConfig {
 public:
  static Config::AppConfig& instance();
  static void initialize(const Config::AppConfig& config);
  static bool is_initialized();

 private:
  static std::optional<Config::AppConfig> config_;
  static bool initialized_;
};

/**
 * @brief Convenience macros for accessing global config
 */
#define SIMULATION_CONFIG() GlobalConfig::instance().simulation
#define DATA_CONFIG() GlobalConfig::instance().data
#define LOGGING_CONFIG() GlobalConfig::instance().logging
#define WEB_CONFIG() GlobalConfig::instance().web

}  // namespace SolarSystem::Utils
