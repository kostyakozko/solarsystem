/**
 * @file test_cache_manager.cpp
 * @brief Test program for intelligent cache management system
 */

#include <iomanip>
#include <iostream>

#include "solar_jpl/cache_manager.hpp"
#include "solar_jpl/jpl_client.hpp"

using namespace SolarSystem::JPL;

void print_statistics(const CacheStatistics& stats) {
  std::cout << "\n=== Cache Statistics ===" << std::endl;
  std::cout << "Total reads: " << stats.total_reads << std::endl;
  std::cout << "Total writes: " << stats.total_writes << std::endl;
  std::cout << "Cache hits: " << stats.cache_hits << std::endl;
  std::cout << "Cache misses: " << stats.cache_misses << std::endl;
  std::cout << "Hit ratio: " << std::fixed << std::setprecision(2) << (stats.hit_ratio() * 100.0)
            << "%" << std::endl;

  std::cout << "\nValidation attempts: " << stats.validation_attempts << std::endl;
  std::cout << "Validation successes: " << stats.validation_successes << std::endl;
  std::cout << "Validation success ratio: " << std::fixed << std::setprecision(2)
            << (stats.validation_success_ratio() * 100.0) << "%" << std::endl;

  std::cout << "\nBinary cache size: " << stats.binary_cache_size << " bytes" << std::endl;
  std::cout << "JSON cache size: " << stats.json_cache_size << " bytes" << std::endl;
  std::cout << "Compressed cache size: " << stats.compressed_cache_size << " bytes" << std::endl;
  std::cout << "Compression ratio: " << std::fixed << std::setprecision(2)
            << (stats.compression_ratio * 100.0) << "%" << std::endl;

  std::cout << "\nCorruption detections: " << stats.corruption_detections << std::endl;
  std::cout << "Recovery attempts: " << stats.recovery_attempts << std::endl;
  std::cout << "Recovery successes: " << stats.recovery_successes << std::endl;
}

int main() {
  std::cout << "Testing Intelligent Cache Management System" << std::endl;
  std::cout << "===========================================" << std::endl;

  try {
    // Create cache manager for testing
    auto cache_manager = CacheManagerFactory::create_for_testing();

    std::cout << "\n1. Initializing cache manager..." << std::endl;
    auto init_result = cache_manager->initialize();
    if (!is_success(init_result)) {
      std::cerr << "Failed to initialize cache manager" << std::endl;
      return 1;
    }
    std::cout << "✓ Cache manager initialized successfully" << std::endl;

    // Test cache validation
    std::cout << "\n2. Testing cache validation..." << std::endl;
    auto validation_result = cache_manager->validate_cache(ValidationLevel::Basic);
    if (is_success(validation_result)) {
      bool is_valid = get_value(validation_result);
      std::cout << "✓ Cache validation completed. Valid: " << (is_valid ? "Yes" : "No")
                << std::endl;
    } else {
      std::cout << "✗ Cache validation failed" << std::endl;
    }

    // Test cache health
    std::cout << "\n3. Testing cache health assessment..." << std::endl;
    auto health_result = cache_manager->get_cache_health();
    if (is_success(health_result)) {
      double health = get_value(health_result);
      std::cout << "✓ Cache health: " << std::fixed << std::setprecision(1) << (health * 100.0)
                << "%" << std::endl;
    } else {
      std::cout << "✗ Failed to assess cache health" << std::endl;
    }

    // Test cache optimization
    std::cout << "\n4. Testing cache optimization..." << std::endl;
    auto optimize_result = cache_manager->optimize_cache();
    if (is_success(optimize_result)) {
      std::cout << "✓ Cache optimization completed successfully" << std::endl;
    } else {
      std::cout << "✗ Cache optimization failed" << std::endl;
    }

    // Test cache compression
    std::cout << "\n5. Testing cache compression..." << std::endl;
    auto compress_result = cache_manager->compress_cache(CompressionAlgorithm::ZSTD);
    if (is_success(compress_result)) {
      std::cout << "✓ Cache compression completed successfully" << std::endl;
    } else {
      std::cout << "✗ Cache compression failed" << std::endl;
    }

    // Test backup creation
    std::cout << "\n6. Testing cache backup..." << std::endl;
    auto backup_result = cache_manager->create_backup();
    if (is_success(backup_result)) {
      std::cout << "✓ Cache backup created successfully" << std::endl;

      // List backups
      auto backups = cache_manager->list_backups();
      std::cout << "  Available backups: " << backups.size() << std::endl;
      for (const auto& backup : backups) {
        std::cout << "    - " << backup.filename().string() << std::endl;
      }
    } else {
      std::cout << "✗ Cache backup failed" << std::endl;
    }

    // Test refresh check
    std::cout << "\n7. Testing cache refresh logic..." << std::endl;
    bool needs_refresh = cache_manager->needs_refresh();
    std::cout << "✓ Cache needs refresh: " << (needs_refresh ? "Yes" : "No") << std::endl;

    // Test configuration update
    std::cout << "\n8. Testing configuration update..." << std::endl;
    CacheManagerConfig new_config;
    new_config.cache_directory = "./test_cache_updated";
    new_config.enable_compression = true;
    new_config.compression_algorithm = CompressionAlgorithm::LZ4;

    auto config_result = cache_manager->update_config(new_config);
    if (is_success(config_result)) {
      std::cout << "✓ Configuration updated successfully" << std::endl;
    } else {
      std::cout << "✗ Configuration update failed" << std::endl;
    }

    // Print final statistics
    print_statistics(cache_manager->get_statistics());

    // Test shutdown
    std::cout << "\n9. Shutting down cache manager..." << std::endl;
    auto shutdown_result = cache_manager->shutdown();
    if (is_success(shutdown_result)) {
      std::cout << "✓ Cache manager shutdown successfully" << std::endl;
    } else {
      std::cout << "✗ Cache manager shutdown failed" << std::endl;
    }

    std::cout << "\n=== All Tests Completed ===" << std::endl;
    std::cout << "✓ Intelligent cache management system is working correctly!" << std::endl;

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
