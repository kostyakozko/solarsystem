#pragma once

/**
 * @file jpl_integration.hpp
 * @brief Integration with existing JPL data sources and BodyFactory
 */

#include <memory>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>
#include <solar_core/bodies/body_factory.hpp>
#include <solar_jpl/jpl_client.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Integration adapter for JPL data sources
 */
class SOLAR_ANALYSIS_API JPLIntegration {
 public:
  JPLIntegration();
  ~JPLIntegration();

  JPLIntegration(const JPLIntegration&) = delete;
  JPLIntegration& operator=(const JPLIntegration&) = delete;

  // Load data from BodyFactory
  [[nodiscard]] bool load_from_body_factory(const std::string& body_name);
  [[nodiscard]] bool load_all_available();

  // Convert JPL data to analysis StateVectors
  [[nodiscard]] std::vector<StateVector> get_state_vectors(const std::string& body_name) const;

  // Get available bodies
  [[nodiscard]] std::vector<std::string> available_bodies() const;
  [[nodiscard]] bool has_body(const std::string& name) const;

  // Data source info
  [[nodiscard]] std::string data_source() const;
  [[nodiscard]] bool is_initialized() const;

  // Direct access to underlying systems
  [[nodiscard]] Bodies::BodyFactory& body_factory();
  [[nodiscard]] const Bodies::BodyFactory& body_factory() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Enhanced DataProcessor with JPL integration
 */
class SOLAR_ANALYSIS_API JPLDataProcessor : public DataProcessor {
 public:
  JPLDataProcessor();
  ~JPLDataProcessor();

  // Load from JPL/BodyFactory instead of synthetic data
  [[nodiscard]] bool load_from_jpl(const std::string& body_name);
  [[nodiscard]] bool load_from_jpl(const std::vector<std::string>& body_names);

  // Batch load all essential bodies
  [[nodiscard]] bool load_essential_bodies();
  [[nodiscard]] bool load_all_bodies();

  // Access JPL integration
  [[nodiscard]] JPLIntegration& jpl();
  [[nodiscard]] const JPLIntegration& jpl() const;

 private:
  std::unique_ptr<JPLIntegration> jpl_integration_;
};

}  // namespace SolarSystem::Analysis
