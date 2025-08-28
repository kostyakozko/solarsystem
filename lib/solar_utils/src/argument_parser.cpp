/**
 * @file argument_parser.cpp
 * @brief Implementation of modern C++20 argument parsing system
 */

#include "solar_utils/argument_parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
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

  // Parse ISO format date (YYYY-MM-DD)
  std::regex iso_pattern(R"(^(\d{4})-(\d{2})-(\d{2})$)");
  std::smatch matches;

  if (std::regex_match(date_str, matches, iso_pattern)) {
    try {
      int year = std::stoi(matches[1]);
      int month = std::stoi(matches[2]);
      int day = std::stoi(matches[3]);

      // Create a time_t for the specified date using UTC
      std::tm tm = {};
      tm.tm_year = year - 1900;  // years since 1900
      tm.tm_mon = month - 1;     // months since January (0-11)
      tm.tm_mday = day;          // day of the month (1-31)
      tm.tm_hour = 12;           // Use noon to avoid timezone issues
      tm.tm_min = 0;
      tm.tm_sec = 0;
      tm.tm_isdst = 0;  // No daylight saving time

// Use timegm if available, otherwise mktime with UTC adjustment
#ifdef __APPLE__
      std::time_t time = timegm(&tm);
#else
      std::time_t time = mktime(&tm);
#endif
      if (time != -1) {
        return ArgumentResult<Date>{Date{time}};
      }
    } catch (const std::exception&) {
      // Fall through to error
    }
  }

  // Invalid date format
  return ArgumentResult<Date>{ArgumentError::InvalidDateFormat};
}

ArgumentResult<Date> Date::from_string_format(const std::string& date_str,
                                              const std::string& expected_format) {
  // Use the simple validation (expected_format is for future enhancement)
  (void)expected_format;  // Suppress unused parameter warning
  auto result = Validation::DateTimeValidator::validate_date(date_str);

  if (!result.is_valid) {
    return ArgumentResult<Date>{ArgumentError::InvalidDateFormat};
  }

  return ArgumentResult<Date>{Date{}};
}

