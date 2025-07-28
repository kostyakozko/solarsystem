#include <iostream>

#include "lib/solar_test/include/solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;

int main() {
  // Clear registry
  TestDiscovery::instance().clear_registry();

  // Check what tests are registered initially
  auto initial_tests = TestDiscovery::instance().get_available_test_names();
  std::cout << "Initial tests registered: " << initial_tests.size() << std::endl;
  for (const auto& name : initial_tests) {
    std::cout << "  - " << name << std::endl;
  }

  // Create test classes with unique names
  class UniqueTestBodyFactory : public TestCase {
   public:
    UniqueTestBodyFactory()
        : TestCase({"UniqueTestBodyFactory",
                    "Test body factory",
                    {"unit"},
                    std::chrono::seconds(10),
                    false}) {}
    void run() override {}
  };

  class UniqueTestSimulationEngine : public TestCase {
   public:
    UniqueTestSimulationEngine()
        : TestCase({"UniqueTestSimulationEngine",
                    "Test simulation engine",
                    {"unit"},
                    std::chrono::seconds(10),
                    false}) {}
    void run() override {}
  };

  // Register tests
  TestDiscovery::instance().register_test_factory(
      "UniqueTestBodyFactory", []() { return std::make_unique<UniqueTestBodyFactory>(); },
      {"unit"});

  TestDiscovery::instance().register_test_factory(
      "UniqueTestSimulationEngine", []() { return std::make_unique<UniqueTestSimulationEngine>(); },
      {"unit"});

  // Check what tests are registered after our registration
  auto after_tests = TestDiscovery::instance().get_available_test_names();
  std::cout << "Tests after registration: " << after_tests.size() << std::endl;
  for (const auto& name : after_tests) {
    std::cout << "  - " << name << std::endl;
  }

  // Test pattern matching
  auto test_pattern_tests = TestDiscovery::instance().discover_tests_by_pattern("UniqueTest.*");
  std::cout << "Tests matching 'UniqueTest.*': " << test_pattern_tests.size() << std::endl;
  for (const auto& test : test_pattern_tests) {
    std::cout << "  - " << test->info().name << std::endl;
  }

  return 0;
}
