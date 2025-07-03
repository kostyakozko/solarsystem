#include <iostream>

#include "solar_jpl/jpl_client.hpp"

int main() {
  std::cout << "🚀 Testing Modern JPL Client (Simple)\n\n";

  try {
    using namespace SolarSystem::JPL;

    // Test configuration
    JPLClientConfig config;
    std::cout << "✅ Config created: " << config.api_endpoint << "\n";
    std::cout << "   Timeout: " << config.request_timeout.count() << "s\n";
    std::cout << "   Max retries: " << config.max_retries << "\n";
    std::cout << "   Cache dir: " << config.cache_directory << "\n";

    // Test configuration validation
    std::string error;
    bool valid = config.is_valid(&error);
    std::cout << "   Valid: " << (valid ? "Yes" : "No");
    if (!valid) std::cout << " (" << error << ")";
    std::cout << "\n\n";

    // Test factory
    auto client = JPLClientFactory::create_default();
    std::cout << "✅ Client created successfully\n";

    // Test cache operations
    bool has_current = client->has_current_year_data();
    std::cout << "   Has current year data: " << (has_current ? "Yes" : "No") << "\n";

    auto metadata = client->get_cache_metadata();
    if (metadata) {
      std::cout << "   Cache metadata found: " << metadata->body_count << " bodies\n";
    } else {
      std::cout << "   No cache metadata (expected for first run)\n";
    }

    // Test utilities
    std::cout << "\n🛠️  Testing utilities:\n";

    auto current_epoch = Utils::get_current_year_epoch();
    auto date_str = Utils::to_jpl_date_string(current_epoch);
    std::cout << "   Current year epoch: " << date_str << "\n";

    auto parsed_date = Utils::from_jpl_date_string(date_str);
    std::cout << "   Date parsing: " << (parsed_date ? "Success" : "Failed") << "\n";

    auto all_ids = Utils::get_all_jpl_ids();
    std::cout << "   Known JPL IDs: " << all_ids.size() << " bodies\n";

    // Test error handling
    std::cout << "\n🔧 Error types:\n";
    for (auto error : {JPLError::NetworkError, JPLError::ParseError, JPLError::CacheError}) {
      std::cout << "   " << Utils::to_string(error) << "\n";
    }

    std::cout << "\n🎉 Modern JPL Client test completed successfully!\n";
    std::cout << "\n💡 Ready for integration with modern simulation system!\n";

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "💥 Error: " << e.what() << "\n";
    return 1;
  }
}
