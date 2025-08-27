#include "solar_core/bodies/body_collection.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <regex>
#include <sstream>

namespace SolarSystem::Bodies {

BodyCollection::BodyCollection(std::vector<CelestialBody> bodies) : bodies_(std::move(bodies)) {
  rebuild_index();
}

void BodyCollection::add_body(CelestialBody body) {
  // Check for duplicate names
  if (contains(body.name())) {
    throw std::invalid_argument("Body with name '" + std::string(body.name()) + "' already exists");
  }

  bodies_.push_back(std::move(body));
  update_index_for_body(bodies_.size() - 1);
}

void BodyCollection::add_body(CelestialBody::Properties props) {
  add_body(CelestialBody{std::move(props)});
}

bool BodyCollection::remove_body(std::string_view name) {
  auto it = std::find_if(bodies_.begin(), bodies_.end(),
                         [name](const CelestialBody& body) { return body.name() == name; });

  if (it != bodies_.end()) {
    bodies_.erase(it);
    rebuild_index();  // Rebuild index after removal
    return true;
  }
  return false;
}

void BodyCollection::clear() {
  bodies_.clear();
  name_index_.clear();
}

std::optional<std::reference_wrapper<CelestialBody>> BodyCollection::find_body(
    std::string_view name) {
  auto it = name_index_.find(std::string(name));
  if (it != name_index_.end() && it->second < bodies_.size()) {
    return std::ref(bodies_[it->second]);
  }
  return std::nullopt;
}

std::optional<std::reference_wrapper<const CelestialBody>> BodyCollection::find_body(
    std::string_view name) const {
  auto it = name_index_.find(std::string(name));
  if (it != name_index_.end() && it->second < bodies_.size()) {
    return std::cref(bodies_[it->second]);
  }
  return std::nullopt;
}

bool BodyCollection::contains(std::string_view name) const {
  return name_index_.find(std::string(name)) != name_index_.end();
}

std::vector<std::reference_wrapper<const CelestialBody>> BodyCollection::filter_by_type(
    BodyType type) const {
  std::vector<std::reference_wrapper<const CelestialBody>> result;
  for (const auto& body : bodies_) {
    if (body.type() == type) {
      result.emplace_back(std::cref(body));
    }
  }
  return result;
}

std::vector<std::reference_wrapper<const CelestialBody>> BodyCollection::filter_by_priority(
    BodyPriority priority) const {
  std::vector<std::reference_wrapper<const CelestialBody>> result;
  for (const auto& body : bodies_) {
    if (body.priority() == priority) {
      result.emplace_back(std::cref(body));
    }
  }
  return result;
}

std::vector<std::reference_wrapper<const CelestialBody>> BodyCollection::filter_essential() const {
  return filter_by_priority(BodyPriority::Essential);
}

std::vector<std::reference_wrapper<const CelestialBody>> BodyCollection::filter_available_at(
    std::chrono::system_clock::time_point time) const {
  std::vector<std::reference_wrapper<const CelestialBody>> result;
  for (const auto& body : bodies_) {
    if (body.is_available_at(time)) {
      result.emplace_back(std::cref(body));
    }
  }
  return result;
}

void BodyCollection::apply_to_all(std::function<void(CelestialBody&)> func) {
  for (auto& body : bodies_) {
    func(body);
  }
}

void BodyCollection::apply_to_all(std::function<void(const CelestialBody&)> func) const {
  for (const auto& body : bodies_) {
    func(body);
  }
}

BodyCollection::size_type BodyCollection::count_by_type(BodyType type) const {
  return static_cast<size_type>(
      std::count_if(bodies_.begin(), bodies_.end(),
                    [type](const CelestialBody& body) { return body.type() == type; }));
}

BodyCollection::size_type BodyCollection::count_by_priority(BodyPriority priority) const {
  return static_cast<size_type>(
      std::count_if(bodies_.begin(), bodies_.end(),
                    [priority](const CelestialBody& body) { return body.priority() == priority; }));
}

double BodyCollection::total_mass() const {
  return std::accumulate(bodies_.begin(), bodies_.end(), 0.0,
                         [](double sum, const CelestialBody& body) { return sum + body.mass(); });
}

Math::Vector3d BodyCollection::center_of_mass() const {
  if (bodies_.empty()) {
    return Math::Vector3d{};
  }

  // Use EXACT same algorithm as legacy getBarycenter() with native long double precision
  long double massSum = 0.0L;
  for (const auto& body : bodies_) {
    massSum += body.mass();  // Now already long double
  }

  // Accumulate weighted positions using long double precision
  long double weighted_x = 0.0L;
  long double weighted_y = 0.0L;
  long double weighted_z = 0.0L;

  long double invMassSum = 1.0L / massSum;

  for (const auto& body : bodies_) {
    long double mass = body.mass();        // Now already long double
    Math::Vector3d pos = body.position();  // Now already long double components

    // Accumulate mass-weighted positions (same as legacy)
    weighted_x += mass * pos.x();
    weighted_y += mass * pos.y();
    weighted_z += mass * pos.z();
  }

  // Apply inverse mass sum (same as legacy - multiply instead of divide)
  long double final_x = weighted_x * invMassSum;
  long double final_y = weighted_y * invMassSum;
  long double final_z = weighted_z * invMassSum;

  return Math::Vector3d{final_x, final_y, final_z};
}

bool BodyCollection::validate() const { return get_validation_errors().empty(); }

std::vector<std::string> BodyCollection::get_validation_errors() const {
  std::vector<std::string> errors;

  // Check for duplicate names
  std::unordered_map<std::string, size_t> name_counts;
  for (const auto& body : bodies_) {
    name_counts[std::string(body.name())]++;
  }

  for (const auto& [name, count] : name_counts) {
    if (count > 1) {
      errors.push_back("Duplicate body name: " + name);
    }
  }

  // Check for invalid masses
  for (size_t i = 0; i < bodies_.size(); ++i) {
    if (bodies_[i].mass() < 0.0) {
      errors.push_back("Body '" + std::string(bodies_[i].name()) + "' has negative mass");
    }
  }

  return errors;
}

std::string BodyCollection::to_json() const {
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"body_count\": " << bodies_.size() << ",\n";
  oss << "  \"total_mass\": " << std::scientific << total_mass() << ",\n";
  oss << "  \"bodies\": [\n";

  for (size_t i = 0; i < bodies_.size(); ++i) {
    const auto& body = bodies_[i];
    oss << "    {\n";
    oss << "      \"name\": \"" << body.name() << "\",\n";
    oss << "      \"type\": \"" << to_string(body.type()) << "\",\n";
    oss << "      \"priority\": \"" << to_string(body.priority()) << "\",\n";
    oss << "      \"mass\": " << std::scientific << body.mass() << ",\n";
    oss << "      \"position\": [" << body.position().x() << ", " << body.position().y() << ", "
        << body.position().z() << "],\n";
    oss << "      \"velocity\": [" << body.velocity().x() << ", " << body.velocity().y() << ", "
        << body.velocity().z() << "]\n";
    oss << "    }";
    if (i < bodies_.size() - 1) oss << ",";
    oss << "\n";
  }

  oss << "  ]\n";
  oss << "}";
  return oss.str();
}

std::string BodyCollection::summary() const {
  std::ostringstream oss;
  oss << "BodyCollection Summary:\n";
  oss << "  Total Bodies: " << bodies_.size() << "\n";
  oss << "  Total Mass: " << std::scientific << total_mass() << " kg\n";

  // Count by type
  oss << "  By Type:\n";
  for (auto type : {BodyType::Star, BodyType::Planet, BodyType::Moon, BodyType::DwarfPlanet,
                    BodyType::Asteroid, BodyType::Spacecraft}) {
    auto count = count_by_type(type);
    if (count > 0) {
      oss << "    " << to_string(type) << ": " << count << "\n";
    }
  }

  // Count by priority
  oss << "  By Priority:\n";
  for (auto priority : {BodyPriority::Essential, BodyPriority::Important, BodyPriority::Optional}) {
    auto count = count_by_priority(priority);
    if (count > 0) {
      oss << "    " << to_string(priority) << ": " << count << "\n";
    }
  }

  return oss.str();
}

void BodyCollection::rebuild_index() {
  name_index_.clear();
  for (size_t i = 0; i < bodies_.size(); ++i) {
    name_index_[std::string(bodies_[i].name())] = i;
  }
}

void BodyCollection::update_index_for_body(size_type index) {
  if (index < bodies_.size()) {
    name_index_[std::string(bodies_[index].name())] = index;
  }
}

// Enhanced validation and consistency checking

bool BodyCollection::validate_comprehensive() const {
  return get_comprehensive_validation_errors().empty();
}

std::vector<std::string> BodyCollection::get_comprehensive_validation_errors() const {
  std::vector<std::string> errors;

  // Basic validation first
  auto basic_errors = get_validation_errors();
  errors.insert(errors.end(), basic_errors.begin(), basic_errors.end());

  // Check for NaN or infinite values in positions and velocities
  for (size_t i = 0; i < bodies_.size(); ++i) {
    const auto& body = bodies_[i];
    const auto& pos = body.position();
    const auto& vel = body.velocity();

    // Check position components
    if (!std::isfinite(static_cast<double>(pos.x())) ||
        !std::isfinite(static_cast<double>(pos.y())) ||
        !std::isfinite(static_cast<double>(pos.z()))) {
      errors.push_back("Body '" + std::string(body.name()) + "' has non-finite position values");
    }

    // Check velocity components
    if (!std::isfinite(static_cast<double>(vel.x())) ||
        !std::isfinite(static_cast<double>(vel.y())) ||
        !std::isfinite(static_cast<double>(vel.z()))) {
      errors.push_back("Body '" + std::string(body.name()) + "' has non-finite velocity values");
    }

    // Check for extremely large values that might indicate data corruption
    long double pos_magnitude = pos.magnitude();
    long double vel_magnitude = vel.magnitude();

    if (pos_magnitude > 1.0e18L) {  // Beyond reasonable solar system scale
      errors.push_back("Body '" + std::string(body.name()) +
                       "' has unreasonably large position magnitude: " +
                       std::to_string(static_cast<double>(pos_magnitude)) + " m");
    }

    if (vel_magnitude > 1.0e6L) {  // Beyond reasonable velocity scale
      errors.push_back("Body '" + std::string(body.name()) +
                       "' has unreasonably large velocity magnitude: " +
                       std::to_string(static_cast<double>(vel_magnitude)) + " m/s");
    }
  }

  // Check for mass consistency (total mass should be reasonable)
  double total = total_mass();
  if (!std::isfinite(total)) {
    errors.push_back("Total mass calculation resulted in non-finite value");
  } else if (total > 1.0e32) {  // Much larger than observable universe mass
    errors.push_back("Total mass is unreasonably large: " + std::to_string(total) + " kg");
  }

  // Check index consistency
  if (name_index_.size() != bodies_.size()) {
    errors.push_back("Name index size (" + std::to_string(name_index_.size()) +
                     ") does not match body count (" + std::to_string(bodies_.size()) + ")");
  }

  // Verify index integrity
  for (const auto& [name, index] : name_index_) {
    if (index >= bodies_.size()) {
      errors.push_back("Name index for '" + name +
                       "' points to invalid body index: " + std::to_string(index));
    } else if (bodies_[index].name() != name) {
      errors.push_back("Name index inconsistency: '" + name + "' maps to body '" +
                       std::string(bodies_[index].name()) + "'");
    }
  }

  return errors;
}

BodyCollection::ConsistencyReport BodyCollection::check_consistency() const {
  ConsistencyReport report;
  report.check_time = std::chrono::system_clock::now();
  report.bodies_checked = bodies_.size();

  // Get comprehensive validation errors
  auto errors = get_comprehensive_validation_errors();

  // Separate into issues and warnings based on severity
  for (const auto& error : errors) {
    if (error.find("unreasonably") != std::string::npos ||
        error.find("non-finite") != std::string::npos ||
        error.find("negative mass") != std::string::npos ||
        error.find("index") != std::string::npos) {
      report.issues.push_back(error);
    } else {
      report.warnings.push_back(error);
    }
  }

  report.is_consistent = report.issues.empty();
  return report;
}

void BodyCollection::maintain_consistency() {
  // Rebuild index to fix any inconsistencies
  rebuild_index();

  // Remove any bodies with invalid data
  auto it = std::remove_if(bodies_.begin(), bodies_.end(), [](const CelestialBody& body) {
    // Remove bodies with negative mass or non-finite values
    if (body.mass() < 0.0L) return true;

    const auto& pos = body.position();
    const auto& vel = body.velocity();

    if (!std::isfinite(static_cast<double>(pos.x())) ||
        !std::isfinite(static_cast<double>(pos.y())) ||
        !std::isfinite(static_cast<double>(pos.z())))
      return true;

    if (!std::isfinite(static_cast<double>(vel.x())) ||
        !std::isfinite(static_cast<double>(vel.y())) ||
        !std::isfinite(static_cast<double>(vel.z())))
      return true;

    return false;
  });

  if (it != bodies_.end()) {
    bodies_.erase(it, bodies_.end());
    rebuild_index();  // Rebuild index after removal
  }
}

// Enhanced operations with error handling

BodyCollection::OperationResult BodyCollection::add_body_validated(const CelestialBody& body) {
  OperationResult result;
  result.affected_bodies = 0;

  // Check for duplicate names
  if (contains(body.name())) {
    result.success = false;
    result.error_message = "Body with name '" + std::string(body.name()) + "' already exists";
    return result;
  }

  // Validate the body data
  if (body.mass() < 0.0L) {
    result.success = false;
    result.error_message = "Body '" + std::string(body.name()) + "' has negative mass";
    return result;
  }

  const auto& pos = body.position();
  const auto& vel = body.velocity();

  // Check for non-finite values
  if (!std::isfinite(static_cast<double>(pos.x())) ||
      !std::isfinite(static_cast<double>(pos.y())) ||
      !std::isfinite(static_cast<double>(pos.z()))) {
    result.success = false;
    result.error_message = "Body '" + std::string(body.name()) + "' has non-finite position values";
    return result;
  }

  if (!std::isfinite(static_cast<double>(vel.x())) ||
      !std::isfinite(static_cast<double>(vel.y())) ||
      !std::isfinite(static_cast<double>(vel.z()))) {
    result.success = false;
    result.error_message = "Body '" + std::string(body.name()) + "' has non-finite velocity values";
    return result;
  }

  // Check for unreasonably large values
  long double pos_magnitude = pos.magnitude();
  long double vel_magnitude = vel.magnitude();

  if (pos_magnitude > 1.0e18L) {
    result.warning = "Body '" + std::string(body.name()) + "' has very large position magnitude: " +
                     std::to_string(static_cast<double>(pos_magnitude)) + " m";
  }

  if (vel_magnitude > 1.0e6L) {
    result.warning = "Body '" + std::string(body.name()) + "' has very large velocity magnitude: " +
                     std::to_string(static_cast<double>(vel_magnitude)) + " m/s";
  }

  // Add the body
  try {
    bodies_.push_back(body);
    update_index_for_body(bodies_.size() - 1);
    result.success = true;
    result.affected_bodies = 1;
  } catch (const std::exception& e) {
    result.success = false;
    result.error_message = "Failed to add body: " + std::string(e.what());
  }

  return result;
}

BodyCollection::OperationResult BodyCollection::add_body_validated(
    CelestialBody::Properties props) {
  return add_body_validated(CelestialBody{std::move(props)});
}

BodyCollection::OperationResult BodyCollection::remove_body_safe(std::string_view name) {
  OperationResult result;
  result.affected_bodies = 0;

  if (name.empty()) {
    result.success = false;
    result.error_message = "Cannot remove body with empty name";
    return result;
  }

  auto it = std::find_if(bodies_.begin(), bodies_.end(),
                         [name](const CelestialBody& body) { return body.name() == name; });

  if (it != bodies_.end()) {
    try {
      bodies_.erase(it);
      rebuild_index();  // Rebuild index after removal
      result.success = true;
      result.affected_bodies = 1;
    } catch (const std::exception& e) {
      result.success = false;
      result.error_message = "Failed to remove body: " + std::string(e.what());
    }
  } else {
    result.success = false;
    result.error_message = "Body '" + std::string(name) + "' not found";
  }

  return result;
}

BodyCollection::OperationResult BodyCollection::update_body(std::string_view name,
                                                            const CelestialBody& updated_body) {
  OperationResult result;
  result.affected_bodies = 0;

  if (name.empty()) {
    result.success = false;
    result.error_message = "Cannot update body with empty name";
    return result;
  }

  // Find the body to update
  auto it = name_index_.find(std::string(name));
  if (it == name_index_.end() || it->second >= bodies_.size()) {
    result.success = false;
    result.error_message = "Body '" + std::string(name) + "' not found";
    return result;
  }

  // Validate the updated body data (same validation as add_body_validated)
  if (updated_body.mass() < 0.0L) {
    result.success = false;
    result.error_message = "Updated body has negative mass";
    return result;
  }

  const auto& pos = updated_body.position();
  const auto& vel = updated_body.velocity();

  if (!std::isfinite(static_cast<double>(pos.x())) ||
      !std::isfinite(static_cast<double>(pos.y())) ||
      !std::isfinite(static_cast<double>(pos.z()))) {
    result.success = false;
    result.error_message = "Updated body has non-finite position values";
    return result;
  }

  if (!std::isfinite(static_cast<double>(vel.x())) ||
      !std::isfinite(static_cast<double>(vel.y())) ||
      !std::isfinite(static_cast<double>(vel.z()))) {
    result.success = false;
    result.error_message = "Updated body has non-finite velocity values";
    return result;
  }

  // Check if name is changing and if new name conflicts
  if (updated_body.name() != name && contains(updated_body.name())) {
    result.success = false;
    result.error_message = "Cannot update body name to '" + std::string(updated_body.name()) +
                           "' - name already exists";
    return result;
  }

  try {
    // Update the body
    bodies_[it->second] = updated_body;

    // If name changed, rebuild index
    if (updated_body.name() != name) {
      rebuild_index();
    }

    result.success = true;
    result.affected_bodies = 1;
  } catch (const std::exception& e) {
    result.success = false;
    result.error_message = "Failed to update body: " + std::string(e.what());
  }

  return result;
}

// Enhanced search and filtering

std::vector<std::reference_wrapper<const CelestialBody>> BodyCollection::search_by_name_pattern(
    const std::string& pattern) const {
  std::vector<std::reference_wrapper<const CelestialBody>> result;

  try {
    std::regex regex_pattern(pattern, std::regex_constants::icase);

    for (const auto& body : bodies_) {
      if (std::regex_search(std::string(body.name()), regex_pattern)) {
        result.emplace_back(std::cref(body));
      }
    }
  } catch (const std::regex_error&) {
    // If regex is invalid, fall back to simple substring search
    std::string lower_pattern = pattern;
    std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), ::tolower);

