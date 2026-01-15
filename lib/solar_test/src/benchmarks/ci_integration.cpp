#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <curl/curl.h>

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
  size_t regressions = static_cast<size_t>(std::count_if(analyses.begin(), analyses.end(),
                                     [](const auto& a) { return a.has_regression; }));

  bool has_critical = std::any_of(analyses.begin(), analyses.end(),
                                  [](const auto& a) { return a.severity == "critical"; });

  file << "{\n";
  file << "  \"performance_summary\": {\n";
  file << "    \"total_benchmarks\": " << total_benchmarks << ",\n";
  file << "    \"regressions_found\": " << regressions << ",\n";
  file << "    \"success_rate\": " << std::fixed << std::setprecision(1)
       << (100.0 * static_cast<double>(total_benchmarks - regressions) /
           static_cast<double>(total_benchmarks))
       << ",\n";
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

  size_t failures = static_cast<size_t>(std::count_if(analyses.begin(), analyses.end(),
                                  [](const auto& a) { return a.has_regression; }));

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
  size_t regressions = static_cast<size_t>(std::count_if(analyses.begin(), analyses.end(),
                                     [](const auto& a) { return a.has_regression; }));

  double success_rate =
      100.0 * static_cast<double>(total - regressions) / static_cast<double>(total);

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
  size_t regressions = static_cast<size_t>(std::count_if(analyses.begin(), analyses.end(),
                                     [](const auto& a) { return a.has_regression; }));

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
       << (100.0 * static_cast<double>(analyses.size() - regressions) /
           static_cast<double>(analyses.size()))
       << "%\n\n";

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

// Email payload structure for libcurl
struct EmailPayload {
  std::vector<std::string> lines;
  size_t current_line = 0;
};

// Callback for libcurl to read email data
static size_t payload_source(char* ptr, size_t size, size_t nmemb, void* userp) {
  EmailPayload* payload = static_cast<EmailPayload*>(userp);

  if (size == 0 || nmemb == 0 || size * nmemb < 1) {
    return 0;
  }

  if (payload->current_line < payload->lines.size()) {
    const std::string& line = payload->lines[payload->current_line];
    size_t len = line.length();

    if (len > size * nmemb) {
      len = size * nmemb;
    }

    memcpy(ptr, line.c_str(), len);
    payload->current_line++;
    return len;
  }

  return 0;
}

// Send email using libcurl SMTP
static bool send_smtp_email(const std::string& smtp_url, const std::string& from,
                            const std::string& to, const std::string& subject,
                            const std::string& body, const std::string& username = "",
                            const std::string& password = "") {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  // Prepare email payload
  EmailPayload payload;
  payload.lines.push_back("From: <" + from + ">\r\n");
  payload.lines.push_back("To: <" + to + ">\r\n");
  payload.lines.push_back("Subject: " + subject + "\r\n");
  payload.lines.push_back("\r\n");

  // Split body into lines
  std::istringstream body_stream(body);
  std::string line;
  while (std::getline(body_stream, line)) {
    payload.lines.push_back(line + "\r\n");
  }

  // Configure libcurl
  curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
  curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + from + ">").c_str());

  struct curl_slist* recipients = nullptr;
  recipients = curl_slist_append(recipients, ("<" + to + ">").c_str());
  curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

  curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
  curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

  // Authentication if provided
  if (!username.empty()) {
    curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
  }

  // TLS/SSL settings
  curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

  // Timeouts
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

  // Perform the send
  CURLcode res = curl_easy_perform(curl);

  // Cleanup
  curl_slist_free_all(recipients);
  curl_easy_cleanup(curl);

  return res == CURLE_OK;
}

void PerformanceAlertSystem::send_email_alert(const std::string& subject, const std::string& body) {
  if (config_.email_recipients.empty()) {
    std::cout << "EMAIL ALERT: No recipients configured\n";
    return;
  }

  // Get SMTP configuration from environment
  const char* smtp_server = std::getenv("SMTP_SERVER");
  const char* smtp_port = std::getenv("SMTP_PORT");
  const char* smtp_from = std::getenv("SMTP_FROM");
  const char* smtp_user = std::getenv("SMTP_USER");
  const char* smtp_pass = std::getenv("SMTP_PASS");
  const char* smtp_tls = std::getenv("SMTP_TLS");

  // Build SMTP URL (supports smtp://, smtps://, smtp+tls://)
  std::string smtp_url;
  if (smtp_server) {
    bool use_tls = smtp_tls && std::string(smtp_tls) == "1";
    smtp_url = use_tls ? "smtps://" : "smtp://";
    smtp_url += smtp_server;
    if (smtp_port) {
      smtp_url += ":" + std::string(smtp_port);
    }
  } else {
    smtp_url = "smtp://localhost:25";
  }

  std::string from = smtp_from ? smtp_from : "noreply@solarsystem.local";
  std::string username = smtp_user ? smtp_user : "";
  std::string password = smtp_pass ? smtp_pass : "";

  // Send to all recipients
  bool sent = false;
  for (const auto& recipient : config_.email_recipients) {
    if (send_smtp_email(smtp_url, from, recipient, subject, body, username, password)) {
      sent = true;
      std::cout << "EMAIL ALERT: Sent to " << recipient << " via " << smtp_url << "\n";
    } else {
      std::cout << "EMAIL ALERT: Failed to send to " << recipient << "\n";
    }
  }

  if (!sent) {
    std::cout << "EMAIL ALERT (SMTP failed, check SMTP_SERVER/SMTP_USER/SMTP_PASS):\n";
    std::cout << "Subject: " << subject << "\nRecipients: ";
    for (const auto& recipient : config_.email_recipients) {
      std::cout << recipient << " ";
    }
    std::cout << "\nBody:\n" << body << "\n\n";
  }
}

