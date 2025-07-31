/**
 * @file mock_service_registry.hpp
 * @brief Mock Service Registry for Dependency Injection
 *
 * Provides a centralized registry for managing mock services and dependency injection:
 * - Service registration and lookup
 * - Automatic mock installation and cleanup
 * - Type-safe service resolution
 * - Scoped service management
 * - Integration with existing mock frameworks
 */

#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "solar_test/mocks/cache_mock.hpp"
#include "solar_test/mocks/jpl_mock.hpp"
#include "solar_test/mocks/network_mock.hpp"
#include "solar_test/mocks/time_mock.hpp"

namespace SolarSystem::Testing::Mocks {

/**
 * @brief Service lifecycle management
 */
enum class ServiceLifetime {
  Singleton,  // Single instance for entire application
  Scoped,     // Single instance per scope/test
  Transient   // New instance every time
};

/**
 * @brief Service registration information
 */
struct ServiceRegistration {
  std::type_index service_type;
  std::string service_name;
  ServiceLifetime lifetime;
  std::function<std::any()> factory;
  std::any singleton_instance;
  bool is_mock = false;
  std::string description;
  std::vector<std::type_index> dependencies;

  // Constructor to initialize service_type
  explicit ServiceRegistration(std::type_index type) : service_type(type) {}

  // Default constructor for map operations
  ServiceRegistration() : service_type(typeid(void)) {}
};

/**
 * @brief Service scope for managing scoped services
 */
class ServiceScope {
 public:
  /**
   * @brief Create new service scope
   */
  ServiceScope();

  /**
   * @brief Destructor - cleans up scoped services
   */
  ~ServiceScope();

  // Non-copyable, non-movable
  ServiceScope(const ServiceScope&) = delete;
  ServiceScope& operator=(const ServiceScope&) = delete;
  ServiceScope(ServiceScope&&) = delete;
  ServiceScope& operator=(ServiceScope&&) = delete;

  /**
   * @brief Get service instance in this scope
   */
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> get_service();

  /**
   * @brief Check if service is registered in this scope
   */
  template <typename T>
  [[nodiscard]] bool has_service() const;

  /**
   * @brief Clear all scoped services
   */
  void clear();

 private:
  std::unordered_map<std::type_index, std::any> scoped_instances_;
  mutable std::mutex scope_mutex_;
  friend class MockServiceRegistry;
};

/**
 * @brief Mock Service Registry Implementation
 *
 * Provides centralized management of mock services for dependency injection.
 * Supports automatic service resolution, lifecycle management, and cleanup.
 */
class MockServiceRegistry {
 public:
  /**
   * @brief Get global registry instance
   */
  static MockServiceRegistry& instance();

  /**
   * @brief Destructor
   */
  ~MockServiceRegistry() = default;

  // Non-copyable, non-movable (singleton)
  MockServiceRegistry(const MockServiceRegistry&) = delete;
  MockServiceRegistry& operator=(const MockServiceRegistry&) = delete;
  MockServiceRegistry(MockServiceRegistry&&) = delete;
  MockServiceRegistry& operator=(MockServiceRegistry&&) = delete;

  // === Service Registration ===

  /**
   * @brief Register service with factory function
   */
  template <typename TInterface, typename TImplementation>
  void register_service(ServiceLifetime lifetime = ServiceLifetime::Singleton,
                        const std::string& name = "");

  /**
   * @brief Register service with custom factory
   */
  template <typename TInterface>
  void register_service(std::function<std::shared_ptr<TInterface>()> factory,
                        ServiceLifetime lifetime = ServiceLifetime::Singleton,
                        const std::string& name = "");

  /**
   * @brief Register singleton instance
   */
  template <typename TInterface>
  void register_instance(std::shared_ptr<TInterface> instance, const std::string& name = "");

  /**
   * @brief Register mock service
   */
  template <typename TInterface, typename TMock>
  void register_mock(ServiceLifetime lifetime = ServiceLifetime::Scoped,
                     const std::string& name = "");

  /**
   * @brief Register mock instance
   */
  template <typename TInterface>
  void register_mock_instance(std::shared_ptr<TInterface> mock_instance,
                              const std::string& name = "");

  // === Service Resolution ===

  /**
   * @brief Resolve service by type
   */
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> resolve(const std::string& name = "");

  /**
   * @brief Try to resolve service (returns nullptr if not found)
   */
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> try_resolve(const std::string& name = "");

  /**
   * @brief Check if service is registered
   */
  template <typename T>
  [[nodiscard]] bool is_registered(const std::string& name = "") const;

