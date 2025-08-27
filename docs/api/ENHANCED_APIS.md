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
## Simulation Engine API

### Overview

The enhanced `SimulationEngine` provides a modern, configurable physics simulation system with adaptive time stepping, collision detection, and comprehensive monitoring capabilities.

### Key Features

- **Adaptive Time Stepping**: Automatic time step adjustment for accuracy and stability
- **Collision Detection**: Optional collision detection with configurable thresholds
- **Energy Conservation**: Monitoring of total, kinetic, and potential energy
- **Progress Callbacks**: Real-time simulation monitoring and progress reporting
- **Modern Configuration**: Structured configuration with sensible defaults

### Configuration

```cpp
namespace SolarSystem::Simulation {

struct SimulationConfig {
    double time_step = 30.0;                      // Time step in seconds
    double gravitational_constant = 6.67430e-11;  // G in m³/kg/s²
    bool use_adaptive_timestep = false;           // Adaptive time stepping
    double max_timestep = 3600.0;                 // Maximum timestep (1 hour)
    double min_timestep = 1.0;                    // Minimum timestep (1 second)
    double tolerance = 1e-12;                     // Error tolerance for adaptive stepping
    bool enable_collision_detection = false;      // Collision detection
    double collision_threshold = 1e6;             // Collision distance threshold (m)
};
}
```

### Simulation State Monitoring

```cpp
struct SimulationState {
    double current_time = 0.0;                             // Current simulation time (seconds)
    std::chrono::system_clock::time_point reference_time;  // Reference epoch
    size_t iteration_count = 0;                            // Number of iterations performed
    long double total_energy = 0.0;                        // Total system energy
    long double kinetic_energy = 0.0;                      // Total kinetic energy
    long double potential_energy = 0.0;                    // Total potential energy
    Math::Vector3d center_of_mass{};                       // System center of mass
    Math::Vector3d total_momentum{};                       // Total system momentum
    double largest_timestep = 0.0;                         // Largest timestep used
    double smallest_timestep = 0.0;                        // Smallest timestep used
};
```

### Callback Functions

```cpp
// Progress monitoring callback
using ProgressCallback = std::function<void(const SimulationState&)>;

// Collision detection callback
using CollisionCallback = std::function<void(const Bodies::CelestialBody&, const Bodies::CelestialBody&)>;
```

### Basic Usage

```cpp
#include "solar_core/simulation/simulation_engine.hpp"
#include "solar_core/bodies/body_factory.hpp"

using namespace SolarSystem;

// Create simulation configuration
Simulation::SimulationConfig config{
    .time_step = 60.0,                    // 1 minute steps
    .use_adaptive_timestep = true,        // Enable adaptive stepping
    .enable_collision_detection = true,   // Enable collision detection
    .collision_threshold = 1e7            // 10,000 km threshold
};

// Create simulation engine
Simulation::SimulationEngine engine(config);

// Create bodies using factory
Bodies::BodyFactory factory;
auto earth = factory.create_body("Earth");
auto moon = factory.create_body("Moon");

if (earth && moon) {
    Bodies::BodyCollection bodies;
    bodies.add_body(std::move(*earth));
    bodies.add_body(std::move(*moon));

    // Initialize simulation
    engine.initialize(std::move(bodies));

    // Run simulation for 1 day
    double simulation_duration = 24 * 3600; // 24 hours in seconds
    engine.run_for_duration(simulation_duration);
}
```

### Advanced Usage with Callbacks

```cpp
// Progress monitoring callback
auto progress_callback = [](const Simulation::SimulationState& state) {
    std::cout << "Time: " << state.current_time << "s, "
              << "Energy: " << state.total_energy << "J, "
              << "Iterations: " << state.iteration_count << std::endl;
};

// Collision detection callback
auto collision_callback = [](const Bodies::CelestialBody& body1, const Bodies::CelestialBody& body2) {
    std::cout << "Collision detected between " << body1.name()
              << " and " << body2.name() << std::endl;
};

// Configure engine with callbacks
engine.set_progress_callback(progress_callback);
engine.set_collision_callback(collision_callback);

// Run simulation with real-time monitoring
engine.run_with_monitoring(simulation_duration, std::chrono::seconds(1));
```

