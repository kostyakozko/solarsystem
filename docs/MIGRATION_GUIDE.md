# Migration Guide

This guide helps you migrate to the modernized build system with shared library support
and external dependencies.

## Overview of Changes

### Build System Modernization (v4.0)

The Solar System Suite build system has been modernized with:

1. **Shared Library Support**: Optional shared library builds
2. **External Dependencies**: Google Test, nlohmann/json via FetchContent
3. **Symbol Visibility**: Proper export macros for shared libraries
4. **Package Configuration**: pkg-config and CMake config files

## Migrating from Static to Shared Libraries

### Why Migrate?

| Aspect | Static | Shared |
|--------|--------|--------|
| Binary size | Larger | 65-89% smaller |
| Incremental builds | Slower | Faster |
| Deployment | Simpler | Requires library paths |
| Runtime performance | Baseline | ~1-2% overhead |

### How to Migrate

Simply add `-DBUILD_SHARED_LIBS=ON` to your CMake configuration:

```bash
# Before (static, still works)
cmake -B build

# After (shared)
cmake -B build -DBUILD_SHARED_LIBS=ON
```

### Deployment Considerations

With shared libraries, ensure the library path is configured:

```bash
# Option 1: Install to system path
sudo cmake --install build --prefix /usr/local

# Option 2: Set library path
export LD_LIBRARY_PATH=/path/to/install/lib:$LD_LIBRARY_PATH  # Linux
export DYLD_LIBRARY_PATH=/path/to/install/lib:$DYLD_LIBRARY_PATH  # macOS

# Option 3: Use RPATH (automatic for installed binaries)
cmake -B build -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON
```

## Migrating Tests to Google Test

### Why Google Test?

- Industry standard with excellent IDE integration
- Rich assertion macros and test fixtures
- Automatic test discovery
- Better error messages

### Test Migration

Tests have been migrated from the custom framework to Google Test:

```cpp
// Old custom framework
TEST_CASE("MyTest") {
    ASSERT_TRUE(condition);
    ASSERT_EQ(expected, actual);
}

// New Google Test
TEST(MyTestSuite, MyTest) {
    EXPECT_TRUE(condition);
    EXPECT_EQ(expected, actual);
}
```

### Running Tests

```bash
# Build with testing
cmake -B build -DENABLE_TESTING=ON
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific tests
ctest --test-dir build -R "SimulationTest"

# Run with Google Test options
./build/tests/unit/test_solar_core --gtest_filter="*Simulation*"
```

## Migrating JSON Code to nlohmann/json

### Why nlohmann/json?

- Robust, well-tested library
- Intuitive API
- Excellent error messages
- Header-only (no additional linking)

### JSON Migration Examples

```cpp
// Old manual JSON building
std::ostringstream json;
json << "{\"name\":\"" << name << "\",\"value\":" << value << "}";

// New nlohmann/json
#include <nlohmann/json.hpp>
nlohmann::json j;
j["name"] = name;
j["value"] = value;
std::string json_str = j.dump();
```

```cpp
// Old manual JSON parsing
// (error-prone string manipulation)

// New nlohmann/json
auto j = nlohmann::json::parse(json_string);
std::string name = j["name"];
int value = j["value"];

// With error handling
try {
    auto j = nlohmann::json::parse(json_string);
    // ...
} catch (const nlohmann::json::parse_error& e) {
    std::cerr << "Parse error: " << e.what() << std::endl;
}
```

## Using Installed Libraries

### With pkg-config

```bash
# Get compiler flags
pkg-config --cflags solar_core

# Get linker flags
pkg-config --libs solar_core

# In Makefile
CFLAGS += $(shell pkg-config --cflags solar_core)
LDFLAGS += $(shell pkg-config --libs solar_core)
```

### With CMake find_package

```cmake
# In your CMakeLists.txt
find_package(SolarSystem REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app SolarSystem::solar_core)
```

## API Changes

### Export Macros

Public API classes and functions now use export macros:

```cpp
#include <solar_core/export.hpp>

// Public class
class SOLAR_CORE_API SimulationEngine {
    // ...
};

// Public function
SOLAR_CORE_API void initialize_system();
```

When using the library:
- Static builds: Macros expand to nothing
- Shared builds: Macros handle dllexport/dllimport (Windows) or visibility (Unix)

### No Code Changes Required

The export macros are designed to be transparent:
- Existing code continues to work
- No changes needed for library users
- Only library developers need to add macros to new public APIs

## Troubleshooting Migration Issues

### "Undefined symbol" errors

Ensure you're linking all required libraries:

```cmake
target_link_libraries(my_app
    SolarSystem::solar_core
    SolarSystem::solar_jpl
    SolarSystem::solar_utils
)
```

### "Cannot find nlohmann/json"

The library is fetched automatically. If using system installation:

```bash
cmake -B build -DUSE_SYSTEM_JSON=ON
```

### "Tests not found"

Ensure testing is enabled:

```bash
cmake -B build -DENABLE_TESTING=ON
cmake --build build
ctest --test-dir build -N  # List all tests
```

### Build fails with visibility errors

Ensure export headers are included:

```cpp
#include <solar_core/export.hpp>  // For SOLAR_CORE_API
```

## Version Compatibility

| Version | Static Libs | Shared Libs | Google Test | nlohmann/json |
|---------|-------------|-------------|-------------|---------------|
| < 4.0   | ✅ | ❌ | Custom | Manual |
| 4.0+    | ✅ | ✅ | ✅ | ✅ |

## Getting Help

- Check [BUILD_OPTIONS.md](BUILD_OPTIONS.md) for configuration details
- See [PERFORMANCE_COMPARISON.md](PERFORMANCE_COMPARISON.md) for benchmarks
- Review [Developer Guide](developer/DEVELOPER_GUIDE.md) for development setup
- Open an issue for migration problems
