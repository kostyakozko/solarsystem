#include "solar_core/simulation/checkpoint.hpp"

#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace SolarSystem::Simulation {

CheckpointManager::CheckpointManager(CheckpointConfig config) : config_(std::move(config)) {
  // Ensure checkpoint directory exists
  auto result = ensure_checkpoint_directory();
  if (!result.has_value()) {
    // Log warning but don't fail construction
  }
}

Utils::Expected<std::string, CheckpointResult> CheckpointManager::save_checkpoint(
    const SimulationEngine& engine, const std::string& checkpoint_id) {
  auto start_time = std::chrono::steady_clock::now();
  last_stats_ = CheckpointStats{};

  // Generate checkpoint ID if not provided
  std::string id = checkpoint_id.empty() ? generate_checkpoint_id() : checkpoint_id;

  if (!engine.is_initialized()) {
    return Utils::Expected<std::string, CheckpointResult>{CheckpointResult::ValidationError};
  }

  // Create checkpoint data
  CheckpointData data;
  data.metadata.checkpoint_id = id;
  data.metadata.created_at = std::chrono::system_clock::now();
  data.metadata.simulation_time = engine.get_current_date();
  data.metadata.simulation_seconds = engine.get_current_time();
  data.metadata.iteration_count = engine.get_state().iteration_count;
  data.metadata.body_count = engine.get_bodies().size();
  data.metadata.integration_method = to_string(engine.get_integration_method());
  data.metadata.time_step = engine.get_config().time_step;
  data.metadata.total_energy = engine.get_state().total_energy;

  data.simulation_config = engine.get_config();
  data.simulation_state = engine.get_state();
  data.bodies = engine.get_bodies();
  data.integration_method = engine.get_integration_method();

  // Serialize checkpoint data
  auto serialized_result = serialize_checkpoint(data);
  if (!serialized_result.has_value()) {
    return Utils::Expected<std::string, CheckpointResult>{serialized_result.error()};
  }

  auto serialized_data = std::move(serialized_result.value());
  data.metadata.uncompressed_size = serialized_data.size();

  // Compress if enabled
  std::vector<uint8_t> final_data;
  if (config_.enable_compression) {
    auto compressed_result = compress_data(serialized_data);
    if (!compressed_result.has_value()) {
      return Utils::Expected<std::string, CheckpointResult>{compressed_result.error()};
    }
    final_data = std::move(compressed_result.value());
    data.metadata.compressed_size = final_data.size();
    last_stats_.compression_ratio = static_cast<double>(data.metadata.uncompressed_size) /
                                    static_cast<double>(data.metadata.compressed_size);
  } else {
    final_data = std::move(serialized_data);
    data.metadata.compressed_size = final_data.size();
    last_stats_.compression_ratio = 1.0;
  }

  // Calculate checksum on the data WITHOUT checksum field
  data.metadata.checksum = calculate_checksum(final_data);

  // Write to file with checksum header
  auto write_result = write_checkpoint_file(id, final_data, data.metadata.checksum);
  if (!write_result.has_value()) {
    return Utils::Expected<std::string, CheckpointResult>{write_result.error()};
  }

  // Update statistics
  auto end_time = std::chrono::steady_clock::now();
  last_stats_.save_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  last_stats_.bytes_written = final_data.size();

  // Cleanup old checkpoints if enabled
  if (config_.auto_cleanup) {
    auto cleanup_result = cleanup_old_checkpoints();
    (void)cleanup_result;  // Suppress unused result warning
  }

  return Utils::Expected<std::string, CheckpointResult>{id};
}