  /**
   * @brief Get all registered services of type
   */
  template <typename T>
  [[nodiscard]] std::vector<std::shared_ptr<T>> resolve_all();

  // === Mock Management ===

  /**
   * @brief Install all registered mocks
   */
  void install_all_mocks();

  /**
   * @brief Remove all installed mocks
   */
  void remove_all_mocks();

  /**
   * @brief Install specific mock type
   */
  template <typename T>
  void install_mock(const std::string& name = "");

  /**
   * @brief Remove specific mock type
   */
  template <typename T>
  void remove_mock(const std::string& name = "");

  /**
   * @brief Check if mock is installed
   */
  template <typename T>
  [[nodiscard]] bool is_mock_installed(const std::string& name = "") const;

  // === Scope Management ===

  /**
   * @brief Create new service scope
   */
  [[nodiscard]] std::unique_ptr<ServiceScope> create_scope();

  /**
   * @brief Set current scope (for scoped services)
   */
  void set_current_scope(ServiceScope* scope);

  /**
   * @brief Get current scope
   */
  [[nodiscard]] ServiceScope* get_current_scope() const;

  // === Registry Management ===

  /**
   * @brief Unregister service
   */
  template <typename T>
  void unregister(const std::string& name = "");

  /**
   * @brief Clear all registrations
   */
  void clear_all();

  /**
   * @brief Clear all mock registrations
   */
  void clear_mocks();

  /**
   * @brief Get registration information
   */
  template <typename T>
  [[nodiscard]] std::optional<ServiceRegistration> get_registration_info(
      const std::string& name = "") const;

  /**
   * @brief Get all registrations
   */
  [[nodiscard]] std::vector<ServiceRegistration> get_all_registrations() const;

  /**
   * @brief Get mock registrations only
   */
  [[nodiscard]] std::vector<ServiceRegistration> get_mock_registrations() const;

  // === Utility Functions ===

  /**
   * @brief Register common mock services
   */
  void register_common_mocks();

  /**
   * @brief Create test environment with all mocks
   */
  [[nodiscard]] std::unique_ptr<ServiceScope> create_test_environment();

  /**
   * @brief Validate service dependencies
   */
  [[nodiscard]] bool validate_dependencies() const;

  /**
   * @brief Get dependency graph
   */
  [[nodiscard]] std::map<std::string, std::vector<std::string>> get_dependency_graph() const;

 private:
  /**
   * @brief Private constructor (singleton)
   */
  MockServiceRegistry() = default;

  // Service storage
  mutable std::mutex registry_mutex_;
  std::unordered_map<std::string, ServiceRegistration> services_;
  std::unordered_map<std::string, std::any> singleton_instances_;
  std::unordered_map<std::string, bool> installed_mocks_;

  // Scope management
  ServiceScope* current_scope_ = nullptr;

  // === Internal Helper Methods ===

  /**
   * @brief Generate service key
   */
  template <typename T>
  [[nodiscard]] std::string make_service_key(const std::string& name = "") const;

  /**
   * @brief Create service instance
   */
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> create_service_instance(const ServiceRegistration& registration);

  /**
   * @brief Get or create singleton
   */
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> get_or_create_singleton(const ServiceRegistration& registration);

  /**
   * @brief Install mock service
   */
  void install_mock_service(const ServiceRegistration& registration);

  /**
   * @brief Remove mock service
   */
  void remove_mock_service(const ServiceRegistration& registration);

  /**
   * @brief Resolve dependencies
   */
  [[nodiscard]] bool resolve_dependencies(const ServiceRegistration& registration) const;
};

/**
 * @brief RAII helper for service scope management
 */
class ScopedServiceRegistry {
 public:
  /**
   * @brief Create scoped registry with automatic cleanup
   */
  explicit ScopedServiceRegistry(bool install_common_mocks = true);

  /**
   * @brief Destructor with automatic cleanup
   */
  ~ScopedServiceRegistry();

  // Non-copyable, non-movable
  ScopedServiceRegistry(const ScopedServiceRegistry&) = delete;
  ScopedServiceRegistry& operator=(const ScopedServiceRegistry&) = delete;
  ScopedServiceRegistry(ScopedServiceRegistry&&) = delete;
  ScopedServiceRegistry& operator=(ScopedServiceRegistry&&) = delete;

  /**
   * @brief Get the service scope
   */
  [[nodiscard]] ServiceScope& scope() { return *scope_; }

  /**
   * @brief Get the registry
   */
  [[nodiscard]] MockServiceRegistry& registry() { return MockServiceRegistry::instance(); }

  /**
   * @brief Register mock for this scope
   */
  template <typename TInterface, typename TMock>
  void register_mock(const std::string& name = "");

