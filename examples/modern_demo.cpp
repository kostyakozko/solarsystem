/**
 * @file modern_demo.cpp
 * @brief Demonstration of modern C++ Solar System architecture
 *
 * This example shows how to use the new modern C++ classes:
 * - BodyFactory for creating celestial bodies
 * - BodyCollection for managing multiple bodies
 * - CelestialBody for individual body operations
 * - Vector3 for mathematical operations
 */

#include <chrono>
#include <iomanip>
#include <iostream>

// Modern C++ Solar System classes
#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/math/vector3.hpp"

using namespace SolarSystem;

void print_separator(const std::string& title) {
  std::cout << "\n" << std::string(60, '=') << "\n";
  std::cout << "  " << title << "\n";
  std::cout << std::string(60, '=') << "\n";
}

void demonstrate_vector3() {
  print_separator("Vector3 Mathematics Demo");

  // Create some vectors
  Math::Vector3d pos1{1.496e11, 0.0, 0.0};  // Earth's approximate position
  Math::Vector3d pos2{2.279e11, 0.0, 0.0};  // Mars' approximate position
  Math::Vector3d vel{29780.0, 0.0, 0.0};    // Earth's orbital velocity

  std::cout << "Earth position: " << Math::to_string(pos1) << " m\n";
  std::cout << "Mars position:  " << Math::to_string(pos2) << " m\n";
  std::cout << "Earth velocity: " << Math::to_string(vel) << " m/s\n";

  // Vector operations
  auto distance_vec = pos2 - pos1;
  double distance = static_cast<double>(distance_vec.magnitude());

  std::cout << "\nDistance vector: " << Math::to_string(distance_vec) << " m\n";
  std::cout << "Distance magnitude: " << std::scientific << distance << " m\n";
  std::cout << "Distance in AU: " << std::fixed << std::setprecision(2) << distance / 1.496e11
            << " AU\n";

  // Normalized vector
  auto direction = distance_vec.normalized();
  std::cout << "Direction (normalized): " << Math::to_string(direction) << "\n";
}

void demonstrate_body_factory() {
  print_separator("BodyFactory Demo");

  // Create a factory with fallback data source (no JPL calls)
  Bodies::BodyFactory::CreationOptions options{
      .preferred_source = Bodies::BodyFactory::DataSource::FALLBACK_DATA,
      .allow_fallback = false  // Only use fallback, don't try JPL
  };

  std::cout << "Creating celestial bodies using BodyFactory (fallback data)...\n\n";

  // Create individual bodies
  Bodies::BodyFactory factory(options);  // Pass options to constructor
  auto earth_result = factory.create_body("Earth");
  if (earth_result.has_value()) {
    const auto& earth = earth_result.value();
    std::cout << "✅ Created Earth:\n";
    std::cout << "   Name: " << earth.name() << "\n";
    std::cout << "   Mass: " << std::scientific << earth.mass() << " kg\n";
    std::cout << "   Type: " << Bodies::to_string(earth.type()) << "\n";
    std::cout << "   Priority: " << Bodies::to_string(earth.priority()) << "\n";
    std::cout << "   Position: " << Math::to_string(earth.position()) << " m\n";
  } else {
    std::cout << "❌ Failed to create Earth: " << earth_result.error() << "\n";
  }

  // Create a collection
  std::cout << "\nCreating inner planets collection...\n";
  auto collection_result = factory.create_inner_planets();
  if (collection_result.has_value()) {
    const auto& collection = collection_result.value();
    std::cout << "✅ Created collection with " << collection.size() << " bodies:\n";

    for (const auto& body : collection) {
      std::cout << "   - " << body.name() << " (" << Bodies::to_string(body.type()) << ")\n";
    }
  } else {
    std::cout << "❌ Failed to create collection: " << collection_result.error() << "\n";
  }
}

