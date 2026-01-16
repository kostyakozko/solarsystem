/**
 * @file cache_recovery.cpp
 * @brief Implementation of Cache Data Recovery System
 */

#include "solar_jpl/cache_recovery.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace SolarSystem::JPL {

/**
 * @brief Constructor
 */
CacheRecoveryManager::CacheRecoveryManager(const std::filesystem::path& cache_directory)
    : cache_directory_(cache_directory) {}

/**
 * @brief Attempt automatic recovery
 */
RecoveryResult CacheRecoveryManager::attempt_automatic_recovery(const RecoveryOptions& options) {
  auto start_time = std::chrono::steady_clock::now();

  RecoveryResult result;
  result.success = false;

  // Get suggested strategies
  auto strategies = analyze_and_suggest_strategies();

  // Try each strategy in order of probability
  for (const auto& strategy : strategies) {
    result.actions_taken.push_back("Attempting " + RecoveryUtils::to_string(strategy));

    bool strategy_success = false;

    switch (strategy) {
      case RecoveryStrategy::CrossFormatRecovery:
        if (options.allow_cross_format_recovery) {
          auto recovery_result = recover_from_cross_format();
          if (is_success(recovery_result)) {
            result.bodies_recovered = get_value(recovery_result).size();
            strategy_success = true;
          }
        }
        break;

      case RecoveryStrategy::BackupRestoration:
        if (options.allow_backup_restoration) {
          auto recovery_result = restore_from_backup();
          if (is_success(recovery_result)) {
            result.bodies_recovered = get_value(recovery_result).size();
            strategy_success = true;
          }
        }
        break;

      case RecoveryStrategy::PartialReconstruction:
        if (options.allow_partial_reconstruction) {
          auto recovery_result = reconstruct_from_partial_data();
          if (is_success(recovery_result)) {
            result.bodies_recovered = get_value(recovery_result).size();
            strategy_success = true;
          }
        }
        break;

      case RecoveryStrategy::MetadataRegeneration:
        if (options.allow_metadata_regeneration) {
          auto metadata_result = regenerate_metadata();
          if (is_success(metadata_result)) {
            strategy_success = true;
          }
        }
        break;

      case RecoveryStrategy::FreshFetch:
        if (options.allow_fresh_fetch) {
          // Clear corrupted cache to allow fresh fetch
          std::error_code ec;
          std::filesystem::remove_all(cache_directory_, ec);
          std::filesystem::create_directories(cache_directory_, ec);
          strategy_success = true;
          result.warnings.push_back("Cache cleared - fresh data fetch required");
        }
        break;

      case RecoveryStrategy::UserGuided:
        if (options.interactive_mode) {
          auto interactive_result = interactive_recovery();
          strategy_success = interactive_result.success;
          result = interactive_result;
        }
        break;
    }

    if (strategy_success) {
      result.success = true;
      result.strategy_used = strategy;
      result.description = "Recovery successful using " + RecoveryUtils::to_string(strategy);
      break;
    }
  }

  auto end_time = std::chrono::steady_clock::now();
  result.recovery_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  last_recovery_result_ = result;
  return result;
}

/**
 * @brief Recover from cross-format
 */
JPLResult<std::vector<EphemerisData>> CacheRecoveryManager::recover_from_cross_format() {
  auto binary_path = cache_directory_ / "ephemeris_cache.bin";
  auto json_path = cache_directory_ / "ephemeris_data.json";

  bool has_binary = has_valid_binary_cache();
  bool has_json = has_valid_json_cache();

  // Try binary first
  if (has_binary) {
    auto binary_data = load_binary_cache();
    if (is_success(binary_data)) {
      // Regenerate JSON from binary
      auto save_result = save_json_cache(get_value(binary_data));
      if (is_success(save_result)) {
        return binary_data;
      }
    }
  }

  // Try JSON if binary failed
  if (has_json) {
    auto json_data = load_json_cache();
    if (is_success(json_data)) {
      // Regenerate binary from JSON
      auto save_result = save_binary_cache(get_value(json_data));
      if (is_success(save_result)) {
        return json_data;
      }
    }
  }

  return JPLError::CacheError;
}

/**
 * @brief Restore from backup
 */
