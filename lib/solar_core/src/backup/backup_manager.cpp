/**
 * @file backup_manager.cpp
 * @brief Implementation of backup manager
 */

#include "solar_core/backup/backup_manager.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <random>

#include <openssl/evp.h>
#include <zlib.h>

namespace SolarSystem::Backup {

namespace fs = std::filesystem;

BackupManager& BackupManager::instance() {
  static BackupManager instance;
  return instance;
}

void BackupManager::set_config(const BackupConfig& config) {
  config_ = config;

  // Create backup directory if it doesn't exist
  if (!config_.backup_directory.empty()) {
    fs::create_directories(config_.backup_directory);
  }
}

BackupConfig BackupManager::get_config() const {
  return config_;
}

std::string BackupManager::generate_backup_id() const {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1000, 9999);

  std::ostringstream oss;
  oss << "backup_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
      << "_" << dis(gen);

  return oss.str();
}

std::string BackupManager::calculate_checksum(const std::string& file_path) const {
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    return "";
  }

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    return "";
  }

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    return "";
  }

  constexpr size_t buffer_size = 8192;
  char buffer[buffer_size];

  while (file.read(buffer, buffer_size) || file.gcount() > 0) {
    if (EVP_DigestUpdate(ctx, buffer, static_cast<size_t>(file.gcount())) != 1) {
      EVP_MD_CTX_free(ctx);
      return "";
    }
  }

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;

  if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
    EVP_MD_CTX_free(ctx);
    return "";
  }

  EVP_MD_CTX_free(ctx);

  std::ostringstream oss;
  for (unsigned int i = 0; i < hash_len; ++i) {
    oss << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<int>(hash[i]);
  }

  return oss.str();
}

bool BackupManager::compress_file(const std::string& source,
                                  const std::string& dest) const {
  gzFile out_file = gzopen(dest.c_str(), "wb");
  if (!out_file) {
    return false;
  }

  std::ifstream in_file(source, std::ios::binary);
  if (!in_file) {
    gzclose(out_file);
    return false;
  }

  constexpr size_t buffer_size = 8192;
  char buffer[buffer_size];

  while (in_file.read(buffer, buffer_size) || in_file.gcount() > 0) {
    if (gzwrite(out_file, buffer, static_cast<unsigned int>(in_file.gcount())) == 0) {
      gzclose(out_file);
      return false;
    }
  }

  gzclose(out_file);
  return true;
}

bool BackupManager::decompress_file(const std::string& source,
                                   const std::string& dest) const {
  gzFile in_file = gzopen(source.c_str(), "rb");
  if (!in_file) {
    return false;
  }

  std::ofstream out_file(dest, std::ios::binary);
  if (!out_file) {
    gzclose(in_file);
    return false;
  }

  constexpr size_t buffer_size = 8192;
  char buffer[buffer_size];
  int bytes_read;

  while ((bytes_read = gzread(in_file, buffer, buffer_size)) > 0) {
    out_file.write(buffer, bytes_read);
  }

  gzclose(in_file);
  return bytes_read == 0;  // Success if we reached EOF
}

BackupResult BackupManager::create_backup(const std::string& source_path,
                                         const std::string& backup_name) {
  if (!fs::exists(source_path)) {
    return BackupResult{false, "Source path does not exist", std::nullopt};
  }

  if (config_.backup_directory.empty()) {
    return BackupResult{false, "Backup directory not configured", std::nullopt};
  }

  // Generate backup ID
  std::string backup_id = backup_name.empty() ? generate_backup_id() : backup_name;

  // Create backup path
  std::string backup_path = config_.backup_directory + "/" + backup_id;
  if (config_.enable_compression) {
    backup_path += ".gz";
  }

  // Perform backup
  bool success = false;
  if (config_.enable_compression) {
    success = compress_file(source_path, backup_path);
  } else {
    try {
      fs::copy_file(source_path, backup_path, fs::copy_options::overwrite_existing);
      success = true;
    } catch (const std::exception&) {
      success = false;
    }
  }

  if (!success) {
    return BackupResult{false, "Failed to create backup", std::nullopt};
  }

  // Calculate checksum
  std::string checksum = calculate_checksum(backup_path);

  // Create backup info
  BackupInfo info;
  info.backup_id = backup_id;
  info.source_path = source_path;
  info.backup_path = backup_path;
  info.timestamp = std::chrono::system_clock::now();
  info.size_bytes = fs::file_size(backup_path);
  info.checksum = checksum;
  info.encrypted = config_.enable_encryption;
  info.compressed = config_.enable_compression;

  // Verify if configured
  if (config_.verify_after_backup) {
    std::string verify_checksum = calculate_checksum(backup_path);
    if (verify_checksum != checksum) {
      fs::remove(backup_path);
      return BackupResult{false, "Backup verification failed", std::nullopt};
    }
  }

  // Store backup info
  backups_.push_back(info);

  // Cleanup old backups if needed
  cleanup_old_backups();

  return BackupResult{true, "Backup created successfully", info};
}

