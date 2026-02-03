#pragma once

/**
 * @file data_processor.hpp
 * @brief Data loading and preparation system for analysis
 */

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <solar_analysis/export.hpp>
#include <solar_core/math/vector3.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Timestamped state vector for a celestial body
 */
struct SOLAR_ANALYSIS_API StateVector {
  std::chrono::system_clock::time_point timestamp;
  Math::Vector3d position;  // km
  Math::Vector3d velocity;  // km/s

  [[nodiscard]] bool is_valid() const noexcept {
    return timestamp != std::chrono::system_clock::time_point{};
  }
};

/**
 * @brief Data quality assessment result
 */
struct SOLAR_ANALYSIS_API DataQuality {
  size_t total_points = 0;
  size_t valid_points = 0;
  size_t invalid_points = 0;
  double completeness = 0.0;  // 0.0 to 1.0
  std::vector<std::string> issues;

  [[nodiscard]] bool is_acceptable(double threshold = 0.9) const noexcept {
    return completeness >= threshold;
  }
};

/**
 * @brief Time range specification for data queries
 */
struct SOLAR_ANALYSIS_API TimeRange {
  std::chrono::system_clock::time_point start;
  std::chrono::system_clock::time_point end;

  [[nodiscard]] bool contains(std::chrono::system_clock::time_point t) const noexcept {
    return t >= start && t <= end;
  }

  [[nodiscard]] std::chrono::duration<double> duration() const noexcept { return end - start; }
};

/**
 * @brief Configuration for data processing operations
 */
struct SOLAR_ANALYSIS_API DataProcessorConfig {
  bool enable_caching = true;
  bool validate_data = true;
  bool interpolate_gaps = false;
  std::chrono::seconds max_gap_size{3600};  // Max gap to interpolate (1 hour)
  double quality_threshold = 0.9;
};

/**
 * @brief Data processor for loading and preparing ephemeris data
 */
class SOLAR_ANALYSIS_API DataProcessor {
 public:
  DataProcessor();
  explicit DataProcessor(const DataProcessorConfig& config);
  ~DataProcessor();

  DataProcessor(const DataProcessor&) = delete;
  DataProcessor& operator=(const DataProcessor&) = delete;
  DataProcessor(DataProcessor&&) noexcept;
  DataProcessor& operator=(DataProcessor&&) noexcept;

  // Configuration
  void set_config(const DataProcessorConfig& config);
  [[nodiscard]] const DataProcessorConfig& config() const;

  // Data loading
  [[nodiscard]] bool load_body_data(const std::string& body_name);
  [[nodiscard]] bool load_body_data(const std::string& body_name, const TimeRange& range);

  // Data access
  [[nodiscard]] std::vector<StateVector> get_data(const std::string& body_name) const;
  [[nodiscard]] std::vector<StateVector> get_data(const std::string& body_name,
                                                  const TimeRange& range) const;
  [[nodiscard]] std::optional<StateVector> get_state_at(
      const std::string& body_name, std::chrono::system_clock::time_point time) const;

  // Data validation
  [[nodiscard]] DataQuality validate_data(const std::string& body_name) const;
  [[nodiscard]] DataQuality validate_data(const std::vector<StateVector>& data) const;

  // Interpolation
  [[nodiscard]] std::optional<StateVector> interpolate(
      const StateVector& before, const StateVector& after,
      std::chrono::system_clock::time_point t) const;

  // Cache management
  void clear_cache();
  void clear_cache(const std::string& body_name);
  [[nodiscard]] size_t cache_size() const;
  [[nodiscard]] std::vector<std::string> cached_bodies() const;

  // Status
  [[nodiscard]] bool has_data(const std::string& body_name) const;
  [[nodiscard]] size_t data_point_count(const std::string& body_name) const;
  [[nodiscard]] std::optional<TimeRange> data_time_range(const std::string& body_name) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace SolarSystem::Analysis
