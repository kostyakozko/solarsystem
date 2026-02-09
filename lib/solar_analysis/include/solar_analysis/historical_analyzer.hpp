#pragma once

/**
 * @file historical_analyzer.hpp
 * @brief Historical data analysis and pattern detection
 */

#include <chrono>
#include <map>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>
#include <solar_analysis/orbital_calculator.hpp>
#include <solar_analysis/statistical_analyzer.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Long-term trend analysis result
 */
struct SOLAR_ANALYSIS_API LongTermTrend {
  std::string parameter;
  double annual_change = 0.0;
  double total_change = 0.0;
  double confidence = 0.0;
  TrendResult trend;
  std::vector<double> yearly_values;
};

/**
 * @brief Period comparison result
 */
struct SOLAR_ANALYSIS_API PeriodComparison {
  TimeRange period1;
  TimeRange period2;
  double mean_difference = 0.0;
  double percent_change = 0.0;
  bool statistically_significant = false;
  double p_value = 0.0;
};

/**
 * @brief Cyclical pattern result
 */
struct SOLAR_ANALYSIS_API CyclicalPattern {
  double period = 0.0;  // seconds
  double amplitude = 0.0;
  double phase = 0.0;
  double strength = 0.0;  // 0 to 1
  std::string description;
};

/**
 * @brief Prediction validation result
 */
struct SOLAR_ANALYSIS_API PredictionValidation {
  double mean_error = 0.0;
  double rmse = 0.0;  // Root mean square error
  double mae = 0.0;   // Mean absolute error
  double mape = 0.0;  // Mean absolute percentage error
  double r_squared = 0.0;
  std::vector<double> errors;
  std::vector<double> confidence_intervals;
};

/**
 * @brief Historical data analyzer
 */
class SOLAR_ANALYSIS_API HistoricalAnalyzer {
 public:
  HistoricalAnalyzer();
  ~HistoricalAnalyzer();

  HistoricalAnalyzer(const HistoricalAnalyzer&) = delete;
  HistoricalAnalyzer& operator=(const HistoricalAnalyzer&) = delete;

  // Long-term trend analysis
  [[nodiscard]] LongTermTrend analyze_long_term_trend(const std::vector<StateVector>& data,
                                                      const std::string& parameter) const;

  [[nodiscard]] std::vector<LongTermTrend> analyze_orbital_evolution(
      const std::vector<StateVector>& data) const;

  // Period comparison
  [[nodiscard]] PeriodComparison compare_periods(const std::vector<StateVector>& data,
                                                 const TimeRange& period1,
                                                 const TimeRange& period2) const;

  [[nodiscard]] std::map<std::string, double> establish_baseline(
      const std::vector<StateVector>& data, const TimeRange& baseline_period) const;

  [[nodiscard]] std::map<std::string, double> compare_to_baseline(
      const std::vector<StateVector>& data, const std::map<std::string, double>& baseline) const;

  // Cyclical pattern detection
  [[nodiscard]] std::vector<CyclicalPattern> detect_cycles(const std::vector<double>& data,
                                                           double sample_rate) const;

  [[nodiscard]] std::vector<double> seasonal_decompose(const std::vector<double>& data,
                                                       size_t period) const;

  [[nodiscard]] std::vector<double> extract_seasonal(const std::vector<double>& data,
                                                     size_t period) const;

  // Prediction validation
  [[nodiscard]] PredictionValidation validate_predictions(const std::vector<double>& predicted,
                                                          const std::vector<double>& actual) const;

  [[nodiscard]] std::vector<double> calculate_confidence_intervals(const std::vector<double>& data,
                                                                   double confidence = 0.95) const;

  [[nodiscard]] double calculate_prediction_accuracy(const OrbitalElements& predicted,
                                                     const OrbitalElements& actual) const;

 private:
  StatisticalAnalyzer stats_;
  OrbitalCalculator orbital_calc_;

  [[nodiscard]] std::vector<double> extract_parameter(const std::vector<StateVector>& data,
                                                      const std::string& parameter) const;
};

}  // namespace SolarSystem::Analysis
