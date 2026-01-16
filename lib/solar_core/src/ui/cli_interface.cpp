/**
 * @file cli_interface.cpp
 * @brief Implementation of unified CLI interface
 */

#include "solar_core/ui/cli_interface.hpp"

#include <iostream>

namespace SolarSystem::UI {

CLIInterface::CLIInterface(const CLIConfig& config) : config_(config) {
  // Create progress indicator
  ProgressConfig progress_config;
  progress_config.style = config_.progress_style;
  progress_config.accessible_mode = config_.accessible_mode;
  progress_config.colored_output = config_.colored_output;
  progress_ = std::make_unique<ProgressIndicator>(progress_config);

  // Create status display
  StatusConfig status_config;
  status_config.accessible_mode = config_.accessible_mode;
  status_config.colored_output = config_.colored_output;
  status_config.compact_mode = config_.quiet;
  status_ = std::make_unique<StatusDisplay>(status_config);
}

void CLIInterface::initialize() {
  // Auto-detect accessibility needs
  auto& accessibility = AccessibilityManager::instance();
  accessibility.auto_detect();

  // Update configuration based on detection
  if (accessibility.is_screen_reader_mode()) {
    config_.accessible_mode = true;
    config_.colored_output = false;

    // Recreate components with updated config
    ProgressConfig progress_config;
    progress_config.accessible_mode = true;
    progress_config.colored_output = false;
    progress_ = std::make_unique<ProgressIndicator>(progress_config);

    StatusConfig status_config;
    status_config.accessible_mode = true;
    status_config.colored_output = false;
    status_ = std::make_unique<StatusDisplay>(status_config);
  }
}

void CLIInterface::show_banner(const std::string& app_name, const std::string& version,
                               const std::string& description) {
  if (config_.quiet) return;

  if (config_.accessible_mode) {
    std::cout << app_name << " version " << version << std::endl;
    if (!description.empty()) {
      std::cout << description << std::endl;
    }
    std::cout << std::endl;
    return;
  }

  std::vector<std::string> lines;
  lines.push_back(app_name + " v" + version);
  if (!description.empty()) {
    lines.push_back(description);
  }

  BoxDisplay::info("Solar System Suite", lines);
  std::cout << std::endl;
}

void CLIInterface::show_help(const std::string& usage, const std::vector<std::string>& options,
                             const std::vector<std::string>& examples) {
  std::cout << "Usage: " << usage << "\n\n";

  if (!options.empty()) {
    std::cout << "Options:\n";
    for (const auto& option : options) {
      std::cout << "  " << option << "\n";
    }
    std::cout << "\n";
  }

  if (!examples.empty()) {
    std::cout << "Examples:\n";
    for (const auto& example : examples) {
      std::cout << "  " << example << "\n";
    }
    std::cout << "\n";
  }

  // Show keyboard shortcuts if enabled
  if (KeyboardShortcuts::are_enabled()) {
    std::cout << "Press 'h' or '?' during operation to see keyboard shortcuts\n\n";
  }
}

void CLIInterface::success(const std::string& message) {
  if (!config_.quiet) {
    status_->success(message);
  }
}

void CLIInterface::info(const std::string& message) {
  if (!config_.quiet) {
    status_->info(message);
  }
}

void CLIInterface::warning(const std::string& message) { status_->warning(message); }

void CLIInterface::error(const std::string& message) { status_->error(message); }

void CLIInterface::debug(const std::string& message) {
  if (config_.verbose) {
    status_->debug(message);
  }
}

void CLIInterface::section(const std::string& title) {
  if (!config_.quiet) {
    status_->section(title);
  }
}

void CLIInterface::field(const std::string& key, const std::string& value) {
  if (!config_.quiet) {
    status_->field(key, value);
  }
}

void CLIInterface::list(const std::vector<std::string>& items) {
  if (!config_.quiet) {
    status_->list(items);
  }
}

void CLIInterface::table(const std::vector<std::string>& headers,
                         const std::vector<std::vector<std::string>>& rows) {
  if (config_.quiet) return;

  status_->table_header(headers);
  for (const auto& row : rows) {
    status_->table_row(row);
  }
}

bool CLIInterface::confirm(const std::string& message, bool default_yes) {
  if (config_.quiet) {
    return default_yes;  // Auto-confirm in quiet mode
  }

  std::cout << message << " [" << (default_yes ? "Y/n" : "y/N") << "]: ";
  std::string response;
  std::getline(std::cin, response);

  if (response.empty()) {
    return default_yes;
  }

  return (response == "y" || response == "Y" || response == "yes" || response == "Yes");
}

std::string CLIInterface::prompt(const std::string& message, const std::string& default_value) {
  if (config_.quiet && !default_value.empty()) {
    return default_value;  // Use default in quiet mode
  }

  if (!default_value.empty()) {
    std::cout << message << " [" << default_value << "]: ";
  } else {
    std::cout << message << ": ";
  }

  std::string response;
  std::getline(std::cin, response);

  if (response.empty() && !default_value.empty()) {
    return default_value;
  }

  return response;
}

void CLIInterface::show_shortcuts() { KeyboardShortcuts::display_shortcuts(); }

// CLIOperation implementation

CLIOperation::CLIOperation(CLIInterface& cli, const std::string& operation_name)
    : cli_(cli), operation_name_(operation_name) {
  if (cli_.config().show_progress && !cli_.is_quiet()) {
    cli_.progress().start(operation_name_);
  }
}

CLIOperation::~CLIOperation() {
  if (!completed_ && cli_.progress().is_active()) {
    cli_.progress().complete();
  }
}

void CLIOperation::update_progress(double progress) {
  if (cli_.config().show_progress && !cli_.is_quiet()) {
    cli_.progress().update(progress);
  }
}

void CLIOperation::update_progress(double progress, const std::string& message) {
  if (cli_.config().show_progress && !cli_.is_quiet()) {
    cli_.progress().update(progress, message);
  }
}

void CLIOperation::complete(const std::string& message) {
  completed_ = true;
  if (cli_.config().show_progress && !cli_.is_quiet()) {
    cli_.progress().complete(message);
  }
}

void CLIOperation::fail(const std::string& message) {
  completed_ = true;
  if (cli_.progress().is_active()) {
    cli_.progress().fail(message);
  }
}

}  // namespace SolarSystem::UI
