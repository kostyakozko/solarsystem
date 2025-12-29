/**
 * @file test_api_management.cpp
 * @brief Unit tests for API management system (Task 15)
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/api/api_manager.hpp"

using namespace SolarSystem::API;
TEST(APIManagementTests, API_Manager_Initialization) {
    APIManager manager;
    // Manager should initialize successfully
    auto endpoints = manager.get_endpoints();
    // Initially no endpoints
}

TEST(APIManagementTests, API_Version_Parsing) {
    auto version = APIVersion::parse("1.2.3");
    if (!version.has_value()) throw std::runtime_error("Should parse valid version");
    if (version->major != 1) throw std::runtime_error("Major version wrong");
    if (version->minor != 2) throw std::runtime_error("Minor version wrong");
    if (version->patch != 3) throw std::runtime_error("Patch version wrong");
}

TEST(APIManagementTests, API_Version_String_Conversion) {
    APIVersion version;
    version.major = 2;
    version.minor = 1;
    version.patch = 0;

    auto str = version.to_string();
    if (str.empty()) throw std::runtime_error("Version string should not be empty");
}

TEST(APIManagementTests, API_Documentation_Generation) {
    APIManager manager;

    auto docs = manager.get_documentation("json");
    if (docs.empty()) throw std::runtime_error("Documentation should not be empty");
}

TEST(APIManagementTests, API_Statistics) {
    APIManager manager;

    auto stats = manager.get_statistics();
    // Statistics should be initialized even if no requests yet
    if (stats.total_requests != 0) throw std::runtime_error("Initial requests should be 0");
}

TEST(APIManagementTests, API_Statistics_Initialization) {
    APIStatistics stats;
    if (stats.total_requests != 0) throw std::runtime_error("Initial requests should be 0");
    if (stats.successful_requests != 0) throw std::runtime_error("Initial success should be 0");
}

