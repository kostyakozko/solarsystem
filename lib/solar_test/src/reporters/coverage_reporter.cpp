#include "solar_test/reporters/coverage_reporter.hpp"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace SolarSystem::Testing {

CoverageReporter::CoverageReporter(Configuration config) : config_(std::move(config)) {
  output_file_.open(config_.output_file);
  if (!output_file_.is_open()) {
    throw std::runtime_error("Failed to open coverage output file: " + config_.output_file);
  }
}

CoverageReporter::CoverageReporter(const std::string& output_file)
    : CoverageReporter(Configuration{output_file}) {}

void CoverageReporter::on_suite_started(const std::string& suite_name, size_t total_tests) {
  if (!quiet_) {
    output_file_ << "Coverage Report for Test Suite: " << suite_name << "\n";
    output_file_ << "Total Tests: " << total_tests << "\n";
    output_file_ << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count()
                 << "\n\n";
  }
}

void CoverageReporter::on_suite_finished(const TestSuiteResult& result) {
  // Update overall coverage from test suite result
  overall_coverage_percentage_ = result.code_coverage_percentage;

  // Generate the appropriate report format
  if (config_.format == "html") {
    generate_html_report();
  } else if (config_.format == "lcov") {
    generate_lcov_report();
  } else {
    generate_text_report();
  }

  output_file_.flush();
}

void CoverageReporter::on_test_started(const std::string& test_name) {
  // Coverage reporter doesn't need to track individual test starts
  (void)test_name;  // Suppress unused parameter warning
}

void CoverageReporter::on_test_finished(const TestResult& result) {
  // Coverage reporter doesn't need to track individual test results
  (void)result;  // Suppress unused parameter warning
}

void CoverageReporter::on_progress(const std::string& message, double percentage) {
  // Coverage reporter doesn't output progress information
  (void)message;     // Suppress unused parameter warning
  (void)percentage;  // Suppress unused parameter warning
}

void CoverageReporter::on_error(const std::string& error_message) {
  output_file_ << "ERROR: " << error_message << "\n";
}

void CoverageReporter::set_coverage_data(const std::map<std::string, double>& file_coverage) {
  file_coverage_data_ = file_coverage;

  // Calculate overall coverage
  if (!file_coverage_data_.empty()) {
    double total_coverage =
        std::accumulate(file_coverage_data_.begin(), file_coverage_data_.end(), 0.0,
                        [](double sum, const auto& pair) { return sum + pair.second; });
    overall_coverage_percentage_ = total_coverage / static_cast<double>(file_coverage_data_.size());
  }
}

void CoverageReporter::add_file_coverage(const std::string& file_path, double coverage_percentage) {
  file_coverage_data_[file_path] = coverage_percentage;

  // Recalculate overall coverage
  if (!file_coverage_data_.empty()) {
    double total_coverage =
        std::accumulate(file_coverage_data_.begin(), file_coverage_data_.end(), 0.0,
                        [](double sum, const auto& pair) { return sum + pair.second; });
    overall_coverage_percentage_ = total_coverage / static_cast<double>(file_coverage_data_.size());
  }
}

double CoverageReporter::overall_coverage() const { return overall_coverage_percentage_; }

bool CoverageReporter::meets_threshold() const {
  return overall_coverage_percentage_ >= config_.minimum_coverage_threshold;
}

void CoverageReporter::generate_text_report() {
  output_file_ << "=== CODE COVERAGE REPORT ===\n\n";

  write_coverage_summary();

  if (!file_coverage_data_.empty()) {
    output_file_ << "\n=== FILE COVERAGE DETAILS ===\n\n";
    write_file_coverage_details();
  }

  output_file_ << "\n=== COVERAGE ANALYSIS ===\n\n";

  if (config_.minimum_coverage_threshold > 0.0) {
    output_file_ << "Coverage Threshold: " << format_percentage(config_.minimum_coverage_threshold)
                 << "\n";
    output_file_ << "Threshold Status: " << (meets_threshold() ? "PASSED" : "FAILED") << "\n";
  }

  output_file_ << "Coverage Status: " << get_coverage_status(overall_coverage_percentage_) << "\n";
}

