# Solar System Suite User Guide

## Quick Start

### Installation

1. **Prerequisites**:
   - C++ compiler with C++11 support
   - CMake 3.15+
   - curl (for JPL data fetching)

2. **Build and Install**:
```bash
git clone <repository-url>
cd solarsystem
mkdir build && cd build
cmake ..
make -j$(nproc)
make install
```

3. **Verify Installation**:
```bash
cd ../install
./solar_system_launcher --status
```

## Applications Overview

The Solar System Suite provides 5 specialized applications:

| Application | Purpose | Best For |
|-------------|---------|----------|
| 🎛️ `solar_system_launcher` | Unified interface | System management, workflows |
| 📡 `solar_system_fetch` | Data management | Updating JPL data, cache management |
| 🌌 `solar_system` | High-performance simulation | Batch processing, specific dates |
| 🌍 `solar_system_realtime` | Live tracking | Demonstrations, current positions |
| 🌐 `solar_system_web` | Interactive visualization | Education, exploration, time travel |

## Getting Started Tutorials

### Tutorial 1: Your First Simulation

**Goal**: Run a basic solar system simulation

```bash
# Check system status
./solar_system_launcher --status

# Update data if needed
./solar_system_launcher --fetch --update

# Run simulation for a specific date
./solar_system_launcher --simulate --date 2025-07-01
```

**Expected Output**:
```
=== Solar System Simulation Results ===
Date: 2025-07-01 00:00:00 UTC
Bodies: 27 celestial objects

Sun:
  Position: (0.00e+00, 0.00e+00, 0.00e+00) km
  
Earth:
  Position: (-2.52e+07, 1.47e+08, -1.23e+03) km
  Distance from Sun: 1.49e+08 km (0.997 AU)
  
[... additional bodies ...]
```

### Tutorial 2: Real-Time Solar System

**Goal**: Watch the solar system in real-time

```bash
# Start real-time tracking
./solar_system_realtime

# With velocities and faster updates
./solar_system_realtime --velocities --display-interval 5
```

**What You'll See**:
- Continuous updates of planetary positions
- Real-time distance calculations
- Current solar system state

### Tutorial 3: Interactive Web Visualization

**Goal**: Explore the solar system in your browser

```bash
# Start web server
./solar_system_web

# Open browser to: http://localhost:8080
```

**Web Interface Features**:
- **Time Travel**: Jump to any historical date
- **Speed Control**: Watch years pass in seconds
- **Orbit Trails**: Visualize planetary paths
- **Interactive 3D**: Rotate and zoom with mouse

### Tutorial 4: Data Management

**Goal**: Understand and manage JPL data

```bash
# Check current data status
./solar_system_fetch

# Force update all data
./solar_system_fetch --force

# Validate cache integrity
./solar_system_fetch --validate
```

## Detailed Application Guides

### 🎛️ Solar System Launcher

**Purpose**: Unified interface for all operations

**Common Commands**:
```bash
# System overview
./solar_system_launcher --status

# Complete workflow: update data then simulate
./solar_system_launcher --fetch --update --simulate --date 2025-12-31

# Auto-fetch and simulate
./solar_system_launcher --simulate --auto-fetch --date 2030-01-01
```

**Options**:
- `--status`: Show system status and data currency
- `--fetch`: Data management operations
- `--simulate`: Run simulation
- `--auto-fetch`: Automatically update data if needed
- `--date YYYY-MM-DD`: Target simulation date

### 📡 Solar System Fetch

**Purpose**: JPL HORIZONS data management

**Data Update Workflow**:
```bash
# Check what data you have
./solar_system_fetch

# Update current year data (smart caching)
./solar_system_fetch --update

# Force complete refresh (bypasses cache)
./solar_system_fetch --force

# Clean and rebuild cache
./solar_system_fetch --clean
./solar_system_fetch --update
```

**Cache System**:
- **Binary Cache**: Fast loading (< 1ms)
- **JSON Cache**: Human-readable backup
- **Smart Caching**: Only updates when needed
- **Validation**: Integrity checking

### 🌌 Solar System Simulation

**Purpose**: High-performance batch simulation

**Basic Usage**:
```bash
# Simulate to specific date
./solar_system --date 2025-07-01

# Verbose output with details
./solar_system --date 2025-12-31 --verbose

# Custom time step (advanced)
./solar_system --date 2025-07-01 --timestep 60
```

**Performance Tips**:
- Uses cached JPL data for maximum speed
- Optimized for batch processing
- Best for scripting and automation

### 🌍 Solar System Realtime

**Purpose**: Live solar system monitoring

**Usage Examples**:
```bash
# Basic real-time tracking
./solar_system_realtime

# Show velocities with fast updates
./solar_system_realtime --velocities --display-interval 5

# Single snapshot (no continuous mode)
./solar_system_realtime --no-continuous

# Quiet mode for data logging
./solar_system_realtime --quiet --display-interval 30 > positions.log
```

**Display Options**:
- `--velocities`: Show velocity vectors
- `--display-interval N`: Update every N seconds
- `--no-continuous`: Single snapshot mode
- `--quiet`: Minimal output for logging

### 🌐 Solar System Web

**Purpose**: Interactive browser-based visualization

**Starting the Server**:
```bash
# Default settings (port 8080)
./solar_system_web

# Custom port and verbose logging
./solar_system_web --port 3000 --verbose

# Specify web root directory
./solar_system_web --web-root /path/to/web/files
```

**Web Interface Guide**:

