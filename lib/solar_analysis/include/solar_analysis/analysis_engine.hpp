#pragma once

/**
 * @file analysis_engine.hpp
 * @brief Core analysis engine for Solar System data analysis
 */

#include <chrono>
#include <memory>
#include <optional>
#include <solar_analysis/export.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Configuration for analysis operations
 */
struct SOLAR_ANALYSIS_API AnalysisConfig {
  std::string data_source = "jpl";
  std::chrono::system_clock::time_point start_time;
  std::chrono::system_clock::time_point end_time;
  bool verbose = false;
  size_t max_data_points = 10000;
};

/**
 * @brief Result of an analysis operation
 */
struct SOLAR_ANALYSIS_API AnalysisResult {
  bool success = false;
  std::string message;
  std::vector<double> values;
  std::chrono::milliseconds duration{0};
};

/**
 * @brief Core analysis engine for celestial body data
 */
class SOLAR_ANALYSIS_API AnalysisEngine {
 public:
  AnalysisEngine();
  explicit AnalysisEngine(const AnalysisConfig& config);
  ~AnalysisEngine();

  // Configuration
  void set_config(const AnalysisConfig& config);
  [[nodiscard]] const AnalysisConfig& config() const;

  // Data loading
  [[nodiscard]] bool load_data(const std::string& body_name);
  [[nodiscard]] bool load_data(const std::vector<std::string>& body_names);
  [[nodiscard]] size_t data_point_count() const;

  // Basic analysis
  [[nodiscard]] AnalysisResult analyze();
  [[nodiscard]] bool is_ready() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Analysis
