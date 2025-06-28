# Solar System Simulation

A high-performance N-body gravitational simulation of the solar system with real-time JPL HORIZONS ephemeris data integration.

## Features

### 🚀 Real-Time JPL Data Integration
- **Automatic data fetching** from NASA JPL HORIZONS API
- **27 celestial bodies** including planets, moons, dwarf planets, and spacecraft
- **Binary cache system** for fast loading and offline operation
- **JSON backup format** for human-readable data storage
- **Automatic fallback** to hardcoded data when network unavailable

### 🌌 Comprehensive Solar System
- **Major planets**: Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune
- **Natural satellites**: Moon, Io, Europa, Ganymede, Callisto, Titan, Rhea, Iapetus, Titania, Oberon, Triton
- **Dwarf planets**: Pluto, Charon, Quaoar, Haumea, Eris
- **Spacecraft**: New Horizons, SpaceX Roadster

### ⚡ High-Performance Simulation
- **Optimized compilation** with native CPU instructions and LTO
- **Efficient algorithms** for gravitational calculations
- **Configurable time steps** and simulation parameters
- **Forward/backward time travel** simulation

## Installation

### Prerequisites
- **C++ compiler** with C++11 support (GCC/Clang)
- **CMake** 3.15 or higher
- **curl** for HTTP requests to JPL HORIZONS API

### Installing CMake
```bash
# macOS with Homebrew
brew install cmake

# Ubuntu/Debian
sudo apt-get install cmake

# CentOS/RHEL
sudo yum install cmake
```

### Build
```bash
git clone <repository-url>
cd solarsystem

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j$(nproc)

# Or use CMake's cross-platform build command
cmake --build . --parallel
```

### Alternative: Legacy Makefile
The original Makefile is still available for compatibility:
```bash
make clean && make
```

## Usage

### Basic Simulation
```bash
# From build directory
./solar_system

# Or install system-wide
make install
solar_system

# Simulate to specific date
./solar_system -d 2025-12-31

# Simulate with custom time step
./solar_system -t 60  # 60-second steps
```

### JPL Data Management
```bash
# Update ephemeris data from JPL HORIZONS
./solar_system -u

# Rebuild binary cache from JSON
./solar_system --rebuild

# Test storage system
./solar_system --test-storage
```

### Advanced Options
```bash
# Show help
./solar_system --help

# Verbose output
./solar_system -v

# Custom simulation parameters
./solar_system -d 2026-01-01 -t 30 -v
```

### Development Commands
```bash
# Format code (from build directory)
make format

# Clean cache files
make clean-cache

# Clean build files
make clean

# Reconfigure build
cd .. && rm -rf build && mkdir build && cd build && cmake ..
```

## Data Sources

### JPL HORIZONS Integration
The simulation automatically fetches real ephemeris data from NASA's JPL HORIZONS system:
- **API Endpoint**: `https://ssd.jpl.nasa.gov/api/horizons.api`
- **Reference Frame**: J2000 Ecliptic
- **Units**: Kilometers and seconds
- **Precision**: Full double precision

### Supported Bodies
| Category | Bodies | JPL IDs |
|----------|--------|---------|
| **Sun** | Sun | 10 |
| **Inner Planets** | Mercury, Venus, Earth, Mars | 199, 299, 399, 499 |
| **Earth System** | Moon | 301 |
| **Jupiter System** | Jupiter, Io, Europa, Ganymede, Callisto | 599, 501-504 |
| **Saturn System** | Saturn, Titan, Rhea, Iapetus | 699, 606, 605, 608 |
| **Uranus System** | Uranus, Titania, Oberon | 799, 703, 704 |
| **Neptune System** | Neptune, Triton | 899, 801 |
| **Pluto System** | Pluto, Charon | 999, 901 |
| **Dwarf Planets** | Quaoar, Haumea, Eris | 50000, 136108, 136199 |
| **Spacecraft** | New Horizons, SpaceX Roadster | -98, -143205 |

## Cache System

### Binary Cache (`ephemeris_cache.bin`)
- **Fast loading**: Optimized binary format
- **Integrity checks**: Magic numbers and checksums
- **Version control**: Automatic invalidation on format changes
- **Memory efficient**: Direct memory mapping

