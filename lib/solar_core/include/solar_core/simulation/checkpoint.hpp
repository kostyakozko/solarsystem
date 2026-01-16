#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/export.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_utils/expected.hpp"

namespace SolarSystem::Simulation {

/**
 * @brief Configuration for checkpoint operations
 */
struct CheckpointConfig {
  std::filesystem::path checkpoint_directory = "checkpoints";
  std::string checkpoint_prefix = "simulation";
  std::chrono::seconds checkpoint_interval{3600};  // Default: every hour
  bool enable_compression = true;
  bool enable_validation = true;
  size_t max_checkpoints = 10;  // Keep last 10 checkpoints
  bool auto_cleanup = true;
};

/**
 * @brief Metadata for a simulation checkpoint
 */
struct CheckpointMetadata {
  std::string checkpoint_id;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point simulation_time;
  double simulation_seconds;
  size_t iteration_count;
  size_t body_count;
  std::string integration_method;
  double time_step;
  long double total_energy;
  std::string checksum;
  size_t compressed_size;
  size_t uncompressed_size;
  std::string version = "1.0";
};

/**
 * @brief Complete simulation state for checkpointing
 */
struct CheckpointData {
  CheckpointMetadata metadata;
  SimulationConfig simulation_config;
  SimulationState simulation_state;
  Bodies::BodyCollection bodies;
  SimulationEngine::IntegrationMethod integration_method;
};

/**
 * @brief Result of checkpoint operations
 */
enum class CheckpointResult {
  Success,
  FileError,
  CompressionError,
  ValidationError,
  CorruptedData,
  IncompatibleVersion,
  InsufficientSpace,
  PermissionDenied
};

/**
 * @brief Checkpoint operation statistics
 */
struct CheckpointStats {
  std::chrono::milliseconds save_time{0};
  std::chrono::milliseconds load_time{0};
  size_t bytes_written = 0;
  size_t bytes_read = 0;
  double compression_ratio = 1.0;
  bool validation_passed = false;
};

/**
 * @brief Checkpoint manager for simulation state persistence
 */
class SOLAR_CORE_API CheckpointManager {
 public:
  explicit CheckpointManager(CheckpointConfig config = {});

  // Configuration
  void set_config(const CheckpointConfig& config) { config_ = config; }
  [[nodiscard]] const CheckpointConfig& get_config() const noexcept { return config_; }

  // Checkpoint operations
  [[nodiscard]] Utils::Expected<std::string, CheckpointResult> save_checkpoint(
      const SimulationEngine& engine, const std::string& checkpoint_id = "");

  [[nodiscard]] Utils::Expected<CheckpointData, CheckpointResult> load_checkpoint(
      const std::string& checkpoint_id) const;

  [[nodiscard]] Utils::Expected<void, CheckpointResult> resume_simulation(
      SimulationEngine& engine, const std::string& checkpoint_id);

  // Checkpoint management
  [[nodiscard]] std::vector<CheckpointMetadata> list_checkpoints() const;
  [[nodiscard]] Utils::Expected<void, CheckpointResult> delete_checkpoint(
      const std::string& checkpoint_id);
  [[nodiscard]] Utils::Expected<void, CheckpointResult> cleanup_old_checkpoints();

  // Validation
  [[nodiscard]] Utils::Expected<bool, CheckpointResult> validate_checkpoint(
      const std::string& checkpoint_id);
  [[nodiscard]] Utils::Expected<CheckpointMetadata, CheckpointResult> get_checkpoint_metadata(
      const std::string& checkpoint_id) const;

  // Statistics
  [[nodiscard]] const CheckpointStats& get_last_stats() const noexcept { return last_stats_; }
  [[nodiscard]] std::string get_status_summary() const;

  // Utility
  [[nodiscard]] std::filesystem::path get_checkpoint_path(const std::string& checkpoint_id) const;
  [[nodiscard]] std::string generate_checkpoint_id() const;
  [[nodiscard]] bool checkpoint_exists(const std::string& checkpoint_id) const;

 private:
  CheckpointConfig config_;
  mutable CheckpointStats last_stats_;

  // Serialization
  [[nodiscard]] Utils::Expected<std::vector<uint8_t>, CheckpointResult> serialize_checkpoint(
      const CheckpointData& data) const;
  [[nodiscard]] Utils::Expected<CheckpointData, CheckpointResult> deserialize_checkpoint(
      const std::vector<uint8_t>& data) const;

  // Compression
  [[nodiscard]] Utils::Expected<std::vector<uint8_t>, CheckpointResult> compress_data(
      const std::vector<uint8_t>& data) const;
  [[nodiscard]] Utils::Expected<std::vector<uint8_t>, CheckpointResult> decompress_data(
      const std::vector<uint8_t>& compressed_data) const;

  // Validation
  [[nodiscard]] std::string calculate_checksum(const std::vector<uint8_t>& data) const;
  [[nodiscard]] bool verify_checksum(const std::vector<uint8_t>& data,
                                     const std::string& expected_checksum) const;

  // File operations
  [[nodiscard]] Utils::Expected<void, CheckpointResult> ensure_checkpoint_directory() const;
  [[nodiscard]] Utils::Expected<void, CheckpointResult> write_checkpoint_file(
      const std::string& checkpoint_id, const std::vector<uint8_t>& data,
      const std::string& checksum = "") const;
  [[nodiscard]] Utils::Expected<std::vector<uint8_t>, CheckpointResult> read_checkpoint_file(
      const std::string& checkpoint_id) const;
};

/**
 * @brief Automatic checkpoint scheduler for long-running simulations
 */
class SOLAR_CORE_API CheckpointScheduler {
 public:
  explicit CheckpointScheduler(CheckpointManager& manager, CheckpointConfig config = {});

  // Scheduling control
  void start_scheduling();
  void stop_scheduling();
  [[nodiscard]] bool is_scheduling() const noexcept { return scheduling_active_; }

  // Manual checkpoint triggers
  [[nodiscard]] bool should_checkpoint(const SimulationState& state) const;
  [[nodiscard]] Utils::Expected<std::string, CheckpointResult> trigger_checkpoint(
      const SimulationEngine& engine);

  // Configuration
  void set_checkpoint_interval(std::chrono::seconds interval) {
    config_.checkpoint_interval = interval;
  }
  [[nodiscard]] std::chrono::seconds get_checkpoint_interval() const noexcept {
    return config_.checkpoint_interval;
  }

 private:
  CheckpointManager& manager_;
  CheckpointConfig config_;
  bool scheduling_active_ = false;
  std::chrono::system_clock::time_point last_checkpoint_time_;
  size_t last_checkpoint_iteration_ = 0;
};

// Utility functions
[[nodiscard]] SOLAR_CORE_API std::string to_string(CheckpointResult result);
[[nodiscard]] CheckpointResult checkpoint_result_from_string(std::string_view str);

}  // namespace SolarSystem::Simulation