  /**
   * @brief Resolve service in this scope
   */
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> resolve(const std::string& name = "");

 private:
  std::unique_ptr<ServiceScope> scope_;
  ServiceScope* previous_scope_;
};

/**
 * @brief Mock service factory helpers
 */
class MockServiceFactory {
 public:
  /**
   * @brief Create JPL mock service
   */
  [[nodiscard]] static std::shared_ptr<JPLMock> create_jpl_mock(JPLMockConfig config = {});

  /**
   * @brief Create cache mock service
   */
  [[nodiscard]] static std::shared_ptr<CacheMock> create_cache_mock(CacheMockConfig config = {});

  /**
   * @brief Create time mock service
   */
  [[nodiscard]] static std::shared_ptr<TimeMock> create_time_mock(TimeMockConfig config = {});

  /**
   * @brief Create network mock service
   */
  [[nodiscard]] static std::shared_ptr<NetworkMock> create_network_mock(
      NetworkMockConfig config = {});

  /**
   * @brief Create all common mocks
   */
  [[nodiscard]] static std::map<std::string, std::any> create_all_common_mocks();
};

/**
 * @brief Service injection helpers
 */
template <typename T>
class ServiceInjector {
 public:
  /**
   * @brief Inject service dependency
   */
  explicit ServiceInjector(const std::string& name = "");

  /**
   * @brief Get injected service
   */
  [[nodiscard]] std::shared_ptr<T> get() const;

  /**
   * @brief Operator-> for direct access
   */
  [[nodiscard]] T* operator->() const;

  /**
   * @brief Operator* for dereferencing
   */
  [[nodiscard]] T& operator*() const;

  /**
   * @brief Check if service is available
   */
  [[nodiscard]] bool is_available() const;

