/**
 * @file advanced_analyzer.cpp
 * @brief Implementation of advanced analysis algorithms
 */

#include "solar_analysis/advanced_analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace SolarSystem::Analysis {

AdvancedAnalyzer::AdvancedAnalyzer() = default;
AdvancedAnalyzer::~AdvancedAnalyzer() = default;

double AdvancedAnalyzer::euclidean_distance(const std::vector<double>& a,
                                            const std::vector<double>& b) const {
  double sum = 0.0;
  size_t n = std::min(a.size(), b.size());
  for (size_t i = 0; i < n; ++i) {
    double diff = a[i] - b[i];
    sum += diff * diff;
  }
  return std::sqrt(sum);
}

std::vector<double> AdvancedAnalyzer::compute_centroid(
    const std::vector<std::vector<double>>& points) const {
  if (points.empty()) return {};

  size_t dims = points[0].size();
  std::vector<double> centroid(dims, 0.0);

  for (const auto& p : points) {
    for (size_t i = 0; i < dims && i < p.size(); ++i) {
      centroid[i] += p[i];
    }
  }

  double n = static_cast<double>(points.size());
  for (auto& c : centroid) {
    c /= n;
  }

  return centroid;
}

ClusteringResult AdvancedAnalyzer::kmeans_cluster(const std::vector<std::vector<double>>& data,
                                                  size_t k, size_t max_iterations) const {
  ClusteringResult result;
  if (data.empty() || k == 0) return result;

  size_t n = data.size();
  k = std::min(k, n);

  // Initialize centroids randomly
  std::vector<std::vector<double>> centroids;
  std::mt19937 rng(42);
  std::vector<size_t> indices(n);
  std::iota(indices.begin(), indices.end(), 0);
  std::shuffle(indices.begin(), indices.end(), rng);
  for (size_t i = 0; i < k; ++i) {
    centroids.push_back(data[indices[i]]);
  }

  result.assignments.resize(n);

  for (size_t iter = 0; iter < max_iterations; ++iter) {
    // Assign points to nearest centroid
    bool changed = false;
    for (size_t i = 0; i < n; ++i) {
      double min_dist = std::numeric_limits<double>::max();
      size_t best_cluster = 0;
      for (size_t j = 0; j < k; ++j) {
        double dist = euclidean_distance(data[i], centroids[j]);
        if (dist < min_dist) {
          min_dist = dist;
          best_cluster = j;
        }
      }
      if (result.assignments[i] != best_cluster) {
        result.assignments[i] = best_cluster;
        changed = true;
      }
    }

    if (!changed) {
      result.iterations = iter + 1;
      break;
    }

    // Update centroids
    for (size_t j = 0; j < k; ++j) {
      std::vector<std::vector<double>> cluster_points;
      for (size_t i = 0; i < n; ++i) {
        if (result.assignments[i] == j) {
          cluster_points.push_back(data[i]);
        }
      }
      if (!cluster_points.empty()) {
        centroids[j] = compute_centroid(cluster_points);
      }
    }

    result.iterations = iter + 1;
  }

  // Build cluster objects
  for (size_t j = 0; j < k; ++j) {
    Cluster cluster;
    cluster.id = j;
    cluster.centroid = centroids[j];
    for (size_t i = 0; i < n; ++i) {
      if (result.assignments[i] == j) {
        cluster.member_indices.push_back(i);
        cluster.intra_cluster_distance += euclidean_distance(data[i], centroids[j]);
      }
    }
    if (!cluster.member_indices.empty()) {
      cluster.intra_cluster_distance /= static_cast<double>(cluster.member_indices.size());
    }
    result.clusters.push_back(cluster);
  }

  return result;
}

size_t AdvancedAnalyzer::estimate_optimal_k(const std::vector<std::vector<double>>& data,
                                            size_t max_k) const {
  if (data.size() < 2) return 1;

  // Elbow method: find k where inertia decrease slows
  std::vector<double> inertias;
  for (size_t k = 1; k <= std::min(max_k, data.size()); ++k) {
    auto result = kmeans_cluster(data, k, 50);
    double inertia = 0.0;
    for (const auto& cluster : result.clusters) {
      inertia +=
          cluster.intra_cluster_distance * static_cast<double>(cluster.member_indices.size());
    }
    inertias.push_back(inertia);
  }

  // Find elbow (max second derivative)
  size_t best_k = 1;
  double max_diff = 0.0;
  for (size_t i = 1; i < inertias.size() - 1; ++i) {
    double diff = (inertias[i - 1] - inertias[i]) - (inertias[i] - inertias[i + 1]);
    if (diff > max_diff) {
      max_diff = diff;
      best_k = i + 1;
    }
  }

  return best_k;
}