    for (const auto& body : bodies_) {
      std::string lower_name = std::string(body.name());
      std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

      if (lower_name.find(lower_pattern) != std::string::npos) {
        result.emplace_back(std::cref(body));
      }
    }
  }

  return result;
}

std::vector<std::reference_wrapper<const CelestialBody>> BodyCollection::filter_by_mass_range(
    long double min_mass, long double max_mass) const {
  std::vector<std::reference_wrapper<const CelestialBody>> result;

  if (min_mass > max_mass) {
    return result;  // Invalid range
  }

  for (const auto& body : bodies_) {
    long double mass = body.mass();
    if (mass >= min_mass && mass <= max_mass) {
      result.emplace_back(std::cref(body));
    }
  }

  return result;
}

std::vector<std::reference_wrapper<const CelestialBody>>
BodyCollection::filter_by_distance_from_point(const Math::Vector3d& point,
                                              long double max_distance) const {
  std::vector<std::reference_wrapper<const CelestialBody>> result;

  if (max_distance < 0.0L) {
    return result;  // Invalid distance
  }

  for (const auto& body : bodies_) {
    long double distance = (body.position() - point).magnitude();
    if (distance <= max_distance) {
      result.emplace_back(std::cref(body));
    }
  }

  return result;
}

// Bulk operations with progress reporting