1. **Time Travel Mode**:
   - Select historical dates (1990, 2000, 2010, 2020+)
   - Click "Start Time Travel" for large jumps
   - Use speed slider for animation

2. **Interactive Controls**:
   - **Mouse Drag**: Rotate view
   - **Mouse Scroll**: Zoom in/out
   - **Speed Control**: 0.1x to 1 year/second

3. **Visual Features**:
   - **Orbit Trails**: Enable to see planetary paths
   - **Labels**: Show planet names
   - **Real-time Sync**: Switch between modes

## Advanced Usage

### Scripting and Automation

**Batch Processing Example**:
```bash
#!/bin/bash
# Process multiple dates
dates=("2025-01-01" "2025-07-01" "2026-01-01")

for date in "${dates[@]}"; do
    echo "Processing $date..."
    ./solar_system --date $date > "results_$date.txt"
done
```

**Data Pipeline Example**:
```bash
#!/bin/bash
# Automated data update and simulation pipeline
./solar_system_launcher --fetch --update
if [ $? -eq 0 ]; then
    ./solar_system_launcher --simulate --date $(date +%Y-%m-%d)
else
    echo "Data update failed, using cached data"
    ./solar_system --date $(date +%Y-%m-%d)
fi
```

### Configuration and Customization

**Environment Variables**:
```bash
# Custom installation directory
export SOLAR_SYSTEM_INSTALL_DIR=/opt/solar_system

# Custom cache location (if implemented)
export SOLAR_SYSTEM_CACHE_DIR=/var/cache/solar_system
```

**Web Server Configuration**:
```bash
# Production deployment
./solar_system_web --port 80 --no-cors

# Development with debugging
./solar_system_web --port 8080 --verbose
```

## Understanding the Data

### Celestial Bodies Coverage

The suite tracks **27 celestial bodies**:

**Essential Bodies** (Always required):
- Sun
- 8 Planets: Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune

**Important Bodies** (Major moons and dwarf planets):
- Earth system: Moon
- Jupiter system: Io, Europa, Ganymede, Callisto
- Saturn system: Titan, Rhea, Iapetus
- Uranus system: Titania, Oberon
- Neptune system: Triton
- Pluto system: Pluto, Charon
- Dwarf planets: Quaoar, Haumea, Eris

**Optional Bodies** (Spacecraft):
- New Horizons
- SpaceX Roadster

### Data Sources and Accuracy

**Primary Source**: NASA JPL HORIZONS System
- **Reference Frame**: J2000 Ecliptic
- **Units**: Kilometers and seconds
- **Precision**: Full double precision
- **Update Frequency**: Automatic yearly updates

**Fallback Sources**:
1. Binary cache (fastest)
2. JSON cache (human-readable)
3. Hardcoded ephemeris (offline capability)

### Coordinate System

**Reference Frame**: J2000 Ecliptic
- **Origin**: Solar System Barycenter
- **X-axis**: Towards vernal equinox (J2000.0)
- **Y-axis**: 90° east in ecliptic plane
- **Z-axis**: Towards ecliptic north pole

**Units**:
- **Distance**: Kilometers
- **Time**: Seconds since Unix epoch
- **Velocity**: Kilometers per second

## Troubleshooting

### Common Issues

**"Failed to fetch JPL data"**:
- **Cause**: Network issues or JPL HORIZONS API unavailable
- **Solution**: Check internet connection, try again later
- **Workaround**: Use cached data (simulation continues automatically)

**"Binary cache corrupted"**:
- **Cause**: Incomplete download or disk issues
- **Solution**: 
  ```bash
  ./solar_system_fetch --clean
  ./solar_system_fetch --update
  ```

**Web interface shows black screen**:
- **Cause**: WebGL not supported or JavaScript errors
- **Solution**: 
  - Check browser console (F12)
  - Try different browser
  - Ensure WebGL is enabled

**"Compilation errors"**:
- **Cause**: Missing dependencies or incompatible compiler
- **Solution**:
  - Install C++11 compatible compiler
  - Install CMake 3.15+
  - Install curl development libraries

### Debug Mode

**Enable Verbose Output**:
```bash
# For applications
./solar_system_launcher --status --verbose

# For web server
./solar_system_web --verbose

# For data fetching
./solar_system_fetch --update --verbose
```

**Check System Status**:
```bash
# Comprehensive system check
./solar_system_launcher --status

# Cache validation
./solar_system_fetch --validate

# Test web server
curl http://localhost:8080/api/status
```

### Performance Optimization

**Improve Startup Time**:
- Keep binary cache files intact
- Use SSD storage for cache
- Ensure sufficient RAM (cache loads into memory)

**Web Interface Performance**:
- Use modern browser with WebGL support
- Close other browser tabs for better performance
- Reduce orbit trail length for older hardware

**Simulation Performance**:
- Use Release build for production
- Enable CPU-specific optimizations
- Use binary cache instead of JSON

## Best Practices

### Data Management
- Update JPL data monthly for current simulations
- Keep cache files backed up
- Use `--validate` periodically to check integrity

### Web Usage
- Use "Start Time Travel" for large date jumps
- Use speed control for smooth animations
- Enable orbit trails for educational purposes

### Scripting
- Always check return codes in scripts
- Use `--quiet` mode for automated processing
- Log outputs for debugging

### Performance
- Use appropriate application for your use case
- Monitor cache hit rates
- Profile performance for large-scale usage

This user guide provides comprehensive coverage of the Solar System Suite capabilities. For technical details, refer to the Developer Guide and API documentation.
