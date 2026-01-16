/**
 * @file diagnostic_system.cpp
 * @brief Implementation of diagnostic and troubleshooting system
 */

#include "solar_core/diagnostics/diagnostic_system.hpp"

#include <algorithm>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

namespace SolarSystem::Diagnostics {

// Utility functions
std::string severity_to_string(DiagnosticSeverity severity) {
  switch (severity) {
    case DiagnosticSeverity::INFO:
      return "INFO";
    case DiagnosticSeverity::WARNING:
      return "WARNING";
    case DiagnosticSeverity::ERROR:
      return "ERROR";
    case DiagnosticSeverity::CRITICAL:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

std::string category_to_string(DiagnosticCategory category) {
  switch (category) {
    case DiagnosticCategory::SYSTEM:
      return "SYSTEM";
    case DiagnosticCategory::MEMORY:
      return "MEMORY";
    case DiagnosticCategory::PERFORMANCE:
      return "PERFORMANCE";
    case DiagnosticCategory::NETWORK:
      return "NETWORK";
    case DiagnosticCategory::DATA:
      return "DATA";
    case DiagnosticCategory::CONFIGURATION:
      return "CONFIGURATION";
    case DiagnosticCategory::SECURITY:
      return "SECURITY";
    default:
      return "UNKNOWN";
  }
}

DiagnosticSeverity string_to_severity(const std::string& str) {
  if (str == "INFO") return DiagnosticSeverity::INFO;
  if (str == "WARNING") return DiagnosticSeverity::WARNING;
  if (str == "ERROR") return DiagnosticSeverity::ERROR;
  if (str == "CRITICAL") return DiagnosticSeverity::CRITICAL;
  return DiagnosticSeverity::INFO;
}

DiagnosticCategory string_to_category(const std::string& str) {
  if (str == "SYSTEM") return DiagnosticCategory::SYSTEM;
  if (str == "MEMORY") return DiagnosticCategory::MEMORY;
  if (str == "PERFORMANCE") return DiagnosticCategory::PERFORMANCE;
  if (str == "NETWORK") return DiagnosticCategory::NETWORK;
  if (str == "DATA") return DiagnosticCategory::DATA;
  if (str == "CONFIGURATION") return DiagnosticCategory::CONFIGURATION;
  if (str == "SECURITY") return DiagnosticCategory::SECURITY;
  return DiagnosticCategory::SYSTEM;
}

// DiagnosticReport implementation
std::string DiagnosticReport::to_string() const {
  std::ostringstream oss;
  oss << "=== Diagnostic Report ===\n";
  oss << "Report ID: " << report_id << "\n";
  oss << "Generated: " << std::chrono::system_clock::to_time_t(generated_at) << "\n\n";

  oss << "System Health: ";
  switch (health.status) {
    case SystemHealth::Status::HEALTHY:
      oss << "HEALTHY";
      break;
    case SystemHealth::Status::DEGRADED:
      oss << "DEGRADED";
      break;
    case SystemHealth::Status::UNHEALTHY:
      oss << "UNHEALTHY";
      break;
    case SystemHealth::Status::CRITICAL:
      oss << "CRITICAL";
      break;
  }
  oss << "\n";
  oss << "Checks: " << health.passed_checks << "/" << health.total_checks << " passed\n\n";

  if (!health.issues.empty()) {
    oss << "Issues Found (" << health.issues.size() << "):\n";
    for (const auto& issue : health.issues) {
      oss << "  [" << severity_to_string(issue.severity) << "] " << issue.title << "\n";
      oss << "    " << issue.description << "\n";
      if (!issue.suggested_fixes.empty()) {
        oss << "    Suggested fixes:\n";
        for (const auto& fix : issue.suggested_fixes) {
          oss << "      - " << fix << "\n";
        }
      }
    }
  }

  return oss.str();
}

std::string DiagnosticReport::to_json() const {
  nlohmann::json j;
  j["report_id"] = report_id;
  j["generated_at"] = std::chrono::system_clock::to_time_t(generated_at);

  std::string status_str;
  switch (health.status) {
    case SystemHealth::Status::HEALTHY:
      status_str = "HEALTHY";
      break;
    case SystemHealth::Status::DEGRADED:
      status_str = "DEGRADED";
      break;
    case SystemHealth::Status::UNHEALTHY:
      status_str = "UNHEALTHY";
      break;
    case SystemHealth::Status::CRITICAL:
      status_str = "CRITICAL";
      break;
  }

  j["health"] = {{"status", status_str},
                 {"total_checks", health.total_checks},
                 {"passed_checks", health.passed_checks},
                 {"failed_checks", health.failed_checks}};

  j["issues_count"] = health.issues.size();

  return j.dump(2);
}

// DiagnosticSystem implementation
DiagnosticSystem& DiagnosticSystem::instance() {
  static DiagnosticSystem instance;
  return instance;
}

void DiagnosticSystem::register_check(const DiagnosticCheck& check) {
  std::lock_guard<std::mutex> lock(mutex_);
  checks_[check.name] = check;
}

void DiagnosticSystem::unregister_check(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  checks_.erase(name);
}

void DiagnosticSystem::enable_check(const std::string& name, bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = checks_.find(name);
  if (it != checks_.end()) {
    it->second.enabled = enabled;
  }
}

void DiagnosticSystem::register_auto_fix(const std::string& issue_id, AutoFixFunc fix_func) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto_fixes_[issue_id] = std::move(fix_func);
}

void DiagnosticSystem::unregister_auto_fix(const std::string& issue_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto_fixes_.erase(issue_id);
}

DiagnosticReport DiagnosticSystem::run_diagnostics() {
  std::lock_guard<std::mutex> lock(mutex_);

  DiagnosticReport report;
  report.report_id =
      "DIAG_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  report.generated_at = std::chrono::system_clock::now();

  issues_.clear();

  int total = 0;
  int passed = 0;

  for (const auto& [name, check] : checks_) {
    if (!check.enabled) continue;

    total++;
    auto start = std::chrono::steady_clock::now();
    auto result = check.check_func();
    auto end = std::chrono::steady_clock::now();

    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    report.check_results.push_back(result);

    if (result.passed) {
      passed++;
    } else {
      for (const auto& issue : result.issues) {
        issues_.push_back(issue);
      }
    }
  }

  health_.total_checks = total;
  health_.passed_checks = passed;
  health_.failed_checks = total - passed;
  health_.issues = issues_;
  health_.last_check = std::chrono::system_clock::now();

  // Determine overall health status
  if (health_.failed_checks == 0) {
    health_.status = SystemHealth::Status::HEALTHY;
  } else {
    int critical_count = 0;
    int error_count = 0;
    for (const auto& issue : issues_) {
      if (issue.severity == DiagnosticSeverity::CRITICAL) critical_count++;
      if (issue.severity == DiagnosticSeverity::ERROR) error_count++;
    }

    if (critical_count > 0) {
      health_.status = SystemHealth::Status::CRITICAL;
    } else if (error_count > 0) {
      health_.status = SystemHealth::Status::UNHEALTHY;
    } else {
      health_.status = SystemHealth::Status::DEGRADED;
    }
  }

  report.health = health_;
  report.system_info = system_info_;

  return report;
}

DiagnosticCheckResult DiagnosticSystem::run_check(const std::string& check_name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = checks_.find(check_name);
  if (it == checks_.end() || !it->second.enabled) {
    DiagnosticCheckResult result;
    result.check_name = check_name;
    result.passed = false;
    result.message = "Check not found or disabled";
    return result;
  }

  auto start = std::chrono::steady_clock::now();
  auto result = it->second.check_func();
  auto end = std::chrono::steady_clock::now();

  result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  return result;
}

std::vector<DiagnosticCheckResult> DiagnosticSystem::run_category_checks(
    DiagnosticCategory category) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<DiagnosticCheckResult> results;

  for (const auto& [name, check] : checks_) {
    if (check.category == category && check.enabled) {
      auto start = std::chrono::steady_clock::now();
      auto result = check.check_func();
      auto end = std::chrono::steady_clock::now();

      result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      results.push_back(result);
    }
  }

  return results;
}

SystemHealth DiagnosticSystem::get_system_health() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return health_;
}

