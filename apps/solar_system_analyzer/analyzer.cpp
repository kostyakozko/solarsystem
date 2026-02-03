/**
 * @file analyzer.cpp
 * @brief Solar System Analyzer - Data analysis application
 *
 * Provides comprehensive data analysis capabilities for celestial body data:
 * - Orbital parameter analysis
 * - Statistical analysis
 * - Data export
 * - Batch processing
 */

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "solar_analysis/analysis_engine.hpp"
#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem::Analysis;
using namespace SolarSystem::Bodies;

namespace {

struct AnalyzerOptions {
  bool show_help = false;
  bool show_version = false;
  bool verbose = false;
  std::vector<std::string> bodies;
  std::string output_format = "text";
  std::string output_file;
};

class ArgumentParser {
 public:
  [[nodiscard]] static std::optional<AnalyzerOptions> parse(int argc, char* argv[]) {
    AnalyzerOptions options;

    for (int i = 1; i < argc; ++i) {
      std::string_view arg = argv[i];

      if (arg == "-h" || arg == "--help") {
        options.show_help = true;
      } else if (arg == "-v" || arg == "--version") {
        options.show_version = true;
      } else if (arg == "--verbose") {
        options.verbose = true;
      } else if (arg == "-b" || arg == "--body") {
        if (i + 1 < argc) {
          options.bodies.push_back(argv[++i]);
        }
      } else if (arg == "-f" || arg == "--format") {
        if (i + 1 < argc) {
          options.output_format = argv[++i];
        }
      } else if (arg == "-o" || arg == "--output") {
        if (i + 1 < argc) {
          options.output_file = argv[++i];
        }
      }
    }

    return options;
  }

  static void print_help() {
    std::cout << "Solar System Analyzer - Data Analysis Tool\n\n"
              << "Usage: solar_system_analyzer [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h, --help           Show this help message\n"
              << "  -v, --version        Show version information\n"
              << "  --verbose            Enable verbose output\n"
              << "  -b, --body NAME      Add celestial body to analyze (can be repeated)\n"
              << "  -f, --format FORMAT  Output format: text, csv, json (default: text)\n"
              << "  -o, --output FILE    Output file (default: stdout)\n\n"
              << "Examples:\n"
              << "  solar_system_analyzer -b Earth -b Mars\n"
              << "  solar_system_analyzer -b Jupiter --format csv -o results.csv\n";
  }

  static void print_version() {
    std::cout << "Solar System Analyzer v4.0.0\n"
              << "Part of Solar System Suite\n";
  }
};

}  // namespace

int main(int argc, char* argv[]) {
  auto options = ArgumentParser::parse(argc, argv);
  if (!options) {
    std::cerr << "Error: Failed to parse arguments\n";
    return 1;
  }

  if (options->show_help) {
    ArgumentParser::print_help();
    return 0;
  }

  if (options->show_version) {
    ArgumentParser::print_version();
    return 0;
  }

  // Default to analyzing all planets if no bodies specified
  if (options->bodies.empty()) {
    options->bodies = {"Sun",     "Mercury", "Venus",  "Earth",  "Mars",
                       "Jupiter", "Saturn",  "Uranus", "Neptune"};
  }

  std::cout << "Solar System Analyzer\n";
  std::cout << "=====================\n\n";

  // Create analysis engine
  AnalysisConfig config;
  config.verbose = options->verbose;

  AnalysisEngine engine(config);

  // Load data for specified bodies
  std::cout << "Loading data for " << options->bodies.size() << " bodies...\n";
  if (!engine.load_data(options->bodies)) {
    std::cerr << "Error: Failed to load data\n";
    return 1;
  }

  std::cout << "Loaded " << engine.data_point_count() << " data points\n\n";

  // Run analysis
  std::cout << "Running analysis...\n";
  auto result = engine.analyze();

  if (result.success) {
    std::cout << "✓ " << result.message << "\n";
  } else {
    std::cerr << "✗ Analysis failed: " << result.message << "\n";
    return 1;
  }

  return 0;
}
