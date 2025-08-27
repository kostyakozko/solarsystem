# Migration Guide: From Placeholder to Enhanced APIs

This guide provides comprehensive instructions for migrating from the original placeholder implementations to the enhanced Solar System Library APIs.

## Table of Contents

1. [Migration Overview](#migration-overview)
2. [Pre-Migration Checklist](#pre-migration-checklist)
3. [Step-by-Step Migration](#step-by-step-migration)
4. [API Mapping Reference](#api-mapping-reference)
5. [Common Migration Patterns](#common-migration-patterns)
6. [Testing Your Migration](#testing-your-migration)
7. [Performance Optimization](#performance-optimization)

## Migration Overview

### What's Changed

The enhanced APIs introduce several major improvements:

- **Type Safety**: Strong typing with enums and structured data
- **Error Handling**: Modern `Expected<T, Error>` pattern instead of exceptions
- **Memory Management**: RAII and move semantics throughout
- **Performance**: Optimized algorithms and data structures
- **Validation**: Comprehensive data validation and consistency checking
- **Modularity**: Clean separation of concerns with factory patterns

### Migration Benefits

- **Reliability**: Robust error handling and validation
- **Performance**: Significant performance improvements (measured via regression testing)
- **Maintainability**: Cleaner, more modular code structure
- **Extensibility**: Easy to add new features and body types
- **Safety**: Memory safety and type safety improvements

### Compatibility

- **Breaking Changes**: Yes, this is a major API revision
- **Compatibility Layer**: Available for gradual migration
- **Migration Tools**: Automated migration scripts provided

## Pre-Migration Checklist

### 1. Backup Your Code

```bash
# Create a backup branch
git checkout -b pre-migration-backup
git commit -am "Backup before migration to enhanced APIs"
git checkout main  # or your working branch
```

### 2. Verify System Requirements

```bash
# Check C++20 support
g++ --version  # Need GCC 10+ or Clang 10+
cmake --version  # Need CMake 3.15+

# Verify build system
cd build
cmake -DCMAKE_CXX_STANDARD=20 ..
make -j$(nproc)
```

### 3. Run Current Tests

```bash
# Ensure your current code works
ctest --output-on-failure

# Document current performance
python3 tests/scripts/generate_baseline.py --output-dir pre-migration-baseline
```

### 4. Analyze Dependencies

```bash
# Find all files that need migration
grep -r "CelestialBody" src/ --include="*.cpp" --include="*.h"
grep -r "create_body" src/ --include="*.cpp" --include="*.h"
grep -r "simulate" src/ --include="*.cpp" --include="*.h"
```

## Step-by-Step Migration

### Step 1: Update Headers and Namespaces

**Before:**
```cpp
#include "celestial_body.h"
#include "simulation.h"

// Global namespace usage
CelestialBody earth;
```

**After:**
```cpp
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

using namespace SolarSystem::Bodies;
using namespace SolarSystem::Simulation;

// Structured initialization
CelestialBody::Properties earth_props{/* ... */};
CelestialBody earth(earth_props);
```

**Migration Script:**
```bash
# Automated header replacement
python3 tools/migrate_headers.py --input-dir src/ --output-dir src_migrated/
```

### Step 2: Migrate Body Creation

**Before (Manual Creation):**
```cpp
CelestialBody create_earth() {
    CelestialBody earth;
    earth.name = "Earth";
    earth.mass = 5.972e24;
    earth.x = 1.496e11;
    earth.y = 0;
    earth.z = 0;
    earth.vx = 0;
    earth.vy = 29780;
    earth.vz = 0;
    return earth;
}

std::vector<CelestialBody> create_solar_system() {
    std::vector<CelestialBody> bodies;
    bodies.push_back(create_earth());
    bodies.push_back(create_mars());
    // ... more manual creation
    return bodies;
}
```

**After (Factory Pattern):**
```cpp
Bodies::BodyCollection create_solar_system() {
    Bodies::BodyFactory factory;

    // Configure factory for your needs
    Bodies::BodyFactory::FactoryConfig config{
        .preferred_source = Bodies::BodyFactory::DataSource::JPL_HORIZONS,
        .fallback_strategy = Bodies::BodyFactory::FallbackStrategy::INTELLIGENT,
        .enable_validation = true
    };
    factory.set_config(config);

    // Create essential bodies with automatic fallback
    auto collection_result = factory.create_essential_bodies();
    if (!collection_result) {
        throw std::runtime_error("Failed to create solar system: " +
                                collection_result.error().message);
    }

    return std::move(*collection_result);
}

// For custom bodies, use Properties structure
CelestialBody create_custom_body() {
    CelestialBody::Properties props{
        .name = "CustomBody",
        .mass = 5.972e24,
        .position = {1.496e11, 0, 0},
        .velocity = {0, 29780, 0},
        .type = BodyType::Planet,
        .priority = BodyPriority::Essential
    };
    return CelestialBody(props);
}
```

### Step 3: Migrate Error Handling

**Before (Exception-Based):**
```cpp
try {
    CelestialBody earth = create_body("Earth");
    // Use earth...
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

**After (Expected Pattern):**
```cpp
Bodies::BodyFactory factory;
auto earth_result = factory.create_body("Earth");

if (earth_result) {
    CelestialBody earth = std::move(*earth_result);
    // Use earth...
} else {
    const auto& error = earth_result.error();
    std::cerr << "Error creating Earth: " << error.message << std::endl;

    // Handle specific error types
    switch (error.type) {
        case Bodies::FactoryError::BODY_NOT_FOUND:
            // Try alternative approach
            break;
        case Bodies::FactoryError::VALIDATION_FAILED:
            // Log validation details
            break;
        case Bodies::FactoryError::DATA_SOURCE_UNAVAILABLE:
            // Switch to offline mode
            break;
    }
}
```

### Step 4: Migrate Simulation Logic

**Before (Manual Loop):**
```cpp
void simulate(std::vector<CelestialBody>& bodies, double dt, int steps) {
    for (int step = 0; step < steps; ++step) {
        // Manual force calculation
        for (auto& body1 : bodies) {
            Vector3 total_force = {0, 0, 0};

            for (const auto& body2 : bodies) {
                if (&body1 != &body2) {
                    Vector3 r = {body2.x - body1.x, body2.y - body1.y, body2.z - body1.z};
                    double distance = sqrt(r.x*r.x + r.y*r.y + r.z*r.z);
                    double force_magnitude = G * body1.mass * body2.mass / (distance * distance);

                    Vector3 force_direction = {r.x/distance, r.y/distance, r.z/distance};
                    total_force.x += force_magnitude * force_direction.x;
                    total_force.y += force_magnitude * force_direction.y;
                    total_force.z += force_magnitude * force_direction.z;
                }
            }

            // Manual integration
            body1.vx += (total_force.x / body1.mass) * dt;
            body1.vy += (total_force.y / body1.mass) * dt;
            body1.vz += (total_force.z / body1.mass) * dt;

            body1.x += body1.vx * dt;
            body1.y += body1.vy * dt;
            body1.z += body1.vz * dt;
        }
    }
}
```

**After (Enhanced Engine):**
```cpp
void simulate(Bodies::BodyCollection bodies, double duration) {
    // Configure simulation for accuracy and stability
    Simulation::SimulationConfig config{
        .time_step = 3600.0,              // 1 hour steps
        .use_adaptive_timestep = true,    // Automatic step adjustment
        .max_timestep = 86400.0,          // Max 1 day
        .min_timestep = 60.0,             // Min 1 minute
        .tolerance = 1e-12,               // High precision
        .enable_collision_detection = false
    };

    Simulation::SimulationEngine engine(config);

    // Set up monitoring
    engine.set_progress_callback([](const Simulation::SimulationState& state) {
        if (state.iteration_count % 24 == 0) {  // Every 24 hours
            std::cout << "Day " << (state.current_time / 86400.0)
                      << ": Energy = " << state.total_energy << " J" << std::endl;
        }
    });

    // Initialize and run
    engine.initialize(std::move(bodies));
    engine.run_for_duration(duration);
}
```

### Step 5: Migrate Collection Management

**Before (Basic Containers):**
```cpp
std::vector<CelestialBody> bodies;
bodies.push_back(earth);
bodies.push_back(mars);

// Manual searching
auto it = std::find_if(bodies.begin(), bodies.end(),
    [](const CelestialBody& b) { return b.name == "Earth"; });

if (it != bodies.end()) {
    // Found Earth
    CelestialBody& earth = *it;
}
```

**After (Enhanced Collection):**
```cpp
Bodies::BodyCollection collection;
collection.add_body(std::move(earth));
collection.add_body(std::move(mars));

// Efficient O(1) lookup
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

// Statistics and analysis
auto stats = collection.get_statistics();
std::cout << "Total mass: " << stats.total_mass << " kg" << std::endl;
std::cout << "Center of mass: " << stats.center_of_mass.to_string() << std::endl;
```

## API Mapping Reference

### Core Types

| Old API | New API | Notes |
|---------|---------|-------|
| `CelestialBody` struct | `Bodies::CelestialBody` class | RAII, type-safe |
| `double x, y, z` | `Math::Vector3d position` | Vector operations |
| `double vx, vy, vz` | `Math::Vector3d velocity` | Vector operations |
| `std::string name` | `std::string_view name()` | Immutable accessor |
| `double mass` | `long double mass()` | Higher precision |

### Factory Functions

| Old API | New API | Notes |
|---------|---------|-------|
| `create_body(name)` | `factory.create_body(name)` | Returns `Expected<T, Error>` |
| `create_solar_system()` | `factory.create_essential_bodies()` | Intelligent data sourcing |
| Manual hardcoding | Factory with fallback | JPL → Cache → Hardcoded |

### Simulation Functions

| Old API | New API | Notes |
|---------|---------|-------|
| `simulate(bodies, dt, steps)` | `engine.run_for_duration(duration)` | Adaptive time stepping |
| Manual force calculation | Built-in physics engine | Optimized algorithms |
| No monitoring | Progress callbacks | Real-time monitoring |

### Error Handling

| Old API | New API | Notes |
|---------|---------|-------|
| `throw std::exception` | `Expected<T, Error>` | No exceptions |
| Generic error messages | Structured error types | Specific error handling |
| Stack unwinding | Explicit error checking | Performance friendly |

## Common Migration Patterns

### Pattern 1: Simple Body Access

```cpp
// Before
CelestialBody earth = bodies[0];  // Unsafe indexing
std::string name = earth.name;    // Direct member access

// After
auto earth_opt = collection.find_body("Earth");  // Safe lookup
if (earth_opt) {
    std::string_view name = earth_opt->name();    // Accessor method
}
```

### Pattern 2: Error Propagation

```cpp
// Before
CelestialBody create_and_validate_body(const std::string& name) {
    CelestialBody body = create_body(name);  // May throw
    validate_body(body);                     // May throw
    return body;
}

// After
Expected<CelestialBody, FactoryError> create_and_validate_body(const std::string& name) {
    auto body_result = factory.create_body(name);
    if (!body_result) {
        return body_result;  // Propagate error
    }

    CelestialBody body = std::move(*body_result);
    auto validation_result = validate_body(body);
    if (!validation_result) {
        return make_error(FactoryError::VALIDATION_FAILED, validation_result.error());
    }

    return body;
}
```

### Pattern 3: Collection Iteration

```cpp
// Before
for (auto& body : bodies) {
    if (body.name == "Earth") {
        // Process Earth
    }
}

// After
// Option 1: Direct lookup
auto earth = collection.find_body("Earth");
if (earth) {
    // Process Earth
}

// Option 2: Filtered iteration
auto planets = collection.filter_by_type(BodyType::Planet);
for (const auto& planet : planets) {
    // Process each planet
}

// Option 3: C++20 ranges
auto inner_planets = collection
    | std::views::filter([](const auto& body) {
        return body.type() == BodyType::Planet &&
               body.position().magnitude() < 5e11;
      });
```

### Pattern 4: Configuration Management

```cpp
// Before
const double G = 6.67430e-11;
const double dt = 3600.0;
const int max_steps = 8760;  // Hours in a year

void simulate_with_config(std::vector<CelestialBody>& bodies) {
    simulate(bodies, dt, max_steps);
}

// After
Simulation::SimulationConfig create_yearly_config() {
    return Simulation::SimulationConfig{
        .time_step = 3600.0,
        .gravitational_constant = 6.67430e-11,
        .use_adaptive_timestep = true,
        .max_timestep = 86400.0,
        .min_timestep = 60.0,
        .tolerance = 1e-12
    };
}

void simulate_with_config(Bodies::BodyCollection bodies) {
    auto config = create_yearly_config();
    Simulation::SimulationEngine engine(config);
    engine.initialize(std::move(bodies));
    engine.run_for_duration(365.25 * 24 * 3600);  // One year
}
```

## Testing Your Migration

### 1. Compilation Test

```bash
# Ensure clean compilation
mkdir build-migration
cd build-migration
cmake -DCMAKE_CXX_STANDARD=20 ..
make -j$(nproc)
```

### 2. Functionality Test

```cpp
// Create a comprehensive test
#include "solar_core/bodies/body_factory.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

int main() {
    try {
        // Test body creation
        Bodies::BodyFactory factory;
        auto earth = factory.create_body("Earth");
        if (!earth) {
            std::cerr << "❌ Body creation failed" << std::endl;
            return 1;
        }

        // Test collection
        Bodies::BodyCollection collection;
        collection.add_body(std::move(*earth));

        if (collection.size() != 1) {
            std::cerr << "❌ Collection management failed" << std::endl;
            return 1;
        }

        // Test simulation
        Simulation::SimulationEngine engine;
        engine.initialize(std::move(collection));
        engine.run_for_duration(3600.0);  // 1 hour

        std::cout << "✅ Migration test passed" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ Migration test failed: " << e.what() << std::endl;
        return 1;
    }
}
```

### 3. Performance Comparison

```bash
# Compare performance before and after migration
python3 tests/scripts/compare_performance.py \
    pre-migration-baseline/combined_baseline.csv \
    build/tests/benchmarks/benchmark_results/comprehensive_benchmark.csv

# Run regression tests
python3 tests/scripts/run_performance_regression_tests.py
```

### 4. Integration Test

```bash
# Run full test suite
ctest --output-on-failure

# Verify specific functionality
ctest -R "Migration" --verbose
```

## Performance Optimization

### Post-Migration Optimizations

1. **Enable Compiler Optimizations:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" ..
```

2. **Use Move Semantics:**
```cpp
// Prefer move over copy
Bodies::BodyCollection collection = std::move(factory_result);
engine.initialize(std::move(collection));
```

3. **Configure for Your Use Case:**
```cpp
// For real-time applications
Simulation::SimulationConfig realtime_config{
    .time_step = 1.0,                     // Small steps
    .use_adaptive_timestep = false,       // Consistent timing
    .enable_collision_detection = false   // Reduce overhead
};

// For accuracy-critical applications
Simulation::SimulationConfig precision_config{
    .time_step = 60.0,
    .use_adaptive_timestep = true,        // Let engine optimize
    .tolerance = 1e-15,                   // High precision
    .enable_collision_detection = true    // Full physics
};
```

### Performance Monitoring

```cpp
// Monitor performance during migration
#include "tests/utils/performance_regression_system.h"

// Set up baseline
initialize_performance_regression_testing("migration_baselines.txt");

// Register key operations
register_performance_test("Migration", "body_creation", []() {
    Bodies::BodyFactory factory;
    auto start = std::chrono::high_resolution_clock::now();

    auto earth = factory.create_body("Earth");

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    return create_performance_metrics(duration.count(), get_memory_usage());
});

// Check for regressions
auto alerts = run_performance_regression_tests();
for (const auto& alert : alerts) {
    std::cout << "Performance alert: " << alert.message << std::endl;
}
```

## Migration Checklist

- [ ] **Backup created** - Code safely backed up
- [ ] **Headers updated** - All includes migrated to new headers
- [ ] **Namespaces added** - Using appropriate namespaces
- [ ] **Body creation migrated** - Using factory pattern
- [ ] **Error handling updated** - Using Expected<T, Error> pattern
- [ ] **Simulation logic migrated** - Using SimulationEngine
- [ ] **Collection management updated** - Using BodyCollection
- [ ] **Tests updated** - All tests pass with new APIs
- [ ] **Performance verified** - No significant regressions
- [ ] **Documentation updated** - Comments and docs reflect new APIs

## Rollback Plan

If migration issues arise:

```bash
# Quick rollback to backup
git checkout pre-migration-backup

# Or selective rollback
git checkout pre-migration-backup -- src/problematic_file.cpp

# Gradual migration approach
git checkout main
git merge --no-commit pre-migration-backup
# Resolve conflicts manually, migrating one file at a time
```

Remember: Migration can be done gradually. The compatibility layer allows you to migrate one component at a time while keeping the rest of your code functional.