JPLResult<std::vector<EphemerisData>> CacheRecoveryManager::restore_from_backup(
    size_t backup_version) {
  auto backup_dir = cache_directory_.parent_path() / "cache_backups";
  auto backup_path = backup_dir / ("backup_" + std::to_string(backup_version));

  if (!std::filesystem::exists(backup_path)) {
    return JPLError::CacheError;
  }

  // Copy backup to cache directory
  std::error_code ec;
  std::filesystem::remove_all(cache_directory_, ec);
  std::filesystem::copy(backup_path, cache_directory_, std::filesystem::copy_options::recursive,
                        ec);

  if (ec) {
    return JPLError::CacheError;
  }

  // Load restored data
  if (has_valid_binary_cache()) {
    return load_binary_cache();
  } else if (has_valid_json_cache()) {
    return load_json_cache();
  }

  return JPLError::CacheError;
}

/**
 * @brief Reconstruct from partial data
 */
JPLResult<std::vector<EphemerisData>> CacheRecoveryManager::reconstruct_from_partial_data() {
  std::vector<std::vector<EphemerisData>> partial_datasets;

  // Try to load whatever we can from binary
  auto binary_result = load_binary_cache();
  if (is_success(binary_result)) {
    partial_datasets.push_back(get_value(binary_result));
  }

  // Try to load whatever we can from JSON
  auto json_result = load_json_cache();
  if (is_success(json_result)) {
    partial_datasets.push_back(get_value(json_result));
  }

  // Try to load from backups
  auto backup_dir = cache_directory_.parent_path() / "cache_backups";
  if (std::filesystem::exists(backup_dir)) {
    for (size_t i = 0; i < 5; ++i) {  // Try up to 5 backups
      auto backup_result = restore_from_backup(i);
      if (is_success(backup_result)) {
        partial_datasets.push_back(get_value(backup_result));
      }
    }
  }

  if (partial_datasets.empty()) {
    return JPLError::CacheError;
  }

  // Merge partial data
  auto merged_data = merge_partial_data(partial_datasets);

  if (merged_data.empty()) {
    return JPLError::CacheError;
  }

  // Save reconstructed data
  (void)save_binary_cache(merged_data);
  (void)save_json_cache(merged_data);

  return merged_data;
}

/**
 * @brief Regenerate metadata
 */
JPLResult<CacheMetadata> CacheRecoveryManager::regenerate_metadata() {
  std::vector<EphemerisData> cache_data;

  // Load data from available source
  if (has_valid_binary_cache()) {
    auto binary_result = load_binary_cache();
    if (is_success(binary_result)) {
      cache_data = get_value(binary_result);
    }
  } else if (has_valid_json_cache()) {
    auto json_result = load_json_cache();
    if (is_success(json_result)) {
      cache_data = get_value(json_result);
    }
  }

  if (cache_data.empty()) {
    return JPLError::CacheError;
  }

  // Create new metadata
  CacheMetadata metadata;
  metadata.created_at = std::chrono::system_clock::now();
  metadata.epoch = cache_data[0].epoch;
  metadata.source = "Regenerated";
  metadata.body_count = cache_data.size();
  metadata.checksum = calculate_checksum(cache_data);

  // Save metadata
  auto metadata_path = cache_directory_ / "metadata.json";
  std::ofstream metadata_file(metadata_path);
  if (!metadata_file.is_open()) {
    return JPLError::CacheError;
  }

  nlohmann::json j;
  j["created_at"] =
      std::chrono::duration_cast<std::chrono::seconds>(metadata.created_at.time_since_epoch())
          .count();
  j["epoch"] =
      std::chrono::duration_cast<std::chrono::seconds>(metadata.epoch.time_since_epoch()).count();
  j["source"] = metadata.source;
  j["body_count"] = metadata.body_count;
  j["checksum"] = metadata.checksum;

  metadata_file << j.dump(2);

  return metadata;
}

/**
 * @brief Analyze and suggest strategies
 */
std::vector<RecoveryStrategy> CacheRecoveryManager::analyze_and_suggest_strategies() {
  std::vector<RecoveryStrategy> strategies;

  // Check what resources are available
  bool has_binary = has_valid_binary_cache();
  bool has_json = has_valid_json_cache();
  bool has_backups = has_available_backups();

  // Prioritize strategies based on available resources
  if (has_binary || has_json) {
    strategies.push_back(RecoveryStrategy::CrossFormatRecovery);
    strategies.push_back(RecoveryStrategy::MetadataRegeneration);
  }

  if (has_backups) {
    strategies.push_back(RecoveryStrategy::BackupRestoration);
  }

  if (has_binary || has_json || has_backups) {
    strategies.push_back(RecoveryStrategy::PartialReconstruction);
  }

  // Fresh fetch as last resort
  strategies.push_back(RecoveryStrategy::FreshFetch);

  return strategies;
}

