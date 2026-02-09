/**
 * @file analysis_cli.cpp
 * @brief Implementation of interactive analysis CLI
 */

#include "solar_analysis/analysis_cli.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace SolarSystem::Analysis {

struct AnalysisCLI::Impl {
  SessionState session;
  std::map<std::string, std::pair<std::string, CommandHandler>> commands;
  DataProcessor data_processor;
  OrbitalCalculator orbital_calculator;
  StatisticalAnalyzer statistical_analyzer;
  DataExporter data_exporter;
};

AnalysisCLI::AnalysisCLI() : impl_(std::make_unique<Impl>()) { register_builtin_commands(); }

AnalysisCLI::~AnalysisCLI() = default;

void AnalysisCLI::register_builtin_commands() {
  register_command("help", "Show help information", [this](const auto& args, auto&) {
    if (args.size() > 1) {
      return CommandResult{true, get_command_help(args[1]), ""};
    }
    return CommandResult{true, get_help(), ""};
  });

  register_command("load", "Load body data: load <body_name>",
                   [this](const auto& args, auto& session) {
                     if (args.size() < 2) {
                       return CommandResult{false, "", "Usage: load <body_name>"};
                     }
                     if (impl_->data_processor.load_body_data(args[1])) {
                       session.loaded_bodies.push_back(args[1]);
                       return CommandResult{true, "Loaded data for " + args[1], ""};
                     }
                     return CommandResult{false, "", "Failed to load data for " + args[1]};
                   });

  register_command("list", "List loaded bodies", [](const auto&, auto& session) {
    std::ostringstream oss;
    oss << "Loaded bodies (" << session.loaded_bodies.size() << "):\n";
    for (const auto& body : session.loaded_bodies) {
      oss << "  - " << body << "\n";
    }
    return CommandResult{true, oss.str(), ""};
  });

  register_command("orbital", "Calculate orbital elements: orbital <body_name>",
                   [this](const auto& args, auto&) {
                     if (args.size() < 2) {
                       return CommandResult{false, "", "Usage: orbital <body_name>"};
                     }
                     auto data = impl_->data_processor.get_data(args[1]);
                     if (data.empty()) {
                       return CommandResult{false, "", "No data for " + args[1]};
                     }
                     auto elem = impl_->orbital_calculator.calculate_elements(data.front());
                     std::ostringstream oss;
                     oss << "Orbital elements for " << args[1] << ":\n"
                         << "  Semi-major axis: " << elem.semi_major_axis << " km\n"
                         << "  Eccentricity: " << elem.eccentricity << "\n"
                         << "  Inclination: " << elem.inclination << " rad\n"
                         << "  Period: " << elem.orbital_period << " s\n";
                     return CommandResult{true, oss.str(), ""};
                   });

  register_command("stats", "Calculate statistics: stats <body_name>",
                   [this](const auto& args, auto&) {
                     if (args.size() < 2) {
                       return CommandResult{false, "", "Usage: stats <body_name>"};
                     }
                     auto data = impl_->data_processor.get_data(args[1]);
                     if (data.empty()) {
                       return CommandResult{false, "", "No data for " + args[1]};
                     }
                     std::vector<double> distances;
                     for (const auto& sv : data) {
                       distances.push_back(static_cast<double>(sv.position.magnitude()));
                     }
                     auto stats = impl_->statistical_analyzer.calculate_statistics(distances);
                     std::ostringstream oss;
                     oss << "Statistics for " << args[1] << " distance:\n"
                         << "  Mean: " << stats.mean << " km\n"
                         << "  Std Dev: " << stats.std_dev << " km\n"
                         << "  Min: " << stats.min << " km\n"
                         << "  Max: " << stats.max << " km\n";
                     return CommandResult{true, oss.str(), ""};
                   });

  register_command("export", "Export data: export <body_name> <filename>",
                   [this](const auto& args, auto&) {
                     if (args.size() < 3) {
                       return CommandResult{false, "", "Usage: export <body_name> <filename>"};
                     }
                     auto data = impl_->data_processor.get_data(args[1]);
                     if (data.empty()) {
                       return CommandResult{false, "", "No data for " + args[1]};
                     }
                     auto result = impl_->data_exporter.export_data(data, args[2]);
                     if (result.success) {
                       return CommandResult{true, "Exported to " + args[2], ""};
                     }
                     return CommandResult{false, "", result.message};
                   });

  register_command("set", "Set variable: set <name> <value>", [](const auto& args, auto& session) {
    if (args.size() < 3) {
      return CommandResult{false, "", "Usage: set <name> <value>"};
    }
    session.variables[args[1]] = args[2];
    return CommandResult{true, "Set " + args[1] + " = " + args[2], ""};
  });

  register_command("get", "Get variable: get <name>", [](const auto& args, auto& session) {
    if (args.size() < 2) {
      return CommandResult{false, "", "Usage: get <name>"};
    }
    auto it = session.variables.find(args[1]);
    if (it != session.variables.end()) {
      return CommandResult{true, args[1] + " = " + it->second, ""};
    }
    return CommandResult{false, "", "Variable not found: " + args[1]};
  });

  register_command("clear", "Clear session data", [this](const auto&, auto& session) {
    session.loaded_bodies.clear();
    session.variables.clear();
    impl_->data_processor.clear_cache();
    return CommandResult{true, "Session cleared", ""};
  });

  register_command("quit", "Exit the CLI",
                   [](const auto&, auto&) { return CommandResult{true, "exit", ""}; });
}

