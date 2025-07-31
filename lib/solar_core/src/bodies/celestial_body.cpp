#include "solar_core/bodies/celestial_body.hpp"

#include <iomanip>
#include <sstream>

#include "solar_core/math/constants.hpp"

namespace SolarSystem::Bodies {

CelestialBody::CelestialBody(Properties props)
    : name_(std::move(props.name)),
      mass_(props.mass),
      position_(props.position),
      velocity_(props.velocity),
      acceleration_{},
      type_(props.type),
      priority_(props.priority),
      jpl_id_(std::move(props.jpl_id)),
      creation_date_(props.creation_date) {
  if (mass_ < 0.0) {
    throw std::invalid_argument("Mass cannot be negative");
  }
  if (name_.empty()) {
    throw std::invalid_argument("Name cannot be empty");
  }
}

void CelestialBody::apply_force(const Math::Vector3d& force, double) noexcept {
  if (mass_ > 0.0) {
    // F = ma, so a = F/m
    acceleration_ += force / mass_;
  }
}

void CelestialBody::update_position(double dt) noexcept {
  // Verlet integration: v = v + a*dt, x = x + v*dt
  velocity_ += acceleration_ * dt;
  position_ += velocity_ * dt;

  // Reset acceleration for next iteration
  acceleration_ = Math::Vector3d{};
}

Math::Vector3d CelestialBody::gravitational_force_to(const CelestialBody& other) const noexcept {
  const auto displacement = other.position_ - position_;
  const auto distance_sq = displacement.magnitude_squared();

  // Avoid division by zero and singularities
  if (distance_sq < 1e-10) {
    return Math::Vector3d{};
  }

  const auto force_magnitude = Math::Constants::G * mass_ * other.mass_ / distance_sq;

  // Force direction is along the displacement vector
  return displacement.normalized() * force_magnitude;
}

long double CelestialBody::distance_to(const CelestialBody& other) const noexcept {
  return Math::distance(position_, other.position_);
}

long double CelestialBody::distance_squared_to(const CelestialBody& other) const noexcept {
  return Math::distance_squared(position_, other.position_);
}

bool CelestialBody::is_available_at(std::chrono::system_clock::time_point time) const noexcept {
  // If no creation date is specified, assume always available
  if (!creation_date_.has_value()) {
    return true;
  }

  // Body is available if the query time is after its creation date
  return time >= creation_date_.value();
}

std::string CelestialBody::to_string() const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3);
  oss << name_ << " (" << Bodies::to_string(type_) << ")\n";
  oss << "  Mass: " << std::scientific << mass_ << " kg\n";
  oss << "  Position: " << Math::to_string(position_) << " m\n";
  oss << "  Velocity: " << Math::to_string(velocity_) << " m/s\n";
  oss << "  Priority: " << Bodies::to_string(priority_);

  if (jpl_id_.has_value()) {
    oss << "\n  JPL ID: " << jpl_id_.value();
  }

  return oss.str();
}

// Utility functions for enum to string conversion
std::string_view to_string(BodyType type) noexcept {
  switch (type) {
    case BodyType::Star:
      return "Star";
    case BodyType::Planet:
      return "Planet";
    case BodyType::Moon:
      return "Moon";
    case BodyType::DwarfPlanet:
      return "Dwarf Planet";
    case BodyType::Asteroid:
      return "Asteroid";
    case BodyType::Spacecraft:
      return "Spacecraft";
    default:
      return "Unknown";
  }
}

std::string_view to_string(BodyPriority priority) noexcept {
  switch (priority) {
    case BodyPriority::Essential:
      return "Essential";
    case BodyPriority::Important:
      return "Important";
    case BodyPriority::Optional:
      return "Optional";
    default:
      return "Unknown";
  }
}

}  // namespace SolarSystem::Bodies
