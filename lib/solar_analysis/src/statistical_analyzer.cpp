/**
 * @file statistical_analyzer.cpp
 * @brief Implementation of statistical analysis tools
 */

#include "solar_analysis/statistical_analyzer.hpp"

#include <cmath>
#include <numeric>

namespace SolarSystem::Analysis {

SummaryStatistics StatisticalAnalyzer::calculate_statistics(const std::vector<double>& data) const {
  SummaryStatistics stats;
  if (data.empty()) return stats;

  stats.count = data.size();
  stats.mean = mean(data);
  stats.median = median(data);
  stats.variance = variance(data);
  stats.std_dev = std::sqrt(stats.variance);
  stats.min = *std::min_element(data.begin(), data.end());
  stats.max = *std::max_element(data.begin(), data.end());
  stats.skewness = skewness(data);
  stats.kurtosis = kurtosis(data);

  return stats;
}

double StatisticalAnalyzer::mean(const std::vector<double>& data) const {
  if (data.empty()) return 0.0;
  return std::accumulate(data.begin(), data.end(), 0.0) / static_cast<double>(data.size());
}

double StatisticalAnalyzer::median(std::vector<double> data) const {
  if (data.empty()) return 0.0;
  std::sort(data.begin(), data.end());
  size_t n = data.size();
  if (n % 2 == 0) {
    return (data[n / 2 - 1] + data[n / 2]) / 2.0;
  }
  return data[n / 2];
}

double StatisticalAnalyzer::variance(const std::vector<double>& data) const {
  if (data.size() < 2) return 0.0;
  double m = mean(data);
  double sum = 0.0;
  for (double x : data) {
    double diff = x - m;
    sum += diff * diff;
  }
  return sum / static_cast<double>(data.size() - 1);
}

double StatisticalAnalyzer::std_deviation(const std::vector<double>& data) const {
  return std::sqrt(variance(data));
}

double StatisticalAnalyzer::skewness(const std::vector<double>& data) const {
  if (data.size() < 3) return 0.0;
  double m = mean(data);
  double s = std_deviation(data);
  if (s < 1e-10) return 0.0;

  double sum = 0.0;
  for (double x : data) {
    double z = (x - m) / s;
    sum += z * z * z;
  }
  return sum / static_cast<double>(data.size());
}

double StatisticalAnalyzer::kurtosis(const std::vector<double>& data) const {
  if (data.size() < 4) return 0.0;
  double m = mean(data);
  double s = std_deviation(data);
  if (s < 1e-10) return 0.0;

  double sum = 0.0;
  for (double x : data) {
    double z = (x - m) / s;
    sum += z * z * z * z;
  }
  return sum / static_cast<double>(data.size()) - 3.0;  // Excess kurtosis
}

CorrelationResult StatisticalAnalyzer::correlation(const std::vector<double>& x,
                                                   const std::vector<double>& y) const {
  CorrelationResult result;
  if (x.size() != y.size() || x.size() < 2) return result;

  double mx = mean(x);
  double my = mean(y);
  double sum_xy = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;

  for (size_t i = 0; i < x.size(); ++i) {
    double dx = x[i] - mx;
    double dy = y[i] - my;
    sum_xy += dx * dy;
    sum_x2 += dx * dx;
    sum_y2 += dy * dy;
  }

  double denom = std::sqrt(sum_x2 * sum_y2);
  if (denom < 1e-10) return result;

  result.coefficient = sum_xy / denom;

  // Approximate p-value using t-distribution
  double n = static_cast<double>(x.size());
  double t =
      result.coefficient * std::sqrt((n - 2) / (1 - result.coefficient * result.coefficient));
  result.p_value = 2.0 * std::exp(-0.5 * t * t);  // Simplified
  result.significant = result.p_value < 0.05;

  return result;
}

RegressionResult StatisticalAnalyzer::linear_regression(const std::vector<double>& x,
                                                        const std::vector<double>& y) const {
  RegressionResult result;
  if (x.size() != y.size() || x.size() < 2) return result;

  double mx = mean(x);
  double my = mean(y);
  double sum_xy = 0.0, sum_x2 = 0.0;

  for (size_t i = 0; i < x.size(); ++i) {
    double dx = x[i] - mx;
    sum_xy += dx * (y[i] - my);
    sum_x2 += dx * dx;
  }

  if (sum_x2 < 1e-10) return result;

  result.slope = sum_xy / sum_x2;
  result.intercept = my - result.slope * mx;

  // Calculate R-squared
  double ss_tot = 0.0, ss_res = 0.0;
  for (size_t i = 0; i < x.size(); ++i) {
    double pred = result.slope * x[i] + result.intercept;
    ss_res += (y[i] - pred) * (y[i] - pred);
    ss_tot += (y[i] - my) * (y[i] - my);
  }
  result.r_squared = (ss_tot > 1e-10) ? 1.0 - ss_res / ss_tot : 0.0;
  result.std_error = std::sqrt(ss_res / static_cast<double>(x.size() - 2));

  return result;
}

std::vector<double> StatisticalAnalyzer::cross_correlation(const std::vector<double>& x,
                                                           const std::vector<double>& y,
                                                           int max_lag) const {
  std::vector<double> result;
  if (x.empty() || y.empty()) return result;

  double mx = mean(x);
  double my = mean(y);
  double sx = std_deviation(x);
  double sy = std_deviation(y);
  if (sx < 1e-10 || sy < 1e-10) return result;

  for (int lag = -max_lag; lag <= max_lag; ++lag) {
    double sum = 0.0;
    int count = 0;
    for (size_t i = 0; i < x.size(); ++i) {
      int j = static_cast<int>(i) + lag;
      if (j >= 0 && j < static_cast<int>(y.size())) {
        sum += (x[i] - mx) * (y[static_cast<size_t>(j)] - my);
        ++count;
      }
    }
    result.push_back(count > 0 ? sum / (static_cast<double>(count) * sx * sy) : 0.0);
  }

  return result;
}

std::vector<Anomaly> StatisticalAnalyzer::detect_anomalies(const std::vector<double>& data,
                                                           double sigma_threshold) const {
  std::vector<Anomaly> anomalies;
  if (data.size() < 3) return anomalies;

  double m = mean(data);
  double s = std_deviation(data);
  if (s < 1e-10) return anomalies;

  for (size_t i = 0; i < data.size(); ++i) {
    double score = std::abs(data[i] - m) / s;
    if (score > sigma_threshold) {
      anomalies.push_back({i, data[i], score, sigma_threshold});
    }
  }

  std::sort(anomalies.begin(), anomalies.end(),
            [](const Anomaly& a, const Anomaly& b) { return a.score > b.score; });

  return anomalies;
}

TrendResult StatisticalAnalyzer::detect_trend(const std::vector<double>& data) const {
  TrendResult result;
  if (data.size() < 2) return result;

  std::vector<double> x(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    x[i] = static_cast<double>(i);
  }

  auto reg = linear_regression(x, data);
  result.slope = reg.slope;
  result.direction = (reg.slope > 0.001) ? 1.0 : (reg.slope < -0.001) ? -1.0 : 0.0;
  result.strength = std::sqrt(reg.r_squared);

  return result;
}

std::vector<double> StatisticalAnalyzer::moving_average(const std::vector<double>& data,
                                                        size_t window) const {
  std::vector<double> result;
  if (data.size() < window || window == 0) return result;

  double sum = 0.0;
  for (size_t i = 0; i < window; ++i) {
    sum += data[i];
  }
  result.push_back(sum / static_cast<double>(window));

  for (size_t i = window; i < data.size(); ++i) {
    sum += data[i] - data[i - window];
    result.push_back(sum / static_cast<double>(window));
  }

  return result;
}

std::vector<double> StatisticalAnalyzer::detrend(const std::vector<double>& data) const {
  std::vector<double> result;
  if (data.size() < 2) return data;

  std::vector<double> x(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    x[i] = static_cast<double>(i);
  }

  auto reg = linear_regression(x, data);

  for (size_t i = 0; i < data.size(); ++i) {
    result.push_back(data[i] - (reg.slope * x[i] + reg.intercept));
  }

  return result;
}

}  // namespace SolarSystem::Analysis
