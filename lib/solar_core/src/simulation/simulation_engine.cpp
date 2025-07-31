#include "solar_core/simulation/simulation_engine.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace SolarSystem::Simulation {

SimulationEngine::SimulationEngine(SimulationConfig config) : config_(std::move(config)) {
  state_.reference_time = std::chrono::system_clock::now();
}

Utils::Expected<void, std::string> SimulationEngine::initialize(
    Bodies::BodyCollection bodies, std::chrono::system_clock::time_point reference_time) {
  if (bodies.empty()) {
    return Utils::Expected<void, std::string>{
        "Cannot initialize simulation with empty body collection"};
  }

  bodies_ = std::move(bodies);
  state_.reference_time = reference_time;
  state_.current_time = 0.0;
  state_.iteration_count = 0;

  // Initialize statistics
  update_statistics();

  initialized_ = true;
  return Utils::Expected<void, std::string>{};
}

Utils::Expected<void, std::string> SimulationEngine::step() { return step(config_.time_step); }

Utils::Expected<void, std::string> SimulationEngine::step(double custom_timestep) {
  if (!initialized_) {
    return Utils::Expected<void, std::string>{"Simulation not initialized"};
  }

  auto validation_result = validate_state();
  if (!validation_result.has_value()) {
    return validation_result;
  }

  double dt = custom_timestep;
  if (config_.use_adaptive_timestep) {
    dt = calculate_adaptive_timestep();
    dt = std::clamp(dt, config_.min_timestep, config_.max_timestep);
  }

  // Perform integration step based on selected method
  Utils::Expected<void, std::string> result;
  switch (integration_method_) {
    case IntegrationMethod::EULER:
      result = step_euler(dt);
      break;
    case IntegrationMethod::LEAPFROG:
      result = step_leapfrog(dt);
      break;
    case IntegrationMethod::RUNGE_KUTTA_4:
      result = step_runge_kutta_4(dt);
      break;
    case IntegrationMethod::VERLET:
      result = step_verlet(dt);
      break;
  }

  if (!result.has_value()) {
    return result;
  }

  // Update simulation state
  state_.current_time += dt;
  state_.iteration_count++;

  // Update timestep statistics
  if (state_.iteration_count == 1) {
    state_.largest_timestep = dt;
    state_.smallest_timestep = dt;
  } else {
    state_.largest_timestep = std::max(state_.largest_timestep, dt);
    state_.smallest_timestep = std::min(state_.smallest_timestep, dt);
  }

  // Check for collisions if enabled
  if (config_.enable_collision_detection) {
    check_collisions();
  }

  // Update statistics
  update_statistics();

  // Call progress callback if set
  if (progress_callback_) {
    progress_callback_(state_);
  }

  return Utils::Expected<void, std::string>{};
}

Utils::Expected<void, std::string> SimulationEngine::simulate_to_time(double target_time_seconds) {
  if (!initialized_) {
    return Utils::Expected<void, std::string>{"Simulation not initialized"};
  }

  // Use fixed timesteps like legacy version - no variable step size for performance and accuracy
  while (state_.current_time < target_time_seconds) {
    auto result = step(config_.time_step);  // Always use exactly config_.time_step (30 seconds)
    if (!result.has_value()) {
      return result;
    }
  }

  return Utils::Expected<void, std::string>{};
}

Utils::Expected<void, std::string> SimulationEngine::simulate_to_date(
    std::chrono::system_clock::time_point target_date) {
  auto duration = target_date - state_.reference_time;
  double target_seconds = std::chrono::duration<double>(duration).count();

  return simulate_to_time(target_seconds);
}

Utils::Expected<void, std::string> SimulationEngine::simulate_duration(double duration_seconds) {
  return simulate_to_time(state_.current_time + duration_seconds);
}

const Bodies::BodyCollection& SimulationEngine::get_bodies() const { return bodies_; }

Bodies::BodyCollection& SimulationEngine::get_bodies() { return bodies_; }

std::chrono::system_clock::time_point SimulationEngine::get_current_date() const {
  auto duration = std::chrono::duration<double>(state_.current_time);
  return state_.reference_time +
         std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);
}

void SimulationEngine::update_statistics() {
  state_.total_energy = calculate_total_energy();
  state_.kinetic_energy = calculate_kinetic_energy();
  state_.potential_energy = calculate_potential_energy();
  state_.center_of_mass = calculate_center_of_mass();
  state_.total_momentum = calculate_total_momentum();
}

long double SimulationEngine::calculate_total_energy() const {
  return calculate_kinetic_energy() + calculate_potential_energy();
}

long double SimulationEngine::calculate_kinetic_energy() const {
  long double total_ke = 0.0;
  for (const auto& body : bodies_) {
    long double v_squared = body.velocity().magnitude_squared();
    total_ke += 0.5 * body.mass() * v_squared;
  }
  return total_ke;
}