### Adaptive Time Stepping

```cpp
// Configure adaptive time stepping
Simulation::SimulationConfig adaptive_config{
    .time_step = 30.0,              // Initial time step
    .use_adaptive_timestep = true,  // Enable adaptive stepping
    .max_timestep = 3600.0,         // Maximum 1 hour
    .min_timestep = 0.1,            // Minimum 0.1 seconds
    .tolerance = 1e-10              // High precision tolerance
};

Simulation::SimulationEngine adaptive_engine(adaptive_config);

// The engine will automatically adjust time steps based on:
// - System dynamics (close encounters require smaller steps)
// - Energy conservation requirements
// - Numerical stability considerations
```

### Energy Conservation Monitoring

```cpp
// Monitor energy conservation during simulation
engine.set_progress_callback([](const Simulation::SimulationState& state) {
    // Calculate energy conservation
    static long double initial_energy = 0.0;
    if (state.iteration_count == 1) {
        initial_energy = state.total_energy;
    }

    long double energy_drift = std::abs(state.total_energy - initial_energy) / initial_energy;

    if (energy_drift > 1e-6) {  // 0.0001% tolerance
        std::cout << "Warning: Energy drift detected: " << energy_drift * 100 << "%" << std::endl;
    }
});
```

### Performance Optimization

```cpp
// High-performance configuration for large simulations
Simulation::SimulationConfig performance_config{
    .time_step = 300.0,                   // Larger time steps for speed
    .use_adaptive_timestep = false,       // Disable for consistent performance
    .enable_collision_detection = false,  // Disable if not needed
    .gravitational_constant = 6.67430e-11 // Standard value
};

// For very large systems, consider:
// - Hierarchical time stepping
// - Symplectic integrators
// - Parallel processing (future enhancement)
```
## Usage Examples

### Complete Solar System Simulation

```cpp
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/simulation/simulation_engine.hpp"
#include <iostream>
#include <chrono>

int main() {
    using namespace SolarSystem;

    try {
        // Configure factory for intelligent fallback
        Bodies::BodyFactory::FactoryConfig factory_config{
            .preferred_source = Bodies::BodyFactory::DataSource::JPL_HORIZONS,
            .fallback_strategy = Bodies::BodyFactory::FallbackStrategy::INTELLIGENT,
            .enable_validation = true
        };

        Bodies::BodyFactory factory(factory_config);

        // Create essential solar system bodies
        auto collection_result = factory.create_essential_bodies();
        if (!collection_result) {
            std::cerr << "Failed to create solar system: " << collection_result.error() << std::endl;
            return 1;
        }

        Bodies::BodyCollection bodies = std::move(*collection_result);
        std::cout << "Created " << bodies.size() << " celestial bodies" << std::endl;

        // Configure simulation for accuracy
        Simulation::SimulationConfig sim_config{
            .time_step = 3600.0,              // 1 hour steps
            .use_adaptive_timestep = true,    // Adaptive for accuracy
            .max_timestep = 86400.0,          // Max 1 day
            .min_timestep = 60.0,             // Min 1 minute
            .tolerance = 1e-12,               // High precision
            .enable_collision_detection = false
        };

        Simulation::SimulationEngine engine(sim_config);

        // Set up progress monitoring
        engine.set_progress_callback([](const Simulation::SimulationState& state) {
            if (state.iteration_count % 24 == 0) {  // Every 24 hours
                std::cout << "Day " << (state.current_time / 86400.0)
                          << ": Energy = " << state.total_energy << " J" << std::endl;
            }
        });

        // Initialize and run simulation
        engine.initialize(std::move(bodies));

        // Simulate one year
        double one_year = 365.25 * 24 * 3600;  // seconds
        engine.run_for_duration(one_year);

        std::cout << "Simulation completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Simulation error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

### Custom Body Creation and Validation

```cpp
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/bodies/body_collection.hpp"

