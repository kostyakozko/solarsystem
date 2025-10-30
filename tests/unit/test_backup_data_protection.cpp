/**
 * @file test_backup_data_protection.cpp
 * @brief Unit tests for backup and data protection systems
 */

#include "../utils/test_framework.h"
#include "solar_core/backup/backup_manager.hpp"
#include "solar_core/backup/data_protection.hpp"

#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace SolarSystem::Backup;
namespace fs = std::filesystem;

// Global test data
std::string test_dir = "test_backup_data";
std::string test_file1;
std::string test_file2;

void setup_test_environment() {
  fs::create_directories(test_dir);

  test_file1 = test_dir + "/test_file1.txt";
  test_file2 = test_dir + "/test_file2.txt";

  std::ofstream f1(test_file1);
  f1 << "Test data for file 1\n";
  f1.close();

  std::ofstream f2(test_file2);
  f2 << "Test data for file 2\n";
  f2.close();

  BackupConfig config;
  config.backup_directory = test_dir + "/backups";
  config.enable_encryption = false;
  config.enable_compression = true;
  config.max_backups = 5;
  config.retention_period = std::chrono::hours(1);
  config.verify_after_backup = true;

  BackupManager::instance().set_config(config);
}

void cleanup_test_environment() {
  if (fs::exists(test_dir)) {
    fs::remove_all(test_dir);
  }
}

int main() {
  TestSuite suite("Backup and Data Protection Tests");

  setup_test_environment();

  suite.run_test("Create Backup", []() {
    auto& backup_mgr = BackupManager::instance();
    auto result = backup_mgr.create_backup(test_file1);

    if (!result.success) throw std::runtime_error("Backup creation failed");
    if (!result.backup_info.has_value()) throw std::runtime_error("No backup info");
  });

  suite.run_test("Restore Backup", []() {
    auto& backup_mgr = BackupManager::instance();

    auto backup_result = backup_mgr.create_backup(test_file1);
    if (!backup_result.success) throw std::runtime_error("Backup creation failed");

    std::string backup_id = backup_result.backup_info->backup_id;
    std::string restore_path = test_dir + "/restored_file.txt";
    auto restore_result = backup_mgr.restore_backup(backup_id, restore_path);

    if (!restore_result.success) throw std::runtime_error("Restore failed");
    if (!fs::exists(restore_path)) throw std::runtime_error("Restored file doesn't exist");
  });

  suite.run_test("List Backups", []() {
    auto& backup_mgr = BackupManager::instance();
    auto backups = backup_mgr.list_backups();

    if (backups.empty()) throw std::runtime_error("No backups found");
  });

  suite.run_test("Protect File", []() {
    auto& protection = DataProtection::instance();
    protection.protect_file(test_file1);

    if (!protection.is_protected(test_file1)) throw std::runtime_error("File not protected");
  });

  suite.run_test("Verify Integrity", []() {
    auto& protection = DataProtection::instance();
    protection.protect_file(test_file2);

    auto result = protection.verify_integrity(test_file2);
    if (result.status != IntegrityStatus::VALID) throw std::runtime_error("Integrity check failed");
  });

  suite.run_test("Detect Corruption", []() {
    auto& protection = DataProtection::instance();
    std::string test_file3 = test_dir + "/test_file3.txt";

    std::ofstream f(test_file3);
    f << "Original content\n";
    f.close();

    protection.protect_file(test_file3);

    std::ofstream f2(test_file3, std::ios::app);
    f2 << "Modified\n";
    f2.close();

    auto result = protection.verify_integrity(test_file3);
    if (result.status != IntegrityStatus::CORRUPTED) throw std::runtime_error("Corruption not detected");
  });

  suite.run_test("Disaster Recovery Plan", []() {
    auto& protection = DataProtection::instance();

    DisasterRecoveryPlan plan;
    plan.plan_name = "Test Recovery Plan";
    plan.critical_files = {test_file1, test_file2};
    plan.backup_location = test_dir + "/backups";

    protection.set_recovery_plan(plan);

    auto retrieved_plan = protection.get_recovery_plan();
    if (retrieved_plan.plan_name != plan.plan_name) throw std::runtime_error("Plan name mismatch");
    if (retrieved_plan.critical_files.size() != 2) throw std::runtime_error("Critical files count mismatch");
  });

  cleanup_test_environment();

  suite.print_summary();
  return suite.get_failed_count() > 0 ? 1 : 0;
}
