#include <iostream>

#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/builders/simulation_builder.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Core::Builders;

int main() {
  std::cout << "🚀 Testing SimulationBuilder with Body Filtering\n\n";

  // Test 1: Create BodySelector and test priority filtering
  std::cout << "🔍 Testing priority filtering (Essential bodies):\n";
  BodySelector essential_selector;
  essential_selector.with_priority(BodyPriority::Essential);

  auto essential_result = essential_selector.build();
  if (essential_result.has_value()) {
    auto essential_bodies = essential_result.value();
    std::cout << "   • Essential bodies: " << essential_bodies.size() << "\n";
    for (const auto& body : essential_bodies) {
      std::cout << "     - " << body.name() << " (Priority: " << static_cast<int>(body.priority())
                << ")\n";
    }
  } else {
    std::cout << "   ❌ Failed to build essential bodies collection\n";
  }
  std::cout << "\n";

  // Test 2: Test type filtering
  std::cout << "🔍 Testing type filtering (Planets):\n";
  BodySelector planet_selector;
  planet_selector.of_type(BodyType::Planet);

  auto planet_result = planet_selector.build();
  if (planet_result.has_value()) {
    auto planets = planet_result.value();
    std::cout << "   • Planets: " << planets.size() << "\n";
    for (const auto& body : planets) {
      std::cout << "     - " << body.name() << " (Type: " << static_cast<int>(body.type()) << ")\n";
    }
  } else {
    std::cout << "   ❌ Failed to build planets collection\n";
  }
  std::cout << "\n";

  // Test 3: Test name filtering
  std::cout << "🔍 Testing name filtering (Earth, Moon, Sun):\n";
  BodySelector name_selector;
  std::vector<std::string> target_names = {"Earth", "Moon", "Sun"};
  name_selector.named(target_names);

  auto name_result = name_selector.build();
  if (name_result.has_value()) {
    auto selected_bodies = name_result.value();
    std::cout << "   • Selected bodies: " << selected_bodies.size() << "\n";
    for (const auto& body : selected_bodies) {
      std::cout << "     - " << body.name() << "\n";
    }
  } else {
    std::cout << "   ❌ Failed to build named bodies collection\n";
  }
  std::cout << "\n";

  // Test 4: Test fluent interface chaining
  std::cout << "🔍 Testing fluent interface (Essential + Planets):\n";
  BodySelector combined_selector;
  combined_selector.essential().of_type(BodyType::Planet);

  auto combined_result = combined_selector.build();
  if (combined_result.has_value()) {
    auto combined_bodies = combined_result.value();
    std::cout << "   • Essential planets: " << combined_bodies.size() << "\n";
    for (const auto& body : combined_bodies) {
      std::cout << "     - " << body.name() << " (Type: " << static_cast<int>(body.type())
                << ", Priority: " << static_cast<int>(body.priority()) << ")\n";
    }
  } else {
    std::cout << "   ❌ Failed to build combined collection\n";
  }
  std::cout << "\n";

  // Test 5: Test custom filter
  std::cout << "🔍 Testing custom filter (bodies with mass > 1e24 kg):\n";
  BodySelector mass_selector;
  mass_selector.where([](const CelestialBody& body) { return body.mass() > 1e24; });

  auto mass_result = mass_selector.build();
  if (mass_result.has_value()) {
    auto massive_bodies = mass_result.value();
    std::cout << "   • Massive bodies: " << massive_bodies.size() << "\n";
    for (const auto& body : massive_bodies) {
      std::cout << "     - " << body.name() << " (Mass: " << body.mass() << " kg)\n";
    }
  } else {
    std::cout << "   ❌ Failed to build massive bodies collection\n";
  }

  std::cout << "\n🎉 SimulationBuilder filtering tests completed!\n";
  return 0;
}
