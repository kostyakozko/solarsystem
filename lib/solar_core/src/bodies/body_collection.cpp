#include "solar_core/bodies/body_collection.hpp"

#include <algorithm>
#include <iomanip>
#include <numeric>
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
  return std::count_if(bodies_.begin(), bodies_.end(),
                       [type](const CelestialBody& body) { return body.type() == type; });
}

BodyCollection::size_type BodyCollection::count_by_priority(BodyPriority priority) const {
  return std::count_if(bodies_.begin(), bodies_.end(), [priority](const CelestialBody& body) {
    return body.priority() == priority;
  });
}

double BodyCollection::total_mass() const {
  return std::accumulate(bodies_.begin(), bodies_.end(), 0.0,
                         [](double sum, const CelestialBody& body) { return sum + body.mass(); });
}

Math::Vector3d BodyCollection::center_of_mass() const {
  if (bodies_.empty()) {
    return Math::Vector3d{};
  }

  double total_mass_val = 0.0;
  Math::Vector3d weighted_position{};

  for (const auto& body : bodies_) {
    double mass = body.mass();
    total_mass_val += mass;
    weighted_position += body.position() * mass;
  }

  if (total_mass_val > 0.0) {
    return weighted_position / total_mass_val;
  }

  return Math::Vector3d{};
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

}  // namespace SolarSystem::Bodies
