/**
 * @file backup_manager.hpp
 * @brief Data backup and recovery system
 */

#ifndef SOLAR_CORE_BACKUP_BACKUP_MANAGER_HPP
#define SOLAR_CORE_BACKUP_BACKUP_MANAGER_HPP

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <functional>
#include <memory>

#include "solar_core/export.hpp"

namespace SolarSystem::Backup {

/**
 * @brief Backup status information
 */
struct BackupInfo {
  std::string backup_id;
  std::string source_path;
  std::string backup_path;
  std::chrono::system_clock::time_point timestamp;
  size_t size_bytes;
  std::string checksum;
  bool encrypted;
  bool compressed;
};

/**
 * @brief Backup configuration
 */
struct BackupConfig {
  std::string backup_directory;
  bool enable_encryption{true};
  bool enable_compression{true};
  size_t max_backups{10};
  std::chrono::hours retention_period{24 * 30};  // 30 days default
  bool verify_after_backup{true};
};

/**
 * @brief Backup result
 */
struct BackupResult {
  bool success;
  std::string message;
  std::optional<BackupInfo> backup_info;
};

/**
 * @brief Recovery result
 */
struct RecoveryResult {
  bool success;
  std::string message;
  std::string restored_path;
};

/**
 * @brief Backup manager for data protection
 */
class SOLAR_CORE_API BackupManager {
 public:
  static BackupManager& instance();

  // Configuration
  void set_config(const BackupConfig& config);
  BackupConfig get_config() const;

  // Backup operations
  BackupResult create_backup(const std::string& source_path,
                            const std::string& backup_name = "");
  BackupResult create_incremental_backup(const std::string& source_path,
                                        const std::string& base_backup_id);

  // Recovery operations
  RecoveryResult restore_backup(const std::string& backup_id,
                               const std::string& destination_path);
  RecoveryResult restore_latest(const std::string& source_path,
                               const std::string& destination_path);

  // Backup management
  std::vector<BackupInfo> list_backups(const std::string& source_path = "") const;
  bool delete_backup(const std::string& backup_id);
  bool verify_backup(const std::string& backup_id);
  void cleanup_old_backups();

  // Automated backup
  void enable_auto_backup(const std::string& source_path,
                         std::chrono::minutes interval);
  void disable_auto_backup(const std::string& source_path);

  // Statistics
  size_t get_total_backup_size() const;
  size_t get_backup_count() const;
  std::optional<BackupInfo> get_latest_backup(const std::string& source_path) const;

 private:
  BackupManager() = default;
  BackupManager(const BackupManager&) = delete;
  BackupManager& operator=(const BackupManager&) = delete;

  std::string generate_backup_id() const;
  std::string calculate_checksum(const std::string& file_path) const;
  bool compress_file(const std::string& source, const std::string& dest) const;
  bool decompress_file(const std::string& source, const std::string& dest) const;

  BackupConfig config_;
  std::vector<BackupInfo> backups_;
};

}  // namespace SolarSystem::Backup

#endif  // SOLAR_CORE_BACKUP_BACKUP_MANAGER_HPP
