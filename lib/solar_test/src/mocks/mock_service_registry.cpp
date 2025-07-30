/**
 * @file mock_service_registry.cpp
 * @brief Mock Service Registry Implementation
 */

#include "solar_test/mocks/mock_service_registry.hpp"

#include <algorithm>
#include <stdexcept>

namespace SolarSystem::Testing::Mocks {

// ServiceScope implementation

ServiceScope::ServiceScope() = default;

ServiceScope::~ServiceScope() { clear(); }

void ServiceScope::clear() {
  std::lock_guard<std::mutex> lock(scope_mutex_);
  scoped_instances_.clear();
}

// MockServiceRegistry implementation

MockServiceRegistry& MockServiceRegistry::instance() {
  static MockServiceRegistry instance;
  return instance;
}

void MockServiceRegistry::install_all_mocks() {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  for (const auto& [key, registration] : services_) {
    if (registration.is_mock) {
      install_mock_service(registration);
    }
  }
}

void MockServiceRegistry::remove_all_mocks() {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  for (const auto& [key, registration] : services_) {
    if (registration.is_mock) {
      remove_mock_service(registration);
    }
  }

  installed_mocks_.clear();
}

std::unique_ptr<ServiceScope> MockServiceRegistry::create_scope() {
  return std::make_unique<ServiceScope>();
}

void MockServiceRegistry::set_current_scope(ServiceScope* scope) { current_scope_ = scope; }

ServiceScope* MockServiceRegistry::get_current_scope() const { return current_scope_; }

void MockServiceRegistry::clear_all() {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  // Remove all installed mocks first
  for (const auto& [key, registration] : services_) {
    if (registration.is_mock) {
      remove_mock_service(registration);
    }
  }

  services_.clear();
  singleton_instances_.clear();
  installed_mocks_.clear();
  current_scope_ = nullptr;
}

void MockServiceRegistry::clear_mocks() {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  // Remove installed mocks
  for (auto it = services_.begin(); it != services_.end();) {
    if (it->second.is_mock) {
      remove_mock_service(it->second);
      singleton_instances_.erase(it->first);
      it = services_.erase(it);
    } else {
      ++it;
    }
  }

  installed_mocks_.clear();
}

std::vector<ServiceRegistration> MockServiceRegistry::get_all_registrations() const {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  std::vector<ServiceRegistration> registrations;
  registrations.reserve(services_.size());

  for (const auto& [key, registration] : services_) {
    registrations.push_back(registration);
  }

  return registrations;
}

std::vector<ServiceRegistration> MockServiceRegistry::get_mock_registrations() const {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  std::vector<ServiceRegistration> mock_registrations;

  for (const auto& [key, registration] : services_) {
    if (registration.is_mock) {
      mock_registrations.push_back(registration);
    }
  }

  return mock_registrations;
}

void MockServiceRegistry::register_common_mocks() {
  // Register JPL mock
  register_mock<JPLMock, JPLMock>(ServiceLifetime::Scoped, "jpl");

  // Register cache mock
  register_mock<CacheMock, CacheMock>(ServiceLifetime::Scoped, "cache");

  // Register time mock
  register_mock<TimeMock, TimeMock>(ServiceLifetime::Scoped, "time");

  // Register network mock
  register_mock<NetworkMock, NetworkMock>(ServiceLifetime::Scoped, "network");
}

std::unique_ptr<ServiceScope> MockServiceRegistry::create_test_environment() {
  auto scope = create_scope();
  set_current_scope(scope.get());

  // Register common mocks if not already registered
  if (services_.empty()) {
    register_common_mocks();
  }

  return scope;
}

bool MockServiceRegistry::validate_dependencies() const {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  for (const auto& [key, registration] : services_) {
    if (!resolve_dependencies(registration)) {
      return false;
    }
  }

  return true;
}

std::map<std::string, std::vector<std::string>> MockServiceRegistry::get_dependency_graph() const {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  std::map<std::string, std::vector<std::string>> graph;

  for (const auto& [key, registration] : services_) {
    std::vector<std::string> dependencies;

    for (const auto& dep_type : registration.dependencies) {
      dependencies.push_back(dep_type.name());
    }

    graph[key] = dependencies;
  }

  return graph;
}

void MockServiceRegistry::install_mock_service(const ServiceRegistration& registration) {
  // This is a simplified implementation. In a real implementation,
  // this would install the mock into the appropriate global location
  // based on the service type.

  std::string key = registration.service_type.name();
  if (!registration.service_name.empty()) {
    key += ":" + registration.service_name;
  }

  installed_mocks_[key] = true;

  // For specific mock types, install them globally
  if (registration.service_type == std::type_index(typeid(JPLMock))) {
    auto mock = resolve<JPLMock>(registration.service_name);
    if (mock) {
      mock->install_as_global_mock();
    }
  } else if (registration.service_type == std::type_index(typeid(CacheMock))) {
    auto mock = resolve<CacheMock>(registration.service_name);
    if (mock) {
      mock->install_as_global_mock();
    }
  } else if (registration.service_type == std::type_index(typeid(TimeMock))) {
    auto mock = resolve<TimeMock>(registration.service_name);
    if (mock) {
      mock->install_as_global_mock();
    }
  } else if (registration.service_type == std::type_index(typeid(NetworkMock))) {
    auto mock = resolve<NetworkMock>(registration.service_name);
    if (mock) {
      mock->install_as_global_mock();
    }
  }
}

void MockServiceRegistry::remove_mock_service(const ServiceRegistration& registration) {
  std::string key = registration.service_type.name();
  if (!registration.service_name.empty()) {
    key += ":" + registration.service_name;
  }

  installed_mocks_.erase(key);

  // For specific mock types, remove them globally
  if (registration.service_type == std::type_index(typeid(JPLMock))) {
    JPLMock::remove_global_mock();
  } else if (registration.service_type == std::type_index(typeid(CacheMock))) {
    CacheMock::remove_global_mock();
  } else if (registration.service_type == std::type_index(typeid(TimeMock))) {
    TimeMock::remove_global_mock();
  } else if (registration.service_type == std::type_index(typeid(NetworkMock))) {
    NetworkMock::remove_global_mock();
  }
}

bool MockServiceRegistry::resolve_dependencies(const ServiceRegistration& registration) const {
  // Simplified dependency resolution - just check if dependencies are registered
  for (const auto& dep_type : registration.dependencies) {
    bool found = false;

    for (const auto& [key, reg] : services_) {
      if (reg.service_type == dep_type) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

// ScopedServiceRegistry implementation

ScopedServiceRegistry::ScopedServiceRegistry(bool install_common_mocks)
    : scope_(MockServiceRegistry::instance().create_scope()),
      previous_scope_(MockServiceRegistry::instance().get_current_scope()) {
  MockServiceRegistry::instance().set_current_scope(scope_.get());

  if (install_common_mocks) {
    MockServiceRegistry::instance().register_common_mocks();
  }
}

ScopedServiceRegistry::~ScopedServiceRegistry() {
  MockServiceRegistry::instance().set_current_scope(previous_scope_);
}

// MockServiceFactory implementation

std::shared_ptr<JPLMock> MockServiceFactory::create_jpl_mock(JPLMockConfig config) {
  return std::make_shared<JPLMock>(std::move(config));
}

std::shared_ptr<CacheMock> MockServiceFactory::create_cache_mock(CacheMockConfig config) {
  return std::make_shared<CacheMock>(std::move(config));
}

std::shared_ptr<TimeMock> MockServiceFactory::create_time_mock(TimeMockConfig config) {
  return std::make_shared<TimeMock>(std::move(config));
}

std::shared_ptr<NetworkMock> MockServiceFactory::create_network_mock(NetworkMockConfig config) {
  return std::make_shared<NetworkMock>(std::move(config));
}

std::map<std::string, std::any> MockServiceFactory::create_all_common_mocks() {
  std::map<std::string, std::any> mocks;

  mocks["jpl"] = create_jpl_mock();
  mocks["cache"] = create_cache_mock();
  mocks["time"] = create_time_mock();
  mocks["network"] = create_network_mock();

  return mocks;
}

}  // namespace SolarSystem::Testing::Mocks