Utils::Expected<CheckpointData, CheckpointResult> CheckpointManager::load_checkpoint(
    const std::string& checkpoint_id) const {
  auto start_time = std::chrono::steady_clock::now();
  last_stats_ = CheckpointStats{};

  // Read checkpoint file (now includes checksum extraction)
  auto checkpoint_path = get_checkpoint_path(checkpoint_id);

  if (!std::filesystem::exists(checkpoint_path)) {
    return Utils::Expected<CheckpointData, CheckpointResult>{CheckpointResult::FileError};
  }

  std::ifstream file(checkpoint_path, std::ios::binary);
  if (!file) {
    return Utils::Expected<CheckpointData, CheckpointResult>{CheckpointResult::FileError};
  }

  // Read checksum header (17 bytes)
  char checksum_header[18] = {0};
  file.read(checksum_header, 17);
  if (!file) {
    return Utils::Expected<CheckpointData, CheckpointResult>{CheckpointResult::FileError};
  }

  std::string stored_checksum(checksum_header);
  // Trim whitespace
  stored_checksum.erase(stored_checksum.find_last_not_of(" \n\r\t") + 1);

  // Read remaining data
  file.seekg(0, std::ios::end);
  size_t file_size = static_cast<size_t>(file.tellg());
  file.seekg(17, std::ios::beg);  // Skip checksum header

  size_t data_size = file_size - 17;
  std::vector<uint8_t> file_data(data_size);
  file.read(reinterpret_cast<char*>(file_data.data()), static_cast<std::streamsize>(data_size));

  if (!file) {
    return Utils::Expected<CheckpointData, CheckpointResult>{CheckpointResult::FileError};
  }

  last_stats_.bytes_read = file_data.size();

  // Store a copy for checksum validation if needed
  std::vector<uint8_t> data_for_checksum;
  if (config_.enable_validation) {
    data_for_checksum = file_data;
  }

  // Decompress if needed
  std::vector<uint8_t> serialized_data;
  if (config_.enable_compression) {
    auto decompressed_result = decompress_data(file_data);
    if (!decompressed_result.has_value()) {
      return Utils::Expected<CheckpointData, CheckpointResult>{decompressed_result.error()};
    }
    serialized_data = std::move(decompressed_result.value());
  } else {
    serialized_data = std::move(file_data);
  }

  // Deserialize checkpoint data
  auto deserialized_result = deserialize_checkpoint(serialized_data);
  if (!deserialized_result.has_value()) {
    return Utils::Expected<CheckpointData, CheckpointResult>{deserialized_result.error()};
  }

  auto checkpoint_data = std::move(deserialized_result.value());

  // Validate checksum if enabled - validate against the data as stored on disk
  if (config_.enable_validation) {
    if (!verify_checksum(data_for_checksum, stored_checksum)) {
      return Utils::Expected<CheckpointData, CheckpointResult>{CheckpointResult::CorruptedData};
    }
    last_stats_.validation_passed = true;
  }

  // Store the checksum in the metadata
  checkpoint_data.metadata.checksum = stored_checksum;

  // Update statistics
  auto end_time = std::chrono::steady_clock::now();
  last_stats_.load_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return Utils::Expected<CheckpointData, CheckpointResult>{std::move(checkpoint_data)};
}

Utils::Expected<void, CheckpointResult> CheckpointManager::resume_simulation(
    SimulationEngine& engine, const std::string& checkpoint_id) {
  // Load checkpoint data
  auto load_result = load_checkpoint(checkpoint_id);
  if (!load_result.has_value()) {
    return Utils::Expected<void, CheckpointResult>{load_result.error()};
  }

  auto checkpoint_data = std::move(load_result.value());

  // Restore simulation configuration
  engine.set_config(checkpoint_data.simulation_config);
  engine.set_integration_method(checkpoint_data.integration_method);

  // Initialize engine with restored state
  auto init_result = engine.initialize(std::move(checkpoint_data.bodies),
                                       checkpoint_data.simulation_state.reference_time);
  if (!init_result.has_value()) {
    return Utils::Expected<void, CheckpointResult>{CheckpointResult::ValidationError};
  }

  // Restore simulation state (this requires access to private members)
  // For now, we'll simulate to the correct time
  auto simulate_result = engine.simulate_to_time(checkpoint_data.simulation_state.current_time);
  if (!simulate_result.has_value()) {
    return Utils::Expected<void, CheckpointResult>{CheckpointResult::ValidationError};
  }

  return Utils::Expected<void, CheckpointResult>{};
}

std::vector<CheckpointMetadata> CheckpointManager::list_checkpoints() const {
  std::vector<CheckpointMetadata> checkpoints;

  if (!std::filesystem::exists(config_.checkpoint_directory)) {
    return checkpoints;
  }

  for (const auto& entry : std::filesystem::directory_iterator(config_.checkpoint_directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".checkpoint") {
      std::string checkpoint_id = entry.path().stem().string();
      if (checkpoint_id.starts_with(config_.checkpoint_prefix)) {
        auto metadata_result = get_checkpoint_metadata(checkpoint_id);
        if (metadata_result.has_value()) {
          checkpoints.push_back(std::move(metadata_result.value()));
        }
      }
    }
  }

  // Sort by creation time (newest first)
  std::sort(checkpoints.begin(), checkpoints.end(),
            [](const CheckpointMetadata& a, const CheckpointMetadata& b) {
              return a.created_at > b.created_at;
            });

  return checkpoints;
}