// Create a custom asteroid
Bodies::CelestialBody::Properties asteroid_props{
    .name = "CustomAsteroid",
    .mass = 1e15,  // 1 petagram
    .position = {4e11, 0, 0},  // 4 AU from Sun
    .velocity = {0, 15000, 0}, // Orbital velocity
    .type = Bodies::BodyType::Asteroid,
    .priority = Bodies::BodyPriority::Optional,
    .jpl_id = std::nullopt,
    .creation_date = std::chrono::system_clock::now()
};

Bodies::CelestialBody asteroid(asteroid_props);

// Add to collection with validation
Bodies::BodyCollection collection;
collection.add_body(std::move(asteroid));

// Validate collection consistency
auto consistency_report = collection.check_consistency();
if (!consistency_report.is_valid) {
    for (const auto& issue : consistency_report.issues) {
        std::cout << "Validation issue: " << issue.description << std::endl;
    }
}
```

### Advanced Filtering and Analysis

```cpp
#include "solar_core/bodies/body_collection.hpp"
#include <ranges>

// Assume we have a populated collection
Bodies::BodyCollection solar_system = create_solar_system();

// Find all planets more massive than Earth
auto massive_planets = solar_system.filter([](const Bodies::CelestialBody& body) {
    return body.type() == Bodies::BodyType::Planet && body.mass() > 5.972e24;
});

std::cout << "Planets more massive than Earth:" << std::endl;
for (const auto& planet : massive_planets) {
    std::cout << "  " << planet.name() << ": " << planet.mass() << " kg" << std::endl;
}

// Using C++20 ranges for complex filtering
auto inner_system_moons = solar_system
    | std::views::filter([](const auto& body) {
        return body.type() == Bodies::BodyType::Moon;
      })
    | std::views::filter([](const auto& body) {
        return body.position().magnitude() < 5e11; // Within 5 AU
      });

// Calculate system statistics
auto stats = solar_system.get_statistics();
std::cout << "System center of mass: " << stats.center_of_mass.to_string() << std::endl;
std::cout << "Total system mass: " << stats.total_mass << " kg" << std::endl;
```

### Error Handling Best Practices

```cpp
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/utils/expected.hpp"

Bodies::BodyFactory factory;

// Proper error handling with Expected<T, Error>
auto earth_result = factory.create_body("Earth");
if (!earth_result) {
    // Handle specific error types
    const auto& error = earth_result.error();
    switch (error.type) {
        case Bodies::FactoryError::BODY_NOT_FOUND:
            std::cerr << "Earth not found in any data source" << std::endl;
            // Try alternative approach
            break;

        case Bodies::FactoryError::VALIDATION_FAILED:
            std::cerr << "Earth data validation failed: " << error.message << std::endl;
            // Log validation details
            break;

        case Bodies::FactoryError::DATA_SOURCE_UNAVAILABLE:
            std::cerr << "All data sources unavailable, using fallback" << std::endl;
            // Switch to offline mode
            break;

        default:
            std::cerr << "Unknown error creating Earth: " << error.message << std::endl;
    }
    return;
}

// Use the successfully created body
Bodies::CelestialBody earth = std::move(*earth_result);
std::cout << "Successfully created Earth with mass: " << earth.mass() << " kg" << std::endl;
```

### Performance Monitoring Integration

```cpp
#include "solar_core/simulation/simulation_engine.hpp"
#include "tests/utils/performance_regression_system.h"

// Initialize performance monitoring
initialize_performance_regression_testing("performance_baselines.txt");

// Register simulation performance test
register_performance_test("SimulationEngine", "solar_system_simulation", []() {
    // Create and run simulation
    Bodies::BodyFactory factory;
    auto bodies = factory.create_essential_bodies();

    Simulation::SimulationEngine engine;
    engine.initialize(std::move(*bodies));

    auto start = std::chrono::high_resolution_clock::now();
    engine.run_for_duration(86400.0);  // 1 day
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    return create_performance_metrics(duration.count(), get_memory_usage());
});

