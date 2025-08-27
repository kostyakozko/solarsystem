# Solar System Library - Troubleshooting Guide

This guide helps you diagnose and resolve common issues when using the enhanced Solar System Library APIs.

## Table of Contents

1. [Quick Diagnostics](#quick-diagnostics)
2. [Common Error Messages](#common-error-messages)
3. [Performance Issues](#performance-issues)
4. [Data Source Problems](#data-source-problems)
5. [Compilation Issues](#compilation-issues)
6. [Runtime Errors](#runtime-errors)
7. [Debugging Tools](#debugging-tools)

## Quick Diagnostics

### System Health Check

Run this comprehensive health check to identify common issues:

```bash
# 1. Verify build system
cmake --version  # Should be 3.15+
g++ --version    # Should support C++20

# 2. Run test suite
cd build
ctest --output-on-failure

# 3. Check performance regression system
python3 tests/scripts/run_performance_regression_tests.py

# 4. Verify benchmark system
python3 tests/scripts/verify_benchmark_system.py
```

### Quick Code Test

```cpp
#include "solar_core/bodies/body_factory.hpp"
#include <iostream>

int main() {
    using namespace SolarSystem::Bodies;

    // Quick factory test
    Bo factory;
    auto earth_result = factory.create_body("Earth");

    if (earth_result) {
        std::cout << "✅ System working: Created " << earth_result->name() << std::endl;
        return 0;
    } else {
        std::cout << "❌ System issue: " << earth_result.error().message << std::endl;
        return 1;
    }
}
```

## Common Error Messages

### 1. Body Factory Errors

#### Error: `FactoryError::BODY_NOT_FOUND`

**Message:** `Body 'Mars' not found in any data source`

**Causes:**
- Typo in body name (case-sensitive)
- Body not available in configured data sources
- Spacecraft requested before launch date

**Solutions:**
```cpp
// Check available bodies
Bodies::BodyFactory factory;
auto available = factory.list_available_bodies();
for (const auto& name : available) {
    std::cout << "Available: " << name << std::endl;
}

// Use case-insensitive search
auto mars_result = factory.find_body_case_insensitive("mars");

// For spacecraft, check date availability
auto voyager_result = factory.create_body("Voyager 1",
    std::chrono::system_clock::from_time_t(/* launch date */));
```

#### Error: `FactoryError::VALIDATION_FAILED`

**Message:** `Body data validation failed: Mass outside realistic bounds`

**Causes:**
- Corrupted data source
- Overly strict validation rules
- Unit conversion errors

**Solutions:**
```cpp
// Relax validation tolerance
Bodies::BodyFactory::FactoryConfig config{
    .validation_tolerance = 0.2,  // 20% tolerance instead of default 10%
    .allow_partial_data = true
};
Bodies::BodyFactory factory(config);

// Or disable validation for testing
config.enable_validation = false;
```

#### Error: `FactoryError::DATA_SOURCE_UNAVAILABLE`

**Message:** `All data sources unavailable`

**Causes:**
- Network connectivity issues
- JPL HORIZONS API down
- Corrupted cache files

**Solutions:**
```cpp
// Configure robust fallback strategy
Bodies::BodyFactory::FactoryConfig config{
    .preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA,
    .fallback_strategy = Bodies::BodyFactory::FallbackStrategy::GRACEFUL
};

// Clear and rebuild cache
factory.clear_cache();
factory.rebuild_cache();
```

### 2. Simulation Engine Errors

#### Error: Simulation becomes unstable

**Symptoms:**
- Bodies flying off to infinity
- NaN or infinite values in positions/velocities
- Energy not conserved

**Diagnostic Code:**
```cpp
engine.set_progress_callback([](const Simulation::SimulationState& state) {
    // Check for NaN values
    if (std::isnan(state.total_energy) || std::isinf(state.total_energy)) {
        std::cerr << "❌ Energy calculation error at iteration " << state.iteration_count << std::endl;
    }

    // Check energy conservation
    static long double initial_energy = state.total_energy;
    if (state.iteration_count == 1) initial_energy = state.total_energy;

    long double drift = std::abs(state.total_energy - initial_energy) / initial_energy;
    if (drift > 1e-3) {  // 0.1% tolerance
        std::cerr << "⚠️  Energy drift: " << (drift * 100) << "%" << std::endl;
    }
});
```

**Solutions:**
```cpp
// Use smaller, adaptive time steps
Simulation::SimulationConfig stable_config{
    .time_step = 60.0,              // Start with 1 minute
    .use_adaptive_timestep = true,  // Let engine adjust
    .max_timestep = 3600.0,         // Max 1 hour
    .min_timestep = 1.0,            // Min 1 second
    .tolerance = 1e-15              // High precision
};

// Check initial conditions
for (const auto& body : bodies) {
    if (body.position().magnitude() > 1e15) {  // > 1000 AU
        std::cerr << "Warning: " << body.name() << " very far from origin" << std::endl;
    }
    if (body.velocity().magnitude() > 1e6) {   // > 1000 km/s
        std::cerr << "Warning: " << body.name() << " has extreme velocity" << std::endl;
    }
}
```

### 3. Collection Errors

#### Error: `std::bad_optional_access`

**Message:** `Attempted to access empty optional`

**Cause:** Trying to access a body that doesn't exist in collection

**Solution:**
```cpp
// Always check optional before access
auto earth_opt = collection.find_body("Earth");
if (earth_opt) {
    const CelestialBody& earth = *earth_opt;
    // Safe to use earth
} else {
    std::cerr << "Earth not found in collection" << std::endl;
    // Handle missing body case
}

// Or use exception-safe access
try {
    const CelestialBody& earth = collection.at("Earth");
    // Use earth
} catch (const std::out_of_range& e) {
    std::cerr << "Body not found: " << e.what() << std::endl;
}
```

## Performance Issues

### 1. Slow Simulation Performance

**Symptoms:**
- Simulation takes much longer than expected
- High CPU usage
- Memory usage growing over time

**Diagnostic Steps:**
```cpp
// Enable performance profiling
#include "tests/utils/performance_profiler.hpp"

PerformanceProfiler profiler;
profiler.start_profiling("simulation");

// Run simulation
engine.run_for_duration(duration);

auto profile = profiler.stop_profiling("simulation");
std::cout << "Execution time: " << profile.execution_time_ms << "ms" << std::endl;
std::cout << "Memory usage: " << profile.peak_memory_mb << "MB" << std::endl;
```

**Solutions:**
```cpp
// Optimize configuration for performance
Simulation::SimulationConfig perf_config{
    .time_step = 3600.0,                  // Larger time steps
    .use_adaptive_timestep = false,       // Consistent performance
    .enable_collision_detection = false   // Disable if not needed
};

// Reduce body count for testing
auto essential_only = collection.filter_by_priority(BodyPriority::Essential);

// Use release build
// cmake -DCMAKE_BUILD_TYPE=Release ..
```

### 2. Memory Leaks

**Detection:**
```bash
# Use AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
make
./your_program

# Or Valgrind
valgrind --leak-check=full ./your_program
```

**Common Causes and Solutions:**
```cpp
// Ensure proper RAII usage
{
    Bodies::BodyCollection collection;  // RAII - automatic cleanup
    // ... use collection
}  // Automatically destroyed here

// Use smart pointers for dynamic allocation
std::unique_ptr<Simulation::SimulationEngine> engine =
    std::make_unique<Simulation::SimulationEngine>(config);

// Avoid raw pointers and manual memory management
// DON'T: CelestialBody* body = new CelestialBody(...);
// DO: CelestialBody body(...);
```

## Data Source Problems

### 1. JPL HORIZONS API Issues

**Error:** `Network timeout connecting to JPL HORIZONS`

**Solutions:**
```cpp
// Configure timeout and retry settings
Bodies::BodyFactory::FactoryConfig config{
    .network_timeout_seconds = 30,
    .max_retry_attempts = 3,
    .retry_delay_seconds = 5
};

// Use cached data as primary source
config.preferred_source = Bodies::BodyFactory::DataSource::CACHED_DATA;
config.fallback_strategy = Bodies::BodyFactory::FallbackStrategy::GRACEFUL;
```

### 2. Cache Corruption

**Symptoms:**
- Inconsistent body data
- Validation failures
- Crashes when loading cached data

**Solutions:**
```cpp
// Clear and rebuild cache
Bodies::BodyFactory factory;
factory.clear_cache();
factory.validate_cache();

// Force fresh data fetch
factory.force_refresh_from_jpl();

// Check cache integrity
if (!factory.verify_cache_integrity()) {
    std::cout << "Cache corrupted, rebuilding..." << std::endl;
    factory.rebuild_cache();
}
```

## Compilation Issues

### 1. C++20 Feature Errors

**Error:** `error: 'concept' does not name a type`

**Solution:**
```bash
# Ensure C++20 support
cmake -DCMAKE_CXX_STANDARD=20 ..

# Check compiler version
g++ --version  # Need GCC 10+ or Clang 10+

# Enable C++20 explicitly
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### 2. Missing Headers

**Error:** `fatal error: 'solar_core/bodies/celestial_body.hpp' file not found`

**Solutions:**
```cmake
# In CMakeLists.txt, ensure proper include directories
target_include_directories(your_target PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/solar_core/include
)

# Check build configuration
cmake --build . --verbose
```

### 3. Linking Errors

**Error:** `undefined reference to 'SolarSystem::Bodies::CelestialBody::CelestialBody'`

**Solutions:**
```cmake
# Ensure proper library linking
target_link_libraries(your_target
    solar_core
    solar_jpl
    solar_utils
)

# Check if libraries were built
ls build/lib/
```

## Runtime Errors

### 1. Segmentation Faults

**Common Causes:**
- Accessing destroyed objects
- Invalid iterators
- Null pointer dereference

**Debugging:**
```bash
# Use GDB for debugging
gdb ./your_program
(gdb) run
# When it crashes:
(gdb) bt  # Print backtrace
(gdb) info registers
(gdb) print variable_name
```

**Prevention:**
```cpp
// Use RAII and smart pointers
std::unique_ptr<CelestialBody> body = std::make_unique<CelestialBody>(props);

// Check pointers before use
if (body != nullptr) {
    // Safe to use body
}

// Use references instead of pointers when possible
void process_body(const CelestialBody& body) {  // Reference - can't be null
    // Implementation
}
```

### 2. Assertion Failures

**Error:** `Assertion failed: expected 5 but got 3`

**Debugging:**
```cpp
// Add debug output before assertions
std::cout << "Debug: collection size = " << collection.size() << std::endl;
assert(collection.size() == expected_size);

// Use conditional assertions for debugging
#ifdef DEBUG
    assert(condition);
#endif

// Replace assertions with proper error handling in production
if (collection.size() != expected_size) {
    throw std::runtime_error("Unexpected collection size: " +
                            std::to_string(collection.size()));
}
```

## Debugging Tools

### 1. Built-in Diagnostics

```cpp
// Enable debug logging
#define SOLAR_SYSTEM_DEBUG_LOGGING
#include "solar_core/utils/debug_logger.hpp"

DebugLogger::set_level(LogLevel::DEBUG);
DebugLogger::enable_component("BodyFactory");
DebugLogger::enable_component("SimulationEngine");
```

### 2. Performance Profiling

```cpp
// Use built-in performance profiler
#include "tests/utils/performance_profiler.hpp"

PerformanceProfiler profiler;
profiler.start_profiling("critical_section");
// ... code to profile ...
auto metrics = profiler.stop_profiling("critical_section");

std::cout << "Time: " << metrics.execution_time_ms << "ms" << std::endl;
std::cout << "Memory: " << metrics.peak_memory_mb << "MB" << std::endl;
```

### 3. Validation Tools

```cpp
// Comprehensive validation
Bodies::BodyCollection collection = /* ... */;

// Check collection consistency
auto consistency = collection.check_consistency();
if (!consistency.is_valid) {
    for (const auto& issue : consistency.issues) {
        std::cout << "Issue: " << issue.description << std::endl;
        std::cout << "Severity: " << issue.severity << std::endl;
    }
}

// Validate individual bodies
for (const auto& body : collection) {
    auto validation = Bodies::validate_body_properties(body);
    if (!validation.is_valid) {
        std::cout << "Body " << body.name() << " validation failed" << std::endl;
    }
}
```

### 4. System Information

```bash
# Check system capabilities
python3 tests/scripts/system_info.py

# Verify dependencies
ldd build/lib/libsolar_core.so  # Linux
otool -L build/lib/libsolar_core.dylib  # macOS

# Check environment
echo $CMAKE_PREFIX_PATH
echo $LD_LIBRARY_PATH
```

## Getting Additional Help

### 1. Enable Verbose Logging

```cpp
// Maximum verbosity for debugging
DebugLogger::set_level(LogLevel::TRACE);
DebugLogger::enable_all_components();
```

### 2. Generate Diagnostic Report

```bash
# Run comprehensive diagnostics
python3 tests/scripts/generate_diagnostic_report.py

# This creates a detailed report including:
# - System information
# - Build configuration
# - Test results
# - Performance metrics
# - Error logs
```

### 3. Community Resources

- **Documentation**: `docs/` directory
- **Examples**: `docs/examples/`
- **Test Cases**: `tests/` directory for usage patterns
- **Performance Data**: `performance_reports/` directory

Remember: When reporting issues, always include:
1. Your system information (OS, compiler version)
2. Build configuration (Debug/Release, CMake version)
3. Minimal reproducible example
4. Complete error messages and stack traces
5. Diagnostic report output
