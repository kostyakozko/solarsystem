#pragma once

/**
 * @file analysis_cli.hpp
 * @brief Interactive command-line interface for data analysis
 */

#include <functional>
#include <map>
#include <memory>
#include <solar_analysis/batch_processor.hpp>
#include <solar_analysis/data_exporter.hpp>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>
#include <solar_analysis/orbital_calculator.hpp>
#include <solar_analysis/statistical_analyzer.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Command result
 */
struct SOLAR_ANALYSIS_API CommandResult {
  bool success = false;
  std::string output;
  std::string error;
};

/**
 * @brief Session state for stateful analysis
 */
struct SOLAR_ANALYSIS_API SessionState {
  std::vector<std::string> loaded_bodies;
  std::map<std::string, std::string> variables;
  std::vector<std::string> command_history;
  std::filesystem::path working_directory = ".";
  bool verbose = false;
};

/**
 * @brief Command handler function type
 */
using CommandHandler = std::function<CommandResult(const std::vector<std::string>&, SessionState&)>;

/**
 * @brief Interactive analysis CLI
 */
class SOLAR_ANALYSIS_API AnalysisCLI {
 public:
  AnalysisCLI();
  ~AnalysisCLI();

  AnalysisCLI(const AnalysisCLI&) = delete;
  AnalysisCLI& operator=(const AnalysisCLI&) = delete;

  // Interactive mode
  void run_interactive();
  [[nodiscard]] CommandResult execute_command(const std::string& command_line);

  // Command registration
  void register_command(const std::string& name, const std::string& help, CommandHandler handler);

  // Session management
  [[nodiscard]] const SessionState& session() const;
  void reset_session();

  // Help
  [[nodiscard]] std::string get_help() const;
  [[nodiscard]] std::string get_command_help(const std::string& command) const;

  // History
  [[nodiscard]] const std::vector<std::string>& history() const;
  void clear_history();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  void register_builtin_commands();
  [[nodiscard]] std::vector<std::string> parse_command(const std::string& line) const;
};

/**
 * @brief Script engine for automated analysis
 */
class SOLAR_ANALYSIS_API ScriptEngine {
 public:
  ScriptEngine();
  explicit ScriptEngine(AnalysisCLI& cli);
  ~ScriptEngine();

  // Script execution
  [[nodiscard]] bool execute_file(const std::filesystem::path& script_path);
  [[nodiscard]] bool execute_string(const std::string& script);

  // Variable management
  void set_variable(const std::string& name, const std::string& value);
  [[nodiscard]] std::optional<std::string> get_variable(const std::string& name) const;
  void clear_variables();

  // Error handling
  [[nodiscard]] std::string last_error() const;
  [[nodiscard]] size_t error_line() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief Simple text-based visualization for CLI
 */
class SOLAR_ANALYSIS_API TextVisualizer {
 public:
  // ASCII chart for time series
  [[nodiscard]] static std::string plot_line(const std::vector<double>& data, int width = 60,
                                             int height = 10);

  // ASCII histogram
  [[nodiscard]] static std::string plot_histogram(const std::vector<double>& data, int bins = 10,
                                                  int width = 40);

  // ASCII bar chart
  [[nodiscard]] static std::string plot_bars(const std::map<std::string, double>& data,
                                             int width = 40);

  // Orbital diagram (top-down view)
  [[nodiscard]] static std::string plot_orbit(const std::vector<StateVector>& trajectory,
                                              int size = 40);
};

}  // namespace SolarSystem::Analysis
