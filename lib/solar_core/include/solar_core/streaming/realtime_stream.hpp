#pragma once

#include <memory>

#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_core/streaming/data_stream.hpp"

namespace SolarSystem::Streaming {

/**
 * @brief Real-time data stream that generates live celestial body data
 *
 * This stream uses the simulation engine to generate real-time positions
 * and velocities for celestial bodies, providing a continuous stream of
 * current solar system state.
 */
class RealtimeStream : public DataStream {
public:
  /**
   * @brief Configuration specific to real-time streaming
   */
  struct RealtimeConfig {
    // Simulation parameters
    Simulation::SimulationEngine::IntegrationMethod integration_method =
        Simulation::SimulationEngine::IntegrationMethod::LEAPFROG;
    double simulation_timestep = 30.0;  // seconds
    bool use_adaptive_timestep = false;

    // Data source preferences
    Bodies::BodyFactory::DataSource preferred_data_source =
        Bodies::BodyFactory::DataSource::JPL_HORIZONS;
    bool allow_fallback = true;
    Bodies::BodyFactory::DefaultBodySet default_body_set =
        Bodies::BodyFactory::DefaultBodySet::IMPORTANT;

    // Real-time specific options
    bool sync_with_system_time = true;  // Sync simulation time with real time
    std::chrono::milliseconds time_sync_tolerance{1000};  // Acceptable time drift
    bool auto_correct_drift = true;     // Automatically correct time drift

    // Performance options
    bool enable_prediction = true;      // Predict positions between updates
    size_t prediction_steps = 5;       // Number of prediction steps
    bool cache_intermediate_states = true;
  };

  explicit RealtimeStream(StreamConfig stream_config = {});
  explicit RealtimeStream(StreamConfig stream_config, RealtimeConfig realtime_config);
  ~RealtimeStream() override = default;

  // Configuration
  void set_realtime_config(const RealtimeConfig& config) { realtime_config_ = config; }
  [[nodiscard]] const RealtimeConfig& get_realtime_config() const noexcept { return realtime_config_; }

  // Data source management
  [[nodiscard]] Utils::Expected<void, std::string> set_data_source(
      std::shared_ptr<Bodies::BodyCollection> bodies) override;

  // Time synchronization
  [[nodiscard]] Utils::Expected<void, std::string> sync_to_current_time();
  [[nodiscard]] std::chrono::system_clock::time_point get_simulation_time() const;
  [[nodiscard]] std::chrono::milliseconds get_time_drift() const;

  // Simulation control
  [[nodiscard]] Utils::Expected<void, std::string> reset_simulation();
  [[nodiscard]] Utils::Expected<void, std::string> advance_simulation_to(
      std::chrono::system_clock::time_point target_time);

protected:
  // DataStream interface implementation
  [[nodiscard]] Utils::Expected<DataSnapshot, std::string> generate_snapshot() override;
  [[nodiscard]] Utils::Expected<void, std::string> initialize_stream() override;
  [[nodiscard]] Utils::Expected<void, std::string> cleanup_stream() override;

private:
  RealtimeConfig realtime_config_;

  // Core components
  std::unique_ptr<Bodies::BodyFactory> body_factory_;
  std::unique_ptr<Simulation::SimulationEngine> simulation_engine_;
  std::shared_ptr<Bodies::BodyCollection> current_bodies_;

  // Time tracking
  std::chrono::system_clock::time_point simulation_start_time_;
  std::chrono::system_clock::time_point last_sync_time_;
  double simulation_time_offset_ = 0.0;  // seconds

  // Prediction cache
  struct PredictionState {
    std::chrono::system_clock::time_point timestamp;
    Bodies::BodyCollection predicted_bodies;
    double confidence = 1.0;
  };
  std::vector<PredictionState> prediction_cache_;
  std::mutex prediction_mutex_;

  // Helper methods
  [[nodiscard]] Utils::Expected<void, std::string> initialize_simulation();
  [[nodiscard]] Utils::Expected<void, std::string> update_simulation_to_current_time();
  [[nodiscard]] Utils::Expected<Bodies::BodyCollection, std::string> get_predicted_state(
      std::chrono::system_clock::time_point target_time);

  void update_prediction_cache();
  void clear_prediction_cache();

  [[nodiscard]] double calculate_quality_score(const Bodies::BodyCollection& bodies) const;
  [[nodiscard]] std::chrono::milliseconds calculate_processing_latency() const;
};

}  // namespace SolarSystem::Streaming
