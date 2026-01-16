#include "solar_test/benchmarks/regression_detector.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <numeric>
#include <sstream>

namespace SolarSystem::Testing {

// PerformanceBaseline implementation
std::string PerformanceBaseline::to_json() const {
  nlohmann::json j;
  j["benchmark_name"] = benchmark_name;
  j["mean_time_ns"] = mean_time.count();
  j["std_dev_ns"] = std_dev.count();
  j["memory_usage_bytes"] = memory_usage_bytes;
  j["operations_per_second"] = operations_per_second;

  // Convert timestamp to seconds since epoch
  auto time_t = std::chrono::system_clock::to_time_t(timestamp);
  j["timestamp"] = time_t;
  j["version"] = version;
  j["metadata"] = metadata;

  return j.dump(2);
}

PerformanceBaseline PerformanceBaseline::from_json(const std::string& json_str) {
  PerformanceBaseline baseline;

  try {
    auto j = nlohmann::json::parse(json_str);

    if (j.contains("benchmark_name")) {
      baseline.benchmark_name = j["benchmark_name"].get<std::string>();
    }
    if (j.contains("mean_time_ns")) {
      baseline.mean_time = std::chrono::nanoseconds(j["mean_time_ns"].get<int64_t>());
    }
    if (j.contains("std_dev_ns")) {
      baseline.std_dev = std::chrono::nanoseconds(j["std_dev_ns"].get<int64_t>());
    }
    if (j.contains("memory_usage_bytes")) {
      baseline.memory_usage_bytes = j["memory_usage_bytes"].get<size_t>();
    }
    if (j.contains("operations_per_second")) {
      baseline.operations_per_second = j["operations_per_second"].get<double>();
    }
    if (j.contains("version")) {
      baseline.version = j["version"].get<std::string>();
    }
    if (j.contains("metadata")) {
      baseline.metadata = j["metadata"].get<std::map<std::string, std::string>>();
    }

    // Set timestamp to current time (simplified)
    baseline.timestamp = std::chrono::system_clock::now();
  } catch (const nlohmann::json::exception&) {
    // Return default baseline on parse error
  }

  return baseline;
}

std::string PerformanceBaseline::to_csv_row() const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(6);

  double mean_ms = static_cast<double>(mean_time.count()) / 1e6;
  double std_dev_ms = static_cast<double>(std_dev.count()) / 1e6;

  oss << benchmark_name << "," << mean_ms << "," << std_dev_ms << "," << memory_usage_bytes << ","
      << operations_per_second << "," << version;

  return oss.str();
}

std::string PerformanceBaseline::csv_header() {
  return "BenchmarkName,MeanTime(ms),StdDev(ms),MemoryUsage(bytes),OpsPerSec,Version";
}

// RegressionAnalysis implementation
bool RegressionAnalysis::exceeds_threshold(double threshold_percentage) const {
  return std::abs(time_regression_percentage) > threshold_percentage ||
         std::abs(memory_regression_percentage) > threshold_percentage ||
         std::abs(ops_regression_percentage) > threshold_percentage;
}

std::string RegressionAnalysis::generate_report() const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2);

  oss << "Performance Regression Analysis: " << benchmark_name << "\n";
  oss << "===============================================\n";
  oss << "Regression Detected: " << (has_regression ? "YES" : "NO") << "\n";
  oss << "Severity: " << severity << "\n\n";

  oss << "Performance Changes:\n";
  oss << "  Execution Time: " << time_regression_percentage << "%\n";
  oss << "  Memory Usage: " << memory_regression_percentage << "%\n";
  oss << "  Operations/Sec: " << ops_regression_percentage << "%\n\n";

  oss << "Baseline vs Current:\n";
  oss << "  Time: " << (static_cast<double>(baseline.mean_time.count()) / 1e6) << "ms -> "
      << (static_cast<double>(current_result.mean_time.count()) / 1e6) << "ms\n";
  oss << "  Memory: " << baseline.memory_usage_bytes << " bytes -> "
      << current_result.memory_usage_bytes << " bytes\n";
  oss << "  Ops/Sec: " << baseline.operations_per_second << " -> "
      << current_result.operations_per_second << "\n\n";

  if (!alerts.empty()) {
    oss << "Alerts:\n";
    for (const auto& alert : alerts) {
      oss << "  - " << alert << "\n";
    }
  }

  return oss.str();
}

std::string RegressionAnalysis::to_json() const {
  nlohmann::json j;
  j["benchmark_name"] = benchmark_name;
  j["has_regression"] = has_regression;
  j["time_regression_percentage"] = time_regression_percentage;
  j["memory_regression_percentage"] = memory_regression_percentage;
  j["ops_regression_percentage"] = ops_regression_percentage;
  j["severity"] = severity;
  j["alerts"] = alerts;

  return j.dump(2);
}