 private:
  std::string service_name_;
  mutable std::shared_ptr<T> cached_service_;
};

// === Template Implementation ===

template <typename T>
std::shared_ptr<T> ServiceScope::get_service() {
  std::lock_guard<std::mutex> lock(scope_mutex_);

  auto type_key = std::type_index(typeid(T));
  auto it = scoped_instances_.find(type_key);

  if (it != scoped_instances_.end()) {
    return std::any_cast<std::shared_ptr<T>>(it->second);
  }

  // Get from registry
  auto service = MockServiceRegistry::instance().resolve<T>();
  if (service) {
    scoped_instances_[type_key] = service;
  }

  return service;
}

template <typename T>
bool ServiceScope::has_service() const {
  std::lock_guard<std::mutex> lock(scope_mutex_);
  auto type_key = std::type_index(typeid(T));
  return scoped_instances_.find(type_key) != scoped_instances_.end();
}

template <typename TInterface, typename TImplementation>
void MockServiceRegistry::register_service(ServiceLifetime lifetime, const std::string& name) {
  auto factory = []() -> std::any {
    return std::static_pointer_cast<TInterface>(std::make_shared<TImplementation>());
  };

  ServiceRegistration registration(std::type_index(typeid(TInterface)));
  registration.service_name = name;
  registration.lifetime = lifetime;
  registration.factory = factory;
  registration.is_mock = false;
  registration.description = "Service: " + std::string(typeid(TInterface).name());

  std::lock_guard<std::mutex> lock(registry_mutex_);
  services_[make_service_key<TInterface>(name)] = registration;
}

template <typename TInterface>
void MockServiceRegistry::register_service(std::function<std::shared_ptr<TInterface>()> factory,
                                           ServiceLifetime lifetime, const std::string& name) {
  auto any_factory = [factory]() -> std::any { return factory(); };

  ServiceRegistration registration(std::type_index(typeid(TInterface)));
  registration.service_name = name;
  registration.lifetime = lifetime;
  registration.factory = any_factory;
  registration.is_mock = false;
  registration.description = "Custom Service: " + std::string(typeid(TInterface).name());

  std::lock_guard<std::mutex> lock(registry_mutex_);
  services_[make_service_key<TInterface>(name)] = registration;
}

template <typename TInterface>
void MockServiceRegistry::register_instance(std::shared_ptr<TInterface> instance,
                                            const std::string& name) {
  ServiceRegistration registration(std::type_index(typeid(TInterface)));
  registration.service_name = name;
  registration.lifetime = ServiceLifetime::Singleton;
  registration.singleton_instance = instance;
  registration.is_mock = false;
  registration.description = "Instance: " + std::string(typeid(TInterface).name());

  std::lock_guard<std::mutex> lock(registry_mutex_);
  services_[make_service_key<TInterface>(name)] = registration;
  singleton_instances_[make_service_key<TInterface>(name)] = instance;
}

template <typename TInterface, typename TMock>
void MockServiceRegistry::register_mock(ServiceLifetime lifetime, const std::string& name) {
  auto factory = []() -> std::any {
    return std::static_pointer_cast<TInterface>(std::make_shared<TMock>());
  };

  ServiceRegistration registration(std::type_index(typeid(TInterface)));
  registration.service_name = name;
  registration.lifetime = lifetime;
  registration.factory = factory;
  registration.is_mock = true;
  registration.description = "Mock: " + std::string(typeid(TMock).name());

  std::lock_guard<std::mutex> lock(registry_mutex_);
  services_[make_service_key<TInterface>(name)] = registration;
}

template <typename TInterface>
void MockServiceRegistry::register_mock_instance(std::shared_ptr<TInterface> mock_instance,
                                                 const std::string& name) {
  ServiceRegistration registration(std::type_index(typeid(TInterface)));
  registration.service_name = name;
  registration.lifetime = ServiceLifetime::Singleton;
  registration.singleton_instance = mock_instance;
  registration.is_mock = true;
  registration.description = "Mock Instance: " + std::string(typeid(TInterface).name());

  std::lock_guard<std::mutex> lock(registry_mutex_);
  services_[make_service_key<TInterface>(name)] = registration;
  singleton_instances_[make_service_key<TInterface>(name)] = mock_instance;
}

template <typename T>
std::shared_ptr<T> MockServiceRegistry::resolve(const std::string& name) {
  std::lock_guard<std::mutex> lock(registry_mutex_);

  auto key = make_service_key<T>(name);
  auto it = services_.find(key);

  if (it == services_.end()) {
    return nullptr;
  }

  const auto& registration = it->second;

  switch (registration.lifetime) {
    case ServiceLifetime::Singleton:
      return get_or_create_singleton<T>(registration);

    case ServiceLifetime::Scoped:
      if (current_scope_) {
        return current_scope_->get_service<T>();
      }
      // Fall through to transient if no scope
      [[fallthrough]];

    case ServiceLifetime::Transient:
      return create_service_instance<T>(registration);
  }

  return nullptr;
}

template <typename T>
std::shared_ptr<T> MockServiceRegistry::try_resolve(const std::string& name) {
  try {
    return resolve<T>(name);
  } catch (...) {
    return nullptr;
  }
}

template <typename T>
bool MockServiceRegistry::is_registered(const std::string& name) const {
  std::lock_guard<std::mutex> lock(registry_mutex_);
  auto key = make_service_key<T>(name);
  return services_.find(key) != services_.end();
}

template <typename T>
std::string MockServiceRegistry::make_service_key(const std::string& name) const {
  std::string type_name = typeid(T).name();
  return name.empty() ? type_name : type_name + ":" + name;
}

template <typename T>
std::shared_ptr<T> MockServiceRegistry::create_service_instance(
    const ServiceRegistration& registration) {
  if (registration.factory) {
    auto instance = registration.factory();
    return std::any_cast<std::shared_ptr<T>>(instance);
  }
  return nullptr;
}

template <typename T>
std::shared_ptr<T> MockServiceRegistry::get_or_create_singleton(
    const ServiceRegistration& registration) {
  auto key = make_service_key<T>(registration.service_name);
  auto it = singleton_instances_.find(key);

  if (it != singleton_instances_.end()) {
    return std::any_cast<std::shared_ptr<T>>(it->second);
  }

  auto instance = create_service_instance<T>(registration);
  if (instance) {
    singleton_instances_[key] = instance;
  }

  return instance;
}

template <typename TInterface, typename TMock>
void ScopedServiceRegistry::register_mock(const std::string& name) {
  registry().register_mock<TInterface, TMock>(ServiceLifetime::Scoped, name);
}

template <typename T>
std::shared_ptr<T> ScopedServiceRegistry::resolve(const std::string&) {
  return scope().get_service<T>();
}

template <typename T>
ServiceInjector<T>::ServiceInjector(const std::string& name) : service_name_(name) {}

template <typename T>
std::shared_ptr<T> ServiceInjector<T>::get() const {
  if (!cached_service_) {
    cached_service_ = MockServiceRegistry::instance().resolve<T>(service_name_);
  }
  return cached_service_;
}

template <typename T>
T* ServiceInjector<T>::operator->() const {
  return get().get();
}

template <typename T>
T& ServiceInjector<T>::operator*() const {
  return *get();
}

template <typename T>
bool ServiceInjector<T>::is_available() const {
  return get() != nullptr;
}

}  // namespace SolarSystem::Testing::Mocks
