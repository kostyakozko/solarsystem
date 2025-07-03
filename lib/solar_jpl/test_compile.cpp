#include <iostream>

#include "solar_jpl/jpl_client.hpp"

int main() {
  std::cout << "Testing modern JPL client compilation...\n";

  try {
    // Test basic compilation
    using namespace SolarSystem::JPL;

    // Test configuration
    JPLClientConfig config;
    std::cout << "Config created: " << config.api_endpoint << "\n";

    // Test factory
    auto client = JPLClientFactory::create_default();
    std::cout << "Client created successfully\n";

    // Test utilities
    auto current_epoch = Utils::get_current_year_epoch();
    auto date_str = Utils::to_jpl_date_string(current_epoch);
    std::cout << "Current epoch: " << date_str << "\n";

    std::cout << "✅ Compilation test successful!\n";
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "❌ Error: " << e.what() << "\n";
    return 1;
  }
}