// TrendAnalysis implementation
std::string TrendAnalysis::generate_report() const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(4);

  oss << "Performance Trend Analysis: " << benchmark_name << "\n";
  oss << "==========================================\n";
  oss << "Data Points: " << data_points.size() << "\n";
  oss << "Trend Direction: " << trend_direction << "\n";
  oss << "Confidence Level: " << confidence_level << "%\n\n";

  oss << "Trend Slopes:\n";
  oss << "  Execution Time: " << time_trend_slope << " ns/sample\n";
  oss << "  Memory Usage: " << memory_trend_slope << " bytes/sample\n";
  oss << "  Operations/Sec: " << ops_trend_slope << " ops/sample\n\n";

  if (data_points.size() >= 2) {
    const auto& first = data_points.front();
    const auto& last = data_points.back();

    oss << "Overall Change (First -> Last):\n";
    oss << "  Time: " << (static_cast<double>(first.execution_time.count()) / 1e6) << "ms -> "
        << (static_cast<double>(last.execution_time.count()) / 1e6) << "ms\n";
    oss << "  Memory: " << first.memory_usage_bytes << " bytes -> " << last.memory_usage_bytes
        << " bytes\n";
    oss << "  Ops/Sec: " << first.operations_per_second << " -> " << last.operations_per_second
        << "\n";
  }

  return oss.str();
}

// RegressionDetector implementation
RegressionDetector::RegressionDetector(RegressionDetectorConfig config)
    : config_(std::move(config)) {
  load_baselines();
  load_trend_data();
}

void RegressionDetector::load_baselines() {
  std::ifstream file(config_.baseline_storage_path);
  if (!file.is_open()) {
    return;  // No baseline file exists yet
  }

  std::string line;
  std::ostringstream buffer;
  while (std::getline(file, line)) {
    buffer << line << "\n";
  }

  std::string json_content = buffer.str();
  if (json_content.empty()) {
    return;
  }

  // Simplified JSON parsing - extract each baseline
  // In production, use a proper JSON library
  size_t pos = 0;
  while ((pos = json_content.find("\"benchmark_name\":", pos)) != std::string::npos) {
    size_t start = json_content.rfind("{", pos);
    size_t end = json_content.find("}", pos) + 1;

    if (start != std::string::npos && end != std::string::npos) {
      std::string baseline_json = json_content.substr(start, end - start);
      auto baseline = PerformanceBaseline::from_json(baseline_json);
      baselines_[baseline.benchmark_name] = baseline;
    }

    pos = end;
  }
}

void RegressionDetector::save_baselines() {
  std::ofstream file(config_.baseline_storage_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open baseline storage file for writing");
  }

  file << "{\n  \"baselines\": [\n";

  bool first = true;
  for (const auto& [name, baseline] : baselines_) {
    if (!first) file << ",\n";
    file << "    " << baseline.to_json();
    first = false;
  }

  file << "\n  ]\n}\n";
}

void RegressionDetector::set_baseline(const std::string& benchmark_name,
                                      const BenchmarkResult& result, const std::string& version) {
  PerformanceBaseline baseline;
  baseline.benchmark_name = benchmark_name;
  baseline.mean_time = result.mean_time;
  baseline.std_dev = result.std_dev;
  baseline.memory_usage_bytes = result.memory_usage_bytes;
  baseline.operations_per_second = result.operations_per_second;
  baseline.timestamp = std::chrono::system_clock::now();
  baseline.version = version;
  baseline.metadata = result.metadata;

  baselines_[benchmark_name] = baseline;
  save_baselines();
}

std::optional<PerformanceBaseline> RegressionDetector::get_baseline(
    const std::string& benchmark_name) const {
  auto it = baselines_.find(benchmark_name);
  if (it != baselines_.end()) {
    return it->second;
  }
  return std::nullopt;
}

