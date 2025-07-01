# Solar System Suite Developer Guide

## Getting Started

### Prerequisites
- **C++ Compiler**: GCC 4.8+ or Clang 3.3+ with C++11 support
- **CMake**: 3.15 or higher
- **curl**: For JPL HORIZONS API access
- **Git**: For version control

### Development Environment Setup

```bash
# Clone the repository
git clone <repository-url>
cd solarsystem

# Create development build
mkdir build-dev && cd build-dev
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests (when available)
make test
```

## Project Structure

```
solarsystem/
├── 📁 lib/                          # Modular libraries
│   ├── 📚 solar_core/               # Simulation engine
│   ├── 📡 solar_jpl/                # JPL integration
│   └── 🔧 solar_utils/              # Shared utilities
├── 📁 apps/                         # Specialized applications
│   ├── 🎛️ solar_system_launcher/    # Unified interface
│   ├── 📡 solar_system_fetch/       # Data management
│   ├── 🌌 solar_system/             # Batch simulation
│   ├── 🌍 solar_system_realtime/    # Live tracking
│   └── 🌐 solar_system_web/         # Web visualization
├── 📁 docs/                         # Documentation
├── 📁 cmake/                        # Build configuration
└── 📁 install/                      # Default installation
```

## Coding Standards

### C++ Style Guide
- **Standard**: C++11 (no newer features to maintain compatibility)
- **Formatting**: Auto-formatted with clang-format (pre-commit hook)
- **Naming**: 
  - Classes: `PascalCase` (e.g., `SolarSystem`)
  - Functions: `snake_case` (e.g., `update_positions`)
  - Variables: `snake_case` (e.g., `current_time`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_BODIES`)

### Code Organization
- **Headers**: `.h` files with include guards
- **Implementation**: `.cpp` files
- **Documentation**: Doxygen-style comments for public APIs
- **Error Handling**: Exceptions for critical errors, return codes for expected failures

### Example Code Style
```cpp
/**
 * @brief Updates planetary positions using numerical integration
 * @param time_step Integration time step in seconds
 * @param target_time Target simulation time
 * @return true if simulation successful, false otherwise
 */
bool SolarSystem::update_positions(double time_step, time_t target_time) {
    if (time_step <= 0) {
        std::cerr << "Invalid time step: " << time_step << std::endl;
        return false;
    }
    
    // Implementation here...
    return true;
}
```

## Library Development

### solar_core Library

**Key Classes**:
- `SolarSystem`: Main simulation state and logic
- `CelestialBody`: Individual body representation

**Adding New Simulation Features**:
1. Add function declaration to `simulation.h`
2. Implement in `simulation.cpp`
3. Update CMakeLists.txt if needed
4. Add documentation comments

**Example: Adding a New Function**:
```cpp
// In simulation.h
/**
 * @brief Calculate orbital period for a given body
 * @param body_index Index of the celestial body
 * @return Orbital period in seconds, or -1 if error
 */
double calculate_orbital_period(int body_index);

// In simulation.cpp
double SolarSystem::calculate_orbital_period(int body_index) {
    if (body_index < 0 || body_index >= BODY_COUNT) {
        return -1.0;
    }
    
    // Implementation using Kepler's laws
    // ...
    
    return period_seconds;
}
```

### solar_jpl Library

**Key Components**:
- JPL HORIZONS API integration
- Caching system (binary + JSON)
- Body classification system

**Adding New Celestial Bodies**:
1. Update `JPL_BODY_MAP` in `jpl_bodies.cpp`
2. Add JPL HORIZONS ID
3. Set appropriate body type (Essential/Important/Optional)
4. Update `BODY_COUNT` if needed

**Example: Adding a New Body**:
```cpp
// In jpl_bodies.cpp
static const JPLBodyInfo JPL_BODY_MAP[] = {
    // ... existing bodies ...
    {"Ceres", 1, BODY_IMPORTANT},  // New dwarf planet
    // ... rest of bodies ...
};
```

**Modifying Cache Behavior**:
- Binary cache: Modify `save_ephemeris_to_binary()` and `load_ephemeris_from_binary()`
- JSON cache: Modify `save_ephemeris_to_json()` and `load_ephemeris_from_json()`
- Smart caching: Adjust logic in `update_ephemeris_data()`

### solar_utils Library

**Common Utilities**:
- Argument parsing: `parse_arguments()`
- Date utilities: `parse_date_string()`, `format_date()`
- String helpers: `trim()`, `split()`

**Adding New Utilities**:
1. Add declaration to appropriate header
2. Implement in corresponding `.cpp` file
3. Ensure thread safety if needed
4. Add unit tests (when testing framework is available)

## Application Development

### Creating a New Application

1. **Create Directory Structure**:
```bash
mkdir apps/solar_system_newapp
cd apps/solar_system_newapp
```

2. **Create CMakeLists.txt**:
```cmake
# Solar System New Application
add_executable(solar_system_newapp
    newapp.cpp
)

target_link_libraries(solar_system_newapp
    solar_core
    solar_jpl
    solar_utils
)

# Install the executable
install(TARGETS solar_system_newapp
    DESTINATION bin
)
```

3. **Create Main Application File**:
```cpp
#include "../../lib/solar_core/simulation.h"
#include "../../lib/solar_jpl/jpl_data.h"
#include "../../lib/solar_utils/args.h"

int main(int argc, char* argv[]) {
    // Parse arguments
    if (!parse_arguments(argc, argv)) {
        return 1;
    }
    
    // Initialize simulation
    if (!initialize_solar_system()) {
        std::cerr << "Failed to initialize solar system" << std::endl;
        return 1;
    }
    
    // Your application logic here
    
    return 0;
}
```

4. **Update Root CMakeLists.txt**:
```cmake
# Add to the applications section
add_subdirectory(apps/solar_system_newapp)
```

### Web Application Development

**Key Files**:
- `web_server.cpp`: HTTP server and API endpoints
- `web/`: Static web files (HTML, CSS, JavaScript)

**Adding New API Endpoints**:
```cpp
// In web_server.cpp, add to handle_request()
if (request.path == "/api/new_endpoint") {
    response.headers["Content-Type"] = "application/json";
    response.body = generate_new_endpoint_json();
    return response;
}
```

**Adding New Web Features**:
1. Update HTML in `web/index.html`
2. Add JavaScript functionality
3. Create corresponding API endpoints
4. Test with browser developer tools

## Build System

### CMake Configuration

**Key CMake Files**:
- Root `CMakeLists.txt`: Main configuration
- `cmake/`: Build utilities and macros
- Library `CMakeLists.txt`: Individual library builds

**Build Types**:
- `Debug`: Development with debug symbols
- `Release`: Optimized production build
- `RelWithDebInfo`: Optimized with debug symbols

**Custom CMake Options**:
```bash
# Custom installation directory
cmake -DSOLAR_SYSTEM_INSTALL_DIR=/custom/path ..

# Debug build with profiling
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_PROFILING=ON ..
```

### Compilation Flags

**Release Optimization**:
- `-O3`: Maximum optimization
- `-march=native`: CPU-specific optimizations
- `-flto`: Link-time optimization
- `-ffast-math`: Fast math operations

**Debug Configuration**:
- `-g`: Debug symbols
- `-O0`: No optimization
- `-DDEBUG`: Debug macros

## Testing Strategy

### Unit Testing (Planned)
```cpp
// Example unit test structure
#include "gtest/gtest.h"
#include "../lib/solar_core/simulation.h"

class SolarSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
    }
    
    void TearDown() override {
        // Clean up
    }
};

