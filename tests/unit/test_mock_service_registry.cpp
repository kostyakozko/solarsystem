/**
 * @file test_mock_service_registry.cpp
 * @brief Unit tests for MockServiceRegistry functionality
 */

#include <memory>
#include <string>

#include "solar_test/solar_test.hpp"

using namespace SolarSystem::Testing;
using namespace SolarSystem::Testing::Mocks;

// Test interface and implementation for testing
class ITestService {
 public:
  virtual ~ITestService() = default;
  virtual std::string get_name() const = 0;
  virtual int get_value() const = 0;
};

class TestServiceImpl : public ITestService {
 public:
  explicit TestServiceImpl(const std::string& name = "TestService", int value = 42)
      : name_(name), value_(value) {}

  std::string get_name() const override { return name_; }
  int get_value() const override { return value_; }

 private:
  std::string name_;
  int value_;
};

class TestServiceMock : public ITestService {
 public:
  explicit TestServiceMock(const std::string& name = "MockService", int value = 99)
      : name_(name), value_(value) {}

  std::string get_name() const override { return name_; }
  int get_value() const override { return value_; }

 private:
  std::string name_;
  int value_;
};

/**
 * @brief Test basic service registration and resolution
 */
class ServiceRegistryBasicTest : public TestCase {
 public:
  ServiceRegistryBasicTest()
      : TestCase({"ServiceRegistryBasicTest",
                  "Test basic service registry operations",
                  {"unit", "mock_registry"}}) {}

  void run() override {
    auto& registry = MockServiceRegistry::instance();
    registry.clear_all();  // Start clean

    // Test service registration
    registry.register_service<ITestService, TestServiceImpl>();
    assert_true(registry.is_registered<ITestService>(), "Service should be registered");

    // Test service resolution
    auto service = registry.resolve<ITestService>();
    assert_true(service.get() != nullptr, "Service should be resolved");
    assert_equals(std::string("TestService"), service->get_name(),
                  "Service should have correct name");
    assert_equals(42, service->get_value(), "Service should have correct value");

    // Test singleton behavior
    auto service2 = registry.resolve<ITestService>();
    assert_true(service.get() == service2.get(), "Singleton services should return same instance");
  }
};

/**
 * @brief Test mock service registration and resolution
 */
class MockServiceRegistryTest : public TestCase {
 public:
  MockServiceRegistryTest()
      : TestCase({"MockServiceRegistryTest",
                  "Test mock service registration",
                  {"unit", "mock_registry"}}) {}

  void run() override {
    auto& registry = MockServiceRegistry::instance();
    registry.clear_all();  // Start clean

    // Test mock registration
    registry.register_mock<ITestService, TestServiceMock>(ServiceLifetime::Scoped);
    assert_true(registry.is_registered<ITestService>(), "Mock service should be registered");

    // Test mock resolution with scope
    auto scope = registry.create_scope();
    registry.set_current_scope(scope.get());

    auto mock_service = registry.resolve<ITestService>();
    assert_true(mock_service.get() != nullptr, "Mock service should be resolved");
    assert_equals(std::string("MockService"), mock_service->get_name(),
                  "Mock should have correct name");
    assert_equals(99, mock_service->get_value(), "Mock should have correct value");

    registry.set_current_scope(nullptr);
  }
};

/**
 * @brief Test service instance registration
 */
class ServiceInstanceTest : public TestCase {
 public:
  ServiceInstanceTest()
      : TestCase({"ServiceInstanceTest",
                  "Test service instance registration",
                  {"unit", "mock_registry"}}) {}

  void run() override {
    auto& registry = MockServiceRegistry::instance();
    registry.clear_all();  // Start clean

    // Create and register instance
    auto instance = std::make_shared<TestServiceImpl>("CustomService", 123);
    registry.register_instance<ITestService>(instance);

    // Test resolution
    auto resolved = registry.resolve<ITestService>();
    assert_true(resolved.get() != nullptr, "Instance should be resolved");
    assert_true(instance.get() == resolved.get(), "Should return exact same instance");
    assert_equals(std::string("CustomService"), resolved->get_name(),
                  "Instance should have correct name");
    assert_equals(123, resolved->get_value(), "Instance should have correct value");
  }
};

/**
 * @brief Test service lifetime management
 */
class ServiceLifetimeTest : public TestCase {
 public:
  ServiceLifetimeTest()
      : TestCase({"ServiceLifetimeTest",
                  "Test service lifetime management",
                  {"unit", "mock_registry"}}) {}