Utils::Expected<void, CheckpointResult> CheckpointManager::cleanup_old_checkpoints() {
  auto checkpoints = list_checkpoints();

  if (checkpoints.size() <= config_.max_checkpoints) {
    return Utils::Expected<void, CheckpointResult>{};
  }

  // Remove oldest checkpoints
  for (size_t i = config_.max_checkpoints; i < checkpoints.size(); ++i) {
    auto delete_result = delete_checkpoint(checkpoints[i].checkpoint_id);
    if (!delete_result.has_value()) {
      // Continue with other deletions even if one fails
    }
  }

  return Utils::Expected<void, CheckpointResult>{};
}

std::string CheckpointManager::generate_checkpoint_id() const {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream oss;
  oss << config_.checkpoint_prefix << "_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
      << "_" << std::setfill('0') << std::setw(3) << ms.count();

  return oss.str();
}

// Private implementation methods

Utils::Expected<std::vector<uint8_t>, CheckpointResult> CheckpointManager::serialize_checkpoint(
    const CheckpointData& data) const {
  std::ostringstream oss;

  // Simple binary serialization (in a real implementation, use a proper serialization library)
  // For now, use a simple text-based format for demonstration

  oss << "CHECKPOINT_VERSION:" << data.metadata.version << "\n";
  oss << "CHECKPOINT_ID:" << data.metadata.checkpoint_id << "\n";
  oss << "CREATED_AT:"
      << std::chrono::duration_cast<std::chrono::seconds>(
             data.metadata.created_at.time_since_epoch())
             .count()
      << "\n";
  oss << "SIMULATION_TIME:"
      << std::chrono::duration_cast<std::chrono::seconds>(
             data.metadata.simulation_time.time_since_epoch())
             .count()
      << "\n";
  oss << "SIMULATION_SECONDS:" << data.metadata.simulation_seconds << "\n";
  oss << "ITERATION_COUNT:" << data.metadata.iteration_count << "\n";
  oss << "BODY_COUNT:" << data.metadata.body_count << "\n";
  oss << "INTEGRATION_METHOD:" << data.metadata.integration_method << "\n";
  oss << "TIME_STEP:" << data.metadata.time_step << "\n";
  oss << "TOTAL_ENERGY:" << data.metadata.total_energy << "\n";
  // NOTE: Checksum is NOT serialized - it's calculated on the serialized data and stored separately

  // Serialize simulation config
  oss << "CONFIG_TIME_STEP:" << data.simulation_config.time_step << "\n";
  oss << "CONFIG_GRAVITATIONAL_CONSTANT:" << data.simulation_config.gravitational_constant << "\n";
  oss << "CONFIG_USE_ADAPTIVE_TIMESTEP:" << data.simulation_config.use_adaptive_timestep << "\n";

  // Serialize simulation state
  oss << "STATE_CURRENT_TIME:" << data.simulation_state.current_time << "\n";
  oss << "STATE_ITERATION_COUNT:" << data.simulation_state.iteration_count << "\n";
  oss << "STATE_TOTAL_ENERGY:" << data.simulation_state.total_energy << "\n";

  // Serialize bodies (simplified)
  oss << "BODIES_START\n";
  for (const auto& body : data.bodies) {
    oss << "BODY:" << body.name() << ":" << body.mass() << ":" << body.position().x() << ":"
        << body.position().y() << ":" << body.position().z() << ":" << body.velocity().x() << ":"
        << body.velocity().y() << ":" << body.velocity().z() << "\n";
  }
  oss << "BODIES_END\n";

  std::string serialized_str = oss.str();
  std::vector<uint8_t> result(serialized_str.begin(), serialized_str.end());

  return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{std::move(result)};
}

