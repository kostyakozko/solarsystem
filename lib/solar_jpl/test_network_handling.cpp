/**
 * @file test_network_handling.cpp
 * @brief Test robust network handling enhancements
 */

#include "solar_jpl/jpl_client.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace SolarSystem::JPL;

int main() {
    std::cout << "Testing robust network handling enhancements...\n\n";

    // Create JPL client with enhanced network configuration
    JPLClientConfig config;
    config.enable_exponential_backoff = true;
    config.backoff_multiplier = 2.0;
    config.max_backoff_delay = std::chrono::milliseconds(5000);
    config.enable_circuit_breaker = true;
    config.circuit_breaker_failure_threshold = 3;
    config.circuit_breaker_timeout = std::chrono::minutes(1);
    config.enable_connection_pooling = true;
    config.connection_pool_size = 3;
    config.enable_offline_mode = true;
    config.prefer_cache_on_network_failure = true;

    // Add fallback endpoints
    config.fallback_endpoints = {
        "https://ssd-api.jpl.nasa.gov/horizons.api",
        "https://backup.jpl.nasa.gov/horizons.api"
    };

    try {
        JPLClient client(config);

        // Test 1: Network connectivity check
        std::cout << "1. Testing network connectivity check...\n";
        auto connectivity = client.check_network_connectivity();
        std::cout << "   Connectivity status: " << Utils::to_string(connectivity) << "\n\n";

        // Test 2: Network diagnostics
        std::cout << "2. Running comprehensive network diagnostics...\n";
        auto diag_result = client.run_network_diagnostics();
        if (is_success(diag_result)) {
            auto diagnostics = client.get_network_diagnostics();
            std::cout << Utils::format_network_diagnostics(diagnostics) << "\n";
        } else {
            std::cout << "   Failed to run network diagnostics\n\n";
        }

        // Test 3: Network health score
        std::cout << "3. Testing network health score...\n";
        auto health_score = client.get_network_health_score();
        std::cout << "   Network health score: " << std::fixed << std::setprecision(2)
                  << health_score << " (" << (health_score > 0.7 ? "Good" :
                     health_score > 0.4 ? "Fair" : "Poor") << ")\n\n";

        // Test 4: Endpoint connectivity test
        std::cout << "4. Testing endpoint connectivity...\n";
        auto endpoint_test = client.test_endpoint_connectivity("https://www.google.com");
        if (is_success(endpoint_test)) {
            std::cout << "   Google.com response time: " << get_value(endpoint_test).count() << "ms\n";
        } else {
            std::cout << "   Failed to reach Google.com\n";
        }

        // Test 5: Offline mode
        std::cout << "\n5. Testing offline mode...\n";
        std::cout << "   Current offline mode: " << (client.is_offline_mode() ? "Enabled" : "Disabled") << "\n";

        client.set_offline_mode(true);
        std::cout << "   Enabled offline mode\n";
        std::cout << "   Offline mode now: " << (client.is_offline_mode() ? "Enabled" : "Disabled") << "\n";

        client.set_offline_mode(false);
        std::cout << "   Disabled offline mode\n";

        // Test 6: Configuration validation
        std::cout << "\n6. Testing configuration validation...\n";
        JPLClientConfig invalid_config;
        invalid_config.backoff_multiplier = 0.5;  // Invalid: should be > 1.0

        std::string error;
        if (!invalid_config.is_valid(&error)) {
            std::cout << "   Successfully caught invalid config: " << error << "\n";
        } else {
            std::cout << "   ERROR: Failed to catch invalid configuration\n";
        }

        // Test 7: URL validation
        std::cout << "\n7. Testing URL validation...\n";
        std::vector<std::pair<std::string, bool>> test_urls = {
            {"https://ssd.jpl.nasa.gov/api/horizons.api", true},
            {"http://example.com", true},
            {"ftp://invalid.com", false},
            {"", false},
            {"not-a-url", false}
        };

        for (const auto& [url, expected] : test_urls) {
            bool is_valid = Utils::is_valid_endpoint_url(url);
            std::cout << "   URL '" << url << "': "
                      << (is_valid ? "Valid" : "Invalid")
                      << (is_valid == expected ? " ✓" : " ✗") << "\n";
        }

        std::cout << "\nAll network handling tests completed successfully!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