ArgumentResult<Date> Date::from_string_with_range(
    const std::string& date_str, const std::chrono::system_clock::time_point& min_date,
    const std::chrono::system_clock::time_point& max_date) {
  auto result =
      Validation::DateTimeValidator::validate_date_with_range(date_str, min_date, max_date);

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

std::string ArgumentParser::contextual_help(const std::string& option_name) const {
  auto option_index = find_option(option_name);
  if (!option_index) {
    return "Option not found: " + option_name;
  }

  const auto& option = options_[*option_index];
  std::ostringstream oss;

  oss << "Option: " << option.long_name();
  if (!option.short_name().empty()) {
    oss << " (" << option.short_name() << ")";
  }
  oss << "\n";

  if (!option.description().empty()) {
    oss << "Description: " << option.description() << "\n";
  }

  if (option.requires_value()) {
    oss << "Requires: A value\n";
    oss << "Usage: " << program_name_ << " " << option.long_name() << " <value>\n";
  } else {
    oss << "Type: Flag (no value required)\n";
    oss << "Usage: " << program_name_ << " " << option.long_name() << "\n";
  }

  return oss.str();
}

ConflictReport ArgumentParser::detect_conflicts(
    const std::vector<std::string>& provided_args) const {
  ConflictReport report;

  // Check for mutually exclusive options
  std::vector<std::string> found_options;

  for (const auto& arg : provided_args) {
    if (arg.starts_with("-")) {
      auto option_index = find_option(arg);
      if (option_index) {
        found_options.push_back(arg);
      }
    }
  }

  // Define some common conflicts
  std::vector<std::pair<std::string, std::string>> known_conflicts = {{"--help", "--version"},
                                                                      {"--quiet", "--verbose"},
                                                                      {"--update", "--no-update"},
                                                                      {"--force", "--no-force"}};

  for (const auto& [opt1, opt2] : known_conflicts) {
    bool has_opt1 =
        std::find(found_options.begin(), found_options.end(), opt1) != found_options.end();
    bool has_opt2 =
        std::find(found_options.begin(), found_options.end(), opt2) != found_options.end();

    if (has_opt1 && has_opt2) {
      report.has_conflicts = true;
      report.conflicting_pairs.emplace_back(opt1, opt2);
      report.resolution_suggestions.push_back("Remove either " + opt1 + " or " + opt2);
    }
  }

  return report;
}

std::vector<std::string> ArgumentParser::find_similar_options(
    const std::string& invalid_option) const {
  std::vector<std::pair<std::string, int>> candidates;

  for (const auto& option : options_) {
    // Check both short and long names
    if (!option.short_name().empty()) {
      int distance = calculate_edit_distance(invalid_option, std::string(option.short_name()));
      if (distance <= 2) {  // Allow up to 2 character differences
        candidates.emplace_back(option.short_name(), distance);
      }
    }

    if (!option.long_name().empty()) {
      int distance = calculate_edit_distance(invalid_option, std::string(option.long_name()));
      if (distance <= 3) {  // Allow up to 3 character differences for longer names
        candidates.emplace_back(option.long_name(), distance);
      }
    }
  }

  // Sort by edit distance (closest first)
  std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

  std::vector<std::string> similar_options;
  for (const auto& [option, distance] : candidates) {
    similar_options.push_back(option);
    if (similar_options.size() >= 3) break;  // Limit to top 3 suggestions
  }

  return similar_options;
}

std::vector<std::string> ArgumentParser::generate_usage_examples() const {
  std::vector<std::string> examples;

  // Generate basic usage
  examples.push_back(program_name_ + " --help");

  // Find some common options and create examples
  for (const auto& option : options_) {
    if (option.requires_value()) {
      if (option.long_name().find("date") != std::string::npos) {
        examples.push_back(program_name_ + " " + std::string(option.long_name()) + " 2025-01-01");
      } else if (option.long_name().find("port") != std::string::npos) {
        examples.push_back(program_name_ + " " + std::string(option.long_name()) + " 8080");
      } else if (option.long_name().find("file") != std::string::npos) {
        examples.push_back(program_name_ + " " + std::string(option.long_name()) +
                           " /path/to/file");
      }
    } else {
      if (option.long_name().find("verbose") != std::string::npos) {
        examples.push_back(program_name_ + " " + std::string(option.long_name()));
      }
    }

    if (examples.size() >= 5) break;  // Limit examples
  }

  return examples;
}

std::vector<std::string> ArgumentParser::get_intelligent_suggestions(
    const std::string& invalid_arg) const {
  std::vector<std::string> suggestions;

  // Check for common typos
  if (invalid_arg == "-h" || invalid_arg == "help") {
    suggestions.push_back("--help");
  } else if (invalid_arg == "-v" || invalid_arg == "verbose") {
    suggestions.push_back("--verbose");
  } else if (invalid_arg.find("date") != std::string::npos) {
    suggestions.push_back("--date");
  } else if (invalid_arg.find("update") != std::string::npos) {
    suggestions.push_back("--update");
  }

  // Add similar options
  auto similar = find_similar_options(invalid_arg);
  suggestions.insert(suggestions.end(), similar.begin(), similar.end());

  // Remove duplicates
  std::sort(suggestions.begin(), suggestions.end());
  suggestions.erase(std::unique(suggestions.begin(), suggestions.end()), suggestions.end());

  return suggestions;
}

int ArgumentParser::calculate_edit_distance(const std::string& s1, const std::string& s2) const {
  const size_t len1 = s1.size();
  const size_t len2 = s2.size();

  std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

  // Initialize base cases
  for (size_t i = 0; i <= len1; ++i) dp[i][0] = static_cast<int>(i);
  for (size_t j = 0; j <= len2; ++j) dp[0][j] = static_cast<int>(j);

  // Fill the DP table
  for (size_t i = 1; i <= len1; ++i) {
    for (size_t j = 1; j <= len2; ++j) {
      if (std::tolower(s1[i - 1]) == std::tolower(s2[j - 1])) {
        dp[i][j] = dp[i - 1][j - 1];
      } else {
        dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
      }
    }
  }

  return dp[len1][len2];
}

void ArgumentParser::add_conflict_rule(const std::string& option1, const std::string& option2) {
  conflicting_options_[option1].push_back(option2);
  conflicting_options_[option2].push_back(option1);
}

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
      std::cerr << "Validation Error for option " << arg << ": " << validation_result.error_message
                << std::endl;

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
    option.execute(validation_result.normalized_value.empty() ? value
                                                              : validation_result.normalized_value);
    return ArgumentResult<size_t>{index + 2};  // Skip both option and value
  } else {
    option.execute();
    return ArgumentResult<size_t>{index + 1};  // Skip just the option
  }
}

