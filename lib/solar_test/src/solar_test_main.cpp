#include <iomanip>
#include <iostream>
#include <sstream>

#include "solar_test/framework/assertions.hpp"
#include "solar_test/solar_test.hpp"

namespace SolarSystem::Testing {

TestRunner::Configuration parse_test_arguments(int argc, char* argv[]) {
  TestRunner::Configuration config;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help" || arg == "-h") {
      std::cout << "Solar System Testing Framework\n\n";
      std::cout << "Usage: " << argv[0] << " [OPTIONS]\n\n";
      std::cout << "Options:\n";
      std::cout << "  -h, --help              Show this help message\n";
      std::cout << "  --parallel              Enable parallel test execution (default)\n";
      std::cout << "  --sequential            Disable parallel test execution\n";
      std::cout << "  --threads N             Set maximum number of threads (default: hardware "
                   "concurrency)\n";
      std::cout << "  --timeout N             Set test timeout in seconds (default: 300)\n";
      std::cout << "  --tag TAG               Run only tests with specified tag\n";
      std::cout << "  --pattern PATTERN       Run tests matching pattern (regex)\n";
      std::cout << "  --verbose               Enable verbose output\n";
      std::cout << "  --quiet                 Enable quiet output\n";
      std::cout
          << "  --output FORMAT         Output format: console, xml, json (default: console)\n";
      std::cout << "  --output-file FILE      Write output to file\n";
      std::cout << "\nExamples:\n";
      std::cout << "  " << argv[0] << " --tag unit\n";
      std::cout << "  " << argv[0] << " --pattern \".*BodyFactory.*\"\n";
      std::cout << "  " << argv[0] << " --sequential --verbose\n";
      exit(0);
    } else if (arg == "--parallel") {
      config.parallel_execution = true;
    } else if (arg == "--sequential") {
      config.parallel_execution = false;
    } else if (arg == "--threads" && i + 1 < argc) {
      config.max_threads = std::stoul(argv[++i]);
    } else if (arg == "--timeout" && i + 1 < argc) {
      config.timeout = std::chrono::seconds(std::stoul(argv[++i]));
    } else if (arg == "--tag" && i + 1 < argc) {
      config.tags.push_back(argv[++i]);
    } else if (arg == "--pattern" && i + 1 < argc) {
      config.test_patterns.push_back(argv[++i]);
    } else if (arg == "--verbose") {
      config.verbose = true;
    } else if (arg == "--quiet") {
      config.quiet = true;
    } else if (arg == "--output" && i + 1 < argc) {
      config.output_format = argv[++i];
    } else if (arg == "--output-file" && i + 1 < argc) {
      config.output_file = argv[++i];
    }
  }

  return config;
}

std::unique_ptr<TestRunner> create_default_test_runner() {
  TestRunner::Configuration config;
  config.parallel_execution = true;
  config.max_threads = std::thread::hardware_concurrency();
  config.timeout = std::chrono::minutes(5);
  config.generate_coverage = false;
  config.verbose = false;
  config.quiet = false;
  config.output_format = "console";

  return std::make_unique<TestRunner>(config);
}

int run_tests(int argc, char* argv[]) {
  try {
    auto config = parse_test_arguments(argc, argv);
    auto runner = std::make_unique<TestRunner>(config);

    // Register all tests from the registry
    auto tests = TestRegistry::instance().create_all_tests();
    for (auto& test : tests) {
      runner->register_test(std::move(test));
    }

    // Set up progress callbacks if verbose
    if (config.verbose) {
      runner->set_progress_callback([](const std::string& message, double percentage) {
        std::cout << "[" << std::fixed << std::setprecision(1) << percentage << "%] " << message
                  << std::endl;
      });

      runner->set_test_started_callback([](const std::string& test_name) {
        std::cout << "Starting: " << test_name << std::endl;
      });

      runner->set_test_completed_callback([](const TestResult& result) {
        if (!result.passed()) {
          std::cout << "  " << result.to_string() << std::endl;
        }
      });
    }

    // Run tests based on configuration
    TestSuiteResult result;
    if (!config.tags.empty()) {
      // Run tests with specific tags
      for (const auto& tag : config.tags) {
        auto tag_result = runner->run_tests_with_tag(tag);
        // Merge results (simplified)
        for (const auto& test_result : tag_result.test_results) {
          result.add_result(test_result);
        }
      }
    } else if (!config.test_patterns.empty()) {
      // Run tests matching patterns
      for (const auto& pattern : config.test_patterns) {
        auto pattern_result = runner->run_tests_matching_pattern(pattern);
        // Merge results (simplified)
        for (const auto& test_result : pattern_result.test_results) {
          result.add_result(test_result);
        }
      }
    } else {
      // Run all tests
      result = runner->run_all_tests();
    }

    // Output results
    if (!config.quiet) {
      std::cout << "\n" << result.summary() << std::endl;

      if (result.failed_count > 0) {
        std::cout << "\nFailed Tests:" << std::endl;
        for (const auto& test_result : result.test_results) {
          if (!test_result.passed()) {
            std::cout << "  " << test_result.to_string() << std::endl;
          }
        }
      }
    }

    // Return appropriate exit code
    return result.all_passed() ? 0 : 1;

  } catch (const std::exception& e) {
    std::cerr << "Error running tests: " << e.what() << std::endl;
    return 2;
  } catch (...) {
    std::cerr << "Unknown error running tests" << std::endl;
    return 2;
  }
}

}  // namespace SolarSystem::Testing
