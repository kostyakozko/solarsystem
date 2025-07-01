/**
 * @file simple_demo.cpp
 * @brief Simple demonstration of modern C++ Solar System core classes
 *
 * This example shows the core modern C++ classes without JPL integration:
 * - Vector3 for mathematical operations
 * - CelestialBody for individual body operations
 * - BodyCollection for managing multiple bodies
 * - BodyDefinition for creating bodies from structured data
 */

#include <iomanip>
#include <iostream>

// Modern C++ Solar System classes (core only)
#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/data/body_definitions.hpp"
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
  double distance = distance_vec.magnitude();

  std::cout << "\nDistance vector: " << Math::to_string(distance_vec) << " m\n";
  std::cout << "Distance magnitude: " << std::scientific << distance << " m\n";
  std::cout << "Distance in AU: " << std::fixed << std::setprecision(2) << distance / 1.496e11
            << " AU\n";

  // Normalized vector
  auto direction = distance_vec.normalized();
  std::cout << "Direction (normalized): " << Math::to_string(direction) << "\n";

  // Vector arithmetic
  auto midpoint = (pos1 + pos2) / 2.0;
  std::cout << "Midpoint: " << Math::to_string(midpoint) << " m\n";

  // Dot product
  auto dot = pos1.dot(pos2);
  std::cout << "Dot product: " << std::scientific << dot << "\n";
}

void demonstrate_celestial_body() {
  print_separator("CelestialBody Demo");

  // Create Earth using modern constructor
  Bodies::CelestialBody::Properties earth_props{.name = "Earth",
                                                .mass = 5.97219e24,
                                                .position = Math::Vector3d{1.496e11, 0.0, 0.0},
                                                .velocity = Math::Vector3d{0.0, 29780.0, 0.0},
                                                .type = Bodies::BodyType::Planet,
                                                .priority = Bodies::BodyPriority::Essential,
                                                .jpl_id = "399"};

  Bodies::CelestialBody earth{earth_props};

  std::cout << "Created Earth:\n";
  std::cout << "  Name: " << earth.name() << "\n";
  std::cout << "  Mass: " << std::scientific << earth.mass() << " kg\n";
  std::cout << "  Type: " << Bodies::to_string(earth.type()) << "\n";
  std::cout << "  Priority: " << Bodies::to_string(earth.priority()) << "\n";
  std::cout << "  Position: " << Math::to_string(earth.position()) << " m\n";
  std::cout << "  Velocity: " << Math::to_string(earth.velocity()) << " m/s\n";
  std::cout << "  Speed: " << std::fixed << std::setprecision(0) << earth.velocity().magnitude()
            << " m/s\n";

  // Create Moon
  Bodies::CelestialBody::Properties moon_props{
      .name = "Moon",
      .mass = 7.342e22,
      .position = Math::Vector3d{1.496e11 + 3.844e8, 0.0, 0.0},
      .velocity = Math::Vector3d{0.0, 29780.0 + 1022.0, 0.0},
      .type = Bodies::BodyType::Moon,
      .priority = Bodies::BodyPriority::Important,
      .jpl_id = "301"};

  Bodies::CelestialBody moon{moon_props};

  std::cout << "\nCreated Moon:\n";
  std::cout << "  Name: " << moon.name() << "\n";
  std::cout << "  Mass: " << std::scientific << moon.mass() << " kg\n";
  std::cout << "  Type: " << Bodies::to_string(moon.type()) << "\n";

  // Calculate distance
  auto distance_vec = moon.position() - earth.position();
  double distance = distance_vec.magnitude();
  std::cout << "\nEarth-Moon distance: " << std::fixed << std::setprecision(0) << distance / 1000.0
            << " km\n";

  // Calculate gravitational force
  auto force_vec = earth.gravitational_force_to(moon);
  double force_magnitude = force_vec.magnitude();
  std::cout << "Gravitational force: " << std::scientific << force_magnitude << " N\n";
}

void demonstrate_body_definitions() {
  print_separator("BodyDefinition Demo");

  std::cout << "Available fallback bodies:\n";

  // Show some bodies from our definitions
  auto earth_def = Data::get_fallback_body("Earth");
  if (earth_def.has_value()) {
    std::cout << "✅ Earth definition found:\n";
    std::cout << "   Mass: " << std::scientific << earth_def->mass << " kg\n";
    std::cout << "   Type: " << Bodies::to_string(earth_def->type) << "\n";
    std::cout << "   Priority: " << Bodies::to_string(earth_def->priority) << "\n";

    // Convert to CelestialBody
    auto earth = earth_def->to_celestial_body();
    std::cout << "   Converted to CelestialBody: " << earth.name() << "\n";
  }

  // Show essential bodies
  auto essential = Data::get_essential_bodies();
  std::cout << "\nEssential bodies (" << essential.size() << "):\n";
  for (const auto& body_def : essential) {
    std::cout << "  - " << body_def.name << " (" << Bodies::to_string(body_def.type) << ")\n";
  }

  // Show by type
  auto planets = Data::get_bodies_by_type(Bodies::BodyType::Planet);
  std::cout << "\nPlanets (" << planets.size() << "):\n";
  for (const auto& body_def : planets) {
    std::cout << "  - " << body_def.name << "\n";
  }
}

