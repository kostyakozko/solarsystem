# Design Document - Build System and Dependency Modernization

## Overview

This design document outlines the technical approach for modernizing the Solar System Suite's build system to support shared libraries and integrate industry-standard external dependencs. The design emphasizes backward compatibility, cross-platform support, and a phased migration strategy that minimizes disruption to existing development workflows.

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Build Configuration                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ CMake Options│  │ Toolchain    │  │ Platform     │     │
│  │ - SHARED/    │  │ Detection    │  │ Detection    │     │
│  │   STATIC     │  │              │  │              │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                  Library Build System                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Symbol Visibility Layer                             │  │
│  │  - Export macros (SOLAR_*_API)                       │  │
│  │  - Platform-specific attributes                      │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                 │
│  ┌──────────────┬──────────────┬──────────────┬─────────┐  │
│  │ solar_core   │ solar_jpl    │ solar_utils  │solar_test│ │
│  │ (.so/.dylib/ │ (.so/.dylib/ │ (.so/.dylib/ │(.so/    │  │
│  │  .dll/.a)    │  .dll/.a)    │  .dll/.a)    │.dylib)  │  │
│  └──────────────┴──────────────┴──────────────┴─────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              External Dependencies Layer                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Google Test  │  │ nlohmann/json│  │ Future libs  │     │
│  │ (FetchContent│  │ (FetchContent│  │              │     │
│  │  or system)  │  │  or system)  │  │              │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Applications                              │
│  ┌──────────────┬──────────────┬──────────────┬─────────┐  │
│  │solar_system  │solar_system_ │solar_system_ │  ...    │  │
│  │              │fetch         │web           │         │  │
│  └──────────────┴──────────────┴──────────────┴─────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. CMake Build Configuration

**Purpose:** Provide flexible build options for library types and external dependencies.

**Key Files:**
- `CMakeLists.txt` (root)
- `lib/CMakeLists.txt`
- `lib/solar_*/CMakeLists.txt`

**Configuration Options:**
```cmake
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(USE_SYSTEM_GTEST "Use system Google Test" OFF)
option(USE_SYSTEM_JSON "Use system nlohmann/json" OFF)
option(ENABLE_SYMBOL_VISIBILITY "Enable symbol visibility control" ON)
option(BUILD_BOTH_LIBS "Build both static and shared libraries" OFF)
```

**Implementation Strategy:**
```cmake
# In lib/solar_core/CMakeLists.txt
if(BUILD_SHARED_LIBS)
    add_library(solar_core SHARED ${SOURCES})
    target_compile_definitions(solar_core PRIVATE SOLAR_CORE_EXPORTS)
    set_target_properties(solar_core PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
else()
    add_library(solar_core STATIC ${SOURCES})
endif()
```

### 2. Symbol Visibility System

**Purpose:** Control which symbols are exported from shared libraries.

**Header Structure:**
```cpp
// lib/solar_core/include/solar_core/export.hpp
#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifdef SOLAR_CORE_EXPORTS
        #define SOLAR_CORE_API __declspec(dllexport)
    #else
        #define SOLAR_CORE_API __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define SOLAR_CORE_API __attribute__((visibility("default")))
    #else
        #define SOLAR_CORE_API
    #endif
#endif

// For classes
#define SOLAR_CORE_CLASS SOLAR_CORE_API

// For template classes (header-only, no export needed)
#define SOLAR_CORE_TEMPLATE
```

**Usage Pattern:**
```cpp
// Public API class
class SOLAR_CORE_API SimulationEngine {
public:
    SOLAR_CORE_API void run();

private:
    void internal_method(); // Not exported
};

// Public API function
SOLAR_CORE_API void initialize_system();
```

**Per-Library Export Headers:**
- `solar_core/export.hpp` → `SOLAR_CORE_API`
- `solar_jpl/export.hpp` → `SOLAR_JPL_API`
- `solar_utils/export.hpp` → `SOLAR_UTILS_API`
- `solar_test/export.hpp` → `SOLAR_TEST_API`

### 3. RPATH/RUNPATH Configuration

**Purpose:** Enable applications to find shared libraries at runtime.

**CMake Configuration:**
```cmake
# Set RPATH for installed binaries
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# For development builds
set(CMAKE_BUILD_RPATH "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# Platform-specific settings
if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()
```

### 4. Google Test Integration

**Purpose:** Replace custom test framework with industry-standard Google Test.