long double SimulationEngine::calculate_potential_energy() const {
  long double total_pe = 0.0;

  // Calculate gravitational potential energy between all pairs
  for (auto it1 = bodies_.begin(); it1 != bodies_.end(); ++it1) {
    for (auto it2 = std::next(it1); it2 != bodies_.end(); ++it2) {
      const auto& body1 = *it1;
      const auto& body2 = *it2;

      long double distance = (body2.position() - body1.position()).magnitude();
      if (distance > 0.0) {
        total_pe -= config_.gravitational_constant * body1.mass() * body2.mass() / distance;
      }
    }
  }

  return total_pe;
}

Math::Vector3d SimulationEngine::calculate_center_of_mass() const {
  return bodies_.center_of_mass();
}

Math::Vector3d SimulationEngine::calculate_total_momentum() const {
  Math::Vector3d total_momentum{};
  for (const auto& body : bodies_) {
    total_momentum += body.velocity() * body.mass();
  }
  return total_momentum;
}

void SimulationEngine::reset() {
  state_ = SimulationState{};
  state_.reference_time = std::chrono::system_clock::now();
  initialized_ = false;
}

std::string SimulationEngine::get_status_summary() const {
  std::ostringstream oss;
  oss << "Simulation Status:\n";
  oss << "  Initialized: " << (initialized_ ? "Yes" : "No") << "\n";
  oss << "  Bodies: " << bodies_.size() << "\n";
  oss << "  Current time: " << state_.current_time << " seconds\n";
  oss << "  Iterations: " << state_.iteration_count << "\n";
  oss << "  Integration method: " << to_string(integration_method_) << "\n";
  oss << "  Time step: " << config_.time_step << " seconds\n";
  oss << "  Total energy: " << state_.total_energy << " J\n";
  oss << "  Kinetic energy: " << state_.kinetic_energy << " J\n";
  oss << "  Potential energy: " << state_.potential_energy << " J\n";
  return oss.str();
}

// Private implementation methods

Utils::Expected<void, std::string> SimulationEngine::step_euler(double dt) {
  // Simple Euler integration: x(t+dt) = x(t) + v(t)*dt, v(t+dt) = v(t) + a(t)*dt

  std::vector<Math::Vector3d> forces(bodies_.size());
  calculate_forces(forces);

  size_t i = 0;
  for (auto& body : bodies_) {
    // Calculate acceleration
    Math::Vector3d acceleration = forces[i] / body.mass();

    // Update position and velocity
    body.set_position(body.position() + body.velocity() * dt);
    body.set_velocity(body.velocity() + acceleration * dt);

    ++i;
  }

  return Utils::Expected<void, std::string>{};
}

Utils::Expected<void, std::string> SimulationEngine::step_leapfrog(double dt) {
  // Leapfrog integration - good for orbital mechanics
  // v(t+dt/2) = v(t) + a(t)*dt/2
  // x(t+dt) = x(t) + v(t+dt/2)*dt
  // v(t+dt) = v(t+dt/2) + a(t+dt)*dt/2

  std::vector<Math::Vector3d> forces(bodies_.size());
  calculate_forces(forces);

  // First half-step for velocities
  size_t i = 0;
  for (auto& body : bodies_) {
    Math::Vector3d acceleration = forces[i] / body.mass();
    body.set_velocity(body.velocity() + acceleration * (dt * 0.5));
    ++i;
  }

  // Full step for positions
  for (auto& body : bodies_) {
    body.set_position(body.position() + body.velocity() * dt);
  }

  // Recalculate forces at new positions
  calculate_forces(forces);

  // Second half-step for velocities
  i = 0;
  for (auto& body : bodies_) {
    Math::Vector3d acceleration = forces[i] / body.mass();
    body.set_velocity(body.velocity() + acceleration * (dt * 0.5));
    ++i;
  }

  return Utils::Expected<void, std::string>{};
}

Utils::Expected<void, std::string> SimulationEngine::step_runge_kutta_4(double dt) {
  // 4th order Runge-Kutta integration - high accuracy
  // This is more complex but provides better accuracy for general ODEs

  // For now, fall back to leapfrog which is more suitable for N-body problems
  return step_leapfrog(dt);
}

Utils::Expected<void, std::string> SimulationEngine::step_verlet(double dt) {
  // Velocity Verlet integration - energy conserving
  // x(t+dt) = x(t) + v(t)*dt + 0.5*a(t)*dt²
  // v(t+dt) = v(t) + 0.5*(a(t) + a(t+dt))*dt

  std::vector<Math::Vector3d> old_forces(bodies_.size());
  calculate_forces(old_forces);

  // Update positions and store old accelerations
  std::vector<Math::Vector3d> old_accelerations(bodies_.size());
  size_t i = 0;
  for (auto& body : bodies_) {
    old_accelerations[i] = old_forces[i] / body.mass();
    body.set_position(body.position() + body.velocity() * dt +
                      old_accelerations[i] * (0.5 * dt * dt));
    ++i;
  }

  // Calculate new forces at new positions
  std::vector<Math::Vector3d> new_forces(bodies_.size());
  calculate_forces(new_forces);

  // Update velocities using average of old and new accelerations
  i = 0;
  for (auto& body : bodies_) {
    Math::Vector3d new_acceleration = new_forces[i] / body.mass();
    body.set_velocity(body.velocity() + (old_accelerations[i] + new_acceleration) * (0.5 * dt));
    ++i;
  }

  return Utils::Expected<void, std::string>{};
}