BodyCollection::BulkOperationResult BodyCollection::bulk_add_bodies(
    const std::vector<CelestialBody>& bodies) {
  BulkOperationResult result;
  auto start_time = std::chrono::steady_clock::now();

  result.total_operations = bodies.size();
  result.successful_operations = 0;
  result.failed_operations = 0;

  for (const auto& body : bodies) {
    auto add_result = add_body_validated(body);
    if (add_result.success) {
      result.successful_operations++;
      if (add_result.warning.has_value()) {
        result.warnings.push_back(add_result.warning.value());
      }
    } else {
      result.failed_operations++;
      result.errors.push_back("Failed to add '" + std::string(body.name()) +
                              "': " + add_result.error_message);
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  result.execution_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return result;
}

BodyCollection::BulkOperationResult BodyCollection::bulk_remove_bodies(
    const std::vector<std::string>& names) {
  BulkOperationResult result;
  auto start_time = std::chrono::steady_clock::now();

  result.total_operations = names.size();
  result.successful_operations = 0;
  result.failed_operations = 0;

  for (const auto& name : names) {
    auto remove_result = remove_body_safe(name);
    if (remove_result.success) {
      result.successful_operations++;
    } else {
      result.failed_operations++;
      result.errors.push_back("Failed to remove '" + name + "': " + remove_result.error_message);
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  result.execution_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return result;
}

BodyCollection::BulkOperationResult BodyCollection::bulk_update_positions(
    const std::function<Math::Vector3d(const CelestialBody&)>& position_updater) {
  BulkOperationResult result;
  auto start_time = std::chrono::steady_clock::now();

  result.total_operations = bodies_.size();
  result.successful_operations = 0;
  result.failed_operations = 0;

  for (auto& body : bodies_) {
    try {
      Math::Vector3d new_position = position_updater(body);

      // Validate new position
      if (!std::isfinite(static_cast<double>(new_position.x())) ||
          !std::isfinite(static_cast<double>(new_position.y())) ||
          !std::isfinite(static_cast<double>(new_position.z()))) {
        result.failed_operations++;
        result.errors.push_back("Position updater returned non-finite values for '" +
                                std::string(body.name()) + "'");
        continue;
      }

      body.set_position(new_position);
      result.successful_operations++;
    } catch (const std::exception& e) {
      result.failed_operations++;
      result.errors.push_back("Failed to update position for '" + std::string(body.name()) +
                              "': " + e.what());
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  result.execution_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return result;
}

}  // namespace SolarSystem::Bodies
