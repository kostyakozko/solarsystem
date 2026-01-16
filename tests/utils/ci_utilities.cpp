/**
 * @file ci_utilities.cpp
 * @brief Implementation of CI/CD integration utilities
 */

#include "ci_utilities.hpp"

#include <sys/resource.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace TestUtils {
namespace CI {

// CIArtifactGenerator implementation

bool CIArtifactGenerator::generate_junit_xml(const std::vector<TestSuiteResult>& suites,
                                             const std::string& output_file) {
  std::ofstream file(output_file);
  if (!file.is_open()) {
    return false;
  }

  // XML header
  file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  file << "<testsuites>\n";

  // Write each test suite
  for (const auto& suite : suites) {
    file << "  <testsuite name=\"" << suite.name << "\" tests=\"" << suite.tests << "\" failures=\""
         << suite.failures << "\" errors=\"" << suite.errors << "\" skipped=\"" << suite.skipped
         << "\" time=\"" << std::fixed << std::setpr << suite.time_seconds << "\">\n";

    // Write each test case
    for (const auto& test_case : suite.test_cases) {
      file << "    <testcase name=\"" << test_case.name << "\" classname=\"" << test_case.classname
           << "\" time=\"" << std::fixed << std::setprecision(3) << test_case.time_seconds
           << "\">\n";

      if (!test_case.passed) {
        file << "      <failure message=\"" << test_case.failure_message << "\" type=\""
             << test_case.failure_type << "\">\n";
        file << "        " << test_case.failure_message << "\n";
        file << "      </failure>\n";
      }

      if (!test_case.system_out.empty()) {
        file << "      <system-out>" << test_case.system_out << "</system-out>\n";
      }

      if (!test_case.system_err.empty()) {
        file << "      <system-err>" << test_case.system_err << "</system-err>\n";
      }

      file << "    </testcase>\n";
    }

    file << "  </testsuite>\n";
  }

  file << "</testsuites>\n";
  file.close();

  return true;
}

bool CIArtifactGenerator::generate_coverage_report(const std::string& coverage_data_file,
                                                   const std::string& output_dir,
                                                   const std::string& format) {
  // Create output directory
  std::filesystem::create_directories(output_dir);

  if (format == "html") {
    // Generate simple HTML coverage report
    std::ofstream file(output_dir + "/coverage.html");
    if (!file.is_open()) {
      return false;
    }

    file << "<!DOCTYPE html>\n";
    file << "<html><head><title>Coverage Report</title></head>\n";
    file << "<body><h1>Code Coverage Report</h1>\n";
    file << "<p>Coverage data file: " << coverage_data_file << "</p>\n";
    file << "<p>Note: Full coverage analysis requires gcov/lcov integration</p>\n";
    file << "</body></html>\n";
    file.close();
  } else if (format == "xml") {
    // Generate XML coverage report (Cobertura format)
    std::ofstream file(output_dir + "/coverage.xml");
    if (!file.is_open()) {
      return false;
    }

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<coverage version=\"1.0\">\n";
    file << "  <sources><source>.</source></sources>\n";
    file << "  <packages></packages>\n";
    file << "</coverage>\n";
    file.close();
  }

  return true;
}

bool CIArtifactGenerator::generate_performance_report(const std::map<std::string, double>& metrics,
                                                      const std::string& output_file) {
  std::ofstream file(output_file);
  if (!file.is_open()) {
    return false;
  }

  file << "# Performance Report\n\n";
  file << "| Metric | Value |\n";
  file << "|--------|-------|\n";

  for (const auto& [name, value] : metrics) {
    file << "| " << name << " | " << std::fixed << std::setprecision(3) << value << " |\n";
  }

  file.close();
  return true;
}

TestSuiteResult CIArtifactGenerator::aggregate_results(
    const std::vector<TestCaseResult>& test_cases, const std::string& suite_name) {
  TestSuiteResult suite;
  suite.name = suite_name;
  suite.tests = static_cast<int>(test_cases.size());
  suite.failures = 0;
  suite.errors = 0;
  suite.skipped = 0;
  suite.time_seconds = 0.0;
  suite.test_cases = test_cases;

  for (const auto& test_case : test_cases) {
    suite.time_seconds += test_case.time_seconds;
    if (!test_case.passed) {
      if (test_case.failure_type == "error") {
        suite.errors++;
      } else {
        suite.failures++;
      }
    }
  }

  return suite;
}

// EnvironmentDetector implementation

bool EnvironmentDetector::is_docker_container() {
  // Check for /.dockerenv file
  if (std::filesystem::exists("/.dockerenv")) {
    return true;
  }

  // Check /proc/1/cgroup for docker
  std::ifstream cgroup("/proc/1/cgroup");
  if (cgroup.is_open()) {
    std::string line;
    while (std::getline(cgroup, line)) {
      if (line.find("docker") != std::string::npos) {
        return true;
      }
    }
  }

  return false;
}

bool EnvironmentDetector::is_kubernetes_pod() {
  // Check for Kubernetes environment variables
  const char* k8s_service_host = std::getenv("KUBERNETES_SERVICE_HOST");
  const char* k8s_service_port = std::getenv("KUBERNETES_SERVICE_PORT");

  if (k8s_service_host && k8s_service_port) {
    return true;
  }

  // Check for /var/run/secrets/kubernetes.io
  if (std::filesystem::exists("/var/run/secrets/kubernetes.io")) {
    return true;
  }

  return false;
}

std::string EnvironmentDetector::detect_ci_system() {
  // GitHub Actions
  if (std::getenv("GITHUB_ACTIONS")) {
    return "GitHub Actions";
  }

  // GitLab CI
  if (std::getenv("GITLAB_CI")) {
    return "GitLab CI";
  }

  // Jenkins
  if (std::getenv("JENKINS_URL") || std::getenv("JENKINS_HOME")) {
    return "Jenkins";
  }

  // Travis CI
  if (std::getenv("TRAVIS")) {
    return "Travis CI";
  }

  // CircleCI
  if (std::getenv("CIRCLECI")) {
    return "CircleCI";
  }

  // Azure Pipelines
  if (std::getenv("TF_BUILD")) {
    return "Azure Pipelines";
  }

  // Generic CI detection
  if (std::getenv("CI")) {
    return "Generic CI";
  }

  return "None";
}

bool EnvironmentDetector::is_ci_environment() { return detect_ci_system() != "None"; }

std::string EnvironmentDetector::detect_cloud_platform() {
  // AWS
  if (std::filesystem::exists("/sys/hypervisor/uuid")) {
    std::ifstream uuid_file("/sys/hypervisor/uuid");
    std::string uuid;
    if (uuid_file >> uuid) {
      if (uuid.substr(0, 3) == "ec2" || uuid.substr(0, 2) == "EC") {
        return "AWS";
      }
    }
  }

  // Check for AWS metadata service
  if (std::getenv("AWS_EXECUTION_ENV") || std::getenv("AWS_REGION")) {
    return "AWS";
  }

  // Azure
  if (std::getenv("AZURE_HTTP_USER_AGENT") || std::filesystem::exists("/var/lib/waagent")) {
    return "Azure";
  }

  // GCP
  if (std::getenv("GCE_METADATA_HOST") || std::filesystem::exists("/var/lib/google")) {
    return "GCP";
  }

  return "None";
}

std::map<std::string, std::string> EnvironmentDetector::get_environment_info() {
  std::map<std::string, std::string> info;

  info["ci_system"] = detect_ci_system();
  info["is_ci"] = is_ci_environment() ? "true" : "false";
  info["is_docker"] = is_docker_container() ? "true" : "false";
  info["is_kubernetes"] = is_kubernetes_pod() ? "true" : "false";
  info["cloud_platform"] = detect_cloud_platform();

  // Add hostname
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    info["hostname"] = hostname;
  }

