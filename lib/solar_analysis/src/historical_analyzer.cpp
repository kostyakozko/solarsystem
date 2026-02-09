/**
 * @file historical_analyzer.cpp
 * @brief Implementation of historical data analysis
 */

#include "solar_analysis/historical_analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace SolarSystem::Analysis {

HistoricalAnalyzer::HistoricalAnalyzer() = default;
HistoricalAnalyzer::~HistoricalAnalyzer() = default;

std::vector<double> HistoricalAnalyzer::extract_parameter(const std::vector<StateVector>& data,
                                                          const std::string& parameter) const {
  std::vector<double> values;
  values.reserve(data.size());

  for (const auto& sv : data) {
    if (parameter == "distance" || parameter == "r") {
      values.push_back(static_cast<double>(sv.position.magnitude()));
    } else if (parameter == "velocity" || parameter == "v") {
      values.push_back(static_cast<double>(sv.velocity.magnitude()));
    } else if (parameter == "x") {
      values.push_back(sv.position.x());
    } else if (parameter == "y") {
      values.push_back(sv.position.y());
    } else if (parameter == "z") {
      values.push_back(sv.position.z());
    }
  }
  return values;
}

LongTermTrend HistoricalAnalyzer::analyze_long_term_trend(const std::vector<StateVector>& data,
                                                          const std::string& parameter) const {
  LongTermTrend result;
  result.parameter = parameter;

  auto values = extract_parameter(data, parameter);
  if (values.size() < 2) return result;

  result.trend = stats_.detect_trend(values);

  // Calculate yearly values (assuming data spans multiple years)
  size_t points_per_year = values.size() / 10;  // Approximate
  if (points_per_year > 0) {
    for (size_t i = 0; i < values.size(); i += points_per_year) {
      size_t end = std::min(i + points_per_year, values.size());
      double sum = std::accumulate(values.begin() + static_cast<long>(i),
                                   values.begin() + static_cast<long>(end), 0.0);
      result.yearly_values.push_back(sum / static_cast<double>(end - i));
    }
  }

  if (!result.yearly_values.empty()) {
    result.annual_change = result.trend.slope * static_cast<double>(points_per_year);
    result.total_change = values.back() - values.front();
  }

  result.confidence = result.trend.strength;
  return result;
}

std::vector<LongTermTrend> HistoricalAnalyzer::analyze_orbital_evolution(
    const std::vector<StateVector>& data) const {
  std::vector<LongTermTrend> trends;

  trends.push_back(analyze_long_term_trend(data, "distance"));
  trends.push_back(analyze_long_term_trend(data, "velocity"));

  return trends;
}

PeriodComparison HistoricalAnalyzer::compare_periods(const std::vector<StateVector>& data,
                                                     const TimeRange& period1,
                                                     const TimeRange& period2) const {
  PeriodComparison result;
  result.period1 = period1;
  result.period2 = period2;

  std::vector<double> values1, values2;

  for (const auto& sv : data) {
    double dist = static_cast<double>(sv.position.magnitude());
    if (period1.contains(sv.timestamp)) {
      values1.push_back(dist);
    } else if (period2.contains(sv.timestamp)) {
      values2.push_back(dist);
    }
  }

  if (values1.empty() || values2.empty()) return result;

  double mean1 = stats_.mean(values1);
  double mean2 = stats_.mean(values2);

  result.mean_difference = mean2 - mean1;
  result.percent_change = (mean1 != 0) ? (result.mean_difference / mean1) * 100.0 : 0.0;

  // Simple t-test approximation
  double var1 = stats_.variance(values1);
  double var2 = stats_.variance(values2);
  double se = std::sqrt(var1 / static_cast<double>(values1.size()) +
                        var2 / static_cast<double>(values2.size()));

  if (se > 1e-10) {
    double t = std::abs(result.mean_difference) / se;
    result.p_value = 2.0 * std::exp(-0.5 * t * t);  // Simplified
    result.statistically_significant = result.p_value < 0.05;
  }

  return result;
}

std::map<std::string, double> HistoricalAnalyzer::establish_baseline(
    const std::vector<StateVector>& data, const TimeRange& baseline_period) const {
  std::map<std::string, double> baseline;

  std::vector<double> distances, velocities;
  for (const auto& sv : data) {
    if (baseline_period.contains(sv.timestamp)) {
      distances.push_back(static_cast<double>(sv.position.magnitude()));
      velocities.push_back(static_cast<double>(sv.velocity.magnitude()));
    }
  }

  if (!distances.empty()) {
    baseline["mean_distance"] = stats_.mean(distances);
    baseline["std_distance"] = stats_.std_deviation(distances);
    baseline["mean_velocity"] = stats_.mean(velocities);
    baseline["std_velocity"] = stats_.std_deviation(velocities);
  }

  return baseline;
}

std::map<std::string, double> HistoricalAnalyzer::compare_to_baseline(
    const std::vector<StateVector>& data, const std::map<std::string, double>& baseline) const {
  std::map<std::string, double> comparison;

  std::vector<double> distances, velocities;
  for (const auto& sv : data) {
    distances.push_back(static_cast<double>(sv.position.magnitude()));
    velocities.push_back(static_cast<double>(sv.velocity.magnitude()));
  }

  if (!distances.empty() && baseline.count("mean_distance")) {
    double current_mean = stats_.mean(distances);
    comparison["distance_deviation"] = current_mean - baseline.at("mean_distance");
    comparison["distance_percent_change"] =
        (baseline.at("mean_distance") != 0)
            ? (comparison["distance_deviation"] / baseline.at("mean_distance")) * 100.0
            : 0.0;
  }

  if (!velocities.empty() && baseline.count("mean_velocity")) {
    double current_mean = stats_.mean(velocities);
    comparison["velocity_deviation"] = current_mean - baseline.at("mean_velocity");
    comparison["velocity_percent_change"] =
        (baseline.at("mean_velocity") != 0)
            ? (comparison["velocity_deviation"] / baseline.at("mean_velocity")) * 100.0
            : 0.0;
  }

  return comparison;
}