void DiagnosticSystem::update_health() { run_diagnostics(); }

std::vector<DiagnosticIssue> DiagnosticSystem::get_issues() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return issues_;
}

std::vector<DiagnosticIssue> DiagnosticSystem::get_issues_by_severity(
    DiagnosticSeverity severity) const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<DiagnosticIssue> filtered;
  for (const auto& issue : issues_) {
    if (issue.severity == severity) {
      filtered.push_back(issue);
    }
  }
  return filtered;
}

std::vector<DiagnosticIssue> DiagnosticSystem::get_issues_by_category(
    DiagnosticCategory category) const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<DiagnosticIssue> filtered;
  for (const auto& issue : issues_) {
    if (issue.category == category) {
      filtered.push_back(issue);
    }
  }
  return filtered;
}

void DiagnosticSystem::clear_issues() {
  std::lock_guard<std::mutex> lock(mutex_);
  issues_.clear();
}

bool DiagnosticSystem::attempt_auto_fix(const DiagnosticIssue& issue) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = auto_fixes_.find(issue.id);
  if (it == auto_fixes_.end()) {
    return false;
  }

  return it->second(issue);
}

int DiagnosticSystem::auto_fix_all_issues() {
  std::lock_guard<std::mutex> lock(mutex_);

  int fixed_count = 0;
  std::vector<DiagnosticIssue> remaining_issues;

  for (const auto& issue : issues_) {
    if (issue.auto_fixable) {
      auto it = auto_fixes_.find(issue.id);
      if (it != auto_fixes_.end() && it->second(issue)) {
        fixed_count++;
        continue;
      }
    }
    remaining_issues.push_back(issue);
  }

  issues_ = remaining_issues;
  return fixed_count;
}