void PerformanceAlertSystem::send_slack_alert(const std::string& message) {
  if (config_.slack_webhook_url.empty()) {
    std::cout << "SLACK ALERT: No webhook URL configured\n";
    return;
  }

  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cout << "SLACK ALERT: Failed to initialize curl\n";
    return;
  }

  // Escape JSON string
  auto escape_json = [](const std::string& str) -> std::string {
    std::string escaped;
    for (char c : str) {
      switch (c) {
        case '"':
          escaped += "\\\"";
          break;
        case '\\':
          escaped += "\\\\";
          break;
        case '\n':
          escaped += "\\n";
          break;
        case '\r':
          escaped += "\\r";
          break;
        case '\t':
          escaped += "\\t";
          break;
        default:
          escaped += c;
      }
    }
    return escaped;
  };

  // Build Slack JSON payload
  std::ostringstream json_payload;
  json_payload << "{"
               << "\"text\":\"" << escape_json(message) << "\","
               << "\"username\":\"Solar System Performance Monitor\","
               << "\"icon_emoji\":\":rocket:\""
               << "}";

  std::string payload = json_payload.str();

  // Configure curl
  curl_easy_setopt(curl, CURLOPT_URL, config_.slack_webhook_url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

  // Set headers
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  // SSL/TLS settings
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

  // Timeouts
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

  // Perform the request
  CURLcode res = curl_easy_perform(curl);

  // Cleanup
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res == CURLE_OK) {
    std::cout << "SLACK ALERT: Sent successfully\n";
  } else {
    std::cout << "SLACK ALERT: Failed to send - " << curl_easy_strerror(res) << "\n";
  }
}

void PerformanceAlertSystem::create_github_issue(const std::string& title,
                                                 const std::string& body) {
  if (config_.github_issue_token.empty()) {
    std::cout << "GITHUB ISSUE: No token configured\n";
    return;
  }

  // Get GitHub repo from environment (format: owner/repo)
  const char* github_repo_env = std::getenv("GITHUB_REPOSITORY");
  std::string github_repo = github_repo_env ? github_repo_env : "";

  if (github_repo.empty()) {
    std::cout << "GITHUB ISSUE: GITHUB_REPOSITORY env var not set (format: owner/repo)\n";
    return;
  }

  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cout << "GITHUB ISSUE: Failed to initialize curl\n";
    return;
  }

  // Escape JSON string
  auto escape_json = [](const std::string& str) -> std::string {
    std::string escaped;
    for (char c : str) {
      switch (c) {
        case '"':
          escaped += "\\\"";
          break;
        case '\\':
          escaped += "\\\\";
          break;
        case '\n':
          escaped += "\\n";
          break;
        case '\r':
          escaped += "\\r";
          break;
        case '\t':
          escaped += "\\t";
          break;
        default:
          escaped += c;
      }
    }
    return escaped;
  };

  // Build GitHub API URL
  std::string api_url = "https://api.github.com/repos/" + github_repo + "/issues";

  // Build JSON payload
  std::ostringstream json_payload;
  json_payload << "{"
               << "\"title\":\"" << escape_json(title) << "\","
               << "\"body\":\"" << escape_json(body) << "\","
               << "\"labels\":[\"performance\",\"automated\"]"
               << "}";

  std::string payload = json_payload.str();

  // Configure curl
  curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

  // Set headers
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
  headers = curl_slist_append(headers, ("Authorization: token " + config_.github_issue_token).c_str());
  headers = curl_slist_append(headers, "User-Agent: SolarSystem-Suite/4.0.0");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  // SSL/TLS settings
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

  // Timeouts
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

  // Capture response
  std::string response;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                     std::string* str = static_cast<std::string*>(userdata);
                     str->append(ptr, size * nmemb);
                     return size * nmemb;
                   });
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  // Perform the request
  CURLcode res = curl_easy_perform(curl);

  // Get HTTP response code
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  // Cleanup
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res == CURLE_OK && http_code == 201) {
    std::cout << "GITHUB ISSUE: Created successfully (#" << github_repo << ")\n";
  } else if (res == CURLE_OK) {
    std::cout << "GITHUB ISSUE: Failed with HTTP " << http_code << "\n";
  } else {
    std::cout << "GITHUB ISSUE: Failed - " << curl_easy_strerror(res) << "\n";
  }
}

}  // namespace SolarSystem::Testing