Utils::Expected<CheckpointData, CheckpointResult> CheckpointManager::deserialize_checkpoint(
    const std::vector<uint8_t>& data) const {
  std::string serialized_str(data.begin(), data.end());
  std::istringstream iss(serialized_str);

  CheckpointData checkpoint_data;
  std::string line;

  // Parse metadata and configuration
  while (std::getline(iss, line)) {
    if (line == "BODIES_START") {
      break;
    }

    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);

    if (key == "CHECKPOINT_VERSION") {
      checkpoint_data.metadata.version = value;
    } else if (key == "CHECKPOINT_ID") {
      checkpoint_data.metadata.checkpoint_id = value;
    } else if (key == "SIMULATION_SECONDS") {
      checkpoint_data.metadata.simulation_seconds = std::stod(value);
    } else if (key == "ITERATION_COUNT") {
      checkpoint_data.metadata.iteration_count = std::stoull(value);
    } else if (key == "INTEGRATION_METHOD") {
      checkpoint_data.metadata.integration_method = value;
      checkpoint_data.integration_method = integration_method_from_string(value);
    } else if (key == "TIME_STEP") {
      checkpoint_data.metadata.time_step = std::stod(value);
    } else if (key == "CONFIG_TIME_STEP") {
      checkpoint_data.simulation_config.time_step = std::stod(value);
    } else if (key == "CONFIG_GRAVITATIONAL_CONSTANT") {
      checkpoint_data.simulation_config.gravitational_constant = std::stod(value);
    } else if (key == "STATE_CURRENT_TIME") {
      checkpoint_data.simulation_state.current_time = std::stod(value);
    } else if (key == "STATE_ITERATION_COUNT") {
      checkpoint_data.simulation_state.iteration_count = std::stoull(value);
    }
  }

  // Parse bodies
  Bodies::BodyCollection bodies;
  while (std::getline(iss, line)) {
    if (line == "BODIES_END") {
      break;
    }

    if (line.starts_with("BODY:")) {
      // Parse body data: BODY:name:mass:px:py:pz:vx:vy:vz
      std::vector<std::string> parts;
      std::istringstream body_iss(line);
      std::string part;

      while (std::getline(body_iss, part, ':')) {
        parts.push_back(part);
      }

      if (parts.size() >= 9) {
        std::string name = parts[1];
        long double mass = std::stold(parts[2]);
        Math::Vector3d position(std::stold(parts[3]), std::stold(parts[4]), std::stold(parts[5]));
        Math::Vector3d velocity(std::stold(parts[6]), std::stold(parts[7]), std::stold(parts[8]));

        Bodies::CelestialBody::Properties props;
        props.name = name;
        props.mass = mass;
        props.position = position;
        props.velocity = velocity;
        props.type = Bodies::BodyType::Planet;             // Default type
        props.priority = Bodies::BodyPriority::Important;  // Default priority

        Bodies::CelestialBody body(props);
        bodies.add_body(std::move(body));
      }
    }
  }

  checkpoint_data.bodies = std::move(bodies);

  return Utils::Expected<CheckpointData, CheckpointResult>{std::move(checkpoint_data)};
}

Utils::Expected<std::vector<uint8_t>, CheckpointResult> CheckpointManager::compress_data(
    const std::vector<uint8_t>& data) const {
  if (!config_.enable_compression) {
    return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{data};
  }

  uLongf compressed_size = compressBound(data.size());
  std::vector<uint8_t> compressed_data(compressed_size);

  int result = compress(compressed_data.data(), &compressed_size, data.data(), data.size());

  if (result != Z_OK) {
    return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{
        CheckpointResult::CompressionError};
  }

  compressed_data.resize(compressed_size);
  return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{std::move(compressed_data)};
}

Utils::Expected<std::vector<uint8_t>, CheckpointResult> CheckpointManager::decompress_data(
    const std::vector<uint8_t>& compressed_data) const {
  // For decompression, we need to know the original size
  // In a real implementation, this would be stored in the file header
  uLongf uncompressed_size = compressed_data.size() * 4;  // Estimate
  std::vector<uint8_t> uncompressed_data(uncompressed_size);

  int result = uncompress(uncompressed_data.data(), &uncompressed_size, compressed_data.data(),
                          compressed_data.size());

  while (result == Z_BUF_ERROR) {
    // Buffer too small, increase size and try again
    uncompressed_size *= 2;
    uncompressed_data.resize(uncompressed_size);
    result = uncompress(uncompressed_data.data(), &uncompressed_size, compressed_data.data(),
                        compressed_data.size());
  }

  if (result != Z_OK) {
    return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{
        CheckpointResult::CompressionError};
  }

  uncompressed_data.resize(uncompressed_size);
  return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{std::move(uncompressed_data)};
}

