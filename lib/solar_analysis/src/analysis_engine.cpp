/**
 * @file analysis_engine.cpp
 * @brief Implementation of the core analysis engine
 */

#include "solar_analysis/analysis_engine.hpp"

#include <iostream>

namespace SolarSystem::Analysis {

struct AnalysisEngine::Impl {
  AnalysisConfig config;
  std::vector<std::string> loaded_bodies;
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
  impl_->loaded_bodies.push_back(body_name);
  impl_->data_points += 100;  // Placeholder
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
  if (!impl_->ready) {
    result.success = false;
    result.message = "No data loaded";
    return result;
  }

  result.success = true;
  result.message =
      "Analysis complete for " + std::to_string(impl_->loaded_bodies.size()) + " bodies";
  return result;
}

bool AnalysisEngine::is_ready() const { return impl_->ready; }

}  // namespace SolarSystem::Analysis