/**
 * @brief Estimate recovery probabilities
 */
std::unordered_map<RecoveryStrategy, double>
CacheRecoveryManager::estimate_recovery_probabilities() {
  std::unordered_map<RecoveryStrategy, double> probabilities;

  bool has_binary = has_valid_binary_cache();
  bool has_json = has_valid_json_cache();
  bool has_backups = has_available_backups();

  // Estimate probabilities based on available resources
  probabilities[RecoveryStrategy::CrossFormatRecovery] = (has_binary || has_json) ? 0.8 : 0.0;

  probabilities[RecoveryStrategy::BackupRestoration] = has_backups ? 0.9 : 0.0;

  probabilities[RecoveryStrategy::PartialReconstruction] =
      (has_binary || has_json || has_backups) ? 0.6 : 0.0;

  probabilities[RecoveryStrategy::MetadataRegeneration] = (has_binary || has_json) ? 0.7 : 0.0;

  probabilities[RecoveryStrategy::FreshFetch] = 1.0;  // Always works

  probabilities[RecoveryStrategy::UserGuided] = 0.5;  // Depends on user

  return probabilities;
}

/**
 * @brief Interactive recovery
 */
RecoveryResult CacheRecoveryManager::interactive_recovery() {
  RecoveryResult result;
  result.success = false;

  std::cout << "\n=== Interactive Cache Recovery ===\n";
  std::cout << "Cache corruption detected. Available recovery options:\n\n";

  auto strategies = analyze_and_suggest_strategies();
  auto probabilities = estimate_recovery_probabilities();

  for (size_t i = 0; i < strategies.size(); ++i) {
    auto strategy = strategies[i];
    auto prob = probabilities[strategy];

    std::cout << (i + 1) << ". " << RecoveryUtils::to_string(strategy);
    std::cout << " (Success probability: " << static_cast<int>(prob * 100) << "%)\n";
  }

  std::cout << "\nSelect recovery strategy (1-" << strategies.size() << "): ";

  size_t choice;
  std::cin >> choice;

  if (choice < 1 || choice > strategies.size()) {
    result.description = "Invalid choice";
    return result;
  }

  auto selected_strategy = strategies[choice - 1];

  std::cout << "\nAttempting " << RecoveryUtils::to_string(selected_strategy) << "...\n";

  // Execute selected strategy
  RecoveryOptions options;
  options.interactive_mode = false;

  // Temporarily enable only the selected strategy
  options.allow_cross_format_recovery =
      (selected_strategy == RecoveryStrategy::CrossFormatRecovery);
  options.allow_backup_restoration = (selected_strategy == RecoveryStrategy::BackupRestoration);
  options.allow_partial_reconstruction =
      (selected_strategy == RecoveryStrategy::PartialReconstruction);
  options.allow_metadata_regeneration =
      (selected_strategy == RecoveryStrategy::MetadataRegeneration);
  options.allow_fresh_fetch = (selected_strategy == RecoveryStrategy::FreshFetch);

  result = attempt_automatic_recovery(options);

  if (result.success) {
    std::cout << "✓ Recovery successful!\n";
    std::cout << "  Bodies recovered: " << result.bodies_recovered << "\n";
  } else {
    std::cout << "✗ Recovery failed\n";
  }

  return result;
}

// Private helper methods

bool CacheRecoveryManager::has_valid_binary_cache() const {
  auto binary_path = cache_directory_ / "ephemeris_cache.bin";
  return std::filesystem::exists(binary_path) && std::filesystem::file_size(binary_path) > 0;
}

bool CacheRecoveryManager::has_valid_json_cache() const {
  auto json_path = cache_directory_ / "ephemeris_data.json";
  return std::filesystem::exists(json_path) && std::filesystem::file_size(json_path) > 0;
}

bool CacheRecoveryManager::has_available_backups() const {
  auto backup_dir = cache_directory_.parent_path() / "cache_backups";
  return std::filesystem::exists(backup_dir) && !std::filesystem::is_empty(backup_dir);
}

