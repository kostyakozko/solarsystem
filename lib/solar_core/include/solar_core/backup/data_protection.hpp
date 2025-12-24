/**
 * @file data_protection.hpp
 * @brief Data protection and integrity monitoring
 */

#ifndef SOLAR_CORE_BACKUP_DATA_PROTECTION_HPP
#define SOLAR_CORE_BACKUP_DATA_PROTECTION_HPP

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <functional>

#include "solar_core/export.hpp"

namespace SolarSystem::Backup {

/**
 * @brief Data integrity status
 */
enum class IntegrityStatus {
  VALID,
  CORRUPTED,
  MISSING,
  UNKNOWN
};

/**
 * @brief Data integrity check result
 */
struct IntegrityCheckResult {
  std::string file_path;
  IntegrityStatus status;
  std::string expected_checksum;
  std::string actual_checksum;
  std::chrono::system_clock::time_point check_time;
  std::string message;
};

/**
 * @brief Protected file metadata
 */
struct ProtectedFile {
  std::string file_path;
  std::string checksum;
  std::chrono::system_clock::time_point last_verified;
  size_t size_bytes;
  bool auto_backup_enabled;
};

/**
 * @brief Disaster recovery plan
 */
struct DisasterRecoveryPlan {
  std::string plan_name;
  std::vector<std::string> critical_files;
  std::string backup_location;
  std::chrono::minutes backup_interval{60};
  bool enable_offsite_backup{false};
  std::string offsite_location;
};

/**
 * @brief Data protection manager
 */
class SOLAR_CORE_API DataProtection {
 public:
  static DataProtection& instance();

  // File protection
  void protect_file(const std::string& file_path);
  void unprotect_file(const std::string& file_path);
  bool is_protected(const std::string& file_path) const;

  // Integrity monitoring
  IntegrityCheckResult verify_integrity(const std::string& file_path);
  std::vector<IntegrityCheckResult> verify_all_protected_files();
  void schedule_integrity_check(std::chrono::minutes interval);

  // Automatic protection
  void enable_auto_protection(const std::string& directory);
  void disable_auto_protection(const std::string& directory);

  // Disaster recovery
  void set_recovery_plan(const DisasterRecoveryPlan& plan);
  DisasterRecoveryPlan get_recovery_plan() const;
  bool execute_recovery_plan();
  bool test_recovery_plan();

  // Monitoring
  std::vector<ProtectedFile> get_protected_files() const;
  size_t get_corrupted_file_count() const;
  std::vector<std::string> get_corrupted_files() const;

  // Repair
  bool repair_file(const std::string& file_path);
  bool repair_all_corrupted_files();

 private:
  DataProtection() = default;
  DataProtection(const DataProtection&) = delete;
  DataProtection& operator=(const DataProtection&) = delete;

  std::string calculate_checksum(const std::string& file_path) const;
  void update_file_metadata(const std::string& file_path);

  std::vector<ProtectedFile> protected_files_;
  std::vector<std::string> auto_protected_directories_;
  DisasterRecoveryPlan recovery_plan_;
};

}  // namespace SolarSystem::Backup

#endif  // SOLAR_CORE_BACKUP_DATA_PROTECTION_HPP