std::vector<size_t> AdvancedAnalyzer::classify_anomalies(const std::vector<Anomaly>& anomalies,
                                                         size_t num_categories) const {
  std::vector<size_t> categories(anomalies.size());
  if (anomalies.empty()) return categories;

  // Classify by score magnitude
  std::vector<double> scores;
  for (const auto& a : anomalies) {
    scores.push_back(a.score);
  }

  double min_score = *std::min_element(scores.begin(), scores.end());
  double max_score = *std::max_element(scores.begin(), scores.end());
  double range = max_score - min_score;

  if (range < 1e-10) {
    std::fill(categories.begin(), categories.end(), 0);
    return categories;
  }

  for (size_t i = 0; i < anomalies.size(); ++i) {
    double normalized = (scores[i] - min_score) / range;
    categories[i] = static_cast<size_t>(normalized * (num_categories - 1));
  }

  return categories;
}

std::vector<double> AdvancedAnalyzer::predict_trend(const std::vector<double>& data,
                                                    size_t future_points) const {
  if (data.size() < 2) return {};

  // Linear extrapolation
  std::vector<double> x(data.size());
  std::iota(x.begin(), x.end(), 0.0);

  auto reg = stats_.linear_regression(x, data);

  std::vector<double> predictions;
  for (size_t i = 0; i < future_points; ++i) {
    double xi = static_cast<double>(data.size() + i);
    predictions.push_back(reg.slope * xi + reg.intercept);
  }

  return predictions;
}

CurveFitResult AdvancedAnalyzer::fit_polynomial(const std::vector<double>& x,
                                                const std::vector<double>& y, int degree) const {
  CurveFitResult result;
  result.model_type = "polynomial_" + std::to_string(degree);

  if (x.size() != y.size() || x.size() < static_cast<size_t>(degree + 1)) {
    return result;
  }

  // For degree 1, use existing linear regression
  if (degree == 1) {
    auto reg = stats_.linear_regression(x, y);
    result.coefficients = {reg.intercept, reg.slope};
    result.r_squared = reg.r_squared;
    result.rmse = reg.std_error;
    return result;
  }

  // Simple polynomial fit using normal equations (degree 2)
  if (degree == 2) {
    size_t n = x.size();
    double sx = 0, sx2 = 0, sx3 = 0, sx4 = 0;
    double sy = 0, sxy = 0, sx2y = 0;

    for (size_t i = 0; i < n; ++i) {
      double xi = x[i], yi = y[i];
      sx += xi;
      sx2 += xi * xi;
      sx3 += xi * xi * xi;
      sx4 += xi * xi * xi * xi;
      sy += yi;
      sxy += xi * yi;
      sx2y += xi * xi * yi;
    }

    double dn = static_cast<double>(n);
    // Solve 3x3 system (simplified)
    double det =
        dn * (sx2 * sx4 - sx3 * sx3) - sx * (sx * sx4 - sx2 * sx3) + sx2 * (sx * sx3 - sx2 * sx2);

    if (std::abs(det) > 1e-10) {
      double a0 = (sy * (sx2 * sx4 - sx3 * sx3) - sx * (sxy * sx4 - sx2y * sx3) +
                   sx2 * (sxy * sx3 - sx2y * sx2)) /
                  det;
      double a1 = (dn * (sxy * sx4 - sx2y * sx3) - sy * (sx * sx4 - sx2 * sx3) +
                   sx2 * (sx * sx2y - sx2 * sxy)) /
                  det;
      double a2 = (dn * (sx2 * sx2y - sx3 * sxy) - sx * (sx * sx2y - sx2 * sxy) +
                   sy * (sx * sx3 - sx2 * sx2)) /
                  det;

      result.coefficients = {a0, a1, a2};

      // Calculate R-squared
      double mean_y = sy / dn;
      double ss_tot = 0, ss_res = 0;
      for (size_t i = 0; i < n; ++i) {
        double pred = a0 + a1 * x[i] + a2 * x[i] * x[i];
        ss_res += (y[i] - pred) * (y[i] - pred);
        ss_tot += (y[i] - mean_y) * (y[i] - mean_y);
      }
      result.r_squared = (ss_tot > 1e-10) ? 1.0 - ss_res / ss_tot : 0.0;
      result.rmse = std::sqrt(ss_res / dn);
    }
  }

  return result;
}

CurveFitResult AdvancedAnalyzer::fit_exponential(const std::vector<double>& x,
                                                 const std::vector<double>& y) const {
  CurveFitResult result;
  result.model_type = "exponential";

  if (x.size() != y.size() || x.empty()) return result;

  // Linearize: ln(y) = ln(a) + b*x
  std::vector<double> log_y;
  for (double yi : y) {
    if (yi > 0) {
      log_y.push_back(std::log(yi));
    } else {
      return result;  // Can't fit exponential to non-positive data
    }
  }

  auto reg = stats_.linear_regression(x, log_y);
  result.coefficients = {std::exp(reg.intercept), reg.slope};  // a, b in y = a * exp(b*x)
  result.r_squared = reg.r_squared;

  // Calculate RMSE in original space
  double ss_res = 0;
  for (size_t i = 0; i < x.size(); ++i) {
    double pred = result.coefficients[0] * std::exp(result.coefficients[1] * x[i]);
    ss_res += (y[i] - pred) * (y[i] - pred);
  }
  result.rmse = std::sqrt(ss_res / static_cast<double>(x.size()));

  return result;
}

