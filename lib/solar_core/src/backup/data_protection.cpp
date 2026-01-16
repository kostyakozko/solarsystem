/**
 * @file data_protection.cpp
 * @brief Implementation of data protection
 */

#include "solar_core/backup/data_protection.hpp"

#include <openssl/evp.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "solar_core/backup/backup_manager.hpp"

namespace SolarSystem::Backup {

namespace fs = std::filesystem;

DataProtection& DataProtection::instance() {
  static DataProtection instance;
  return instance;
}

std::string DataProtection::calculate_checksum(const std::string& file_path) const {
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
    oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hash[i]);
  }

  return oss.str();
}

void DataProtection::protect_file(const std::string& file_path) {
  if (!fs::exists(file_path)) {
    return;
  }

  // Check if already protected
  auto it =
      std::find_if(protected_files_.begin(), protected_files_.end(),
                   [&file_path](const ProtectedFile& pf) { return pf.file_path == file_path; });

  if (it != protected_files_.end()) {
    // Update existing protection
    update_file_metadata(file_path);
    return;
  }

  // Add new protected file
  ProtectedFile pf;
  pf.file_path = file_path;
  pf.checksum = calculate_checksum(file_path);
  pf.last_verified = std::chrono::system_clock::now();
  pf.size_bytes = fs::file_size(file_path);
  pf.auto_backup_enabled = false;

  protected_files_.push_back(pf);

  // Create initial backup
  auto& backup_mgr = BackupManager::instance();
  backup_mgr.create_backup(file_path);
}

void DataProtection::unprotect_file(const std::string& file_path) {
  protected_files_.erase(
      std::remove_if(protected_files_.begin(), protected_files_.end(),
                     [&file_path](const ProtectedFile& pf) { return pf.file_path == file_path; }),
      protected_files_.end());
}

bool DataProtection::is_protected(const std::string& file_path) const {
  return std::any_of(protected_files_.begin(), protected_files_.end(),
                     [&file_path](const ProtectedFile& pf) { return pf.file_path == file_path; });
}

void DataProtection::update_file_metadata(const std::string& file_path) {
  auto it =
      std::find_if(protected_files_.begin(), protected_files_.end(),
                   [&file_path](const ProtectedFile& pf) { return pf.file_path == file_path; });

  if (it != protected_files_.end()) {
    it->checksum = calculate_checksum(file_path);
    it->last_verified = std::chrono::system_clock::now();
    it->size_bytes = fs::file_size(file_path);
  }
}

IntegrityCheckResult DataProtection::verify_integrity(const std::string& file_path) {
  IntegrityCheckResult result;
  result.file_path = file_path;
  result.check_time = std::chrono::system_clock::now();

  // Find protected file
  auto it =
      std::find_if(protected_files_.begin(), protected_files_.end(),
                   [&file_path](const ProtectedFile& pf) { return pf.file_path == file_path; });

  if (it == protected_files_.end()) {
    result.status = IntegrityStatus::UNKNOWN;
    result.message = "File is not protected";
    return result;
  }

  // Check if file exists
  if (!fs::exists(file_path)) {
    result.status = IntegrityStatus::MISSING;
    result.expected_checksum = it->checksum;
    result.message = "File is missing";
    return result;
  }

  // Calculate current checksum
  std::string current_checksum = calculate_checksum(file_path);
  result.expected_checksum = it->checksum;
  result.actual_checksum = current_checksum;

  // Compare checksums
  if (current_checksum == it->checksum) {
    result.status = IntegrityStatus::VALID;
    result.message = "File integrity verified";
    it->last_verified = result.check_time;
  } else {
    result.status = IntegrityStatus::CORRUPTED;
    result.message = "File has been modified or corrupted";
  }

  return result;
}

std::vector<IntegrityCheckResult> DataProtection::verify_all_protected_files() {
  std::vector<IntegrityCheckResult> results;

  for (const auto& pf : protected_files_) {
    results.push_back(verify_integrity(pf.file_path));
  }

  return results;
}

