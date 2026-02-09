/**
 * @file test_solar_analysis.cpp
 * @brief Unit tests for solar_analysis library
 */

#include <gtest/gtest.h>

#include <cmath>
#include <solar_analysis/advanced_analyzer.hpp>
#include <solar_analysis/data_exporter.hpp>
#include <solar_analysis/data_models.hpp>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/historical_analyzer.hpp>
#include <solar_analysis/orbital_calculator.hpp>
#include <solar_analysis/statistical_analyzer.hpp>

using namespace SolarSystem::Analysis;

// ============================================================================
// Orbital Calculator Tests
// ============================================================================

class OrbitalCalculatorTest : public ::testing::Test {
 protected:
  OrbitalCalculator calc;
};

TEST_F(OrbitalCalculatorTest, KeplerEquationSolving) {
  // Test Kepler equation: M = E - e*sin(E)
  double e = 0.5;
  double M = 1.0;
  double E = calc.solve_kepler(M, e);

  // Verify: M should equal E - e*sin(E)
  double computed_M = E - e * std::sin(E);
  EXPECT_NEAR(computed_M, M, 1e-9);
}

TEST_F(OrbitalCalculatorTest, CircularOrbitPeriod) {
  // Earth's semi-major axis ~1 AU = 1.496e8 km
  double a = 1.496e8;
  double period = calc.calculate_orbital_period(a);

  // Should be approximately 1 year in seconds (~3.15e7 s)
  EXPECT_NEAR(period, 3.156e7, 1e6);
}

TEST_F(OrbitalCalculatorTest, PeriapsisApoapsis) {
  double a = 1.0e8;
  double e = 0.2;

  double periapsis = calc.calculate_periapsis(a, e);
  double apoapsis = calc.calculate_apoapsis(a, e);

  EXPECT_DOUBLE_EQ(periapsis, a * (1 - e));
  EXPECT_DOUBLE_EQ(apoapsis, a * (1 + e));
  EXPECT_LT(periapsis, apoapsis);
}

TEST_F(OrbitalCalculatorTest, OrbitalElementsFromStateVector) {
  // Create a simple circular orbit state
  StateVector state;
  state.position = SolarSystem::Math::Vector3d(1.496e8, 0, 0);  // 1 AU on x-axis
  state.velocity = SolarSystem::Math::Vector3d(0, 29.78, 0);    // ~30 km/s tangential

  auto elements = calc.calculate_elements(state);

  EXPECT_GT(elements.semi_major_axis, 0);
  EXPECT_GE(elements.eccentricity, 0);
  EXPECT_LT(elements.eccentricity, 1);
}

// ============================================================================
// Statistical Analyzer Tests
// ============================================================================

class StatisticalAnalyzerTest : public ::testing::Test {
 protected:
  StatisticalAnalyzer stats;
};

TEST_F(StatisticalAnalyzerTest, MeanCalculation) {
  std::vector<double> data = {1, 2, 3, 4, 5};
  EXPECT_DOUBLE_EQ(stats.mean(data), 3.0);
}

TEST_F(StatisticalAnalyzerTest, MedianOddCount) {
  std::vector<double> data = {1, 3, 5, 7, 9};
  EXPECT_DOUBLE_EQ(stats.median(data), 5.0);
}

TEST_F(StatisticalAnalyzerTest, MedianEvenCount) {
  std::vector<double> data = {1, 2, 3, 4};
  EXPECT_DOUBLE_EQ(stats.median(data), 2.5);
}

TEST_F(StatisticalAnalyzerTest, VarianceCalculation) {
  std::vector<double> data = {2, 4, 4, 4, 5, 5, 7, 9};
  double var = stats.variance(data);
  EXPECT_NEAR(var, 4.571, 0.01);
}

TEST_F(StatisticalAnalyzerTest, LinearRegression) {
  std::vector<double> x = {1, 2, 3, 4, 5};
  std::vector<double> y = {2, 4, 6, 8, 10};  // y = 2x

  auto result = stats.linear_regression(x, y);

  EXPECT_NEAR(result.slope, 2.0, 1e-10);
  EXPECT_NEAR(result.intercept, 0.0, 1e-10);
  EXPECT_NEAR(result.r_squared, 1.0, 1e-10);
}

TEST_F(StatisticalAnalyzerTest, AnomalyDetection) {
  std::vector<double> data = {1, 2, 2, 2, 2, 2, 2, 100};  // 100 is anomaly

  auto anomalies = stats.detect_anomalies(data, 2.0);

  EXPECT_FALSE(anomalies.empty());
  EXPECT_EQ(anomalies[0].index, 7);
}

// ============================================================================
// Data Processor Tests
// ============================================================================

TEST(DataProcessorTest, LoadAndRetrieveData) {
  DataProcessor processor;

  EXPECT_TRUE(processor.load_body_data("Earth"));
  EXPECT_TRUE(processor.has_data("Earth"));
  EXPECT_GT(processor.data_point_count("Earth"), 0);
}

