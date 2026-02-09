#pragma once

/**
 * @file advanced_analyzer.hpp
 * @brief Advanced analysis algorithms including clustering and optimization
 */

#include <functional>
#include <map>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>
#include <solar_analysis/statistical_analyzer.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Cluster result
 */
struct SOLAR_ANALYSIS_API Cluster {
  size_t id = 0;
  std::vector<size_t> member_indices;
  std::vector<double> centroid;
  double intra_cluster_distance = 0.0;
};

/**
 * @brief Clustering result
 */
struct SOLAR_ANALYSIS_API ClusteringResult {
  std::vector<Cluster> clusters;
  std::vector<size_t> assignments;  // cluster id for each point
  double silhouette_score = 0.0;
  size_t iterations = 0;
};

/**
 * @brief Optimization result
 */
struct SOLAR_ANALYSIS_API OptimizationResult {
  std::vector<double> optimal_params;
  double optimal_value = 0.0;
  size_t iterations = 0;
  bool converged = false;
  double convergence_error = 0.0;
};

/**
 * @brief Curve fit result
 */
struct SOLAR_ANALYSIS_API CurveFitResult {
  std::vector<double> coefficients;
  double r_squared = 0.0;
  double rmse = 0.0;
  std::string model_type;
};

/**
 * @brief Sensitivity analysis result
 */
struct SOLAR_ANALYSIS_API SensitivityResult {
  std::map<std::string, double> sensitivities;
  std::string most_sensitive_param;
  double max_sensitivity = 0.0;
};

/**
 * @brief Objective function type for optimization
 */
using ObjectiveFunction = std::function<double(const std::vector<double>&)>;

/**
 * @brief Advanced analyzer with ML and optimization capabilities
 */
class SOLAR_ANALYSIS_API AdvancedAnalyzer {
 public:
  AdvancedAnalyzer();
  ~AdvancedAnalyzer();

  // Clustering (K-means)
  [[nodiscard]] ClusteringResult kmeans_cluster(const std::vector<std::vector<double>>& data,
                                                size_t k, size_t max_iterations = 100) const;

  [[nodiscard]] size_t estimate_optimal_k(const std::vector<std::vector<double>>& data,
                                          size_t max_k = 10) const;

  // Classification / Anomaly categorization
  [[nodiscard]] std::vector<size_t> classify_anomalies(const std::vector<Anomaly>& anomalies,
                                                       size_t num_categories = 3) const;

  // Predictive modeling
  [[nodiscard]] std::vector<double> predict_trend(const std::vector<double>& data,
                                                  size_t future_points) const;

  [[nodiscard]] CurveFitResult fit_polynomial(const std::vector<double>& x,
                                              const std::vector<double>& y, int degree) const;

  [[nodiscard]] CurveFitResult fit_exponential(const std::vector<double>& x,
                                               const std::vector<double>& y) const;

  // Numerical optimization
  [[nodiscard]] OptimizationResult minimize(ObjectiveFunction func,
                                            const std::vector<double>& initial_guess,
                                            double tolerance = 1e-6,
                                            size_t max_iterations = 1000) const;

  [[nodiscard]] OptimizationResult gradient_descent(ObjectiveFunction func,
                                                    const std::vector<double>& initial_guess,
                                                    double learning_rate = 0.01,
                                                    size_t max_iterations = 1000) const;

  // Multi-objective optimization (returns Pareto front)
  [[nodiscard]] std::vector<std::vector<double>> pareto_optimize(
      const std::vector<ObjectiveFunction>& objectives,
      const std::vector<std::vector<double>>& candidates) const;

  // Parameter estimation
  [[nodiscard]] std::vector<double> least_squares_fit(
      const std::vector<double>& x, const std::vector<double>& y,
      std::function<double(double, const std::vector<double>&)> model, size_t num_params) const;

  // Sensitivity analysis
  [[nodiscard]] SensitivityResult analyze_sensitivity(ObjectiveFunction func,
                                                      const std::vector<double>& params,
                                                      const std::vector<std::string>& param_names,
                                                      double delta = 0.01) const;

 private:
  StatisticalAnalyzer stats_;

  [[nodiscard]] double euclidean_distance(const std::vector<double>& a,
                                          const std::vector<double>& b) const;
  [[nodiscard]] std::vector<double> compute_centroid(
      const std::vector<std::vector<double>>& points) const;
};

}  // namespace SolarSystem::Analysis
