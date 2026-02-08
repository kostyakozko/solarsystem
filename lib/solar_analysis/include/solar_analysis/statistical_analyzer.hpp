#pragma once

/**
 * @file statistical_analyzer.hpp
 * @brief Statistical analysis tools for celestial body data
 */

#include <algorithm>
#include <cmath>
#include <optional>
#include <solar_analysis/export.hpp>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Summary statistics result
 */
struct SOLAR_ANALYSIS_API SummaryStatistics {
  double mean = 0.0;
  double median = 0.0;
  double variance = 0.0;
  double std_dev = 0.0;
  double min = 0.0;
  double max = 0.0;
  double skewness = 0.0;
  double kurtosis = 0.0;
  size_t count = 0;
};

/**
 * @brief Correlation result
 */
struct SOLAR_ANALYSIS_API CorrelationResult {
  double coefficient = 0.0;  // Pearson correlation [-1, 1]
  double p_value = 0.0;
  bool significant = false;
};

/**
 * @brief Linear regression result
 */
struct SOLAR_ANALYSIS_API RegressionResult {
  double slope = 0.0;
  double intercept = 0.0;
  double r_squared = 0.0;
  double std_error = 0.0;
};

/**
 * @brief Anomaly detection result
 */
struct SOLAR_ANALYSIS_API Anomaly {
  size_t index = 0;
  double value = 0.0;
  double score = 0.0;  // Number of std devs from mean
  double threshold = 0.0;
};

/**
 * @brief Time series trend result
 */
struct SOLAR_ANALYSIS_API TrendResult {
  double slope = 0.0;
  double direction = 0.0;  // +1 increasing, -1 decreasing, 0 stable
  double strength = 0.0;   // 0 to 1
};

/**
 * @brief Statistical analyzer for data analysis
 */
class SOLAR_ANALYSIS_API StatisticalAnalyzer {
 public:
  // Summary statistics
  [[nodiscard]] SummaryStatistics calculate_statistics(const std::vector<double>& data) const;

  // Individual calculations
  [[nodiscard]] double mean(const std::vector<double>& data) const;
  [[nodiscard]] double median(std::vector<double> data) const;
  [[nodiscard]] double variance(const std::vector<double>& data) const;
  [[nodiscard]] double std_deviation(const std::vector<double>& data) const;
  [[nodiscard]] double skewness(const std::vector<double>& data) const;
  [[nodiscard]] double kurtosis(const std::vector<double>& data) const;

  // Correlation and regression
  [[nodiscard]] CorrelationResult correlation(const std::vector<double>& x,
                                              const std::vector<double>& y) const;
  [[nodiscard]] RegressionResult linear_regression(const std::vector<double>& x,
                                                   const std::vector<double>& y) const;
  [[nodiscard]] std::vector<double> cross_correlation(const std::vector<double>& x,
                                                      const std::vector<double>& y,
                                                      int max_lag) const;

  // Anomaly detection
  [[nodiscard]] std::vector<Anomaly> detect_anomalies(const std::vector<double>& data,
                                                      double sigma_threshold = 3.0) const;

  // Time series
  [[nodiscard]] TrendResult detect_trend(const std::vector<double>& data) const;
  [[nodiscard]] std::vector<double> moving_average(const std::vector<double>& data,
                                                   size_t window) const;
  [[nodiscard]] std::vector<double> detrend(const std::vector<double>& data) const;
};

}  // namespace SolarSystem::Analysis
