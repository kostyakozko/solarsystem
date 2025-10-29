/**
 * @file diagnostic_system.hpp
 * @brief Comprehensive diagnostic and troubleshooting system
 */

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SolarSystem::Diagnostics {

/**
 * @brief Diagnostic severity levels
 */
enum class DiagnosticSeverity {
  INFO,
  WARNING,
  ERROR,
  CRITICAL
};

/**
 * @brief Diagnostic category
 */
enum class DiagnosticCategory {
  SYSTEM,
  MEMORY,
  PERFORMANCE,
  NETWORK,
  DATA,
  CONFIGURATION,
  SECURITY
};

/**
 * @brief Diagnostic issue
 */
struct DiagnosticIssue {
  std::string id;
  DiagnosticSeverity severity;
  DiagnosticCategory category;
  std::string title;
  std::string description;
  std::chrono::system_clock::time_point timestamp;
  std::map<std::string, std::string> metadata;
  std::vector<std::string> suggested_fixes;
  bool auto_fixable;

  DiagnosticIssue()
      : severity(DiagnosticSeverity::INFO),
        category(DiagnosticCategory::SYSTEM),
        timestamp(std::chrono::system_clock::now()),
        auto_fixable(false) {}
};

/**
 * @brief Diagnostic check result
 */
struct DiagnosticCheckResult {
  std::string check_name;
  bool passed;
  std::string message;
  std::vector<DiagnosticIssue> issues;
  std::chrono::milliseconds duration;

  DiagnosticCheckResult()
      : passed(true), duration(0) {}
};

/**
 * @brief Diagnostic check function type
 */
using DiagnosticCheckFunc = std::function<DiagnosticCheckResult()>;

/**
 * @brief Auto-fix function type
 */
using AutoFixFunc = std::function<bool(const DiagnosticIssue&)>;

/**
 * @brief Diagnostic check
 */
struct DiagnosticCheck {
  std::string name;
  std::string description;
  DiagnosticCategory category;
  DiagnosticCheckFunc check_func;
  bool enabled;

  DiagnosticCheck()
      : category(DiagnosticCategory::SYSTEM), enabled(true) {}
};

/**
 * @brief System health status
 */
struct SystemHealth {
  enum class Status {
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    CRITICAL
  };

  Status status;
  int total_checks;
  int passed_checks;
  int failed_checks;
  std::vector<DiagnosticIssue> issues;
  std::chrono::system_clock::time_point last_check;

  SystemHealth()
      : status(Status::HEALTHY),
        total_checks(0),
        passed_checks(0),
        failed_checks(0),
        last_check(std::chrono::system_clock::now()) {}
};

/**
 * @brief Diagnostic report
 */
struct DiagnosticReport {
  std::string report_id;
  std::chrono::system_clock::time_point generated_at;
  SystemHealth health;
  std::vector<DiagnosticCheckResult> check_results;
  std::map<std::string, std::string> system_info;

  std::string to_string() const;
  std::string to_json() const;
};

/**
 * @brief Diagnostic system
 */
class DiagnosticSystem {
public:
  static DiagnosticSystem& instance();

  // Check registration
  void register_check(const DiagnosticCheck& check);
  void unregister_check(const std::string& name);
  void enable_check(const std::string& name, bool enabled);

  // Auto-fix registration
  void register_auto_fix(const std::string& issue_id, AutoFixFunc fix_func);
  void unregister_auto_fix(const std::string& issue_id);

  // Diagnostic execution
  DiagnosticReport run_diagnostics();
  DiagnosticCheckResult run_check(const std::string& check_name);
  std::vector<DiagnosticCheckResult> run_category_checks(DiagnosticCategory category);

  // Health monitoring
  SystemHealth get_system_health() const;
  void update_health();

  // Issue management
  std::vector<DiagnosticIssue> get_issues() const;
  std::vector<DiagnosticIssue> get_issues_by_severity(DiagnosticSeverity severity) const;
  std::vector<DiagnosticIssue> get_issues_by_category(DiagnosticCategory category) const;
  void clear_issues();

  // Auto-fix
  bool attempt_auto_fix(const DiagnosticIssue& issue);
  int auto_fix_all_issues();

  // Report generation
  DiagnosticReport generate_report();
  std::string generate_summary() const;

  // System information
  void collect_system_info();
  std::map<std::string, std::string> get_system_info() const;

private:
  DiagnosticSystem() = default;

  mutable std::mutex mutex_;
  std::map<std::string, DiagnosticCheck> checks_;
  std::map<std::string, AutoFixFunc> auto_fixes_;
  std::vector<DiagnosticIssue> issues_;
  SystemHealth health_;
  std::map<std::string, std::string> system_info_;
};

/**
 * @brief Built-in diagnostic checks
 */
class BuiltInChecks {
public:
  static DiagnosticCheckResult check_memory_usage();
  static DiagnosticCheckResult check_disk_space();
  static DiagnosticCheckResult check_file_permissions();
  static DiagnosticCheckResult check_configuration();
  static DiagnosticCheckResult check_dependencies();
  static DiagnosticCheckResult check_network_connectivity();
  static DiagnosticCheckResult check_data_integrity();
};

/**
 * @brief Troubleshooting assistant
 */
class TroubleshootingAssistant {
public:
  static TroubleshootingAssistant& instance();

  // Problem detection
  std::vector<std::string> detect_problems();
  std::vector<std::string> suggest_solutions(const DiagnosticIssue& issue);

  // Common problems
  bool is_memory_issue(const DiagnosticIssue& issue);
  bool is_configuration_issue(const DiagnosticIssue& issue);
  bool is_network_issue(const DiagnosticIssue& issue);

  // Solution database
  void add_solution(const std::string& problem_pattern, const std::string& solution);
  std::vector<std::string> find_solutions(const std::string& problem);

private:
  TroubleshootingAssistant() = default;

  std::mutex mutex_;
  std::map<std::string, std::vector<std::string>> solution_database_;
};

/**
 * @brief Utility functions
 */
std::string severity_to_string(DiagnosticSeverity severity);
std::string category_to_string(DiagnosticCategory category);
DiagnosticSeverity string_to_severity(const std::string& str);
DiagnosticCategory string_to_category(const std::string& str);

}  // namespace SolarSystem::Diagnostics
