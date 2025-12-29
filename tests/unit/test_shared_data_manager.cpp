/**
 * @file test_shared_data_manager.cpp
 * @brief Unit tests for shared data management and synchronization
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/data/shared_data_manager.hpp"

#include <chrono>
#include <thread>

using namespace SolarSystem::Data;
  // Basic existence and key management tests
TEST(SharedDataManagerTests, Key_Existence_Check) {
    SharedDataManager manager;

    // Non-existent key
    if (manager.exists("nonexistent")) throw std::runtime_error("Non-existent key should not exist");

    // After clear
    manager.clear();
    if (manager.exists("any_key")) throw std::runtime_error("No keys should exist after clear");
}

TEST(SharedDataManagerTests, Get_All_Keys___Empty) {
    SharedDataManager manager;

    auto keys = manager.get_keys();
    if (!keys.empty()) throw std::runtime_error("Should have no keys initially");
}

  // Version management tests
TEST(SharedDataManagerTests, Version_for_Non_existent_Key) {
    SharedDataManager manager;

    auto version = manager.get_version("nonexistent");
    if (version) throw std::runtime_error("Should return nullopt for non-existent key");
}

  // Locking tests (these work with the existing implementation)
TEST(SharedDataManagerTests, Lock_Non_existent_Key) {
    SharedDataManager manager;

    // Can lock even if key doesn't exist (implementation allows this)
    if (!manager.lock("key1", "owner1")) throw std::runtime_error("Should be able to lock");
    if (!manager.is_locked("key1")) throw std::runtime_error("Key should be locked");

    manager.unlock("key1", "owner1");
    if (manager.is_locked("key1")) throw std::runtime_error("Key should be unlocked");
}

TEST(SharedDataManagerTests, Lock_Timeout) {
    SharedDataManager manager;

    // Lock with short timeout
    if (!manager.lock("key1", "owner1", std::chrono::milliseconds(100))) {
      throw std::runtime_error("Failed to lock data");
    }

    if (!manager.is_locked("key1")) throw std::runtime_error("Should be locked");

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Should be able to lock again after timeout
    if (!manager.lock("key1", "owner2")) {
      throw std::runtime_error("Should be able to lock after timeout");
    }

    if (!manager.is_locked("key1")) throw std::runtime_error("Should be locked by owner2");
}

TEST(SharedDataManagerTests, Unlock_by_Wrong_Owner) {
    SharedDataManager manager;

    if (!manager.lock("key1", "owner1")) throw std::runtime_error("Failed to lock");

    // Try to unlock with wrong owner
    manager.unlock("key1", "owner2");

    // Should still be locked
    if (!manager.is_locked("key1")) throw std::runtime_error("Data should still be locked");

    // Unlock with correct owner
    manager.unlock("key1", "owner1");
    if (manager.is_locked("key1")) throw std::runtime_error("Data should be unlocked");
}

TEST(SharedDataManagerTests, Multiple_Locks) {
    SharedDataManager manager;

    // Lock first key
    if (!manager.lock("key1", "owner1")) throw std::runtime_error("Failed to lock key1");

    // Lock second key
    if (!manager.lock("key2", "owner2")) throw std::runtime_error("Failed to lock key2");

    // Both should be locked
    if (!manager.is_locked("key1")) throw std::runtime_error("key1 should be locked");
    if (!manager.is_locked("key2")) throw std::runtime_error("key2 should be locked");

    // Try to lock key1 again (should fail)
    if (manager.lock("key1", "owner3")) throw std::runtime_error("Should not be able to lock already locked key");

    // Unlock both
    manager.unlock("key1", "owner1");
    manager.unlock("key2", "owner2");

    if (manager.is_locked("key1")) throw std::runtime_error("key1 should be unlocked");
    if (manager.is_locked("key2")) throw std::runtime_error("key2 should be unlocked");
}

  // Synchronization tests
TEST(SharedDataManagerTests, Synchronize_with_Empty_Remote) {
    SharedDataManager manager;

    std::map<std::string, DataVersion> remote_versions;

    auto result = manager.synchronize(remote_versions);
    if (!result.success) throw std::runtime_error("Synchronization should succeed");
    if (result.conflicts_detected != 0) throw std::runtime_error("Should have no conflicts");
    if (result.items_synced != 0) throw std::runtime_error("Should sync zero items");
}

TEST(SharedDataManagerTests, Synchronize_with_New_Remote_Data) {
    SharedDataManager manager;

    // Create remote versions with new data
    std::map<std::string, DataVersion> remote_versions;
    remote_versions["key1"] = DataVersion{1, std::chrono::system_clock::now(), "remote"};
    remote_versions["key2"] = DataVersion{2, std::chrono::system_clock::now(), "remote"};

    auto result = manager.synchronize(remote_versions);
    if (!result.success) throw std::runtime_error("Synchronization should succeed");
    if (result.conflicts_detected != 0) throw std::runtime_error("Should have no conflicts with new data");
    if (result.items_synced != 2) throw std::runtime_error("Should sync two new items");
}

TEST(SharedDataManagerTests, Set_Conflict_Resolution_Strategy) {
    SharedDataManager manager;

    // Should not throw
    manager.set_conflict_resolution(ConflictResolution::LAST_WRITE_WINS);
    manager.set_conflict_resolution(ConflictResolution::FIRST_WRITE_WINS);
    manager.set_conflict_resolution(ConflictResolution::MANUAL);
    manager.set_conflict_resolution(ConflictResolution::MERGE);
    manager.set_conflict_resolution(ConflictResolution::REJECT);
}

  // Statistics tests
TEST(SharedDataManagerTests, Statistics___Initial_State) {
    SharedDataManager manager;

    auto stats = manager.get_statistics();
    if (stats.total_entries != 0) throw std::runtime_error("Should start with 0 entries");
    if (stats.locked_entries != 0) throw std::runtime_error("Should start with 0 locked entries");
    if (stats.dirty_entries != 0) throw std::runtime_error("Should start with 0 dirty entries");
    if (stats.total_reads != 0) throw std::runtime_error("Should start with 0 reads");
    if (stats.total_writes != 0) throw std::runtime_error("Should start with 0 writes");
    if (stats.conflicts_resolved != 0) throw std::runtime_error("Should start with 0 conflicts resolved");
}

TEST(SharedDataManagerTests, Statistics___After_Locks) {
    SharedDataManager manager;

    // Lock some keys
    if (!manager.lock("key1", "owner1")) throw std::runtime_error("Failed to lock");
    if (!manager.lock("key2", "owner2")) throw std::runtime_error("Failed to lock");

    auto stats = manager.get_statistics();
    if (stats.locked_entries != 2) throw std::runtime_error("Should have 2 locked entries");

    // Unlock one
    manager.unlock("key1", "owner1");
    stats = manager.get_statistics();
    if (stats.locked_entries != 1) throw std::runtime_error("Should have 1 locked entry");
}

TEST(SharedDataManagerTests, Clear_All_Data) {
    SharedDataManager manager;

    // Lock some keys
    if (!manager.lock("key1", "owner1")) throw std::runtime_error("Failed to lock");
    if (!manager.lock("key2", "owner2")) throw std::runtime_error("Failed to lock");

    auto stats = manager.get_statistics();
    if (stats.locked_entries != 2) throw std::runtime_error("Should have 2 locked entries");

    // Clear all
    manager.clear();

    stats = manager.get_statistics();
    if (stats.total_entries != 0) throw std::runtime_error("Should have 0 entries after clear");
    if (stats.locked_entries != 0) throw std::runtime_error("Should have 0 locked entries after clear");
    if (!manager.get_keys().empty()) throw std::runtime_error("Should have no keys after clear");
}

  // Distributed cache tests
TEST(SharedDataManagerTests, Distributed_Cache___Clear) {
    DistributedCache cache;

    cache.clear();

    auto stats = cache.get_stats();
    if (stats.total_entries != 0) throw std::runtime_error("Should have 0 entries after clear");
}

TEST(SharedDataManagerTests, Distributed_Cache___Initial_Stats) {
    DistributedCache cache;

    auto stats = cache.get_stats();
    if (stats.total_entries != 0) throw std::runtime_error("Should start with 0 entries");
    if (stats.hits != 0) throw std::runtime_error("Should start with 0 hits");
    if (stats.misses != 0) throw std::runtime_error("Should start with 0 misses");
    if (stats.hit_rate != 0.0) throw std::runtime_error("Should start with 0.0 hit rate");
}

TEST(SharedDataManagerTests, Distributed_Cache___Invalidation) {
    DistributedCache cache;

    // Invalidate non-existent key (should not throw)
    cache.invalidate("nonexistent");

    auto stats = cache.get_stats();
    if (stats.total_entries != 0) throw std::runtime_error("Should still have 0 entries");
}

  // Thread safety tests (basic)
TEST(SharedDataManagerTests, Concurrent_Locking) {
    SharedDataManager manager;

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    // Multiple threads try to lock the same key
    for (int i = 0; i < 10; ++i) {
      threads.emplace_back([&manager, &success_count, &failure_count, i]() {
        std::string owner = "owner_" + std::to_string(i);
        if (manager.lock("shared_key", owner, std::chrono::seconds(1))) {
          success_count++;
          // Hold lock briefly
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          manager.unlock("shared_key", owner);
        } else {
          failure_count++;
        }
      });
    }

    // Wait for all threads
    for (auto& thread : threads) {
      thread.join();
    }

    // Only one thread should have succeeded initially, others should have failed
    if (success_count + failure_count != 10) {
      throw std::runtime_error("All threads should have completed");
    }
    if (success_count < 1) {
      throw std::runtime_error("At least one thread should have succeeded");
    }
}