  void run() override {
    auto& registry = MockServiceRegistry::instance();
    registry.clear_all();  // Start clean

    // Test transient lifetime
    registry.register_service<ITestService, TestServiceImpl>(ServiceLifetime::Transient);

    auto service1 = registry.resolve<ITestService>();
    auto service2 = registry.resolve<ITestService>();
    assert_true(service1.get() != nullptr, "First transient service should be resolved");
    assert_true(service2.get() != nullptr, "Second transient service should be resolved");
    assert_true(service1.get() != service2.get(),
                "Transient services should be different instances");

    registry.clear_all();

    // Test singleton lifetime (default)
    registry.register_service<ITestService, TestServiceImpl>(ServiceLifetime::Singleton);

    auto singleton1 = registry.resolve<ITestService>();
    auto singleton2 = registry.resolve<ITestService>();
    assert_true(singleton1.get() != nullptr, "First singleton service should be resolved");
    assert_true(singleton2.get() != nullptr, "Second singleton service should be resolved");
    assert_true(singleton1.get() == singleton2.get(), "Singleton services should be same instance");
  }
};

/**
 * @brief Test service scope functionality
 */
class ServiceScopeTest : public TestCase {
 public:
  ServiceScopeTest()
      : TestCase(
            {"ServiceScopeTest", "Test service scope functionality", {"unit", "mock_registry"}}) {}

  void run() override {
    auto& registry = MockServiceRegistry::instance();
    registry.clear_all();  // Start clean

    // Register scoped service
    registry.register_service<ITestService, TestServiceImpl>(ServiceLifetime::Scoped);

    // Test with first scope
    auto scope1 = registry.create_scope();
    registry.set_current_scope(scope1.get());

    auto service1a = registry.resolve<ITestService>();
    auto service1b = registry.resolve<ITestService>();
    assert_true(service1a.get() != nullptr, "Service should be resolved in scope1");
    assert_true(service1a.get() == service1b.get(),
                "Services in same scope should be same instance");

    // Test with second scope
    auto scope2 = registry.create_scope();
    registry.set_current_scope(scope2.get());

    auto service2 = registry.resolve<ITestService>();
    assert_true(service2.get() != nullptr, "Service should be resolved in scope2");
    assert_true(service1a.get() != service2.get(),
                "Services in different scopes should be different instances");

    registry.set_current_scope(nullptr);
  }
};

/**
 * @brief Test mock service factory
 */
class MockServiceFactoryTest : public TestCase {
 public:
  MockServiceFactoryTest()
      : TestCase(
            {"MockServiceFactoryTest", "Test mock service factory", {"unit", "mock_registry"}}) {}

  void run() override {
    // Test JPL mock creation
    auto jpl_mock = MockServiceFactory::create_jpl_mock();
    assert_true(jpl_mock.get() != nullptr, "JPL mock should be created");

    // Test cache mock creation
    auto cache_mock = MockServiceFactory::create_cache_mock();
    assert_true(cache_mock.get() != nullptr, "Cache mock should be created");

    // Test time mock creation
    auto time_mock = MockServiceFactory::create_time_mock();
    assert_true(time_mock.get() != nullptr, "Time mock should be created");

    // Test network mock creation
    auto network_mock = MockServiceFactory::create_network_mock();
    assert_true(network_mock.get() != nullptr, "Network mock should be created");

    // Test all common mocks creation
    auto all_mocks = MockServiceFactory::create_all_common_mocks();
    assert_false(all_mocks.empty(), "Should create multiple common mocks");
  }
};

// Register all tests
SOLAR_REGISTER_TEST_WITH_TAGS(ServiceRegistryBasicTest, "unit", "mock_registry");
SOLAR_REGISTER_TEST_WITH_TAGS(MockServiceRegistryTest, "unit", "mock_registry");
SOLAR_REGISTER_TEST_WITH_TAGS(ServiceInstanceTest, "unit", "mock_registry");
SOLAR_REGISTER_TEST_WITH_TAGS(ServiceLifetimeTest, "unit", "mock_registry");
SOLAR_REGISTER_TEST_WITH_TAGS(ServiceScopeTest, "unit", "mock_registry");
SOLAR_REGISTER_TEST_WITH_TAGS(MockServiceFactoryTest, "unit", "mock_registry");

SOLAR_TEST_MAIN();
