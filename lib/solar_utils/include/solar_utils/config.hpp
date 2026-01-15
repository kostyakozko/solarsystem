#pragma once

#include "solar_utils/export.hpp"

#include <chrono>
#include <filesystem>
#include <string>

namespace SolarSystem::Utils {

/**
 * @brief Configuration log levels (separate from Logger::Level to avoid ODR violations)
 */
enum class ConfigLogLevel { DEBUG, INFO, WARNING, ERROR };

/**
 * @brief Configuration structures
 */
namespace Config {

/**
 * @brief Simulation configuration
 */
struct SimulationConfig {
  double timestep = 3600.0;           // Default: 1 hour
  int max_iterations = 1000000;       // Default: 1M iterations
  bool enable_adaptive_timestep = false;
  double tolerance = 1e-12;
  bool enable_collision_detection = false;
};

/**
 * @brief Logging configuration
 */
struct LoggingConfig {
  ConfigLogLevel min_level = ConfigLogLevel::INFO;
  bool console_output = true;
  bool colored_output = true;
  std::string log_file;
  bool enable_file_logging = false;
  size_t max_file_size = 10 * 1024 * 1024;  // 10MB
  int max_backup_files = 5;
};

/**
 * @brief Output configuration
 */
struct OutputConfig {
  std::string format = "json";
  std::filesystem::path output_directory = "output";
  bool compress_output = false;
  bool include_metadata = true;
  int precision = 12;
};

/**
 * @brief Performance configuration
 */
struct PerformanceConfig {
  int thread_count = 0;  // 0 = auto-detect
  bool enable_simd = true;
  bool enable_gpu_acceleration = false;
  size_t memory_limit = 0;  // 0 = no limit
};

/**
 * @brief Network configuration
 */
struct NetworkConfig {
  std::string jpl_base_url = "https://ssd.jpl.nasa.gov/api/horizons.api";
  std::chrono::seconds timeout{30};
  int max_retries = 3;
  bool enable_caching = true;
  std::chrono::hours cache_expiry{24};
};

/**
 * @brief Application configuration
 */
struct AppConfig {
  std::string version = "4.0.0";
  bool debug_mode = false;

  SimulationConfig simulation;
  LoggingConfig logging;
  OutputConfig output;
  PerformanceConfig performance;
  NetworkConfig network;
};

/**
 * @brief Get default configuration
 */
SOLAR_UTILS_API AppConfig get_default();

/**
 * @brief Load configuration from file
 */
SOLAR_UTILS_API AppConfig load_from_file(const std::filesystem::path& config_file);

/**
 * @brief Save configuration to file
 */
SOLAR_UTILS_API void save_to_file(const AppConfig& config, const std::filesystem::path& config_file);

}  // namespace Config
}  // namespace SolarSystem::Utils