TEST_F(SolarSystemTest, InitializationTest) {
    EXPECT_TRUE(initialize_solar_system());
    EXPECT_GT(get_body_count(), 0);
}
```

### Integration Testing
- Test complete workflows (fetch → simulate → output)
- Validate API endpoints
- Check cache consistency
- Performance benchmarking

### Manual Testing Checklist
- [ ] All applications compile and run
- [ ] Web interface loads and functions
- [ ] JPL data fetching works
- [ ] Cache system operates correctly
- [ ] Error handling behaves properly

## Debugging

### Common Debug Techniques

**Verbose Mode**:
```bash
# Enable verbose output for debugging
./solar_system_web --verbose
./solar_system_launcher --status --verbose
```

**GDB Debugging**:
```bash
# Compile with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Debug with GDB
gdb ./bin/solar_system
(gdb) run --date 2025-07-01
(gdb) bt  # backtrace on crash
```

**Memory Debugging**:
```bash
# Use Valgrind for memory leak detection
valgrind --leak-check=full ./bin/solar_system --date 2025-07-01
```

### Common Issues and Solutions

**Build Errors**:
- Check C++11 compiler support
- Verify CMake version (3.15+)
- Ensure all dependencies are installed

**Runtime Errors**:
- Check file permissions for cache files
- Verify network connectivity for JPL API
- Validate input date formats

**Performance Issues**:
- Profile with `gprof` or `perf`
- Check cache hit rates
- Monitor memory usage

## Contributing

### Development Workflow

1. **Create Feature Branch**:
```bash
git checkout -b feature/new-feature-name
```

2. **Make Changes**:
- Follow coding standards
- Add appropriate documentation
- Test thoroughly

3. **Commit Changes**:
```bash
git add .
git commit -m "feat: Add new feature description"
```

4. **Pre-commit Checks**:
- Code formatting (automatic via hook)
- Build verification
- Basic functionality testing

5. **Submit Pull Request**:
- Describe changes clearly
- Include test results
- Reference any related issues

### Code Review Guidelines

**What to Look For**:
- Code correctness and logic
- Performance implications
- Memory management
- Error handling
- Documentation completeness
- Coding standard compliance

**Review Process**:
1. Automated checks (formatting, build)
2. Manual code review
3. Testing verification
4. Documentation review
5. Approval and merge

## Performance Optimization

### Profiling Tools
- **gprof**: Function-level profiling
- **perf**: System-level performance analysis
- **Valgrind**: Memory usage and leak detection

### Optimization Strategies
- **Algorithmic**: Choose efficient algorithms
- **Memory**: Minimize allocations and cache misses
- **I/O**: Batch operations and use binary formats
- **Compilation**: Use appropriate compiler flags

### Benchmarking
```cpp
// Example benchmarking code
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
// Code to benchmark
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Operation took: " << duration.count() << " microseconds" << std::endl;
```

## Documentation

### API Documentation
- Use Doxygen comments for all public functions
- Include parameter descriptions and return values
- Provide usage examples

### User Documentation
- Keep README.md updated
- Document new features and changes
- Include installation and usage instructions

### Architecture Documentation
- Update architecture diagrams for major changes
- Document design decisions
- Explain trade-offs and alternatives considered

This developer guide provides the foundation for contributing to and extending the Solar System Suite. For specific questions or clarifications, refer to the existing code examples and architecture documentation.