void demonstrate_body_collection() {
  print_separator("BodyCollection Demo");

  Bodies::BodyFactory::CreationOptions options{
      .preferred_source = Bodies::BodyFactory::DataSource::FALLBACK_DATA, .allow_fallback = false};
  Bodies::BodyFactory factory(options);

  // Create a solar system collection
  auto collection_result = factory.create_solar_system();
  if (!collection_result.has_value()) {
    std::cout << "❌ Failed to create solar system: " << collection_result.error() << "\n";
    return;
  }

  const auto& solar_system = collection_result.value();

  std::cout << "Solar System Collection Analysis:\n";
  std::cout << "Total bodies: " << solar_system.size() << "\n";
  std::cout << "Total mass: " << std::scientific << solar_system.total_mass() << " kg\n";

  // Filter by type
  auto planets = solar_system.filter_by_type(Bodies::BodyType::Planet);
  auto moons = solar_system.filter_by_type(Bodies::BodyType::Moon);
  auto stars = solar_system.filter_by_type(Bodies::BodyType::Star);

  std::cout << "\nBy Type:\n";
  std::cout << "  Stars: " << stars.size() << "\n";
  std::cout << "  Planets: " << planets.size() << "\n";
  std::cout << "  Moons: " << moons.size() << "\n";

  // Filter by priority
  auto essential = solar_system.filter_essential();
  auto important = solar_system.filter_by_priority(Bodies::BodyPriority::Important);
  auto optional = solar_system.filter_by_priority(Bodies::BodyPriority::Optional);

  std::cout << "\nBy Priority:\n";
  std::cout << "  Essential: " << essential.size() << "\n";
  std::cout << "  Important: " << important.size() << "\n";
  std::cout << "  Optional: " << optional.size() << "\n";

  // Center of mass calculation
  auto com = solar_system.center_of_mass();
  std::cout << "\nCenter of mass: " << Math::to_string(com) << " m\n";

  // Find specific body
  auto earth_ref = solar_system.find_body("Earth");
  if (earth_ref.has_value()) {
    const auto& earth = earth_ref->get();
    std::cout << "\nEarth details:\n";
    std::cout << "  Position: " << Math::to_string(earth.position()) << " m\n";
    std::cout << "  Velocity: " << Math::to_string(earth.velocity()) << " m/s\n";
    std::cout << "  Speed: " << std::fixed << std::setprecision(0) << earth.velocity().magnitude()
              << " m/s\n";
  }
}

void demonstrate_physics_calculations() {
  print_separator("Physics Calculations Demo");

  Bodies::BodyFactory::CreationOptions options{
      .preferred_source = Bodies::BodyFactory::DataSource::FALLBACK_DATA, .allow_fallback = false};
  Bodies::BodyFactory factory(options);

  // Create Earth and Moon
  auto earth_result = factory.create_body("Earth");
  auto moon_result = factory.create_body("Moon");

  if (!earth_result.has_value() || !moon_result.has_value()) {
    std::cout << "❌ Failed to create Earth or Moon\n";
    return;
  }

  auto earth = earth_result.value();
  auto moon = moon_result.value();

  std::cout << "Earth-Moon System Analysis:\n";

  // Calculate distance
  auto distance_vec = moon.position() - earth.position();
  double distance = static_cast<double>(distance_vec.magnitude());

  std::cout << "Distance: " << std::scientific << distance << " m\n";
  std::cout << "Distance: " << std::fixed << std::setprecision(0) << distance / 1000.0 << " km\n";

  // Calculate gravitational force
  auto force_vec = earth.gravitational_force_to(moon);
  double force_magnitude = static_cast<double>(force_vec.magnitude());

  std::cout << "Gravitational force: " << std::scientific << force_magnitude << " N\n";

  // Calculate orbital velocity (simplified circular orbit)
  double orbital_velocity = static_cast<double>(std::sqrt(6.67430e-11 * earth.mass() / distance));
  std::cout << "Required orbital velocity: " << std::fixed << std::setprecision(0)
            << orbital_velocity << " m/s\n";

  // Moon's actual velocity relative to Earth
  auto relative_velocity = moon.velocity() - earth.velocity();
  double actual_velocity = static_cast<double>(relative_velocity.magnitude());
  std::cout << "Moon's actual velocity: " << std::fixed << std::setprecision(0) << actual_velocity
            << " m/s\n";
}

int main() {
  std::cout << "🌌 Modern C++ Solar System Architecture Demo\n";
  std::cout << "=============================================\n";
  std::cout << "This demonstration shows the new modern C++ classes in action.\n";

  try {
    // Demonstrate each component
    demonstrate_vector3();
    demonstrate_body_factory();
    demonstrate_body_collection();
    demonstrate_physics_calculations();

    print_separator("Demo Complete");
    std::cout << "✅ All modern C++ components working successfully!\n";
    std::cout << "\nKey Features Demonstrated:\n";
    std::cout << "  • Vector3<T> mathematical operations\n";
    std::cout << "  • BodyFactory multi-source data creation\n";
    std::cout << "  • BodyCollection smart container operations\n";
    std::cout << "  • CelestialBody physics calculations\n";
    std::cout << "  • Type-safe enum classes and error handling\n";
    std::cout << "  • Modern C++ patterns (RAII, move semantics, Expected<T,E>)\n";

  } catch (const std::exception& e) {
    std::cerr << "❌ Error during demonstration: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