DetailedArgumentResult<void> ArgumentParser::parse_with_details(std::span<const char* const> args) {
  for (size_t i = 1; i < args.size(); /* increment handled in loop */) {
    auto result = parse_argument_detailed(args, i);
    if (!result) {
      return DetailedArgumentResult<void>{result.error()};
    }
    i = result.value();
  }
  return DetailedArgumentResult<void>{};  // Success
}

DetailedArgumentResult<void> ArgumentParser::parse_with_details(int argc,
                                                                const char* const argv[]) {
  return parse_with_details(std::span<const char* const>(argv, static_cast<size_t>(argc)));
}

DetailedArgumentResult<size_t> ArgumentParser::parse_argument_detailed(
    std::span<const char* const> args, size_t index) {
  std::string_view arg = args[index];
  std::string arg_str(arg);

  auto option_index = find_option(arg);
  if (!option_index) {
    DetailedArgumentError error(ArgumentError::UnknownOption, "Unknown option: " + arg_str,
                                "Option '" + arg_str + "' is not recognized");

    // Add intelligent suggestions
    error.similar_options = find_similar_options(arg_str);
    error.suggestions = get_intelligent_suggestions(arg_str);

    // Add contextual help
    if (!error.similar_options.empty()) {
      error.help_text = "Did you mean one of: " + error.similar_options[0] + "?";
      error.usage_example = contextual_help(error.similar_options[0]);
    } else {
      error.help_text = "Use --help to see all available options";
      auto examples = generate_usage_examples();
      if (!examples.empty()) {
        error.usage_example = examples[0];
      }
    }

    return DetailedArgumentResult<size_t>{error};
  }

  const auto& option = options_[*option_index];

  if (option.requires_value()) {
    if (index + 1 >= args.size()) {
      DetailedArgumentError error(
          ArgumentError::MissingValue, "Missing value for option: " + arg_str,
          "Option '" + arg_str + "' requires a value but none was provided");

      error.help_text = contextual_help(arg_str);
      error.usage_example = program_name_ + " " + arg_str + " <value>";

      return DetailedArgumentResult<size_t>{error};
    }

    std::string value = args[index + 1];

    // Use comprehensive validation if available
    auto validation_result = option.validate_comprehensive_value(value);
    if (!validation_result.is_valid) {
      DetailedArgumentError error(
          ArgumentError::ValidationFailed,
          "Invalid value for option " + arg_str + ": " + validation_result.error_message,
          "The value '" + value + "' is not valid for option '" + arg_str + "'");

      error.suggestions = validation_result.suggestions;
      error.help_text = contextual_help(arg_str);

      if (!validation_result.expected_formats.empty()) {
        error.usage_example = "Expected formats: " + validation_result.expected_formats[0];
      }

      return DetailedArgumentResult<size_t>{error};
    }

    // Use the validated (and potentially sanitized) value
    option.execute(validation_result.normalized_value.empty() ? value
                                                              : validation_result.normalized_value);
    return DetailedArgumentResult<size_t>{index + 2};  // Skip both option and value
  } else {
    option.execute();
    return DetailedArgumentResult<size_t>{index + 1};  // Skip just the option
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

  parser_.add_option(Option("", "--body-set", "Select standardized body set (essential|important|complete)")
                         .requires_value()
                         .action([this](const std::optional<std::string>& value) {
                           if (value) {
                             std::string set = *value;
                             // Convert to lowercase for case-insensitive comparison
                             std::transform(set.begin(), set.end(), set.begin(), ::tolower);
                             if (set == "essential" || set == "important" || set == "complete") {
                               config_.body_set = set;
                             } else {
                               throw std::invalid_argument("Invalid body set. Use: essential, important, or complete");
                             }
                           }
                         }));
}

ArgumentResult<SimulationConfig> SimulationArgumentParser::parse(int argc,
                                                                 const char* const argv[]) {
  config_ = SimulationConfig{};  // Reset config

  // Try detailed parsing first for better error messages
  auto detailed_result = parser_.parse_with_details(argc, argv);
  if (!detailed_result) {
    const auto& error = detailed_result.error();

    // Print intelligent error information
    std::cerr << "Error: " << error.error_message << std::endl;

    if (!error.context.empty()) {
      std::cerr << "Context: " << error.context << std::endl;
    }

    if (!error.suggestions.empty()) {
      std::cerr << "Suggestions:" << std::endl;
      for (const auto& suggestion : error.suggestions) {
        std::cerr << "  - " << suggestion << std::endl;
      }
    }

    if (!error.similar_options.empty()) {
      std::cerr << "Similar options:" << std::endl;
      for (const auto& option : error.similar_options) {
        std::cerr << "  - " << option << std::endl;
      }
    }

    if (error.help_text.has_value()) {
      std::cerr << "Help: " << *error.help_text << std::endl;
    }

    if (error.usage_example.has_value()) {
      std::cerr << "Example: " << *error.usage_example << std::endl;
    }

    // Convert detailed error to simple error for return
    return ArgumentResult<SimulationConfig>{error.error_code};
  }

  // Check for conflicts
  std::vector<std::string> args_vec;
  for (int i = 1; i < argc; ++i) {
    args_vec.emplace_back(argv[i]);
  }

  auto conflict_report = parser_.detect_conflicts(args_vec);
  if (conflict_report.has_conflicts) {
    std::cerr << "Argument conflicts detected:" << std::endl;
    for (const auto& [opt1, opt2] : conflict_report.conflicting_pairs) {
      std::cerr << "  - " << opt1 << " conflicts with " << opt2 << std::endl;
    }
    for (const auto& suggestion : conflict_report.resolution_suggestions) {
      std::cerr << "  Suggestion: " << suggestion << std::endl;
    }
    return ArgumentResult<SimulationConfig>{ArgumentError::ConflictingOptions};
  }

  return ArgumentResult<SimulationConfig>{config_};
}

void SimulationArgumentParser::print_usage() const {
  std::cout << "Solar System Simulation\n\n";
  parser_.print_help();
  std::cout << "\nExamples:\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << "                         # Use current date (complete body set)\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " -d 2025-12-31           # Simulate to Dec 31, 2025\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " --body-set essential    # Fast simulation (9 bodies)\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " --body-set important    # Balanced simulation (18 bodies)\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " -u                      # Update JPL data\n";
  std::cout << "  " << parser_.help().substr(7, parser_.help().find(' ', 7) - 7)
            << " --rebuild               # Rebuild binary cache\n";
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

  // Try detailed parsing first for better error messages
  auto detailed_result = parser_.parse_with_details(argc, argv);
  if (!detailed_result) {
    const auto& error = detailed_result.error();

    // Print intelligent error information
    std::cerr << "Error: " << error.error_message << std::endl;

    if (!error.context.empty()) {
      std::cerr << "Context: " << error.context << std::endl;
    }

    if (!error.suggestions.empty()) {
      std::cerr << "Suggestions:" << std::endl;
      for (const auto& suggestion : error.suggestions) {
        std::cerr << "  - " << suggestion << std::endl;
      }
    }

    if (!error.similar_options.empty()) {
      std::cerr << "Similar options:" << std::endl;
      for (const auto& option : error.similar_options) {
        std::cerr << "  - " << option << std::endl;
      }
    }

    if (error.help_text.has_value()) {
      std::cerr << "Help: " << *error.help_text << std::endl;
    }

    if (error.usage_example.has_value()) {
      std::cerr << "Example: " << *error.usage_example << std::endl;
    }

    return ArgumentResult<ExtendedConfig>{error.error_code};
  }

  // Check for conflicts
  std::vector<std::string> args_vec;
  for (int i = 1; i < argc; ++i) {
    args_vec.emplace_back(argv[i]);
  }

  auto conflict_report = parser_.detect_conflicts(args_vec);
  if (conflict_report.has_conflicts) {
    std::cerr << "Argument conflicts detected:" << std::endl;
    for (const auto& [opt1, opt2] : conflict_report.conflicting_pairs) {
      std::cerr << "  - " << opt1 << " conflicts with " << opt2 << std::endl;
    }
    for (const auto& suggestion : conflict_report.resolution_suggestions) {
      std::cerr << "  Suggestion: " << suggestion << std::endl;
    }
    return ArgumentResult<ExtendedConfig>{ArgumentError::ConflictingOptions};
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

  parser_.add_option(Option("", "--body-set", "Select standardized body set (essential|important|complete)")
                         .requires_value()
                         .action([this](const std::optional<std::string>& value) {
                           if (value) {
                             std::string set = *value;
                             // Convert to lowercase for case-insensitive comparison
                             std::transform(set.begin(), set.end(), set.begin(), ::tolower);
                             if (set == "essential" || set == "important" || set == "complete") {
                               config_.body_set = set;
                             } else {
                               throw std::invalid_argument("Invalid body set. Use: essential, important, or complete");
                             }
                           }
                         }));
}

ArgumentResult<RealtimeConfig> RealtimeArgumentParser::parse(int argc, const char* const argv[]) {
  config_ = RealtimeConfig{};  // Reset config

  // Try detailed parsing first for better error messages
  auto detailed_result = parser_.parse_with_details(argc, argv);
  if (!detailed_result) {
    const auto& error = detailed_result.error();

    // Print intelligent error information
    std::cerr << "Error: " << error.error_message << std::endl;

    if (!error.context.empty()) {
      std::cerr << "Context: " << error.context << std::endl;
    }

    if (!error.suggestions.empty()) {
      std::cerr << "Suggestions:" << std::endl;
      for (const auto& suggestion : error.suggestions) {
        std::cerr << "  - " << suggestion << std::endl;
      }
    }

    if (!error.similar_options.empty()) {
      std::cerr << "Similar options:" << std::endl;
      for (const auto& option : error.similar_options) {
        std::cerr << "  - " << option << std::endl;
      }
    }

    if (error.help_text.has_value()) {
      std::cerr << "Help: " << *error.help_text << std::endl;
    }

    if (error.usage_example.has_value()) {
      std::cerr << "Example: " << *error.usage_example << std::endl;
    }

    return ArgumentResult<RealtimeConfig>{error.error_code};
  }

  // Check for conflicts
  std::vector<std::string> args_vec;
  for (int i = 1; i < argc; ++i) {
    args_vec.emplace_back(argv[i]);
  }

  auto conflict_report = parser_.detect_conflicts(args_vec);
  if (conflict_report.has_conflicts) {
    std::cerr << "Argument conflicts detected:" << std::endl;
    for (const auto& [opt1, opt2] : conflict_report.conflicting_pairs) {
      std::cerr << "  - " << opt1 << " conflicts with " << opt2 << std::endl;
    }
    for (const auto& suggestion : conflict_report.resolution_suggestions) {
      std::cerr << "  Suggestion: " << suggestion << std::endl;
    }
    return ArgumentResult<RealtimeConfig>{ArgumentError::ConflictingOptions};
  }

  // Validate configuration
  std::string validation_error;
  if (!config_.is_valid(&validation_error)) {
    std::cerr << "Configuration error: " << validation_error << std::endl;

    // Provide intelligent suggestions for configuration errors
    if (validation_error.find("interval") != std::string::npos) {
      std::cerr << "Suggestion: Use positive values for intervals (e.g., --update-interval 1)"
                << std::endl;
    }
    if (validation_error.find("duration") != std::string::npos) {
      std::cerr << "Suggestion: Use positive values for duration (e.g., --duration 60)"
                << std::endl;
    }

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
  std::cout << "  --body-set SET         Use standardized body set (essential|important|complete)\n";
  std::cout << "                         essential: Sun + 8 planets (9 bodies, fastest)\n";
  std::cout << "                         important: + major moons + dwarf planets (18 bodies, balanced)\n";
  std::cout << "                         complete: All available bodies (27 bodies, comprehensive)\n";
  std::cout << "                         Default: important\n";
  std::cout << "  --bodies LIST          Monitor specific bodies (comma-separated)\n";
  std::cout << "                         Example: --bodies Sun,Earth,Moon,Mars\n";
  std::cout << "                         Note: Overrides --body-set option\n\n";

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

SolarSystem::Utils::Validation::ValidationResult
SolarSystem::Utils::Option::validate_comprehensive_value(const std::string& value) const {
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