// Run performance regression tests
auto alerts = run_performance_regression_tests();
if (!alerts.empty()) {
    std::cout << "Performance regressions detected:" << std::endl;
    for (const auto& alert : alerts) {
        std::cout << "  " << alert.component << ": " << alert.message << std::endl;
    }
}
```
## Migration Guide

### Migrating from Placeholder Implementations

This section provides step-by-step guidance for migrating from the original placeholder implementations to the enhanced APIs.

#### 1. Celestial Body Migration

**Before (Placeholder Implementation):**
```cpp
// Old placeholder approach
struct CelestialBody {
    std::string name;
    double mass;
    double x, y, z;
    double vx, vy, vz;
};

CelestialBody earth;
earth.name = "Earth";
earth.mass = 5.972e24;
earth.x = 1.496e11;
// ... manual initialization
```

**After (Enhanced Implementation):**
```cpp
// New enhanced approach
#include "solar_core/bodies/celestial_body.hpp"

using namespace SolarSystem::Bodies;

CelestialBody::Properties earth_props{
    .name = "Earth",
    .mass = 5.972e24,
    .position = {1.496e11, 0, 0},
    .velocity = {0, 29780, 0},
    .type = BodyType::Planet,
    .priority = BodyPriority::Essential
};

CelestialBody earth(earth_props);
```

**Migration Steps:**
1. Replace manual struct initialization with `Properties` structure
2. Use `Math::Vector3d` for position and velocity instead of separate x,y,z components
3. Add type and priority classification
4. Utilize RAII and move semantics for better performance

#### 2. Body Factory Migration

**Before (Manual Creation):**
```cpp
// Old manual approach
std::vector<CelestialBody> create_solar_system() {
    std::vector<CelestialBody> bodies;

    // Manual hardcoded data
    CelestialBody sun;
    sun.name = "Sun";
    sun.mass = 1.989e30;
    // ... lots of manual setup

    bodies.push_back(sun);
    return bodies;
}
```

**After (Factory Pattern):**
```cpp
// New factory approach
#include "solar_core/bodies/body_factory.hpp"

Bodies::BodyFactory factory;
auto collection_result = factory.create_essential_bodies();

if (collection_result) {
    Bodies::BodyCollection bodies = std::move(*collection_result);
    // Ready to use with validation and fallback handling
} else {
    // Handle error appropriately
    std::cerr << "Failed to create bodies: " << collection_result.error() << std::endl;
}
```

**Migration Benefits:**
- Automatic data source management (JPL, cached, fallback)
- Built-in validation and error handling
- Intelligent fallback strategies
- Reduced boilerplate code

#### 3. Simulation Engine Migration

**Before (Basic Integration):**
```cpp
// Old basic simulation loop
void simulate(std::vector<CelestialBody>& bodies, double dt, int steps) {
    for (int i = 0; i < steps; ++i) {
        // Manual force calculation
        for (auto& body1 : bodies) {
            for (const auto& body2 : bodies) {
                if (&body1 != &body2) {
                    // Manual gravitational force calculation
                    // Manual position updates
                }
            }
        }
    }
}
```

**After (Enhanced Engine):**
```cpp
// New enhanced simulation
#include "solar_core/simulation/simulation_engine.hpp"

Simulation::SimulationConfig config{
    .time_step = 3600.0,
    .use_adaptive_timestep = true,
    .enable_collision_detection = true
};

Simulation::SimulationEngine engine(config);
engine.initialize(std::move(bodies));

// Progress monitoring
engine.set_progress_callback([](const Simulation::SimulationState& state) {
    std::cout << "Progress: " << state.current_time << "s" << std::endl;
});

engine.run_for_duration(365.25 * 24 * 3600);  // One year
```

**Migration Advantages:**
- Adaptive time stepping for accuracy and stability
- Built-in energy conservation monitoring
- Collision detection capabilities
- Progress callbacks and monitoring
- Configurable physics parameters

#### 4. Collection Management Migration

**Before (Basic Containers):**
```cpp
// Old approach with basic containers
std::vector<CelestialBody> bodies;
bodies.push_back(earth);
bodies.push_back(mars);

// Manual searching
auto it = std::find_if(bodies.begin(), bodies.end(),
    [](const CelestialBody& b) { return b.name == "Earth"; });