OptimizationResult AdvancedAnalyzer::minimize(ObjectiveFunction func,
                                              const std::vector<double>& initial_guess,
                                              double /*tolerance*/, size_t max_iterations) const {
  return gradient_descent(func, initial_guess, 0.01, max_iterations);
}

OptimizationResult AdvancedAnalyzer::gradient_descent(ObjectiveFunction func,
                                                      const std::vector<double>& initial_guess,
                                                      double learning_rate,
                                                      size_t max_iterations) const {
  OptimizationResult result;
  result.optimal_params = initial_guess;

  if (initial_guess.empty()) return result;

  std::vector<double> params = initial_guess;
  double prev_value = func(params);

  for (size_t iter = 0; iter < max_iterations; ++iter) {
    // Compute numerical gradient
    std::vector<double> gradient(params.size());
    double h = 1e-8;

    for (size_t i = 0; i < params.size(); ++i) {
      std::vector<double> params_plus = params;
      std::vector<double> params_minus = params;
      params_plus[i] += h;
      params_minus[i] -= h;
      gradient[i] = (func(params_plus) - func(params_minus)) / (2.0 * h);
    }

    // Update parameters
    for (size_t i = 0; i < params.size(); ++i) {
      params[i] -= learning_rate * gradient[i];
    }

    double current_value = func(params);
    result.convergence_error = std::abs(current_value - prev_value);

    if (result.convergence_error < 1e-10) {
      result.converged = true;
      result.iterations = iter + 1;
      break;
    }

    prev_value = current_value;
    result.iterations = iter + 1;
  }

  result.optimal_params = params;
  result.optimal_value = func(params);

  return result;
}

std::vector<std::vector<double>> AdvancedAnalyzer::pareto_optimize(
    const std::vector<ObjectiveFunction>& objectives,
    const std::vector<std::vector<double>>& candidates) const {
  std::vector<std::vector<double>> pareto_front;

  if (objectives.empty() || candidates.empty()) return pareto_front;

  // Evaluate all candidates
  std::vector<std::vector<double>> scores(candidates.size());
  for (size_t i = 0; i < candidates.size(); ++i) {
    for (const auto& obj : objectives) {
      scores[i].push_back(obj(candidates[i]));
    }
  }

  // Find non-dominated solutions
  for (size_t i = 0; i < candidates.size(); ++i) {
    bool dominated = false;
    for (size_t j = 0; j < candidates.size(); ++j) {
      if (i == j) continue;

      // Check if j dominates i
      bool j_better_in_all = true;
      bool j_strictly_better_in_one = false;
      for (size_t k = 0; k < objectives.size(); ++k) {
        if (scores[j][k] > scores[i][k]) {
          j_better_in_all = false;
        }
        if (scores[j][k] < scores[i][k]) {
          j_strictly_better_in_one = true;
        }
      }
      if (j_better_in_all && j_strictly_better_in_one) {
        dominated = true;
        break;
      }
    }
    if (!dominated) {
      pareto_front.push_back(candidates[i]);
    }
  }

  return pareto_front;
}

std::vector<double> AdvancedAnalyzer::least_squares_fit(
    const std::vector<double>& x, const std::vector<double>& y,
    std::function<double(double, const std::vector<double>&)> model, size_t num_params) const {
  // Use gradient descent to minimize sum of squared residuals
  auto objective = [&](const std::vector<double>& params) {
    double sum = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
      double residual = y[i] - model(x[i], params);
      sum += residual * residual;
    }
    return sum;
  };

  std::vector<double> initial(num_params, 1.0);
  auto result = minimize(objective, initial);
  return result.optimal_params;
}

SensitivityResult AdvancedAnalyzer::analyze_sensitivity(ObjectiveFunction func,
                                                        const std::vector<double>& params,
                                                        const std::vector<std::string>& param_names,
                                                        double delta) const {
  SensitivityResult result;

  double base_value = func(params);

  for (size_t i = 0; i < params.size() && i < param_names.size(); ++i) {
    std::vector<double> perturbed = params;
    perturbed[i] *= (1.0 + delta);

    double perturbed_value = func(perturbed);
    double sensitivity = std::abs(perturbed_value - base_value) / (std::abs(base_value) + 1e-10);

    result.sensitivities[param_names[i]] = sensitivity;

    if (sensitivity > result.max_sensitivity) {
      result.max_sensitivity = sensitivity;
      result.most_sensitive_param = param_names[i];
    }
  }

  return result;
}

}  // namespace SolarSystem::Analysis