void demonstrate_body_collection() {
  print_separator("BodyCollection Demo");

  // Create a collection and add bodies manually
  Bodies::BodyCollection collection;

  // Add Sun
  collection.add_body(Bodies::CelestialBody::Properties{.name = "Sun",
                                                        .mass = 1.98847e30,
                                                        .position = Math::Vector3d{0.0, 0.0, 0.0},
                                                        .velocity = Math::Vector3d{0.0, 0.0, 0.0},
                                                        .type = Bodies::BodyType::Star,
                                                        .priority = Bodies::BodyPriority::Essential,
                                                        .jpl_id = "10"});

  // Add Earth
  collection.add_body(
      Bodies::CelestialBody::Properties{.name = "Earth",
                                        .mass = 5.97219e24,
                                        .position = Math::Vector3d{1.496e11, 0.0, 0.0},
                                        .velocity = Math::Vector3d{0.0, 29780.0, 0.0},
                                        .type = Bodies::BodyType::Planet,
                                        .priority = Bodies::BodyPriority::Essential,
                                        .jpl_id = "399"});

  // Add Mars
  collection.add_body(
      Bodies::CelestialBody::Properties{.name = "Mars",
                                        .mass = 6.41693e23,
                                        .position = Math::Vector3d{2.279e11, 0.0, 0.0},
                                        .velocity = Math::Vector3d{0.0, 24077.0, 0.0},
                                        .type = Bodies::BodyType::Planet,
                                        .priority = Bodies::BodyPriority::Essential,
                                        .jpl_id = "499"});

  std::cout << "Created collection with " << collection.size() << " bodies\n";
  std::cout << "Total mass: " << std::scientific << collection.total_mass() << " kg\n";

  // Filter operations
  auto stars = collection.filter_by_type(Bodies::BodyType::Star);
  auto planets = collection.filter_by_type(Bodies::BodyType::Planet);
  auto essential = collection.filter_essential();

  std::cout << "\nFiltering results:\n";
  std::cout << "  Stars: " << stars.size() << "\n";
  std::cout << "  Planets: " << planets.size() << "\n";
  std::cout << "  Essential: " << essential.size() << "\n";

  // Find specific body
  auto earth_ref = collection.find_body("Earth");
  if (earth_ref.has_value()) {
    const auto& earth = earth_ref->get();
    std::cout << "\nFound Earth:\n";
    std::cout << "  Position: " << Math::to_string(earth.position()) << " m\n";
    std::cout << "  Speed: " << std::fixed << std::setprecision(0) << earth.velocity().magnitude()
              << " m/s\n";
  }

  // Center of mass
  auto com = collection.center_of_mass();
  std::cout << "\nCenter of mass: " << Math::to_string(com) << " m\n";

  // Iterate through all bodies
  std::cout << "\nAll bodies:\n";
  for (const auto& body : collection) {
    std::cout << "  - " << body.name() << " (" << Bodies::to_string(body.type()) << ")\n";
  }
}

void demonstrate_bulk_operations() {
  print_separator("Bulk Operations Demo");

  // Create collection from fallback definitions
  Bodies::BodyCollection collection;

  // Add several bodies from definitions
  std::vector<std::string> body_names = {"Sun", "Mercury", "Venus", "Earth", "Mars"};

  for (const auto& name : body_names) {
    auto body_def = Data::get_fallback_body(name);
    if (body_def.has_value()) {
      collection.add_body(body_def->to_celestial_body());
    }
  }

  std::cout << "Created inner solar system with " << collection.size() << " bodies\n";

  // Apply bulk operation - move all bodies by offset
  Math::Vector3d offset{1.0e9, 0.0, 0.0};  // 1 million km offset
  std::cout << "\nApplying offset of " << Math::to_string(offset) << " m to all bodies...\n";

  collection.apply_to_all(
      [&offset](Bodies::CelestialBody& body) { body.set_position(body.position() + offset); });

  // Show new positions
  std::cout << "\nNew positions:\n";
  for (const auto& body : collection) {
    std::cout << "  " << body.name() << ": " << Math::to_string(body.position()) << " m\n";
  }

  // Apply filtered operation - only to planets
  std::cout << "\nDoubling velocity of planets only...\n";
  collection.apply_to_filtered(
      [](const Bodies::CelestialBody& body) { return body.type() == Bodies::BodyType::Planet; },
      [](Bodies::CelestialBody& body) { body.set_velocity(body.velocity() * 2.0); });

  // Show planet velocities
  auto planets = collection.filter_by_type(Bodies::BodyType::Planet);
  std::cout << "\nPlanet velocities after doubling:\n";
  for (const auto& planet_ref : planets) {
    const auto& planet = planet_ref.get();
    std::cout << "  " << planet.name() << ": " << std::fixed << std::setprecision(0)
              << planet.velocity().magnitude() << " m/s\n";
  }
}

int main() {
  std::cout << "🌌 Modern C++ Solar System Core Classes Demo\n";
  std::cout << "=============================================\n";
  std::cout << "This demonstration shows the core modern C++ classes in action.\n";

  try {
    // Demonstrate each component
    demonstrate_vector3();
    demonstrate_celestial_body();
    demonstrate_body_definitions();
    demonstrate_body_collection();
    demonstrate_bulk_operations();

    print_separator("Demo Complete");
    std::cout << "✅ All modern C++ core components working successfully!\n";
    std::cout << "\nKey Features Demonstrated:\n";
    std::cout << "  • Vector3<T> mathematical operations and utilities\n";
    std::cout << "  • CelestialBody RAII design with physics calculations\n";
    std::cout << "  • BodyDefinition structured data with type safety\n";
    std::cout << "  • BodyCollection smart container with filtering\n";
    std::cout << "  • Bulk operations and functional programming patterns\n";
    std::cout << "  • Modern C++ features: RAII, move semantics, strong typing\n";
    std::cout << "  • Clean separation of concerns and modular design\n";

  } catch (const std::exception& e) {
    std::cerr << "❌ Error during demonstration: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