  return info;
}

// EnhancedMemoryMonitor implementation

EnhancedMemoryMonitor::MemoryUsage EnhancedMemoryMonitor::get_current_usage() {
  MemoryUsage usage{};

  struct rusage rusage_data;
  getrusage(RUSAGE_SELF, &rusage_data);

#ifdef __APPLE__
  // On macOS, ru_maxrss is in bytes
  usage.rss_bytes = static_cast<size_t>(rusage_data.ru_maxrss);
  usage.peak_rss_bytes = usage.rss_bytes;
#else
  // On Linux, ru_maxrss is in kilobytes
  usage.rss_bytes = static_cast<size_t>(rusage_data.ru_maxrss) * 1024;
  usage.peak_rss_bytes = usage.rss_bytes;
#endif

  // Get system memory info
  long pages = sysconf(_SC_PHYS_PAGES);
  long avail_pages = sysconf(_SC_AVPHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);

  if (pages > 0 && page_size > 0) {
    size_t total_memory = static_cast<size_t>(pages) * static_cast<size_t>(page_size);
    usage.available_bytes = static_cast<size_t>(avail_pages) * static_cast<size_t>(page_size);
    usage.usage_percent =
        (static_cast<double>(usage.rss_bytes) / static_cast<double>(total_memory)) * 100.0;
  }

  // Virtual memory size (approximate)
  usage.virtual_bytes = usage.rss_bytes * 2;  // Simplified estimate

  return usage;
}

bool EnhancedMemoryMonitor::check_memory_available(size_t required_bytes) {
  auto usage = get_current_usage();
  return usage.available_bytes >= required_bytes;
}

size_t EnhancedMemoryMonitor::get_memory_limit() {
  // Check for cgroup memory limit (Docker/Kubernetes)
  std::ifstream limit_file("/sys/fs/cgroup/memory/memory.limit_in_bytes");
  if (limit_file.is_open()) {
    size_t limit;
    limit_file >> limit;
    // Check if it's a real limit (not the default huge value)
    if (limit < (1ULL << 62)) {  // Less than ~4 exabytes
      return limit;
    }
  }

  // Return system physical memory as limit
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  return static_cast<size_t>(pages) * static_cast<size_t>(page_size);
}

bool EnhancedMemoryMonitor::detect_memory_leak(size_t baseline_bytes, size_t current_bytes,
                                               double threshold_percent) {
  if (baseline_bytes == 0) {
    return false;  // Can't detect leak without baseline
  }

  double increase_percent =
      ((static_cast<double>(current_bytes) - static_cast<double>(baseline_bytes)) /
       static_cast<double>(baseline_bytes)) *
      100.0;

  return increase_percent > threshold_percent;
}

}  // namespace CI
}  // namespace TestUtils
