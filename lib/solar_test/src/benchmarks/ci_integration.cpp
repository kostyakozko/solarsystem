#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "solar_test/benchmarks/regression_detector.hpp"

namespace SolarSystem::Testing {

// CIIntegration implementation
void CIIntegration::generate_github_actions_output(const std::vector<RegressionAnalysis>& analyses,
                                                   const std::string& output_path) {
  std::ofstream file(output_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open GitHub Actions output file for writing");
  }

  size_t total_benchmarks = analyses.size();
  size_t regressions = std::count_if(analyses.begin(), analyses.end(),
                                     [](const auto& a) { return a.has_regression; });

  bool has_critical = std::any_of(analyses.begin(), analyses.end(),
                                  [](const auto& a) { return a.severity == "critical"; });

  file << "{\n";
  file << "  \"performance_summary\": {\n";
  file << "    \"total_benchmarks\": " << total_benchmarks << ",\n";
  file << "    \"regressions_found\": " << regressions << ",\n";
  file << "    \"success_rate\": " << std::fixed << std::setprecision(1)
       << (100.0 * (total_benchmarks - regressions) / total_benchmarks) << ",\n";
  file << "    \"has_critical_regressions\": " << (has_critical ? "true" : "false") << ",\n";
  file << "    \"status\": \""
       << (regressions == 0 ? "success" : (has_critical ? "failure" : "warning")) << "\"\n";
  file << "  },\n";

  file << "  \"benchmark_results\": [\n";
  for (size_t i = 0; i < analyses.size(); ++i) {
    const auto& analysis = analyses[i];
    file << "    {\n";
    file << "      \"name\": \"" << analysis.benchmark_name << "\",\n";
    file << "      \"has_regression\": " << (analysis.has_regression ? "true" : "false") << ",\n";
    file << "      \"severity\": \"" << analysis.severity << "\",\n";
    file << "      \"time_change_percent\": " << analysis.time_regression_percentage << ",\n";
    file << "      \"memory_change_percent\": " << analysis.memory_regression_percentage << ",\n";
    file << "      \"ops_change_percent\": " << analysis.ops_regression_percentage << "\n";
    file << "    }";
    if (i < analyses.size() - 1) file << ",";
    file << "\n";
  }
  file << "  ]\n";
  file << "}\n";
}

void CIIntegration::generate_jenkins_output(const std::vector<RegressionAnalysis>& analyses,
                                            const std::string& output_path) {
  std::ofstream file(output_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open Jenkins output file for writing");
  }

  // Jenkins XML format for test results
  file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  file << "<testsuite name=\"PerformanceRegressionTests\" tests=\"" << analyses.size() << "\"";

  size_t failures = std::count_if(analyses.begin(), analyses.end(),
                                  [](const auto& a) { return a.has_regression; });

  file << " failures=\"" << failures << "\" time=\"0\">\n";

  for (const auto& analysis : analyses) {
    file << "  <testcase name=\"" << analysis.benchmark_name
         << "\" classname=\"PerformanceTest\">\n";

    if (analysis.has_regression) {
      file << "    <failure message=\"Performance regression detected\">\n";
      file << "      Time regression: " << analysis.time_regression_percentage << "%\n";
      file << "      Memory regression: " << analysis.memory_regression_percentage << "%\n";
      file << "      Ops regression: " << analysis.ops_regression_percentage << "%\n";
      file << "      Severity: " << analysis.severity << "\n";
      file << "    </failure>\n";
    }

    file << "  </testcase>\n";
  }

  file << "</testsuite>\n";
}

void CIIntegration::set_github_step_output(const std::string& name, const std::string& value) {
  // Set GitHub Actions step output
  const char* github_output = std::getenv("GITHUB_OUTPUT");
  if (github_output) {
    std::ofstream file(github_output, std::ios::app);
    if (file.is_open()) {
      file << name << "=" << value << "\n";
    }
  }

  // Also output to stdout for visibility
  std::cout << "::set-output name=" << name << "::" << value << "\n";
}

void CIIntegration::exit_with_ci_code(const std::vector<RegressionAnalysis>& analyses,
                                      double failure_threshold) {
  bool has_major_regressions = false;

  for (const auto& analysis : analyses) {
    if (analysis.has_regression) {
      double max_regression = std::max({std::abs(analysis.time_regression_percentage),
                                        std::abs(analysis.memory_regression_percentage),
                                        std::abs(analysis.ops_regression_percentage)});

      if (max_regression >= failure_threshold) {
        has_major_regressions = true;
        break;
      }
    }
  }

  // Set GitHub Actions outputs
  set_github_step_output("has_regressions", has_major_regressions ? "true" : "false");
  set_github_step_output("regression_count", std::to_string(std::count_if(
                                                 analyses.begin(), analyses.end(),
                                                 [](const auto& a) { return a.has_regression; })));

  std::exit(has_major_regressions ? 1 : 0);
}

std::string CIIntegration::generate_performance_badge(
    const std::vector<RegressionAnalysis>& analyses) {
  if (analyses.empty()) {
    return "performance-unknown-lightgrey";
  }

  size_t total = analyses.size();
  size_t regressions = std::count_if(analyses.begin(), analyses.end(),
                                     [](const auto& a) { return a.has_regression; });

  double success_rate = 100.0 * (total - regressions) / total;

  std::ostringstream oss;
  oss << "performance-" << std::fixed << std::setprecision(0) << success_rate << "%25-";

  if (success_rate >= 95.0) {
    oss << "brightgreen";
  } else if (success_rate >= 85.0) {
    oss << "green";
  } else if (success_rate >= 70.0) {
    oss << "yellow";
  } else if (success_rate >= 50.0) {
    oss << "orange";
  } else {
    oss << "red";
  }

  return oss.str();
}

// PerformanceAlertSystem implementation
PerformanceAlertSystem::PerformanceAlertSystem(AlertConfig config) : config_(std::move(config)) {}

void PerformanceAlertSystem::send_regression_alert(const RegressionAnalysis& analysis) {
  if (!analysis.has_regression) {
    return;
  }

  std::ostringstream subject;
  subject << "Performance Regression Detected: " << analysis.benchmark_name;

  std::ostringstream body;
  body << "A performance regression has been detected in benchmark: " << analysis.benchmark_name
       << "\n\n";
  body << "Regression Details:\n";
  body << "- Severity: " << analysis.severity << "\n";
  body << "- Execution Time Change: " << analysis.time_regression_percentage << "%\n";
  body << "- Memory Usage Change: " << analysis.memory_regression_percentage << "%\n";
  body << "- Operations/Sec Change: " << analysis.ops_regression_percentage << "%\n\n";

  if (!analysis.alerts.empty()) {
    body << "Alerts:\n";
    for (const auto& alert : analysis.alerts) {
      body << "- " << alert << "\n";
    }
    body << "\n";
  }

  body << "Please investigate and address this regression.\n";

  if (config_.enable_email_alerts) {
    send_email_alert(subject.str(), body.str());
  }

  if (config_.enable_slack_alerts) {
    send_slack_alert(body.str());
  }

  if (config_.enable_github_issues) {
    create_github_issue(subject.str(), body.str());
  }
}

void PerformanceAlertSystem::send_trend_alert(const TrendAnalysis& analysis) {
  if (analysis.trend_direction != "degrading") {
    return;
  }

  std::ostringstream subject;
  subject << "Performance Trend Alert: " << analysis.benchmark_name;

  std::ostringstream body;
  body << "A degrading performance trend has been detected in benchmark: "
       << analysis.benchmark_name << "\n\n";
  body << "Trend Details:\n";
  body << "- Direction: " << analysis.trend_direction << "\n";
  body << "- Confidence: " << analysis.confidence_level << "%\n";
  body << "- Data Points: " << analysis.data_points.size() << "\n";
  body << "- Time Trend Slope: " << analysis.time_trend_slope << " ns/sample\n";
  body << "- Memory Trend Slope: " << analysis.memory_trend_slope << " bytes/sample\n\n";
  body << "Consider investigating the cause of this performance degradation.\n";

  if (config_.enable_email_alerts) {
    send_email_alert(subject.str(), body.str());
  }

  if (config_.enable_slack_alerts) {
    send_slack_alert(body.str());
  }
}

void PerformanceAlertSystem::send_batch_alert(const std::vector<RegressionAnalysis>& analyses) {
  size_t regressions = std::count_if(analyses.begin(), analyses.end(),
                                     [](const auto& a) { return a.has_regression; });

  if (regressions == 0) {
    return;
  }

  std::ostringstream subject;
  subject << "Performance Regression Report: " << regressions << " regressions found";

  std::ostringstream body;
  body << "Performance regression report summary:\n\n";
  body << "Total Benchmarks: " << analyses.size() << "\n";
  body << "Regressions Found: " << regressions << "\n";
  body << "Success Rate: " << std::fixed << std::setprecision(1)
       << (100.0 * (analyses.size() - regressions) / analyses.size()) << "%\n\n";

  body << "Regressions by Severity:\n";
  std::map<std::string, size_t> by_severity;
  for (const auto& analysis : analyses) {
    if (analysis.has_regression) {
      by_severity[analysis.severity]++;
    }
  }

  for (const auto& [severity, count] : by_severity) {
    body << "- " << severity << ": " << count << "\n";
  }

  body << "\nDetailed Results:\n";
  for (const auto& analysis : analyses) {
    if (analysis.has_regression) {
      body << "- " << analysis.benchmark_name << " (" << analysis.severity << "): "
           << "Time " << analysis.time_regression_percentage << "%, "
           << "Memory " << analysis.memory_regression_percentage << "%\n";
    }
  }

  if (config_.enable_email_alerts) {
    send_email_alert(subject.str(), body.str());
  }

  if (config_.enable_slack_alerts) {
    send_slack_alert(body.str());
  }
}

void PerformanceAlertSystem::send_email_alert(const std::string& subject, const std::string& body) {
  // Simplified email implementation - in production, use a proper email library
  std::cout << "EMAIL ALERT (would be sent if configured):\n";
  std::cout << "Subject: " << subject << "\n";
  std::cout << "Body:\n" << body << "\n";
  std::cout << "Recipients: ";
  for (const auto& recipient : config_.email_recipients) {
    std::cout << recipient << " ";
  }
  std::cout << "\n\n";
}

void PerformanceAlertSystem::send_slack_alert(const std::string& message) {
  // Simplified Slack implementation - in production, use HTTP client to send to webhook
  std::cout << "SLACK ALERT (would be sent if configured):\n";
  std::cout << "Webhook: " << config_.slack_webhook_url << "\n";
  std::cout << "Message: " << message << "\n\n";
}

void PerformanceAlertSystem::create_github_issue(const std::string& title,
                                                 const std::string& body) {
  // Simplified GitHub issue creation - in production, use GitHub API
  std::cout << "GITHUB ISSUE (would be created if configured):\n";
  std::cout << "Title: " << title << "\n";
  std::cout << "Body: " << body << "\n";
  std::cout << "Token: " << (config_.github_issue_token.empty() ? "not configured" : "configured")
            << "\n\n";
}

}  // namespace SolarSystem::Testing
