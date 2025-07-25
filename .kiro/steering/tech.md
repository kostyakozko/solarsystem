# Solar System Suite Technology Stack

## Build System

- **CMake 3.15+**: Modern cross-platform build system with out-of-source builds
- **C++20 Standard**: Modern C++ with concepts, smart pointers, and RAII patterns
- **Static Libraries**: All libraries built as static for maximum performance and LTO benefits

## Tech Stack

### Core Technologies
- **C++20**: Primary language with modern features (concepts, ranges, coroutines ready)
- **Standard Library Only**: Zero external dependencies for maximum portability
- **POSIX/Standard APIs**: Cross-platform compatibility (macOS, Linux, Windows)

### External Dependencies
- **curl**: Required for JPL HORIZONS API data fetching
- **Threads**: Standard threading library for concurrent operations
- **WebGL**: Client-side 3D rendering for web interface

### Development Tools
- **clang-format**: Code formatting with Google style (100 column limit)
- **Doxygen**: API documentation generation
- **Git**: Version control with comprehensive documentation

## Compiler Requirements

### Supported Compilers
- **GCC 7+**: With C++20 support
- **Clang 5+**: With C++20 support
- **MSVC**: Visual Studio 2019+ (Windows)

### Optimization Flags
- **Release**: `-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops`
- **Debug**: `-g -O0 -Wall -Wextra`
- **Profiling**: `-pg` (optional with ENABLE_PROFILING)

## Common Build Commands

### Initial Setup
```bash
# Clone and setup
git clone <repository-url>
cd solarsystem
mkdir build && cd build
```

### Standard Build
```bash
# Configure and build
cmake ..
make -j$(nproc)
make install
```

### Custom Installation
```bash
# Set custom install directory
export SOLAR_SYSTEM_INSTALL_DIR=/opt/solar_system
cmake ..
make -j$(nproc) && make install
```

### Development Commands
```bash
# Format all code
make format

# Clean cache files
make clean-cache

# Build specific targets
make libraries        # Build all libraries
make applications     # Build all applications
make solar_core       # Build specific library

# Debug build
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Testing (when enabled)
```bash
# Enable testing
cmake -DENABLE_TESTING=ON ..
make

# Run tests
make test-unit
make test-integration
make test-benchmarks
make test-all
```

## Architecture Libraries

### solar_core
- **Purpose**: High-performance N-body simulation engine
- **Key Files**: `simulation.h/cpp`, `solar_system.h/cpp`
- **Dependencies**: solar_utils, solar_jpl, Threads

### solar_jpl
- **Purpose**: JPL HORIZONS API integration and caching
- **Key Files**: `jpl_data.h/cpp`, `jpl_bodies.h/cpp`
- **Dependencies**: solar_utils, curl (external)

### solar_utils
- **Purpose**: Shared utilities and argument parsing
- **Key Files**: `args.h/cpp`, utility functions
- **Dependencies**: Standard library only

## Performance Characteristics

- **Link-Time Optimization (LTO)**: Enabled for Release builds
- **Native CPU Instructions**: `-march=native -mtune=native`
- **Smart Caching**: Binary cache loading < 1ms
- **Parallel JPL Fetching**: 3 concurrent connections
- **WebGL Rendering**: 60 FPS interactive visualization

## Code Style Standards

- **Google Style Base**: Modified for 100 column limit
- **Indentation**: 2 spaces, no tabs
- **Braces**: Attach style (`{` on same line)
- **Pointers**: Left-aligned (`int* ptr`)
- **Auto-formatting**: Use `make format` or `./format-code.sh`
- **Headers**: Include guards, proper ordering (system, local)
