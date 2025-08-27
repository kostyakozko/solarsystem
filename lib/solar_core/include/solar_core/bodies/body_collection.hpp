#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <optional>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "solar_core/bodies/celestial_body.hpp"

namespace SolarSystem::Bodies {

/**
 * @brief Modern container for managing collections of celestial bodies
 *
 * Provides efficient storage, lookup, and filtering of celestial bodies
 * with modern C++ features like ranges, algorithms, and type safety.
 */
class BodyCollection {
 public:
  // Type aliases for modern C++ style
  using iterator = std::vector<CelestialBody>::iterator;
  using const_iterator = std::vector<CelestialBody>::const_iterator;
  using size_type = std::vector<CelestialBody>::size_type;

  // Constructors
  BodyCollection() = default;
  explicit BodyCollection(std::vector<CelestialBody> bodies);

  // Copy and move semantics
  BodyCollection(const BodyCollection&) = default;
  BodyCollection(BodyCollection&&) = default;
  BodyCollection& operator=(const BodyCollection&) = default;
  BodyCollection& operator=(BodyCollection&&) = default;

  // Destructor
  ~BodyCollection() = default;

  // Body management
  void add_body(CelestialBody body);
  void add_body(CelestialBody::Properties props);
  bool remove_body(std::string_view name);
  void clear();

  // Size and capacity
  [[nodiscard]] size_type size() const noexcept { return bodies_.size(); }
  [[nodiscard]] bool empty() const noexcept { return bodies_.empty(); }
  void reserve(size_type capacity) { bodies_.reserve(capacity); }

  // Element access
  [[nodiscard]] CelestialBody& at(size_type index) { return bodies_.at(index); }
  [[nodiscard]] const CelestialBody& at(size_type index) const { return bodies_.at(index); }
  [[nodiscard]] CelestialBody& operator[](size_type index) { return bodies_[index]; }
  [[nodiscard]] const CelestialBody& operator[](size_type index) const { return bodies_[index]; }

  // Iterators
  [[nodiscard]] iterator begin() { return bodies_.begin(); }
  [[nodiscard]] const_iterator begin() const { return bodies_.begin(); }
  [[nodiscard]] const_iterator cbegin() const { return bodies_.cbegin(); }
  [[nodiscard]] iterator end() { return bodies_.end(); }
  [[nodiscard]] const_iterator end() const { return bodies_.end(); }
  [[nodiscard]] const_iterator cend() const { return bodies_.cend(); }

  // Body lookup
  [[nodiscard]] std::optional<std::reference_wrapper<CelestialBody>> find_body(
      std::string_view name);
  [[nodiscard]] std::optional<std::reference_wrapper<const CelestialBody>> find_body(
      std::string_view name) const;
  [[nodiscard]] bool contains(std::string_view name) const;

  // Filtering operations (using modern C++ ranges when available)
  [[nodiscard]] std::vector<std::reference_wrapper<const CelestialBody>> filter_by_type(
      BodyType type) const;
  [[nodiscard]] std::vector<std::reference_wrapper<const CelestialBody>> filter_by_priority(
      BodyPriority priority) const;
  [[nodiscard]] std::vector<std::reference_wrapper<const CelestialBody>> filter_essential() const;
  [[nodiscard]] std::vector<std::reference_wrapper<const CelestialBody>> filter_available_at(
      std::chrono::system_clock::time_point time) const;

  // Bulk operations
  void apply_to_all(std::function<void(CelestialBody&)> func);
  void apply_to_all(std::function<void(const CelestialBody&)> func) const;

  template <typename Predicate>
  void apply_to_filtered(Predicate pred, std::function<void(CelestialBody&)> func);

  template <typename Predicate>
  void apply_to_filtered(Predicate pred, std::function<void(const CelestialBody&)> func) const;

  // Statistics and analysis
  [[nodiscard]] size_type count_by_type(BodyType type) const;
  [[nodiscard]] size_type count_by_priority(BodyPriority priority) const;
  [[nodiscard]] double total_mass() const;
  [[nodiscard]] Math::Vector3d center_of_mass() const;

  // Enhanced validation and consistency checking
  [[nodiscard]] bool validate() const;
  [[nodiscard]] std::vector<std::string> get_validation_errors() const;
  [[nodiscard]] bool validate_comprehensive() const;
  [[nodiscard]] std::vector<std::string> get_comprehensive_validation_errors() const;

  // Consistency management
  struct ConsistencyReport {
    bool is_consistent;
    std::vector<std::string> issues;
    std::vector<std::string> warnings;
    size_t bodies_checked;
    std::chrono::system_clock::time_point check_time;
  };

  [[nodiscard]] ConsistencyReport check_consistency() const;
  void maintain_consistency();

  // Enhanced operations with error handling
  struct OperationResult {
    bool success;
    std::string error_message;
    std::optional<std::string> warning;
    size_t affected_bodies;
  };

  [[nodiscard]] OperationResult add_body_validated(const CelestialBody& body);
  [[nodiscard]] OperationResult add_body_validated(CelestialBody::Properties props);
  [[nodiscard]] OperationResult remove_body_safe(std::string_view name);
  [[nodiscard]] OperationResult update_body(std::string_view name,
                                            const CelestialBody& updated_body);

  // Enhanced search and filtering with error handling
  [[nodiscard]] std::vector<std::reference_wrapper<const CelestialBody>> search_by_name_pattern(
      const std::string& pattern) const;

  [[nodiscard]] std::vector<std::reference_wrapper<const CelestialBody>> filter_by_mass_range(
      long double min_mass, long double max_mass) const;

  [[nodiscard]] std::vector<std::reference_wrapper<const CelestialBody>>
  filter_by_distance_from_point(const Math::Vector3d& point, long double max_distance) const;

  // Bulk operations with progress reporting
  struct BulkOperationResult {
    size_t total_operations;
    size_t successful_operations;
    size_t failed_operations;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::chrono::milliseconds execution_time;
  };

  [[nodiscard]] BulkOperationResult bulk_add_bodies(const std::vector<CelestialBody>& bodies);
  [[nodiscard]] BulkOperationResult bulk_remove_bodies(const std::vector<std::string>& names);
  [[nodiscard]] BulkOperationResult bulk_update_positions(
      const std::function<Math::Vector3d(const CelestialBody&)>& position_updater);

  // Serialization
  [[nodiscard]] std::string to_json() const;
  [[nodiscard]] std::string summary() const;

 private:
  std::vector<CelestialBody> bodies_;
  std::unordered_map<std::string, size_type> name_index_;

  void rebuild_index();
  void update_index_for_body(size_type index);
};

// Template implementations
template <typename Predicate>
void BodyCollection::apply_to_filtered(Predicate pred, std::function<void(CelestialBody&)> func) {
  for (auto& body : bodies_) {
    if (pred(body)) {
      func(body);
    }
  }
}

template <typename Predicate>
void BodyCollection::apply_to_filtered(Predicate pred,
                                       std::function<void(const CelestialBody&)> func) const {
  for (const auto& body : bodies_) {
    if (pred(body)) {
      func(body);
    }
  }
}

}  // namespace SolarSystem::Bodies