### JSON Cache (`ephemeris_data.json`)
- **Human readable**: Easy inspection and debugging
- **Backup format**: Fallback when binary cache fails
- **Portable**: Cross-platform compatibility
- **Editable**: Manual data modification if needed

### Cache Hierarchy
1. **Binary cache** (fastest)
2. **JSON cache** (fallback)
3. **Original hardcoded data** (offline fallback)

## Architecture

### Build System
- **CMake 3.15+**: Modern cross-platform build system
- **Out-of-source builds**: Clean separation in `build/` directory
- **Automatic dependency tracking**: Efficient incremental builds
- **Cross-platform support**: macOS, Linux, Windows
- **IDE integration**: Generate Xcode, VS Code, CLion projects

### Core Components
- **`solar_system.cpp`**: Main simulation loop and user interface
- **`jpl_data.cpp`**: JPL HORIZONS API integration and caching
- **`jpl_bodies.cpp`**: Body ID mapping and metadata
- **`simulation.cpp`**: N-body gravitational calculations
- **`model.cpp`**: Physics and mathematical models
- **`constants.cpp`**: Solar system body definitions

### Data Flow
```
JPL HORIZONS API → HTTP Client → Parser → Binary Cache → Simulation
                              ↓
                         JSON Cache → Validation → Physics Engine
```

## Performance

### Optimizations
- **Native compilation**: `-march=native -mtune=native`
- **Link-time optimization**: `-flto -fwhole-program`
- **Fast math**: `-ffast-math` for floating-point operations
- **Loop unrolling**: `-funroll-loops` for tight computation loops

### Benchmarks
- **Cache loading**: < 1ms for binary cache
- **JPL data fetch**: ~30-60 seconds for all 27 bodies
- **Simulation step**: Microseconds per time step

## Error Handling

### Network Issues
- **Graceful degradation**: Falls back to cached or hardcoded data
- **Retry logic**: Automatic retry for transient failures
- **Timeout handling**: Prevents hanging on slow connections

### Data Validation
- **Format verification**: Validates JPL response format
- **Checksum validation**: Ensures data integrity
- **Range checking**: Validates astronomical values

## Development

### Building with Debug Info
```bash
# Debug build
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# With profiling
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_PROFILING=ON ..
make
```

### Code Style
- **C++11 standard**: Modern C++ features
- **Automatic formatting**: Pre-commit hook with clang-format (see `FORMATTING.md`)
- **Consistent style**: Google C++ Style Guide with customizations
- **Clear naming**: Descriptive variable and function names
- **Comprehensive comments**: Document complex algorithms

### Testing
```bash
# Test storage system
./solar_system --test-storage

# Validate data integrity
./solar_system --validate

# Performance profiling
./solar_system --profile
```

## Troubleshooting

### Common Issues

**"Failed to fetch JPL data"**
- Check internet connection
- Verify JPL HORIZONS API availability
- Use cached data: simulation continues with last known good data

**"Binary cache corrupted"**
- Delete cache files: `rm ephemeris_*.*`
- Rebuild: `./solar_system -u`

**"Compilation errors"**
- Ensure C++11 support: `g++ --version`
- Install dependencies: `curl`, `make`
- Check system compatibility

### Debug Mode
```bash
# Enable verbose output
./solar_system -v

# Check cache status
ls -la ephemeris_*.*

# Validate data
./solar_system --validate
```

## Contributing

### Development Setup
1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Make changes with proper formatting
4. Test thoroughly: `make test`
5. Submit pull request

### Code Standards
- Follow existing code style
- Add tests for new features
- Update documentation
- Ensure backward compatibility

## License

[Add your license information here]

## Acknowledgments

- **NASA JPL HORIZONS**: Ephemeris data source
- **Solar System Dynamics Group**: JPL HORIZONS API
- **Contributors**: [List contributors]

## References

- [JPL HORIZONS System](https://ssd.jpl.nasa.gov/horizons/)
- [HORIZONS API Documentation](https://ssd-api.jpl.nasa.gov/doc/horizons.html)
- [Solar System Dynamics](https://ssd.jpl.nasa.gov/)

---

**Last Updated**: June 2025  
**Version**: 2.0 (JPL HORIZONS Integration)