JPLResult<std::vector<EphemerisData>> CacheRecoveryManager::load_binary_cache() const {
  // Simplified implementation - would use actual binary loading
  return JPLError::CacheError;
}

JPLResult<std::vector<EphemerisData>> CacheRecoveryManager::load_json_cache() const {
  // Simplified implementation - would use actual JSON loading
  return JPLError::CacheError;
}

JPLVoidResult CacheRecoveryManager::save_binary_cache(const std::vector<EphemerisData>&) const {
  // Simplified implementation - would use actual binary saving
  return success();
}

JPLVoidResult CacheRecoveryManager::save_json_cache(const std::vector<EphemerisData>&) const {
  // Simplified implementation - would use actual JSON saving
  return success();
}

uint64_t CacheRecoveryManager::calculate_checksum(const std::vector<EphemerisData>& data) const {
  uint64_t checksum = 0;
  for (const auto& body_data : data) {
    checksum += static_cast<uint64_t>(body_data.jpl_id);
    std::hash<std::string> hasher;
    checksum += hasher(body_data.body_name);
  }
  return checksum;
}

bool CacheRecoveryManager::validate_recovered_data(const std::vector<EphemerisData>& data) const {
  if (data.empty()) {
    return false;
  }

  // Basic validation - check that all bodies have valid data
  for (const auto& body : data) {
    if (body.jpl_id <= 0 || body.body_name.empty()) {
      return false;
    }
  }

  return true;
}

std::vector<EphemerisData> CacheRecoveryManager::merge_partial_data(
    const std::vector<std::vector<EphemerisData>>& partial_datasets) const {
  std::unordered_map<int, EphemerisData> merged_map;

  // Merge data from all sources, preferring newer data
  for (const auto& dataset : partial_datasets) {
    for (const auto& body_data : dataset) {
      auto it = merged_map.find(body_data.jpl_id);
      if (it == merged_map.end()) {
        // New body, add it
        merged_map[body_data.jpl_id] = body_data;
      } else {
        // Body exists, keep the one with newer epoch
        if (body_data.epoch > it->second.epoch) {
          it->second = body_data;
        }
      }
    }
  }

  // Convert map to vector
  std::vector<EphemerisData> merged_data;
  merged_data.reserve(merged_map.size());
  for (const auto& [jpl_id, body_data] : merged_map) {
    merged_data.push_back(body_data);
  }

  return merged_data;
}

// Utility functions

namespace RecoveryUtils {

std::string to_string(RecoveryStrategy strategy) {
  switch (strategy) {
    case RecoveryStrategy::CrossFormatRecovery:
      return "Cross-Format Recovery (Binary <-> JSON)";
    case RecoveryStrategy::BackupRestoration:
      return "Backup Restoration";
    case RecoveryStrategy::PartialReconstruction:
      return "Partial Data Reconstruction";
    case RecoveryStrategy::MetadataRegeneration:
      return "Metadata Regeneration";
    case RecoveryStrategy::FreshFetch:
      return "Fresh Data Fetch";
    case RecoveryStrategy::UserGuided:
      return "User-Guided Recovery";
    default:
      return "Unknown Strategy";
  }
}

std::string format_recovery_result(const RecoveryResult& result) {
  std::ostringstream formatted;

  formatted << "=== Recovery Result ===\n";
  formatted << "Success: " << (result.success ? "Yes" : "No") << "\n";
  formatted << "Strategy: " << to_string(result.strategy_used) << "\n";
  formatted << "Description: " << result.description << "\n";
  formatted << "Duration: " << result.recovery_duration.count() << " ms\n";
  formatted << "Bodies Recovered: " << result.bodies_recovered << "\n";
  formatted << "Bodies Lost: " << result.bodies_lost << "\n";

  if (!result.warnings.empty()) {
    formatted << "\nWarnings:\n";
    for (const auto& warning : result.warnings) {
      formatted << "  - " << warning << "\n";
    }
  }

  if (!result.actions_taken.empty()) {
    formatted << "\nActions Taken:\n";
    for (const auto& action : result.actions_taken) {
      formatted << "  - " << action << "\n";
    }
  }

  return formatted.str();
}

bool get_user_confirmation(const std::string& prompt) {
  std::cout << prompt << " (y/n): ";
  char response;
  std::cin >> response;
  return (response == 'y' || response == 'Y');
}

}  // namespace RecoveryUtils

}  // namespace SolarSystem::JPL
