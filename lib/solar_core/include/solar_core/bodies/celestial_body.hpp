#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "solar_core/math/vector3.hpp"

namespace SolarSystem::Bodies {

/**
 * @brief Classification of celestial body types
 */
enum class BodyType { Star, Planet, Moon, DwarfPlanet, Asteroid, Spacecraft };

/**
 * @brief Priority classification for smart body handling
 */
enum class BodyPriority {
  Essential,  // Planets - always included in simulations
  Important,  // Major moons - included by default
  Optional    // Spacecraft - historical date aware
};

/**
 * @brief Modern celestial body class with RAII and strong typing
 */
class CelestialBody {
 public:
  /**
   * @brief Properties structure for celestial body construction
   */
  struct Properties {
    std::string name;
    long double mass;         // kg - CHANGED TO LONG DOUBLE FOR LEGACY COMPATIBILITY
    Math::Vector3d position;  // meters
    Math::Vector3d velocity;  // m/s
    BodyType type;
    BodyPriority priority;
    std::optional<std::string> jpl_id;
    std::optional<std::chrono::system_clock::time_point> creation_date;
  };

  /**
   * @brief Construct celestial body with properties
   * @param props Body properties
   */
  explicit CelestialBody(Properties props);

  // Copy and move constructors
  CelestialBody(const CelestialBody&) = default;
  CelestialBody(CelestialBody&&) = default;
  CelestialBody& operator=(const CelestialBody&) = default;
  CelestialBody& operator=(CelestialBody&&) = default;

  // Destructor
  ~CelestialBody() = default;

  // Accessors
  [[nodiscard]] std::string_view name() const noexcept { return name_; }
  [[nodiscard]] long double mass() const noexcept { return mass_; }  // CHANGED TO LONG DOUBLE
  [[nodiscard]] const Math::Vector3d& position() const noexcept { return position_; }
  [[nodiscard]] const Math::Vector3d& velocity() const noexcept { return velocity_; }
  [[nodiscard]] const Math::Vector3d& acceleration() const noexcept { return acceleration_; }
  [[nodiscard]] BodyType type() const noexcept { return type_; }
  [[nodiscard]] BodyPriority priority() const noexcept { return priority_; }
  [[nodiscard]] const std::optional<std::string>& jpl_id() const noexcept { return jpl_id_; }
  [[nodiscard]] const std::optional<std::chrono::system_clock::time_point>& creation_date()
      const noexcept {
    return creation_date_;
  }

  // State modification
  void set_position(const Math::Vector3d& position) noexcept { position_ = position; }
  void set_velocity(const Math::Vector3d& velocity) noexcept { velocity_ = velocity; }
  void set_state(const Math::Vector3d& position, const Math::Vector3d& velocity) noexcept {
    position_ = position;
    velocity_ = velocity;
  }

  // Physics operations
  void apply_force(const Math::Vector3d& force, double dt) noexcept;
  void update_position(double dt) noexcept;
  void reset_acceleration() noexcept { acceleration_ = Math::Vector3d{}; }

  // Gravitational interactions
  [[nodiscard]] Math::Vector3d gravitational_force_to(const CelestialBody& other) const noexcept;
  [[nodiscard]] double distance_to(const CelestialBody& other) const noexcept;
  [[nodiscard]] double distance_squared_to(const CelestialBody& other) const noexcept;

  // Utility functions
  [[nodiscard]] bool is_available_at(std::chrono::system_clock::time_point time) const noexcept;
  [[nodiscard]] std::string to_string() const;

  // Comparison operators
  bool operator==(const CelestialBody& other) const noexcept { return name_ == other.name_; }

  bool operator!=(const CelestialBody& other) const noexcept { return !(*this == other); }

 private:
  std::string name_;
  long double mass_;             // kg - CHANGED TO LONG DOUBLE FOR LEGACY COMPATIBILITY
  Math::Vector3d position_;      // meters
  Math::Vector3d velocity_;      // m/s
  Math::Vector3d acceleration_;  // m/s²
  BodyType type_;
  BodyPriority priority_;
  std::optional<std::string> jpl_id_;
  std::optional<std::chrono::system_clock::time_point> creation_date_;
};

/**
 * @brief Convert BodyType to string representation
 */
[[nodiscard]] std::string_view to_string(BodyType type) noexcept;

/**
 * @brief Convert BodyPriority to string representation
 */
[[nodiscard]] std::string_view to_string(BodyPriority priority) noexcept;

}  // namespace SolarSystem::Bodies