**CMake Integration:**
```cmake
# In tests/CMakeLists.txt
if(USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Link tests
add_executable(test_simulation test_simulation.cpp)
target_link_libraries(test_simulation
    PRIVATE
        solar_core
        GTest::gtest_main
)

# Enable Google Test discovery
include(GoogleTest)
gtest_discover_tests(test_simulation)
```

**Migration Pattern:**
```cpp
// Old custom framework
TEST_CASE("Simulation Basic Test") {
    ASSERT_TRUE(condition);
    ASSERT_EQ(expected, actual);
}

// New Google Test
TEST(SimulationTest, BasicTest) {
    EXPECT_TRUE(condition);
    EXPECT_EQ(expected, actual);
}
```

**Compatibility Layer (Optional):**
```cpp
// tests/utils/test_compat.hpp
// Provides backward compatibility during migration
#define TEST_CASE(name) TEST(LegacyTest, name)
#define ASSERT_TRUE(x) EXPECT_TRUE(x)
#define ASSERT_EQ(x, y) EXPECT_EQ(x, y)
// ... other mappings
```

### 5. nlohmann/json Integration

**Purpose:** Replace custom JSON handling with robust library.

**CMake Integration:**
```cmake
if(USE_SYSTEM_JSON)
    find_package(nlohmann_json REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(json)
endif()

target_link_libraries(solar_core PRIVATE nlohmann_json::nlohmann_json)
```

**Usage Pattern:**
```cpp
// Old custom JSON
std::ostringstream json;
json << "{\"name\":\"" << name << "\",\"value\":" << value << "}";

// New nlohmann/json
nlohmann::json j;
j["name"] = name;
j["value"] = value;
std::string json_str = j.dump();

// Parsing
auto j = nlohmann::json::parse(json_string);
std::string name = j["name"];
int value = j["value"];
```

### 6. Package Config Generation

**Purpose:** Enable other projects to find and use Solar System Suite libraries.

**pkg-config Files:**
```cmake
# Generate .pc files
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/solar_core.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/solar_core.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/solar_core.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
```

**solar_core.pc.in:**
```
prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@

Name: solar_core
Description: Solar System Suite Core Library
Version: @PROJECT_VERSION@
Requires: solar_utils solar_jpl
Libs: -L${libdir} -lsolar_core
Cflags: -I${includedir}
```

**CMake Config Files:**
```cmake
# Generate CMake config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/SolarSystemConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SolarSystem
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SolarSystemConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SolarSystem
)
```

## Data Models

### Library Versioning

**Semantic Versioning:**
- MAJOR.MINOR.PATCH (e.g., 4.0.0)
- SOVERSION = MAJOR (e.g., 4)

**Version Management:**
```cmake
set(PROJECT_VERSION_MAJOR 4)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set_target_properties(solar_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
```

**ABI Compatibility Rules:**
- Increment MAJOR: Breaking ABI changes (remove functions, change signatures)
- Increment MINOR: Add new functions (ABI compatible)
- Increment PATCH: Bug fixes only (ABI compatible)

## Error Handling

### Build Configuration Errors

**Missing Dependencies:**
```cmake
if(USE_SYSTEM_GTEST)
    find_package(GTest)
    if(NOT GTest_FOUND)
        message(FATAL_ERROR
            "Google Test not found. Either install it or set USE_SYSTEM_GTEST=OFF")
    endif()
endif()
```

**Platform Incompatibilities:**
```cmake
if(WIN32 AND BUILD_SHARED_LIBS AND NOT MSVC)
    message(WARNING
        "Shared libraries on Windows require MSVC or MinGW with proper export handling")
endif()
```

### Runtime Loading Errors

**Shared Library Not Found:**
- Use `ldd` (Linux), `otool -L` (macOS), or `dumpbin` (Windows) to diagnose
- Verify RPATH is set correctly
- Check LD_LIBRARY_PATH (Linux) or DYLD_LIBRARY_PATH (macOS)

**Symbol Resolution Errors:**
- Ensure all exported symbols are properly marked with export macros
- Check for missing dependencies in link line
- Verify symbol visibility settings

## Testing Strategy

### Build Configuration Testing

**Test Matrix:**
```yaml
configurations:
  - name: "Static Libraries"
    options: "-DBUILD_SHARED_LIBS=OFF"

  - name: "Shared Libraries"
    options: "-DBUILD_SHARED_LIBS=ON"

  - name: "System Dependencies"
    options: "-DBUILD_SHARED_LIBS=ON -DUSE_SYSTEM_GTEST=ON -DUSE_SYSTEM_JSON=ON"

  - name: "Both Library Types"
    options: "-DBUILD_BOTH_LIBS=ON"

platforms:
  - ubuntu-latest
  - macos-latest
  - windows-latest
```