TEST(DataProcessorTest, DataValidation) {
  DataProcessor processor;
  ASSERT_TRUE(processor.load_body_data("Mars"));

  auto quality = processor.validate_data("Mars");

  EXPECT_GT(quality.total_points, 0);
  EXPECT_EQ(quality.valid_points, quality.total_points);
  EXPECT_DOUBLE_EQ(quality.completeness, 1.0);
}

TEST(DataProcessorTest, CacheManagement) {
  DataProcessor processor;
  ASSERT_TRUE(processor.load_body_data("Venus"));

  EXPECT_EQ(processor.cache_size(), 1);

  processor.clear_cache();
  EXPECT_EQ(processor.cache_size(), 0);
}

// ============================================================================
// Coordinate Conversion Tests
// ============================================================================

TEST(CoordinateConverterTest, CartesianToSphericalRoundTrip) {
  SolarSystem::Math::Vector3d cart(1.0, 2.0, 3.0);

  auto sph = CoordinateConverter::cartesian_to_spherical(cart);
  auto back = CoordinateConverter::spherical_to_cartesian(sph);

  EXPECT_NEAR(back.x(), cart.x(), 1e-10);
  EXPECT_NEAR(back.y(), cart.y(), 1e-10);
  EXPECT_NEAR(back.z(), cart.z(), 1e-10);
}

TEST(CoordinateConverterTest, EclipticEquatorialRoundTrip) {
  SolarSystem::Math::Vector3d ecl(1.0, 0.5, 0.2);

  auto equ = CoordinateConverter::ecliptic_to_equatorial(ecl);
  auto back = CoordinateConverter::equatorial_to_ecliptic(equ);

  EXPECT_NEAR(back.x(), ecl.x(), 1e-10);
  EXPECT_NEAR(back.y(), ecl.y(), 1e-10);
  EXPECT_NEAR(back.z(), ecl.z(), 1e-10);
}

// ============================================================================
// Data Export Tests
// ============================================================================

TEST(DataExporterTest, ExportToString) {
  DataExporter exporter;
  std::vector<StateVector> data(3);

  auto now = std::chrono::system_clock::now();
  for (size_t i = 0; i < 3; ++i) {
    data[i].timestamp = now + std::chrono::hours(static_cast<long>(i));
    data[i].position = SolarSystem::Math::Vector3d(static_cast<double>(i), 0, 0);
    data[i].velocity = SolarSystem::Math::Vector3d(0, static_cast<double>(i), 0);
  }

  std::string csv = exporter.export_to_string(data);

  EXPECT_FALSE(csv.empty());
  EXPECT_NE(csv.find("pos_x"), std::string::npos);
}

// ============================================================================
// Advanced Analyzer Tests
// ============================================================================

TEST(AdvancedAnalyzerTest, KMeansClustering) {
  AdvancedAnalyzer analyzer;

  std::vector<std::vector<double>> data = {{0, 0}, {1, 0}, {0, 1}, {10, 10}, {11, 10}, {10, 11}};

  auto result = analyzer.kmeans_cluster(data, 2);

  EXPECT_EQ(result.clusters.size(), 2);
  EXPECT_EQ(result.assignments.size(), 6);
}

TEST(AdvancedAnalyzerTest, PolynomialFit) {
  AdvancedAnalyzer analyzer;

  std::vector<double> x = {0, 1, 2, 3, 4};
  std::vector<double> y = {0, 1, 4, 9, 16};  // y = x^2

  auto result = analyzer.fit_polynomial(x, y, 2);

  EXPECT_EQ(result.coefficients.size(), 3);
  EXPECT_GT(result.r_squared, 0.99);
}

TEST(AdvancedAnalyzerTest, GradientDescent) {
  AdvancedAnalyzer analyzer;

  // Minimize f(x) = (x-3)^2
  auto func = [](const std::vector<double>& params) {
    double x = params[0];
    return (x - 3) * (x - 3);
  };

  auto result = analyzer.gradient_descent(func, {0.0}, 0.1, 1000);

  EXPECT_NEAR(result.optimal_params[0], 3.0, 0.1);
  EXPECT_NEAR(result.optimal_value, 0.0, 0.1);
}

// ============================================================================
// Historical Analyzer Tests
// ============================================================================

TEST(HistoricalAnalyzerTest, PredictionValidation) {
  HistoricalAnalyzer analyzer;

  std::vector<double> predicted = {1.0, 2.0, 3.0, 4.0, 5.0};
  std::vector<double> actual = {1.1, 2.1, 2.9, 4.0, 5.1};

  auto result = analyzer.validate_predictions(predicted, actual);

  EXPECT_LT(result.rmse, 0.2);
  EXPECT_GT(result.r_squared, 0.99);
}

TEST(HistoricalAnalyzerTest, ConfidenceIntervals) {
  HistoricalAnalyzer analyzer;

  std::vector<double> data = {10, 11, 9, 10, 11, 10, 9, 10, 11, 10};

  auto intervals = analyzer.calculate_confidence_intervals(data, 0.95);

  EXPECT_EQ(intervals.size(), 2);
  EXPECT_LT(intervals[0], intervals[1]);
}
