/**
 * Solar System Data Fetcher
 *
 * Dedicated utility for fetching and managing JPL HORIZONS ephemeris data.
 * This application handles all network operations and data caching,
 * allowing the main simulation to focus purely on computation.
 */

#include <ctime>
#include <iostream>
#include <string>

// Include our modular libraries
#include "args.h"
#include "jpl_data.h"

void print_fetch_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [OPTIONS]\n";
  std::cout << "Solar System Data Fetcher - JPL HORIZONS Data Management\n\n";
  std::cout << "Options:\n";
  std::cout << "  -u, --update       Update ephemeris data from NASA JPL for current year\n";
  std::cout << "  -y, --year YEAR    Update data for specific year (default: current year)\n";
  std::cout << "  -f, --force        Force update even if current year data exists\n";
  std::cout << "      --rebuild      Rebuild binary cache from JSON data\n";
  std::cout << "      --validate     Validate existing cache integrity\n";
  std::cout << "      --status       Show current cache status\n";
  std::cout << "      --test-storage Test JSON/binary storage system\n";
  std::cout << "      --clean        Remove all cache files\n";
  std::cout << "  -v, --verbose      Enable verbose output\n";
  std::cout << "  -h, --help         Show this help message\n\n";
  std::cout << "Examples:\n";
  std::cout << "  " << program_name << " --update              # Update current year data\n";
  std::cout << "  " << program_name << " --year 2024 --update  # Update 2024 data\n";
  std::cout << "  " << program_name << " --force --update      # Force update current year\n";
  std::cout << "  " << program_name << " --status              # Check cache status\n";
  std::cout << "  " << program_name << " --validate            # Validate cache\n";
  std::cout << "  " << program_name << " --clean               # Clean all cache\n";
}

void print_status() {
  std::cout << "=== Solar System Data Cache Status ===\n";

  if (has_current_ephemeris_data()) {
    time_t epoch = get_ephemeris_epoch();
    const char* source = get_ephemeris_source();
    const struct tm* tm_epoch = localtime(&epoch);
    int cached_year = tm_epoch->tm_year + 1900;

    std::cout << "✓ Cache Status: ACTIVE\n";
    std::cout << "  Data Source: " << source << "\n";
    std::cout << "  Cached Year: " << cached_year << "\n";
    std::cout << "  Last Updated: " << ctime(&epoch);

    // Check if current year
    time_t now = time(NULL);
    const struct tm* tm_now = localtime(&now);
    int current_year = tm_now->tm_year + 1900;

    if (cached_year == current_year) {
      std::cout << "  Status: ✓ UP TO DATE for " << current_year << "\n";
    } else {
      std::cout << "  Status: ⚠ OUTDATED (current year: " << current_year << ")\n";
    }
  } else {
    std::cout << "✗ Cache Status: NO CACHED DATA\n";
    std::cout << "  Using: Original hardcoded ephemeris data\n";
    std::cout << "  Recommendation: Run --update to fetch current JPL data\n";
  }

  std::cout << "\n";
}

bool clean_cache() {
  std::cout << "Cleaning ephemeris cache files...\n";

  // Remove cache files
  if (system("rm -f ephemeris_cache.bin ephemeris_data.json") == 0) {
    std::cout << "✓ Cache files removed successfully\n";
    return true;
  } else {
    std::cerr << "✗ Failed to remove cache files\n";
    return false;
  }
}

int main(int argc, char* argv[]) {
  // Parse command line arguments
  Args args;
  if (!parse_args(argc, argv, args)) {
    print_fetch_usage(argv[0]);
    return 1;
  }

  // Handle help
  if (args.show_help) {
    print_fetch_usage(argv[0]);
    return 0;
  }

  // Initialize JPL data system
  if (!initialize_jpl_data()) {
    std::cerr << "Failed to initialize JPL data system\n";
    return 1;
  }

  bool success = true;

  // Handle different operations
  if (args.show_status) {
    print_status();
  } else if (args.clean_cache) {
    success = clean_cache();
  } else if (args.validate_cache) {
    std::cout << "Validating cache integrity...\n";
    if (has_current_ephemeris_data()) {
      std::cout << "✓ Cache validation successful\n";
    } else {
      std::cout << "✗ Cache validation failed or no cache present\n";
      success = false;
    }
  } else if (args.test_storage) {
    std::cout << "Testing storage system...\n";
    success = test_storage_system();
    if (success) {
      std::cout << "✓ Storage system test passed\n";
    } else {
      std::cout << "✗ Storage system test failed\n";
    }
  } else if (args.rebuild_cache) {
    std::cout << "Rebuilding binary cache from JSON...\n";
    success = rebuild_binary_cache();
    if (success) {
      std::cout << "✓ Binary cache rebuilt successfully\n";
    } else {
      std::cout << "✗ Failed to rebuild binary cache\n";
    }
  } else if (args.update_data || args.force_update) {
    if (args.force_update) {
      std::cout << "Force updating ephemeris data...\n";
      success = force_update_ephemeris_data();
    } else {
      std::cout << "Updating ephemeris data...\n";
      success = update_ephemeris_data();
    }

    if (success) {
      std::cout << "✓ Ephemeris data update completed successfully\n";
      print_status();
    } else {
      std::cout << "✗ Ephemeris data update failed\n";
    }
  } else {
    // Default action: show status
    print_status();
    std::cout << "Use --help for available options\n";
  }

  return success ? 0 : 1;
}
