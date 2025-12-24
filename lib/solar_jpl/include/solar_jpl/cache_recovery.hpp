/**
 * @file cache_recovery.hpp
 * @brief Cache Data Recovery System
 *
 * Provides comprehensive data recovery capabilities including:
 * - Automatic repair for minor corruption
 * - Backup data source integration
 * - Data reconstruction from partial information
 * - User-guided recovery workflows
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solar_jpl/export.hpp"
#include "jpl_client.hpp"

namespace SolarSystem::JPL {

/**
 * @brief Recovery strategy types
 */
enum class RecoveryStrategy {
  CrossFormatRecovery,      // Recover from alternate format (binary/JSON)
  BackupRestoration,        // Restore from backup
  PartialReconstruction,    // Reconstruct from partial data
  MetadataRegeneration,     // Regenerate metadata
  FreshFetch,               // Clear and fetch fresh data
  UserGuided                // Interactive user-guided recovery
};

/**
 * @brief Recovery result information
 */
struct RecoveryResult {
  bool success = false;
  RecoveryStrategy strategy_used;
  std::string description;
  std::chrono::milliseconds recovery_duration{0};
  size_t bodies_recovered = 0;
  size_t bodies_lost = 0;
  std::vector<std::string> warnings;
  std::vector<std::string> actions_taken;
};

/**
 * @brief Recovery options for user-guided recovery
 */
struct RecoveryOptions {
  bool allow_cross_format_recovery = true;
  bool allow_backup_restoration = true;
  bool allow_partial_reconstruction = true;
  bool allow_metadata_regeneration = true;
  bool allow_fresh_fetch = true;
  bool interactive_mode = false;
  size_t max_recovery_attempts = 3;
  std::chrono::seconds recovery_timeout = std::chrono::seconds(300);
};

/**
 * @brief Cache Recovery Manager
 */
class SOLAR_JPL_API CacheRecoveryManager {
 public:
  /**
   * @brief Construct recovery manager
   */
  explicit CacheRecoveryManager(const std::filesystem::path& cache_directory);

  /**
   * @brief Destructor
   */
  ~CacheRecoveryManager() = default;

  // Non-copyable and non-movable
  CacheRecoveryManager(const CacheRecoveryManager&) = delete;
  CacheRecoveryManager& operator=(const CacheRecoveryManager&) = delete;
  CacheRecoveryManager(CacheRecoveryManager&&) = delete;
  CacheRecoveryManager& operator=(CacheRecoveryManager&&) = delete;

  /**
   * @brief Attempt automatic recovery
   */
  [[nodiscard]] RecoveryResult attempt_automatic_recovery(
      const RecoveryOptions& options = {});

  /**
   * @brief Recover from cross-format (binary <-> JSON)
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> recover_from_cross_format();

  /**
   * @brief Restore from backup
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> restore_from_backup(
      size_t backup_version = 0);

  /**
   * @brief Reconstruct from partial data
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> reconstruct_from_partial_data();

  /**
   * @brief Regenerate metadata from cache files
   */
  [[nodiscard]] JPLResult<CacheMetadata> regenerate_metadata();

  /**
   * @brief Analyze corruption and suggest recovery strategies
   */
  [[nodiscard]] std::vector<RecoveryStrategy> analyze_and_suggest_strategies();

  /**
   * @brief Get recovery success probability for each strategy
   */
  [[nodiscard]] std::unordered_map<RecoveryStrategy, double> estimate_recovery_probabilities();

  /**
   * @brief Interactive user-guided recovery
   */
  [[nodiscard]] RecoveryResult interactive_recovery();

  /**
   * @brief Get recovery statistics
   */
  [[nodiscard]] const RecoveryResult& get_last_recovery_result() const {
    return last_recovery_result_;
  }

 private:
  std::filesystem::path cache_directory_;
  RecoveryResult last_recovery_result_;

  /**
   * @brief Check if binary cache exists and is readable
   */
  [[nodiscard]] bool has_valid_binary_cache() const;

  /**
   * @brief Check if JSON cache exists and is readable
   */
  [[nodiscard]] bool has_valid_json_cache() const;

  /**
   * @brief Check if backups are available
   */
  [[nodiscard]] bool has_available_backups() const;

  /**
   * @brief Load data from binary cache
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> load_binary_cache() const;

  /**
   * @brief Load data from JSON cache
   */
  [[nodiscard]] JPLResult<std::vector<EphemerisData>> load_json_cache() const;

  /**
   * @brief Save data to binary cache
   */
  [[nodiscard]] JPLVoidResult save_binary_cache(
      const std::vector<EphemerisData>& data) const;

  /**
   * @brief Save data to JSON cache
   */
  [[nodiscard]] JPLVoidResult save_json_cache(
      const std::vector<EphemerisData>& data) const;

  /**
   * @brief Calculate checksum for data
   */
  [[nodiscard]] uint64_t calculate_checksum(
      const std::vector<EphemerisData>& data) const;

  /**
   * @brief Validate recovered data
   */
  [[nodiscard]] bool validate_recovered_data(
      const std::vector<EphemerisData>& data) const;

  /**
   * @brief Merge partial data from multiple sources
   */
  [[nodiscard]] std::vector<EphemerisData> merge_partial_data(
      const std::vector<std::vector<EphemerisData>>& partial_datasets) const;
};

/**
 * @brief Utility functions for recovery
 */
namespace RecoveryUtils {

/**
 * @brief Convert recovery strategy to string
 */
[[nodiscard]] std::string to_string(RecoveryStrategy strategy);

/**
 * @brief Format recovery result as human-readable string
 */
[[nodiscard]] std::string format_recovery_result(const RecoveryResult& result);

/**
 * @brief Get user confirmation for recovery action
 */
[[nodiscard]] bool get_user_confirmation(const std::string& prompt);

}  // namespace RecoveryUtils

}  // namespace SolarSystem::JPL

