#pragma once

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/data/body_definitions.hpp"
#include "solar_core/utils/expected.hpp"
#include "solar_jpl/jpl_client.hpp"

namespace SolarSystem::Bodies {

/**
 * @brief Factory for creating celestial bodies from various data sources
 *
 * This class integrates with the existing JPL data system and provides
 * a modern C++ interface for creating bodies from:
 * - JPL HORIZONS data (primary)
 * - Cached ephemeris data
 * - Hardcoded fallback data (from constants.cpp)
 */

class BodyFactory {
 public:
  enum class DataSource {
    JPL_HORIZONS,  // Fetch from JPL API
    CACHED_DATA,   // Use cached ephemeris data
    FALLBACK_DATA  // Use hardcoded constants
  };

  // Enhanced fallback strategies
  enum class FallbackStrategy {
    STRICT,           // No fallback, fail if preferred source unavailable
    GRACEFUL,         // Try all sources in order, use best available
    INTELLIGENT,      // Assess data quality and choose best source
    PARTIAL_ALLOWED,  // Allow partial data with warnings
    HYBRID            // Combine data from multiple sources
  };

  enum class DataQuality {
    EXCELLENT,   // Recent JPL data with full validation
    GOOD,        // Cached JPL data or recent fallback
    ACCEPTABLE,  // Older cached data or basic fallback
    POOR,        // Very old or incomplete data
    UNKNOWN      // Quality cannot be assessed
  };

  struct CreationOptions {
    DataSource preferred_source = DataSource::JPL_HORIZONS;
    std::chrono::system_clock::time_point reference_time = std::chrono::system_clock::now();
    bool allow_fallback = true;
    bool validate_data = true;
    FallbackStrategy fallback_strategy = FallbackStrategy::GRACEFUL;
    DataQuality minimum_quality = DataQuality::ACCEPTABLE;
    bool allow_partial_data = false;
    bool prefer_recent_data = true;
    std::chrono::hours max_data_age{24 * 30};  // 30 days default
  };

  // Data quality assessment
  struct DataSourceInfo {
    DataSource source;
    DataQuality quality;
    std::chrono::system_clock::time_point last_updated;
    bool is_complete;
    std::vector<std::string> missing_fields;
    std::string quality_reason;
  };

  struct FallbackResult {
    bool success;
    DataSource source_used;
    DataQuality data_quality;
    std::vector<std::string> warnings;
    std::vector<std::string> fallback_chain;
    std::chrono::milliseconds total_time;
  };

  // Constructor
  BodyFactory() : current_source_("UNINITIALIZED"), data_initialized_(false) {
    initialize_internal_data();
  }
  explicit BodyFactory(CreationOptions options);

  // Single body creation
  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_body(
      std::string_view name) const;
  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_body(
      std::string_view name, const CreationOptions& options) const;

  // Bulk body creation
  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_collection(
      const std::vector<std::string>& body_names) const;
  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_collection(
      const std::vector<std::string>& body_names, const CreationOptions& options) const;

  // Predefined collections (using existing JPL/cache system)
  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_solar_system() const;
  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_solar_system(
      const CreationOptions& options) const;

  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_inner_planets() const;
  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_inner_planets(
      const CreationOptions& options) const;

  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_essential_bodies() const;
  [[nodiscard]] Utils::Expected<BodyCollection, std::string> create_essential_bodies(
      const CreationOptions& options) const;

  // Integration with existing system
  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_from_legacy_data(
      std::string_view name) const;

  // Utility functions
  [[nodiscard]] std::vector<std::string> get_available_bodies() const;
  [[nodiscard]] bool is_body_available(std::string_view name,
                                       std::chrono::system_clock::time_point time) const;

  [[nodiscard]] bool has_current_ephemeris_data() const noexcept {
    return current_source_ != "ORIGINAL_DATA";
  }