std::vector<CyclicalPattern> HistoricalAnalyzer::detect_cycles(const std::vector<double>& data,
                                                               double sample_rate) const {
  std::vector<CyclicalPattern> patterns;
  if (data.size() < 4) return patterns;

  // Simple autocorrelation-based cycle detection
  auto xcorr = stats_.cross_correlation(data, data, static_cast<int>(data.size() / 2));

  // Find peaks in autocorrelation (excluding lag 0)
  for (size_t i = 2; i < xcorr.size() - 1; ++i) {
    if (xcorr[i] > xcorr[i - 1] && xcorr[i] > xcorr[i + 1] && xcorr[i] > 0.3) {
      CyclicalPattern pattern;
      int lag = static_cast<int>(i) - static_cast<int>(data.size() / 2);
      pattern.period = std::abs(lag) / sample_rate;
      pattern.strength = xcorr[i];
      pattern.amplitude = stats_.std_deviation(data);
      pattern.description = "Detected cycle at lag " + std::to_string(lag);
      patterns.push_back(pattern);
    }
  }

  std::sort(
      patterns.begin(), patterns.end(),
      [](const CyclicalPattern& a, const CyclicalPattern& b) { return a.strength > b.strength; });

  return patterns;
}

std::vector<double> HistoricalAnalyzer::seasonal_decompose(const std::vector<double>& data,
                                                           size_t period) const {
  if (data.size() < period * 2) return data;

  // Extract trend using moving average
  auto trend = stats_.moving_average(data, period);

  // Detrended = original - trend
  std::vector<double> detrended;
  size_t offset = period / 2;
  for (size_t i = 0; i < trend.size() && (i + offset) < data.size(); ++i) {
    detrended.push_back(data[i + offset] - trend[i]);
  }

  return detrended;
}

std::vector<double> HistoricalAnalyzer::extract_seasonal(const std::vector<double>& data,
                                                         size_t period) const {
  if (data.size() < period) return {};

  // Average values at each position in the cycle
  std::vector<double> seasonal(period, 0.0);
  std::vector<size_t> counts(period, 0);

  for (size_t i = 0; i < data.size(); ++i) {
    size_t pos = i % period;
    seasonal[pos] += data[i];
    counts[pos]++;
  }

  for (size_t i = 0; i < period; ++i) {
    if (counts[i] > 0) {
      seasonal[i] /= static_cast<double>(counts[i]);
    }
  }

  return seasonal;
}

PredictionValidation HistoricalAnalyzer::validate_predictions(
    const std::vector<double>& predicted, const std::vector<double>& actual) const {
  PredictionValidation result;

  size_t n = std::min(predicted.size(), actual.size());
  if (n == 0) return result;

  double sum_error = 0.0, sum_sq_error = 0.0, sum_abs_error = 0.0, sum_pct_error = 0.0;

  for (size_t i = 0; i < n; ++i) {
    double error = predicted[i] - actual[i];
    result.errors.push_back(error);
    sum_error += error;
    sum_sq_error += error * error;
    sum_abs_error += std::abs(error);
    if (std::abs(actual[i]) > 1e-10) {
      sum_pct_error += std::abs(error / actual[i]);
    }
  }

  double dn = static_cast<double>(n);
  result.mean_error = sum_error / dn;
  result.rmse = std::sqrt(sum_sq_error / dn);
  result.mae = sum_abs_error / dn;
  result.mape = (sum_pct_error / dn) * 100.0;

  // R-squared
  double mean_actual = stats_.mean(actual);
  double ss_tot = 0.0, ss_res = 0.0;
  for (size_t i = 0; i < n; ++i) {
    ss_res += (actual[i] - predicted[i]) * (actual[i] - predicted[i]);
    ss_tot += (actual[i] - mean_actual) * (actual[i] - mean_actual);
  }
  result.r_squared = (ss_tot > 1e-10) ? 1.0 - ss_res / ss_tot : 0.0;

  return result;
}

std::vector<double> HistoricalAnalyzer::calculate_confidence_intervals(
    const std::vector<double>& data, double confidence) const {
  std::vector<double> intervals;
  if (data.empty()) return intervals;

  double mean = stats_.mean(data);
  double std_dev = stats_.std_deviation(data);
  double n = static_cast<double>(data.size());

  // Z-score for confidence level (simplified)
  double z = (confidence >= 0.99) ? 2.576 : (confidence >= 0.95) ? 1.96 : 1.645;

  double margin = z * std_dev / std::sqrt(n);
  intervals.push_back(mean - margin);  // Lower bound
  intervals.push_back(mean + margin);  // Upper bound

  return intervals;
}

double HistoricalAnalyzer::calculate_prediction_accuracy(const OrbitalElements& predicted,
                                                         const OrbitalElements& actual) const {
  // Calculate relative errors for key parameters
  double errors = 0.0;
  int count = 0;

  auto rel_error = [](double pred, double act) {
    return (std::abs(act) > 1e-10) ? std::abs(pred - act) / std::abs(act) : 0.0;
  };

  errors += rel_error(predicted.semi_major_axis, actual.semi_major_axis);
  errors += rel_error(predicted.eccentricity, actual.eccentricity);
  errors += rel_error(predicted.orbital_period, actual.orbital_period);
  count = 3;

  double mean_error = errors / count;
  return std::max(0.0, 1.0 - mean_error);  // Accuracy as 1 - error
}

}  // namespace SolarSystem::Analysis
