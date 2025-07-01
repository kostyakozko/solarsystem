/**
 * @file solar_system_fixed.cpp
 * @brief Fixed modern C++ version that uses legacy data directly
 *
 * This version bypasses the BodyFactory and uses the legacy constants directly
 * to ensure identical physics and barycenter calculations.
 */

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

#include "args.h"
#include "constants.h"
#include "jpl_data.h"
#include "model.h"
#include "simulation.h"  // For perform_simulation_step

using namespace std;

/**
 * @brief Simple Vector3d class for modernization (avoiding namespace conflicts)
 */
class Vector3d {
 public:
  double x_, y_, z_;

  Vector3d(double x, double y, double z) : x_(x), y_(y), z_(z) {}

  double x() const { return x_; }
  double y() const { return y_; }
  double z() const { return z_; }

  double magnitude() const { return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_); }
};

/**
 * @brief Simple Body class for modernization (avoiding namespace conflicts)
 */
class Body {
 public:
  std::string name_;
  double mass_;
  Vector3d position_;
  Vector3d velocity_;

  Body(const std::string& name, double mass, const Vector3d& position, const Vector3d& velocity)
      : name_(name), mass_(mass), position_(position), velocity_(velocity) {}

  const std::string& name() const { return name_; }
  double mass() const { return mass_; }
  const Vector3d& position() const { return position_; }
  const Vector3d& velocity() const { return velocity_; }

  void set_position(const Vector3d& pos) { position_ = pos; }
  void set_velocity(const Vector3d& vel) { velocity_ = vel; }
};

/**
 * @brief Simple BodyCollection class for modernization
 */
class BodyCollection {
 private:
  std::vector<Body> bodies_;

 public:
  void add_body(const Body& body) { bodies_.push_back(body); }

  size_t size() const { return bodies_.size(); }

  Body& operator[](size_t index) { return bodies_[index]; }
  const Body& operator[](size_t index) const { return bodies_[index]; }

  auto begin() { return bodies_.begin(); }
  auto end() { return bodies_.end(); }
  auto begin() const { return bodies_.begin(); }
  auto end() const { return bodies_.end(); }

  Vector3d center_of_mass() const {
    if (bodies_.empty()) {
      return Vector3d{0.0, 0.0, 0.0};
    }

    double total_mass = 0.0;
    Vector3d weighted_position{0.0, 0.0, 0.0};

    for (const auto& body : bodies_) {
      total_mass += body.mass();
      weighted_position.x_ += body.mass() * body.position().x();
      weighted_position.y_ += body.mass() * body.position().y();
      weighted_position.z_ += body.mass() * body.position().z();
    }

    if (total_mass > 0.0) {
      weighted_position.x_ /= total_mass;
      weighted_position.y_ /= total_mass;
      weighted_position.z_ /= total_mass;
    }

    return weighted_position;
  }
};

/**
 * @brief Convert legacy coord to modern Vector3d
 */
Vector3d coord_to_vector3d(const coord& c) {
  return Vector3d{static_cast<double>(c.x), static_cast<double>(c.y), static_cast<double>(c.z)};
}

/**
 * @brief Convert legacy velocity to modern Vector3d
 */
Vector3d velocity_to_vector3d(const velocity& v) {
  return Vector3d{static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z)};
}

/**
 * @brief Calculate distance between two Vector3d points
 */