std::string CheckpointManager::calculate_checksum(const std::vector<uint8_t>& data) const {
  // Simple checksum using CRC32
  uLong crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, data.data(), static_cast<uInt>(data.size()));

  std::ostringstream oss;
  oss << std::hex << crc;
  return oss.str();
}

bool CheckpointManager::verify_checksum(const std::vector<uint8_t>& data,
                                        const std::string& expected_checksum) const {
  return calculate_checksum(data) == expected_checksum;
}

Utils::Expected<void, CheckpointResult> CheckpointManager::ensure_checkpoint_directory() const {
  std::error_code ec;
  if (!std::filesystem::exists(config_.checkpoint_directory, ec)) {
    if (!std::filesystem::create_directories(config_.checkpoint_directory, ec)) {
      return Utils::Expected<void, CheckpointResult>{CheckpointResult::FileError};
    }
  }
  return Utils::Expected<void, CheckpointResult>{};
}

Utils::Expected<void, CheckpointResult> CheckpointManager::write_checkpoint_file(
    const std::string& checkpoint_id, const std::vector<uint8_t>& data,
    const std::string& checksum) const {
  auto checkpoint_path = get_checkpoint_path(checkpoint_id);

  std::ofstream file(checkpoint_path, std::ios::binary);
  if (!file) {
    return Utils::Expected<void, CheckpointResult>{CheckpointResult::FileError};
  }

  // Write checksum header (fixed size: 16 bytes for hex checksum + newline)
  std::string checksum_header = checksum + "\n";
  checksum_header.resize(17, ' ');  // Pad to 17 bytes (16 + newline)
  file.write(checksum_header.c_str(), 17);

  // Write data
  file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  if (!file) {
    return Utils::Expected<void, CheckpointResult>{CheckpointResult::FileError};
  }

  return Utils::Expected<void, CheckpointResult>{};
}

Utils::Expected<std::vector<uint8_t>, CheckpointResult> CheckpointManager::read_checkpoint_file(
    const std::string& checkpoint_id) const {
  auto checkpoint_path = get_checkpoint_path(checkpoint_id);

  if (!std::filesystem::exists(checkpoint_path)) {
    return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{CheckpointResult::FileError};
  }

  std::ifstream file(checkpoint_path, std::ios::binary);
  if (!file) {
    return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{CheckpointResult::FileError};
  }

  file.seekg(0, std::ios::end);
  size_t file_size = static_cast<size_t>(file.tellg());
  file.seekg(0, std::ios::beg);

  // Read checksum header (17 bytes)
  char checksum_header[18] = {0};
  file.read(checksum_header, 17);
  if (!file) {
    return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{CheckpointResult::FileError};
  }

  // Read remaining data
  size_t data_size = file_size - 17;
  std::vector<uint8_t> data(data_size);
  file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data_size));

  if (!file) {
    return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{CheckpointResult::FileError};
  }

  return Utils::Expected<std::vector<uint8_t>, CheckpointResult>{std::move(data)};
}

std::filesystem::path CheckpointManager::get_checkpoint_path(
    const std::string& checkpoint_id) const {
  return config_.checkpoint_directory / (checkpoint_id + ".checkpoint");
}

bool CheckpointManager::checkpoint_exists(const std::string& checkpoint_id) const {
  return std::filesystem::exists(get_checkpoint_path(checkpoint_id));
}

Utils::Expected<void, CheckpointResult> CheckpointManager::delete_checkpoint(
    const std::string& checkpoint_id) {
  auto checkpoint_path = get_checkpoint_path(checkpoint_id);

  if (!std::filesystem::exists(checkpoint_path)) {
    return Utils::Expected<void, CheckpointResult>{CheckpointResult::FileError};
  }

  std::error_code ec;
  if (!std::filesystem::remove(checkpoint_path, ec)) {
    return Utils::Expected<void, CheckpointResult>{CheckpointResult::FileError};
  }

  return Utils::Expected<void, CheckpointResult>{};
}

