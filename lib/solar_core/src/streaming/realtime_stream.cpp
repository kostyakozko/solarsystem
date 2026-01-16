#include "solar_core/streaming/realtime_stream.hpp"

#include <algorithm>
#include <cmath>

#include "solar_utils/logging.hpp"

// Helper function to create success Expected
template <typename T>
SolarSystem::Utils::Expected<T, std::string> make_success() {
  if constexpr (std::is_void_v<T>) {
    return SolarSystem::Utils::Expected<void, std::string>();
  } else {
    return SolarSystem::Utils::Expected<T, std::string>(T{});
  }
}

// Helper function to create error Expected
template <typename T>
SolarSystem::Utils::Expected<T, std::string> make_error(const std::string& error) {
  return SolarSystem::Utils::Expected<T, std::string>(error);
}

namespace SolarSystem::Streaming {

using namespace SolarSystem::Utils;

RealtimeStream::RealtimeStream(StreamConfig stream_config) : DataStream(std::move(stream_config)) {
  body_factory_ = std::make_unique<Bodies::BodyFactory>();
  simulation_engine_ = std::make_unique<Simulation::SimulationEngine>();
}

RealtimeStream::RealtimeStream(StreamConfig stream_config, RealtimeConfig realtime_config)
    : DataStream(std::move(stream_config)), realtime_config_(std::move(realtime_config)) {
  body_factory_ = std::make_unique<Bodies::BodyFactory>();
  simulation_engine_ = std::make_unique<Simulation::SimulationEngine>();
}

Utils::Expected<void, std::string> RealtimeStream::set_data_source(
    std::shared_ptr<Bodies::BodyCollection> bodies) {
  if (!bodies) {
    return Utils::Expected<void, std::string>("Bodies collection is null");
  }

  current_bodies_ = bodies;

  // Initialize simulation with the new bodies
  if (is_running()) {
    return initialize_simulation();
  }

  return Utils::Expected<void, std::string>();
}

Utils::Expected<void, std::string> RealtimeStream::sync_to_current_time() {
  auto now = std::chrono::system_clock::now();

  if (!simulation_engine_->is_initialized()) {
    return Utils::Expected<void, std::string>("Simulation engine not initialized");
  }

  // Calculate time difference
  auto current_sim_time = get_simulation_time();
  auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - current_sim_time);

  if (std::abs(time_diff.count()) > realtime_config_.time_sync_tolerance.count() / 1000) {
    if (realtime_config_.auto_correct_drift) {
      LOG_INFO("RealtimeStream",
               "Correcting time drift: " + std::to_string(time_diff.count()) + "s");

      // Advance simulation to current time
      auto advance_result = advance_simulation_to(now);
      if (!advance_result) {
        return advance_result;
      }

      last_sync_time_ = now;
    } else {
      LOG_WARN("RealtimeStream", "Time drift detected but auto-correction disabled: " +
                                     std::to_string(time_diff.count()) + "s");
    }
  }

  return make_success<void>();
}

std::chrono::system_clock::time_point RealtimeStream::get_simulation_time() const {
  if (!simulation_engine_->is_initialized()) {
    return std::chrono::system_clock::now();
  }

  return simulation_engine_->get_current_date();
}

std::chrono::milliseconds RealtimeStream::get_time_drift() const {
  auto now = std::chrono::system_clock::now();
  auto sim_time = get_simulation_time();

  return std::chrono::duration_cast<std::chrono::milliseconds>(now - sim_time);
}

Utils::Expected<void, std::string> RealtimeStream::reset_simulation() {
  if (!current_bodies_) {
    return make_error<void>("No bodies available for simulation");
  }

  // Reset simulation engine
  simulation_engine_->reset();

  // Reinitialize with current time
  auto now = std::chrono::system_clock::now();
  auto init_result = simulation_engine_->initialize(*current_bodies_, now);
  if (!init_result) {
    return make_error<void>("Failed to initialize simulation: " + init_result.error());
  }

  simulation_start_time_ = now;
  last_sync_time_ = now;
  simulation_time_offset_ = 0.0;

  // Clear prediction cache
  clear_prediction_cache();

  LOG_INFO("RealtimeStream", "Simulation reset successfully");
  return make_success<void>();
}