void AnalysisCLI::run_interactive() {
  std::cout << "Solar System Analysis CLI\n";
  std::cout << "Type 'help' for available commands, 'quit' to exit.\n\n";

  std::string line;
  while (true) {
    std::cout << "analysis> ";
    if (!std::getline(std::cin, line)) break;

    if (line.empty()) continue;

    auto result = execute_command(line);
    if (!result.output.empty()) {
      if (result.output == "exit") break;
      std::cout << result.output << "\n";
    }
    if (!result.error.empty()) {
      std::cerr << "Error: " << result.error << "\n";
    }
  }
}

CommandResult AnalysisCLI::execute_command(const std::string& command_line) {
  impl_->session.command_history.push_back(command_line);

  auto args = parse_command(command_line);
  if (args.empty()) {
    return {false, "", "Empty command"};
  }

  auto it = impl_->commands.find(args[0]);
  if (it == impl_->commands.end()) {
    return {false, "", "Unknown command: " + args[0]};
  }

  return it->second.second(args, impl_->session);
}

void AnalysisCLI::register_command(const std::string& name, const std::string& help,
                                   CommandHandler handler) {
  impl_->commands[name] = {help, std::move(handler)};
}

const SessionState& AnalysisCLI::session() const { return impl_->session; }

void AnalysisCLI::reset_session() {
  impl_->session = SessionState{};
  impl_->data_processor.clear_cache();
}

std::string AnalysisCLI::get_help() const {
  std::ostringstream oss;
  oss << "Available commands:\n";
  for (const auto& [name, info] : impl_->commands) {
    oss << "  " << name << " - " << info.first << "\n";
  }
  return oss.str();
}

std::string AnalysisCLI::get_command_help(const std::string& command) const {
  auto it = impl_->commands.find(command);
  if (it != impl_->commands.end()) {
    return command + ": " + it->second.first;
  }
  return "Unknown command: " + command;
}

const std::vector<std::string>& AnalysisCLI::history() const {
  return impl_->session.command_history;
}

void AnalysisCLI::clear_history() { impl_->session.command_history.clear(); }

std::vector<std::string> AnalysisCLI::parse_command(const std::string& line) const {
  std::vector<std::string> args;
  std::istringstream iss(line);
  std::string token;
  while (iss >> token) {
    args.push_back(token);
  }
  return args;
}

// ScriptEngine implementation

struct ScriptEngine::Impl {
  AnalysisCLI* cli = nullptr;
  std::map<std::string, std::string> variables;
  std::string last_error;
  size_t error_line = 0;
};

ScriptEngine::ScriptEngine() : impl_(std::make_unique<Impl>()) {}

ScriptEngine::ScriptEngine(AnalysisCLI& cli) : impl_(std::make_unique<Impl>()) {
  impl_->cli = &cli;
}

ScriptEngine::~ScriptEngine() = default;

