/**
 * @file data_processor.cpp
 * @brief Implementation of data loading and preparation system
 */

#include "solar_analysis/data_processor.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace SolarSystem::Analysis {

struct DataProcessor::Impl {
  DataProcessorConfig config;
  std::unordered_map<std::string, std::vector<StateVector>> cache;

  explicit Impl(const DataProcessorConfig& cfg) : config(cfg) {}
};

DataProcessor::DataProcessor() : impl_(std::make_unique<Impl>(DataProcessorConfig{})) {}

DataProcessor::DataProcessor(const DataProcessorConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

DataProcessor::~DataProcessor() = default;

DataProcessor::DataProcessor(DataProcessor&&) noexcept = default;
DataProcessor& DataProcessor::operator=(DataProcessor&&) noexcept = default;

void DataProcessor::set_config(const DataProcessorConfig& config) { impl_->config = config; }

const DataProcessorConfig& DataProcessor::config() const { return impl_->config; }

bool DataProcessor::load_body_data(const std::string& body_name) {
  // Generate sample data for the body
  std::vector<StateVector> data;
  auto now = std::chrono::system_clock::now();

  // Create 100 data points spanning 100 days
  for (int i = 0; i < 100; ++i) {
    StateVector sv;
    sv.timestamp = now - std::chrono::hours(24 * i);

    // Simple circular orbit approximation
    double angle = static_cast<double>(i) * 0.0628;  // ~100 points per orbit
    double radius = 1.5e8;                           // ~1 AU in km
    sv.position = Math::Vector3d(radius * std::cos(angle), radius * std::sin(angle), 0.0);

    double velocity = 30.0;  // ~30 km/s
    sv.velocity = Math::Vector3d(-velocity * std::sin(angle), velocity * std::cos(angle), 0.0);

    data.push_back(sv);
  }

  // Sort by timestamp
  std::sort(data.begin(), data.end(),
            [](const StateVector& a, const StateVector& b) { return a.timestamp < b.timestamp; });

  if (impl_->config.enable_caching) {
    impl_->cache[body_name] = std::move(data);
  }

  return true;
}

bool DataProcessor::load_body_data(const std::string& body_name, const TimeRange& range) {
  if (!load_body_data(body_name)) {
    return false;
  }

  // Filter to time range
  auto& data = impl_->cache[body_name];
  auto it = std::remove_if(data.begin(), data.end(), [&range](const StateVector& sv) {
    return !range.contains(sv.timestamp);
  });
  data.erase(it, data.end());

  return !data.empty();
}

std::vector<StateVector> DataProcessor::get_data(const std::string& body_name) const {
  auto it = impl_->cache.find(body_name);
  if (it != impl_->cache.end()) {
    return it->second;
  }
  return {};
}

std::vector<StateVector> DataProcessor::get_data(const std::string& body_name,
                                                 const TimeRange& range) const {
  auto data = get_data(body_name);
  std::vector<StateVector> filtered;
  std::copy_if(data.begin(), data.end(), std::back_inserter(filtered),
               [&range](const StateVector& sv) { return range.contains(sv.timestamp); });
  return filtered;
}

std::optional<StateVector> DataProcessor::get_state_at(
    const std::string& body_name, std::chrono::system_clock::time_point time) const {
  auto data = get_data(body_name);
  if (data.empty()) {
    return std::nullopt;
  }

  // Find surrounding points for interpolation
  auto it = std::lower_bound(data.begin(), data.end(), time,
                             [](const StateVector& sv, std::chrono::system_clock::time_point t) {
                               return sv.timestamp < t;
                             });

  if (it == data.end()) {
    return data.back();
  }
  if (it == data.begin()) {
    return data.front();
  }

  auto before = std::prev(it);
  return interpolate(*before, *it, time);
}

DataQuality DataProcessor::validate_data(const std::string& body_name) const {
  return validate_data(get_data(body_name));
}

DataQuality DataProcessor::validate_data(const std::vector<StateVector>& data) const {
  DataQuality quality;
  quality.total_points = data.size();

  for (const auto& sv : data) {
    if (sv.is_valid() && sv.position.magnitude() > 0) {
      ++quality.valid_points;
    } else {
      ++quality.invalid_points;
      quality.issues.push_back("Invalid state vector detected");
    }
  }

  quality.completeness = quality.total_points > 0 ? static_cast<double>(quality.valid_points) /
                                                        static_cast<double>(quality.total_points)
                                                  : 0.0;

  return quality;
}

std::optional<StateVector> DataProcessor::interpolate(
    const StateVector& before, const StateVector& after,
    std::chrono::system_clock::time_point t) const {
  if (t < before.timestamp || t > after.timestamp) {
    return std::nullopt;
  }

  auto total = std::chrono::duration<double>(after.timestamp - before.timestamp).count();
  if (total <= 0) {
    return before;
  }

  auto elapsed = std::chrono::duration<double>(t - before.timestamp).count();
  double ratio = elapsed / total;

  StateVector result;
  result.timestamp = t;
  result.position = before.position + (after.position - before.position) * ratio;
  result.velocity = before.velocity + (after.velocity - before.velocity) * ratio;

  return result;
}

void DataProcessor::clear_cache() { impl_->cache.clear(); }

void DataProcessor::clear_cache(const std::string& body_name) { impl_->cache.erase(body_name); }

size_t DataProcessor::cache_size() const { return impl_->cache.size(); }

std::vector<std::string> DataProcessor::cached_bodies() const {
  std::vector<std::string> bodies;
  bodies.reserve(impl_->cache.size());
  for (const auto& [name, _] : impl_->cache) {
    bodies.push_back(name);
  }
  return bodies;
}

bool DataProcessor::has_data(const std::string& body_name) const {
  return impl_->cache.find(body_name) != impl_->cache.end();
}

size_t DataProcessor::data_point_count(const std::string& body_name) const {
  auto it = impl_->cache.find(body_name);
  return it != impl_->cache.end() ? it->second.size() : 0;
}

std::optional<TimeRange> DataProcessor::data_time_range(const std::string& body_name) const {
  auto it = impl_->cache.find(body_name);
  if (it == impl_->cache.end() || it->second.empty()) {
    return std::nullopt;
  }

  const auto& data = it->second;
  return TimeRange{data.front().timestamp, data.back().timestamp};
}

}  // namespace SolarSystem::Analysis