Utils::Expected<void, std::string> RealtimeStream::advance_simulation_to(
    std::chrono::system_clock::time_point target_time) {
  if (!simulation_engine_->is_initialized()) {
    return make_error<void>("Simulation engine not initialized");
  }

  auto current_time = get_simulation_time();
  if (target_time <= current_time) {
    return make_success<void>();  // Already at or past target time
  }

  // Calculate time difference in seconds
  auto time_diff =
      std::chrono::duration_cast<std::chrono::duration<double>>(target_time - current_time);
  double duration_seconds = time_diff.count();

  // Advance simulation
  auto advance_result = simulation_engine_->simulate_duration(duration_seconds);
  if (!advance_result) {
    return make_error<void>("Failed to advance simulation: " + advance_result.error());
  }

  // Update prediction cache
  if (realtime_config_.cache_intermediate_states) {
    update_prediction_cache();
  }

  return make_success<void>();
}

Utils::Expected<DataSnapshot, std::string> RealtimeStream::generate_snapshot() {
  auto start_time = std::chrono::steady_clock::now();

  if (!simulation_engine_->is_initialized()) {
    return Utils::Expected<DataSnapshot, std::string>("Simulation engine not initialized");
  }

  // Sync to current time if enabled
  if (realtime_config_.sync_with_system_time) {
    auto sync_result = update_simulation_to_current_time();
    if (!sync_result) {
      LOG_WARN("RealtimeStream", "Failed to sync simulation time: " + sync_result.error());
    }
  }

  // Get current bodies from simulation
  const auto& bodies = simulation_engine_->get_bodies();

  // Create snapshot
  auto now = std::chrono::system_clock::now();
  DataSnapshot snapshot(bodies, now);

  // Calculate quality score
  double quality_score = calculate_quality_score(bodies);
  snapshot.overall_quality = quality_score;

  // Calculate processing latency
  auto processing_latency = calculate_processing_latency();
  snapshot.processing_time = processing_latency;

  // Add metadata
  for (auto& point : snapshot.data_points) {
    point.data_source = "realtime_simulation";
    point.latency = processing_latency;
    point.quality_score = quality_score;
  }

  auto end_time = std::chrono::steady_clock::now();
  auto generation_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  snapshot.processing_time = generation_time;

  return Utils::Expected<DataSnapshot, std::string>(std::move(snapshot));
}

Utils::Expected<void, std::string> RealtimeStream::initialize_stream() {
  if (!current_bodies_) {
    return make_error<void>("No bodies available for streaming");
  }

  // Initialize simulation
  auto init_result = initialize_simulation();
  if (!init_result) {
    return init_result;
  }

  LOG_INFO("RealtimeStream", "Real-time stream initialized successfully");
  return make_success<void>();
}

Utils::Expected<void, std::string> RealtimeStream::cleanup_stream() {
  // Clear prediction cache
  clear_prediction_cache();

  // Reset simulation
  if (simulation_engine_) {
    simulation_engine_->reset();
  }

  LOG_INFO("RealtimeStream", "Real-time stream cleaned up successfully");
  return make_success<void>();
}

Utils::Expected<void, std::string> RealtimeStream::initialize_simulation() {
  if (!current_bodies_) {
    return make_error<void>("No bodies available for simulation");
  }

  // Configure simulation engine
  Simulation::SimulationConfig sim_config;
  sim_config.time_step = realtime_config_.simulation_timestep;
  sim_config.use_adaptive_timestep = realtime_config_.use_adaptive_timestep;
  simulation_engine_->set_config(sim_config);
  simulation_engine_->set_integration_method(realtime_config_.integration_method);

  // Initialize with current time
  auto now = std::chrono::system_clock::now();
  auto init_result = simulation_engine_->initialize(*current_bodies_, now);
  if (!init_result) {
    return make_error<void>("Failed to initialize simulation: " + init_result.error());
  }

  simulation_start_time_ = now;
  last_sync_time_ = now;
  simulation_time_offset_ = 0.0;

  LOG_INFO("RealtimeStream",
           "Simulation initialized with " + std::to_string(current_bodies_->size()) + " bodies");

  return make_success<void>();
}