bool ScriptEngine::execute_file(const std::filesystem::path& script_path) {
  std::ifstream file(script_path);
  if (!file) {
    impl_->last_error = "Cannot open file: " + script_path.string();
    return false;
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  return execute_string(oss.str());
}

bool ScriptEngine::execute_string(const std::string& script) {
  if (!impl_->cli) {
    impl_->last_error = "No CLI attached";
    return false;
  }

  std::istringstream iss(script);
  std::string line;
  size_t line_num = 0;

  while (std::getline(iss, line)) {
    ++line_num;

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') continue;

    // Variable substitution
    for (const auto& [name, value] : impl_->variables) {
      std::string var = "$" + name;
      size_t pos;
      while ((pos = line.find(var)) != std::string::npos) {
        line.replace(pos, var.length(), value);
      }
    }

    auto result = impl_->cli->execute_command(line);
    if (!result.success) {
      impl_->last_error = result.error;
      impl_->error_line = line_num;
      return false;
    }
  }

  return true;
}

void ScriptEngine::set_variable(const std::string& name, const std::string& value) {
  impl_->variables[name] = value;
}

std::optional<std::string> ScriptEngine::get_variable(const std::string& name) const {
  auto it = impl_->variables.find(name);
  return it != impl_->variables.end() ? std::optional{it->second} : std::nullopt;
}

void ScriptEngine::clear_variables() { impl_->variables.clear(); }

std::string ScriptEngine::last_error() const { return impl_->last_error; }

size_t ScriptEngine::error_line() const { return impl_->error_line; }

// TextVisualizer implementation

std::string TextVisualizer::plot_line(const std::vector<double>& data, int width, int height) {
  if (data.empty()) return "";

  double min_val = *std::min_element(data.begin(), data.end());
  double max_val = *std::max_element(data.begin(), data.end());
  double range = max_val - min_val;
  if (range < 1e-10) range = 1.0;

  std::vector<std::string> canvas(static_cast<size_t>(height),
                                  std::string(static_cast<size_t>(width), ' '));

  for (size_t i = 0; i < data.size() && i < static_cast<size_t>(width); ++i) {
    int y = static_cast<int>((data[i] - min_val) / range * (height - 1));
    y = std::clamp(y, 0, height - 1);
    canvas[static_cast<size_t>(height - 1 - y)][i] = '*';
  }

  std::ostringstream oss;
  for (const auto& row : canvas) {
    oss << "|" << row << "\n";
  }
  oss << "+" << std::string(static_cast<size_t>(width), '-') << "\n";
  return oss.str();
}

std::string TextVisualizer::plot_histogram(const std::vector<double>& data, int bins, int width) {
  if (data.empty()) return "";

  double min_val = *std::min_element(data.begin(), data.end());
  double max_val = *std::max_element(data.begin(), data.end());
  double bin_width = (max_val - min_val) / bins;
  if (bin_width < 1e-10) bin_width = 1.0;

  std::vector<int> counts(static_cast<size_t>(bins), 0);
  for (double v : data) {
    int bin = static_cast<int>((v - min_val) / bin_width);
    bin = std::clamp(bin, 0, bins - 1);
    counts[static_cast<size_t>(bin)]++;
  }

  int max_count = *std::max_element(counts.begin(), counts.end());
  if (max_count == 0) max_count = 1;

  std::ostringstream oss;
  for (int i = 0; i < bins; ++i) {
    int bar_len = counts[static_cast<size_t>(i)] * width / max_count;
    oss << std::string(static_cast<size_t>(bar_len), '#') << " " << counts[static_cast<size_t>(i)]
        << "\n";
  }
  return oss.str();
}

std::string TextVisualizer::plot_bars(const std::map<std::string, double>& data, int width) {
  if (data.empty()) return "";

  double max_val = 0;
  for (const auto& [_, v] : data) {
    max_val = std::max(max_val, v);
  }
  if (max_val < 1e-10) max_val = 1.0;

  std::ostringstream oss;
  for (const auto& [name, value] : data) {
    int bar_len = static_cast<int>(value / max_val * width);
    oss << name << ": " << std::string(static_cast<size_t>(bar_len), '#') << " " << value << "\n";
  }
  return oss.str();
}

std::string TextVisualizer::plot_orbit(const std::vector<StateVector>& trajectory, int size) {
  if (trajectory.empty()) return "";

  // Find bounds
  double max_r = 0;
  for (const auto& sv : trajectory) {
    max_r = std::max(max_r, static_cast<double>(sv.position.magnitude()));
  }
  if (max_r < 1e-10) max_r = 1.0;

  std::vector<std::string> canvas(static_cast<size_t>(size),
                                  std::string(static_cast<size_t>(size), ' '));
  int center = size / 2;

  // Draw Sun at center
  canvas[static_cast<size_t>(center)][static_cast<size_t>(center)] = 'O';

  // Draw orbit points
  for (const auto& sv : trajectory) {
    int x = center + static_cast<int>(sv.position.x() / max_r * (center - 1));
    int y = center - static_cast<int>(sv.position.y() / max_r * (center - 1));
    x = std::clamp(x, 0, size - 1);
    y = std::clamp(y, 0, size - 1);
    canvas[static_cast<size_t>(y)][static_cast<size_t>(x)] = '.';
  }

  std::ostringstream oss;
  for (const auto& row : canvas) {
    oss << row << "\n";
  }
  return oss.str();
}

}  // namespace SolarSystem::Analysis
