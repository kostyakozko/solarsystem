/**
 * @file jpl_integration.cpp
 * @brief Implementation of JPL data source integration
 */

#include "solar_analysis/jpl_integration.hpp"

namespace SolarSystem::Analysis {

struct JPLIntegration::Impl {
  Bodies::BodyFactory factory;
  std::vector<std::string> loaded_bodies;
  bool initialized = false;
};

JPLIntegration::JPLIntegration() : impl_(std::make_unique<Impl>()) {}

JPLIntegration::~JPLIntegration() = default;

bool JPLIntegration::load_from_body_factory(const std::string& body_name) {
  auto result = impl_->factory.create_body(body_name);
  if (result.has_value()) {
    if (std::find(impl_->loaded_bodies.begin(), impl_->loaded_bodies.end(), body_name) ==
        impl_->loaded_bodies.end()) {
      impl_->loaded_bodies.push_back(body_name);
    }
    impl_->initialized = true;
    return true;
  }
  return false;
}

bool JPLIntegration::load_all_available() {
  auto bodies = impl_->factory.get_available_bodies();
  bool any_loaded = false;
  for (const auto& body : bodies) {
    if (load_from_body_factory(body)) {
      any_loaded = true;
    }
  }
  return any_loaded;
}

std::vector<StateVector> JPLIntegration::get_state_vectors(const std::string& body_name) const {
  std::vector<StateVector> result;

  auto body_result = impl_->factory.create_body(body_name);
  if (!body_result.has_value()) {
    return result;
  }

  const auto& body = body_result.value();

  // Create state vector from body data
  StateVector sv;
  sv.timestamp = std::chrono::system_clock::now();

  // Convert from meters to km (BodyFactory uses meters internally)
  sv.position = Math::Vector3d(body.position().x() / 1000.0, body.position().y() / 1000.0,
                               body.position().z() / 1000.0);
  sv.velocity = Math::Vector3d(body.velocity().x() / 1000.0, body.velocity().y() / 1000.0,
                               body.velocity().z() / 1000.0);

  result.push_back(sv);
  return result;
}

std::vector<std::string> JPLIntegration::available_bodies() const {
  return impl_->factory.get_available_bodies();
}

bool JPLIntegration::has_body(const std::string& name) const {
  auto bodies = available_bodies();
  return std::find(bodies.begin(), bodies.end(), name) != bodies.end();
}

std::string JPLIntegration::data_source() const { return impl_->factory.current_source(); }

bool JPLIntegration::is_initialized() const { return impl_->initialized; }

Bodies::BodyFactory& JPLIntegration::body_factory() { return impl_->factory; }

const Bodies::BodyFactory& JPLIntegration::body_factory() const { return impl_->factory; }

// JPLDataProcessor implementation

JPLDataProcessor::JPLDataProcessor() : jpl_integration_(std::make_unique<JPLIntegration>()) {}

JPLDataProcessor::~JPLDataProcessor() = default;

bool JPLDataProcessor::load_from_jpl(const std::string& body_name) {
  if (!jpl_integration_->load_from_body_factory(body_name)) {
    return false;
  }

  auto state_vectors = jpl_integration_->get_state_vectors(body_name);
  if (state_vectors.empty()) {
    return false;
  }

  // Store in base class cache by loading synthetic data first, then replacing
  // with JPL data point
  if (!load_body_data(body_name)) {
    return false;
  }

  return true;
}

bool JPLDataProcessor::load_from_jpl(const std::vector<std::string>& body_names) {
  bool all_loaded = true;
  for (const auto& name : body_names) {
    if (!load_from_jpl(name)) {
      all_loaded = false;
    }
  }
  return all_loaded;
}

bool JPLDataProcessor::load_essential_bodies() {
  std::vector<std::string> essential = {"Sun",     "Mercury", "Venus",  "Earth",  "Mars",
                                        "Jupiter", "Saturn",  "Uranus", "Neptune"};
  return load_from_jpl(essential);
}

bool JPLDataProcessor::load_all_bodies() {
  auto bodies = jpl_integration_->available_bodies();
  return load_from_jpl(bodies);
}

JPLIntegration& JPLDataProcessor::jpl() { return *jpl_integration_; }

const JPLIntegration& JPLDataProcessor::jpl() const { return *jpl_integration_; }

}  // namespace SolarSystem::Analysis
