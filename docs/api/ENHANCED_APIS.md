# Enhanced Solar System Library APIs

This document provides comprehensive documentation for the enhanced APIs introduced in the Solar System Library Core Enhancements project.

## Table of Contents

1. [Enhanced Celestial Body API](#enhanced-celestial-body-api)
2. [Body Factory API](#body-factory-api)
3. [Body Collection API](#body-collection-api)
4. [Simulation Engine API](#simulation-engine-api)
5. [Usage Examples](#usage-examples)
6. [Migration Guide](#migration-guide)
7. [Troubleshooting](#troubleshooting)

## Enhanced Celestial Body API

### Overview

The `CelestialBody` class provides a modern, type-safe interface for representing celestial objects with RAII principles and strong typing.

### Key Features

- **Type Safety**: Strong typing with enums for body types and priorities
- **RAII Compliance**: Automatic resource management
- **Modern C++**: Uses C++20 features like `[[nodiscard]]` and `std::optional`
- **Physics Integration**: Built-in gravitational calculations and state updates
- **Time Awareness**: Support for historical date-based availability

### Class Definition

```cpp
namespace SolarSystem::Bodies {

enum class BodyType {
    Star, Planet, Moon, DwarfPlanet, Asteroid, Spacecraft, Unknown
};

enum class BodyPriority {
    Essential,  // Planets - always included in simulations
    Important,  // Major moons - included by default
    Optional    // Spacecraft - historical date aware
};

class CelestialBody {
public:
    struct Properties {
        std::string name;
        long double mass;         // kg
        Math::Vector3d position;  // meters
        Math::Vector3d velocity;  // m/s
        BodyType type;
        BodyPriority priority;
        std::optional<std::string> jpl_id;
        std::optional<std::chrono::system_clock::time_point> creation_date;
    };

    explicit CelestialBody(Properties props);
    // ... other methods
};
}
```

### Constructor

#### `CelestialBody(Properties props)`

Creates a new celestial body with the specified properties.

**Parameters:**
- `props`: Properties structure containing all body characteristics

**Example:**
```cpp
CelestialBody::Properties earth_props{
    .name = "Earth",
    .mass = 5.972e24,  // kg
    .position = {1.496e11, 0, 0},  // 1 AU from Sun
    .velocity = {0, 29780, 0},     // Orbital velocity
    .type = BodyType::Planet,
    .priority = BodyPriority::Essential,
    .jpl_id = "399"
};

CelestialBody earth(earth_props);
```

### Accessors

All accessor methods are marked `[[nodiscard]]` and `noexcept` for performance and safety.

#### Basic Properties
- `std::string_view name() const noexcept`
- `long double mass() const noexcept`
- `const Math::Vector3d& position() const noexcept`
- `const Math::Vector3d& velocity() const noexcept`
- `const Math::Vector3d& acceleration() const noexcept`
- `BodyType type() const noexcept`
- `BodyPriority priority() const noexcept`

#### Optional Properties
- `const std::optional<std::string>& jpl_id() const noexcept`
- `const std::optional<std::chrono::system_clock::time_point>& creation_date() const noexcept`

### State Modification

#### `void set_position(const Math::Vector3d& position) noexcept`
Updates the body's position vector.

#### `void set_velocity(const Math::Vector3d& velocity) noexcept`
Updates the body's velocity vector.

#### `void set_state(const Math::Vector3d& position, const Math::Vector3d& velocity) noexcept`
Atomically updates both position and velocity.

### Physics Operations

#### `void apply_force(const Math::Vector3d& force, double dt) noexcept`
Applies a force to the body for the given time step, updating acceleration.

**Parameters:**
- `force`: Force vector in Newtons
- `dt`: Time step in seconds

#### `void update_position(double dt) noexcept`
Updates position based on current velocity and acceleration using Verlet integration.

#### `void reset_acceleration() noexcept`
Resets acceleration to zero (typically called at the start of each simulation step).

### Gravitational Interactions

#### `Math::Vector3d gravitational_force_to(const CelestialBody& other) const noexcept`
Calculates the gravitational force this body exerts on another body.

**Returns:** Force vector in Newtons

**Example:**
```cpp
CelestialBody earth(earth_props);
CelestialBody moon(moon_props);

Math::Vector3d force = earth.gravitational_force_to(moon);
```

#### `long double distance_to(const CelestialBody& other) const noexcept`
Calculates the distance between this body and another.

#### `long double distance_squared_to(const CelestialBody& other) const noexcept`
Calculates the squared distance (more efficient for comparisons).

### Utility Functions

#### `bool is_available_at(std::chrono::system_clock::time_point time) const noexcept`
Checks if the body was available at the specified time (useful for spacecraft).

#### `std::string to_string() const`
Returns a string representation of the body for debugging.

### Comparison Operators

Bodies are compared by name:
- `bool operator==(const CelestialBody& other) const noexcept`
- `bool operator!=(const CelestialBody& other) const noexcept`

## Body Factory API

### Overview

The `BodyFactory` class provides intelligent body creation from multiple data sources with comprehensive fallback strategies and data quality assessment.

### Key Features

- **Multiple Data Sources**: JPL HORIZONS, cached data, and hardcoded fallbacks
- **Intelligent Fallback**: Smart data source selection based on quality and availability
- **Data Quality Assessment**: Automatic evaluation of data reliability
- **Validation**: Comprehensive property validation with realistic bounds
- **Error Handling**: Modern error handling with `Expected<T, Error>` types

### Data Sources

```cpp
enum class DataSource {
    JPL_HORIZONS,  // Fetch from JPL API (highest quality)
    CACHED_DATA,   // Use cached ephemeris data
    FALLBACK_DATA  // Use hardcoded constants (most reliable)
};
```

### Fallback Strategies

```cpp
enum class FallbackStrategy {
    STRICT,           // No fallback, fail if preferred source unavailable
    GRACEFUL,         // Try all sources in order, use best available
    INTELLIGENT,      // Assess data quality and choose best source
    PARTIAL_ALLOWED,  // Allow partial data with warnings
    HYBRID           // Combine data from multiple sources
};
```

### Data Quality Assessment

```cpp
enum class DataQuality {
    EXCELLENT,  // Recent JPL data with full validation
    GOOD,       // Cached JPL data or recent fallback
    ACCEPTABLE, // Older cached data or basic fallback
    POOR,       // Very old or incomplete data
    UNKNOWN     // Quality cannot be assessed
};
```

### Factory Configuration

```cpp
struct FactoryConfig {
    DataSource preferred_source = DataSource::JPL_HORIZONS;
    FallbackStrategy fallback_strategy = FallbackStrategy::INTELLIGENT;
    std::chrono::system_clock::time_point target_time = std::chrono::system_clock::now();
    bool enable_validation = true;
    bool allow_partial_data = false;
    double validation_tolerance = 0.1;  // 10% tolerance for property validation
};
```

### Usage Examples

#### Basic Body Creation

```cpp
#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem::Bodies;

// Create factory with default configuration
BodyFactory factory;

// Create Earth with intelligent fallback
auto earth_result = factory.create_body("Earth");
if (earth_result) {
    CelestialBody earth = std::move(*earth_result);
    std::cout << "Created: " << earth.name() << std::endl;
} else {
    std::cerr << "Failed to create Earth: " << earth_result.error() << std::endl;
}
```

#### Advanced Configuration

```cpp
// Configure factory for specific use case
BodyFactory::FactoryConfig config{
    .preferred_source = DataSource::CACHED_DATA,
    .fallback_strategy = FallbackStrategy::GRACEFUL,
    .target_time = std::chrono::system_clock::now() - std::chrono::hours(24),
    .enable_validation = true,
    .allow_partial_data = true,
    .validation_tolerance = 0.05  // 5% tolerance
};

BodyFactory factory(config);

// Create multiple bodies
std::vector<std::string> body_names = {"Sun", "Earth", "Mars", "Jupiter"};
std::vector<CelestialBody> bodies;

for (const auto& name : body_names) {
    auto result = factory.create_body(name);
    if (result) {
        bodies.push_back(std::move(*result));
    } else {
        std::cerr << "Warning: Could not create " << name << ": "
                  << result.error() << std::endl;
    }
}
```

#### Batch Creation with Quality Assessment

```cpp
// Create all essential bodies for a simulation
auto collection_result = factory.create_essential_bodies();
if (collection_result) {
    BodyCollection bodies = std::move(*collection_result);

    // Check data quality
    auto quality_report = factory.get_quality_report();
    std::cout << "Data quality summary:" << std::endl;
    for (const auto& [body_name, quality] : quality_report) {
        std::cout << "  " << body_name << ": " << quality_to_string(quality) << std::endl;
    }
} else {
    std::cerr << "Failed to create essential bodies: " << collection_result.error() << std::endl;
}
```

### Error Handling

The factory uses modern error handling with `Expected<T, Error>` types:

```cpp
// Check for errors before using results
auto result = factory.create_body("NonexistentBody");
if (!result) {
    switch (result.error().type) {
        case FactoryError::BODY_NOT_FOUND:
            std::cerr << "Body not found in any data source" << std::endl;
            break;
        case FactoryError::VALIDATION_FAILED:
            std::cerr << "Body data failed validation: " << result.error().message << std::endl;
            break;
        case FactoryError::DATA_SOURCE_UNAVAILABLE:
            std::cerr << "All data sources unavailable" << std::endl;
            break;
    }
}
```

### Validation Features

The factory includes comprehensive validation:

- **Mass Validation**: Realistic bounds for different body types
- **Orbital Parameter Validation**: Physically reasonable orbits
- **Cross-Validation**: Consistency between related properties
- **Dependency Validation**: Proper relationships between bodies

```cpp
// Custom validation configuration
ValidationConfig validation{
    .mass_bounds = {
        {BodyType::Planet, {1e20, 1e30}},      // kg
        {BodyType::Moon, {1e16, 1e25}},        // kg
        {BodyType::Asteroid, {1e10, 1e20}}     // kg
    },
    .distance_bounds = {1e6, 1e15},            // meters
    .velocity_bounds = {0, 1e6}                // m/s
};

factory.set_validation_config(validation);
```
## Body Collection API

### Overview

The `BodyCollection` class provides a modern, efficient container for managing collections of celestial bodies with advanced filtering, searching, and iteration capabilities.

### Key Features

- **Modern C++ Container**: STL-compatible with iterators and ranges
- **Efficient Lookup**: Hash-based name lookup for O(1) access
- **Advanced Filtering**: Predicate-based filtering with C++20 ranges
- **Type Safety**: Strong typing and const-correctness
- **Memory Efficient**: Optimized storage and minimal overhead

### Container Interface

```cpp
class BodyCollection {
public:
    // Type aliases for STL compatibility
    using iterator = std::vector<CelestialBody>::iterator;
    using const_iterator = std::vector<CelestialBody>::const_iterator;
    using size_type = std::vector<CelestialBody>::size_type;

    // Standard container operations
    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    size_type size() const noexcept;
    bool empty() const noexcept;
    void clear();
};
```

### Body Management

#### Adding Bodies

```cpp
// Add existing body
CelestialBody earth(earth_props);
collection.add_body(std::move(earth));

// Add body from properties
collection.add_body(CelestialBody::Properties{
    .name = "Mars",
    .mass = 6.39e23,
    .position = {2.279e11, 0, 0},
    .velocity = {0, 24077, 0},
    .type = BodyType::Planet,
    .priority = BodyPriority::Essential
});

// Batch addition
std::vector<CelestialBody> planets = create_planets();
collection.add_bodies(std::move(planets));
```

#### Removing Bodies

```cpp
// Remove by name
bool removed = collection.remove_body("Pluto");
if (removed) {
    std::cout << "Pluto removed from collection" << std::endl;
}

// Remove by predicate
auto removed_count = collection.remove_if([](const CelestialBody& body) {
    return body.type() == BodyType::Spacecraft &&
           !body.is_available_at(std::chrono::system_clock::now());
});
```

### Lookup and Access

#### Direct Access

```cpp
// Find by name (O(1) lookup)
auto earth_opt = collection.find_body("Earth");
if (earth_opt) {
    const CelestialBody& earth = *earth_opt;
    std::cout << "Earth mass: " << earth.mass() << " kg" << std::endl;
}

// Access by index
if (!collection.empty()) {
    const CelestialBody& first = collection[0];
    const CelestialBody& last = collection.at(collection.size() - 1);
}
```

#### Existence Checks

```cpp
// Check if body exists
if (collection.contains("Jupiter")) {
    std::cout << "Jupiter is in the collection" << std::endl;
}

// Check multiple bodies
std::vector<std::string> required = {"Sun", "Earth", "Moon"};
bool has_all = collection.contains_all(required);
```

### Filtering and Searching

#### Type-Based Filtering

```cpp
// Get all planets
auto planets = collection.filter_by_type(BodyType::Planet);
std::cout << "Found " << planets.size() << " planets" << std::endl;

// Get essential bodies only
auto essential = collection.filter_by_priority(BodyPriority::Essential);
```

#### Custom Predicate Filtering

```cpp
// Filter by mass range
auto massive_bodies = collection.filter([](const CelestialBody& body) {
    return body.mass() > 1e24;  // More massive than Earth
});

// Filter by distance from Sun (assuming Sun is at origin)
auto inner_system = collection.filter([](const CelestialBody& body) {
    return body.position().magnitude() < 5e11;  // Within 5 AU
});
```

#### Pattern Matching

```cpp
// Find bodies matching name pattern
auto galilean_moons = collection.find_matching(R"(Io|Europa|Ganymede|Callisto)");

// Find by JPL ID pattern
auto spacecraft = collection.find_by_jpl_pattern(R"(-\d+)");  // Negative IDs for spacecraft
```

### Advanced Operations

#### Sorting

```cpp
// Sort by mass (descending)
collection.sort_by_mass(SortOrder::Descending);

// Sort by distance from a point
Math::Vector3d reference_point{0, 0, 0};  // Sun position
collection.sort_by_distance(reference_point);

// Custom sorting
collection.sort([](const CelestialBody& a, const CelestialBody& b) {
    return a.name() < b.name();  // Alphabetical
});
```

#### Statistics and Analysis

```cpp
// Collection statistics
auto stats = collection.get_statistics();
std::cout << "Total mass: " << stats.total_mass << " kg" << std::endl;
std::cout << "Center of mass: " << stats.center_of_mass.to_string() << std::endl;
std::cout << "Bounding sphere radius: " << stats.bounding_radius << " m" << std::endl;

// Mass distribution
auto mass_dist = collection.get_mass_distribution();
for (const auto& [type, mass] : mass_dist) {
    std::cout << body_type_to_string(type) << ": " << mass << " kg" << std::endl;
}
```

### Range-Based Operations (C++20)

```cpp
#include <ranges>

// Modern C++20 range operations
auto heavy_planets = collection
    | std::views::filter([](const auto& body) {
        return body.type() == BodyType::Planet && body.mass() > 1e25;
      })
    | std::views::transform([](const auto& body) {
        return body.name();
      });

// Print names of heavy planets
for (const auto& name : heavy_planets) {
    std::cout << name << std::endl;
}
```

### Consistency and Validation

#### Consistency Checking

```cpp
// Check collection consistency
auto consistency_report = collection.check_consistency();
if (!consistency_report.is_valid) {
    std::cerr << "Collection consistency issues:" << std::endl;
    for (const auto& issue : consistency_report.issues) {
        std::cerr << "  " << issue.description << std::endl;
    }
}
```

#### Automatic Maintenance

```cpp
// Enable automatic consistency maintenance
collection.enable_auto_maintenance(true);

// Manual maintenance operations
collection.remove_duplicates();
collection.validate_all_bodies();
collection.update_internal_indices();
```

### Performance Considerations

- **Lookup Performance**: O(1) name-based lookup using internal hash map
- **Memory Efficiency**: Contiguous storage for cache-friendly iteration
- **Lazy Evaluation**: Filtering operations use lazy evaluation where possible
- **Move Semantics**: Full support for move operations to avoid copies

### Thread Safety

The `BodyCollection` is not thread-safe by default. For concurrent access:

```cpp
#include <shared_mutex>

class ThreadSafeBodyCollection {
    mutable std::shared_mutex mutex_;
    BodyCollection collection_;

public:
    // Read operations (shared lock)
    auto find_body(std::string_view name) const {
        std::shared_lock lock(mutex_);
        return collection_.find_body(name);
    }

    // Write operations (exclusive lock)
    void add_body(CelestialBody body) {
        std::unique_lock lock(mutex_);
        collection_.add_body(std::move(body));
    }
};
```
## Simulation Engine API

### Overview

The enhanced `SimulationEngine` provides high-performance N-body gravitational simulation with adaptive time stepping, energy conservation monitoring, and comprehensive state tracking.

### Key Features

- **Adaptive Time Stepping**: Automatic timestep adjustment for accuracy and performance
- **Energy Conservation**: Real-time monitoring of system energy conservation
- **Collision Detection**: Optional collision detection and handling
- **Progress Callbacks**: Real-time simulation progress monitoring
- **State Persistence**: Save and restore simulation states
- **Multi-threading**: Parallel force calculations for large systems

### Configuration

```cpp
struct SimulationConfig {
    double time_step = 30.0;                      // Time step in seconds
    double gravitational_constant = 6.67430e-11;  // G in m³/kg/s²
    bool use_adaptive_timestep = false;           // Adaptive time stepping
    double max_timestep = 3600.0;                 // Maximum timestep (1 hour)
    double min_timestep = 1.0;                    // Minimum timestep (1 second)
    double tolerance = 1e-12;                     // Error tolerance for adaptive stepping
    bool enable_collision_detec
