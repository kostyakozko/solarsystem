# Build Options Documentation

This document describes all CMake build options available for the Solar System Suite.

## Quick Reference

```bash
# Static libraries (default, maximum performance)
cmake -B build

# Shared libraries (smaller binaries, faster incremental builds)
cmake -B build -DBUILD_SHARED_LIBS=ON

# With testing enabled
cmake -B build -DENABLE_TESTING=ON

# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Full development setup
cmake -B build -DBUILD_SHARED_LIBS=ON -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
```

## Build Configuration Options

### Library Type

#### `BUILD_SHARED_LIBS` (Default: OFF)

Controls whether libraries are built as static (.a) or shared (.so/.dylib).

```bash
# Static libraries (default)
cmake -B build -DBUILD_SHARED_LIBS=OFF

# Shared libraries
cmake -B build -DBUILD_SHARED_LIBS=ON
```

**Static Libraries (OFF)**:
- Maximum runtime performance
- Simpler deployment (no library path configuration)
- Full link-time optimization (LTO) across libraries
- Larger executable sizes

**Shared Libraries (ON)**:
- Smaller executable sizes (65-89% reduction)
- Faster incremental builds during development
- Libraries can be updated without relinking applications
- Slight startup overhead (~15-20ms)

See [Performance Comparison](PERFORMANCE_COMPARISON.md) for detailed benchmarks.

### Testing

#### `ENABLE_TESTING` (Default: OFF)

Enables the test suite using Google Test.

```bash
cmake -B build -DENABLE_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

When enabled:
- Google Test is fetched via FetchContent (or uses system installation)
- 288 tests are available (unit, integration, benchmarks)
- Test discovery is automatic via `gtest_discover_tests`

### Build Type

#### `CMAKE_BUILD_TYPE` (Default: Release)

Standard CMake build type option.

| Type | Optimization | Debug Info | Use Case |
|------|--------------|------------|----------|
| Release | -O3, LTO | None | Production |
| Debug | -O0 | Full | Development |
| RelWithDebInfo | -O2 | Full | Profiling |
| MinSizeRel | -Os | None | Size-constrained |

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

### External Dependencies

#### `USE_SYSTEM_GTEST` (Default: OFF)

Use system-installed Google Test instead of FetchContent.

```bash
# Use system Google Test
cmake -B build -DUSE_SYSTEM_GTEST=ON -DENABLE_TESTING=ON
```

#### `USE_SYSTEM_JSON` (Default: OFF)

Use system-installed nlohmann/json instead of FetchContent.

```bash
# Use system nlohmann/json
cmake -B build -DUSE_SYSTEM_JSON=ON
```

#### `USE_SYSTEM_MSGPACK` (Default: OFF)

Use system-installed msgpack-c instead of FetchContent.

```bash
# Use system msgpack
cmake -B build -DUSE_SYSTEM_MSGPACK=ON
```

### Installation

#### `CMAKE_INSTALL_PREFIX` / `SOLAR_SYSTEM_INSTALL_DIR`

Set custom installation directory.

```bash
# Via CMake variable
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/solar_system

# Via environment variable
export SOLAR_SYSTEM_INSTALL_DIR=/opt/solar_system
cmake -B build
```

### Symbol Visibility

#### `ENABLE_SYMBOL_VISIBILITY` (Default: ON)

Controls symbol visibility for shared libraries.

When ON (recommended for shared libraries):
- Uses `-fvisibility=hidden` by default
- Only explicitly exported symbols are visible
- Reduces binary size and improves load time

## Platform-Specific Notes

### macOS

```bash
# Default build (uses system clang)
cmake -B build

# Shared libraries create .dylib files with proper install_name
cmake -B build -DBUILD_SHARED_LIBS=ON
```

### Linux

```bash
# Default build (uses system gcc/g++)
cmake -B build

# Shared libraries create .so files with proper SONAME
cmake -B build -DBUILD_SHARED_LIBS=ON
```

### Windows

Windows builds are supported via:

1. **WSL (Recommended)**: Use the Linux build instructions in WSL
2. **Native MSVC**: Use vcpkg for dependencies

See [requirements-windows.txt](../requirements-windows.txt) for detailed instructions.

## Recommended Configurations

### Development

Fast iteration with debugging support:

```bash
cmake -B build \
  -DBUILD_SHARED_LIBS=ON \
  -DENABLE_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Debug
```

### Production

Maximum performance for deployment:

```bash
cmake -B build \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### CI/CD

Comprehensive testing:

```bash
cmake -B build \
  -DENABLE_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Release
```

## Verifying Build Configuration

After configuration, CMake prints a summary:

```
-- Solar System Suite Configuration:
--   Version: 4.0.0
--   Build type: Release
--   C++ standard: 20
--   Build system modernization:
--     Shared libraries: OFF
--     System Google Test: OFF
--     System nlohmann/json: OFF
--     Symbol visibility: ON
```

## Troubleshooting

### "Cannot find Google Test"

```bash
# Either install system Google Test or use FetchContent (default)
cmake -B build -DUSE_SYSTEM_GTEST=OFF -DENABLE_TESTING=ON
```

### "Shared library not found at runtime"

Check RPATH configuration:
```bash
# macOS
otool -L ./bin/solar_system

# Linux
ldd ./bin/solar_system
```

### "Symbol not found"

Ensure export macros are used for public API:
```cpp
#include <solar_core/export.hpp>

class SOLAR_CORE_API MyClass { ... };
```

## Related Documentation

- [Performance Comparison](PERFORMANCE_COMPARISON.md) - Static vs shared benchmarks
- [Migration Guide](MIGRATION_GUIDE.md) - Upgrading from previous versions
- [Developer Guide](developer/DEVELOPER_GUIDE.md) - Development setup
