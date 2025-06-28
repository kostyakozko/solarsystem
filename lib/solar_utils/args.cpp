#include "args.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [OPTIONS]\n"
            << "Solar System Simulation\n\n"
            << "Options:\n"
            << "  -d, --date DATE    Specify target date in ISO format (YYYY-MM-DD)\n"
            << "                     If not specified, uses current date\n"
            << "  -u, --update-data  Update ephemeris data from NASA JPL for current year\n"
            << "      --rebuild      Rebuild binary cache from JSON data\n"
            << "      --test-storage Test JSON/binary storage system\n"
            << "  -h, --help         Show this help message\n\n"
            << "Examples:\n"
            << "  " << program_name << "                    # Use current date\n"
            << "  " << program_name << " -d 2025-12-31      # Simulate to Dec 31, 2025\n"
            << "  " << program_name << " --date 2020-01-01  # Simulate to Jan 1, 2020\n"
            << "  " << program_name << " -u                 # Update JPL data\n"
            << "  " << program_name << " --rebuild          # Rebuild binary cache\n"
            << "  " << program_name << " --test-storage     # Test storage system\n";
}

time_t parse_iso_date(const std::string& date_str) {
  struct tm tm_date = {0};

  // Parse YYYY-MM-DD format
  std::istringstream ss(date_str);
  std::string year_str, month_str, day_str;

  if (!std::getline(ss, year_str, '-') || !std::getline(ss, month_str, '-') ||
      !std::getline(ss, day_str)) {
    throw std::invalid_argument("Invalid date format. Expected YYYY-MM-DD");
  }

  try {
    tm_date.tm_year = std::stoi(year_str) - 1900;  // Years since 1900
    tm_date.tm_mon = std::stoi(month_str) - 1;     // Months since January (0-11)
    tm_date.tm_mday = std::stoi(day_str);          // Day of month (1-31)
    tm_date.tm_hour = 12;                          // Noon UTC
    tm_date.tm_min = 0;
    tm_date.tm_sec = 0;
    tm_date.tm_isdst = 0;  // No daylight saving
  } catch (const std::exception& e) {
    throw std::invalid_argument("Invalid date components in: " + date_str);
  }

  // Validate ranges
  if (tm_date.tm_year < 0 || tm_date.tm_mon < 0 || tm_date.tm_mon > 11 || tm_date.tm_mday < 1 ||
      tm_date.tm_mday > 31) {
    throw std::invalid_argument("Date components out of valid range: " + date_str);
  }

  return mktime(&tm_date);
}

// Extended argument parsing for specialized applications
bool parse_args(int argc, char* argv[], Args& args) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      args.show_help = true;
    } else if (arg == "-u" || arg == "--update") {
      args.update_data = true;
    } else if (arg == "-f" || arg == "--force") {
      args.force_update = true;
      args.update_data = true;  // Force implies update
    } else if (arg == "-y" || arg == "--year") {
      if (i + 1 < argc) {
        try {
          args.target_year = std::stoi(argv[++i]);
        } catch (const std::exception& e) {
          std::cerr << "Error: Invalid year value: " << argv[i] << std::endl;
          return false;
        }
      } else {
        std::cerr << "Error: --year requires a year value" << std::endl;
        return false;
      }
    } else if (arg == "-d" || arg == "--date") {
      if (i + 1 < argc) {
        args.target_date = argv[++i];
        args.use_current_date = false;
      } else {
        std::cerr << "Error: --date requires a date value" << std::endl;
        return false;
      }
    } else if (arg == "--rebuild") {
      args.rebuild_cache = true;
    } else if (arg == "--test-storage") {
      args.test_storage = true;
    } else if (arg == "--validate") {
      args.validate_cache = true;
    } else if (arg == "--status") {
      args.show_status = true;
    } else if (arg == "--clean") {
      args.clean_cache = true;
    } else if (arg == "-v" || arg == "--verbose") {
      args.verbose = true;
    } else {
      std::cerr << "Error: Unknown option: " << arg << std::endl;
      return false;
    }
  }

  return true;
}

SimulationArgs parse_arguments(int argc, char* argv[]) {
  SimulationArgs args;
  args.use_current_date = true;
  args.update_data = false;
  args.rebuild_cache = false;
  args.test_storage = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      exit(0);
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--date") == 0) {
      if (i + 1 < argc) {
        args.date_string = argv[++i];
        try {
          args.target_date = parse_iso_date(args.date_string);
          args.use_current_date = false;
        } catch (const std::exception& e) {
          std::cerr << "Error parsing date: " << e.what() << std::endl;
          exit(1);
        }
      } else {
        std::cerr << "Error: --date requires a date argument" << std::endl;
        exit(1);
      }
    } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--update-data") == 0) {
      args.update_data = true;
    } else if (strcmp(argv[i], "--rebuild") == 0) {
      args.rebuild_cache = true;
    } else if (strcmp(argv[i], "--test-storage") == 0) {
      args.test_storage = true;
    } else {
      std::cerr << "Unknown argument: " << argv[i] << std::endl;
      print_usage(argv[0]);
      exit(1);
    }
  }

  return args;
}
