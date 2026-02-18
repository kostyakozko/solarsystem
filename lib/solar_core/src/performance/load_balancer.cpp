/**
 * @file load_balancer.cpp
 * @brief Implementation of load balancing
 */

#include "solar_core/performance/load_balancer.hpp"

#include <algorithm>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace SolarSystem::Performance {

struct LoadBalancer::Impl {
  LoadBalancerConfig config;
  std::vector<Backend> backends;
  size_t round_robin_index = 0;
  LoadBalancerStats stats;
  mutable std::mutex mutex;
  std::thread health_check_thread;
  std::atomic<bool> running{false};

  explicit Impl(LoadBalancerConfig cfg) : config(std::move(cfg)) {}

  ~Impl() {
    if (running.load()) {
      running.store(false);
      if (health_check_thread.joinable()) {
        health_check_thread.join();
      }
    }
  }

  std::optional<Backend> select_round_robin() {
    auto healthy = get_healthy_backends_internal();
    if (healthy.empty()) return std::nullopt;

    auto& backend = healthy[round_robin_index % healthy.size()];
    round_robin_index++;

    return backend;
  }

  std::optional<Backend> select_least_connections() {
    auto healthy = get_healthy_backends_internal();
    if (healthy.empty()) return std::nullopt;

    auto it =
        std::min_element(healthy.begin(), healthy.end(), [](const Backend& a, const Backend& b) {
          return a.active_connections < b.active_connections;
        });

    return *it;
  }

  std::optional<Backend> select_random() {
    auto healthy = get_healthy_backends_internal();
    if (healthy.empty()) return std::nullopt;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(healthy.size()) - 1);

    return healthy[static_cast<size_t>(dis(gen))];
  }

  std::optional<Backend> select_weighted_round_robin() {
    auto healthy = get_healthy_backends_internal();
    if (healthy.empty()) return std::nullopt;

    // Calculate total weight
    int total_weight = 0;
    for (const auto& backend : healthy) {
      total_weight += backend.weight;
    }

    // Select based on weight
    int target = static_cast<int>(round_robin_index % static_cast<size_t>(total_weight));
    round_robin_index++;

    int current_weight = 0;
    for (const auto& backend : healthy) {
      current_weight += backend.weight;
      if (target < current_weight) {
        return backend;
      }
    }

    return healthy[0];
  }

  std::vector<Backend> get_healthy_backends_internal() const {
    std::vector<Backend> healthy;
    std::copy_if(backends.begin(), backends.end(), std::back_inserter(healthy),
                 [](const Backend& b) { return b.healthy; });
    return healthy;
  }
};

LoadBalancer::LoadBalancer(LoadBalancerConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

LoadBalancer::~LoadBalancer() = default;

LoadBalancer::LoadBalancer(LoadBalancer&&) noexcept = default;
LoadBalancer& LoadBalancer::operator=(LoadBalancer&&) noexcept = default;

void LoadBalancer::add_backend(const Backend& backend) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->backends.push_back(backend);
  impl_->stats.total_backends++;
  if (backend.healthy) {
    impl_->stats.healthy_backends++;
  }
}

void LoadBalancer::remove_backend(const std::string& backend_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = std::find_if(impl_->backends.begin(), impl_->backends.end(),
                         [&backend_id](const Backend& b) { return b.id == backend_id; });

  if (it != impl_->backends.end()) {
    if (it->healthy) {
      impl_->stats.healthy_backends--;
    }
    impl_->backends.erase(it);
    impl_->stats.total_backends--;
  }
}

std::optional<Backend> LoadBalancer::select_backend() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  impl_->stats.total_requests++;

  std::optional<Backend> selected;

  switch (impl_->config.algorithm) {
    case LoadBalancingAlgorithm::ROUND_ROBIN:
      selected = impl_->select_round_robin();
      break;
    case LoadBalancingAlgorithm::LEAST_CONNECTIONS:
      selected = impl_->select_least_connections();
      break;
    case LoadBalancingAlgorithm::RANDOM:
      selected = impl_->select_random();
      break;
    case LoadBalancingAlgorithm::WEIGHTED_ROUND_ROBIN:
      selected = impl_->select_weighted_round_robin();
      break;
  }

  if (selected) {
    impl_->stats.successful_requests++;
  } else {
    impl_->stats.failed_requests++;
  }

  return selected;
}

void LoadBalancer::set_backend_health(const std::string& backend_id, bool healthy) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = std::find_if(impl_->backends.begin(), impl_->backends.end(),
                         [&backend_id](const Backend& b) { return b.id == backend_id; });

  if (it != impl_->backends.end()) {
    bool was_healthy = it->healthy;
    it->healthy = healthy;

    if (was_healthy && !healthy) {
      impl_->stats.healthy_backends--;
    } else if (!was_healthy && healthy) {
      impl_->stats.healthy_backends++;
    }
  }
}

void LoadBalancer::update_backend_connections(const std::string& backend_id, int delta) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = std::find_if(impl_->backends.begin(), impl_->backends.end(),
                         [&backend_id](const Backend& b) { return b.id == backend_id; });

  if (it != impl_->backends.end()) {
    it->active_connections =
        static_cast<size_t>(std::max(0, static_cast<int>(it->active_connections) + delta));
  }
}

std::vector<Backend> LoadBalancer::get_backends() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->backends;
}

std::vector<Backend> LoadBalancer::get_healthy_backends() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->get_healthy_backends_internal();
}

LoadBalancerStats LoadBalancer::get_statistics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->stats;
}

void LoadBalancer::start_health_checks() {
  if (impl_->running.exchange(true)) {
    return;  // Already running
  }

  impl_->health_check_thread = std::thread([this]() {
    while (impl_->running.load()) {
      std::this_thread::sleep_for(impl_->config.health_check_interval);

      std::lock_guard<std::mutex> lock(impl_->mutex);
      size_t healthy_count = 0;
      for (auto& backend : impl_->backends) {
        // Mark backend unhealthy if connections exceed capacity threshold
        if (backend.active_connections > static_cast<size_t>(backend.weight) * 100) {
          backend.healthy = false;
        } else if (!backend.healthy && backend.active_connections == 0) {
          backend.healthy = true;
        }

        backend.last_health_check = std::chrono::system_clock::now();
        if (backend.healthy) healthy_count++;
      }
      impl_->stats.healthy_backends = healthy_count;
    }
  });
}

void LoadBalancer::stop_health_checks() {
  impl_->running.store(false);
  if (impl_->health_check_thread.joinable()) {
    impl_->health_check_thread.join();
  }
}

}  // namespace SolarSystem::Performance