void CoverageReporter::generate_html_report() {
  output_file_ << "<!DOCTYPE html>\n";
  output_file_ << "<html>\n<head>\n";
  output_file_ << "<title>Code Coverage Report</title>\n";
  output_file_ << "<style>\n";
  output_file_ << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
  output_file_ << ".summary { background-color: #f0f0f0; padding: 15px; border-radius: 5px; }\n";
  output_file_ << ".high-coverage { color: green; font-weight: bold; }\n";
  output_file_ << ".medium-coverage { color: orange; font-weight: bold; }\n";
  output_file_ << ".low-coverage { color: red; font-weight: bold; }\n";
  output_file_ << "table { border-collapse: collapse; width: 100%; margin-top: 20px; }\n";
  output_file_ << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
  output_file_ << "th { background-color: #f2f2f2; }\n";
  output_file_ << "</style>\n</head>\n<body>\n";

  output_file_ << "<h1>Code Coverage Report</h1>\n";

  output_file_ << "<div class=\"summary\">\n";
  output_file_ << "<h2>Coverage Summary</h2>\n";
  output_file_ << "<p>Overall Coverage: <span class=\""
               << (overall_coverage_percentage_ >= 80   ? "high"
                   : overall_coverage_percentage_ >= 60 ? "medium"
                                                        : "low")
               << "-coverage\">" << format_percentage(overall_coverage_percentage_)
               << "</span></p>\n";

  if (config_.minimum_coverage_threshold > 0.0) {
    output_file_ << "<p>Threshold: " << format_percentage(config_.minimum_coverage_threshold)
                 << " - <strong>" << (meets_threshold() ? "PASSED" : "FAILED") << "</strong></p>\n";
  }
  output_file_ << "</div>\n";

  if (!file_coverage_data_.empty()) {
    output_file_ << "<h2>File Coverage Details</h2>\n";
    output_file_ << "<table>\n";
    output_file_ << "<tr><th>File</th><th>Coverage</th><th>Status</th></tr>\n";

    for (const auto& [file_path, coverage] : file_coverage_data_) {
      std::string status_class = coverage >= 80 ? "high" : coverage >= 60 ? "medium" : "low";
      output_file_ << "<tr>\n";
      output_file_ << "<td>" << file_path << "</td>\n";
      output_file_ << "<td class=\"" << status_class << "-coverage\">"
                   << format_percentage(coverage) << "</td>\n";
      output_file_ << "<td>" << get_coverage_status(coverage) << "</td>\n";
      output_file_ << "</tr>\n";
    }

    output_file_ << "</table>\n";
  }

  output_file_ << "</body>\n</html>\n";
}

void CoverageReporter::generate_lcov_report() {
  // Basic LCOV format output
  output_file_ << "TN:\n";  // Test name (empty)

  for (const auto& [file_path, coverage] : file_coverage_data_) {
    output_file_ << "SF:" << file_path << "\n";  // Source file

    // For simplicity, we'll assume 100 lines per file and calculate covered lines
    int total_lines = 100;
    int covered_lines = static_cast<int>(coverage * total_lines / 100.0);

    // Line data (simplified)
    for (int i = 1; i <= total_lines; ++i) {
      output_file_ << "DA:" << i << "," << (i <= covered_lines ? "1" : "0") << "\n";
    }

    output_file_ << "LF:" << total_lines << "\n";    // Lines found
    output_file_ << "LH:" << covered_lines << "\n";  // Lines hit
    output_file_ << "end_of_record\n";
  }
}

void CoverageReporter::write_coverage_summary() {
  output_file_ << "Overall Coverage: " << format_percentage(overall_coverage_percentage_) << "\n";

  if (!file_coverage_data_.empty()) {
    output_file_ << "Files Analyzed: " << file_coverage_data_.size() << "\n";

    // Calculate statistics
    auto min_max =
        std::minmax_element(file_coverage_data_.begin(), file_coverage_data_.end(),
                            [](const auto& a, const auto& b) { return a.second < b.second; });

    output_file_ << "Minimum Coverage: " << format_percentage(min_max.first->second) << " ("
                 << min_max.first->first << ")\n";
    output_file_ << "Maximum Coverage: " << format_percentage(min_max.second->second) << " ("
                 << min_max.second->first << ")\n";
  }
}

void CoverageReporter::write_file_coverage_details() {
  // Sort files by coverage percentage (lowest first)
  std::vector<std::pair<std::string, double>> sorted_files(file_coverage_data_.begin(),
                                                           file_coverage_data_.end());

  std::sort(sorted_files.begin(), sorted_files.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

  output_file_ << std::left << std::setw(50) << "File" << std::setw(12) << "Coverage"
               << "Status\n";
  output_file_ << std::string(70, '-') << "\n";

  for (const auto& [file_path, coverage] : sorted_files) {
    output_file_ << std::left << std::setw(50) << file_path << std::setw(12)
                 << format_percentage(coverage) << get_coverage_status(coverage) << "\n";
  }
}

std::string CoverageReporter::format_percentage(double percentage) const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << percentage << "%";
  return oss.str();
}

std::string CoverageReporter::get_coverage_status(double percentage) const {
  if (percentage >= 90.0) return "Excellent";
  if (percentage >= 80.0) return "Good";
  if (percentage >= 70.0) return "Fair";
  if (percentage >= 60.0) return "Poor";
  return "Critical";
}

}  // namespace SolarSystem::Testing