RegressionAnalysis RegressionDetector::analyze_regression(const BenchmarkResult& result) {
  RegressionAnalysis analysis;
  analysis.benchmark_name = result.name;
  analysis.current_result = result;
  analysis.analysis_time = std::chrono::system_clock::now();

  auto baseline_opt = get_baseline(result.name);
  if (!baseline_opt) {
    analysis.alerts.push_back("No baseline found for benchmark: " + result.name);
    return analysis;
  }

  analysis.baseline = *baseline_opt;

  // Calculate regression percentages
  analysis.time_regression_percentage =
      calculate_regression_percentage(static_cast<double>(baseline_opt->mean_time.count()),
                                      static_cast<double>(result.mean_time.count()));

  analysis.memory_regression_percentage =
      calculate_regression_percentage(static_cast<double>(baseline_opt->memory_usage_bytes),
                                      static_cast<double>(result.memory_usage_bytes));

  analysis.ops_regression_percentage = calculate_regression_percentage(
      baseline_opt->operations_per_second, result.operations_per_second);

  // Check for regressions
  bool time_regression =
      std::abs(analysis.time_regression_percentage) > config_.time_regression_threshold;
  bool memory_regression =
      std::abs(analysis.memory_regression_percentage) > config_.memory_regression_threshold;
  bool ops_regression =
      std::abs(analysis.ops_regression_percentage) > config_.ops_regression_threshold;

  analysis.has_regression = time_regression || memory_regression || ops_regression;

  // Generate alerts
  if (time_regression) {
    std::ostringstream oss;
    oss << "Execution time regression: " << analysis.time_regression_percentage << "%";
    analysis.alerts.push_back(oss.str());
  }

  if (memory_regression) {
    std::ostringstream oss;
    oss << "Memory usage regression: " << analysis.memory_regression_percentage << "%";
    analysis.alerts.push_back(oss.str());
  }

  if (ops_regression) {
    std::ostringstream oss;
    oss << "Operations/sec regression: " << analysis.ops_regression_percentage << "%";
    analysis.alerts.push_back(oss.str());
  }

  // Determine severity
  analysis.severity = determine_severity(std::abs(analysis.time_regression_percentage),
                                         std::abs(analysis.memory_regression_percentage),
                                         std::abs(analysis.ops_regression_percentage));

  // Auto-update baseline if improvement and enabled
  if (config_.auto_update_baseline && !analysis.has_regression) {
    bool improved = analysis.time_regression_percentage < -5.0 ||  // 5% improvement
                    analysis.ops_regression_percentage > 5.0;      // 5% improvement

    if (improved) {
      set_baseline(result.name, result, analysis.baseline.version);
      analysis.alerts.push_back("Baseline updated due to performance improvement");
    }
  }

  return analysis;
}

void RegressionDetector::add_trend_data_point(const std::string& benchmark_name,
                                              const BenchmarkResult& result,
                                              const std::string& version) {
  TrendDataPoint point;
  point.timestamp = std::chrono::system_clock::now();
  point.execution_time = result.mean_time;
  point.memory_usage_bytes = result.memory_usage_bytes;
  point.operations_per_second = result.operations_per_second;
  point.version = version;
  point.metadata = result.metadata;

  trend_data_[benchmark_name].push_back(point);

  // Keep only recent data points (e.g., last 100)
  auto& data = trend_data_[benchmark_name];
  if (data.size() > 100) {
    data.erase(data.begin(), data.begin() + static_cast<std::ptrdiff_t>(data.size() - 100));
  }

  save_trend_data();
}

TrendAnalysis RegressionDetector::analyze_trends(const std::string& benchmark_name) {
  TrendAnalysis analysis;
  analysis.benchmark_name = benchmark_name;

  auto it = trend_data_.find(benchmark_name);
  if (it == trend_data_.end() || it->second.size() < config_.min_samples_for_trend) {
    analysis.trend_direction = "insufficient_data";
    return analysis;
  }

  analysis.data_points = it->second;

  // Extract time series data
  std::vector<double> times, memory_values, ops_values;
  for (const auto& point : analysis.data_points) {
    times.push_back(static_cast<double>(point.execution_time.count()));
    memory_values.push_back(static_cast<double>(point.memory_usage_bytes));
    ops_values.push_back(point.operations_per_second);
  }

  // Calculate trend slopes
  analysis.time_trend_slope = calculate_trend_slope(times);
  analysis.memory_trend_slope = calculate_trend_slope(memory_values);
  analysis.ops_trend_slope = calculate_trend_slope(ops_values);

  // Determine trend direction
  double time_threshold = 0.01;  // 1% change threshold
  if (std::abs(analysis.time_trend_slope) < time_threshold) {
    analysis.trend_direction = "stable";
  } else if (analysis.time_trend_slope > 0) {
    analysis.trend_direction = "degrading";
  } else {
    analysis.trend_direction = "improving";
  }

  // Calculate confidence (simplified)
  analysis.confidence_level =
      std::min(95.0, static_cast<double>(analysis.data_points.size()) * 10.0);

  return analysis;
}