  [[nodiscard]] std::chrono::system_clock::time_point current_epoch() const {
    return current_epoch_;
  }
  [[nodiscard]] const std::string& current_source() const { return current_source_; }
  [[nodiscard]] bool is_initialized() const noexcept { return data_initialized_; }

  [[nodiscard]] bool has_current_year_ephemeris_data() const noexcept;

  [[nodiscard]] SolarSystem::Utils::Expected<void, std::string> rebuild_cache();

  [[nodiscard]] SolarSystem::Utils::Expected<void, std::string> fetch_current_ephemeris_data(
      std::chrono::system_clock::time_point time = get_current_year_epoch());

  [[nodiscard]] SolarSystem::Utils::Expected<void, std::string> test_storage_system();

  [[nodiscard]] static std::chrono::system_clock::time_point get_current_year_epoch() noexcept;

  // Intelligent fallback strategies
  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_body_with_intelligent_fallback(
      std::string_view name, const CreationOptions& options) const;

  [[nodiscard]] std::vector<DataSourceInfo> assess_data_sources(
      std::string_view name, const CreationOptions& options) const;

  [[nodiscard]] DataQuality assess_data_quality(
      const CelestialBody& body, DataSource source,
      std::chrono::system_clock::time_point reference_time) const;

  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_with_hybrid_approach(
      std::string_view name, const CreationOptions& options) const;

  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_with_partial_data(
      std::string_view name, const CreationOptions& options) const;

  [[nodiscard]] FallbackResult execute_fallback_strategy(std::string_view name,
                                                         const CreationOptions& options) const;

  // Data source prioritization
  [[nodiscard]] std::vector<DataSource> get_prioritized_sources(
      std::string_view name, const CreationOptions& options) const;

  [[nodiscard]] bool is_data_source_available(DataSource source, std::string_view name) const;

  [[nodiscard]] std::chrono::system_clock::time_point get_data_source_timestamp(
      DataSource source, std::string_view name) const;

 private:
  // Helper methods for intelligent fallback strategies
  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_with_graceful_fallback(
      std::string_view name, const CreationOptions& options) const;

  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_with_quality_assessment(
      std::string_view name, const CreationOptions& options) const;

 private:
  CreationOptions default_options_;
  std::unique_ptr<SolarSystem::JPL::JPLClient> jpl_client_;
  std::chrono::system_clock::time_point current_epoch_;
  std::string current_source_;
  bool data_initialized_;
  std::vector<SolarSystem::JPL::EphemerisData> cached_ephemeris_;

  // Integration with existing JPL system
  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_from_jpl(
      std::string_view name, std::chrono::system_clock::time_point time) const;

  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_from_cache(
      std::string_view name, std::chrono::system_clock::time_point time) const;

  [[nodiscard]] Utils::Expected<CelestialBody, std::string> create_from_fallback(
      std::string_view name) const;

  // Helper functions
  [[nodiscard]] BodyType determine_body_type(std::string_view name) const;
  [[nodiscard]] BodyPriority determine_body_priority(std::string_view name) const;
  [[nodiscard]] std::optional<int> get_jpl_id(std::string_view name) const;

  void initialize_internal_data();

  // Comprehensive validation methods
  [[nodiscard]] Utils::Expected<void, std::string> validate_physical_properties(
      const CelestialBody::Properties& props) const;
  [[nodiscard]] Utils::Expected<void, std::string> validate_mass_bounds(
      long double mass, BodyType type, std::string_view name) const;
  [[nodiscard]] Utils::Expected<void, std::string> validate_orbital_parameters(
      const Math::Vector3d& position, const Math::Vector3d& velocity, BodyType type,
      std::string_view name) const;
  [[nodiscard]] Utils::Expected<void, std::string> validate_cross_properties(
      const CelestialBody::Properties& props) const;
  [[nodiscard]] Utils::Expected<void, std::string> validate_body_relationships(
      const CelestialBody::Properties& props,
      const std::vector<CelestialBody>& existing_bodies) const;
};

}  // namespace SolarSystem::Bodies