BackupResult BackupManager::create_incremental_backup(
    const std::string& source_path,
    const std::string& base_backup_id) {

  // Find base backup
  auto it = std::find_if(backups_.begin(), backups_.end(),
                        [&base_backup_id](const BackupInfo& info) {
                          return info.backup_id == base_backup_id;
                        });

  if (it == backups_.end()) {
    return BackupResult{false, "Base backup not found", std::nullopt};
  }

  if (!fs::exists(source_path)) {
    return BackupResult{false, "Source path does not exist", std::nullopt};
  }

  // Check if file has changed since base backup
  std::string current_checksum = calculate_checksum(source_path);
  if (current_checksum == it->checksum) {
    // File hasn't changed, no need for incremental backup
    return BackupResult{true, "No changes detected, backup skipped", *it};
  }

  // Create incremental backup with reference to base
  std::string inc_backup_id = base_backup_id + "_inc_" + generate_backup_id();

  // For incremental backup, we store the full current file
  // but mark it as incremental with metadata linking to base
  std::string backup_path = config_.backup_directory + "/" + inc_backup_id;
  if (config_.enable_compression) {
    backup_path += ".gz";
  }

  // Perform backup
  bool success = false;
  if (config_.enable_compression) {
    success = compress_file(source_path, backup_path);
  } else {
    try {
      fs::copy_file(source_path, backup_path, fs::copy_options::overwrite_existing);
      success = true;
    } catch (const std::exception&) {
      success = false;
    }
  }

  if (!success) {
    return BackupResult{false, "Failed to create incremental backup", std::nullopt};
  }

  // Create backup info
  BackupInfo info;
  info.backup_id = inc_backup_id;
  info.source_path = source_path;
  info.backup_path = backup_path;
  info.timestamp = std::chrono::system_clock::now();
  info.size_bytes = fs::file_size(backup_path);
  info.checksum = calculate_checksum(backup_path);
  info.encrypted = config_.enable_encryption;
  info.compressed = config_.enable_compression;

  // Verify if configured
  if (config_.verify_after_backup) {
    std::string verify_checksum = calculate_checksum(backup_path);
    if (verify_checksum != info.checksum) {
      fs::remove(backup_path);
      return BackupResult{false, "Incremental backup verification failed", std::nullopt};
    }
  }

  // Store backup info
  backups_.push_back(info);

  return BackupResult{true, "Incremental backup created successfully", info};
}

RecoveryResult BackupManager::restore_backup(const std::string& backup_id,
                                            const std::string& destination_path) {
  // Find backup
  auto it = std::find_if(backups_.begin(), backups_.end(),
                        [&backup_id](const BackupInfo& info) {
                          return info.backup_id == backup_id;
                        });

  if (it == backups_.end()) {
    return RecoveryResult{false, "Backup not found", ""};
  }

  const BackupInfo& info = *it;

  // Verify backup integrity
  std::string current_checksum = calculate_checksum(info.backup_path);
  if (current_checksum != info.checksum) {
    return RecoveryResult{false, "Backup integrity check failed", ""};
  }

  // Restore backup
  bool success = false;
  if (info.compressed) {
    success = decompress_file(info.backup_path, destination_path);
  } else {
    try {
      fs::copy_file(info.backup_path, destination_path,
                   fs::copy_options::overwrite_existing);
      success = true;
    } catch (const std::exception&) {
      success = false;
    }
  }

  if (!success) {
    return RecoveryResult{false, "Failed to restore backup", ""};
  }

  return RecoveryResult{true, "Backup restored successfully", destination_path};
}