### Test Migration Strategy

**Phase 1: Parallel Testing**
- Keep both old and new test frameworks
- Run same tests with both frameworks
- Verify identical results

**Phase 2: Gradual Migration**
- Convert tests file by file
- Use compatibility layer for unconverted tests
- Maintain 100% test coverage

**Phase 3: Cleanup**
- Remove custom test framework
- Remove compatibility layer
- Update documentation

### Performance Testing

**Benchmarks:**
- Startup time (shared vs static)
- Link time (development builds)
- Runtime performance (PIC overhead)
- Memory usage (shared library overhead)

**Acceptance Criteria:**
- Shared library runtime performance within 5% of static
- Development link time reduced by at least 50%
- Memory overhead acceptable (<10MB per application)

## Migration Plan

### Phase 1: Infrastructure (Weeks 1-2)

1. Add CMake options for shared libraries
2. Create export header files for all libraries
3. Set up RPATH configuration
4. Create pkg-config and CMake config templates

### Phase 2: Symbol Visibility (Weeks 3-4)

1. Audit public API surface
2. Add export macros to public headers
3. Test shared library builds on all platforms
4. Fix symbol visibility issues

### Phase 3: External Dependencies (Weeks 5-6)

1. Integrate Google Test via FetchContent
2. Create test compatibility layer
3. Integrate nlohmann/json
4. Create JSON migration utilities

### Phase 4: Test Migration (Weeks 7-10)

1. Convert unit tests to Google Test (parallel with old framework)
2. Convert integration tests
3. Convert benchmark tests
4. Remove custom test framework

### Phase 5: Validation and Documentation (Weeks 11-12)

1. Run full test suite on all platforms
2. Performance benchmarking
3. Update build documentation
4. Create migration guide for users
5. Update CI/CD pipelines

## Performance Considerations

### Position Independent Code (PIC)

**Overhead:**
- x86_64: Minimal (<2% typically)
- ARM64: Negligible (position-independent by default)
- x86 (32-bit): Higher overhead (5-10%)

**Mitigation:**
- Use LTO when possible
- Profile hot paths
- Consider keeping performance-critical code in static libraries

### Link-Time Optimization (LTO)

**With Shared Libraries:**
```cmake
if(BUILD_SHARED_LIBS)
    # LTO still beneficial for intra-library optimization
    set_target_properties(solar_core PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()
```

**Limitations:**
- Cross-library LTO not possible with shared libraries
- Consider static builds for maximum performance deployments

### Memory Considerations

**Shared Library Benefits:**
- Single copy in memory shared by all processes
- Reduced disk space usage

**Shared Library Costs:**
- PLT/GOT overhead for function calls
- Additional memory for dynamic linker structures

## Cross-Platform Considerations

### macOS Specifics

**Install Names:**
```cmake
set_target_properties(solar_core PROPERTIES
    INSTALL_NAME_DIR "@rpath"
    BUILD_WITH_INSTALL_NAME_DIR TRUE
)
```

**Framework Support (Optional):**
```cmake
option(BUILD_FRAMEWORK "Build as macOS Framework" OFF)
if(APPLE AND BUILD_FRAMEWORK)
    set_target_properties(solar_core PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER com.solarsystem.core
    )
endif()
```

### Windows Specifics

**DLL Export/Import:**
- Requires explicit `__declspec(dllexport/dllimport)`
- Export header must handle both building and using library
- Consider using .def files for complex exports

**Runtime Dependencies:**
- DLLs must be in PATH or same directory as executable
- Consider using `install(RUNTIME_DEPENDENCY_SET)` for automatic DLL copying

### Linux Specifics

**SONAME Management:**
```cmake
set_target_properties(solar_core PROPERTIES
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)
```

**Symbol Versioning (Advanced):**
```cmake
# Optional: Use version scripts for fine-grained ABI control
set_target_properties(solar_core PROPERTIES
    LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/solar_core.map"
)
```

## Documentation Requirements

### Build Documentation

- Update README.md with new build options
- Document CMake configuration options
- Provide platform-specific build instructions
- Include troubleshooting guide

### API Documentation

- Mark exported symbols in Doxygen
- Document ABI stability guarantees
- Provide migration guide from static to shared
- Include examples of using installed libraries

### Developer Documentation

- Symbol visibility guidelines
- Adding new public APIs
- ABI compatibility checklist
- Performance profiling guide