Utils::Expected<void, std::string> RealtimeStream::update_simulation_to_current_time() {
  auto now = std::chrono::system_clock::now();

  // Check if we need to advance simulation
  auto current_sim_time = get_simulation_time();
  auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - current_sim_time);

  if (time_diff.count() > 0) {
    // Advance simulation to current time
    auto advance_result = advance_simulation_to(now);
    if (!advance_result) {
      return advance_result;
    }
  }

  return make_success<void>();
}

Utils::Expected<Bodies::BodyCollection, std::string> RealtimeStream::get_predicted_state(
    std::chrono::system_clock::time_point target_time) {
  if (!realtime_config_.enable_prediction) {
    return Utils::Expected<Bodies::BodyCollection, std::string>("Prediction disabled");
  }

  std::lock_guard<std::mutex> lock(prediction_mutex_);

  // Look for cached prediction
  for (const auto& prediction : prediction_cache_) {
    auto duration_diff = target_time - prediction.timestamp;
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        duration_diff.count() >= 0 ? duration_diff : -duration_diff);

    if (time_diff < std::chrono::milliseconds{100}) {  // Within 100ms tolerance
      return Utils::Expected<Bodies::BodyCollection, std::string>(prediction.predicted_bodies);
    }
  }

  // No suitable cached prediction found
  return Utils::Expected<Bodies::BodyCollection, std::string>("No suitable prediction available");
}

void RealtimeStream::update_prediction_cache() {
  if (!realtime_config_.enable_prediction || !simulation_engine_->is_initialized()) {
    return;
  }

  std::lock_guard<std::mutex> lock(prediction_mutex_);

  // Clear old predictions
  auto now = std::chrono::system_clock::now();
  prediction_cache_.erase(
      std::remove_if(
          prediction_cache_.begin(), prediction_cache_.end(),
          [now](const PredictionState& pred) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - pred.timestamp);
            return age > std::chrono::seconds{30};  // Remove predictions older than 30 seconds
          }),
      prediction_cache_.end());

  // Add current state as a prediction
  PredictionState current_state;
  current_state.timestamp = now;
  current_state.predicted_bodies = simulation_engine_->get_bodies();
  current_state.confidence = 1.0;

  prediction_cache_.push_back(current_state);

  // Limit cache size
  if (prediction_cache_.size() > realtime_config_.prediction_steps) {
    prediction_cache_.erase(prediction_cache_.begin());
  }
}

void RealtimeStream::clear_prediction_cache() {
  std::lock_guard<std::mutex> lock(prediction_mutex_);
  prediction_cache_.clear();
}

double RealtimeStream::calculate_quality_score(const Bodies::BodyCollection& bodies) const {
  if (bodies.empty()) {
    return 0.0;
  }

  double total_quality = 0.0;
  size_t valid_bodies = 0;

  for (const auto& body : bodies) {
    double body_quality = 1.0;

    // Check for invalid positions
    const auto& pos = body.position();
    if (std::isnan(pos.x()) || std::isnan(pos.y()) || std::isnan(pos.z()) || std::isinf(pos.x()) ||
        std::isinf(pos.y()) || std::isinf(pos.z())) {
      body_quality *= 0.1;
    }

    // Check for invalid velocities
    const auto& vel = body.velocity();
    if (std::isnan(vel.x()) || std::isnan(vel.y()) || std::isnan(vel.z()) || std::isinf(vel.x()) ||
        std::isinf(vel.y()) || std::isinf(vel.z())) {
      body_quality *= 0.1;
    }

    // Check for reasonable mass
    if (body.mass() <= 0.0 || std::isnan(body.mass()) || std::isinf(body.mass())) {
      body_quality *= 0.5;
    }

    // Check time drift
    auto time_drift = get_time_drift();
    if (time_drift > realtime_config_.time_sync_tolerance) {
      body_quality *= 0.8;
    }

    total_quality += body_quality;
    valid_bodies++;
  }

  return valid_bodies > 0 ? total_quality / static_cast<double>(valid_bodies) : 0.0;
}

std::chrono::milliseconds RealtimeStream::calculate_processing_latency() const {
  // Calculate latency based on time drift and processing overhead
  auto time_drift = get_time_drift();

  // Add estimated processing overhead
  auto processing_overhead = std::chrono::milliseconds{10};  // Estimated 10ms overhead

  auto abs_drift = time_drift.count() >= 0 ? time_drift : -time_drift;
  return abs_drift + processing_overhead;
}

}  // namespace SolarSystem::Streaming
