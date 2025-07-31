#include "solar_test/reporters/test_reporter.hpp"

#include "solar_test/reporters/console_reporter.hpp"
#include "solar_test/reporters/coverage_reporter.hpp"
#include "solar_test/reporters/json_reporter.hpp"
#include "solar_test/reporters/tap_reporter.hpp"
#include "solar_test/reporters/xml_reporter.hpp"

namespace SolarSystem::Testing {

std::unique_ptr<TestReporter> TestReporterFactory::create_console_reporter(bool colorized) {
  ConsoleReporter::Configuration config;
  config.colorized = colorized;
  return std::make_unique<ConsoleReporter>(config);
}

std::unique_ptr<TestReporter> TestReporterFactory::create_xml_reporter(
    const std::string& output_file) {
  return std::make_unique<XmlReporter>(output_file);
}

std::unique_ptr<TestReporter> TestReporterFactory::create_json_reporter(
    const std::string& output_file) {
  return std::make_unique<JsonReporter>(output_file);
}

std::unique_ptr<TestReporter> TestReporterFactory::create_tap_reporter(
    const std::string& output_file) {
  return std::make_unique<TapReporter>(output_file);
}

std::unique_ptr<TestReporter> TestReporterFactory::create_coverage_reporter(
    const std::string& output_file) {
  return std::make_unique<CoverageReporter>(output_file);
}

}  // namespace SolarSystem::Testing