std::string RegressionDetector::generate_regression_report(
    const std::vector<RegressionAnalysis>& analyses) {
  std::ostringstream oss;
  oss << "Performance Regression Report\n";
  oss << "============================\n";
  oss << "Generated: "
      << std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count()
      << "\n\n";

  size_t total_benchmarks = analyses.size();
  size_t regressions = static_cast<size_t>(std::count_if(
      analyses.begin(), analyses.end(), [](const auto& a) { return a.has_regression; }));

  oss << "Summary:\n";
  oss << "  Total Benchmarks: " << total_benchmarks << "\n";
  oss << "  Regressions Found: " << regressions << "\n";
  oss << "  Success Rate: " << std::fixed << std::setprecision(1)
      << (100.0 * static_cast<double>(total_benchmarks - regressions) /
          static_cast<double>(total_benchmarks))
      << "%\n\n";

  // Group by severity
  std::map<std::string, std::vector<RegressionAnalysis>> by_severity;
  for (const auto& analysis : analyses) {
    if (analysis.has_regression) {
      by_severity[analysis.severity].push_back(analysis);
    }
  }

  for (const auto& [severity, group] : by_severity) {
    oss << severity << " Regressions (" << group.size() << "):\n";
    for (const auto& analysis : group) {
      oss << "  - " << analysis.benchmark_name << " (Time: " << analysis.time_regression_percentage
          << "%"
          << ", Memory: " << analysis.memory_regression_percentage << "%"
          << ", Ops: " << analysis.ops_regression_percentage << "%)\n";
    }
    oss << "\n";
  }

  return oss.str();
}

void RegressionDetector::generate_ci_output(const std::vector<RegressionAnalysis>& analyses,
                                            const std::string& output_path) {
  std::ofstream file(output_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open CI output file for writing");
  }

  file << "{\n";
  file << "  \"regression_summary\": {\n";
  file << "    \"total_benchmarks\": " << analyses.size() << ",\n";

  size_t regressions = static_cast<size_t>(std::count_if(
      analyses.begin(), analyses.end(), [](const auto& a) { return a.has_regression; }));
  file << "    \"regressions_found\": " << regressions << ",\n";
  file << "    \"has_critical_regressions\": "
       << (has_critical_regressions(analyses) ? "true" : "false") << "\n";
  file << "  },\n";

  file << "  \"analyses\": [\n";
  for (size_t i = 0; i < analyses.size(); ++i) {
    file << "    " << analyses[i].to_json();
    if (i < analyses.size() - 1) file << ",";
    file << "\n";
  }
  file << "  ]\n";
  file << "}\n";
}

bool RegressionDetector::has_critical_regressions(
    const std::vector<RegressionAnalysis>& analyses) const {
  return std::any_of(analyses.begin(), analyses.end(),
                     [](const auto& a) { return a.severity == "critical"; });
}

double RegressionDetector::calculate_regression_percentage(double baseline, double current) const {
  if (baseline == 0.0) return 0.0;
  return ((current - baseline) / baseline) * 100.0;
}

std::string RegressionDetector::determine_severity(double time_regression, double memory_regression,
                                                   double ops_regression) const {
  double max_regression = std::max({time_regression, memory_regression, ops_regression});

  if (max_regression >= 30.0) {
    return "critical";
  } else if (max_regression >= 20.0) {
    return "major";
  } else if (max_regression >= 10.0) {
    return "minor";
  } else {
    return "negligible";
  }
}

double RegressionDetector::calculate_trend_slope(const std::vector<double>& values) const {
  if (values.size() < 2) return 0.0;

  // Simple linear regression slope calculation
  size_t n = values.size();
  double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

  for (size_t i = 0; i < n; ++i) {
    double x = static_cast<double>(i);
    double y = values[i];

    sum_x += x;
    sum_y += y;
    sum_xy += x * y;
    sum_x2 += x * x;
  }

  double denominator = static_cast<double>(n) * sum_x2 - sum_x * sum_x;
  if (std::abs(denominator) < 1e-10) return 0.0;

  return (static_cast<double>(n) * sum_xy - sum_x * sum_y) / denominator;
}

void RegressionDetector::load_trend_data() {
  std::ifstream file(config_.trend_storage_path);
  if (!file.is_open()) {
    return;  // No trend file exists yet
  }

  // Simplified implementation - in production, use proper JSON parsing
  // For now, just initialize empty trend data
  trend_data_.clear();
}

void RegressionDetector::save_trend_data() {
  std::ofstream file(config_.trend_storage_path);
  if (!file.is_open()) {
    return;  // Fail silently for now
  }

  file << "{\n  \"trend_data\": {\n";

  bool first_benchmark = true;
  for (const auto& [benchmark_name, data_points] : trend_data_) {
    if (!first_benchmark) file << ",\n";

    file << "    \"" << benchmark_name << "\": [\n";

    bool first_point = true;
    for (const auto& point : data_points) {
      if (!first_point) file << ",\n";

      file << "      {\n";
      file << "        \"execution_time_ns\": " << point.execution_time.count() << ",\n";
      file << "        \"memory_usage_bytes\": " << point.memory_usage_bytes << ",\n";
      file << "        \"operations_per_second\": " << point.operations_per_second << ",\n";
      file << "        \"version\": \"" << point.version << "\"\n";
      file << "      }";

      first_point = false;
    }

    file << "\n    ]";
    first_benchmark = false;
  }

  file << "\n  }\n}\n";
}

}  // namespace SolarSystem::Testing