void SimulationEngine::calculate_forces(std::vector<Math::Vector3d>& forces) const {
  forces.assign(bodies_.size(), Math::Vector3d{});

  size_t i = 0;
  for (auto it1 = bodies_.begin(); it1 != bodies_.end(); ++it1, ++i) {
    size_t j = i + 1;
    for (auto it2 = std::next(it1); it2 != bodies_.end(); ++it2, ++j) {
      Math::Vector3d force = calculate_gravitational_force(*it1, *it2);
      forces[i] += force;
      forces[j] -= force;  // Newton's third law
    }
  }
}

Math::Vector3d SimulationEngine::calculate_gravitational_force(
    const Bodies::CelestialBody& body1, const Bodies::CelestialBody& body2) const {
  Math::Vector3d r = body2.position() - body1.position();
  long double distance = r.magnitude();

  if (distance == 0.0) {
    return Math::Vector3d{};  // Avoid division by zero
  }

  long double force_magnitude =
      config_.gravitational_constant * body1.mass() * body2.mass() / (distance * distance);
  return r.normalized() * force_magnitude;
}

double SimulationEngine::calculate_adaptive_timestep() const {
  // Simple adaptive timestep based on maximum acceleration
  long double max_acceleration = 0.0;

  std::vector<Math::Vector3d> forces(bodies_.size());
  calculate_forces(forces);

  size_t i = 0;
  for (const auto& body : bodies_) {
    if (body.mass() > 0.0) {
      long double acceleration = forces[i].magnitude() / body.mass();
      max_acceleration = std::max(max_acceleration, acceleration);
    }
    ++i;
  }

  if (max_acceleration > 0.0) {
    // Choose timestep such that position change is reasonable
    return std::sqrt(config_.tolerance / static_cast<double>(max_acceleration));
  }

  return config_.time_step;
}

void SimulationEngine::check_collisions() {
  for (auto it1 = bodies_.begin(); it1 != bodies_.end(); ++it1) {
    for (auto it2 = std::next(it1); it2 != bodies_.end(); ++it2) {
      long double distance = (it2->position() - it1->position()).magnitude();
      if (distance < config_.collision_threshold) {
        if (collision_callback_) {
          collision_callback_(*it1, *it2);
        }
      }
    }
  }
}

Utils::Expected<void, std::string> SimulationEngine::validate_state() const {
  if (bodies_.empty()) {
    return Utils::Expected<void, std::string>{"No bodies in simulation"};
  }

  for (const auto& body : bodies_) {
    if (body.mass() <= 0.0) {
      return Utils::Expected<void, std::string>{"Body with non-positive mass: " +
                                                std::string(body.name())};
    }

    if (!std::isfinite(body.position().x()) || !std::isfinite(body.position().y()) ||
        !std::isfinite(body.position().z())) {
      return Utils::Expected<void, std::string>{"Body with invalid position: " +
                                                std::string(body.name())};
    }

    if (!std::isfinite(body.velocity().x()) || !std::isfinite(body.velocity().y()) ||
        !std::isfinite(body.velocity().z())) {
      return Utils::Expected<void, std::string>{"Body with invalid velocity: " +
                                                std::string(body.name())};
    }
  }

  return Utils::Expected<void, std::string>{};
}

// Utility functions

std::string to_string(SimulationEngine::IntegrationMethod method) {
  switch (method) {
    case SimulationEngine::IntegrationMethod::EULER:
      return "Euler";
    case SimulationEngine::IntegrationMethod::LEAPFROG:
      return "Leapfrog";
    case SimulationEngine::IntegrationMethod::RUNGE_KUTTA_4:
      return "Runge-Kutta 4";
    case SimulationEngine::IntegrationMethod::VERLET:
      return "Verlet";
  }
  return "Unknown";
}

SimulationEngine::IntegrationMethod integration_method_from_string(std::string_view str) {
  if (str == "Euler" || str == "euler") return SimulationEngine::IntegrationMethod::EULER;
  if (str == "Leapfrog" || str == "leapfrog") return SimulationEngine::IntegrationMethod::LEAPFROG;
  if (str == "Runge-Kutta 4" || str == "rk4")
    return SimulationEngine::IntegrationMethod::RUNGE_KUTTA_4;
  if (str == "Verlet" || str == "verlet") return SimulationEngine::IntegrationMethod::VERLET;
  return SimulationEngine::IntegrationMethod::LEAPFROG;  // Default
}

}  // namespace SolarSystem::Simulation