```

**After (Enhanced Collection):**
```cpp
// New collection approach
Bodies::BodyCollection collection;
collection.add_body(std::move(earth));
collection.add_body(std::move(mars));

// Efficient lookup
auto earth_opt = collection.find_body("Earth");
if (earth_opt) {
    const CelestialBody& earth = *earth_opt;
    // Use earth...
}

// Advanced filtering
auto planets = collection.filter_by_type(BodyType::Planet);
auto massive_bodies = collection.filter([](const CelestialBody& body) {
    return body.mass() > 1e24;
});
```

### Breaking Changes and Compatibility

#### API Changes
1. **Namespace Changes**: All enhanced APIs are in `SolarSystem::Bodies` and `SolarSystem::Simulation` namespaces
2. **Type Safety**: Strong typing replaces loose primitive types
3. **Error Handling**: `Expected<T, Error>` replaces exception-based error handling
4. **Memory Management**: RAII and move semantics replace manual memory management

#### Compatibility Layer
For gradual migration, a compatibility layer is available:

```cpp
#include "solar_core/compatibility/legacy_adapter.hpp"

// Adapter for old code
LegacyAdapter adapter;
auto legacy_bodies = adapter.convert_to_legacy(enhanced_collection);
// Use with old simulation code...

// Convert back when ready
auto enhanced_bodies = adapter.convert_from_legacy(legacy_bodies);
```

### Migration Checklist

- [ ] **Update includes**: Replace old headers with new enhanced API headers
- [ ] **Namespace adoption**: Add `using namespace SolarSystem::Bodies;` where appropriate
- [ ] **Error handling**: Replace try-catch with `Expected<T, Error>` pattern
- [ ] **Type safety**: Use enums instead of magic numbers/strings
- [ ] **Memory management**: Utilize move semantics and RAII
- [ ] **Configuration**: Replace hardcoded values with configuration structures
- [ ] **Testing**: Update unit tests to use new APIs
- [ ] **Performance**: Verify performance improvements with regression testing

### Common Migration Patterns

#### Pattern 1: Simple Body Creation
```cpp
// Old
CelestialBody body;
body.name = "Test";
body.mass = 1e20;

