#pragma once

#include <algorithm>
#include <functional>
#include <ranges>
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

  // Validation
  [[nodiscard]] bool validate() const;
  [[nodiscard]] std::vector<std::string> get_validation_errors() const;

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
