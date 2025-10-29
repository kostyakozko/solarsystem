/**
 * @file test_diagnostic_system.cpp
 * @brief Unit tests for diagnostic and troubleshooting system
 */

#include "../utils/test_framework.h"

#include "solar_core/diagnostics/diagnostic_system.hpp"

using namespace SolarSystem::Diagnostics;

int main() {
  TestSuite suite("Diagnostic System Tests");

  // Utility function tests
  suite.run_test("Severity String Conversion", []() {
    if (severity_to_string(DiagnosticSeverity::INFO) != "INFO") {
      throw std::runtime_error("Wrong INFO string");
    }
    if (severity_to_string(DiagnosticSeverity::WARNING) != "WARNING") {
      throw std::runtime_error("Wrong WARNING string");
    }
    if (severity_to_string(DiagnosticSeverity::ERROR) != "ERROR") {
      throw std::runtime_error("Wrong ERROR string");
    }
    if (severity_to_string(DiagnosticSeverity::CRITICAL) != "CRITICAL") {
      throw std::runtime_error("Wrong CRITICAL string");
    }

    if (string_to_severity("INFO") != DiagnosticSeverity::INFO) {
      throw std::runtime_error("Wrong INFO parse");
    }
    if (string_to_severity("WARNING") != DiagnosticSeverity::WARNING) {
      throw std::runtime_error("Wrong WARNING parse");
    }
  });

  suite.run_test("Category String Conversion", []() {
    if (category_to_string(DiagnosticCategory::SYSTEM) != "SYSTEM") {
      throw std::runtime_error("Wrong SYSTEM string");
    }
    if (category_to_string(DiagnosticCategory::MEMORY) != "MEMORY") {
      throw std::runtime_error("Wrong MEMORY string");
    }

    if (string_to_category("SYSTEM") != DiagnosticCategory::SYSTEM) {
      throw std::runtime_error("Wrong SYSTEM parse");
    }
    if (string_to_category("MEMORY") != DiagnosticCategory::MEMORY) {
      throw std::runtime_error("Wrong MEMORY parse");
    }
  });

  // DiagnosticIssue tests
  suite.run_test("Diagnostic Issue Creation", []() {
    DiagnosticIssue issue;
    issue.id = "TEST_001";
    issue.severity = DiagnosticSeverity::WARNING;
    issue.category = DiagnosticCategory::CONFIGURATION;
    issue.title = "Test Issue";
    issue.description = "This is a test issue";
    issue.auto_fixable = true;
    issue.suggested_fixes.push_back("Fix 1");
    issue.suggested_fixes.push_back("Fix 2");

    if (issue.id != "TEST_001") throw std::runtime_error("Wrong ID");
    if (issue.severity != DiagnosticSeverity::WARNING) throw std::runtime_error("Wrong severity");
    if (issue.suggested_fixes.size() != 2) throw std::runtime_error("Wrong fixes count");
  });

  // DiagnosticCheck tests
  suite.run_test("Diagnostic Check Registration", []() {
    auto& diag = DiagnosticSystem::instance();

    DiagnosticCheck check;
    check.name = "test_check";
    check.description = "Test check";
    check.category = DiagnosticCategory::SYSTEM;
    check.enabled = true;
    check.check_func = []() {
      DiagnosticCheckResult result;
      result.check_name = "test_check";
      result.passed = true;
      result.message = "Check passed";
      return result;
    };

    diag.register_check(check);

    auto result = diag.run_check("test_check");
    if (!result.passed) throw std::runtime_error("Check should pass");
    if (result.check_name != "test_check") throw std::runtime_error("Wrong check name");

    diag.unregister_check("test_check");
  });

  suite.run_test("Diagnostic Check Enable/Disable", []() {
    auto& diag = DiagnosticSystem::instance();

    DiagnosticCheck check;
    check.name = "enable_test";
    check.description = "Enable test";
    check.category = DiagnosticCategory::SYSTEM;
    check.enabled = true;
    check.check_func = []() {
      DiagnosticCheckResult result;
      result.check_name = "enable_test";
      result.passed = true;
      return result;
    };

    diag.register_check(check);

    diag.enable_check("enable_test", false);
    auto result = diag.run_check("enable_test");
    if (result.passed) throw std::runtime_error("Disabled check should not pass");

    diag.enable_check("enable_test", true);
    result = diag.run_check("enable_test");
    if (!result.passed) throw std::runtime_error("Enabled check should pass");

    diag.unregister_check("enable_test");
  });

  // System health tests
  suite.run_test("System Health Status", []() {
    auto& diag = DiagnosticSystem::instance();

    auto health = diag.get_system_health();

    // Health should have valid status
    if (health.total_checks < 0) throw std::runtime_error("Invalid total checks");
    if (health.passed_checks < 0) throw std::runtime_error("Invalid passed checks");
  });

  // Issue management tests
  suite.run_test("Issue Management", []() {
    auto& diag = DiagnosticSystem::instance();
    diag.clear_issues();

    auto issues = diag.get_issues();
    if (!issues.empty()) throw std::runtime_error("Should have no issues after clear");
  });

  suite.run_test("Issues By Severity", []() {
    auto& diag = DiagnosticSystem::instance();
    diag.clear_issues();

    // Register a check that creates issues
    DiagnosticCheck check;
    check.name = "severity_test";
    check.description = "Severity test";
    check.category = DiagnosticCategory::SYSTEM;
    check.enabled = true;
    check.check_func = []() {
      DiagnosticCheckResult result;
      result.check_name = "severity_test";
      result.passed = false;

      DiagnosticIssue issue1;
      issue1.id = "ISSUE_1";
      issue1.severity = DiagnosticSeverity::WARNING;
      issue1.title = "Warning issue";
      result.issues.push_back(issue1);

      DiagnosticIssue issue2;
      issue2.id = "ISSUE_2";
      issue2.severity = DiagnosticSeverity::ERROR;
      issue2.title = "Error issue";
      result.issues.push_back(issue2);

      return result;
    };

    diag.register_check(check);
    diag.run_diagnostics();

    auto warnings = diag.get_issues_by_severity(DiagnosticSeverity::WARNING);
    auto errors = diag.get_issues_by_severity(DiagnosticSeverity::ERROR);

    if (warnings.empty()) throw std::runtime_error("Should have warning issues");
    if (errors.empty()) throw std::runtime_error("Should have error issues");

    diag.unregister_check("severity_test");
    diag.clear_issues();
  });

  suite.run_test("Issues By Category", []() {
    auto& diag = DiagnosticSystem::instance();
    diag.clear_issues();

    DiagnosticCheck check;
    check.name = "category_test";
    check.description = "Category test";
    check.category = DiagnosticCategory::MEMORY;
    check.enabled = true;
    check.check_func = []() {
      DiagnosticCheckResult result;
      result.check_name = "category_test";
      result.passed = false;

      DiagnosticIssue issue;
      issue.id = "MEM_001";
      issue.category = DiagnosticCategory::MEMORY;
      issue.title = "Memory issue";
      result.issues.push_back(issue);

      return result;
    };

    diag.register_check(check);
    diag.run_diagnostics();

    auto memory_issues = diag.get_issues_by_category(DiagnosticCategory::MEMORY);
    if (memory_issues.empty()) throw std::runtime_error("Should have memory issues");

    diag.unregister_check("category_test");
    diag.clear_issues();
  });

  // Auto-fix tests
  suite.run_test("Auto-Fix Registration", []() {
    auto& diag = DiagnosticSystem::instance();

    bool fix_called = false;
    diag.register_auto_fix("TEST_FIX", [&fix_called](const DiagnosticIssue&) {
      fix_called = true;
      return true;
    });

    DiagnosticIssue issue;
    issue.id = "TEST_FIX";
    issue.auto_fixable = true;

    bool result = diag.attempt_auto_fix(issue);
    if (!result) throw std::runtime_error("Auto-fix should succeed");
    if (!fix_called) throw std::runtime_error("Fix function should be called");

    diag.unregister_auto_fix("TEST_FIX");
  });

  // Report generation tests
  suite.run_test("Report Generation", []() {
    auto& diag = DiagnosticSystem::instance();
    diag.clear_issues();

    auto report = diag.generate_report();

    if (report.report_id.empty()) throw std::runtime_error("Report should have ID");
    if (report.check_results.empty()) {
      // It's okay if no checks are registered
    }
  });

  suite.run_test("Report String Format", []() {
    auto& diag = DiagnosticSystem::instance();
    diag.clear_issues();

    auto report = diag.generate_report();
    std::string report_str = report.to_string();

    if (report_str.empty()) throw std::runtime_error("Report string should not be empty");
    if (report_str.find("Diagnostic Report") == std::string::npos) {
      throw std::runtime_error("Report should contain title");
    }
  });

  suite.run_test("Report JSON Format", []() {
    auto& diag = DiagnosticSystem::instance();
    diag.clear_issues();

    auto report = diag.generate_report();
    std::string json = report.to_json();

    if (json.empty()) throw std::runtime_error("JSON should not be empty");
    if (json.find("report_id") == std::string::npos) {
      throw std::runtime_error("JSON should contain report_id");
    }
  });

  suite.run_test("Summary Generation", []() {
    auto& diag = DiagnosticSystem::instance();

    std::string summary = diag.generate_summary();

    if (summary.empty()) throw std::runtime_error("Summary should not be empty");
    if (summary.find("System Health") == std::string::npos) {
      throw std::runtime_error("Summary should contain health status");
    }
  });

  // Built-in checks tests
  suite.run_test("Built-in Memory Check", []() {
    auto result = BuiltInChecks::check_memory_usage();
    if (result.check_name.empty()) throw std::runtime_error("Check should have name");
  });

  suite.run_test("Built-in Disk Check", []() {
    auto result = BuiltInChecks::check_disk_space();
    if (result.check_name.empty()) throw std::runtime_error("Check should have name");
  });

  suite.run_test("Built-in Configuration Check", []() {
    auto result = BuiltInChecks::check_configuration();
    if (result.check_name.empty()) throw std::runtime_error("Check should have name");
  });

  // Troubleshooting assistant tests
  suite.run_test("Troubleshooting Assistant Problem Detection", []() {
    auto& assistant = TroubleshootingAssistant::instance();

    auto problems = assistant.detect_problems();
    // Should return list of problems (may be empty)
  });

  suite.run_test("Troubleshooting Assistant Solution Database", []() {
    auto& assistant = TroubleshootingAssistant::instance();

    assistant.add_solution("memory", "Increase memory allocation");
    assistant.add_solution("memory", "Reduce memory usage");

    auto solutions = assistant.find_solutions("memory leak detected");
    if (solutions.size() < 2) throw std::runtime_error("Should find at least 2 solutions");
  });

  suite.run_test("Troubleshooting Assistant Issue Classification", []() {
    auto& assistant = TroubleshootingAssistant::instance();

    DiagnosticIssue memory_issue;
    memory_issue.category = DiagnosticCategory::MEMORY;

    if (!assistant.is_memory_issue(memory_issue)) {
      throw std::runtime_error("Should identify as memory issue");
    }

    DiagnosticIssue config_issue;
    config_issue.category = DiagnosticCategory::CONFIGURATION;

    if (!assistant.is_configuration_issue(config_issue)) {
      throw std::runtime_error("Should identify as configuration issue");
    }
  });

  // System info tests
  suite.run_test("System Info Collection", []() {
    auto& diag = DiagnosticSystem::instance();

    diag.collect_system_info();
    auto info = diag.get_system_info();

    if (info.empty()) throw std::runtime_error("Should have system info");
  });

  // Category checks tests
  suite.run_test("Run Category Checks", []() {
    auto& diag = DiagnosticSystem::instance();

    DiagnosticCheck check;
    check.name = "category_check_test";
    check.description = "Category check test";
    check.category = DiagnosticCategory::PERFORMANCE;
    check.enabled = true;
    check.check_func = []() {
      DiagnosticCheckResult result;
      result.check_name = "category_check_test";
      result.passed = true;
      return result;
    };

    diag.register_check(check);

    auto results = diag.run_category_checks(DiagnosticCategory::PERFORMANCE);
    if (results.empty()) throw std::runtime_error("Should have performance check results");

    diag.unregister_check("category_check_test");
  });

  suite.print_summary();
  return suite.all_passed() ? 0 : 1;
}
