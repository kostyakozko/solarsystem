/**
 * @file analysis_engine.cpp
 * @brief Implementation of the core analysis engine
 */

#include "solar_analysis/analysis_engine.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

#include "solar_core/bodies/body_factory.hpp"

namespace SolarSystem::Analysis {

struct BodyData {
  std::string name;
  double mass = 0;
  double distance_from_origin = 0;
  double velocity_magnitude = 0;
};

struct AnalysisEngine::Impl {
  AnalysisConfig config;
  std::vector<std::string> loaded_bodies;
  std::vector<BodyData> body_data;
  size_t data_points = 0;
  bool ready = false;
};

AnalysisEngine::AnalysisEngine() : impl_(std::make_unique<Impl>()) {}

AnalysisEngine::AnalysisEngine(const AnalysisConfig& config) : impl_(std::make_unique<Impl>()) {
  impl_->config = config;
}

AnalysisEngine::~AnalysisEngine() = default;

void AnalysisEngine::set_config(const AnalysisConfig& config) { impl_->config = config; }

const AnalysisConfig& AnalysisEngine::config() const { return impl_->config; }

bool AnalysisEngine::load_data(const std::string& body_name) {
  Bodies::BodyFactory factory;
  auto result = factory.create_body(body_name);
  if (!result.has_value()) {
    return false;
  }

  const auto& body = result.value();
  BodyData bd;
  bd.name = body_name;
  bd.mass = static_cast<double>(body.mass());
  bd.distance_from_origin = body.position().magnitude();
  bd.velocity_magnitude = body.velocity().magnitude();

  impl_->loaded_bodies.push_back(body_name);
  impl_->body_data.push_back(bd);
  impl_->data_points += 3;
  impl_->ready = true;
  return true;
}

bool AnalysisEngine::load_data(const std::vector<std::string>& body_names) {
  for (const auto& name : body_names) {
    if (!load_data(name)) {
      return false;
    }
  }
  return true;
}

size_t AnalysisEngine::data_point_count() const { return impl_->data_points; }

AnalysisResult AnalysisEngine::analyze() {
  AnalysisResult result;
  if (!impl_->ready || impl_->body_data.empty()) {
    result.success = false;
    result.message = "No data loaded";
    return result;
  }

  const auto& data = impl_->body_data;
  size_t n = data.size();

  double total_mass = 0;
  double min_dist = data[0].distance_from_origin;
  double max_dist = data[0].distance_from_origin;
  double sum_dist = 0;
  double min_vel = data[0].velocity_magnitude;
  double max_vel = data[0].velocity_magnitude;
  double sum_vel = 0;

  for (const auto& bd : data) {
    total_mass += bd.mass;
    min_dist = std::min(min_dist, bd.distance_from_origin);
    max_dist = std::max(max_dist, bd.distance_from_origin);
    sum_dist += bd.distance_from_origin;
    min_vel = std::min(min_vel, bd.velocity_magnitude);
    max_vel = std::max(max_vel, bd.velocity_magnitude);
    sum_vel += bd.velocity_magnitude;
  }

  double mean_dist = sum_dist / static_cast<double>(n);
  double mean_vel = sum_vel / static_cast<double>(n);

  std::ostringstream oss;
  oss << "Analysis complete for " << n << " bodies\n"
      << "Total mass: " << total_mass << " kg\n"
      << "Distance from origin - min: " << min_dist << " max: " << max_dist
      << " mean: " << mean_dist << " m\n"
      << "Velocity magnitude - min: " << min_vel << " max: " << max_vel << " mean: " << mean_vel
      << " m/s";

  result.success = true;
  result.message = oss.str();
  result.values = {total_mass, min_dist, max_dist, mean_dist, min_vel, max_vel, mean_vel};
  return result;
}

bool AnalysisEngine::is_ready() const { return impl_->ready; }

}  // namespace SolarSystem::Analysis
