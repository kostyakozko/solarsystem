#include <iostream>

#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem::Bodies;

int main() {
  std::cout << "🚀 Testing BodyFactory with JPL Integration\n\n";

  // Test 1: Create BodyFactory with JPL source
  std::cout << "✅ Creating BodyFactory with JPL HORIZONS source...\n";
  BodyFactory::CreationOptions options;
  options.preferred_source = BodyFactory::DataSource::JPL_HORIZONS;
  options.allow_fallback = true;

  BodyFactory factory(options);

  std::cout << "📊 Factory initialized: " << (factory.is_initialized() ? "Yes" : "No") << "\n";
  std::cout << "📅 Current source: " << factory.current_source() << "\n";
  std::cout << "🗓️  Has current year data: "
            << (factory.has_current_year_ephemeris_data() ? "Yes" : "No") << "\n\n";

  // Test 2: Get available bodies
  std::cout << "🌍 Available bodies:\n";
  auto available_bodies = factory.get_available_bodies();
  std::cout << "📊 Total bodies: " << available_bodies.size() << "\n";
  for (size_t i = 0; i < std::min(available_bodies.size(), size_t(5)); ++i) {
    std::cout << "   • " << available_bodies[i] << "\n";
  }
  if (available_bodies.size() > 5) {
    std::cout << "   ... and " << (available_bodies.size() - 5) << " more\n";
  }
  std::cout << "\n";

  // Test 3: Create a body using fallback data
  std::cout << "🌍 Testing body creation (fallback data):\n";
  auto earth_result = factory.create_body("Earth");
  if (earth_result.has_value()) {
    auto earth = earth_result.value();
    std::cout << "✅ Created Earth successfully\n";
    std::cout << "   • Name: " << earth.name() << "\n";
    std::cout << "   • Mass: " << earth.mass() << " kg\n";
    std::cout << "   • Type: " << static_cast<int>(earth.type()) << "\n";
    std::cout << "   • Priority: " << static_cast<int>(earth.priority()) << "\n";
    std::cout << "   • Position: [" << earth.position().x() << ", " << earth.position().y() << ", "
              << earth.position().z() << "]\n";
  } else {
    std::cout << "❌ Failed to create Earth: " << earth_result.error() << "\n";
  }
  std::cout << "\n";

  // Test 4: Test storage system
  std::cout << "🧪 Testing storage system:\n";
  auto storage_result = factory.test_storage_system();
  if (storage_result.has_value()) {
    std::cout << "✅ Storage system test passed\n";
  } else {
    std::cout << "❌ Storage system test failed: " << storage_result.error() << "\n";
  }
  std::cout << "\n";

  // Test 5: Create solar system collection
  std::cout << "🌌 Testing solar system collection creation:\n";
  auto solar_system_result = factory.create_solar_system();
  if (solar_system_result.has_value()) {
    auto solar_system = solar_system_result.value();
    std::cout << "✅ Created solar system collection\n";
    std::cout << "   • Total bodies: " << solar_system.size() << "\n";

    // Show first few bodies
    size_t count = 0;
    for (const auto& body : solar_system) {
      if (count >= 3) break;
      std::cout << "   • " << body.name() << " (Type: " << static_cast<int>(body.type())
                << ", Priority: " << static_cast<int>(body.priority()) << ")\n";
      count++;
    }
  } else {
    std::cout << "❌ Failed to create solar system: " << solar_system_result.error() << "\n";
  }

  std::cout << "\n🎉 BodyFactory testing completed!\n";
  return 0;
}