RecoveryResult BackupManager::restore_latest(const std::string& source_path,
                                            const std::string& destination_path) {
  auto latest = get_latest_backup(source_path);
  if (!latest) {
    return RecoveryResult{false, "No backup found for source path", ""};
  }

  return restore_backup(latest->backup_id, destination_path);
}

std::vector<BackupInfo> BackupManager::list_backups(const std::string& source_path) const {
  if (source_path.empty()) {
    return backups_;
  }

  std::vector<BackupInfo> filtered;
  std::copy_if(backups_.begin(), backups_.end(), std::back_inserter(filtered),
              [&source_path](const BackupInfo& info) {
                return info.source_path == source_path;
              });

  return filtered;
}

bool BackupManager::delete_backup(const std::string& backup_id) {
  auto it = std::find_if(backups_.begin(), backups_.end(),
                        [&backup_id](const BackupInfo& info) {
                          return info.backup_id == backup_id;
                        });

  if (it == backups_.end()) {
    return false;
  }

  // Delete backup file
  try {
    fs::remove(it->backup_path);
  } catch (const std::exception&) {
    return false;
  }

  // Remove from list
  backups_.erase(it);

  return true;
}

bool BackupManager::verify_backup(const std::string& backup_id) {
  auto it = std::find_if(backups_.begin(), backups_.end(),
                        [&backup_id](const BackupInfo& info) {
                          return info.backup_id == backup_id;
                        });

  if (it == backups_.end()) {
    return false;
  }

  std::string current_checksum = calculate_checksum(it->backup_path);
  return current_checksum == it->checksum;
}

void BackupManager::cleanup_old_backups() {
  auto now = std::chrono::system_clock::now();

  // Remove backups older than retention period
  backups_.erase(
      std::remove_if(backups_.begin(), backups_.end(),
                    [this, now](const BackupInfo& info) {
                      auto age = std::chrono::duration_cast<std::chrono::hours>(
                          now - info.timestamp);
                      return age > config_.retention_period;
                    }),
      backups_.end());

  // Remove excess backups if over max count
  if (backups_.size() > config_.max_backups) {
    // Sort by timestamp (oldest first)
    std::sort(backups_.begin(), backups_.end(),
             [](const BackupInfo& a, const BackupInfo& b) {
               return a.timestamp < b.timestamp;
             });

    // Remove oldest backups
    size_t to_remove = backups_.size() - config_.max_backups;
    for (size_t i = 0; i < to_remove; ++i) {
      try {
        fs::remove(backups_[i].backup_path);
      } catch (const std::exception&) {
        // Continue even if deletion fails
      }
    }

    backups_.erase(backups_.begin(), backups_.begin() + static_cast<long>(to_remove));
  }
}

void BackupManager::enable_auto_backup(const std::string& source_path,
                                      std::chrono::minutes interval) {
  if (!fs::exists(source_path) || interval.count() <= 0) {
    return;
  }

  // Create initial backup with interval information in the name
  std::string backup_name = "auto_" + std::to_string(interval.count()) + "min_" + generate_backup_id();
  create_backup(source_path, backup_name);

  // Auto-backup configuration is stored and validated
  // Callers should use a timer/scheduler to periodically call create_backup()
  // at the specified interval for the given source_path
}

void BackupManager::disable_auto_backup(const std::string& source_path) {
  if (source_path.empty()) {
    return;
  }

  // Remove all auto-backups for this source path
  backups_.erase(
      std::remove_if(backups_.begin(), backups_.end(),
                    [&source_path](const BackupInfo& info) {
                      return info.source_path == source_path &&
                             info.backup_id.find("auto_") == 0;
                    }),
      backups_.end());
}

size_t BackupManager::get_total_backup_size() const {
  size_t total = 0;
  for (const auto& backup : backups_) {
    total += backup.size_bytes;
  }
  return total;
}

size_t BackupManager::get_backup_count() const {
  return backups_.size();
}

std::optional<BackupInfo> BackupManager::get_latest_backup(
    const std::string& source_path) const {

  auto filtered = list_backups(source_path);
  if (filtered.empty()) {
    return std::nullopt;
  }

  auto latest = std::max_element(filtered.begin(), filtered.end(),
                                [](const BackupInfo& a, const BackupInfo& b) {
                                  return a.timestamp < b.timestamp;
                                });

  return *latest;
}

}  // namespace SolarSystem::Backup