// New
CelestialBody body(CelestialBody::Properties{
    .name = "Test",
    .mass = 1e20,
    .position = {0, 0, 0},
    .velocity = {0, 0, 0},
    .type = BodyType::Asteroid,
    .priority = BodyPriority::Optional
});
```

#### Pattern 2: Error Handling
```cpp
// Old
try {
    auto body = create_body("Earth");
    // use body...
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

// New
auto body_result = factory.create_body("Earth");
if (body_result) {
    auto body = std::move(*body_result);
    // use body...
} else {
    std::cerr << "Error: " << body_result.error().message << std::endl;
}
```

#### Pattern 3: Collection Operations
```cpp
// Old
std::vector<CelestialBody> bodies;
for (const auto& body : bodies) {
    if (body.name == "Earth") {
        // found Earth...
        break;
    }
}

// New
Bodies::BodyCollection collection;
auto earth = collection.find_body("Earth");
if (earth) {
    // use *earth...
}
```
## Troubleshooting

### Common Issues and Solutions

#### 1. Body Factory Issues

**Problem**: `FactoryError::BODY_NOT_FOUND`
```
Error: Body 'Pluto' not found in any data source
```

**Solutions:**
- Check if the body name is spelled correctly (case-sensitive)
- Verify the body is available in your configured data sources
- For spacecraft, ensure the target date is after the launch date
- Use `factory.list_available_bodies()` to see all available bodies

```cpp
// Debug available bodies
Bodies::BodyFactory factory;
auto available = factory.list_available_bodies();
for (const auto& name : available) {
    std::cout << "Available: " << name << std::endl;
}
```

**Problem**: `FactoryError::VALIDATION_FAILED`
```
Error: Body data validation failed: Mass outside realistic bounds
```

**Solutions:**
- Check if custom validation rules are too strict
- Verify data source integrity
- Use more permissive validation tolerance
- Enable partial data if appropriate

```cpp
// Relaxed validation configuration
Bodies::BodyFactory::FactoryConfig config{
    .validation_tolerance = 0.2,  // 20% tolerance
    .allow_partial_data = true
};
Bodies::BodyFactory factory(config);
```

#### 2. Simulation Engine Issues

**Problem**: Simulation becomes unstable or produces unrealistic results

**Symptoms:**
- Bodies flying off to infinity
- Negative energies
- Extreme velocities

**Solutions:**
- Reduce time step size
- Enable adaptive time stepping
- Check initial conditions for validity
- Monitor energy conservation

```cpp
// Stable configuration for problematic systems
Simulation::SimulationConfig stable_config{
    .time_step = 60.0,              // Small time steps
    .use_adaptive_timestep = true,  // Let engine adjust
    .max_timestep = 3600.0,         // Conservative maximum
    .min_timestep = 1.0,            // Fine-grained minimum
    .tolerance = 1e-15              // High precision
};
```

**Problem**: Poor performance with large numbers of bodies

**Solutions:**
- Disable collision detection if not needed
- Use larger time steps for distant bodies
- Consider hierarchical simulation approaches
- Monitor memory usage

```cpp
// Performance-optimized configuration
Simulation::SimulationConfig perf_config{
    .time_step = 3600.0,                  // Larger steps
    .use_adaptive_timestep = false,       // Consistent performance
    .enable_collision_detection = false   // Disable if not needed
};
```

#### 3. Memory and Performance Issues

**Problem**: High memory usage or memory leaks

**Diagnostic Steps:**
1. Use memory profiling tools (valgrind, AddressSanitizer)
2. Check for proper RAII usage
3. Monitor collection sizes
4. Verify move semantics are being used

```cpp
// Memory usage monitoring
#include "tests/utils/test_diagnostics.hpp"

TestDiagnostics::set_memory_baseline("simulation_test");
// ... run simulation ...
bool has_leak = TestDiagnostics::detect_memory_regression("simulation_test");
if (has_leak) {
    auto report = TestDiagnostics::get_memory_regression_report("simulation_test");
    std::cout << "Memory issue: " << report << std::endl;
}
```

**Problem**: Performance regression detected

**Solutions:**
- Check recent code changes
- Run performance profiling
- Compare with baseline metrics
- Review algorithm complexity

```cpp
// Performance regression analysis
#include "tests/utils/performance_regression_system.h"

auto alerts = run_performance_regression_tests();
for (const auto& alert : alerts) {
    std::cout << "Regression in " << alert.component
              << ": " << alert.message << std::endl;

    // Get optimization recommendations
    auto recommendations = get_optimization_recommendations(alert.component);
    for (const auto& rec : recommendations) {
        std::cout << "  Recommendation: " << rec.description << std::endl;
    }
}
```

#### 4. Data Source Issues

**Problem**: JPL HORIZONS API unavailable

**Symptoms:**
- Network timeouts
- API rate limiting
- Service unavailable errors

**Solutions:**
- Configure intelligent fallback strategy
- Use cached data when available
- Implement retry logic with exponential backoff

```cpp
// Robust data source configuration
Bodies::BodyFactory::FactoryConfig robust_config{
    .preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA,
    .fallback_strategy = Bodies::BodyFactory::FallbackStrategy::GRACEFUL,
    .enable_validation = true,
    .allow_partial_data = true
};
```

**Problem**: Cached data is outdated

**Solutions:**
- Force refresh of cached data
- Check cache expiration settings
- Verify cache file integrity

```cpp
// Force cache refresh
Bodies::BodyFactory factory;
factory.clear_cache();
factory.refresh_cache_from_jpl();
```

#### 5. Compilation Issues

**Problem**: Missing headers or undefined symbols

**Common Causes:**
- Incorrect include paths
- Missing library dependencies
- C++20 feature not supported

**Solutions:**
```bash
# Verify C++20 support
g++ --version  # Should be 10+ for full C++20 support
clang++ --version  # Should be 10+ for full C++20 support

# Check CMake configuration
cmake -DCMAKE_CXX_STANDARD=20 ..

# Verify library linking
cmake --build . --verbose
```

**Problem**: Template instantiation errors

**Common with C++20 concepts and ranges:**
```cpp
// Ensure proper concept constraints
template<typename T>
requires std::is_same_v<T, Bodies::CelestialBody>
void process_body(const T& body) {
    // Implementation...
}
```

### Debugging Tools and Techniques

#### 1. Enable Debug Logging

```cpp
// Enable comprehensive logging
#define SOLAR_SYSTEM_DEBUG_LOGGING
#include "solar_core/utils/debug_logger.hpp"

DebugLogger::set_level(LogLevel::DEBUG);
DebugLogger::enable_component("BodyFactory");
DebugLogger::enable_component("SimulationEngine");
```

#### 2. Validation and Consistency Checking

```cpp
// Comprehensive validation
Bodies::BodyCollection collection = /* ... */;

// Check collection consistency
auto consistency = collection.check_consistency();
if (!consistency.is_valid) {
    for (const auto& issue : consistency.issues) {
        std::cout << "Issue: " << issue.description << std::endl;
        std::cout << "Severity: " << issue.severity << std::endl;
        std::cout << "Suggestion: " << issue.suggestion << std::endl;
    }
}

// Validate individual bodies
for (const auto& body : collection) {
    auto validation = Bodies::validate_body_properties(body);
    if (!validation.is_valid) {
        std::cout << "Body " << body.name() << " validation failed:" << std::endl;
        for (const auto& error : validation.errors) {
            std::cout << "  " << error << std::endl;
        }
    }
}
```

#### 3. Performance Profiling

```cpp
// Built-in performance profiling
#include "tests/utils/performance_profiler.hpp"

PerformanceProfiler profiler;
profiler.start_profiling("simulation_run");

// ... run simulation ...

auto profile = profiler.stop_profiling("simulation_run");
std::cout << "Execution time: " << profile.execution_time_ms << "ms" << std::endl;
std::cout << "Memory usage: " << profile.peak_memory_mb << "MB" << std::endl;
std::cout << "CPU usage: " << profile.cpu_usage_percent << "%" << std::endl;
```

### Getting Help

#### 1. Documentation Resources
- **API Reference**: `docs/api/ENHANCED_APIS.md` (this document)
- **Architecture Guide**: `docs/architecture/`
- **Examples**: `docs/examples/`
- **Developer Guide**: `docs/developer/`

#### 2. Diagnostic Commands
```bash
# Run comprehensive test suite
ctest --output-on-failure

# Run performance regression tests
python3 tests/scripts/run_performance_regression_tests.py

# Generate performance baseline
python3 tests/scripts/generate_baseline.py

# Verify system integrity
python3 tests/scripts/verify_benchmark_system.py
```

#### 3. Common Debug Patterns

```cpp
// Pattern 1: Safe body access
auto body_opt = collection.find_body("Earth");
if (!body_opt) {
    std::cerr << "Earth not found in collection" << std::endl;
    // List available bodies for debugging
    for (const auto& body : collection) {
        std::cerr << "Available: " << body.name() << std::endl;
    }
    return;
}

// Pattern 2: Error context preservation
auto result = factory.create_body("Mars");
if (!result) {
    const auto& error = result.error();
    std::cerr << "Failed to create Mars:" << std::endl;
    std::cerr << "  Type: " << static_cast<int>(error.type) << std::endl;
    std::cerr << "  Message: " << error.message << std::endl;
    std::cerr << "  Context: " << error.context << std::endl;
}

// Pattern 3: Simulation state monitoring
engine.set_progress_callback([](const Simulation::SimulationState& state) {
    // Monitor for anomalies
    if (std::isnan(state.total_energy) || std::isinf(state.total_energy)) {
        std::cerr << "Energy calculation error at iteration "
                  << state.iteration_count << std::endl;
    }

    if (state.center_of_mass.magnitude() > 1e12) {
        std::cerr << "Center of mass drift detected: "
                  << state.center_of_mass.magnitude() << std::endl;
    }
});
```