Utils::Expected<bool, CheckpointResult> CheckpointManager::validate_checkpoint(
    const std::string& checkpoint_id) {
  auto load_result = load_checkpoint(checkpoint_id);
  if (!load_result.has_value()) {
    return Utils::Expected<bool, CheckpointResult>{load_result.error()};
  }
  return Utils::Expected<bool, CheckpointResult>{true};
}

Utils::Expected<CheckpointMetadata, CheckpointResult> CheckpointManager::get_checkpoint_metadata(
    const std::string& checkpoint_id) const {
  auto load_result = load_checkpoint(checkpoint_id);
  if (!load_result.has_value()) {
    return Utils::Expected<CheckpointMetadata, CheckpointResult>{load_result.error()};
  }

  return Utils::Expected<CheckpointMetadata, CheckpointResult>{
      std::move(load_result.value().metadata)};
}

std::string CheckpointManager::get_status_summary() const {
  std::ostringstream oss;
  oss << "Checkpoint Manager Status:\n";
  oss << "  Directory: " << config_.checkpoint_directory << "\n";
  oss << "  Compression: " << (config_.enable_compression ? "Enabled" : "Disabled") << "\n";
  oss << "  Validation: " << (config_.enable_validation ? "Enabled" : "Disabled") << "\n";
  oss << "  Max checkpoints: " << config_.max_checkpoints << "\n";
  oss << "  Last operation stats:\n";
  oss << "    Save time: " << last_stats_.save_time.count() << " ms\n";
  oss << "    Load time: " << last_stats_.load_time.count() << " ms\n";
  oss << "    Compression ratio: " << last_stats_.compression_ratio << "\n";
  oss << "    Validation passed: " << (last_stats_.validation_passed ? "Yes" : "No") << "\n";

  auto checkpoints = list_checkpoints();
  oss << "  Available checkpoints: " << checkpoints.size() << "\n";

  return oss.str();
}

// CheckpointScheduler implementation

CheckpointScheduler::CheckpointScheduler(CheckpointManager& manager, CheckpointConfig config)
    : manager_(manager), config_(std::move(config)) {}

void CheckpointScheduler::start_scheduling() {
  scheduling_active_ = true;
  last_checkpoint_time_ = std::chrono::system_clock::now();
}

void CheckpointScheduler::stop_scheduling() { scheduling_active_ = false; }

bool CheckpointScheduler::should_checkpoint(const SimulationState& /* state */) const {
  if (!scheduling_active_) {
    return false;
  }

  auto now = std::chrono::system_clock::now();
  auto time_since_last = now - last_checkpoint_time_;

  return time_since_last >= config_.checkpoint_interval;
}

Utils::Expected<std::string, CheckpointResult> CheckpointScheduler::trigger_checkpoint(
    const SimulationEngine& engine) {
  auto result = manager_.save_checkpoint(engine);
  if (result.has_value()) {
    last_checkpoint_time_ = std::chrono::system_clock::now();
    last_checkpoint_iteration_ = engine.get_state().iteration_count;
  }
  return result;
}

// Utility functions

std::string to_string(CheckpointResult result) {
  switch (result) {
    case CheckpointResult::Success:
      return "Success";
    case CheckpointResult::FileError:
      return "File Error";
    case CheckpointResult::CompressionError:
      return "Compression Error";
    case CheckpointResult::ValidationError:
      return "Validation Error";
    case CheckpointResult::CorruptedData:
      return "Corrupted Data";
    case CheckpointResult::IncompatibleVersion:
      return "Incompatible Version";
    case CheckpointResult::InsufficientSpace:
      return "Insufficient Space";
    case CheckpointResult::PermissionDenied:
      return "Permission Denied";
  }
  return "Unknown";
}

CheckpointResult checkpoint_result_from_string(std::string_view str) {
  if (str == "Success") return CheckpointResult::Success;
  if (str == "File Error") return CheckpointResult::FileError;
  if (str == "Compression Error") return CheckpointResult::CompressionError;
  if (str == "Validation Error") return CheckpointResult::ValidationError;
  if (str == "Corrupted Data") return CheckpointResult::CorruptedData;
  if (str == "Incompatible Version") return CheckpointResult::IncompatibleVersion;
  if (str == "Insufficient Space") return CheckpointResult::InsufficientSpace;
  if (str == "Permission Denied") return CheckpointResult::PermissionDenied;
  return CheckpointResult::FileError;  // Default
}

}  // namespace SolarSystem::Simulation