void DataProtection::schedule_integrity_check(std::chrono::minutes interval) {
  if (interval.count() <= 0) {
    return;
  }

  // Perform initial integrity check
  verify_all_protected_files();

  // Integrity check configuration is validated
  // Callers should use a timer/scheduler to periodically call
  // verify_all_protected_files() at the specified interval
}

void DataProtection::enable_auto_protection(const std::string& directory) {
  if (!fs::exists(directory) || !fs::is_directory(directory)) {
    return;
  }

  auto_protected_directories_.push_back(directory);

  // Protect all files in directory
  for (const auto& entry : fs::recursive_directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      protect_file(entry.path().string());
    }
  }
}

void DataProtection::disable_auto_protection(const std::string& directory) {
  auto_protected_directories_.erase(std::remove(auto_protected_directories_.begin(),
                                                auto_protected_directories_.end(), directory),
                                    auto_protected_directories_.end());
}

void DataProtection::set_recovery_plan(const DisasterRecoveryPlan& plan) {
  recovery_plan_ = plan;

  // Protect all critical files
  for (const auto& file : plan.critical_files) {
    protect_file(file);
  }
}

DisasterRecoveryPlan DataProtection::get_recovery_plan() const { return recovery_plan_; }

bool DataProtection::execute_recovery_plan() {
  if (recovery_plan_.critical_files.empty()) {
    return false;
  }

  auto& backup_mgr = BackupManager::instance();
  bool all_success = true;

  for (const auto& file : recovery_plan_.critical_files) {
    // Restore from latest backup
    auto result = backup_mgr.restore_latest(file, file + ".recovered");
    if (!result.success) {
      all_success = false;
    }
  }

  return all_success;
}

bool DataProtection::test_recovery_plan() {
  if (recovery_plan_.critical_files.empty()) {
    return false;
  }

  auto& backup_mgr = BackupManager::instance();

  // Verify all critical files have backups
  for (const auto& file : recovery_plan_.critical_files) {
    auto latest = backup_mgr.get_latest_backup(file);
    if (!latest) {
      return false;
    }

    // Verify backup integrity
    if (!backup_mgr.verify_backup(latest->backup_id)) {
      return false;
    }
  }

  return true;
}

std::vector<ProtectedFile> DataProtection::get_protected_files() const { return protected_files_; }

size_t DataProtection::get_corrupted_file_count() const {
  size_t count = 0;

  for (const auto& pf : protected_files_) {
    if (!fs::exists(pf.file_path)) {
      ++count;
      continue;
    }

    std::string current_checksum = calculate_checksum(pf.file_path);
    if (current_checksum != pf.checksum) {
      ++count;
    }
  }

  return count;
}

std::vector<std::string> DataProtection::get_corrupted_files() const {
  std::vector<std::string> corrupted;

  for (const auto& pf : protected_files_) {
    if (!fs::exists(pf.file_path)) {
      corrupted.push_back(pf.file_path);
      continue;
    }

    std::string current_checksum = calculate_checksum(pf.file_path);
    if (current_checksum != pf.checksum) {
      corrupted.push_back(pf.file_path);
    }
  }

  return corrupted;
}

bool DataProtection::repair_file(const std::string& file_path) {
  auto& backup_mgr = BackupManager::instance();

  // Try to restore from latest backup
  auto result = backup_mgr.restore_latest(file_path, file_path);

  if (result.success) {
    // Update metadata after successful repair
    update_file_metadata(file_path);
  }

  return result.success;
}

bool DataProtection::repair_all_corrupted_files() {
  auto corrupted = get_corrupted_files();
  bool all_success = true;

  for (const auto& file : corrupted) {
    if (!repair_file(file)) {
      all_success = false;
    }
  }

  return all_success;
}

}  // namespace SolarSystem::Backup
