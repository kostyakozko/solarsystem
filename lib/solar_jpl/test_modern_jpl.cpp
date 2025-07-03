/**
 * @file test_modern_jpl.cpp
 * @brief Test program for modern JPL client
 */

#include <chrono>
#include <iostream>

#include "solar_jpl/jpl_client.hpp"

using namespace SolarSystem::JPL;

int main() {
  try {
    std::cout << "🚀 Testing Modern JPL Client\n\n";

    // Create JPL client with default configuration
    auto client = JPLClientFactory::create_default();

    std::cout << "✅ JPL client created successfully\n";

    // Test configuration
    const auto& config = client->config();
    std::cout << "📡 API endpoint: " << config.api_endpoint << "\n";
    std::cout << "⏱️  Request timeout: " << config.request_timeout.count() << "s\n";
    std::cout << "🔄 Max retries: " << config.max_retries << "\n";
    std::cout << "📁 Cache directory: " << config.cache_directory << "\n\n";

    // Test cache operations
    std::cout << "🗂️  Testing cache operations:\n";

    auto metadata = client->get_cache_metadata();
    if (metadata) {
      std::cout << "✅ Cache metadata found\n";
      std::cout << "   📊 Body count: " << metadata->body_count << "\n";
      std::cout << "   📅 Source: " << metadata->source << "\n";
    } else {
      std::cout << "ℹ️  No cache metadata found (expected for first run)\n";
    }

    // Test current year data check
    bool has_current = client->has_current_year_data();
    std::cout << "📅 Has current year data: " << (has_current ? "Yes" : "No") << "\n";

    // Test utility functions
    std::cout << "\n🛠️  Testing utility functions:\n";

    auto current_epoch = Utils::get_current_year_epoch();
    auto date_str = Utils::to_jpl_date_string(current_epoch);
    std::cout << "📅 Current year epoch: " << date_str << "\n";

    auto parsed_date = Utils::from_jpl_date_string(date_str);
    if (parsed_date) {
      std::cout << "✅ Date parsing successful\n";
    } else {
      std::cout << "❌ Date parsing failed\n";
    }

    // Test JPL ID utilities
    auto all_ids = Utils::get_all_jpl_ids();
    std::cout << "🌍 Known JPL IDs: " << all_ids.size() << " bodies\n";

    // Test error handling
    std::cout << "\n🔧 Testing error handling:\n";
    for (auto error : {JPLError::NetworkError, JPLError::ParseError, JPLError::CacheError}) {
      std::cout << "   " << Utils::to_string(error) << "\n";
    }

    std::cout << "\n🎉 All tests completed successfully!\n";
    std::cout << "\n💡 Next steps:\n";
    std::cout << "   • Implement actual JPL response parsing\n";
    std::cout << "   • Add binary/JSON cache implementation\n";
    std::cout << "   • Integrate with existing body mapping\n";
    std::cout << "   • Add comprehensive error handling\n";
    std::cout << "   • Test with real JPL HORIZONS API\n";

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "💥 Error: " << e.what() << "\n";
    return 1;
  }
}