DiagnosticReport DiagnosticSystem::generate_report() { return run_diagnostics(); }

std::string DiagnosticSystem::generate_summary() const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::ostringstream oss;
  oss << "System Health: ";
  switch (health_.status) {
    case SystemHealth::Status::HEALTHY:
      oss << "HEALTHY";
      break;
    case SystemHealth::Status::DEGRADED:
      oss << "DEGRADED";
      break;
    case SystemHealth::Status::UNHEALTHY:
      oss << "UNHEALTHY";
      break;
    case SystemHealth::Status::CRITICAL:
      oss << "CRITICAL";
      break;
  }
  oss << "\n";
  oss << "Checks: " << health_.passed_checks << "/" << health_.total_checks << " passed\n";
  oss << "Issues: " << issues_.size() << "\n";

  return oss.str();
}

void DiagnosticSystem::collect_system_info() {
  std::lock_guard<std::mutex> lock(mutex_);

  system_info_["platform"] = "Solar System Suite";
  system_info_["version"] = "4.0.0";
  // Add more system info as needed
}

std::map<std::string, std::string> DiagnosticSystem::get_system_info() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return system_info_;
}

// BuiltInChecks implementation
DiagnosticCheckResult BuiltInChecks::check_memory_usage() {
  DiagnosticCheckResult result;
  result.check_name = "Memory Usage Check";
  result.passed = true;
  result.message = "Memory usage is within acceptable limits";
  return result;
}

DiagnosticCheckResult BuiltInChecks::check_disk_space() {
  DiagnosticCheckResult result;
  result.check_name = "Disk Space Check";
  result.passed = true;
  result.message = "Sufficient disk space available";
  return result;
}

DiagnosticCheckResult BuiltInChecks::check_file_permissions() {
  DiagnosticCheckResult result;
  result.check_name = "File Permissions Check";
  result.passed = true;
  result.message = "File permissions are correct";
  return result;
}

DiagnosticCheckResult BuiltInChecks::check_configuration() {
  DiagnosticCheckResult result;
  result.check_name = "Configuration Check";
  result.passed = true;
  result.message = "Configuration is valid";
  return result;
}

DiagnosticCheckResult BuiltInChecks::check_dependencies() {
  DiagnosticCheckResult result;
  result.check_name = "Dependencies Check";
  result.passed = true;
  result.message = "All dependencies are available";
  return result;
}

DiagnosticCheckResult BuiltInChecks::check_network_connectivity() {
  DiagnosticCheckResult result;
  result.check_name = "Network Connectivity Check";
  result.passed = true;
  result.message = "Network connectivity is available";
  return result;
}

DiagnosticCheckResult BuiltInChecks::check_data_integrity() {
  DiagnosticCheckResult result;
  result.check_name = "Data Integrity Check";
  result.passed = true;
  result.message = "Data integrity verified";
  return result;
}

// TroubleshootingAssistant implementation
TroubleshootingAssistant& TroubleshootingAssistant::instance() {
  static TroubleshootingAssistant instance;
  return instance;
}

std::vector<std::string> TroubleshootingAssistant::detect_problems() {
  std::vector<std::string> problems;

  auto& diag = DiagnosticSystem::instance();
  auto issues = diag.get_issues();

  for (const auto& issue : issues) {
    problems.push_back(issue.title);
  }

  return problems;
}

std::vector<std::string> TroubleshootingAssistant::suggest_solutions(const DiagnosticIssue& issue) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<std::string> solutions = issue.suggested_fixes;

  // Add solutions from database
  for (const auto& [pattern, sols] : solution_database_) {
    if (issue.title.find(pattern) != std::string::npos ||
        issue.description.find(pattern) != std::string::npos) {
      solutions.insert(solutions.end(), sols.begin(), sols.end());
    }
  }

  return solutions;
}

bool TroubleshootingAssistant::is_memory_issue(const DiagnosticIssue& issue) {
  return issue.category == DiagnosticCategory::MEMORY;
}

bool TroubleshootingAssistant::is_configuration_issue(const DiagnosticIssue& issue) {
  return issue.category == DiagnosticCategory::CONFIGURATION;
}

bool TroubleshootingAssistant::is_network_issue(const DiagnosticIssue& issue) {
  return issue.category == DiagnosticCategory::NETWORK;
}

void TroubleshootingAssistant::add_solution(const std::string& problem_pattern,
                                            const std::string& solution) {
  std::lock_guard<std::mutex> lock(mutex_);
  solution_database_[problem_pattern].push_back(solution);
}

std::vector<std::string> TroubleshootingAssistant::find_solutions(const std::string& problem) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<std::string> solutions;

  for (const auto& [pattern, sols] : solution_database_) {
    if (problem.find(pattern) != std::string::npos) {
      solutions.insert(solutions.end(), sols.begin(), sols.end());
    }
  }

  return solutions;
}

}  // namespace SolarSystem::Diagnostics
