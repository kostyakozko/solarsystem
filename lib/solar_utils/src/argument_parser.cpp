/**
 * @file argument_parser.cpp
 * @brief Implementation of modern C++20 argument parsing system
 */

#include "solar_utils/argument_parser.hpp"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace SolarSystem::Utils;

namespace SolarSystem::Utils {

std::string to_string(ArgumentError error) {
  switch (error) {
    case ArgumentError::UnknownOption:
      return "Unknown option";
    case ArgumentError::MissingValue:
      return "Missing value for option";
    case ArgumentError::InvalidValue:
      return "Invalid value for option";
    case ArgumentError::InvalidDateFormat:
      return "Invalid date format";
    case ArgumentError::DateOutOfRange:
      return "Date out of valid range";
    case ArgumentError::ConflictingOptions:
      return "Conflicting options specified";
    case ArgumentError::MissingRequiredOption:
      return "Missing required option";
    case ArgumentError::ValidationFailed:
      return "Validation failed";
    case ArgumentError::SanitizationFailed:
      return "Input sanitization failed";
    case ArgumentError::FormatNotSupported:
      return "Format not supported";
    default:
      return "Unknown error";
  }
}

// Enhanced Date implementation
ArgumentResult<Date> Date::from_string(const std::string& date_str) {
  auto result = Validation::DateTimeValidator::validate_date(date_str);

  if (!result.is_valid) {
    return ArgumentResult<Date>{ArgumentError::InvalidDateFormat};
  }

  // For now, just return current time since we simplified the API
  return ArgumentResult<Date>{Date{}};
}

ArgumentResult<Date> Date::from_string_format(const std::string& date_str,
                                              const std::string& expected_format) {
  // Use the simple validation
  auto result = Validation::DateTimeValidator::validate_date(date_str);

  if (!result.is_valid) {
    return ArgumentResult<Date>{ArgumentError::InvalidDateFormat};
  }

  return ArgumentResult<Date>{Date{}};
}

ArgumentResult<Date> Date::from_string_with_range(const std::string& date_str,
                                                  const std::chrono::system_clock::time_point& min_date,
                                                  const std::chrono::system_clock::time_point& max_date) {
  auto result = Validation::DateTimeValidator::validate_date_with_range(date_str, min_date, max_date);

  if (!result.is_valid) {
    return ArgumentResult<Date>{ArgumentError::DateOutOfRange};
  }

  return ArgumentResult<Date>{Date{}};
}

std::vector<std::string> Date::get_supported_formats() {
  return Validation::DateTimeValidator::get_supported_formats();
}

Date Date::now() { return Date{std::chrono::system_clock::now()}; }

std::time_t Date::to_time_t() const { return std::chrono::system_clock::to_time_t(time_point_); }

std::string Date::to_string() const {
  auto time_t_val = to_time_t();
  std::tm* tm_ptr = std::gmtime(&time_t_val);
  if (!tm_ptr) {
    return "Invalid Date";
  }

  std::ostringstream oss;
  oss << std::put_time(tm_ptr, "%Y-%m-%d");
  return oss.str();
}

// SimulationConfig implementation
Date SimulationConfig::get_target_date() const { return target_date.value_or(Date::now()); }

// ExtendedConfig implementation
Date ExtendedConfig::get_target_date() const { return target_date.value_or(Date::now()); }

// Option implementation
bool Option::matches(std::string_view arg) const { return arg == short_name_ || arg == long_name_; }

// Option methods are implemented at the end of the file

void Option::execute(const std::optional<std::string>& value) const {
  if (action_) {
    action_(value);
  }
}

// ArgumentParser implementation
ArgumentParser& ArgumentParser::add_option(Option option) {
  size_t index = options_.size();

  // Map both short and long names to the option index
  if (!option.short_name().empty()) {
    option_map_[std::string(option.short_name())] = index;
  }
  if (!option.long_name().empty()) {
    option_map_[std::string(option.long_name())] = index;
  }

  options_.emplace_back(std::move(option));
  return *this;
}

ArgumentResult<void> ArgumentParser::parse(std::span<const char* const> args) {
  for (size_t i = 1; i < args.size(); /* increment handled in loop */) {
    auto result = parse_argument(args, i);
    if (!result) {
      return ArgumentResult<void>{result.error()};
    }
    i = result.value();
  }
  return ArgumentResult<void>{};  // Success - default constructor
}

ArgumentResult<void> ArgumentParser::parse(int argc, const char* const argv[]) {
  return parse(std::span<const char* const>(argv, static_cast<size_t>(argc)));
}

std::string ArgumentParser::help() const {
  std::ostringstream oss;
  oss << "Usage: " << program_name_ << " [OPTIONS]\n\n";
  oss << "Options:\n";

  for (const auto& option : options_) {
    oss << "  ";
    if (!option.short_name().empty()) {
      oss << option.short_name();
      if (!option.long_name().empty()) {
        oss << ", ";
      }
    }
    if (!option.long_name().empty()) {
      oss << option.long_name();
    }
    if (option.requires_value()) {
      oss << " VALUE";
    }
    oss << "\n";

    if (!option.description().empty()) {
      oss << "      " << option.description() << "\n";
    }
    oss << "\n";
  }

  return oss.str();
}

void ArgumentParser::print_help() const { std::cout << help(); }

std::optional<size_t> ArgumentParser::find_option(std::string_view name) const {
  auto it = option_map_.find(std::string(name));
  return it != option_map_.end() ? std::make_optional(it->second) : std::nullopt;
}

ArgumentResult<size_t> ArgumentParser::parse_argument(std::span<const char* const> args,
                                                      size_t index) {
  std::string_view arg = args[index];

  auto option_index = find_option(arg);
  if (!option_index) {
    return ArgumentResult<size_t>{ArgumentError::UnknownOption};
  }

  const auto& option = options_[*option_index];

  if (option.requires_value()) {
    if (index + 1 >= args.size()) {
      return ArgumentResult<size_t>{ArgumentError::MissingValue};
    }

    std::string value = args[index + 1];

    // Use comprehensive validation if available
    auto validation_result = option.validate_comprehensive_value(value);
    if (!validation_result.is_valid) {
      // Print detailed error message
      std::cerr << "Validation Error for option " << arg << ": "
                << validation_result.error_message << std::endl;

      // Provide suggestions if available
      if (!validation_result.suggestions.empty()) {
        std::cerr << "Suggestions:" << std::endl;
        for (const auto& suggestion : validation_result.suggestions) {
          std::cerr << "  - " << suggestion << std::endl;
        }
      }

      if (!validation_result.expected_formats.empty()) {
        std::cerr << "Expected formats:" << std::endl;
        for (const auto& format : validation_result.expected_formats) {
          std::cerr << "  - " << format << std::endl;
        }
      }

      return ArgumentResult<size_t>{ArgumentError::ValidationFailed};
    }

    // Use the validated (and potentially sanitized) value
    option.execute(validation_result.normalized_value.empty() ? value : validation_result.normalized_value);
    return ArgumentResult<size_t>{index + 2};  // Skip both option and value
  } else {
    option.execute();
    return ArgumentResult<size_t>{index + 1};  // Skip just the option
  }
}

// SimulationArgumentParser implementation
SimulationArgumentParser::SimulationArgumentParser(std::string_view program_name)
    : parser_(program_name) {
  parser_.add_option(
      Option("-h", "--help", "Show this help message").as_flag().action([this](const auto&) {
        print_usage();
        std::exit(0);
      }));

  parser_.add_option(
      Option("-d", "--date", "Specify target date in ISO format (YYYY-MM-DD)")
          .requires_value()
          .validate([](const std::string& value) { return Date::from_string(value).has_value(); })
          .action([this](const std::optional<std::string>& value) {
            if (value) {
              auto date_result = Date::from_string(*value);
              if (date_result) {
                config_.target_date = date_result.value();
                config_.date_string = *value;
                config_.use_current_date = false;
              }
            }
          }));

  parser_.add_option(Option("-u", "--update-data", "Update ephemeris data from NASA JPL")
                         .as_flag()
                         .action([this](const auto&) { config_.update_data = true; }));

  parser_.add_option(Option("", "--rebuild", "Rebuild binary cache from JSON data")
                         .as_flag()
                         .action([this](const auto&) { config_.rebuild_cache = true; }));

  parser_.add_option(Option("", "--test-storage", "Test JSON/binary storage system")
                         .as_flag()
                         .action([this](const auto&) { config_.test_storage = true; }));

  parser_.add_option(
      Option("-v", "--verbose", "Enable verbose output").as_flag().action([this](const auto&) {
        config_.verbose = true;
      }));
}

ArgumentResult<SimulationConfig> SimulationArgumentParser::parse(int argc,
                                                                 const char* const argv[]) {
  config_ = SimulationConfig{};  // Reset config

  auto result = parser_.parse(argc, argv);
  if (!result) {
    return ArgumentResult<SimulationConfig>{result.error()};
  }

  return ArgumentResult<SimulationConfig>{config_};
}

void SimulationArgumentParser::print_usage() const {
  std::cout << "Solar System Simulation\n\n";
  parser_.print_help();
  std::cout << "\nExamples:\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << "                    # Use current date\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " -d 2025-12-31      # Simulate to Dec 31, 2025\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " --date 2020-01-01  # Simulate to Jan 1, 2020\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " -u                 # Update JPL data\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " --rebuild          # Rebuild binary cache\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " --test-storage     # Test storage system\n";
}

// ExtendedArgumentParser implementation
ExtendedArgumentParser::ExtendedArgumentParser(std::string_view program_name)
    : parser_(program_name) {
  parser_.add_option(
      Option("-h", "--help", "Show this help message").as_flag().action([this](const auto&) {
        config_.show_help = true;
      }));

  parser_.add_option(
      Option("-d", "--date", "Specify target date in ISO format (YYYY-MM-DD)")
          .requires_value()
          .validate([](const std::string& value) { return Date::from_string(value).has_value(); })
          .action([this](const std::optional<std::string>& value) {
            if (value) {
              auto date_result = Date::from_string(*value);
              if (date_result) {
                config_.target_date = date_result.value();
                config_.use_current_date = false;
              }
            }
          }));

  parser_.add_option(Option("-u", "--update", "Update ephemeris data from NASA JPL")
                         .as_flag()
                         .action([this](const auto&) { config_.update_data = true; }));

  parser_.add_option(Option("-f", "--force", "Force update (bypass smart caching)")
                         .as_flag()
                         .action([this](const auto&) {
                           config_.force_update = true;
                           config_.update_data = true;  // Force implies update
                         }));

  parser_.add_option(Option("-y", "--year", "Specify target year for data operations")
                         .requires_value()
                         .validate([](const std::string& value) {
                           try {
                             int year = std::stoi(value);
                             return year >= 1000 && year <= 3000;  // Reasonable range
                           } catch (...) {
                             return false;
                           }
                         })
                         .action([this](const auto& value) {
                           if (value) {
                             config_.target_year = std::stoi(*value);
                           }
                         }));

  parser_.add_option(Option("", "--rebuild", "Rebuild binary cache from JSON data")
                         .as_flag()
                         .action([this](const auto&) { config_.rebuild_cache = true; }));

  parser_.add_option(Option("", "--test-storage", "Test JSON/binary storage system")
                         .as_flag()
                         .action([this](const auto&) { config_.test_storage = true; }));

  parser_.add_option(
      Option("", "--validate", "Validate cache integrity").as_flag().action([this](const auto&) {
        config_.validate_cache = true;
      }));

  parser_.add_option(
      Option("", "--status", "Show system status").as_flag().action([this](const auto&) {
        config_.show_status = true;
      }));

  parser_.add_option(
      Option("", "--clean", "Clean cache files").as_flag().action([this](const auto&) {
        config_.clean_cache = true;
      }));

  parser_.add_option(
      Option("-v", "--verbose", "Enable verbose output").as_flag().action([this](const auto&) {
        config_.verbose = true;
      }));
}

ArgumentResult<ExtendedConfig> ExtendedArgumentParser::parse(int argc, const char* const argv[]) {
  config_ = ExtendedConfig{};  // Reset config

  auto result = parser_.parse(argc, argv);
  if (!result) {
    return ArgumentResult<ExtendedConfig>{result.error()};
  }

  return ArgumentResult<ExtendedConfig>{config_};
}

void ExtendedArgumentParser::print_usage() const { parser_.print_help(); }

// RealtimeConfig implementation
bool RealtimeConfig::is_valid(std::string* error) const {
  using namespace std::chrono_literals;

  if (update_interval <= 0s) {
    if (error) *error = "Update interval must be positive";
    return false;
  }

  if (display_interval <= 0s) {
    if (error) *error = "Display interval must be positive";
    return false;
  }

  if (duration_limit.has_value() && *duration_limit <= 0s) {
    if (error) *error = "Duration limit must be positive";
    return false;
  }

  return true;
}

// RealtimeArgumentParser implementation
RealtimeArgumentParser::RealtimeArgumentParser(std::string_view program_name)
    : parser_(program_name), program_name_(program_name) {
  parser_.add_option(
      Option("-h", "--help", "Show this help message").as_flag().action([this](const auto&) {
        print_usage();
        std::exit(0);
      }));

  parser_.add_option(Option("", "--positions", "Show celestial body positions (default)")
                         .as_flag()
                         .action([this](const auto&) { config_.show_positions = true; }));

  parser_.add_option(Option("", "--velocities", "Show velocity vectors in addition to positions")
                         .as_flag()
                         .action([this](const auto&) { config_.show_velocities = true; }));

  parser_.add_option(Option("", "--no-summary", "Hide monitoring summary information")
                         .as_flag()
                         .action([this](const auto&) { config_.show_summary = false; }));

  parser_.add_option(Option("", "--no-continuous", "Single snapshot mode (no continuous updates)")
                         .as_flag()
                         .action([this](const auto&) { config_.continuous_mode = false; }));

  parser_.add_option(Option("-q", "--quiet", "Minimal output (positions only)")
                         .as_flag()
                         .action([this](const auto&) { config_.quiet_mode = true; }));

  parser_.add_option(
      Option("-v", "--verbose", "Enable verbose logging").as_flag().action([this](const auto&) {
        config_.verbose_output = true;
      }));

  parser_.add_option(Option("", "--auto-fetch", "Auto-fetch current JPL data if needed")
                         .as_flag()
                         .action([this](const auto&) { config_.auto_fetch_data = true; }));

  parser_.add_option(
      Option("", "--update-interval", "Update simulation every N seconds (default: 1)")
          .requires_value()
          .validate([](const std::string& value) {
            try {
              int seconds = std::stoi(value);
              return seconds > 0;
            } catch (...) {
              return false;
            }
          })
          .action([this](const std::optional<std::string>& value) {
            if (value) {
              int seconds = std::stoi(*value);
              config_.update_interval = std::chrono::seconds(seconds);
            }
          }));

  parser_.add_option(Option("", "--display-interval", "Update display every N seconds (default: 1)")
                         .requires_value()
                         .validate([](const std::string& value) {
                           try {
                             int seconds = std::stoi(value);
                             return seconds > 0;
                           } catch (...) {
                             return false;
                           }
                         })
                         .action([this](const std::optional<std::string>& value) {
                           if (value) {
                             int seconds = std::stoi(*value);
                             config_.display_interval = std::chrono::seconds(seconds);
                           }
                         }));

  parser_.add_option(Option("", "--duration", "Stop monitoring after N seconds")
                         .requires_value()
                         .validate([](const std::string& value) {
                           try {
                             int seconds = std::stoi(value);
                             return seconds > 0;
                           } catch (...) {
                             return false;
                           }
                         })
                         .action([this](const std::optional<std::string>& value) {
                           if (value) {
                             int seconds = std::stoi(*value);
                             config_.duration_limit = std::chrono::seconds(seconds);
                           }
                         }));

  parser_.add_option(Option("", "--bodies", "Monitor specific bodies (comma-separated)")
                         .requires_value()
                         .action([this](const std::optional<std::string>& value) {
                           if (value) {
                             std::stringstream ss(*value);
                             std::string body;
                             config_.selected_bodies.clear();
                             while (std::getline(ss, body, ',')) {
                               // Trim whitespace
                               body.erase(0, body.find_first_not_of(" \t"));
                               body.erase(body.find_last_not_of(" \t") + 1);
                               if (!body.empty()) {
                                 config_.selected_bodies.push_back(body);
                               }
                             }
                           }
                         }));
}

ArgumentResult<RealtimeConfig> RealtimeArgumentParser::parse(int argc, const char* const argv[]) {
  config_ = RealtimeConfig{};  // Reset config

  auto result = parser_.parse(argc, argv);
  if (!result) {
    return ArgumentResult<RealtimeConfig>{result.error()};
  }

  // Validate configuration
  std::string error;
  if (!config_.is_valid(&error)) {
    std::cerr << "Configuration error: " << error << std::endl;
    return ArgumentResult<RealtimeConfig>{ArgumentError::InvalidValue};
  }

  return ArgumentResult<RealtimeConfig>{config_};
}

void RealtimeArgumentParser::print_usage() const {
  std::cout << "╭─────────────────────────────────────────────────────────╮\n";
  std::cout << "│       Solar System Real-Time Monitor (Modern)          │\n";
  std::cout << "│          Live Solar System Tracking & Display          │\n";
  std::cout << "╰─────────────────────────────────────────────────────────╯\n\n";

  std::cout << "Usage: " << program_name_ << " [OPTIONS]\n\n";

  std::cout << "🖥️  Display Options:\n";
  std::cout << "  --positions        Show celestial body positions (default)\n";
  std::cout << "  --velocities       Show velocity vectors in addition to positions\n";
  std::cout << "  --no-summary       Hide monitoring summary information\n";
  std::cout << "  --no-continuous    Single snapshot mode (no continuous updates)\n\n";

  std::cout << "⏱️  Timing Options:\n";
  std::cout << "  --update-interval N    Update simulation every N seconds (default: 1)\n";
  std::cout << "  --display-interval N   Update display every N seconds (default: 1)\n";
  std::cout << "  --duration N           Stop monitoring after N seconds\n\n";

  std::cout << "🌍 Body Selection:\n";
  std::cout << "  --bodies LIST          Monitor specific bodies (comma-separated)\n";
  std::cout << "                         Example: --bodies Sun,Earth,Moon,Mars\n";
  std::cout << "                         Default: Essential and important bodies\n\n";

  std::cout << "⚙️  Options:\n";
  std::cout << "  --auto-fetch           Auto-fetch current JPL data if needed\n";
  std::cout << "  -q, --quiet            Minimal output (positions only)\n";
  std::cout << "  -v, --verbose          Enable verbose logging\n";
  std::cout << "  -h, --help             Show this help message\n\n";

  std::cout << "💡 Examples:\n";
  std::cout << "  " << program_name_ << "                           # Basic real-time monitoring\n";
  std::cout << "  " << program_name_
            << " --velocities              # Show positions and velocities\n";
  std::cout << "  " << program_name_ << " --update-interval 5       # Update every 5 seconds\n";
  std::cout << "  " << program_name_ << " --duration 60             # Monitor for 1 minute\n";
  std::cout << "  " << program_name_ << " --bodies Sun,Earth,Moon   # Monitor specific bodies\n";
  std::cout << "  " << program_name_ << " --no-continuous           # Single snapshot\n";
  std::cout << "  " << program_name_
            << " --quiet --auto-fetch      # Minimal output with data fetch\n\n";

  std::cout << "🌟 Modern Features:\n";
  std::cout << "  • Beautiful real-time terminal interface with Unicode\n";
  std::cout << "  • Structured logging with colors and timestamps\n";
  std::cout << "  • Type-safe configuration with validation\n";
  std::cout << "  • Integration with Solar System Suite fluent APIs\n";
  std::cout << "  • Graceful shutdown handling (Ctrl+C)\n";
  std::cout << "  • RAII-based resource management\n";
}

}  // namespace SolarSystem::Utils

// Option class implementation
bool SolarSystem::Utils::Option::is_valid(const std::string& value) const {
  // Try comprehensive validation first
  if (!validation_type_.empty()) {
    auto result = validate_comprehensive_value(value);
    return result.is_valid;
  }

  // Fall back to legacy validator
  return !validator_ || validator_(value);
}

SolarSystem::Utils::Validation::ValidationResult SolarSystem::Utils::Option::validate_comprehensive_value(const std::string& value) const {
  if (!validation_type_.empty()) {
    if (validation_type_ == "choice" && !allowed_values_.empty()) {
      return Validation::StringValidator::validate_choice(value, allowed_values_);
    } else if (validation_type_ == "int") {
      return Validation::NumericValidator::validate_int(value, min_int_, max_int_);
    } else if (validation_type_ == "double") {
      return Validation::NumericValidator::validate_double(value, min_double_, max_double_);
    } else {
      return Validation::InputValidator::validate_argument("", value, validation_type_);
    }
  }

  // Fall back to legacy validator
  if (validator_ && !validator_(value)) {
    return Validation::ValidationResult("Value failed validation");
  }

  return Validation::ValidationResult(true, value);
}
