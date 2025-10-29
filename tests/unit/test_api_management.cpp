/**
 * @file test_api_management.cpp
 * @brief Unit tests for API management system (Task 15)
 */

#include "../utils/test_framework.h"

#include "solar_core/api/api_manager.hpp"

using namespace SolarSystem::API;

int main() {
  TestSuite suite("API Management Tests");

  suite.run_test("API Manager Initialization", []() {
    APIManager manager;
    // Manager should initialize successfully
    auto endpoints = manager.get_endpoints();
    // Initially no endpoints
  });

  suite.run_test("API Version Parsing", []() {
    auto version = APIVersion::parse("1.2.3");
    if (!version.has_value()) throw std::runtime_error("Should parse valid version");
    if (version->major != 1) throw std::runtime_error("Major version wrong");
    if (version->minor != 2) throw std::runtime_error("Minor version wrong");
    if (version->patch != 3) throw std::runtime_error("Patch version wrong");
  });

  suite.run_test("API Version String Conversion", []() {
    APIVersion version;
    version.major = 2;
    version.minor = 1;
    version.patch = 0;

    auto str = version.to_string();
    if (str.empty()) throw std::runtime_error("Version string should not be empty");
  });

  suite.run_test("API Documentation Generation", []() {
    APIManager manager;

    auto docs = manager.get_documentation("json");
    if (docs.empty()) throw std::runtime_error("Documentation should not be empty");
  });

  suite.run_test("API Statistics", []() {
    APIManager manager;

    auto stats = manager.get_statistics();
    // Statistics should be initialized even if no requests yet
    if (stats.total_requests != 0) throw std::runtime_error("Initial requests should be 0");
  });

  suite.run_test("API Statistics Initialization", []() {
    APIStatistics stats;
    if (stats.total_requests != 0) throw std::runtime_error("Initial requests should be 0");
    if (stats.successful_requests != 0) throw std::runtime_error("Initial success should be 0");
  });

  suite.print_summary();
  return suite.all_passed() ? 0 : 1;
}