double distance(const Vector3d& a, const Vector3d& b) {
  double dx = a.x() - b.x();
  double dy = a.y() - b.y();
  double dz = a.z() - b.z();
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

/**
 * @brief Create modern BodyCollection from legacy SolarSystem data
 */
BodyCollection create_body_collection_from_legacy() {
  BodyCollection bodies;

  for (int i = 0; i < ::count; i++) {
    Vector3d position = coord_to_vector3d(SolarSystem[i].position);
    Vector3d velocity = velocity_to_vector3d(SolarSystem[i].speed);

    Body body(SolarSystem[i].name, SolarSystem[i].mass, position, velocity);
    bodies.add_body(body);
  }

  return bodies;
}

/**
 * @brief Print barycenter using modern types but legacy format
 */
void print_barycenter(const Vector3d& barycenter) {
  // Legacy format: (x;y;z;distance)
  double distance_from_origin = barycenter.magnitude();
  std::cout << "Barycenter: (" << barycenter.x() << ";" << barycenter.y() << ";" << barycenter.z()
            << ";" << distance_from_origin << ")" << std::endl;
}

/**
 * @brief Modern print function using BodyCollection
 */
void print_all_bodies(const BodyCollection& bodies,
                      std::chrono::system_clock::time_point target_time) {
  // Calculate barycenter using modern BodyCollection
  Vector3d barycenter = bodies.center_of_mass();
  print_barycenter(barycenter);

  // Convert modern time to time_t for legacy formatting
  auto target_time_t = std::chrono::system_clock::to_time_t(target_time);
  std::cout << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << std::endl;

  // Print all bodies using modern types and modern data structures
  for (const auto& body : bodies) {
    // Calculate relative position (same as legacy: position - barycenter)
    Vector3d relative_position{body.position().x() - barycenter.x(),
                               body.position().y() - barycenter.y(),
                               body.position().z() - barycenter.z()};

    // Calculate distance from barycenter
    double distance_from_barycenter = relative_position.magnitude();

    // Print in exact legacy format
    std::cout << std::setw(15) << body.name() << std::setw(21) << std::scientific
              << relative_position.x() << std::setw(21) << std::scientific << relative_position.y()
              << std::setw(21) << std::scientific << relative_position.z() << std::setw(21)
              << std::scientific << distance_from_barycenter << std::setw(21) << std::scientific
              << body.velocity().x() << std::setw(21) << std::scientific << body.velocity().y()
              << std::setw(21) << std::scientific << body.velocity().z() << std::endl;
  }
}
Vector3d calculate_barycenter_modern() {
  // Same algorithm as legacy getBarycenter() but using Vector3d
  long double massSum = 0;
  for (int i = 0; i < ::count; i++) {
    massSum += SolarSystem[i].mass;
  }

  Vector3d weighted_position{0.0, 0.0, 0.0};
  long double invMassSum = 1 / massSum;

  for (int i = 0; i < ::count; i++) {
    // Convert legacy position to modern Vector3d
    Vector3d position = coord_to_vector3d(SolarSystem[i].position);

    // Accumulate mass-weighted positions (same as legacy algorithm)
    weighted_position.x_ += SolarSystem[i].mass * position.x();
    weighted_position.y_ += SolarSystem[i].mass * position.y();
    weighted_position.z_ += SolarSystem[i].mass * position.z();
  }

  // Apply inverse mass sum (same as legacy)
  weighted_position.x_ *= invMassSum;
  weighted_position.y_ *= invMassSum;
  weighted_position.z_ *= invMassSum;

  return weighted_position;
}

/**
 * @brief Modern simulation using legacy physics exactly
 */
bool run_fixed_simulation(const SimulationArgs& args,
                          std::chrono::system_clock::time_point start_time,
                          std::chrono::system_clock::time_point target_time) {
  bool forward = target_time > start_time;
  std::cout << (forward ? "Running forward simulation" : "Running backward simulation (past date)")
            << std::endl;

  std::cout << "Created " << ::count << " celestial bodies" << std::endl;

  // Print simulation info
  std::cout << "Solar System Simulation" << std::endl;

  // Convert chrono time points to time_t for legacy formatting
  auto start_time_t = std::chrono::system_clock::to_time_t(start_time);
  auto target_time_t = std::chrono::system_clock::to_time_t(target_time);

  std::cout << "Start date: "
            << std::put_time(std::localtime(&start_time_t), "%a %b %d %H:%M:%S %Y") << std::endl;

  if (args.use_current_date) {
    std::cout << "Target date: Current time ("
              << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << ")"
              << std::endl;
  } else {
    std::cout << "Target date: " << args.date_string << " ("
              << std::put_time(std::localtime(&target_time_t), "%a %b %d %H:%M:%S %Y") << ")"
              << std::endl;
  }

  // Print initial barycenter using modern calculation
  BodyCollection initial_bodies = create_body_collection_from_legacy();
  print_barycenter(initial_bodies.center_of_mass());

  // Run simulation using EXACT legacy method but with modern time types
  auto current_time = start_time;
  std::chrono::seconds time_step_duration{forward ? dt : -dt};

  while (forward ? (current_time < target_time) : (current_time > target_time)) {
    // Still use legacy perform_simulation_step (physics unchanged)
    perform_simulation_step(forward ? dt : -dt);

    // Advance time using modern chrono
    current_time += time_step_duration;
  }

  // Print final results using modern BodyCollection
  BodyCollection bodies = create_body_collection_from_legacy();
  print_all_bodies(bodies, target_time);

  return true;
}

/**
 * @brief Handle JPL data operations
 */
bool handle_jpl_operations(const SimulationArgs& args) {
  if (args.update_data) {
    std::cout << "Updating ephemeris data..." << std::endl;
    if (update_ephemeris_data()) {
      std::cout << "Ephemeris data updated successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Failed to update ephemeris data" << std::endl;
      return false;
    }
  }

  if (args.rebuild_cache) {
    std::cout << "Rebuilding binary cache..." << std::endl;
    if (rebuild_binary_cache()) {
      std::cout << "Binary cache rebuilt successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Failed to rebuild binary cache" << std::endl;
      return false;
    }
  }

  if (args.test_storage) {
    std::cout << "Testing storage system..." << std::endl;
    if (save_current_data_for_testing()) {
      std::cout << "Storage system test completed successfully" << std::endl;
      return true;
    } else {
      std::cerr << "Storage system test failed" << std::endl;
      return false;
    }
  }

  return true;
}

/**
 * @brief Main application - uses legacy physics directly
 */
int main(int argc, char* argv[]) {
  // Parse command line arguments
  SimulationArgs args = parse_arguments(argc, argv);

  // Initialize JPL data system
  initialize_jpl_data();

  // Set output precision
  std::cout.precision(12);

  // Handle JPL data operations
  if (args.update_data || args.rebuild_cache || args.test_storage) {
    return handle_jpl_operations(args) ? 0 : 1;
  }

  // Determine starting date (same logic as legacy)
  time_t start_time;
  if (has_current_ephemeris_data()) {
    start_time = get_ephemeris_epoch();
    std::cout << "Using JPL ephemeris data: " << get_ephemeris_source() << std::endl;
  } else {
    struct tm start_timeinfo = {};
    start_timeinfo.tm_sec = 0;
    start_timeinfo.tm_min = 0;
    start_timeinfo.tm_hour = 0;
    start_timeinfo.tm_mday = 11;
    start_timeinfo.tm_mon = 1;
    start_timeinfo.tm_year = 2018 - 1900;
    start_timeinfo.tm_isdst = -1;
    start_time = mktime(&start_timeinfo);
    std::cout << "Using original ephemeris data: " << get_ephemeris_source() << std::endl;
  }

  // Show ephemeris data status
  std::cout << "Ephemeris epoch: "
            << std::put_time(std::localtime(&start_time), "%a %b %d %H:%M:%S %Y") << std::endl;

  if (!has_current_year_ephemeris_data()) {
    std::cout << "Note: Consider updating ephemeris data with -u for current year" << std::endl;
  }

  // Convert time_t to modern chrono time points
  auto start_time_chrono = std::chrono::system_clock::from_time_t(start_time);
  auto target_time_chrono = std::chrono::system_clock::from_time_t(args.target_date);

  // Run the fixed simulation using modern time types
  bool success = run_fixed_simulation(args, start_time_chrono, target_time_chrono);

  return success ? 0 : 1;
}
